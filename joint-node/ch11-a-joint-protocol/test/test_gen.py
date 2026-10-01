#!/usr/bin/env python3
"""Break the description on purpose, one fault at a time, and confirm the
generator refuses it.

    python test/test_gen.py

A validator that has never rejected anything is a validator nobody has tested.
Each case below is a layout mistake that is silent at run time: the frame still
packs, the fields still have values, and the numbers are wrong on the wire.
"""
import copy
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from gen_msgs import validate, SpecError                     # noqa: E402

GOOD = yaml.safe_load((ROOT / "proto" / "messages.yaml").read_bytes())


def mutate(fn):
    spec = copy.deepcopy(GOOD)
    fn(spec)
    return spec


def overlap(s):
    s["messages"]["state"]["fields"][1]["offset"] = 2           # into position_counts


def hole(s):
    for f in s["messages"]["state"]["fields"][1:]:
        f["offset"] += 2
    s["messages"]["state"]["length"] = 26


def wrong_total(s):
    s["messages"]["state"]["length"] = 20


def length_not_allowed(s):
    s["messages"]["state"]["fields"].append(
        dict(name="extra", type="u8", offset=24, unit="count", note="one byte too many"))
    s["messages"]["state"]["length"] = 25                      # 25 is not an allowed length


def duplicate_code(s):
    s["function_codes"][1]["code"] = s["function_codes"][0]["code"]


def duplicate_field(s):
    s["messages"]["state"]["fields"][1]["name"] = "position_counts"


def code_too_wide(s):
    s["function_codes"][0]["code"] = 0x20                      # 11 - 7 = 4 bits available


def unknown_type(s):
    s["messages"]["state"]["fields"][0]["type"] = "f32"


def unknown_function_code(s):
    s["messages"]["state"]["function_code"] = "TELEMETRY"


CASES = [
    ("a field that overlaps the one before it", overlap, "overlaps"),
    ("a hole between two fields", hole, "unaccounted for"),
    ("fields that do not add up to the declared length", wrong_total, "but length says"),
    ("a length the frame format does not allow", length_not_allowed, "not a length"),
    ("two function codes sharing a value", duplicate_code, "share value"),
    ("two fields sharing a name", duplicate_field, "duplicate field name"),
    ("a function code too wide for the identifier", code_too_wide, "does not fit"),
    ("a type the generator has never heard of", unknown_type, "unknown type"),
    ("a message naming a function code that does not exist", unknown_function_code, "unknown function code"),
]


def main():
    failures = []

    try:
        validate(copy.deepcopy(GOOD))
    except SpecError as e:
        failures.append(f"the real description was rejected:\n{e}")

    for label, fn, expect in CASES:
        try:
            validate(mutate(fn))
        except SpecError as e:
            if expect not in str(e):
                failures.append(f"{label}: rejected, but the reason did not mention "
                                f"{expect!r}:\n{e}")
        else:
            failures.append(f"{label}: ACCEPTED, and it should not have been")

    if failures:
        print(f"FAIL  {len(failures)} problems:")
        for f in failures:
            print("  " + f)
        return 1

    print(f"ok  the real description validates, and {len(CASES)} broken ones are refused")
    return 0


if __name__ == "__main__":
    sys.exit(main())
