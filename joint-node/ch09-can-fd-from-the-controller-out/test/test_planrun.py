#!/usr/bin/env python3
"""The executor reference, checked against its vectors and against its point.

    python test/test_planrun.py

The comparison against the vector file is the cheap half. The point of the file is
the five scenarios, and what makes them worth having is that each one fires a
different failure path, which is asserted below rather than hoped for. A test
suite in which every case fails the same way has tested one thing five times.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_initplan as ip                                       # noqa: E402
import gen_msgram as mr                                         # noqa: E402
import gen_planrun as g                                         # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_planrun.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    configs = {name: cfg for name, cfg, _why in g.CASES}
    verdicts, outcomes, completed = set(), set(), 0

    for c in doc["cases"]:
        steps, _timing = ip.plan(configs[c["scenario"]], mr.LAYOUT)
        if steps is None:
            fail(f"{c['scenario']}: its configuration does not plan")
            continue
        model = g.Model(c["scenario"])
        result = g.run(steps, model)

        if result != c["result"]:
            fail(f"{c['scenario']}: the result differs from the file")
            for k in sorted(set(result) | set(c["result"])):
                if result.get(k) != c["result"].get(k):
                    fail(f"  {k}: {result.get(k)} against the file's "
                         f"{c['result'].get(k)}")
            continue
        if model.log != c["log"]:
            fail(f"{c['scenario']}: the access log differs from the file")
            for i, (a, b) in enumerate(zip(model.log, c["log"])):
                if a != b:
                    fail(f"  access {i + 1}: {a} against the file's {b}")
                    break
            continue

        verdicts.add(result["verdict"])
        outcomes.add((result["verdict"], result["step"]))
        if result["ok"]:
            completed += 1

        # A refusal must stop where it failed. Continuing past a refused
        # configuration change would write the rest of the configuration into a
        # peripheral that is not listening, and every one of those writes would
        # appear to succeed.
        if not result["ok"] and result["step"] >= len(steps):
            fail(f"{c['scenario']}: refused at step {result['step']} of "
                 f"{len(steps)}, which is not a refusal that stopped anything")

        # A refusal must name the step it failed at.
        if not result["ok"] and not result["name"]:
            fail(f"{c['scenario']}: refused without naming the step")

        # A bounded wait must stop AT the bound, not near it.
        if result["verdict"].startswith("wait-"):
            if result["spins"] != doc["spin_limit"]:
                fail(f"{c['scenario']}: a timed out wait span "
                     f"{result['spins']} reads and the limit is "
                     f"{doc['spin_limit']}")
        if result["verdict"] == "modify-refused":
            if result["spins"] != doc["retries"]:
                fail(f"{c['scenario']}: a refused modify took "
                     f"{result['spins']} attempts and the limit is "
                     f"{doc['retries']}")

    # Each scenario has to fail in its own place, or the suite has tested one
    # thing six times. The test is on the pair of verdict and step, not on the
    # verdict alone: two scenarios legitimately share "modify-refused", because a
    # bit that never sets and a bit that never clears are the same kind of
    # failure at opposite ends of the sequence, and collapsing them would hide
    # the fact that one is caught at step 6 and the other at step 22.
    if completed != 1:
        fail(f"{completed} scenarios completed, and exactly one should")
    if len(outcomes) != len(doc["cases"]):
        fail(f"{len(doc['cases'])} scenarios produced {len(outcomes)} distinct "
             f"outcomes, so two of them are the same test: {sorted(outcomes)}")

    # And every verdict the executor can reach for a step has to be reached by
    # something, or a branch of it is written and never run.
    for needed in ("ok", "expect-mismatch", "modify-refused",
                   "wait-set-timeout", "write-not-read-back"):
        if needed not in verdicts:
            fail(f"no scenario produces {needed}, so that path of the executor "
                 f"is written and never run")

    # The endianness check must refuse before any write happens at all, because
    # that is its whole purpose: a wrong peripheral base accepts every write and
    # reads back zeroes, and writing into it is what makes the symptom appear
    # somewhere else.
    endn = next(c for c in doc["cases"] if c["scenario"] == "endn wrong")
    if any(a["kind"] == "write" for a in endn["log"]):
        fail("the endianness check let a write happen before refusing")
    if endn["result"]["step"] != 2:
        fail(f"the endianness check refused at step {endn['result']['step']} "
             f"rather than 2")

    # And the cooperative run must touch every step, or the failing ones are being
    # compared against a plan that was never fully walked.
    coop = next(c for c in doc["cases"] if c["scenario"] == "cooperative")
    if coop["result"]["steps_done"] != coop["plan_steps"]:
        fail(f"the cooperative run completed {coop['result']['steps_done']} of "
             f"{coop['plan_steps']} steps")

    # The declined write must be caught by the read-back and nothing else, which
    # is the one failure in this file that a sequence without read-back would
    # report as a success.
    decl = next(c for c in doc["cases"] if c["scenario"] == "test declines")
    if decl["result"]["verdict"] != "write-not-read-back":
        fail(f"a declined write reported {decl['result']['verdict']}")
    elif decl["result"]["read_back"] == decl["result"]["wrote"]:
        fail("a declined write read back what was written, so the model is not "
             "declining anything")

    if failures:
        print(f"test_planrun: {len(failures)} failure(s)")
        return 1
    print(f"ok  {len(doc['cases'])} scenarios, {len(outcomes)} distinct "
          f"outcomes over {len(verdicts)} verdicts, every access log matched")
    return 0


if __name__ == "__main__":
    sys.exit(main())
