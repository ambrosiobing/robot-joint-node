#!/usr/bin/env python3
"""The log format, and the deliberate gap.

    python test/test_record.py

A recording is only useful if the file is the one the standard tools write, and
if replaying it twice produces the same traffic twice. The second property is
what makes a dropped frame reproducible, and a dropped frame is how chapter 11's
gap detection is demonstrated.
"""
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from master.frames import Frame, FrameError                      # noqa: E402
from master import record                                        # noqa: E402

failures = []


def check(cond, what):
    if not cond:
        failures.append(what)


def test_line_shapes():
    """Written against lines the standard logger produces, not against our own
    output, or the test would only prove this file agrees with itself."""
    known = [
        ("(1696118400.123456) vcan0 123#DEAD", 0x123, b"\xde\xad", False, False),
        ("(1696118400.123456) vcan0 7FF#", 0x7FF, b"", False, False),
        ("(1696118400.123456) vcan0 1A3##1" + "11" * 12, 0x1A3, b"\x11" * 12, True, True),
        ("(1696118400.123456) can0 00000456#01", 0x456, b"\x01", False, False),
    ]
    for line, can_id, data, fd, brs in known:
        frame, iface = record.from_line(line)
        check(frame.can_id == can_id, f"{line}: identifier came back {frame.can_id:#x}")
        check(frame.data == data, f"{line}: payload came back {frame.data.hex()}")
        check(frame.fd == fd, f"{line}: flexible-data flag came back {frame.fd}")
        check(frame.brs == brs, f"{line}: rate switch came back {frame.brs}")
        check(iface in ("vcan0", "can0"), f"{line}: interface came back {iface}")

    # The last one has an eight digit identifier, so it is an extended frame.
    frame, _ = record.from_line("(1.0) can0 00000456#01")
    check(frame.extended, "an eight digit identifier did not come back extended")


def test_round_trip_through_a_file():
    frames = [Frame(0x200 + i, bytes([i]) * (i % 8), timestamp=1000.0 + i / 1000)
              for i in range(10)]
    frames.append(Frame(0x300, bytes(24), fd=True, brs=True, timestamp=1001.0))

    with tempfile.TemporaryDirectory() as d:
        path = Path(d) / "run.log"
        record.write(path, frames)
        back = record.read(path)

    check(len(back) == len(frames), f"wrote {len(frames)} frames and read {len(back)}")
    for a, b in zip(frames, back):
        check(a == b, f"{a} came back as {b}")
        check(abs((a.timestamp or 0) - (b.timestamp or 0)) < 1e-6,
              f"timestamp drifted on {a}")


def test_replay_is_repeatable():
    """The same file read twice gives the same frames. Obvious, and it is the
    property the whole idea of replay rests on."""
    frames = [Frame(0x100, bytes([i]), timestamp=1.0 + i) for i in range(5)]
    with tempfile.TemporaryDirectory() as d:
        path = Path(d) / "a.log"
        record.write(path, frames)
        check(record.read(path) == record.read(path), "two reads of one file differ")


def test_deliberate_gap():
    frames = [Frame(0x100, bytes([i]), timestamp=float(i)) for i in range(10)]
    kept, dropped = record.drop(frames, every=3)

    check(len(kept) == 7, f"dropping every third of ten left {len(kept)}, expected 7")
    check(dropped == [2, 5, 8], f"dropped positions were {dropped}, expected [2, 5, 8]")
    check(all(f in frames for f in kept), "drop invented a frame that was not there")

    try:
        record.drop(frames, every=1)
        failures.append("dropping every frame was accepted as a gap")
    except ValueError:
        pass


def test_refusals():
    for bad in ("not a log line at all",
                "(1696118400.123456) vcan0 123#DEA",        # odd hex digits
                "(1696118400.123456) vcan0 1A3##",          # no flags nibble
                "1696118400.123456 vcan0 123#DEAD"):        # no brackets
        try:
            record.from_line(bad)
            failures.append(f"accepted {bad!r}, and it should not have")
        except FrameError:
            pass


def main():
    for fn in (test_line_shapes, test_round_trip_through_a_file, test_replay_is_repeatable,
               test_deliberate_gap, test_refusals):
        fn()
    if failures:
        print(f"FAIL  {len(failures)} problems:")
        for f in failures:
            print("  " + f)
        return 1
    print("ok  log format, file round trip, repeatable replay and a deliberate gap")
    return 0


if __name__ == "__main__":
    sys.exit(main())
