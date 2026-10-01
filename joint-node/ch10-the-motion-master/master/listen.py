#!/usr/bin/env python3
"""Watch a bus, and optionally write what it saw to a file.

    python -m master.listen vcan0
    python -m master.listen can0 --out run.log --seconds 10

Deliberately dumb about meaning. It prints identifiers and bytes, not named
signals, because the names belong to the protocol description in chapter 11 and
this chapter is the socket layer underneath it. Keeping the two apart is what
lets this tool watch traffic from a node whose protocol it has never been told.
"""
import argparse
import sys
import time

from .frames import format_frame
from . import record, transport


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("interface", nargs="?", default="vcan0")
    ap.add_argument("--out", help="write a log file the standard tools can read")
    ap.add_argument("--seconds", type=float, help="stop after this long")
    ap.add_argument("--classic", action="store_true",
                    help="classic frames only, which truncates anything longer")
    a = ap.parse_args(argv)

    try:
        transport.require()
    except transport.Unsupported as e:
        print(e, file=sys.stderr)
        return 2

    deadline = time.time() + a.seconds if a.seconds else None
    seen = 0
    frames = []

    try:
        with transport.Bus(a.interface, fd=not a.classic, timeout=0.5) as bus:
            while deadline is None or time.time() < deadline:
                try:
                    frame = bus.recv()
                except TimeoutError:
                    continue
                except OSError:
                    continue
                frame.timestamp = time.time()
                seen += 1
                print(f"({frame.timestamp:.6f}) {a.interface} {format_frame(frame)}")
                if a.out:
                    frames.append(frame)
    except KeyboardInterrupt:
        pass

    if a.out and frames:
        record.write(a.out, frames, a.interface)
        print(f"{len(frames)} frames written to {a.out}", file=sys.stderr)

    print(f"{seen} frames seen on {a.interface}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
