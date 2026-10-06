#!/usr/bin/env python3
"""The length code reference, against its vectors and its invariants.

    python test/test_frame.py

The half that runs on the win11 aquamarine authoring laptop. test_frame.c checks
the same vector file and is built on the WSL side of win11 skyhorizon and in
continuous integration.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_frame_vectors as g                               # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_frame.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))

    for c in doc["codes"]:
        if g.length_for_code(c["code"], False) != c["classic_length"]:
            fail(f"code {c['code']} classic no longer matches the file")
        if g.length_for_code(c["code"], True) != c["fd_length"]:
            fail(f"code {c['code']} flexible-data no longer matches the file")

    for l in doc["lengths"]:
        if g.code_for_length(l["length"], False) != l["classic_code"]:
            fail(f"length {l['length']} classic no longer matches the file")
        if g.code_for_length(l["length"], True) != l["fd_code"]:
            fail(f"length {l['length']} flexible-data no longer matches")

    # Round trip: a length with a code comes back as itself, never as a
    # neighbour. This is the property the payload depends on.
    round_trips = 0
    for length in range(0, 65):
        code = g.code_for_length(length, True)
        if code is None:
            continue
        back = g.length_for_code(code, True)
        if back != length:
            fail(f"{length} bytes became code {code} and came back as {back}")
        round_trips += 1
    if round_trips != 16:
        fail(f"{round_trips} lengths round tripped and the format has 16")

    # The codes are strictly increasing, so no two lengths share one and no
    # length is unreachable.
    if list(g.FD_LENGTH_FOR_CODE) != sorted(set(g.FD_LENGTH_FOR_CODE)):
        fail("the code table is not strictly increasing")

    # Above code 8 the two formats disagree on purpose, and that is the trap.
    for code in range(g.CLASSIC_MAX + 1, 16):
        if g.length_for_code(code, False) != g.CLASSIC_MAX:
            fail(f"classic code {code} did not clamp to {g.CLASSIC_MAX}")
        if g.length_for_code(code, True) <= g.CLASSIC_MAX:
            fail(f"flexible-data code {code} is not above {g.CLASSIC_MAX}")

    if g.length_for_code(16, True) is not None:
        fail("code 16 was given a length, and the field is four bits")

    refused = 0
    for f in doc["frames"]:
        ok = g.frame_ok(f["length"], f["fd"], f["brs"])
        if ok != f["ok"]:
            fail(f"{f['label']}: {ok} and the file says {f['ok']}")
        if not f["ok"]:
            refused += 1

    if refused == 0:
        fail("no frame was refused, so the refusal path is untested")

    # The three frames chapter 9 step 4 sends have to be the legal ones.
    for label in ("classic, 8 bytes",
                  "flexible-data, 64 bytes, no rate switch",
                  "flexible-data, 64 bytes, rate switch"):
        f = next((x for x in doc["frames"] if x["label"] == label), None)
        if f is None:
            fail(f"the step 4 frame {label!r} is missing from the vectors")
        elif not f["ok"]:
            fail(f"the step 4 frame {label!r} is refused, and the chapter "
                 f"sends it")

    if failures:
        print(f"test_frame: {len(failures)} failure(s)")
        return 1
    print(f"ok  16 codes both formats, 65 lengths, {round_trips} round trips, "
          f"{len(doc['frames'])} frames with {refused} refused")
    return 0


if __name__ == "__main__":
    sys.exit(main())
