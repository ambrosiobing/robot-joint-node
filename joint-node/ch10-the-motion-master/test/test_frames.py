#!/usr/bin/env python3
"""The frame layout, on any machine.

    python test/test_frames.py

None of this opens a socket, so it runs on the authoring laptop. What it checks
is the part that is wrong silently: a frame whose identifier overflowed into the
flag bits, a payload length that cannot exist, a flexible-data frame decoded as
a classic one because the socket option was never set.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from master.frames import (CAN_EFF_FLAG, CANFD_BRS, CANFD_FDF, CAN_MTU, CANFD_MTU,
                           Frame, FrameError, decode, format_frame)   # noqa: E402

failures = []


def check(cond, what):
    if not cond:
        failures.append(what)


def refuses(fn, what):
    try:
        fn()
    except FrameError:
        return
    failures.append(f"accepted {what}, and it should not have")


def test_sizes():
    check(len(Frame(0x123, b"\x01\x02").encode()) == CAN_MTU,
          "a classic frame is not 16 bytes on the wire")
    check(len(Frame(0x123, bytes(24), fd=True).encode()) == CANFD_MTU,
          "a flexible-data frame is not 72 bytes on the wire")


def test_round_trip():
    for frame in (Frame(0x000, b""),
                  Frame(0x7FF, bytes(range(8))),
                  Frame(0x123, bytes(24), fd=True, brs=True),
                  Frame(0x1FFFFFFF, bytes(64), fd=True, brs=True, esi=True, extended=True),
                  Frame(0x1, b"\xff" * 12, fd=True)):
        got = decode(frame.encode())
        check(got == frame, f"round trip changed {format_frame(frame)} into {format_frame(got)}")


def test_identifier_flags():
    """The extended bit lives above the identifier, so a standard frame and an
    extended frame with the same number are different frames on the wire."""
    std = Frame(0x123, b"")
    ext = Frame(0x123, b"", extended=True)
    check(std.raw_id == 0x123, "a standard identifier gained a flag bit")
    check(ext.raw_id == (0x123 | CAN_EFF_FLAG), "an extended frame lost its flag bit")
    check(decode(ext.encode()).extended, "the extended flag did not survive a round trip")
    check(not decode(std.encode()).extended, "a standard frame came back extended")


def test_refusals():
    refuses(lambda: Frame(0x800, b""), "an identifier too wide for 11 bits")
    refuses(lambda: Frame(0x20000000, b"", extended=True), "an identifier too wide for 29 bits")
    refuses(lambda: Frame(0x123, bytes(9)), "a 9 byte classic frame")
    refuses(lambda: Frame(0x123, bytes(9), fd=True), "a 9 byte flexible-data frame")
    refuses(lambda: Frame(0x123, bytes(63), fd=True), "a 63 byte flexible-data frame")
    refuses(lambda: Frame(0x123, b"", brs=True), "a rate switch on a classic frame")
    refuses(lambda: decode(bytes(20)), "a buffer that is neither frame size")


def test_fd_flags_are_in_the_frame_not_the_id():
    f = Frame(0x123, bytes(16), fd=True, brs=True)
    raw = f.encode()
    check(raw[5] & CANFD_FDF, "the flexible-data flag is not in the flags byte")
    check(raw[5] & CANFD_BRS, "the rate switch flag is not in the flags byte")
    check(decode(raw).brs, "the rate switch did not survive a round trip")


def test_printed_form():
    check(format_frame(Frame(0x123, b"\xde\xad")) == "123#DEAD",
          "a classic frame does not print the way the tools print it")
    check(format_frame(Frame(0x123, bytes(12), fd=True, brs=True)) == "123##1" + "00" * 12,
          "a flexible-data frame does not print the way the tools print it")


def main():
    for fn in (test_sizes, test_round_trip, test_identifier_flags, test_refusals,
               test_fd_flags_are_in_the_frame_not_the_id, test_printed_form):
        fn()
    if failures:
        print(f"FAIL  {len(failures)} problems:")
        for f in failures:
            print("  " + f)
        return 1
    print("ok  frame layout, identifier flags, length rules and printed form")
    return 0


if __name__ == "__main__":
    sys.exit(main())
