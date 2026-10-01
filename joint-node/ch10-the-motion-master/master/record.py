"""Record frames to a file and play them back.

The format is the one the standard logger writes, so a recording made here can
be read by tools that have never heard of this repository, and a recording made
by those tools can be replayed by this one:

    (1696118400.123456) vcan0 123#DEADBEEF
    (1696118400.124556) vcan0 1A3##1112233445566778899AABBCCDDEEFF00

Replay is what makes a missing frame reproducible. Chapter 11's sequence gap
detection is demonstrated by dropping a frame on purpose, and dropping it the
same way twice is only possible if the traffic came from a file rather than from
a bus that will never repeat itself exactly.
"""
import re
import time

from .frames import Frame, FrameError, format_frame

LINE = re.compile(r"^\((?P<ts>\d+\.\d+)\)\s+(?P<iface>\S+)\s+"
                  r"(?P<id>[0-9A-Fa-f]+)(?P<sep>##|#)(?P<rest>[0-9A-Fa-f]*)$")


def to_line(frame, interface="vcan0", timestamp=None):
    ts = timestamp if timestamp is not None else (frame.timestamp or time.time())
    return f"({ts:.6f}) {interface} {format_frame(frame)}"


def from_line(line):
    """One logged line back into a frame and the interface it came from."""
    m = LINE.match(line.strip())
    if not m:
        raise FrameError(f"not a log line: {line.strip()!r}")

    ident = m.group("id")
    extended = len(ident) > 3
    can_id = int(ident, 16)
    rest = m.group("rest")

    if m.group("sep") == "##":
        if not rest:
            raise FrameError(f"a flexible-data line needs a flags nibble: {line.strip()!r}")
        flags = int(rest[0], 16)
        body = rest[1:]
        if len(body) % 2:
            raise FrameError(f"odd number of hex digits in {line.strip()!r}")
        frame = Frame(can_id=can_id, data=bytes.fromhex(body), fd=True,
                      brs=bool(flags & 0x1), esi=bool(flags & 0x2),
                      extended=extended, timestamp=float(m.group("ts")))
    else:
        if len(rest) % 2:
            raise FrameError(f"odd number of hex digits in {line.strip()!r}")
        frame = Frame(can_id=can_id, data=bytes.fromhex(rest), fd=False,
                      extended=extended, timestamp=float(m.group("ts")))

    return frame, m.group("iface")


def write(path, frames, interface="vcan0"):
    with open(path, "w", encoding="ascii") as fh:
        for f in frames:
            fh.write(to_line(f, interface) + "\n")


def read(path):
    """Every frame in the file, in order, with the timestamps it was given."""
    out = []
    with open(path, encoding="ascii") as fh:
        for n, line in enumerate(fh, 1):
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            try:
                frame, _ = from_line(line)
            except FrameError as e:
                raise FrameError(f"{path}:{n}: {e}") from None
            out.append(frame)
    return out


def drop(frames, every):
    """Every nth frame removed, which is how a gap is made on purpose.

    Returns the kept frames and the sequence positions that were removed, so a
    test can assert that the detector found exactly the gaps that were made and
    not one more.
    """
    if every < 2:
        raise ValueError("dropping every frame, or every first frame, is not a gap")
    kept, dropped = [], []
    for i, f in enumerate(frames):
        if (i + 1) % every == 0:
            dropped.append(i)
        else:
            kept.append(f)
    return kept, dropped
