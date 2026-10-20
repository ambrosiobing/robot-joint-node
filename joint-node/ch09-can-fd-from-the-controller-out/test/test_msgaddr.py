#!/usr/bin/env python3
"""Element addressing, checked against its vectors and against its invariants.

    python test/test_msgaddr.py

The vector comparison catches drift. The invariants below catch the ways this
arithmetic goes wrong while still returning a number that looks like an answer,
which is every way it goes wrong:

  - an empty section's offset is the next section's offset
  - the wrong element size lands partway through a neighbouring element
  - an offset used as an address, or an address as an offset
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_msgaddr as g                                          # noqa: E402
import gen_msgram as mr                                          # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_msgaddr.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def every_element(layout):
    """Every addressable element of a layout, as (section, index, offset)."""
    for name in g.SECTIONS:
        for index in range(layout[name]):
            off = g.offset_of(layout, name, index)
            if off is None:
                fail(f"{name} element {index} of {layout[name]} configured was "
                     f"refused, so a configured element has no address")
                continue
            yield name, index, off


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    layout = doc["layout"]
    m = doc["map"]
    accepted = refused = 0

    for c in doc["cases"]:
        off = g.offset_of(layout, c["section"], c["index"])
        addr = g.address_of(layout, c["section"], c["index"], c["base"])

        if (off is not None) != c["offset_ok"]:
            fail(f"{c['label']}: offset_ok is {off is not None}, the file says "
                 f"{c['offset_ok']}")
            continue
        if (addr is not None) != c["address_ok"]:
            fail(f"{c['label']}: address_ok is {addr is not None}, the file "
                 f"says {c['address_ok']}")
            continue
        if off is not None and off != c["offset"]:
            fail(f"{c['label']}: offset 0x{off:X} against the file's "
                 f"0x{c['offset']:X}")
        if addr is not None and addr != c["address"]:
            fail(f"{c['label']}: address 0x{addr:X} against the file's "
                 f"0x{c['address']:X}")

        if addr is None:
            refused += 1
        else:
            accepted += 1

        # An offset is not an address, and the difference is the base. A case
        # that lost the base would still return a plausible number.
        if off is not None and addr is not None:
            if addr - off != c["base"]:
                fail(f"{c['label']}: the address minus the offset is "
                     f"0x{addr - off:X} and the base is 0x{c['base']:X}")
            if addr == off:
                fail(f"{c['label']}: the address and the offset are the same "
                     f"number, so one of the two is being used for the other")

        # Labels and behaviour have to agree, or the table lies to a reader.
        if not c["address_ok"] and not c["label"].startswith("REFUSE"):
            fail(f"{c['label']}: refused but not labelled as a refusal")
        if c["address_ok"] and c["label"].startswith("REFUSE"):
            fail(f"{c['label']}: labelled a refusal and accepted")

    # ---------------------------------------------------------- the invariants

    # 1. The section order is the memory's order, and the C enumerates it in
    # the same order. A section's offset is the sum of every section before it,
    # so a reordering silently moves every section but the first.
    if g.SECTIONS != mr.SECTION_ORDER:
        fail(f"the addressing order {g.SECTIONS} is not the layout order "
             f"{mr.SECTION_ORDER}, and every offset after the first depends "
             f"on it")
    if doc["sections"] != mr.SECTION_ORDER:
        fail("the vector file's section order is not the layout order")

    # 2. No two different elements may share an offset, and no element may
    # overlap another. This is the generic form of the empty-section defect and
    # of every wrong element size: both produce an offset that belongs to
    # something else, and both are invisible at the call site.
    spans = []
    for name, index, off in every_element(layout):
        spans.append((off, off + m[name]["element_bytes"], name, index))
    spans.sort()
    for (a_lo, a_hi, a_name, a_i), (b_lo, b_hi, b_name, b_i) in zip(
            spans, spans[1:]):
        if a_hi > b_lo:
            fail(f"{a_name} element {a_i} runs from 0x{a_lo:X} to 0x{a_hi:X} "
                 f"and {b_name} element {b_i} begins at 0x{b_lo:X}, so two "
                 f"elements share memory")
    if len(set(s[0] for s in spans)) != len(spans):
        fail("two elements have the same offset")

    # 3. Every element lies wholly inside its own section, and the last element
    # of the last section ends exactly at the total. A layout that tiles with a
    # hole wastes memory silently; one that tiles with an overlap corrupts.
    for name, index, off in every_element(layout):
        sec = m[name]
        if off < sec["offset"] or \
                off + sec["element_bytes"] > sec["offset"] + sec["bytes"]:
            fail(f"{name} element {index} at 0x{off:X} is not inside its own "
                 f"section, which runs 0x{sec['offset']:X} for "
                 f"{sec['bytes']} bytes")
    if spans and spans[-1][1] != doc["total_bytes"]:
        fail(f"the last element ends at 0x{spans[-1][1]:X} and the layout "
             f"totals 0x{doc['total_bytes']:X}, so the sections do not tile")

    # 4. THE DEFECT, named rather than implied. Every section the layout gives
    # no elements shares its offset with whatever follows it, so index 0 of an
    # empty section is a real offset pointing at somebody else's first element.
    # The refusal is what makes that safe, and this rule proves the collision
    # exists rather than assuming it does.
    empty = [name for name in g.SECTIONS if layout[name] == 0]
    if not empty:
        fail("no section in this layout is empty, so the case this file exists "
             "for is not reachable from the layout under test and the refusal "
             "is untested")
    for name in empty:
        after = [s for s in g.SECTIONS[g.SECTIONS.index(name) + 1:]
                 if layout[s] > 0]
        if after and m[name]["offset"] != m[after[0]]["offset"]:
            fail(f"{name} is empty but its offset 0x{m[name]['offset']:X} is "
                 f"not {after[0]}'s 0x{m[after[0]]['offset']:X}, so the "
                 f"collision this refusal guards against has moved and the "
                 f"rule no longer describes the layout")
        if g.offset_of(layout, name, 0) is not None:
            fail(f"{name} is empty and index 0 was given an offset, which is "
                 f"{after[0] if after else 'the end of the memory'}")

    # 4b. Contiguity, which is the same facts as 2 and 3 in one pass and is
    # the form the C uses: every configured element, walked in the memory's own
    # order, begins exactly where the previous one ended. It catches an overlap,
    # a hole, a wrong element size and a reordered enumeration together.
    expect = 0
    for name, index, off in every_element(layout):
        if off != expect:
            fail(f"{name} element {index} is at 0x{off:X} and the previous "
                 f"element ended at 0x{expect:X}")
        expect = off + m[name]["element_bytes"]
    if expect != doc["total_bytes"]:
        fail(f"walking every element ends at 0x{expect:X} and the layout "
             f"totals 0x{doc['total_bytes']:X}")

    # 5. Word alignment. This memory is read and written as words, and a
    # misaligned access to it faults rather than returning the wrong thing,
    # which at least is loud; the arithmetic that produces one is still wrong.
    for name, index, off in every_element(layout):
        if off % mr.WORD != 0:
            fail(f"{name} element {index} is at 0x{off:X}, which is not a "
                 f"multiple of {mr.WORD}")

    # 6. The first element of the first section is at offset zero, and no
    # element is at the base address. The second half catches an offset and an
    # address being swapped, which is the one mistake whose wrong answer is
    # inside the right memory.
    first = g.offset_of(layout, g.SECTIONS[0], 0)
    if first != 0:
        fail(f"the first element of the first section is at 0x{first:X} and "
             f"the memory begins at 0")
    for name, index, off in every_element(layout):
        if off == doc["msgram_base"]:
            fail(f"{name} element {index} has an offset equal to the message "
                 f"memory base, so an offset and an address cannot be told "
                 f"apart by inspection here")

    # 7. An index one past the count is refused for every section, including
    # the sections that are full to the part's own maximum.
    for name in g.SECTIONS:
        if g.offset_of(layout, name, layout[name]) is not None:
            fail(f"{name} index {layout[name]} was accepted and the layout "
                 f"configured {layout[name]} elements")

    if failures:
        print(f"test_msgaddr: {len(failures)} failure(s)")
        return 1
    total = sum(layout[n] for n in g.SECTIONS)
    print(f"ok  {len(doc['cases'])} accesses, {accepted} resolved and "
          f"{refused} refused, {total} elements tile {doc['total_bytes']} "
          f"bytes with no overlap")
    return 0


if __name__ == "__main__":
    sys.exit(main())
