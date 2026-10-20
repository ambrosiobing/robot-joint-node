#!/usr/bin/env python3
"""From an element index to the place that element lives, and the vectors.

    python tools/gen_msgaddr.py            regenerate the vectors
    python tools/gen_msgaddr.py --check    fail if the committed ones differ
    python tools/gen_msgaddr.py --table    print the table for doc/message-memory.md

Chapter 9 stage two. gen_msgram.py says where each section starts and how big
it is. gen_element.py says what the words of one element mean. Neither says
where element seven of the transmit buffers is, and every access to the message
memory needs exactly that.

It is a multiply and an add, so the question is why it is a file. Three answers,
and the layout this chapter configures contains the first one:

  - AN EMPTY SECTION HAS A PERFECTLY GOOD OFFSET. This node configures no
    dedicated receive buffers, so that section begins exactly where the
    transmit event section begins. Ask for receive buffer 0 and the multiply
    and add return that offset without complaint: write there and the first
    transmit event is overwritten, read there and an event comes back shaped
    like a frame. Nothing reports either.

  - THE SECTIONS DO NOT SHARE AN ELEMENT SIZE. One word, two words, eighteen
    words, depending on the section. The wrong size gives an address partway
    through a neighbouring element.

  - AN OFFSET IS NOT AN ADDRESS. The start-address registers hold offsets from
    the message memory base; a pointer needs the base added. Both numbers look
    reasonable and they are never equal. The register fields are where this
    chapter got that wrong the first time.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_msgram as mr                                          # noqa: E402

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_msgaddr.json"
VECTORS_H = HERE / "test" / "vectors_msgaddr.h"

# The C enumerates the sections in the order the memory holds them, and the
# order is load bearing: a section's offset is the sum of every section before
# it. The names here are gen_msgram's, and the index into this list is the
# enumerator's value in msgaddr.h. The test checks the two agree.
SECTIONS = list(mr.SECTION_ORDER)

# This controller instance's message memory, from the register map in
# doc/node-registers.md. [datasheet] RM0455, the FDCAN1 message RAM base.
MSGRAM_BASE = 0x4000AC00


def section_index(name):
    return SECTIONS.index(name)


def offset_of(layout, name, index):
    """The offset from the message memory base, or None when refused.

    The reference for the C. Every refusal below is an access that would
    otherwise land on memory belonging to a different section.
    """
    if name not in SECTIONS:
        return None
    m = mr.compute(layout)
    sec = next(s for s in m["sections"] if s["name"] == name)

    elem = sec["element_bytes"]
    if elem == 0:
        return None
    # An empty section refuses index 0 as well. This is the one that matters:
    # its offset is the offset of the section that follows it.
    if sec["elements"] == 0 or index >= sec["elements"] or index < 0:
        return None
    # The element must lie wholly inside the span the map gave the section. The
    # count and the span are computed separately, so comparing them is what
    # catches one of the two drifting.
    if (index + 1) * elem > sec["bytes"]:
        return None
    return sec["offset"] + index * elem


def address_of(layout, name, index, base):
    """The absolute address, or None when refused."""
    off = offset_of(layout, name, index)
    if off is None:
        return None
    # A base of zero is what a forgotten argument looks like, and the message
    # memory of this part is not at zero.
    if base == 0:
        return None
    if off > 0xFFFFFFFF - base:
        return None
    return base + off


# Each case names what it is for. The accepted ones prove the arithmetic; the
# refused ones are the whole point of the file.
CASES = [
    # (label, section, index, base)
    ("the first standard filter, where the memory begins",
     "std_filters", 0, MSGRAM_BASE),
    ("the last standard filter of the eight configured",
     "std_filters", 7, MSGRAM_BASE),
    ("the first extended filter, which is two words per element",
     "ext_filters", 0, MSGRAM_BASE),
    ("the last extended filter of the four configured",
     "ext_filters", 3, MSGRAM_BASE),
    ("the first element of receive FIFO 0, where the frames start",
     "rx_fifo0", 0, MSGRAM_BASE),
    ("the last element of receive FIFO 0, sixteen in",
     "rx_fifo0", 15, MSGRAM_BASE),
    ("the first element of receive FIFO 1",
     "rx_fifo1", 0, MSGRAM_BASE),
    ("the first transmit event, two words, right after the empty section",
     "tx_event", 0, MSGRAM_BASE),
    ("the last transmit event of the eight configured",
     "tx_event", 7, MSGRAM_BASE),
    ("the first transmit buffer, which is what step 4 writes",
     "tx_buffers", 0, MSGRAM_BASE),
    ("the last transmit buffer of the eight configured",
     "tx_buffers", 7, MSGRAM_BASE),
    # The refusals.
    ("REFUSE, dedicated receive buffer 0, a section with no elements, whose "
     "offset is the first transmit event and looks entirely reasonable",
     "rx_buffers", 0, MSGRAM_BASE),
    ("REFUSE, dedicated receive buffer 1, the same section one further in",
     "rx_buffers", 1, MSGRAM_BASE),
    ("REFUSE, standard filter 8 of the eight configured, which is the first "
     "extended filter",
     "std_filters", 8, MSGRAM_BASE),
    ("REFUSE, transmit buffer 8 of the eight configured, which is past the "
     "end of the whole layout",
     "tx_buffers", 8, MSGRAM_BASE),
    ("REFUSE, receive FIFO 0 element 16 of the sixteen configured",
     "rx_fifo0", 16, MSGRAM_BASE),
    ("REFUSE, an index the part itself would never produce",
     "tx_buffers", 0xFFFFFFFF, MSGRAM_BASE),
    ("REFUSE, a base of zero, which is what a forgotten argument looks like",
     "tx_buffers", 0, 0),
    ("REFUSE, a base so high that the sum is not an address",
     "tx_buffers", 0, 0xFFFFFFF0),
]


def build():
    layout = mr.LAYOUT
    m = mr.compute(layout)
    out = []
    for label, name, index, base in CASES:
        off = offset_of(layout, name, index)
        addr = address_of(layout, name, index, base)
        out.append({
            "label": label,
            "section": name,
            "section_index": section_index(name),
            "index": index,
            "base": base,
            "elements": layout[name],
            "element_bytes": mr.ELEMENT_BYTES[name],
            # Two separate answers, because a case can have a legal offset and
            # no legal address: the two base refusals below are exactly that.
            "offset_ok": off is not None,
            "offset": off if off is not None else 0,
            "address_ok": addr is not None,
            "address": addr if addr is not None else 0,
        })
    return {
        "note": "Generated by tools/gen_msgaddr.py. Do not edit.",
        "msgram_base": MSGRAM_BASE,
        "sections": SECTIONS,
        "layout": dict(layout),
        "map": {s["name"]: {"offset": s["offset"], "bytes": s["bytes"],
                            "elements": s["elements"],
                            "element_bytes": s["element_bytes"]}
                for s in m["sections"]},
        "total_bytes": m["total_bytes"],
        "cases": out,
    }


def as_header(doc):
    lines = [
        "/* Generated by tools/gen_msgaddr.py. Do not edit. */",
        "#ifndef JOINT_MSGADDR_VECTORS_H",
        "#define JOINT_MSGADDR_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        f"#define MA_MSGRAM_BASE 0x{doc['msgram_base']:08X}u",
        "",
        "typedef struct {",
        "    const char *label;",
        "    const char *section_name;",
        "    uint32_t    section;      /* the msgram_section_t enumerator */",
        "    uint32_t    index;",
        "    uint32_t    base;",
        "    uint32_t    elements;     /* what the layout configured */",
        "    uint32_t    element_bytes;",
        "    bool        offset_ok;",
        "    uint32_t    offset;",
        "    bool        address_ok;",
        "    uint32_t    address;",
        "} ma_vector_t;",
        "",
        "static const ma_vector_t MA_VECTORS[] = {",
    ]
    for c in doc["cases"]:
        lines.append(
            '    {{ "{label}", "{sec}", {si}u, {idx}u, 0x{base:08X}u, {n}u, '
            "{eb}u, {ook}, 0x{off:08X}u, {aok}, 0x{addr:08X}u }},".format(
                label=c["label"], sec=c["section"], si=c["section_index"],
                idx=c["index"], base=c["base"], n=c["elements"],
                eb=c["element_bytes"],
                ook="true" if c["offset_ok"] else "false", off=c["offset"],
                aok="true" if c["address_ok"] else "false", addr=c["address"]))
    lines += [
        "};",
        "",
        "#define MA_VECTOR_COUNT "
        "((int) (sizeof MA_VECTORS / sizeof MA_VECTORS[0]))",
        "",
        "/* The sections in the order the memory holds them, so the test can "
        "check that",
        " * the C enumerator and the generator's list have not drifted apart. "
        "*/",
        "static const char *const MA_SECTION_NAMES[] = {",
    ]
    for name in doc["sections"]:
        lines.append(f'    "{name}",')
    lines += [
        "};",
        "",
        "#define MA_SECTION_COUNT "
        "((int) (sizeof MA_SECTION_NAMES / sizeof MA_SECTION_NAMES[0]))",
        "",
        "#endif /* JOINT_MSGADDR_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    rows = ["| Case | Section | Elements | Bytes each | Offset | Address |",
            "|---|---|---|---|---|---|"]
    for c in doc["cases"]:
        off = (f"`0x{c['offset']:04X}`" if c["offset_ok"] else "refused")
        addr = (f"`0x{c['address']:08X}`" if c["address_ok"] else "refused")
        rows.append(
            f"| {c['label']} | {c['section']} | {c['elements']} | "
            f"{c['element_bytes']} | {off} | {addr} |")
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
        refused = sum(1 for c in doc["cases"] if not c["address_ok"])
        print(f"msgaddr vectors match the generator ({len(doc['cases'])} "
              f"accesses, {refused} of them refused)")
        return 0

    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    refused = sum(1 for c in doc["cases"] if not c["address_ok"])
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} accesses, {refused} of them refused")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
