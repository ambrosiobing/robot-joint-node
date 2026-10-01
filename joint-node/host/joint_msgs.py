# Generated from proto/messages.yaml. Do not edit.
# description 356c7b841e583485
# Regenerate with: python tools/gen_msgs.py proto/messages.yaml

"""The host twin of src/bus/joint_msgs.c, from the same description."""
import struct

NODE_ID_BITS = 7
NODE_ID_MAX = 127


def msg_id(fc, node):
    return (fc << NODE_ID_BITS) | (node & NODE_ID_MAX)


def msg_fc(mid):
    return mid >> NODE_ID_BITS


def msg_node(mid):
    return mid & NODE_ID_MAX


FC_EMERGENCY = 0x01  # a node has entered a safe state
FC_COMMAND = 0x02  # master to node, every period
FC_STATE = 0x03  # node to master, every period
FC_DIAGNOSTIC = 0x04  # low rate, anything not needed every period


STATE_LEN = 24
STATE_FC = FC_STATE
STATE_FMT = '<iiiIHHBBBB'
STATE_FIELDS = ['position_counts', 'velocity_mrad_s', 'effort_mnm', 'stamp_us', 'valid_flags', 'fault_flags', 'mode', 'effort_source', 'model_version', 'seq']


def pack_state(**kw):
    """Pack a state frame. Every field is required: a default here would
    be a value invented by the encoder rather than chosen by the caller."""
    missing = [n for n in STATE_FIELDS if n not in kw]
    if missing:
        raise ValueError("missing fields: " + ", ".join(missing))
    return struct.pack(STATE_FMT, *[kw[n] for n in STATE_FIELDS])


def unpack_state(buf):
    if len(buf) != STATE_LEN:
        raise ValueError(f"state frame is {len(buf)} bytes, expected {STATE_LEN}")
    vals = struct.unpack(STATE_FMT, buf)
    return dict(zip(STATE_FIELDS, vals))


COMMAND_LEN = 16
COMMAND_FC = FC_COMMAND
COMMAND_FMT = '<iIBBBBI'
COMMAND_FIELDS = ['target', 'valid_until_us', 'mode_req', 'expiry_policy', 'seq', 'reserved_11', 'reserved_12']


def pack_command(**kw):
    """Pack a command frame. Every field is required: a default here would
    be a value invented by the encoder rather than chosen by the caller."""
    missing = [n for n in COMMAND_FIELDS if n not in kw]
    if missing:
        raise ValueError("missing fields: " + ", ".join(missing))
    return struct.pack(COMMAND_FMT, *[kw[n] for n in COMMAND_FIELDS])


def unpack_command(buf):
    if len(buf) != COMMAND_LEN:
        raise ValueError(f"command frame is {len(buf)} bytes, expected {COMMAND_LEN}")
    vals = struct.unpack(COMMAND_FMT, buf)
    return dict(zip(COMMAND_FIELDS, vals))
