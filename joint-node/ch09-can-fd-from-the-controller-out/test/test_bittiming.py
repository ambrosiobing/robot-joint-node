#!/usr/bin/env python3
"""The bit timing reference, checked against its own vectors and its invariants.

    python test/test_bittiming.py

This is the half that runs on the win11 aquamarine authoring laptop. The C half,
test/test_bittiming.c, checks the same vector file and has never been compiled,
so a green run here does not say the C is right. It says the vectors are right,
which is what the C will be judged against.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_bt_vectors as g                                  # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    cases = doc["cases"]
    accepted = refused = 0

    for c in cases:
        lim = g.DATA if c["phase"] == "data" else g.NOMINAL
        r = g.bt_compute(c["kernel_hz"], c["bitrate"],
                         c["want_sp_permille"], lim)

        # The vector file says what the reference produced. If it no longer
        # produces it, the file is stale and --check would have caught it; this
        # catches the case where somebody edits the reference and regenerates
        # without looking at what changed.
        if (r is not None) != c["ok"]:
            fail(f"{c['label']}: solved is {r is not None}, "
                 f"the file says {c['ok']}")
            continue
        if r != c["result"]:
            fail(f"{c['label']}: {r} does not match the file's {c['result']}")
            continue

        if not c["ok"]:
            refused += 1
            continue
        accepted += 1

        # Invariants that have to hold whatever the inputs were.
        if 1 + r["seg1"] + r["seg2"] != r["tq_per_bit"]:
            fail(f"{c['label']}: 1 + {r['seg1']} + {r['seg2']} is not "
                 f"{r['tq_per_bit']} quanta")
        if not (lim.prescaler[0] <= r["prescaler"] <= lim.prescaler[1]):
            fail(f"{c['label']}: prescaler {r['prescaler']} is outside "
                 f"{lim.prescaler}, which no register can hold")
        if not (lim.seg1[0] <= r["seg1"] <= lim.seg1[1]):
            fail(f"{c['label']}: seg1 {r['seg1']} is outside {lim.seg1}")
        if not (lim.seg2[0] <= r["seg2"] <= lim.seg2[1]):
            fail(f"{c['label']}: seg2 {r['seg2']} is outside {lim.seg2}")
        if r["sjw"] > lim.sjw[1] or r["sjw"] > r["seg2"]:
            fail(f"{c['label']}: jump width {r['sjw']} exceeds seg2 "
                 f"{r['seg2']} or its limit {lim.sjw[1]}")
        if r["tq_per_bit"] < lim.min_tq:
            fail(f"{c['label']}: {r['tq_per_bit']} quanta is below the "
                 f"floor of {lim.min_tq}")

        # The register word, and then the same four numbers read back out of
        # it. A solved timing that cannot be written is not a solved timing,
        # and a word that does not unpack to what went in is an off-by-one in
        # a shift that no amount of staring at the arithmetic would find.
        fields = g.DBTP_FIELDS if c["phase"] == "data" else g.NBTP_FIELDS
        packer = g.pack_dbtp if c["phase"] == "data" else g.pack_nbtp
        word = packer(r)
        if word is None:
            fail(f"{c['label']}: solved, but will not fit its register")
        elif word != c["register_word"]:
            fail(f"{c['label']}: packs to 0x{word:08X}, the file says "
                 f"0x{c['register_word']:08X}")
        else:
            back = g.unpack(word, fields)
            for key in ("prescaler", "seg1", "seg2", "sjw", "tq_per_bit",
                        "sample_point_permille"):
                if back[key] != r[key]:
                    fail(f"{c['label']}: {key} went in as {r[key]} and came "
                         f"back out of 0x{word:08X} as {back[key]}")

        # The bit rate has to come back out exactly. This is the property the
        # whole refusal rule exists to protect, so it is worth asserting rather
        # than trusting the loop that produced it.
        tq_hz = c["kernel_hz"] // r["prescaler"]
        if c["kernel_hz"] % r["prescaler"]:
            fail(f"{c['label']}: prescaler {r['prescaler']} does not divide "
                 f"the kernel clock exactly")
        elif tq_hz // r["tq_per_bit"] != c["bitrate"] or \
                tq_hz % r["tq_per_bit"]:
            fail(f"{c['label']}: the timing gives "
                 f"{tq_hz / r['tq_per_bit']:.3f} bit/s, not {c['bitrate']}")

    # The two design points chapter 9 commits to in its budget table have to
    # land on their stated sample points exactly, not nearly.
    for label, want in (("design nominal, 80 MHz", 800),
                        ("design data, 80 MHz", 750)):
        c = next(x for x in cases if x["label"] == label)
        if not c["ok"]:
            fail(f"{label}: refused, and the chapter's budget table depends "
                 f"on it solving")
        elif c["result"]["sample_point_permille"] != want:
            fail(f"{label}: sample point is "
                 f"{c['result']['sample_point_permille'] / 10} per cent, and "
                 f"the budget table says {want / 10}")

    if refused == 0:
        fail("not one case was refused, so the refusal path is untested")

    # The packing has its own refusal path, and it is the one that matters most,
    # because the failure it prevents is silent. A nominal timing at 80 MHz has
    # a segment 1 of 127; the data field is five bits wide and would truncate it
    # to 31, configuring a bit rate nobody chose with no error reported. So
    # packing a nominal timing as a data word must refuse rather than truncate.
    wide = next(x for x in cases if x["label"] == "design nominal, 80 MHz")
    if g.pack_dbtp(wide["result"]) is not None:
        fail("a nominal timing packed as a data word was accepted, and "
             "truncating it silently is the failure this guard exists for")

    # And the other direction is legal, because a data timing is small. This is
    # here so the guard above is known to be refusing for the right reason
    # rather than refusing everything.
    narrow = next(x for x in cases if x["label"] == "design data, 80 MHz")
    if g.pack_nbtp(narrow["result"]) is None:
        fail("a data timing refused by the nominal packer, which has wider "
             "fields, so the packer is rejecting valid input")

    # The transmitter delay compensation bit is the only thing in these two
    # words that is not a timing, so it is checked separately: setting it must
    # change exactly one bit and nothing else.
    plain = g.pack_dbtp(narrow["result"], tdc=False)
    with_tdc = g.pack_dbtp(narrow["result"], tdc=True)
    if plain ^ with_tdc != g.DBTP_TDC:
        fail(f"the TDC flag changed 0x{plain ^ with_tdc:08X} rather than "
             f"only 0x{g.DBTP_TDC:08X}")

    if failures:
        print(f"test_bittiming: {len(failures)} failure(s)")
        return 1
    print(f"ok  {len(cases)} vectors, {accepted} solved and {refused} refused, "
          f"both design points exact")
    return 0


if __name__ == "__main__":
    sys.exit(main())
