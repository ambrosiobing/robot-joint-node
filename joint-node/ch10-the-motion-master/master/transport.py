"""The socket, and the only part of this chapter that needs a kernel.

Everything else in master/ is arithmetic and byte layout and runs anywhere.
This file is separated so that the rest stays testable on a laptop with no CAN
support at all, which is most laptops.

The interface name is the only thing that changes when real hardware arrives:

    Bus("vcan0")    the virtual interface, no hardware anywhere
    Bus("can0")     the adapter, once it is seated and the driver is bound

Nothing else in this chapter knows the difference, which is the point of doing
the host half first.
"""
import socket
import struct

from .frames import CAN_MTU, CANFD_MTU, decode

CAN_RAW_FD_FRAMES = 5          # setsockopt: accept flexible-data frames too
CAN_RAW_FILTER = 1
SO_TIMESTAMPNS = 35


class Unsupported(RuntimeError):
    """This machine has no SocketCAN. Not a failure of the code under test."""


def available():
    return hasattr(socket, "AF_CAN") and hasattr(socket, "CAN_RAW")


def require():
    if not available():
        raise Unsupported(
            "this Python has no AF_CAN, so there is no SocketCAN here. "
            "The frame layout and the log format are still testable; the socket "
            "is not. Run this part on the Pi 4, or any Linux with the vcan "
            "module: modprobe vcan; ip link add dev vcan0 type vcan; "
            "ip link set up vcan0")


class Bus:
    """A raw CAN socket bound to one interface."""

    def __init__(self, interface="vcan0", fd=True, timeout=None):
        require()
        self.interface = interface
        self.fd = fd
        self.sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        if fd:
            # Without this the kernel truncates every flexible-data frame to a
            # classic one, silently, and the payload above eight bytes is gone
            # before any of this chapter's code sees it.
            self.sock.setsockopt(socket.SOL_CAN_RAW, CAN_RAW_FD_FRAMES, 1)
        if timeout is not None:
            self.sock.settimeout(timeout)
        self.sock.bind((interface,))

    def filter(self, pairs):
        """Accept only these (identifier, mask) pairs.

        Filtering here means the kernel drops the rest before it reaches this
        process. On a four joint bus that is the difference between waking for
        every frame and waking for the three that are addressed to this node.
        """
        blob = b"".join(struct.pack("=II", can_id, mask) for can_id, mask in pairs)
        self.sock.setsockopt(socket.SOL_CAN_RAW, CAN_RAW_FILTER, blob)

    def send(self, frame):
        self.sock.send(frame.encode())

    def recv(self):
        buf = self.sock.recv(CANFD_MTU if self.fd else CAN_MTU)
        return decode(buf)

    def close(self):
        self.sock.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()
