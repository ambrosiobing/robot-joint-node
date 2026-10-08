#!/usr/bin/env python3
"""The configuration order, checked against its vectors and against its rules.

    python test/test_initplan.py

The comparison against the vector file is the cheap half. The valuable half is
the invariants below, because each one of them is a failure that has already been
made somewhere or that cannot be seen on a board:

  - a register that points into the message RAM, written before the RAM is
    cleared, reads an uninitialised element back as a parity error
  - TEST written before CCCR is a write to a locked register, so loopback stays
    off while every other bit reads back correctly
  - CCE set while INIT is clear is refused by the hardware, not obeyed
  - INIT cleared before CCE is cleared leaves a running controller mid
    configuration

None of those four announces itself. So they are asserted here, on a host, where
a failure is a line of output rather than a bus that is quiet for no reason.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_initplan as g                                        # noqa: E402
import gen_msgram as mr                                         # noqa: E402

VECTORS = Path(__file__).resolve().parent / "vectors_initplan.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def index_of(steps, op, reg, value=None):
    """The first step matching, or None. Used by the ordering rules."""
    for i, st in enumerate(steps):
        if st["op_name"] != op or st["reg"] != reg:
            continue
        if value is not None and st["value"] != value:
            continue
        return i
    return None


def check_order(label, steps):
    R, C = g.REG, g.CCCR

    # INIT is set, and confirmed, before CCE is even attempted.
    set_init = index_of(steps, "modify", R["CCCR"], C["INIT"])
    wait_init = index_of(steps, "wait set", R["CCCR"])
    set_cce = index_of(steps, "modify", R["CCCR"], C["CCE"])
    if set_init is None:
        fail(f"{label}: nothing sets CCCR.INIT, so nothing may be configured")
        return
    if set_cce is None:
        fail(f"{label}: nothing sets CCCR.CCE")
        return
    if not set_init < set_cce:
        fail(f"{label}: CCE at step {set_cce + 1} comes before INIT at "
             f"{set_init + 1}, and the hardware refuses that rather than "
             f"obeying it")
    if wait_init is None or not set_init < wait_init < set_cce:
        fail(f"{label}: INIT is not waited on between setting it and opening "
             f"configuration")

    # The message RAM is cleared before anything points into it.
    clear = next((i for i, st in enumerate(steps)
                  if st["op_name"] == "clear ram"), None)
    if clear is None:
        fail(f"{label}: the message RAM is never cleared, and an uninitialised "
             f"element reads back as a parity error")
    else:
        pointing = [R[n] for n in ("SIDFC", "XIDFC", "RXF0C", "RXF1C", "RXBC",
                                   "TXEFC", "TXBC")]
        for i, st in enumerate(steps):
            if st["reg"] in pointing and st["op_name"] == "write" and i < clear:
                fail(f"{label}: step {i + 1} points into the message RAM "
                     f"before step {clear + 1} clears it")

    # CCCR before TEST, because CCCR.TEST is what unlocks TEST.
    mode = index_of(steps, "modify", R["CCCR"], None)
    test_write = index_of(steps, "write", R["TEST"])
    mode_steps = [i for i, st in enumerate(steps)
                  if st["op_name"] == "modify" and st["reg"] == R["CCCR"]
                  and (st["mask"] & C["TEST"])]
    if test_write is None:
        fail(f"{label}: the TEST register is never written, so its state is "
             f"whatever the last image left")
    elif not mode_steps:
        fail(f"{label}: nothing touches CCCR.TEST, so TEST is never writable")
    elif not min(mode_steps) < test_write:
        fail(f"{label}: TEST at step {test_write + 1} is written before CCCR "
             f"unlocks it at {min(mode_steps) + 1}, which fails silently")

    # The timing is inside the configuration window.
    nbtp = index_of(steps, "write", R["NBTP"])
    clear_cce = [i for i, st in enumerate(steps)
                 if st["op_name"] == "modify" and st["reg"] == R["CCCR"]
                 and st["mask"] == C["CCE"] and st["value"] == 0]
    if nbtp is None:
        fail(f"{label}: NBTP is never written")
    elif not clear_cce:
        fail(f"{label}: CCE is never cleared, so configuration is left open")
    elif not set_cce < nbtp < clear_cce[0]:
        fail(f"{label}: NBTP at step {nbtp + 1} is outside the CCE window "
             f"{set_cce + 1} to {clear_cce[0] + 1}")

    # CCE is closed before INIT is released, and INIT is released last.
    clear_init = [i for i, st in enumerate(steps)
                  if st["op_name"] == "modify" and st["reg"] == R["CCCR"]
                  and st["mask"] == C["INIT"] and st["value"] == 0]
    if not clear_init:
        fail(f"{label}: INIT is never cleared, so the controller never starts")
    elif clear_cce and not clear_cce[0] < clear_init[0]:
        fail(f"{label}: INIT is released at step {clear_init[0] + 1} before "
             f"CCE is closed at {clear_cce[0] + 1}, leaving a running "
             f"controller mid configuration")
    else:
        after = [st for st in steps[clear_init[0] + 1:]
                 if st["op_name"] not in ("wait clear", "wait set", "read")]
        if after:
            fail(f"{label}: {len(after)} step(s) configure the controller after "
                 f"it was started, and the first is {after[0]['name']}")

    # The block is proven to answer before anything is written to it.
    first_write = next((i for i, st in enumerate(steps)
                        if st["op_name"] in ("write", "modify")), None)
    endn = index_of(steps, "expect", R["ENDN"])
    if endn is None:
        fail(f"{label}: nothing checks ENDN, and a wrong base address reads "
             f"back as zeroes that look merely unconfigured")
    elif first_write is not None and not endn < first_write:
        fail(f"{label}: the first write at step {first_write + 1} precedes the "
             f"ENDN check at {endn + 1}")


def main():
    doc = json.loads(VECTORS.read_text(encoding="utf-8"))
    cases = doc["cases"]
    solved = refused = 0

    for c in cases:
        cfg = {k: c["config"][k] for k in ("kernel_hz", "bitrate",
                                          "want_sp_permille", "mode",
                                          "one_shot")}
        steps, timing = g.plan(cfg, mr.LAYOUT)

        if (steps is not None) != c["ok"]:
            fail(f"{c['label']}: planned is {steps is not None}, the file says "
                 f"{c['ok']}")
            continue
        if not c["ok"]:
            refused += 1
            continue
        solved += 1

        if steps != c["steps"]:
            fail(f"{c['label']}: the plan differs from the file")
            for i, (a, b) in enumerate(zip(steps, c["steps"])):
                if a != b:
                    fail(f"  step {i + 1}: {a} against the file's {b}")
            continue
        if timing != c["timing"]:
            fail(f"{c['label']}: timing {timing} against the file's "
                 f"{c['timing']}")
        if len(steps) > doc["plan_max"]:
            fail(f"{c['label']}: {len(steps)} steps exceeds the declared "
                 f"maximum of {doc['plan_max']}, which the C sizes an array by")

        check_order(c["label"], steps)

    # The mode bits are the one place where two configurations must differ, so a
    # table that produced the same plan for every mode would pass everything
    # above and be useless.
    plans = {}
    for c in cases:
        if c["ok"]:
            plans[c["config"]["mode"], c["config"]["one_shot"]] = c["steps"]
    if len(plans) < 4:
        fail(f"only {len(plans)} distinct configurations solved, so the mode "
             f"handling is barely exercised")
    seen = {}
    for key, steps in plans.items():
        sig = json.dumps(steps, sort_keys=True)
        if sig in seen:
            fail(f"{key} and {seen[sig]} produced an identical plan, so one of "
                 f"them is not doing what its name says")
        seen[sig] = key

    # Internal loopback must keep the frame off the wire, and that is MON.
    internal = next(c for c in cases
                    if c["config"]["mode"] == "loopback internal")
    mode_step = next(st for st in internal["steps"]
                     if st["op_name"] == "modify"
                     and st["reg"] == g.REG["CCCR"]
                     and (st["mask"] & g.CCCR["TEST"]))
    for bit in ("TEST", "MON"):
        if not mode_step["value"] & g.CCCR[bit]:
            fail(f"internal loopback does not set CCCR.{bit}, and without MON "
                 f"the frame goes out on the real bus")
    external = next(c for c in cases
                    if c["config"]["mode"] == "loopback external")
    ext_step = next(st for st in external["steps"]
                    if st["op_name"] == "modify"
                    and st["reg"] == g.REG["CCCR"]
                    and (st["mask"] & g.CCCR["TEST"]))
    if ext_step["value"] & g.CCCR["MON"]:
        fail("external loopback sets CCCR.MON, which would keep the frame off "
             "the wire and make it internal")

    if refused == 0:
        fail("not one configuration was refused, so the refusal path is "
             "untested")

    if failures:
        print(f"test_initplan: {len(failures)} failure(s)")
        return 1
    print(f"ok  {len(cases)} configurations, {solved} planned and {refused} "
          f"refused, every ordering rule held")
    return 0


if __name__ == "__main__":
    sys.exit(main())
