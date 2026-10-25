#!/usr/bin/env python3
"""The baud divisor, against its vectors and against its invariants.

    python test/test_baud.py

The vector comparison catches drift. The rules below catch arithmetic that is
internally wrong, which regenerated vectors would agree with perfectly, and the
one that matters most is the last: every refusal reason has to be reachable.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_baud as g                                             # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_baud.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    accepted = refused = 0

    for c in doc["cases"]:
        r = g.solve(c["fck_hz"], c["baud"])
        if r["ok"] != c["ok"]:
            fail(f"{c['label']}: solved is {r['ok']} and the file says "
                 f"{c['ok']}")
            continue
        if not r["ok"]:
            refused += 1
            if not c["label"].startswith("REFUSE"):
                fail(f"{c['label']}: refused but not labelled as a refusal")
            continue
        accepted += 1
        if c["label"].startswith("REFUSE"):
            fail(f"{c['label']}: labelled a refusal and accepted")
        for field in ("presc_index", "usartdiv", "brr", "actual_baud"):
            if r[field] != c[field]:
                fail(f"{c['label']}: {field} is {r[field]} and the file says "
                     f"{c[field]}")

        # The achieved rate, recomputed from the REGISTER WORD rather than
        # carried along. A chain that reports the rate it meant to produce
        # instead of the one the register gives would agree with itself.
        pres_clk = g.round_closest(c["fck_hz"], g.PRESC_VAL[r["presc_index"]])
        from_brr = g.round_closest(pres_clk, r["brr"])
        if from_brr != r["actual_baud"]:
            fail(f"{c['label']}: the register word gives {from_brr} and the "
                 f"case says {r['actual_baud']}")

        # And the error, recomputed here against the limit.
        err = abs(from_brr - c["baud"]) * 1000 / c["baud"]
        if err > doc["error_limit_permille"]:
            fail(f"{c['label']}: the error is {err:.1f} per thousand and the "
                 f"limit is {doc['error_limit_permille']}")

    # ---------------------------------------------------------- the invariants

    # 1. AT 16 TIMES OVERSAMPLING THE REGISTER WORD IS THE DIVISOR. The driver
    # composes it as a mantissa shifted by four with the remainder in the low
    # four bits, which for a divide by sixteen is an identity. Asserted over a
    # sweep rather than taken from the comment that says so, because if it ever
    # stops holding the whole file is wrong in a way no single case shows.
    for d in list(range(16, 4096)) + [0xFFF0, 0xFFFF]:
        if g.brr_from_div(d) != d:
            fail(f"a divisor of {d} composes to {g.brr_from_div(d)}, and at 16 "
                 f"times oversampling the two are the same number")
            break

    # 2. Rounding is to NEAREST, and that choice has to matter somewhere or it
    # is not worth stating. Both halves are checked: the function never differs
    # from the true quotient by more than half, and there is at least one real
    # case where truncating would give a different divisor.
    for n, d in ((115200, 1), (64000000, 115200), (8000000, 115200),
                 (1650000, 100000), (7, 2), (5, 2)):
        got = g.round_closest(n, d)
        if abs(got - n / d) > 0.5:
            fail(f"round_closest({n}, {d}) is {got} and {n / d:.3f} rounds to "
                 f"{round(n / d)}")
    differs = [(n, d) for n, d in ((64000000, 115200), (8000000, 115200))
               if g.round_closest(n, d) != n // d]
    if not differs:
        fail("truncating would give the same answer as rounding on every case "
             "here, so the rounding rule is untested and the comment claiming "
             "it matters is unsupported")

    # 3. A bigger clock at the same baud needs a bigger divisor, and the same
    # clock at a bigger baud needs a smaller one. A solver that had the division
    # inverted would still produce plausible looking register words.
    prev = 0
    for fck in (16000000, 32000000, 64000000, 100000000, 120000000):
        r = g.solve(fck, doc["console_baud"])
        if not r["ok"]:
            fail(f"{fck} Hz at {doc['console_baud']} was refused")
            continue
        if r["usartdiv"] <= prev:
            fail(f"at {fck} Hz the divisor is {r['usartdiv']} and at the next "
                 f"lower clock it was {prev}, so the division is the wrong way "
                 f"round")
        prev = r["usartdiv"]

    # 4. Every accepted solve anywhere in a sweep obeys all three limits. The
    # cases cover the rates this board might use; this covers the shape.
    swept = 0
    for fck in range(1000000, 200000001, 1000000):
        for baud in (9600, 115200, 1000000):
            r = g.solve(fck, baud)
            swept += 1
            if not r["ok"]:
                continue
            if r["usartdiv"] < doc["usartdiv_min"]:
                fail(f"{baud} from {fck} Hz was accepted with a divisor of "
                     f"{r['usartdiv']}, under the floor of "
                     f"{doc['usartdiv_min']}")
            if r["brr"] > doc["brr_max"]:
                fail(f"{baud} from {fck} Hz was accepted with a register word "
                     f"of 0x{r['brr']:X}, past sixteen bits")
            if r["error_permille"] > doc["error_limit_permille"]:
                fail(f"{baud} from {fck} Hz was accepted at "
                     f"{r['error_permille']} per thousand, past the limit")
            if r["brr"] != r["usartdiv"]:
                fail(f"{baud} from {fck} Hz: the register word and the divisor "
                     f"differ at 16 times oversampling")

    # 5. The prescaler chosen is the smallest that works, because dividing the
    # clock first throws away resolution and a larger prescaler at the same rate
    # gives a coarser divisor.
    for fck in (16000000, 64000000, 120000000):
        r = g.solve(fck, doc["console_baud"])
        if not r["ok"]:
            continue
        for smaller in range(r["presc_index"]):
            presc = g.PRESC_VAL[smaller]
            d = g.round_closest(g.round_closest(fck, presc),
                                doc["console_baud"])
            if d is not None and d >= doc["usartdiv_min"] \
                    and g.brr_from_div(d) <= doc["brr_max"]:
                fail(f"at {fck} Hz the solver took prescaler "
                     f"{r['presc']} and {presc} would also have worked, which "
                     f"gives a finer divisor")

    # 6. EVERY REFUSAL REASON MUST BE REACHED BY A CASE. This rule exists
    # because two of them were not: the error limit was written down and the
    # case meant to trip it was refused by the under-16 rule first, and the
    # exhaustion path at the end of the loop had no case at all. A refusal
    # nothing reaches is a claim about behaviour and not behaviour.
    reasons = {
        "not a rate": "a clock or a baud of zero",
        "8 times oversampling": "a divisor under the floor",
        "per thousand and the limit is": "an error past the limit",
        "inside 16 bits": "a divisor past sixteen bits with every prescaler",
    }
    seen = [c.get("why", "") for c in doc["cases"] if not c["ok"]]
    for needle, what in reasons.items():
        if not any(needle in why for why in seen):
            fail(f"no case reaches the refusal for {what}, so that path is "
                 f"written down and never exercised")

    if failures:
        print(f"test_baud: {len(failures)} failure(s)")
        return 1
    print(f"ok  {len(doc['cases'])} rates, {accepted} solved and {refused} "
          f"refused, {swept} swept, the register word is the divisor "
          f"throughout, every refusal reason reached")
    return 0


if __name__ == "__main__":
    sys.exit(main())
