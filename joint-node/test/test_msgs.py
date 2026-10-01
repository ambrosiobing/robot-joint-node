#!/usr/bin/env python3
"""Round trip the host codec against the generated vectors.

    python test/test_msgs.py

The same vectors drive test/test_msgs.c, so the node's packer and the host's
decoder are checked against one source rather than against each other. If this
passes and the C test fails, the two implementations disagree and the vector
file says exactly which field and which value.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "host"))

import joint_msgs as jm                                      # noqa: E402

VECTORS = ROOT / "test" / "vectors.json"


def check(cond, what, failures):
    if not cond:
        failures.append(what)
    return cond


def main():
    vectors = json.loads(VECTORS.read_text(encoding="utf-8"))
    failures = []
    checked = 0

    for mname, m in vectors["messages"].items():
        pack = getattr(jm, f"pack_{mname}")
        unpack = getattr(jm, f"unpack_{mname}")
        declared = m["length"]

        for case in m["cases"]:
            label = f"{mname} / {case['label']}"
            buf = pack(**case["fields"])

            check(len(buf) == declared,
                  f"{label}: packed {len(buf)} bytes, description says {declared}",
                  failures)

            got = unpack(buf)
            for name, want in case["fields"].items():
                check(got[name] == want,
                      f"{label}: {name} went in as {want} and came out as {got.get(name)}",
                      failures)
                checked += 1

    # The length the format allows is a property of the layout, not of a frame,
    # so it is checked once per message rather than once per case.
    for mname, m in vectors["messages"].items():
        allowed = (0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64)
        check(m["length"] in allowed,
              f"{mname}: length {m['length']} is not a length the frame format allows",
              failures)

    # A frame of the wrong length must be refused rather than decoded.
    for mname, m in vectors["messages"].items():
        unpack = getattr(jm, f"unpack_{mname}")
        for wrong in (m["length"] - 1, m["length"] + 1):
            if wrong < 0:
                continue
            try:
                unpack(bytes(wrong))
                failures.append(f"{mname}: decoded a {wrong} byte frame, which is not its length")
            except ValueError:
                pass

    # A missing field must be refused rather than defaulted, because a default
    # here is a value invented by the encoder.
    for mname in vectors["messages"]:
        pack = getattr(jm, f"pack_{mname}")
        try:
            pack()
            failures.append(f"{mname}: packed with no fields at all")
        except ValueError:
            pass

    if failures:
        print(f"FAIL  {len(failures)} problems:")
        for f in failures[:20]:
            print("  " + f)
        if len(failures) > 20:
            print(f"  ... and {len(failures) - 20} more")
        return 1

    cases = sum(len(m["cases"]) for m in vectors["messages"].values())
    print(f"ok  {cases} frames, {checked} field comparisons, "
          f"description {vectors['description']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
