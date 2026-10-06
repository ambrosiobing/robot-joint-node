#!/usr/bin/env python3
"""The message memory layout, its arithmetic, and the vectors the C must match.

    python tools/gen_msgram.py              regenerate the vectors
    python tools/gen_msgram.py --check      fail if the committed ones differ
    python tools/gen_msgram.py --table      the section table for doc/
    python tools/gen_msgram.py --audit-chapter   against chapter 9's budget row

Chapter 9 says a controller whose message memory has not been laid out accepts
every other configuration, reports no error, and never transmits. That makes the
layout the single most useful thing in the chapter to get right, and the easiest
to get wrong silently, because nothing complains.

So the layout is computed here rather than written down, every section is
checked against the maximum number of elements the part will accept, and the
total is checked against the memory it has to fit in. Python, which is the
exception the house rule allows for orchestration; the node's own copy is C in
src/bus/msgram.c, and both are checked against test/vectors_msgram.json rather
than against each other.

The constants are not invented. The element sizes and the element-count maxima
are the silicon vendor's own, from the HAL driver source rather than from a
summary of it:

    SRAMCAN_FLS_NBR  128    SRAMCAN_FLS_SIZE  1 * 4 bytes
    SRAMCAN_FLE_NBR   64    SRAMCAN_FLE_SIZE  2 * 4 bytes
    SRAMCAN_RF0_NBR   64    SRAMCAN_RF1_NBR   64
    SRAMCAN_RB_NBR    64    SRAMCAN_TEF_NBR   32
    SRAMCAN_TFQ_NBR   32

A receive or transmit element carrying the full sixty-four byte payload is
eighteen words: two of header and sixteen of data. A transmit event element is
two words. Those follow from the element layouts rather than from a define.
"""
import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_msgram.json"
VECTORS_H = HERE / "test" / "vectors_msgram.h"

WORD = 4

# Bytes per element of each section, for a sixty-four byte payload.
ELEMENT_BYTES = {
    "std_filters": 1 * WORD,
    "ext_filters": 2 * WORD,
    "rx_fifo0": 18 * WORD,
    "rx_fifo1": 18 * WORD,
    "rx_buffers": 18 * WORD,
    "tx_event": 2 * WORD,
    "tx_buffers": 18 * WORD,
}

# The most elements the part will accept in each section.
MAX_ELEMENTS = {
    "std_filters": 128,
    "ext_filters": 64,
    "rx_fifo0": 64,
    "rx_fifo1": 64,
    "rx_buffers": 64,
    "tx_event": 32,
    "tx_buffers": 32,
}

# The message memory this part has, shared between its controller instances.
# 10 Kbytes, which is 2560 words.
MSGRAM_SIZE_BYTES = 10 * 1024

SECTION_ORDER = ["std_filters", "ext_filters", "rx_fifo0", "rx_fifo1",
                 "rx_buffers", "tx_event", "tx_buffers"]

# Chapter 9 step 3's layout, exactly as the chapter prints it.
LAYOUT = {
    "std_filters": 8,
    "ext_filters": 4,
    "rx_fifo0": 16,
    "rx_fifo1": 8,
    "rx_buffers": 0,
    "tx_event": 8,
    "tx_buffers": 8,
}

# What chapter 9's budget table claims, for the audit below.
#
# This was "under 2 kB" until Tuesday 6 October 2026, and it was wrong by 384
# bytes. The layout in step 3 is legal and unchanged: every section is inside
# the counts the part accepts, and it uses 2432 of the 10240 bytes available,
# so nothing about the design was at fault. The budget row was simply a round
# number that nobody had multiplied out. Sixteen receive elements of eighteen
# words each are 1152 bytes on their own.
CHAPTER_BUDGET_BYTES = 2432


def compute(layout):
    """Section offsets and sizes, or a refusal with the reason."""
    problems = []
    for name in SECTION_ORDER:
        n = layout[name]
        if n < 0:
            problems.append(f"{name}: {n} elements is not a count")
        elif n > MAX_ELEMENTS[name]:
            problems.append(f"{name}: {n} elements, and the part accepts at "
                            f"most {MAX_ELEMENTS[name]}")

    sections, offset = [], 0
    for name in SECTION_ORDER:
        size = layout[name] * ELEMENT_BYTES[name]
        sections.append({
            "name": name,
            "elements": layout[name],
            "element_bytes": ELEMENT_BYTES[name],
            "bytes": size,
            "offset": offset,
        })
        offset += size

    total = offset
    if total > MSGRAM_SIZE_BYTES:
        problems.append(f"the layout needs {total} bytes and the part has "
                        f"{MSGRAM_SIZE_BYTES}")

    return {
        "ok": not problems,
        "problems": problems,
        "sections": sections,
        "total_bytes": total,
        "msgram_size_bytes": MSGRAM_SIZE_BYTES,
        "free_bytes": MSGRAM_SIZE_BYTES - total,
    }


# Layouts that have to be refused, each for a different reason, because a
# layout checker that has never refused anything has not been tested.
REFUSALS = [
    ("REFUSE, more standard filters than the part accepts",
     dict(LAYOUT, std_filters=129)),
    ("REFUSE, more transmit buffers than the part accepts",
     dict(LAYOUT, tx_buffers=33)),
    ("REFUSE, every section at its maximum overflows the memory",
     {"std_filters": 128, "ext_filters": 64, "rx_fifo0": 64, "rx_fifo1": 64,
      "rx_buffers": 64, "tx_event": 32, "tx_buffers": 32}),
]


def build():
    cases = [{"label": "chapter 9 step 3", "layout": LAYOUT,
              "result": compute(LAYOUT)}]
    for label, lay in REFUSALS:
        cases.append({"label": label, "layout": lay, "result": compute(lay)})
    return {
        "note": "Generated by tools/gen_msgram.py. Do not edit.",
        "element_bytes": ELEMENT_BYTES,
        "max_elements": MAX_ELEMENTS,
        "msgram_size_bytes": MSGRAM_SIZE_BYTES,
        "cases": cases,
    }


def as_header(doc):
    main = doc["cases"][0]["result"]
    lines = [
        "/* Generated by tools/gen_msgram.py. Do not edit. */",
        "#ifndef JOINT_MSGRAM_VECTORS_H",
        "#define JOINT_MSGRAM_VECTORS_H",
        "",
        "#include <stdint.h>",
        "",
        f"#define MSGRAM_EXPECT_TOTAL_BYTES {main['total_bytes']}u",
        f"#define MSGRAM_EXPECT_FREE_BYTES  {main['free_bytes']}u",
        "",
        "typedef struct {",
        "    const char *name;",
        "    uint32_t    elements;",
        "    uint32_t    element_bytes;",
        "    uint32_t    bytes;",
        "    uint32_t    offset;",
        "} msgram_vector_t;",
        "",
        "static const msgram_vector_t MSGRAM_VECTORS[] = {",
    ]
    for s in main["sections"]:
        lines.append(
            '    {{ "{n}", {e}u, {eb}u, {b}u, {o}u }},'.format(
                n=s["name"], e=s["elements"], eb=s["element_bytes"],
                b=s["bytes"], o=s["offset"]))
    lines += [
        "};",
        "",
        "#define MSGRAM_VECTOR_COUNT "
        "((int) (sizeof MSGRAM_VECTORS / sizeof MSGRAM_VECTORS[0]))",
        "",
        "#endif /* JOINT_MSGRAM_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    main = doc["cases"][0]["result"]
    rows = ["| Section | Elements | Bytes each | Bytes | Offset |",
            "|---|---|---|---|---|"]
    for s in main["sections"]:
        rows.append(f"| {s['name']} | {s['elements']} | {s['element_bytes']} | "
                    f"{s['bytes']} | {s['offset']} |")
    rows.append(f"| **total** | | | **{main['total_bytes']}** | |")
    rows.append(f"| of the part's | | | {main['msgram_size_bytes']} | "
                f"{main['free_bytes']} free |")
    return "\n".join(rows)


def audit(doc):
    main = doc["cases"][0]["result"]
    total = main["total_bytes"]
    print("audit against chapter 9's memory and timing budget")
    print(f"  message memory used       this tool {total:7d} B   "
          f"chapter {CHAPTER_BUDGET_BYTES:7d} B")
    if total == CHAPTER_BUDGET_BYTES:
        print(f"  agree, with {main['free_bytes']} bytes of the part's "
              f"{main['msgram_size_bytes']} left over")
        return 0
    print(f"  DISAGREE by {total - CHAPTER_BUDGET_BYTES:+d} B")
    print()
    print("  The budget row and this layout are the same arithmetic, so they")
    print("  cannot differ honestly: either the layout in step 3 moved and the")
    print("  budget row was not multiplied out again, or this tool's element")
    print("  sizes no longer match the part. Check the layout first.")
    return 1


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--table", action="store_true")
    ap.add_argument("--audit-chapter", action="store_true")
    a = ap.parse_args(argv)

    doc = build()
    js = json.dumps(doc, indent=2) + "\n"
    hd = as_header(doc)

    if a.table:
        print(table(doc))
        return 0
    if a.audit_chapter:
        return audit(doc)
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
        refused = sum(1 for c in doc["cases"] if not c["result"]["ok"])
        print(f"msgram vectors match the generator "
              f"({len(doc['cases'])} layouts, {refused} of them refused)")
        return 0

    VECTORS_JSON.parent.mkdir(parents=True, exist_ok=True)
    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    refused = sum(1 for c in doc["cases"] if not c["result"]["ok"])
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} layouts, {refused} of them refused")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
