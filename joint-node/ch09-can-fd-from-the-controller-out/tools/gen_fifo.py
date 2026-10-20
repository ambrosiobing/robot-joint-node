#!/usr/bin/env python3
"""The queue protocol, as arithmetic over status words, and its vectors.

    python tools/gen_fifo.py            regenerate the vectors
    python tools/gen_fifo.py --check    fail if the committed ones differ
    python tools/gen_fifo.py --table    print the tables for doc/node-queues.md

Chapter 9 stage two. gen_msgaddr.py turns an index into a place. This is where
the index comes from, and where the write that releases or launches an element
is composed.

Nothing here touches a register: every function takes a status word read
somewhere else. On a board each mistake below presents as a frame missing,
duplicated or late, never as an error, which is why the whole protocol is
arithmetic that can be checked on a laptop.

THE ONE THAT WILL BITE. The receive acknowledge register takes AN INDEX. The
transmit add-request register takes A BIT PER BUFFER.

    RXF0A <- 3           releases through element 3
    TXBAR <- 1 << 3      requests transmission of buffer 3

Writing `1 << 3` to the acknowledge register releases through element 8, losing
five elements nobody read. Writing `3` to the add request register requests
buffers 0 and 1, one of which was never filled. Both writes are accepted and
neither errors.

[kernel] The field positions and the acknowledge encoding are from the mainline
Linux driver for this peripheral, the Bosch M_CAN, in
drivers/net/can/m_can/m_can.c, read on Tuesday 20 October 2026:

    M_CAN_RXF0S 0xa4   RXFS_RFL BIT(25)        RXFS_FF BIT(24)
    M_CAN_RXF0A 0xa8   RXFS_FPI_MASK GENMASK(21, 16)
    M_CAN_TXFQS 0xc4   RXFS_FGI_MASK GENMASK(13, 8)
    M_CAN_TXBAR 0xd0   RXFS_FFL_MASK GENMASK(6, 0)
    M_CAN_TXBRP 0xcc   TXFQS_TFQF BIT(21)      TXFQS_TFQPI_MASK GENMASK(20, 16)
                       TXFQS_TFGI_MASK GENMASK(12, 8)
                       TXFQS_TFFL_MASK GENMASK(5, 0)

The driver reads the get index out of RXF0S and writes that same value, not a
mask and not the index plus one, into RXF0A. It reads a run of elements while
tracking the index itself and writes ONE acknowledge afterwards carrying the
last index read.

[inferred] That one acknowledge releases the whole run rather than a single
element follows from the driver doing it, not from any statement that it does.
The board run that settles it: read two frames, acknowledge the second index
only, and see whether the fill level falls by two or by one. Until that run
exists this file and the chapter both say inferred.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_msgram as mr                                          # noqa: E402

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_fifo.json"
VECTORS_H = HERE / "test" / "vectors_fifo.h"

RXFS_FFL_MASK = 0x7F
RXFS_FGI_SHIFT, RXFS_FGI_MASK = 8, 0x3F
RXFS_FPI_SHIFT, RXFS_FPI_MASK = 16, 0x3F
RXFS_FF = 1 << 24
RXFS_RFL = 1 << 25

TXFQS_TFFL_MASK = 0x3F
TXFQS_TFGI_SHIFT, TXFQS_TFGI_MASK = 8, 0x1F
TXFQS_TFQPI_SHIFT, TXFQS_TFQPI_MASK = 16, 0x1F
TXFQS_TFQF = 1 << 21

RX_INDEX_LIMIT = RXFS_FGI_MASK + 1      # 64
TX_INDEX_LIMIT = TXFQS_TFGI_MASK + 1    # 32

# The queues this node configures, from the layout in gen_msgram.py, so the
# cases below exercise the sizes that actually exist rather than round numbers.
RX0 = mr.LAYOUT["rx_fifo0"]             # 16
RX1 = mr.LAYOUT["rx_fifo1"]             # 8
TXB = mr.LAYOUT["tx_buffers"]           # 8


def rx_word(fill, get, put, full=False, lost=False):
    """Compose a receive status word from its fields."""
    w = (fill & RXFS_FFL_MASK)
    w |= (get & RXFS_FGI_MASK) << RXFS_FGI_SHIFT
    w |= (put & RXFS_FPI_MASK) << RXFS_FPI_SHIFT
    if full:
        w |= RXFS_FF
    if lost:
        w |= RXFS_RFL
    return w


def tx_word(free, get, put, full=False):
    """Compose a transmit status word from its fields."""
    w = (free & TXFQS_TFFL_MASK)
    w |= (get & TXFQS_TFGI_MASK) << TXFQS_TFGI_SHIFT
    w |= (put & TXFQS_TFQPI_MASK) << TXFQS_TFQPI_SHIFT
    if full:
        w |= TXFQS_TFQF
    return w


def rx_parse(word, configured):
    """The decoded receive status, or None when refused."""
    if configured == 0 or configured > RX_INDEX_LIMIT:
        return None
    fill = word & RXFS_FFL_MASK
    get = (word >> RXFS_FGI_SHIFT) & RXFS_FGI_MASK
    put = (word >> RXFS_FPI_SHIFT) & RXFS_FPI_MASK
    if fill > configured or get >= configured or put >= configured:
        return None
    full = bool(word & RXFS_FF)
    # The full flag and the fill level are two statements of one fact. A word
    # that makes them disagree is not describing a queue, and trusting either
    # one alone is how a full queue gets written to.
    if full != (fill == configured):
        return None
    return {"fill": fill, "get_index": get, "put_index": put,
            "full": full, "lost": bool(word & RXFS_RFL)}


def tx_parse(word, configured):
    """The decoded transmit status, or None when refused."""
    if configured == 0 or configured > TX_INDEX_LIMIT:
        return None
    freel = word & TXFQS_TFFL_MASK
    get = (word >> TXFQS_TFGI_SHIFT) & TXFQS_TFGI_MASK
    put = (word >> TXFQS_TFQPI_SHIFT) & TXFQS_TFQPI_MASK
    if freel > configured or get >= configured or put >= configured:
        return None
    full = bool(word & TXFQS_TFQF)
    if full != (freel == 0):
        return None
    return {"free": freel, "get_index": get, "put_index": put, "full": full}


def rx_at(state, configured, n):
    """The index of the n-th waiting element, or None when refused."""
    if state is None or configured == 0:
        return None
    # An empty FIFO refuses n of 0. Its get index points at the element last
    # released, the controller does not clear the memory, and a read there
    # returns a frame that has already been delivered once.
    if n >= state["fill"]:
        return None
    if state["get_index"] >= configured:
        return None
    return (state["get_index"] + n) % configured


def rx_ack_index(state, configured, n):
    """The value to write to the acknowledge register, or None when refused."""
    if state is None:
        return None
    # Nothing read means nothing to release, and writing the get index would
    # release an element nobody has looked at.
    if n == 0 or n > state["fill"]:
        return None
    return rx_at(state, configured, n - 1)


def tx_put(state):
    """Where to write the next frame, or None when refused."""
    if state is None:
        return None
    # A full queue still reports a put index, and it belongs to a frame already
    # requested and not yet sent.
    if state["full"] or state["free"] == 0:
        return None
    return state["put_index"]


def tx_request_mask(index, configured):
    """The value to write to the add request register, or None when refused."""
    if configured == 0 or configured > TX_INDEX_LIMIT:
        return None
    if index >= configured:
        return None
    return 1 << index


def tx_pending(word, index, configured):
    """Whether that buffer is still waiting, or None when refused."""
    if configured == 0 or configured > TX_INDEX_LIMIT:
        return None
    if index >= configured:
        return None
    return bool(word & (1 << index))


# ------------------------------------------------------------ the status words
PARSE_CASES = [
    # (label, side, word, configured)
    ("receive FIFO 0 empty, which still reports a get index",
     "rx", rx_word(0, 0, 0), RX0),
    ("one frame waiting at the start of receive FIFO 0",
     "rx", rx_word(1, 0, 1), RX0),
    ("three waiting near the end, the put index already wrapped",
     "rx", rx_word(3, 13, 0), RX0),
    ("receive FIFO 0 full, sixteen waiting, flag and level agreeing",
     "rx", rx_word(16, 0, 0, full=True), RX0),
    ("two waiting and the sticky lost flag set, which is the only trace a "
     "dropped frame leaves",
     "rx", rx_word(2, 7, 9, lost=True), RX0),
    ("receive FIFO 1, eight elements, half full",
     "rx", rx_word(4, 2, 6), RX1),
    ("REFUSE, a fill level of 17 in a queue of 16, so the controller and the "
     "layout disagree about the size",
     "rx", rx_word(17, 0, 1), RX0),
    ("REFUSE, a get index of 16 in a queue of 16, which addresses the next "
     "section",
     "rx", rx_word(1, 16, 0), RX0),
    ("REFUSE, the full flag set with three waiting",
     "rx", rx_word(3, 0, 3, full=True), RX0),
    ("REFUSE, sixteen waiting in a queue of 16 and the full flag clear",
     "rx", rx_word(16, 0, 0), RX0),
    ("REFUSE, a queue the layout configured with no elements",
     "rx", rx_word(0, 0, 0), 0),
    ("every transmit buffer free",
     "tx", tx_word(TXB, 0, 0), TXB),
    ("three frames queued of eight buffers",
     "tx", tx_word(5, 0, 3), TXB),
    ("the transmit queue full, flag and free level agreeing",
     "tx", tx_word(0, 3, 3, full=True), TXB),
    ("REFUSE, nine free in a queue of eight",
     "tx", tx_word(9, 0, 0), TXB),
    ("REFUSE, the full flag set with two free",
     "tx", tx_word(2, 0, 6, full=True), TXB),
    ("REFUSE, no free elements and the full flag clear",
     "tx", tx_word(0, 3, 3), TXB),
    ("REFUSE, a transmit queue of 33, past what the index field can address",
     "tx", tx_word(1, 0, 0), 33),
]

# -------------------------------------------------------- the receive protocol
RX_OP_CASES = [
    # (label, word, configured, n)
    ("read the first of three waiting, at get index 13",
     rx_word(3, 13, 0), RX0, 0),
    ("read the second of the three, at index 14",
     rx_word(3, 13, 0), RX0, 1),
    ("read the third of the three, index 15, the last element of the queue",
     rx_word(3, 13, 0), RX0, 2),
    ("read the third of three from get index 14, WHICH WRAPS to index 0",
     rx_word(3, 14, 1), RX0, 2),
    ("REFUSE, the fourth of three waiting",
     rx_word(3, 13, 0), RX0, 3),
    ("REFUSE, the first element of an EMPTY queue, whose get index is a "
     "perfectly good number pointing at an element already released",
     rx_word(0, 5, 5), RX0, 0),
    ("read the last element of a full queue",
     rx_word(16, 0, 0, full=True), RX0, 15),
]

# ------------------------------------------------------- the transmit protocol
TX_OP_CASES = [
    # (label, word, configured, index, pending_word)
    ("write the next frame into an empty queue and request buffer 0",
     tx_word(TXB, 0, 0), TXB, 0, 0x00),
    ("write into a queue with three already in it, buffer 3",
     tx_word(5, 0, 3), TXB, 3, 0x07),
    ("buffer 7, the last one this layout configures",
     tx_word(1, 0, 7), TXB, 7, 0x7F),
    ("REFUSE to write into a full queue, whose put index belongs to a frame "
     "already requested and not yet sent",
     tx_word(0, 3, 3, full=True), TXB, 3, 0xFF),
    ("REFUSE buffer 8 of the eight configured",
     tx_word(TXB, 0, 0), TXB, 8, 0x00),
    ("REFUSE buffer 31, which the register can address and the layout cannot",
     tx_word(TXB, 0, 0), TXB, 31, 0x00),
]


def build():
    parse = []
    for label, side, word, configured in PARSE_CASES:
        st = rx_parse(word, configured) if side == "rx" \
            else tx_parse(word, configured)
        parse.append({
            "label": label, "side": side, "word": word,
            "configured": configured, "ok": st is not None,
            "fill": st.get("fill", 0) if st and side == "rx" else 0,
            "free": st.get("free", 0) if st and side == "tx" else 0,
            "get_index": st["get_index"] if st else 0,
            "put_index": st["put_index"] if st else 0,
            "full": st["full"] if st else False,
            "lost": st.get("lost", False) if st and side == "rx" else False,
        })

    rx_ops = []
    for label, word, configured, n in RX_OP_CASES:
        st = rx_parse(word, configured)
        at = rx_at(st, configured, n)
        ack = rx_ack_index(st, configured, n + 1)
        rx_ops.append({
            "label": label, "word": word, "configured": configured, "n": n,
            "at_ok": at is not None, "at": at if at is not None else 0,
            # The acknowledge after reading n+1 elements, which is the write
            # that follows having read the element at `at`.
            "ack_ok": ack is not None, "ack": ack if ack is not None else 0,
        })

    tx_ops = []
    for label, word, configured, index, pend in TX_OP_CASES:
        st = tx_parse(word, configured)
        put = tx_put(st)
        mask = tx_request_mask(index, configured)
        pending = tx_pending(pend, index, configured)
        tx_ops.append({
            "label": label, "word": word, "configured": configured,
            "index": index, "pending_word": pend,
            "put_ok": put is not None, "put": put if put is not None else 0,
            "mask_ok": mask is not None, "mask": mask if mask is not None else 0,
            "pending_ok": pending is not None,
            "pending": bool(pending) if pending is not None else False,
        })

    return {
        "note": "Generated by tools/gen_fifo.py. Do not edit.",
        "rx_fifo0": RX0, "rx_fifo1": RX1, "tx_buffers": TXB,
        "parse": parse, "rx_ops": rx_ops, "tx_ops": tx_ops,
    }


def as_header(doc):
    lines = [
        "/* Generated by tools/gen_fifo.py. Do not edit. */",
        "#ifndef JOINT_FIFO_VECTORS_H",
        "#define JOINT_FIFO_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        f"#define FV_RX_FIFO0 {doc['rx_fifo0']}u",
        f"#define FV_RX_FIFO1 {doc['rx_fifo1']}u",
        f"#define FV_TX_BUFFERS {doc['tx_buffers']}u",
        "",
        "typedef struct {",
        "    const char *label;",
        "    bool        is_rx;",
        "    uint32_t    word;",
        "    uint32_t    configured;",
        "    bool        ok;",
        "    uint32_t    fill;        /* receive side */",
        "    uint32_t    free_level;  /* transmit side */",
        "    uint32_t    get_index;",
        "    uint32_t    put_index;",
        "    bool        full;",
        "    bool        lost;",
        "} fv_parse_t;",
        "",
        "static const fv_parse_t FV_PARSE[] = {",
    ]
    for c in doc["parse"]:
        lines.append(
            '    {{ "{label}", {rx}, 0x{word:08X}u, {conf}u, {ok}, {fill}u, '
            "{free}u, {get}u, {put}u, {full}, {lost} }},".format(
                label=c["label"], rx="true" if c["side"] == "rx" else "false",
                word=c["word"], conf=c["configured"],
                ok="true" if c["ok"] else "false", fill=c["fill"],
                free=c["free"], get=c["get_index"], put=c["put_index"],
                full="true" if c["full"] else "false",
                lost="true" if c["lost"] else "false"))
    lines += [
        "};",
        "",
        "#define FV_PARSE_COUNT ((int) (sizeof FV_PARSE / sizeof FV_PARSE[0]))",
        "",
        "typedef struct {",
        "    const char *label;",
        "    uint32_t    word;",
        "    uint32_t    configured;",
        "    uint32_t    n;",
        "    bool        at_ok;",
        "    uint32_t    at;",
        "    bool        ack_ok;      /* after reading n + 1 elements */",
        "    uint32_t    ack;",
        "} fv_rx_op_t;",
        "",
        "static const fv_rx_op_t FV_RX_OPS[] = {",
    ]
    for c in doc["rx_ops"]:
        lines.append(
            '    {{ "{label}", 0x{word:08X}u, {conf}u, {n}u, {aok}, {at}u, '
            "{kok}, {ack}u }},".format(
                label=c["label"], word=c["word"], conf=c["configured"],
                n=c["n"], aok="true" if c["at_ok"] else "false", at=c["at"],
                kok="true" if c["ack_ok"] else "false", ack=c["ack"]))
    lines += [
        "};",
        "",
        "#define FV_RX_OP_COUNT "
        "((int) (sizeof FV_RX_OPS / sizeof FV_RX_OPS[0]))",
        "",
        "typedef struct {",
        "    const char *label;",
        "    uint32_t    word;",
        "    uint32_t    configured;",
        "    uint32_t    index;",
        "    uint32_t    pending_word;",
        "    bool        put_ok;",
        "    uint32_t    put;",
        "    bool        mask_ok;",
        "    uint32_t    mask;",
        "    bool        pending_ok;",
        "    bool        pending;",
        "} fv_tx_op_t;",
        "",
        "static const fv_tx_op_t FV_TX_OPS[] = {",
    ]
    for c in doc["tx_ops"]:
        lines.append(
            '    {{ "{label}", 0x{word:08X}u, {conf}u, {idx}u, 0x{pw:08X}u, '
            "{pok}, {put}u, {mok}, 0x{mask:08X}u, {gok}, {pend} }},".format(
                label=c["label"], word=c["word"], conf=c["configured"],
                idx=c["index"], pw=c["pending_word"],
                pok="true" if c["put_ok"] else "false", put=c["put"],
                mok="true" if c["mask_ok"] else "false", mask=c["mask"],
                gok="true" if c["pending_ok"] else "false",
                pend="true" if c["pending"] else "false"))
    lines += [
        "};",
        "",
        "#define FV_TX_OP_COUNT "
        "((int) (sizeof FV_TX_OPS / sizeof FV_TX_OPS[0]))",
        "",
        "#endif /* JOINT_FIFO_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    out = ["### The status words", "",
           "| Case | Side | Word | Configured | Waiting or free | Get | Put | "
           "Full | Lost |", "|---|---|---|---|---|---|---|---|---|"]
    for c in doc["parse"]:
        side = "receive" if c["side"] == "rx" else "transmit"
        if not c["ok"]:
            out.append(f"| {c['label']} | {side} | `0x{c['word']:08X}` | "
                       f"{c['configured']} | refused | refused | refused | "
                       f"refused | refused |")
            continue
        n = c["fill"] if c["side"] == "rx" else c["free"]
        out.append(
            f"| {c['label']} | {side} | `0x{c['word']:08X}` | "
            f"{c['configured']} | {n} | {c['get_index']} | {c['put_index']} | "
            f"{'yes' if c['full'] else 'no'} | "
            f"{'yes' if c['lost'] else 'no'} |")

    out += ["", "### Reading and releasing", "",
            "| Case | Get | Waiting | n | Element to read | Acknowledge write |",
            "|---|---|---|---|---|---|"]
    for c in doc["rx_ops"]:
        st = rx_parse(c["word"], c["configured"])
        get = st["get_index"] if st else 0
        fill = st["fill"] if st else 0
        at = f"{c['at']}" if c["at_ok"] else "refused"
        ack = (f"`0x{c['ack']:02X}` (index {c['ack']})" if c["ack_ok"]
               else "refused")
        out.append(f"| {c['label']} | {get} | {fill} | {c['n']} | {at} | "
                   f"{ack} |")

    out += ["", "### Filling and sending", "",
            "| Case | Buffer to fill | Add request write | Still pending |",
            "|---|---|---|---|"]
    for c in doc["tx_ops"]:
        put = f"{c['put']}" if c["put_ok"] else "refused"
        mask = (f"`0x{c['mask']:02X}` (bit {c['index']})" if c["mask_ok"]
                else "refused")
        pend = ("yes" if c["pending"] else "no") if c["pending_ok"] \
            else "refused"
        out.append(f"| {c['label']} | {put} | {mask} | {pend} |")
    return "\n".join(out)


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

    n = len(doc["parse"]) + len(doc["rx_ops"]) + len(doc["tx_ops"])
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
        print(f"fifo vectors match the generator ({n} cases over the three "
              f"tables)")
        return 0

    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: {n} cases over "
          f"the three tables")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
