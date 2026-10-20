#!/usr/bin/env python3
"""The queue protocol, against its vectors and against its invariants.

    python test/test_fifo.py

The vector comparison catches drift. The invariants catch the ways this protocol
goes wrong while every register write still succeeds:

  - the acknowledge index written as a mask, or the add request as an index
  - an index that wraps at the width of the field instead of the queue size
  - a read at the get index of an empty queue, which returns a frame twice
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_fifo as g                                             # noqa: E402
import gen_msgaddr as ma                                         # noqa: E402
import gen_msgram as mr                                          # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_fifo.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def check_parse(doc):
    for c in doc["parse"]:
        st = (g.rx_parse(c["word"], c["configured"]) if c["side"] == "rx"
              else g.tx_parse(c["word"], c["configured"]))
        if (st is not None) != c["ok"]:
            fail(f"{c['label']}: parsed is {st is not None}, the file says "
                 f"{c['ok']}")
            continue
        if st is None:
            if not c["label"].startswith("REFUSE"):
                fail(f"{c['label']}: refused but not labelled as a refusal")
            continue
        if c["label"].startswith("REFUSE"):
            fail(f"{c['label']}: labelled a refusal and accepted")
        want = {"get_index": c["get_index"], "put_index": c["put_index"],
                "full": c["full"]}
        if c["side"] == "rx":
            want["fill"] = c["fill"]
            want["lost"] = c["lost"]
        else:
            want["free"] = c["free"]
        if st != want:
            fail(f"{c['label']}: parsed {st} against the file's {want}")


def check_ops(doc):
    for c in doc["rx_ops"]:
        st = g.rx_parse(c["word"], c["configured"])
        at = g.rx_at(st, c["configured"], c["n"])
        ack = g.rx_ack_index(st, c["configured"], c["n"] + 1)
        if (at is not None) != c["at_ok"] or \
                (at is not None and at != c["at"]):
            fail(f"{c['label']}: element to read is {at}, the file says "
                 f"{c['at'] if c['at_ok'] else 'refused'}")
        if (ack is not None) != c["ack_ok"] or \
                (ack is not None and ack != c["ack"]):
            fail(f"{c['label']}: acknowledge is {ack}, the file says "
                 f"{c['ack'] if c['ack_ok'] else 'refused'}")
        # The acknowledge that follows reading element n is the index of
        # element n itself, because the write releases through it.
        if at is not None and ack is not None and at != ack:
            fail(f"{c['label']}: read element {at} and the acknowledge write "
                 f"is {ack}, so the release does not include what was read")

    for c in doc["tx_ops"]:
        st = g.tx_parse(c["word"], c["configured"])
        put = g.tx_put(st)
        mask = g.tx_request_mask(c["index"], c["configured"])
        pend = g.tx_pending(c["pending_word"], c["index"], c["configured"])
        if (put is not None) != c["put_ok"] or \
                (put is not None and put != c["put"]):
            fail(f"{c['label']}: buffer to fill is {put}, the file says "
                 f"{c['put'] if c['put_ok'] else 'refused'}")
        if (mask is not None) != c["mask_ok"] or \
                (mask is not None and mask != c["mask"]):
            fail(f"{c['label']}: add request is {mask}, the file says "
                 f"{c['mask'] if c['mask_ok'] else 'refused'}")
        if (pend is not None) != c["pending_ok"] or \
                (pend is not None and pend != c["pending"]):
            fail(f"{c['label']}: pending is {pend}, the file says "
                 f"{c['pending'] if c['pending_ok'] else 'refused'}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    check_parse(doc)
    check_ops(doc)

    rx0 = doc["rx_fifo0"]
    txb = doc["tx_buffers"]

    # ---------------------------------------------------------- the invariants

    # 1. THE ENCODINGS MUST NEVER AGREE. The acknowledge register takes an
    # index and the add request register takes a bit per buffer, so a value
    # written to the wrong one of the two is always wrong and never accidentally
    # right. If any index made the two equal, a swap would pass a test by luck
    # on that index and fail on every other, which is the worst way to find out.
    for index in range(txb):
        mask = g.tx_request_mask(index, txb)
        if mask == index:
            fail(f"buffer {index}: the add request mask and the acknowledge "
                 f"index are both {index}, so writing one to the other's "
                 f"register would be indistinguishable from correct")
        if mask != 1 << index:
            fail(f"buffer {index}: the add request mask is {mask} and a bit "
                 f"per buffer is {1 << index}")

    # 2. Exhaustive over every status word this queue can produce. Each field
    # must come back as it went in, and acceptance must depend on the full flag
    # agreeing with the fill level and on nothing else.
    checked = 0
    for fill in range(rx0 + 1):
        for get in range(rx0):
            for put in range(rx0):
                for full in (False, True):
                    word = g.rx_word(fill, get, put, full=full)
                    st = g.rx_parse(word, rx0)
                    want_ok = (full == (fill == rx0))
                    checked += 1
                    if (st is not None) != want_ok:
                        fail(f"fill {fill} get {get} put {put} full {full}: "
                             f"parsed is {st is not None} and the flag and the "
                             f"level {'agree' if want_ok else 'disagree'}")
                        continue
                    if st is None:
                        continue
                    if (st["fill"], st["get_index"], st["put_index"],
                            st["full"]) != (fill, get, put, full):
                        fail(f"fill {fill} get {get} put {put}: came back as "
                             f"{st}")

    # 3. The lost flag must decode on its own. A frame is dropped once and the
    # flag stays set, so a decoder that let it disturb another field would turn
    # one old loss into a queue that reads wrong from then on.
    for fill in range(rx0 + 1):
        plain = g.rx_parse(g.rx_word(fill, 1, 2, full=(fill == rx0)), rx0)
        marked = g.rx_parse(
            g.rx_word(fill, 1, 2, full=(fill == rx0), lost=True), rx0)
        if plain is None or marked is None:
            fail(f"fill {fill}: a legal word was refused while checking the "
                 f"lost flag")
            continue
        if plain["lost"] or not marked["lost"]:
            fail(f"fill {fill}: the lost flag did not decode")
        if {k: v for k, v in plain.items() if k != "lost"} != \
                {k: v for k, v in marked.items() if k != "lost"}:
            fail(f"fill {fill}: setting the lost flag changed another field")

    # 4. Reading a FULL queue must visit every element exactly once. That is
    # the wrap, stated as a property rather than as one example: a wrap at the
    # width of the field instead of the queue size revisits some elements and
    # never reaches others, and both reads succeed.
    for get in range(rx0):
        st = g.rx_parse(g.rx_word(rx0, get, get, full=True), rx0)
        if st is None:
            fail(f"a full queue with get index {get} was refused")
            continue
        seen = [g.rx_at(st, rx0, n) for n in range(rx0)]
        if None in seen:
            fail(f"get index {get}: a full queue refused one of its own "
                 f"elements")
            continue
        if sorted(seen) != list(range(rx0)):
            fail(f"get index {get}: reading a full queue visits {seen}, which "
                 f"is not every element exactly once")

    # 5. Every index the protocol hands out must be an index the message memory
    # can address. This is the one rule that spans two files: a wrap at 64
    # instead of 16 produces indices msgaddr refuses, and without this rule the
    # two modules would each be right on their own terms.
    layout = mr.LAYOUT
    m = mr.compute(layout)
    for get in range(rx0):
        st = g.rx_parse(g.rx_word(rx0, get, get, full=True), rx0)
        for n in range(rx0):
            index = g.rx_at(st, rx0, n)
            if ma.offset_of(layout, "rx_fifo0", index) is None:
                fail(f"the protocol offers receive element {index} and the "
                     f"message memory refuses to address it")
    for index in range(txb):
        if ma.offset_of(layout, "tx_buffers", index) is None:
            fail(f"the protocol offers transmit buffer {index} and the "
                 f"message memory refuses to address it")
    if m["sections"] and not m["ok"]:
        fail("this node's layout is refused, so none of the above is reachable")

    # 6. An empty queue refuses every n, including 0. This is the read that
    # returns a frame already delivered, and the get index of an empty queue is
    # a perfectly ordinary number.
    for get in range(rx0):
        st = g.rx_parse(g.rx_word(0, get, get), rx0)
        for n in range(rx0 + 1):
            if g.rx_at(st, rx0, n) is not None:
                fail(f"an empty queue with get index {get} offered element "
                     f"{n}")
        if g.rx_ack_index(st, rx0, 0) is not None or \
                g.rx_ack_index(st, rx0, 1) is not None:
            fail(f"an empty queue with get index {get} composed an "
                 f"acknowledge")

    # 7. A transmit queue refuses a put exactly when it is full, and offers an
    # addressable index otherwise.
    for free in range(txb + 1):
        st = g.tx_parse(g.tx_word(free, 0, 3, full=(free == 0)), txb)
        if st is None:
            fail(f"free level {free}: a legal transmit word was refused")
            continue
        put = g.tx_put(st)
        if (put is None) != (free == 0):
            fail(f"free level {free}: put is "
                 f"{'refused' if put is None else put} and the queue is "
                 f"{'full' if free == 0 else 'not full'}")
        if put is not None and put >= txb:
            fail(f"free level {free}: put index {put} is past the {txb} "
                 f"buffers configured")

    # 8. An index past what the layout configured is refused by both transmit
    # composers, even though the registers themselves could address it.
    for index in range(txb, g.TX_INDEX_LIMIT + 1):
        if g.tx_request_mask(index, txb) is not None:
            fail(f"buffer {index} was given an add request mask and the "
                 f"layout configures {txb}")
        if g.tx_pending(0xFFFFFFFF, index, txb) is not None:
            fail(f"buffer {index} was read from the pending word and the "
                 f"layout configures {txb}")

    if failures:
        print(f"test_fifo: {len(failures)} failure(s)")
        return 1
    total = len(doc["parse"]) + len(doc["rx_ops"]) + len(doc["tx_ops"])
    print(f"ok  {total} cases and {checked} status words swept, every full "
          f"queue read visits all {rx0} elements once, both encodings distinct")
    return 0


if __name__ == "__main__":
    sys.exit(main())
