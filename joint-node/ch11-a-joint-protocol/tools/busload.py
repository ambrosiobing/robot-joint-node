#!/usr/bin/env python3
"""Bus load arithmetic, done before believing the design.

    python tools/busload.py --joints 4 --rate 1000 --nominal 500000 --data 2000000
    python tools/busload.py --audit-chapter

A frame is not its payload. There is arbitration at the slow rate, a checksum,
an acknowledgement and a gap before the next frame. The model below is stated
rather than hidden, because the answer is only as good as its assumptions and
the assumptions are the part worth arguing with.

Model
-----
Every frame costs a fixed number of bits in the arbitration phase, which runs
at the nominal rate whatever the data rate is, plus the payload and a fixed
overhead in the data phase. Chapter 11 gives both constants directly: 67
arbitration bits, and a data phase of the payload plus 48 bits. This tool
adopts those two numbers so its per-frame figures can be checked against the
chapter, and exposes them as options so a different model can be tried.

The arithmetic is approximate and it is the right kind of approximate: it is
wrong in the direction of pessimism, and it is wrong by a few per cent rather
than by a factor.
"""
import argparse
import sys

ARB_BITS_DEFAULT = 67        # identifier, control, and the trailing fields
DATA_OVERHEAD_DEFAULT = 48   # checksum, stuff count, delimiters, in the data phase

# What chapter 11 step 7 prints, for the audit mode below.
#
# These three option figures were 91, 61 and 38 until Tuesday 6 October 2026,
# and they were wrong. The chapter printed them inside a block that claims to be
# this program's output, so the quickest way to settle it was to run the command
# the chapter itself quotes. The per-frame costs and the baseline were right and
# are unchanged; only the options were wrong, and they were wrong in the
# direction that flatters the design. The chapter now reproduces what the tool
# prints, and this table is what makes the two stay together.
CHAPTER = {
    "state_us": 254.0,
    "command_us": 222.0,
    "baseline_pct": 190.0,
    "options": {
        "command at 250 Hz": 124.0,
        "arbitration at 1 Mbit/s": 137.0,
        "both": 90.0,
    },
}


def frame_us(payload_bytes, nominal, data, arb_bits, data_overhead):
    """Microseconds on the wire for one frame."""
    arb_s = arb_bits / nominal
    data_s = (payload_bytes * 8 + data_overhead) / data
    return (arb_s + data_s) * 1e6


def load(joints, state_rate, command_rate, state_us, command_us):
    """Fraction of one second the bus is busy."""
    return joints * (state_rate * state_us + command_rate * command_us) / 1e6


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--joints", type=int, default=4)
    ap.add_argument("--rate", type=int, default=1000, help="control rate in Hz")
    ap.add_argument("--nominal", type=int, default=500_000, help="arbitration bit rate")
    ap.add_argument("--data", type=int, default=2_000_000, help="data phase bit rate")
    ap.add_argument("--state-bytes", type=int, default=24)
    ap.add_argument("--command-bytes", type=int, default=16)
    ap.add_argument("--arb-bits", type=int, default=ARB_BITS_DEFAULT)
    ap.add_argument("--data-overhead", type=int, default=DATA_OVERHEAD_DEFAULT)
    ap.add_argument("--audit-chapter", action="store_true",
                    help="compare this model against the figures printed in chapter 11")
    a = ap.parse_args(argv)

    def frames(nominal):
        return (frame_us(a.state_bytes, nominal, a.data, a.arb_bits, a.data_overhead),
                frame_us(a.command_bytes, nominal, a.data, a.arb_bits, a.data_overhead))

    s_us, c_us = frames(a.nominal)

    print(f"model: {a.arb_bits} arbitration bits at {a.nominal/1000:g} kbit/s, "
          f"payload plus {a.data_overhead} bits at {a.data/1e6:g} Mbit/s")
    print(f"per joint per period: state {a.state_bytes} B, command {a.command_bytes} B")
    print(f"state frame:   {a.arb_bits} arbitration bits + "
          f"{a.state_bytes * 8 + a.data_overhead} data bits -> {s_us:.0f} us")
    print(f"command frame: {a.arb_bits} arbitration bits + "
          f"{a.command_bytes * 8 + a.data_overhead} data bits -> {c_us:.0f} us")
    print()

    baseline = load(a.joints, a.rate, a.rate, s_us, c_us)
    period_ms = 1000 / a.rate
    print(f"{a.joints} joints, both directions, {a.rate} Hz: "
          f"{baseline * period_ms:.2f} ms of every {period_ms:.2f} ms "
          f"-> {baseline * 100:.0f}%")

    if baseline <= 1.0:
        print("FITS")
        return 0

    print("DOES NOT FIT. Options, in order of preference:")
    s_fast, c_fast = frames(1_000_000)
    options = [
        # Kept short on purpose: this output is quoted verbatim in chapter 11,
        # and the house limit for a code block there is 96 columns.
        ("state at {r} Hz, command at {q} Hz, interpolated on the node".format(
            r=a.rate, q=a.rate // 4),
         load(a.joints, a.rate, a.rate // 4, s_us, c_us)),
        ("raise the arbitration rate to 1 Mbit/s",
         load(a.joints, a.rate, a.rate, s_fast, c_fast)),
        ("both",
         load(a.joints, a.rate, a.rate // 4, s_fast, c_fast)),
    ]
    for label, frac in options:
        verdict = "" if frac <= 1.0 else "   still does not fit"
        print(f"  {label:<62} -> {frac * 100:3.0f}%{verdict}")

    if a.audit_chapter:
        print()
        print("audit against chapter 11 step 7")
        rows = [("state frame us", s_us, CHAPTER["state_us"]),
                ("command frame us", c_us, CHAPTER["command_us"]),
                ("baseline per cent", baseline * 100, CHAPTER["baseline_pct"])]
        for label, (opt_label, frac) in zip(CHAPTER["options"], options):
            rows.append((f"option: {label}", frac * 100, CHAPTER["options"][label]))
        bad = 0
        for label, mine, theirs in rows:
            delta = mine - theirs
            agree = abs(delta) < 1.0
            if not agree:
                bad += 1
            print(f"  {label:<46} this tool {mine:7.1f}   chapter {theirs:7.1f}   "
                  f"{'agree' if agree else f'DIFFER by {delta:+.1f}'}")
        if bad:
            print(f"\n  {bad} figures disagree. The chapter prints these numbers inside a")
            print("  block that claims to be this program's output, so they are not two")
            print("  opinions: either the chapter was edited without rerunning the command")
            print("  it quotes, or the model here changed under it. Run the command in the")
            print("  chapter, compare, and correct whichever one moved. Quote neither")
            print("  number until they match.")
        else:
            print("\n  the chapter and this tool agree on every figure")
        return 1 if bad else 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
