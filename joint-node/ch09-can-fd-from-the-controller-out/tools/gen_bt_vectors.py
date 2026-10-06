#!/usr/bin/env python3
"""The bit timing arithmetic, and the vectors the C must reproduce.

    python tools/gen_bt_vectors.py            regenerate the vectors
    python tools/gen_bt_vectors.py --check    fail if the committed ones differ
    python tools/gen_bt_vectors.py --table    print the table for doc/bit-timing.md

Chapter 9 asks for bit timing computed from the kernel clock with both phases
checked for exact division. This is the reference for that arithmetic. It is
Python, which is the exception the house rule allows for orchestration: the
node's own copy is C, in src/bus/bittiming.c, and the two are checked against
test/vectors.json rather than against each other, so when they disagree the
vector file says which case and which field.

There is no C compiler on the win11 aquamarine authoring laptop, so the C half
is never built where it is written. It is built in continuous integration, which
runs it against these same vectors.

The limits are not invented. They are the widths of the register fields that
hold these numbers on this part, read from the field definitions rather than
from a summary of them: a field of n bits holds 0 to 2^n - 1, and the hardware
adds one to the segment and prescaler fields, so a 9 bit prescaler field means a
prescaler of 1 to 512. The two phases have different widths, which is the whole
reason the limits are a parameter here rather than a constant.
"""
import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors.json"
VECTORS_H = HERE / "test" / "vectors.h"


class Limits:
    """One phase's register field widths, expressed as the values they hold."""

    def __init__(self, name, prescaler, seg1, seg2, sjw):
        self.name = name
        self.prescaler = prescaler      # (min, max)
        self.seg1 = seg1
        self.seg2 = seg2
        self.sjw = sjw

    # A bit divided into very few quanta is legal and useless: the sample point
    # can only land on one of a handful of positions, so it cannot be put where
    # the design wants it, and the jump width has almost nothing to work with.
    # This floor is a design choice of this volume, not a register limit and not
    # a quotation from the standard, which has not been read here. The register
    # limit is the three quanta below, and it is what the hardware enforces.
    QUALITY_FLOOR_TQ = 8

    @property
    def register_min_tq(self):
        # One quantum of synchronisation, plus the smallest of each segment.
        return 1 + self.seg1[0] + self.seg2[0]

    @property
    def min_tq(self):
        return max(self.register_min_tq, self.QUALITY_FLOOR_TQ)

    @property
    def max_tq(self):
        return 1 + self.seg1[1] + self.seg2[1]

    def as_dict(self):
        return {"name": self.name, "prescaler": list(self.prescaler),
                "seg1": list(self.seg1), "seg2": list(self.seg2),
                "sjw": list(self.sjw), "min_tq": self.min_tq,
                "max_tq": self.max_tq}


# The nominal phase carries the arbitration, where every node must agree, so its
# fields are wide. The data phase only has to survive one transmitter, so its
# fields are narrower and a data prescaler above 32 cannot be written at all.
NOMINAL = Limits("nominal", (1, 512), (1, 256), (1, 128), (1, 128))
DATA = Limits("data", (1, 32), (1, 32), (1, 16), (1, 16))


def bt_compute(kernel_hz, bitrate, want_sp_permille, lim):
    """Return the timing for one phase, or None if no exact solution exists.

    Exact means exact: the prescaler must divide the kernel clock with no
    remainder, and the resulting quantum rate must divide the bit rate the same
    way. A bit rate that is one part in a thousand out works between two nodes
    that are wrong in the same direction and fails against anything else, so an
    inexact answer is refused rather than rounded into place.
    """
    for pre in range(lim.prescaler[0], lim.prescaler[1] + 1):
        if kernel_hz % pre:
            continue                                   # inexact prescaler
        tq_hz = kernel_hz // pre
        if tq_hz % bitrate:
            continue                                   # inexact bit time
        tq_per_bit = tq_hz // bitrate
        if tq_per_bit < lim.min_tq or tq_per_bit > lim.max_tq:
            continue

        # Integer arithmetic, rounding half up, and no float anywhere in the
        # decision. Python rounds half to even and C rounds half away from zero,
        # so a float here would have made the two sides disagree on any case
        # that lands exactly on a half. One does: 75 per cent of 30 quanta is
        # 22.5, and the two languages would have chosen 22 and 23.
        before = (want_sp_permille * tq_per_bit + 500) // 1000
        if before == 0 or before > tq_per_bit:
            continue
        seg1 = before - 1                              # 1 tq is the sync
        seg2 = tq_per_bit - seg1 - 1
        if not (lim.seg1[0] <= seg1 <= lim.seg1[1]):
            continue
        if not (lim.seg2[0] <= seg2 <= lim.seg2[1]):
            continue

        return {
            "prescaler": pre,
            "tq_per_bit": tq_per_bit,
            "seg1": seg1,
            "seg2": seg2,
            "sjw": min(seg2, lim.sjw[1]),
            "sample_point_permille":
                ((1 + seg1) * 1000 + tq_per_bit // 2) // tq_per_bit,
        }
    return None


# The design points chapter 9 commits to in its budget table, and the cases that
# have to be refused. A calculator that has never refused anything is a
# calculator nobody has tested.
CASES = [
    # (label, kernel_hz, bitrate, want_sp, phase)
    ("design nominal, 80 MHz", 80_000_000, 500_000, 0.80, NOMINAL),
    ("design data, 80 MHz", 80_000_000, 2_000_000, 0.75, DATA),
    ("nominal at 100 MHz", 100_000_000, 500_000, 0.80, NOMINAL),
    ("data at 100 MHz", 100_000_000, 2_000_000, 0.75, DATA),
    ("nominal at 60 MHz", 60_000_000, 500_000, 0.80, NOMINAL),
    ("data at 60 MHz", 60_000_000, 2_000_000, 0.75, DATA),
    ("nominal at 40 MHz", 40_000_000, 500_000, 0.80, NOMINAL),
    ("data at 40 MHz", 40_000_000, 2_000_000, 0.75, DATA),
    ("nominal, 1 Mbit/s at 80 MHz", 80_000_000, 1_000_000, 0.80, NOMINAL),
    ("data, 5 Mbit/s at 80 MHz", 80_000_000, 5_000_000, 0.75, DATA),
    ("data, 8 Mbit/s at 80 MHz", 80_000_000, 8_000_000, 0.75, DATA),

    # Refusals. Each one fails for a different reason, and each was checked to
    # be a refusal rather than assumed to be: the first draft of this list had
    # three cases labelled REFUSE and only one of them refused.
    ("REFUSE, the bit rate does not divide the quantum rate",
     80_000_000, 666_667, 0.80, NOMINAL),
    ("REFUSE, no prescaler divides this kernel clock into this bit rate",
     33_000_000, 2_000_000, 0.75, DATA),
    ("REFUSE, 3 quanta a bit is legal but below this volume's floor",
     6_000_000, 2_000_000, 0.75, DATA),
    ("REFUSE, 2 quanta a bit is below what the registers can hold",
     80_000_000, 40_000_000, 0.75, DATA),
]


def build():
    out = []
    for label, kernel_hz, bitrate, want_sp, lim in CASES:
        r = bt_compute(kernel_hz, bitrate, round(want_sp * 1000), lim)
        out.append({
            "label": label,
            "kernel_hz": kernel_hz,
            "bitrate": bitrate,
            "want_sp_permille": round(want_sp * 1000),
            "phase": lim.name,
            "ok": r is not None,
            "result": r,
        })
    return {
        "note": "Generated by tools/gen_bt_vectors.py. Do not edit.",
        "limits": {"nominal": NOMINAL.as_dict(), "data": DATA.as_dict()},
        "cases": out,
    }


def as_header(doc):
    """The same vectors as a C header, so the C test needs no JSON parser."""
    lines = [
        "/* Generated by tools/gen_bt_vectors.py. Do not edit. */",
        "#ifndef JOINT_BT_VECTORS_H",
        "#define JOINT_BT_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        "typedef struct {",
        "    const char *label;",
        "    uint32_t    kernel_hz;",
        "    uint32_t    bitrate;",
        "    uint32_t    want_sp_permille;",
        "    bool        data_phase;   /* false: nominal */",
        "    bool        ok;",
        "    uint32_t    prescaler;",
        "    uint32_t    seg1;",
        "    uint32_t    seg2;",
        "    uint32_t    sjw;",
        "    uint32_t    sample_point_permille;",
        "} bt_vector_t;",
        "",
        "static const bt_vector_t BT_VECTORS[] = {",
    ]
    for c in doc["cases"]:
        r = c["result"] or {"prescaler": 0, "seg1": 0, "seg2": 0, "sjw": 0,
                            "sample_point_permille": 0}
        lines.append(
            '    {{ "{label}", {k}u, {b}u, {sp}u, {dp}, {ok}, '
            "{pre}u, {s1}u, {s2}u, {sjw}u, {asp}u }},".format(
                label=c["label"], k=c["kernel_hz"], b=c["bitrate"],
                sp=c["want_sp_permille"],
                dp="true" if c["phase"] == "data" else "false",
                ok="true" if c["ok"] else "false",
                pre=r["prescaler"], s1=r["seg1"], s2=r["seg2"], sjw=r["sjw"],
                asp=r["sample_point_permille"]))
    lines += [
        "};",
        "",
        "#define BT_VECTOR_COUNT "
        "((int) (sizeof BT_VECTORS / sizeof BT_VECTORS[0]))",
        "",
        "#endif /* JOINT_BT_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    rows = ["| Case | Phase | Kernel | Bit rate | Prescaler | tq | seg1 | seg2 | "
            "SJW | Sample point |",
            "|---|---|---|---|---|---|---|---|---|---|"]
    for c in doc["cases"]:
        if not c["ok"]:
            rows.append(f"| {c['label']} | {c['phase']} | "
                        f"{c['kernel_hz'] // 1_000_000} MHz | "
                        f"{c['bitrate'] / 1e6:g} Mbit/s | "
                        "refused | refused | refused | refused | refused | "
                        "no exact solution |")
            continue
        r = c["result"]
        rows.append(
            f"| {c['label']} | {c['phase']} | {c['kernel_hz'] // 1_000_000} MHz | "
            f"{c['bitrate'] / 1e6:g} Mbit/s | {r['prescaler']} | "
            f"{r['tq_per_bit']} | {r['seg1']} | {r['seg2']} | {r['sjw']} | "
            f"{r['sample_point_permille'] / 10:g} per cent |")
    return "\n".join(rows)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="fail if the committed vectors differ from these")
    ap.add_argument("--table", action="store_true",
                    help="print the table for doc/bit-timing.md")
    a = ap.parse_args(argv)

    doc = build()
    js = json.dumps(doc, indent=2) + "\n"
    hd = as_header(doc)

    if a.table:
        print(table(doc))
        return 0

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
        print(f"vectors match the generator ({len(doc['cases'])} cases, "
              f"{sum(1 for c in doc['cases'] if not c['ok'])} of them refusals)")
        return 0

    VECTORS_JSON.parent.mkdir(parents=True, exist_ok=True)
    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} cases, "
          f"{sum(1 for c in doc['cases'] if not c['ok'])} of them refusals")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
