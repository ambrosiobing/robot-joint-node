#!/usr/bin/env python3
"""Walking the plan against a register model, and the log the C must reproduce.

    python tools/gen_planrun.py            regenerate the vectors
    python tools/gen_planrun.py --check    fail if the committed ones differ
    python tools/gen_planrun.py --table    print the table for doc/node-order.md

Chapter 9 stage two. initplan says what to do; planrun does it. Its two rules are
that every write is read back and every wait is bounded, and both exist because
the failure they prevent is silent on real hardware.

A rule like that is only worth having if its own failure paths have been
exercised, and on a board they cannot be: a register that declines a write does
not announce it, which is the entire problem. So the hardware is replaced by a
model small enough to specify exactly, the plan is walked against it, and the
sequence of bus accesses that comes out is the oracle.

THE MODEL, and the C in test/test_planrun.c implements the same one. Any
disagreement shows up as a differing transaction log, which names the access
rather than the symptom.

  - 256 words of storage, indexed by offset / 4, all zero except ENDN, which
    reads 0x87654321, and CCCR, which starts with INIT set because the part comes
    out of reset stopped
  - a read returns the stored word
  - a write stores the word, except that CCCR's CSR and CSA bits are forced to
    zero, because they are a request and its acknowledgement rather than settings
  - clearing the message RAM is recorded as one access and changes no register

and then one deviation per scenario, each chosen so that exactly one of the
executor's failure paths is the one that fires.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_initplan as ip                                       # noqa: E402
import gen_msgram as mr                                         # noqa: E402

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_planrun.json"
VECTORS_H = HERE / "test" / "vectors_planrun.h"

REG, CCCR = ip.REG, ip.CCCR
VOLATILE = CCCR["CSA"] | CCCR["CSR"]

RETRIES = 10
SPIN_LIMIT = 50

# The scenarios, as a name the C switches on too.
SCENARIOS = ["cooperative", "endn wrong", "cce stuck", "cce reverts",
             "init stuck", "test declines"]
SCENARIO = {name: i for i, name in enumerate(SCENARIOS)}

VERDICTS = ["ok", "expect-mismatch", "modify-refused", "wait-set-timeout",
            "wait-clear-timeout", "write-not-read-back", "io-unusable"]


class Model:
    """The register model. Deliberately dull, and specified in the docstring
    above so the C can implement the same one rather than something similar."""

    def __init__(self, scenario):
        self.scenario = scenario
        self.mem = {}
        self.mem[REG["ENDN"]] = 0x87654321
        self.mem[REG["CCCR"]] = CCCR["INIT"]
        if scenario == "endn wrong":
            self.mem[REG["ENDN"]] = 0x12345678
        self.log = []

    def read(self, reg):
        v = self.mem.get(reg, 0)
        self.log.append({"kind": "read", "reg": reg, "value": v})
        # A bit that does not latch: it reads back correctly once, immediately
        # after the write, and is gone by the next read. That is what turns a
        # modify that succeeds into a wait that never finishes.
        if self.scenario == "cce reverts" and reg == REG["CCCR"]:
            self.mem[reg] = v & ~CCCR["CCE"]
        return v

    def write(self, reg, value):
        self.log.append({"kind": "write", "reg": reg, "value": value})
        # The TEST register is writable only while CCCR.TEST is set. A model that
        # ignores writes to it is exactly what a sequence writing TEST too early
        # meets, and the point is that the hardware says nothing about it.
        if reg == REG["TEST"] and self.scenario == "test declines":
            return
        if reg == REG["CCCR"]:
            value &= ~VOLATILE
            if self.scenario == "cce stuck":
                value &= ~CCCR["CCE"]           # the bit never sets
            if self.scenario == "init stuck":
                value |= CCCR["INIT"]           # the bit never clears
        self.mem[reg] = value

    def clear_ram(self, start, end):
        self.log.append({"kind": "clear", "reg": start, "value": end})


def run(steps, model, retries=RETRIES, spin_limit=SPIN_LIMIT):
    """The reference executor. The C in src/bus/planrun.c is the one that ships;
    this exists so the two can be compared against one log rather than against
    each other."""
    result = {"step": 0, "steps_done": 0, "name": "", "op": 0, "reg": 0,
              "wrote": 0, "read_back": 0, "spins": 0, "ok": False,
              "verdict": "io-unusable"}

    def finish(n, st, wrote, read_back, spins, ok, verdict):
        result.update(step=n, steps_done=n - 1, name=st["name"], op=st["op"],
                      reg=st["reg"], wrote=wrote, read_back=read_back,
                      spins=spins, ok=ok, verdict=verdict)

    for i, st in enumerate(steps):
        n = i + 1
        op = st["op_name"]
        ignore = VOLATILE if st["reg"] == REG["CCCR"] else 0

        if op == "read":
            finish(n, st, 0, model.read(st["reg"]), 0, True, "ok")

        elif op == "expect":
            got = model.read(st["reg"])
            ok = (got & st["mask"]) == (st["value"] & st["mask"])
            finish(n, st, st["value"], got, 0, ok,
                   "ok" if ok else "expect-mismatch")
            if not ok:
                return result

        elif op == "write":
            model.write(st["reg"], st["value"])
            got = model.read(st["reg"])
            ok = (got & ~ignore & 0xFFFFFFFF) == (st["value"] & ~ignore
                                                 & 0xFFFFFFFF)
            finish(n, st, st["value"], got, 1, ok,
                   "ok" if ok else "write-not-read-back")
            if not ok:
                return result

        elif op == "modify":
            attempts, want, got, ok = 0, 0, 0, False
            while attempts < retries:
                cur = model.read(st["reg"])
                want = (cur & ~st["mask"]) | (st["value"] & st["mask"])
                want &= 0xFFFFFFFF
                model.write(st["reg"], want)
                got = model.read(st["reg"])
                attempts += 1
                if (got & ~ignore & 0xFFFFFFFF) == (want & ~ignore
                                                    & 0xFFFFFFFF):
                    ok = True
                    break
            finish(n, st, want, got, attempts, ok,
                   "ok" if ok else "modify-refused")
            if not ok:
                return result

        elif op in ("wait set", "wait clear"):
            want_set = op == "wait set"
            spins, got, ok = 0, 0, False
            while spins < spin_limit:
                got = model.read(st["reg"])
                spins += 1
                if want_set:
                    ok = (got & st["mask"]) == st["mask"]
                else:
                    ok = (got & st["mask"]) == 0
                if ok:
                    break
            finish(n, st, 0, got, spins, ok,
                   "ok" if ok else ("wait-set-timeout" if want_set
                                    else "wait-clear-timeout"))
            if not ok:
                return result

        elif op == "clear ram":
            model.clear_ram(st["reg"], st["mask"])
            finish(n, st, 0, 0, 0, True, "ok")

        else:
            finish(n, st, 0, 0, 0, False, "failed")
            return result

    result["ok"] = True
    result["steps_done"] = len(steps)
    result["verdict"] = "ok"
    return result


# The configurations walked. Five scenarios use the first image's own, because a
# failure path tested against a configuration nobody ships proves less. The sixth
# needs loopback, for the reason given beside it.
FIRST_IMAGE = {"kernel_hz": 8_000_000, "bitrate": 500_000,
               "want_sp_permille": 800, "mode": "normal", "one_shot": False}
LOOPBACK = dict(FIRST_IMAGE, mode="loopback internal")

CASES = [
    ("cooperative", FIRST_IMAGE,
     "every register behaves, and the whole plan completes"),
    ("endn wrong", FIRST_IMAGE,
     "a wrong peripheral base: the endianness check catches it at step 2, "
     "before a single write"),
    ("cce stuck", FIRST_IMAGE,
     "CCE never sets, which is what happens when INIT is not really set: ten "
     "attempts, then a named refusal"),
    ("cce reverts", FIRST_IMAGE,
     "CCE reads back correctly once and is gone by the next read, so the modify "
     "succeeds and the wait does not"),
    ("init stuck", FIRST_IMAGE,
     "INIT never clears, so the controller cannot be started and the last "
     "modify is the one that says so"),
    # This one is the silent failure the documentation names, caught. TEST is
    # writable only while CCCR.TEST is set, and a sequence that writes it too
    # early gets no complaint from the hardware at all. It needs loopback,
    # because in normal mode TEST is written with zero and its reset value is
    # zero, so a declined write is indistinguishable from an obeyed one.
    ("test declines", LOOPBACK,
     "TEST ignores the write, as it does while CCCR.TEST is clear: only the "
     "read-back notices, and only in a mode where TEST is non-zero"),
]


def build():
    out = []
    for scenario, cfg, why in CASES:
        steps, _timing = ip.plan(cfg, mr.LAYOUT)
        if steps is None:
            raise SystemExit(f"{scenario}: its configuration does not plan")
        model = Model(scenario)
        result = run(steps, model)
        out.append({
            "scenario": scenario,
            "scenario_code": SCENARIO[scenario],
            "mode": cfg["mode"],
            "mode_code": ip.MODE[cfg["mode"]],
            "one_shot": cfg["one_shot"],
            "why": why,
            "plan_steps": len(steps),
            "result": result,
            "accesses": len(model.log),
            "log": model.log,
        })
    return {
        "note": "Generated by tools/gen_planrun.py. Do not edit.",
        "retries": RETRIES,
        "spin_limit": SPIN_LIMIT,
        "scenarios": SCENARIOS,
        "verdicts": VERDICTS,
        "cases": out,
    }


def as_header(doc):
    lines = [
        "/* Generated by tools/gen_planrun.py. Do not edit. */",
        "#ifndef JOINT_PLANRUN_VECTORS_H",
        "#define JOINT_PLANRUN_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        f"#define PR_RETRIES    {doc['retries']}u",
        f"#define PR_SPIN_LIMIT {doc['spin_limit']}u",
        "",
        "typedef struct {",
        "    uint32_t kind;    /* 0 read, 1 write, 2 clear */",
        "    uint32_t reg;",
        "    uint32_t value;",
        "} pr_access_t;",
        "",
        "typedef struct {",
        "    const char         *scenario;",
        "    uint32_t            scenario_code;",
        "    uint32_t            mode;",
        "    bool                one_shot;",
        "    uint32_t            plan_steps;",
        "    bool                ok;",
        "    uint32_t            step;",
        "    uint32_t            steps_done;",
        "    uint32_t            spins;",
        "    uint32_t            wrote;",
        "    uint32_t            read_back;",
        "    const char         *verdict;",
        "    uint32_t            access_count;",
        "    const pr_access_t  *log;",
        "} pr_vector_t;",
        "",
    ]
    kind = {"read": 0, "write": 1, "clear": 2}
    for i, c in enumerate(doc["cases"]):
        lines.append(f"static const pr_access_t PR_LOG_{i}[] = {{")
        for a in c["log"]:
            lines.append("    {{ {k}u, 0x{reg:03X}u, 0x{val:08X}u }},".format(
                k=kind[a["kind"]], reg=a["reg"], val=a["value"]))
        lines.append("};")
        lines.append("")
    lines.append("static const pr_vector_t PR_VECTORS[] = {")
    for i, c in enumerate(doc["cases"]):
        r = c["result"]
        lines.append(
            '    {{ "{s}", {sc}u, {mode}u, {os}, {ps}u, {ok}, {step}u, '
            '{done}u, {spins}u, 0x{wrote:08X}u, 0x{rb:08X}u, "{v}", {n}u, '
            "PR_LOG_{i} }},".format(
                s=c["scenario"], sc=c["scenario_code"], mode=c["mode_code"],
                os="true" if c["one_shot"] else "false", ps=c["plan_steps"],
                ok="true" if r["ok"] else "false", step=r["step"],
                done=r["steps_done"], spins=r["spins"], wrote=r["wrote"],
                rb=r["read_back"], v=r["verdict"], n=c["accesses"], i=i))
    lines += [
        "};",
        "",
        "#define PR_VECTOR_COUNT "
        "((int) (sizeof PR_VECTORS / sizeof PR_VECTORS[0]))",
        "",
        "#endif /* JOINT_PLANRUN_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    rows = ["| Scenario | Result | Step | Spins | Verdict | Bus accesses | "
            "What it models |",
            "|---|---|---|---|---|---|---|"]
    for c in doc["cases"]:
        r = c["result"]
        rows.append(
            f"| {c['scenario']} | {'completed' if r['ok'] else 'refused'} | "
            f"{r['step']} of {c['plan_steps']} | {r['spins']} | "
            f"`{r['verdict']}` | {c['accesses']} | {c['why']} |")
    return "\n".join(rows)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--table", action="store_true")
    a = ap.parse_args(argv)

    doc = build()
    js = json.dumps(doc, indent=2) + "\n"
    hd = as_header(doc)

    if a.table:
        print(table(doc))
        return 0

    if a.check:
        bad = 0
        for path, want in ((VECTORS_JSON, js), (VECTORS_H, hd)):
            have = path.read_text(encoding="utf-8") if path.exists() else ""
            if have.replace("\r\n", "\n") != want:
                print(f"{path.name} differs from the generator")
                bad += 1
        if bad:
            print("a hand edit does not survive here; regenerate instead")
            return 1
        failed = sum(1 for c in doc["cases"] if not c["result"]["ok"])
        print(f"planrun vectors match the generator ({len(doc['cases'])} "
              f"scenarios, {failed} of them refusals)")
        return 0

    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    failed = sum(1 for c in doc["cases"] if not c["result"]["ok"])
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} scenarios, {failed} of them refusals")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
