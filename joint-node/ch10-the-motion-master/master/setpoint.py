#!/usr/bin/env python3
"""A setpoint source: something for the node to listen to.

    python -m master.setpoint vcan0 --rate 1000 --seconds 5
    python -m master.setpoint vcan0 --replay run.log
    python -m master.setpoint vcan0 --replay run.log --drop-every 3

The payload here is opaque bytes on purpose. What a command frame means is
chapter 11's business; what this chapter owes is a source that emits at a
chosen rate, keeps its own schedule honestly, and can replay a file so a fault
is reproducible.

The rate it reports is the rate it achieved, not the rate it was asked for. A
generator that claims 1000 Hz while delivering 780 is the kind of instrument
that makes everything measured against it wrong.
"""
import argparse
import sys
import time

from .frames import Frame
from . import record, transport


def paced(interval, count):
    """Yield at a fixed wall clock cadence rather than sleeping a fixed amount.

    Sleeping the interval accumulates every scheduling delay; sleeping until the
    next deadline does not, which is the difference between drifting a second a
    minute and not drifting at all.
    """
    start = time.perf_counter()
    for i in range(count):
        target = start + i * interval
        now = time.perf_counter()
        if target > now:
            time.sleep(target - now)
        yield i


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("interface", nargs="?", default="vcan0")
    ap.add_argument("--id", type=lambda s: int(s, 0), default=0x101,
                    help="identifier to send on, default 0x101")
    ap.add_argument("--rate", type=float, default=100.0, help="frames per second")
    ap.add_argument("--seconds", type=float, default=1.0)
    ap.add_argument("--replay", help="send the frames in this log file instead")
    ap.add_argument("--drop-every", type=int,
                    help="with --replay, drop every nth frame on purpose")
    a = ap.parse_args(argv)

    try:
        transport.require()
    except transport.Unsupported as e:
        print(e, file=sys.stderr)
        return 2

    if a.replay:
        frames = record.read(a.replay)
        dropped = []
        if a.drop_every:
            frames, dropped = record.drop(frames, a.drop_every)
        with transport.Bus(a.interface) as bus:
            begin = time.perf_counter()
            base = frames[0].timestamp if frames and frames[0].timestamp else None
            for f in frames:
                if base is not None and f.timestamp is not None:
                    target = begin + (f.timestamp - base)
                    now = time.perf_counter()
                    if target > now:
                        time.sleep(target - now)
                bus.send(f)
        print(f"replayed {len(frames)} frames from {a.replay}", file=sys.stderr)
        if dropped:
            print(f"dropped {len(dropped)} on purpose, at positions {dropped}",
                  file=sys.stderr)
        return 0

    count = max(1, int(a.rate * a.seconds))
    interval = 1.0 / a.rate
    begin = time.perf_counter()
    with transport.Bus(a.interface) as bus:
        for i in paced(interval, count):
            bus.send(Frame(a.id, i.to_bytes(4, "little") + bytes(4)))
    elapsed = time.perf_counter() - begin

    achieved = count / elapsed if elapsed else 0.0
    print(f"{count} frames in {elapsed:.3f} s on {a.interface}: "
          f"{achieved:.1f} Hz achieved against {a.rate:.1f} Hz asked for",
          file=sys.stderr)
    if abs(achieved - a.rate) > 0.05 * a.rate:
        print("that is more than five per cent off, so do not quote the asked-for "
              "rate anywhere", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
