#!/usr/bin/env python3
"""The console's baud divisor, its error, and the vectors the C must reproduce.

    python tools/gen_baud.py            regenerate the vectors
    python tools/gen_baud.py --check    fail if the committed ones differ
    python tools/gen_baud.py --table    print the table for doc/console.md

Chapter 4's image floor, third piece. The map says where things go and the
startup gets the part to main; this is the arithmetic the console needs before
it can put a character anywhere.

IT IS THE SAME SHAPE AS CHAPTER 9'S BIT TIMING AND IT FAILS THE SAME WAY. A
divisor that is wrong by a lot produces obvious rubbish on the terminal, which
is a good failure. A divisor that is wrong by two or three per cent produces
output that is mostly readable with occasional wrong characters, and the natural
reading of that is a loose wire or a flaky adapter rather than arithmetic.

AND A CONSOLE CANNOT CHECK ITS OWN BAUD. Chapter 10's lesson about a serial
loopback applies here in a harder form: a loopback shares its clock with itself
and so passes at any rate. This console has no loopback at all; it writes into a
transmitter and never hears back. So the only evidence available is that the
characters a PC at a known fixed rate receives are the characters that were
sent, which makes the TERMINAL the instrument and the board the thing measured.
That is why the first image has to print the clock it assumed and the divisor it
computed: legible output then confirms the assumption, and illegible output says
which assumption to doubt.

THE KERNEL CLOCK IS NOT KNOWN YET, and this file does not pretend otherwise.
`[unconfirmed]` The USART3 kernel clock comes from a selector whose reset value
is the APB1 peripheral clock, and what that is after reset on this part has not
been read. Chapter 9 found that this part's clock selector registers differ in
NAME from the better documented sibling's, so the sibling's reset values are not
evidence here either. The table below therefore solves every plausible rate
rather than picking one, so that whichever the board turns out to run at, the
row is already computed and the error is already known.

`[kernel]` The arithmetic is from the mainline Linux driver,
drivers/tty/serial/stm32-usart.c and stm32-usart.h, read on Sunday
25 October 2026:

    usartdiv = DIV_ROUND_CLOSEST(uart_clk / presc, baud)   rounded, not cut
    usartdiv < 16  requires 8 times oversampling, CR1.OVER8
    otherwise      16 times oversampling, which this chapter uses
    brr = ((usartdiv / 16) << 4) | (usartdiv % 16)
    USART_BRR_DIV_M_SHIFT is 4, so at 16 times that expression IS usartdiv
    presc values are 1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256

`[vendor]` USART3 is at 0x40004800 with a 0x400 window, its clock is APB1 bit 18
and GPIOD's is AHB4 bit 3, from Zephyr's dts/arm/st/h7/stm32h7.dtsi. The enable
registers are RCC + 0x0E8 and RCC + 0x0E0 with RCC at 0x58024400, from NuttX's
arch/arm/src/stm32h7/hardware/stm32h7x3xx_rcc.h, which NuttX itself uses for the
H7B3, this part's own family, rather than only for the H743.
"""
import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_baud.json"
VECTORS_H = HERE / "test" / "vectors_baud.h"

# `[kernel]` stm32_usart_presc_val in drivers/tty/serial/stm32-usart.c, and
# USART_PRESC_MAX is 0b1011, which is the last index of this list.
PRESC_VAL = [1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256]

# `[kernel]` Below 16 the driver switches to 8 times oversampling. This chapter
# uses 16 times only, so 16 is a floor and not a preference: a divisor under it
# needs a mode this console refuses to enter.
USARTDIV_MIN = 16
BRR_MAX = 0xFFFF

# The error this chapter will accept, and it is a CHOICE rather than a reading.
#
# `[arithmetic]` An 8N1 frame is ten bit times. The receiver samples each bit at
# its middle, so the last bit's sample drifts by nine and a half bit times of
# accumulated error; half a bit of margin therefore allows about 5 per cent in
# total. Both ends contribute, and the receiver's own tolerance is unknown here,
# so half of that is the limit taken: 25 per thousand. A divisor inside it is
# accepted; outside it is refused, because output that is mostly readable is the
# failure this file exists to prevent.
ERROR_LIMIT_PERMILLE = 25

CONSOLE_BAUD = 115200       # `[vendor]` the board's own device tree


def round_closest(n, d):
    """Rounded to nearest, which is what the driver does. Truncating instead is
    a silent error source: at 64 MHz and 115200 it changes the divisor from 556
    to 555 and the error from 0.1 to 1.9 per thousand."""
    if d <= 0:
        return None
    return (n + d // 2) // d


def brr_from_div(usartdiv):
    """The register word. At 16 times oversampling this equals usartdiv, and the
    test asserts that rather than taking this comment's word for it."""
    return ((usartdiv // 16) << 4) | (usartdiv % 16)


def solve(fck_hz, baud, limit_permille=ERROR_LIMIT_PERMILLE):
    """The smallest prescaler that lands a legal divisor, or a refusal.

    Returns a dict, with `ok` False and a `why` when it cannot be done. The
    smallest prescaler is preferred because dividing the clock first throws away
    resolution: the same baud reached through a larger prescaler has a coarser
    divisor and therefore a larger error.
    """
    if fck_hz <= 0 or baud <= 0:
        return {"ok": False, "why": "a clock or a baud of zero is not a rate"}

    for index, presc in enumerate(PRESC_VAL):
        pres_clk = round_closest(fck_hz, presc)
        usartdiv = round_closest(pres_clk, baud)
        if usartdiv is None or usartdiv < USARTDIV_MIN:
            # Too small: this would need 8 times oversampling, which this
            # chapter does not enter. A larger prescaler makes it smaller still,
            # so there is no point continuing.
            return {"ok": False,
                    "why": f"the divisor is {usartdiv} and below "
                           f"{USARTDIV_MIN} the part needs 8 times "
                           f"oversampling, which this console refuses"}
        brr = brr_from_div(usartdiv)
        if brr > BRR_MAX:
            continue        # a larger prescaler may bring it into range
        actual = round_closest(pres_clk, usartdiv)
        err = abs(actual - baud) * 1000
        err_permille = err / baud
        if err_permille > limit_permille:
            return {"ok": False,
                    "why": f"the achievable rate is {actual} against a wanted "
                           f"{baud}, which is {err_permille:.1f} per thousand "
                           f"and the limit is {limit_permille}"}
        return {"ok": True, "presc_index": index, "presc": presc,
                "usartdiv": usartdiv, "brr": brr, "actual_baud": actual,
                "error_permille": round(err_permille, 3), "why": ""}

    return {"ok": False,
            "why": f"no prescaler brings the divisor inside 16 bits"}


# Every kernel clock this part might plausibly be running USART3 from, because
# which one it IS has not been read. When the board says, the row is already
# here. The 8 MHz entry is the HSE the board fits, which chapter 9 established
# is the only PLL-free source for the bus; 64 MHz is the HSI this family starts
# on; the rest are the usual bus rates.
CANDIDATE_CLOCKS = [
    (8000000, "the 8 MHz HSE this board fits, which chapter 9 uses for the bus"),
    (16000000, "a plausible divided bus rate"),
    (32000000, "a plausible divided bus rate"),
    (64000000, "the HSI this family comes out of reset on"),
    (100000000, "a plausible APB1 rate once the PLL is configured"),
    (120000000, "a plausible APB1 rate once the PLL is configured"),
]

# The refusals, each for a different reason, because a solver that only ever
# succeeds says nothing about what it will not do.
REFUSAL_CASES = [
    ("REFUSE, a kernel clock of zero", 0, CONSOLE_BAUD),
    ("REFUSE, a baud of zero", 64000000, 0),
    ("REFUSE, 115200 from a 1 MHz clock, whose divisor is under 16 and would "
     "need the 8 times oversampling this console does not enter",
     1000000, CONSOLE_BAUD),
    # This one was chosen so the divisor clears 16 and the ROUNDING is what
    # fails. The first attempt at it, 3 Mbit from 8 MHz, was refused by the
    # under-16 rule instead, which left the error limit written down and never
    # exercised: a refusal reason nothing reaches is a claim and not a
    # behaviour. 1.65 MHz over 100 kbit gives a divisor of 17 and an achievable
    # rate of 97059, which is 29.4 per thousand against a limit of 25.
    ("REFUSE, 100 kbit from 1.65 MHz, where the divisor is legal at 17 and the "
     "rounding error alone is past the limit",
     1650000, 100000),
    # And this reaches the exhaustion path: every prescaler leaves a divisor
    # past sixteen bits, so the loop runs out rather than any single rule
    # firing. Without it the function's last return was unreachable.
    ("REFUSE, 1 baud from 500 MHz, where even the 256 prescaler leaves the "
     "divisor past sixteen bits",
     500000000, 1),
]


def build():
    rows = []
    for fck, note in CANDIDATE_CLOCKS:
        r = solve(fck, CONSOLE_BAUD)
        rows.append({"label": f"{CONSOLE_BAUD} baud from {fck} Hz",
                     "note": note, "fck_hz": fck, "baud": CONSOLE_BAUD, **r})
    for label, fck, baud in REFUSAL_CASES:
        r = solve(fck, baud)
        rows.append({"label": label, "note": "", "fck_hz": fck, "baud": baud,
                     **r})
    return {
        "note": "Generated by tools/gen_baud.py. Do not edit.",
        "console_baud": CONSOLE_BAUD,
        "usartdiv_min": USARTDIV_MIN,
        "brr_max": BRR_MAX,
        "error_limit_permille": ERROR_LIMIT_PERMILLE,
        "presc_val": PRESC_VAL,
        "cases": rows,
    }


def as_header(doc):
    lines = [
        "/* Generated by tools/gen_baud.py. Do not edit. */",
        "#ifndef JOINT_BAUD_VECTORS_H",
        "#define JOINT_BAUD_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        f"#define BV_CONSOLE_BAUD {doc['console_baud']}u",
        f"#define BV_USARTDIV_MIN {doc['usartdiv_min']}u",
        f"#define BV_BRR_MAX 0x{doc['brr_max']:04X}u",
        f"#define BV_ERROR_LIMIT_PERMILLE {doc['error_limit_permille']}u",
        "",
        "/* The prescaler values, in index order, from the mainline driver. */",
        "static const uint32_t BV_PRESC_VAL[] = { "
        + ", ".join(f"{v}u" for v in doc["presc_val"]) + " };",
        "",
        "typedef struct {",
        "    const char *label;",
        "    uint32_t    fck_hz;",
        "    uint32_t    baud;",
        "    bool        ok;",
        "    uint32_t    presc_index;",
        "    uint32_t    usartdiv;",
        "    uint32_t    brr;",
        "    uint32_t    actual_baud;",
        "} bv_vector_t;",
        "",
        "static const bv_vector_t BV_VECTORS[] = {",
    ]
    for c in doc["cases"]:
        lines.append(
            '    {{ "{label}", {fck}u, {baud}u, {ok}, {pi}u, {div}u, '
            "0x{brr:04X}u, {act}u }},".format(
                label=c["label"], fck=c["fck_hz"], baud=c["baud"],
                ok="true" if c["ok"] else "false",
                pi=c.get("presc_index", 0), div=c.get("usartdiv", 0),
                brr=c.get("brr", 0), act=c.get("actual_baud", 0)))
    lines += [
        "};",
        "",
        "#define BV_VECTOR_COUNT "
        "((int) (sizeof BV_VECTORS / sizeof BV_VECTORS[0]))",
        "",
        "#endif /* JOINT_BAUD_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    rows = ["| Case | Kernel clock | Prescaler | USARTDIV | BRR | Achieved | "
            "Error | Why it is in the table |",
            "|---|---|---|---|---|---|---|---|"]
    for c in doc["cases"]:
        if not c["ok"]:
            rows.append(f"| {c['label']} | {c['fck_hz']} Hz | refused | "
                        f"refused | refused | refused | refused | "
                        f"{c['why']} |")
            continue
        rows.append(
            f"| {c['label']} | {c['fck_hz']} Hz | {c['presc']} | "
            f"{c['usartdiv']} | `0x{c['brr']:04X}` | {c['actual_baud']} | "
            f"{c['error_permille']} per thousand | {c['note']} |")
    return "\n".join(rows)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--table", action="store_true")
    a = ap.parse_args(argv)

    doc = build()
    js = json.dumps(doc, indent=2) + "\n"
    hd = as_header(doc)

    if a.table:
        print(table(doc))
        return 0

    refused = sum(1 for c in doc["cases"] if not c["ok"])
    if a.check:
        bad = 0
        for path, want in ((VECTORS_JSON, js), (VECTORS_H, hd)):
            have = path.read_text(encoding="utf-8") if path.exists() else ""
            if have.replace("\r\n", "\n") != want:
                print(f"{path.name} differs from the generator")
                bad += 1
        if bad:
            print("a hand edit does not survive here; regenerate instead")
            return 1
        print(f"baud vectors match the generator ({len(doc['cases'])} rates, "
              f"{refused} of them refused)")
        return 0

    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} rates, {refused} of them refused")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
