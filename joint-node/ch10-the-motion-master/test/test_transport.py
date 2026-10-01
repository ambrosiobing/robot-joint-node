#!/usr/bin/env python3
"""The socket, where there is one.

    python test/test_transport.py

On a machine with no SocketCAN this reports that it was skipped and why, and
exits zero. A skipped test that exits one trains people to ignore a red build;
a skipped test that pretends to pass is worse. It says which it did.

On Linux with the virtual interface up, it is a real test: two sockets on one
interface, a frame sent on one and received on the other, which is the whole
first half of the chapter working with no hardware anywhere.

    sudo modprobe vcan
    sudo ip link add dev vcan0 type vcan
    sudo ip link set up vcan0
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from master.frames import Frame                                  # noqa: E402
from master import transport                                     # noqa: E402

IFACE = "vcan0"


def skipped(reason):
    print(f"skip  {reason}")
    return 0


def main():
    if not transport.available():
        try:
            transport.require()
        except transport.Unsupported as e:
            return skipped(str(e).split(". ")[0] +
                           ". The frame layout and the log format were still checked.")

    try:
        rx = transport.Bus(IFACE, timeout=2.0)
    except OSError as e:
        return skipped(f"{IFACE} is not up ({e}). Bring it up with: "
                       f"sudo modprobe vcan; sudo ip link add dev {IFACE} type vcan; "
                       f"sudo ip link set up {IFACE}")

    failures = []
    with rx, transport.Bus(IFACE) as tx:
        for sent in (Frame(0x123, b"\x01\x02\x03"),
                     Frame(0x321, bytes(24), fd=True, brs=True)):
            tx.send(sent)
            got = rx.recv()
            if got != sent:
                failures.append(f"sent {sent} and received {got}")

        # A filter the kernel applies: everything but 0x123 is dropped before
        # this process is woken at all.
        rx.filter([(0x123, 0x7FF)])
        tx.send(Frame(0x456, b"\xff"))
        tx.send(Frame(0x123, b"\xaa"))
        got = rx.recv()
        if got.can_id != 0x123:
            failures.append(f"a filtered socket received {got.can_id:#x}")

    if failures:
        print(f"FAIL  {len(failures)} problems:")
        for f in failures:
            print("  " + f)
        return 1
    print(f"ok  two sockets on {IFACE}, classic and flexible-data, and a kernel filter")
    return 0


if __name__ == "__main__":
    sys.exit(main())
