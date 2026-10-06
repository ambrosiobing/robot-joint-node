#!/usr/bin/env python3
"""The message memory reference, against its vectors and its invariants.

    python test/test_msgram.py

The half that runs on the win11 aquamarine authoring laptop. test_msgram.c
checks the same vector file and is built only in continuous integration.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_msgram as g                                      # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_msgram.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    cases = doc["cases"]
    refused = 0

    for c in cases:
        r = g.compute(c["layout"])
        if r != c["result"]:
            fail(f"{c['label']}: the reference no longer produces the "
                 f"committed result")
            continue
        if not r["ok"]:
            refused += 1
            if not r["problems"]:
                fail(f"{c['label']}: refused with no reason given")
            continue

        # The sections must tile the space: each starts where the last ended,
        # with no hole and no overlap. An overlap is two sections writing the
        # same words, which looks like corruption rather than misconfiguration.
        off = 0
        for s in r["sections"]:
            if s["offset"] != off:
                fail(f"{c['label']}: {s['name']} starts at {s['offset']} and "
                     f"the previous section ends at {off}")
            if s["bytes"] != s["elements"] * s["element_bytes"]:
                fail(f"{c['label']}: {s['name']} is {s['bytes']} bytes for "
                     f"{s['elements']} elements of {s['element_bytes']}")
            off += s["bytes"]

        if off != r["total_bytes"]:
            fail(f"{c['label']}: the sections sum to {off} and the total says "
                 f"{r['total_bytes']}")
        if r["total_bytes"] + r["free_bytes"] != r["msgram_size_bytes"]:
            fail(f"{c['label']}: used plus free is not the memory size")
        if r["total_bytes"] > r["msgram_size_bytes"]:
            fail(f"{c['label']}: accepted a layout that does not fit")

        for s in r["sections"]:
            if s["elements"] > g.MAX_ELEMENTS[s["name"]]:
                fail(f"{c['label']}: {s['name']} has {s['elements']} elements "
                     f"and the part accepts {g.MAX_ELEMENTS[s['name']]}")

    # The chapter's own layout has to be the one that is accepted, because the
    # whole chapter is written around it.
    main_case = cases[0]
    if main_case["label"] != "chapter 9 step 3" or not main_case["result"]["ok"]:
        fail("the chapter's own layout is not the accepted case")

    if refused == 0:
        fail("not one layout was refused, so the refusal path is untested")

    if failures:
        print(f"test_msgram: {len(failures)} failure(s)")
        return 1
    total = main_case["result"]["total_bytes"]
    free = main_case["result"]["free_bytes"]
    print(f"ok  {len(cases)} layouts, {refused} refused, the chapter's own "
          f"tiles {total} bytes with {free} free")
    return 0


if __name__ == "__main__":
    sys.exit(main())
