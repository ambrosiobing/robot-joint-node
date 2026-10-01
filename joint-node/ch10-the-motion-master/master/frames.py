"""The frame as the kernel lays it out, independent of any socket.

This module never opens anything. That is deliberate: the layout, the flags and
the length rules are the part worth testing, and they are testable on a laptop
with no CAN interface, no driver and no Linux. The socket lives next door in
transport.py and is the only thing that needs a kernel.

The two structures are the kernel's own:

    struct can_frame     16 bytes:  id u32, len u8, pad u8, res0 u8, res1 u8, data[8]
    struct canfd_frame   72 bytes:  id u32, len u8, flags u8, res0 u8, res1 u8, data[64]

The identifier carries three flag bits above the number itself, which is why an
identifier is never compared without masking first.
"""
import struct
from dataclasses import dataclass, field

# Flags that ride in the top bits of the identifier word.
CAN_EFF_FLAG = 0x80000000      # 29 bit identifier rather than 11
CAN_RTR_FLAG = 0x40000000      # remote request, no payload
CAN_ERR_FLAG = 0x20000000      # an error frame from the driver, not from a node

CAN_SFF_MASK = 0x000007FF
CAN_EFF_MASK = 0x1FFFFFFF

# Flags in the flexible-data frame's own byte.
CANFD_BRS = 0x01               # the data phase runs at the faster rate
CANFD_ESI = 0x02               # the sender is error passive
CANFD_FDF = 0x04               # this is a flexible-data frame

CAN_MTU = 16
CANFD_MTU = 72

# Above eight bytes the payload length jumps. A frame is built to land on one of
# these, and a length between them cannot be put on a wire at all.
FD_LENGTHS = (0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64)

_CLASSIC = struct.Struct("<IBBBB8s")
_FD = struct.Struct("<IBBBB64s")


class FrameError(ValueError):
    """A frame that could not exist on a wire."""


@dataclass
class Frame:
    can_id: int
    data: bytes = b""
    fd: bool = False
    brs: bool = False           # only meaningful when fd is True
    esi: bool = False
    extended: bool = False
    timestamp: float | None = field(default=None, compare=False)

    def __post_init__(self):
        limit = CAN_EFF_MASK if self.extended else CAN_SFF_MASK
        if not 0 <= self.can_id <= limit:
            raise FrameError(
                f"identifier {self.can_id:#x} does not fit in "
                f"{'29' if self.extended else '11'} bits")
        if self.fd:
            if len(self.data) not in FD_LENGTHS:
                raise FrameError(
                    f"{len(self.data)} bytes is not a length the frame format allows. "
                    f"Allowed: {', '.join(map(str, FD_LENGTHS))}")
        else:
            if len(self.data) > 8:
                raise FrameError(
                    f"{len(self.data)} bytes needs a flexible-data frame; "
                    f"a classic frame carries at most 8")
            if self.brs:
                raise FrameError("a classic frame has no rate switch to set")

    @property
    def raw_id(self):
        return self.can_id | (CAN_EFF_FLAG if self.extended else 0)

    def encode(self):
        if self.fd:
            flags = CANFD_FDF | (CANFD_BRS if self.brs else 0) | (CANFD_ESI if self.esi else 0)
            return _FD.pack(self.raw_id, len(self.data), flags, 0, 0, self.data)
        return _CLASSIC.pack(self.raw_id, len(self.data), 0, 0, 0, self.data)


def decode(buf, timestamp=None):
    """A frame from the bytes a socket handed over. The length decides which
    structure it is, which is exactly how the kernel tells them apart."""
    if len(buf) == CANFD_MTU:
        raw, length, flags, _, _, payload = _FD.unpack(buf)
        fd = True
    elif len(buf) == CAN_MTU:
        raw, length, flags, _, _, payload = _CLASSIC.unpack(buf)
        fd = False
    else:
        raise FrameError(f"{len(buf)} bytes is neither a classic frame ({CAN_MTU}) "
                         f"nor a flexible-data frame ({CANFD_MTU})")

    extended = bool(raw & CAN_EFF_FLAG)
    can_id = raw & (CAN_EFF_MASK if extended else CAN_SFF_MASK)
    return Frame(can_id=can_id,
                 data=payload[:length],
                 fd=fd,
                 brs=bool(flags & CANFD_BRS) if fd else False,
                 esi=bool(flags & CANFD_ESI) if fd else False,
                 extended=extended,
                 timestamp=timestamp)


def format_id(frame):
    return f"{frame.can_id:08X}" if frame.extended else f"{frame.can_id:03X}"


def format_frame(frame):
    """The shape the standard tools print, so a line from here and a line from
    candump can be compared without a translation step in between."""
    body = frame.data.hex().upper()
    if frame.fd:
        flags = (CANFD_BRS if frame.brs else 0) | (CANFD_ESI if frame.esi else 0)
        return f"{format_id(frame)}##{flags:X}{body}"
    return f"{format_id(frame)}#{body}"
