#!/usr/bin/env python3
"""The element layout, checked against its vectors and against its invariants.

    python test/test_element.py

The vector comparison catches drift. The invariants below catch the three ways
this layout goes wrong without complaining, each of which would send or receive
a frame that is not the one anybody asked for:

  - the identifier in the wrong bits, which changes arbitration priority
  - the length code confused with the length, which moves the wrong byte count
  - the payload packed big endian, which reverses every four byte group
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_element as g                                         # noqa: E402
import gen_frame_vectors as fr                                  # noqa: E402
import gen_msgram as mr                                         # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_element.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    built = refused = 0

    for c in doc["cases"]:
        words = g.build_tx(c["id"], c["data"], c["fd"], c["brs"],
                           c["extended"], c["marker"], c["want_event"])

        if (words is not None) != c["ok"]:
            fail(f"{c['label']}: built is {words is not None}, the file says "
                 f"{c['ok']}")
            continue
        if not c["ok"]:
            refused += 1
            continue
        built += 1

        if words != c["words"]:
            fail(f"{c['label']}: words differ from the file")
            for i, (a, b) in enumerate(zip(words, c["words"])):
                if a != b:
                    fail(f"  word {i}: 0x{a:08X} against the file's 0x{b:08X}")
                    break
            continue

        # The round trip. Everything the caller gave must come back, and the
        # format must come back too, because a standard and an extended frame
        # with the same number are different frames.
        p = g.parse_rx(words)
        if p is None:
            fail(f"{c['label']}: built but does not parse")
            continue
        if p != c["parsed"]:
            fail(f"{c['label']}: parsed {p} against the file's {c['parsed']}")
            continue
        if p["id"] != c["id"]:
            fail(f"{c['label']}: id went in as 0x{c['id']:X} and came back as "
                 f"0x{p['id']:X}")
        if p["extended"] != c["extended"]:
            fail(f"{c['label']}: extended went in as {c['extended']} and came "
                 f"back as {p['extended']}")
        if p["data"] != c["data"]:
            fail(f"{c['label']}: payload went in as {c['data']} and came back "
                 f"as {p['data']}")
        if p["fd"] != c["fd"] or p["brs"] != c["brs"]:
            fail(f"{c['label']}: format flags did not survive the round trip")

        # The exact-length policy, asserted rather than assumed. If this ever
        # stops holding, the zero fill in the C stops being defensive and
        # becomes load bearing, and the comment there says so.
        nbytes, _code = g.wire_bytes(len(c["data"]), c["fd"])
        if nbytes != len(c["data"]):
            fail(f"{c['label']}: the code carries {nbytes} bytes for a "
                 f"{len(c['data'])} byte frame, so this chapter is padding "
                 f"after all and element.c's fill is no longer defensive")

        # The word count has to follow from the length and nothing else.
        want_words = g.HEADER_WORDS + (len(c["data"]) + 3) // 4
        if len(words) != want_words:
            fail(f"{c['label']}: {len(words)} words for {len(c['data'])} "
                 f"bytes, expected {want_words}")

    # ---------------------------------------------------------- the invariants

    # 1. The same number in the two formats must produce different words. This
    # is the defect the first draft of the generator had: it chose the format
    # from the magnitude, so a small extended identifier was unreachable and
    # every such frame silently went out as standard.
    std = next(c for c in doc["cases"]
               if c["label"].startswith("0x123 as a STANDARD"))
    ext = next(c for c in doc["cases"]
               if c["label"].startswith("0x123 as an EXTENDED"))
    if std["words"][0] == ext["words"][0]:
        fail("0x123 standard and 0x123 extended produced the same first word, "
             "so the two formats are not distinguishable and one of them is "
             "unreachable")
    if not (ext["words"][0] & g.W0_XTD) or (std["words"][0] & g.W0_XTD):
        fail("the extended bit does not follow the requested format")

    # 2. A standard identifier must occupy bits 28 to 18 and nothing lower.
    for c in doc["cases"]:
        if not c["ok"] or c["extended"]:
            continue
        low = c["words"][0] & ((1 << g.ID_STD_SHIFT) - 1)
        if low != 0:
            fail(f"{c['label']}: a standard identifier left 0x{low:X} in the "
                 f"bits below {g.ID_STD_SHIFT}, so it was not shifted")

    # 3. The payload must be little endian within each word, checked on a case
    # whose bytes are all different so a reversal cannot hide.
    probe = g.build_tx(0x1, [0x11, 0x22, 0x33, 0x44], False, False)
    if probe is None:
        fail("the endianness probe refused to build")
    elif probe[g.HEADER_WORDS] != 0x44332211:
        fail(f"bytes 11 22 33 44 packed to 0x{probe[g.HEADER_WORDS]:08X}, and "
             f"little endian is 0x44332211")

    # 4. The largest element must match what msgram.h reserves for a frame, or
    # the layout and the codec disagree about how much room a frame needs.
    biggest = max(len(c["words"]) for c in doc["cases"] if c["ok"])
    if biggest != doc["max_words"]:
        fail(f"the largest element is {biggest} words and the header says "
             f"{doc['max_words']}")
    msgram_words = mr.ELEMENT_BYTES["rx_fifo0"] // 4
    if biggest != msgram_words:
        fail(f"element.h sizes a frame at {biggest} words and msgram sizes its "
             f"frame sections at {msgram_words}, so one of them is wrong about "
             f"how much message memory a frame occupies")

    # 5. A short read must be refused rather than returning a tail of whatever
    # followed. This is the receive side's one silent failure.
    full = next(c for c in doc["cases"]
                if c["ok"] and c["word_count"] > g.HEADER_WORDS)
    if g.parse_rx(full["words"][:-1]) is not None:
        fail("an element one word short of its own length code was parsed "
             "rather than refused")

    # 6. Sizing an element is a function of the FRAME, and it must refuse
    # exactly the frames frame_ok refuses: no more, because a marker above 255
    # and a buffer too small refuse a build without making the frame illegal,
    # and no fewer, because a caller sizes a buffer with words_for and fills it
    # with build_tx. The C had these two disagree on a zero length frame that
    # was refused for a reason other than its length, and the pair of zero
    # length cases in the vectors is what now reaches that input.
    for c in doc["cases"]:
        got = g.words_for(len(c["data"]), c["fd"], c["brs"])
        legal = fr.frame_ok(len(c["data"]), c["fd"], c["brs"])
        if (got is not None) != legal:
            fail(f"{c['label']}: words_for returned {got} and frame_ok says "
                 f"the frame is {'legal' if legal else 'illegal'}")
        if got != (c["words_for"] or None):
            fail(f"{c['label']}: words_for returned {got} and the file says "
                 f"{c['words_for']}")
        if c["ok"] and got != c["word_count"]:
            fail(f"{c['label']}: words_for says {got} and the build wrote "
                 f"{c['word_count']}")

    zero_legal = [c for c in doc["cases"]
                  if not c["data"] and c["frame_ok"]]
    zero_refused = [c for c in doc["cases"]
                    if not c["data"] and not c["frame_ok"]]
    if not zero_legal or not zero_refused:
        fail("the zero length boundary needs both a legal frame and a refused "
             "one, or nothing distinguishes a refusal from an empty payload, "
             "which is the defect this pair was added for")

    # 7. Every refusal must refuse for a stated reason, and there must be
    # enough of them that the refusal path is exercised.
    if refused == 0:
        fail("not one frame was refused, so the refusal path is untested")
    for c in doc["cases"]:
        if not c["ok"] and not c["label"].startswith("REFUSE"):
            fail(f"{c['label']}: refused but not labelled as a refusal, so "
                 f"either the label or the behaviour is wrong")

    if failures:
        print(f"test_element: {len(failures)} failure(s)")
        return 1
    print(f"ok  {len(doc['cases'])} frames, {built} built and {refused} "
          f"refused, every round trip exact")
    return 0


if __name__ == "__main__":
    sys.exit(main())
