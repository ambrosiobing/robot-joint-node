#!/usr/bin/env python3
"""A frame as the message memory holds it, and the vectors the C must reproduce.

    python tools/gen_element.py            regenerate the vectors
    python tools/gen_element.py --check    fail if the committed ones differ
    python tools/gen_element.py --table    print the table for doc/element.md

Chapter 9 stage two. frame.py knows the length code, gen_msgram.py knows where
the sections live, and neither turns a frame into the words a controller reads.

The bit positions are from the mainline Linux driver for this peripheral, the
Bosch M_CAN, because the element format is Bosch's rather than any one vendor's.
Both DLC fields were confirmed rather than inferred: the transmit side defines
TX_BUF_DLC_MASK as bits 19:16, and the receive side has no such constant and
extracts the same field as (word >> 16) & 0x0F.

Three things in this layout fail silently, and each one has vectors of its own:

  - a standard identifier is shifted up by eighteen and an extended one is not,
    so confusing them sends a frame with a different arbitration priority rather
    than an error
  - the length written is a CODE, and only some lengths have one, so nine bytes
    is REFUSED here while the controller end pads it to twelve
  - the payload is little endian within each word, and getting that backwards
    reverses every four byte group, which reads as corruption rather than layout

The identifier format is a PARAMETER rather than something deduced from the
value, because 0x123 standard and 0x123 extended are different frames on the
wire and a receiver filtering for one does not see the other. The first draft of
this file deduced it from the magnitude, which made every small extended
identifier unreachable, and the vectors are what caught it.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_frame_vectors as fr                                  # noqa: E402

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_element.json"
VECTORS_H = HERE / "test" / "vectors_element.h"

W0_ESI = 1 << 31
W0_XTD = 1 << 30
W0_RTR = 1 << 29

ID_EXT_MASK = 0x1FFFFFFF
ID_STD_MASK = 0x000007FF
ID_STD_SHIFT = 18

T1_MM_SHIFT = 24
T1_MM_MASK = 0xFF
T1_EFC = 1 << 23
W1_FDF = 1 << 21
W1_BRS = 1 << 20
DLC_SHIFT = 16
DLC_MASK = 0x0F

R1_ANMF = 1 << 31
R1_RXTS_MASK = 0x0000FFFF

HEADER_WORDS = 2
FRAME_FD_MAX = 64
MAX_WORDS = HEADER_WORDS + FRAME_FD_MAX // 4


def wire_bytes(length, fd):
    """Bytes the wire carries, which is the code's length and not the caller's."""
    code = fr.code_for_length(length, fd)
    if code is None:
        return None, None
    return fr.length_for_code(code, fd), code


def words_for(length, fd, brs):
    """How many words an element carrying this frame occupies, or None when the
    frame is refused.

    A function of the FRAME alone, which is why it knows nothing about the
    message marker or the caller's buffer: those refuse a build without making
    the frame illegal. The C has the same function and a caller sizes a buffer
    with one and fills it with the other, so the two must refuse the same
    inputs. They did not: the C told a refusal from an empty frame by asking
    whether the length was zero, and a zero length frame refused for any other
    reason came back sized rather than refused.
    """
    if not fr.frame_ok(length, fd, brs):
        return None
    nbytes, _code = wire_bytes(length, fd)
    if nbytes is None:
        return None
    return HEADER_WORDS + (nbytes + 3) // 4


def build_tx(fid, data, fd, brs, extended=False, marker=0, want_event=False):
    """The words, or None when the frame is refused."""
    length = len(data)
    if not fr.frame_ok(length, fd, brs):
        return None
    if not 0 <= marker <= T1_MM_MASK:
        return None
    nbytes, code = wire_bytes(length, fd)
    if nbytes is None:
        return None

    # The format is the caller's choice, and the value has to fit it. Promoting
    # a too-large value to extended would send a frame the caller did not ask
    # for, and demoting a small one to standard makes small extended identifiers
    # unreachable.
    if extended:
        if fid > ID_EXT_MASK:
            return None
        w0 = W0_XTD | (fid & ID_EXT_MASK)
    else:
        if fid > ID_STD_MASK:
            return None
        w0 = (fid & ID_STD_MASK) << ID_STD_SHIFT

    w1 = (code & DLC_MASK) << DLC_SHIFT
    w1 |= (marker & T1_MM_MASK) << T1_MM_SHIFT
    if want_event:
        w1 |= T1_EFC
    if fd:
        w1 |= W1_FDF
        if brs:
            w1 |= W1_BRS

    words = [w0, w1]
    for i in range(0, nbytes, 4):
        w = 0
        for b in range(4):
            at = i + b
            v = data[at] if at < length else 0
            w |= v << (8 * b)
        words.append(w)
    return words


def parse_rx(words):
    """The frame, or None when the element is refused."""
    if len(words) < HEADER_WORDS:
        return None
    w0, w1 = words[0], words[1]
    if w0 & W0_RTR:
        return None

    fd = bool(w1 & W1_FDF)
    code = (w1 >> DLC_SHIFT) & DLC_MASK
    nbytes = fr.length_for_code(code, fd)
    if nbytes is None:
        return None

    need = HEADER_WORDS + (nbytes + 3) // 4
    if need > len(words):
        return None

    fid = (w0 & ID_EXT_MASK) if (w0 & W0_XTD) \
        else ((w0 >> ID_STD_SHIFT) & ID_STD_MASK)

    data = []
    for i in range(nbytes):
        w = words[HEADER_WORDS + i // 4]
        data.append((w >> (8 * (i % 4))) & 0xFF)

    return {
        "id": fid,
        "extended": bool(w0 & W0_XTD),
        "fd": fd,
        "brs": fd and bool(w1 & W1_BRS),
        "len": nbytes,
        "data": data,
        "timestamp": w1 & R1_RXTS_MASK,
        "anmf": bool(w1 & R1_ANMF),
    }


def counter(n):
    """The payload chapter 10 actually sends: a little endian counter in eight
    bytes, so bytes 4 to 7 are always zero. Its stuffing behaviour is measured
    in chapter 10 part three and the figure there is specific to it."""
    return [(n >> (8 * i)) & 0xFF for i in range(4)] + [0, 0, 0, 0]


# Each case names what it is for. A vector that exists only to be round tripped
# proves the pair of functions agree with each other and nothing else.
CASES = [
    # (label, id, data, fd, brs, extended, marker, want_event)
    ("the chapter 10 test frame, standard id 0x123, four bytes classic",
     0x123, [0xDE, 0xAD, 0xBE, 0xEF], False, False, False, 0, False),
    ("the chapter 10 setpoint frame, id 0x101, eight byte counter",
     0x101, counter(0), False, False, False, 0, False),
    ("a standard id at its maximum, 0x7FF",
     0x7FF, [1], False, False, False, 0, False),
    ("identifier zero, which must not be mistaken for an empty element",
     0x000, [0xFF], False, False, False, 0, False),
    ("an extended identifier, which sits in different bits entirely",
     0x1ABCDEF, [1, 2, 3, 4], False, False, True, 0, False),
    ("an extended identifier at its maximum, 0x1FFFFFFF",
     0x1FFFFFFF, [0xAA], False, False, True, 0, False),
    # The pair that the first draft could not tell apart. Same number, two
    # formats, two different frames, and the words must differ.
    ("0x123 as a STANDARD identifier, the first of a matched pair",
     0x123, [1], False, False, False, 0, False),
    ("0x123 as an EXTENDED identifier, the second of the pair",
     0x123, [1], False, False, True, 0, False),
    ("zero length, which is a legal frame and writes no payload word",
     0x200, [], False, False, False, 0, False),
    ("zero length flexible with a rate switch, legal, and the contrast that "
     "makes the zero length refusal below mean something",
     0x201, [], True, True, False, 0, False),
    ("twelve bytes, the first length code that is not its own index",
     0x300, list(range(1, 13)), True, False, False, 0, False),
    ("forty-eight bytes, a code in the steeper part of the table",
     0x301, list(range(1, 49)), True, False, False, 0, False),
    ("a full 64 byte flexible frame, the largest element",
     0x302, list(range(64)), True, True, False, 0, False),
    ("a message marker and an event request, which live in the second word",
     0x400, [9, 9], False, False, False, 0xA5, True),
    ("a marker at its maximum, 255",
     0x401, [1], False, False, False, 255, False),
    # The refusals. The first two are the ones the controller end pads and this
    # end will not, which is the difference worth knowing about.
    ("REFUSE, nine bytes flexible: no code stands for 9, and this end does "
     "not pad where the controller end does",
     0x500, list(range(9)), True, False, False, 0, False),
    ("REFUSE, thirty-three bytes flexible, for the same reason",
     0x501, list(range(33)), True, False, False, 0, False),
    ("REFUSE, nine bytes in a classic frame",
     0x502, list(range(9)), False, False, False, 0, False),
    ("REFUSE, a rate switch without the flexible format",
     0x503, [1], False, True, False, 0, False),
    ("REFUSE, a message marker above 255",
     0x504, [1], False, False, False, 256, False),
    # The refusal whose own length is zero. The C told a refusal from an empty
    # frame by asking whether the length was zero, so this one built where the
    # reference refused it, and no case reached that input until this one.
    ("REFUSE, zero length with a rate switch and no flexible format, the "
     "refusal whose own length is zero",
     0x505, [], False, True, False, 0, False),
    ("REFUSE, an identifier past 29 bits even as extended",
     0x20000000, [1], False, False, True, 0, False),
    ("REFUSE, an identifier past 11 bits asked for as standard",
     0x800, [1], False, False, False, 0, False),
]


def build():
    out = []
    for label, fid, data, fd, brs, xtd, marker, efc in CASES:
        words = build_tx(fid, data, fd, brs, xtd, marker, efc)
        parsed = parse_rx(words) if words else None
        out.append({
            "label": label,
            "id": fid,
            "data": list(data),
            "fd": fd,
            "brs": brs,
            "extended": xtd,
            "marker": marker,
            "want_event": efc,
            # Whether the FRAME is legal, which is not the same question as
            # whether a build succeeds: a marker above 255 and a buffer too
            # small both refuse a build and leave the frame legal.
            "frame_ok": fr.frame_ok(len(data), fd, brs),
            "words_for": words_for(len(data), fd, brs) or 0,
            "ok": words is not None,
            "words": words,
            "word_count": len(words) if words else 0,
            "parsed": parsed,
        })
    return {
        "note": "Generated by tools/gen_element.py. Do not edit.",
        "header_words": HEADER_WORDS,
        "max_words": MAX_WORDS,
        "cases": out,
    }


def as_header(doc):
    lines = [
        "/* Generated by tools/gen_element.py. Do not edit. */",
        "#ifndef JOINT_ELEMENT_VECTORS_H",
        "#define JOINT_ELEMENT_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        "typedef struct {",
        "    const char     *label;",
        "    uint32_t        id;",
        "    const uint8_t  *data;",
        "    uint32_t        len;",
        "    bool            fd;",
        "    bool            brs;",
        "    bool            extended;",
        "    uint32_t        marker;",
        "    bool            want_event;",
        "    bool            ok;",
        "    uint32_t        word_count;",
        "    const uint32_t *words;",
        "    uint32_t        parsed_len;   /* the code's length, not the caller's */",
        "    bool            frame_ok;     /* the FRAME is legal, which is not the same as a build succeeding */",
        "    uint32_t        words_for;    /* what elem_words_for must answer, 0 for a refused frame */",
        "} el_vector_t;",
        "",
    ]
    for i, c in enumerate(doc["cases"]):
        if c["data"]:
            body = ", ".join(f"0x{b:02X}u" for b in c["data"])
            lines.append(f"static const uint8_t EL_DATA_{i}[] = {{ {body} }};")
        if c["ok"]:
            body = ", ".join(f"0x{w:08X}u" for w in c["words"])
            lines.append(f"static const uint32_t EL_WORDS_{i}[] = {{ {body} }};")
    lines.append("")
    lines.append("static const el_vector_t EL_VECTORS[] = {")
    for i, c in enumerate(doc["cases"]):
        lines.append(
            '    {{ "{label}", 0x{id:X}u, {data}, {len}u, {fd}, {brs}, {xtd}, '
            "{mm}u, {efc}, {ok}, {n}u, {words}, {plen}u, {fok}, {wfor}u }},".format(
                label=c["label"], id=c["id"],
                data=f"EL_DATA_{i}" if c["data"] else "0",
                len=len(c["data"]), fd="true" if c["fd"] else "false",
                brs="true" if c["brs"] else "false",
                xtd="true" if c["extended"] else "false", mm=c["marker"],
                efc="true" if c["want_event"] else "false",
                ok="true" if c["ok"] else "false", n=c["word_count"],
                words=f"EL_WORDS_{i}" if c["ok"] else "0",
                plen=c["parsed"]["len"] if c["parsed"] else 0,
                fok="true" if c["frame_ok"] else "false",
                wfor=c["words_for"]))
    lines += [
        "};",
        "",
        "#define EL_VECTOR_COUNT "
        "((int) (sizeof EL_VECTORS / sizeof EL_VECTORS[0]))",
        "",
        "#endif /* JOINT_ELEMENT_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def plural(n):
    """One byte, two bytes. A generated table that prints "1 bytes" reads as
    though nobody looked at it, and after that neither does the reader."""
    return f"{n} byte" if n == 1 else f"{n} bytes"


def table(doc):
    """The identifier format and the frame format are separate columns, because
    they are independent and the words carry them in different places: the
    identifier format in the first word, the frame format in the second. A
    table showing only one of the two would leave a reader unable to check the
    bit printed next to it."""
    rows = ["| Case | Given | Identifier | Frame | On the wire | Words | "
            "First word | Second word |",
            "|---|---|---|---|---|---|---|---|"]
    for c in doc["cases"]:
        idfmt = "extended" if c["extended"] else "standard"
        if c["fd"]:
            frfmt = "flexible, rate switch" if c["brs"] else "flexible"
        elif c["brs"]:
            # The contradiction this row exists to show. Printing plain
            # "classic" here would hide the asked-for rate switch, and the
            # reader would be left wondering what the refusal was about.
            frfmt = "classic, rate switch asked for"
        else:
            frfmt = "classic"
        if not c["ok"]:
            rows.append(f"| {c['label']} | {plural(len(c['data']))} | "
                        f"{idfmt} | {frfmt} | "
                        "refused | refused | refused | refused |")
            continue
        pp = c["parsed"]
        rows.append(
            f"| {c['label']} | {plural(len(c['data']))} | {idfmt} | {frfmt} | "
            f"**{plural(pp['len'])}** | {c['word_count']} | "
            f"`0x{c['words'][0]:08X}` | `0x{c['words'][1]:08X}` |")
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
        refused = sum(1 for c in doc["cases"] if not c["ok"])
        print(f"element vectors match the generator ({len(doc['cases'])} frames, "
              f"{refused} of them refused)")
        return 0

    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    refused = sum(1 for c in doc["cases"] if not c["ok"])
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} frames, {refused} of them refused")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
