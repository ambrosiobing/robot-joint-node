#!/usr/bin/env python3
"""The configuration order, and the vectors the C must reproduce.

    python tools/gen_initplan.py            regenerate the vectors
    python tools/gen_initplan.py --check    fail if the committed ones differ
    python tools/gen_initplan.py --table    print the table for doc/node-order.md

Chapter 9 stage two. The values were settled on Wednesday 7 October 2026 and
written up in doc/node-registers.md. This is the reference for the ORDER, which
is the part that matters most, because three of the four things that were guessed
about it were guessed wrong and every one of those three fails silently: an
uninitialised message RAM reads back as a parity error, a test register written
before the control register is a write to a locked register, and a global filter
of zero accepts rather than rejects.

A sequence that fails silently cannot be debugged on the board, so it is produced
as data by a function with no hardware in it, and checked here against the C on a
host with no controller.

The node's own copy is C, in src/bus/initplan.c. The two are never compared
against each other: both are compared against test/vectors_initplan.json, so when
they disagree the file says which step and which field.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_bt_vectors as bt                                     # noqa: E402
import gen_msgram as mr                                         # noqa: E402

HERE = Path(__file__).resolve().parent.parent
VECTORS_JSON = HERE / "test" / "vectors_initplan.json"
VECTORS_H = HERE / "test" / "vectors_initplan.h"

# The Bosch M_CAN offsets, from the mainline driver's own enum. The peripheral is
# not ST's design, which is why a driver that runs on parts from several vendors
# beats any one vendor's summary.
REG = {
    "CREL": 0x000, "ENDN": 0x004, "DBTP": 0x00C, "TEST": 0x010,
    "CCCR": 0x018, "NBTP": 0x01C, "ECR": 0x040, "PSR": 0x044,
    "IR": 0x050, "GFC": 0x080, "SIDFC": 0x084, "XIDFC": 0x088,
    "RXF0C": 0x0A0, "RXF0S": 0x0A4, "RXF0A": 0x0A8, "RXBC": 0x0AC,
    "RXF1C": 0x0B0, "RXESC": 0x0BC, "TXBC": 0x0C0, "TXFQS": 0x0C4,
    "TXESC": 0x0C8, "TXBAR": 0x0D0, "TXEFC": 0x0F0,
}

CCCR = {"INIT": 1 << 0, "CCE": 1 << 1, "ASM": 1 << 2, "CSA": 1 << 3,
        "CSR": 1 << 4, "MON": 1 << 5, "DAR": 1 << 6, "TEST": 1 << 7,
        "FDOE": 1 << 8, "BRSE": 1 << 9, "NISO": 1 << 15}

TEST_LBCK = 1 << 4
ESC_64B = 0x7

# Operation codes, in the same order as the C enum, because the vector file
# carries the number and a reordering on one side only would be invisible.
OPS = ["read", "write", "modify", "wait set", "wait clear", "clear ram",
       "expect"]
OP = {name: i for i, name in enumerate(OPS)}

MODES = ["normal", "loopback internal", "loopback external", "listen only"]
MODE = {name: i for i, name in enumerate(MODES)}

PLAN_MAX = 32


def mode_bits(mode, one_shot):
    """What a mode change clears and what it sets.

    Both, because a mode change is a read, modify, write. FDOE and BRSE are in
    the clear mask because the first image is classic and a controller left in FD
    mode by a previous image would otherwise stay there.
    """
    clear = (CCCR["TEST"] | CCCR["MON"] | CCCR["ASM"] | CCCR["DAR"]
             | CCCR["FDOE"] | CCCR["BRSE"] | CCCR["NISO"])
    set_ = 0
    if mode == "loopback internal":
        set_ |= CCCR["TEST"] | CCCR["MON"]
    elif mode == "loopback external":
        set_ |= CCCR["TEST"]
    elif mode == "listen only":
        set_ |= CCCR["MON"]
    if one_shot:
        set_ |= CCCR["DAR"]
    return clear, set_


def plan(cfg, layout):
    """The ordered operations, or None when anything about it must be refused.

    Refusals happen before the first step, so a plan that exists is a plan that
    can be run.
    """
    # gen_msgram.compute returns a refusal as ok=False with the reasons, never
    # as None, and it reports the sections as a list rather than as flat keys.
    # Both of those were got wrong on the first attempt here, which is exactly
    # what reading the sibling before writing the copy is meant to prevent.
    mp = mr.compute(layout)
    if not mp["ok"]:
        return None, None
    off = {sec["name"]: sec["offset"] for sec in mp["sections"]}

    timing = bt.bt_compute(cfg["kernel_hz"], cfg["bitrate"],
                           cfg["want_sp_permille"], bt.NOMINAL)
    if timing is None:
        return None, None

    nbtp = bt.pack_nbtp(timing)
    if nbtp is None:
        return None, None

    clear, set_ = mode_bits(cfg["mode"], cfg["one_shot"])
    test = TEST_LBCK if cfg["mode"].startswith("loopback") else 0

    s = []

    def step(op, reg, mask, value, name):
        s.append({"op": OP[op], "op_name": op, "reg": reg, "mask": mask,
                  "value": value, "name": name})

    # 1 and 2. Prove the block answers before configuring it. A wrong base
    # address reads back as zeroes, and zeroes look like a peripheral that is
    # merely unconfigured. ENDN is the better of the two: one correct value and
    # no other.
    step("read", REG["CREL"], 0, 0,
         "core release, and which M_CAN version this part carries")
    step("expect", REG["ENDN"], 0xFFFFFFFF, 0x87654321,
         "endianness, the one register with one answer")

    # 3 and 4. INIT, then confirmed. The driver refuses to configure a peripheral
    # that is not in INIT rather than trying, so the confirmation is not manners.
    step("modify", REG["CCCR"], CCCR["INIT"], CCCR["INIT"],
         "stop the peripheral")
    step("wait set", REG["CCCR"], CCCR["INIT"], 0,
         "wait for it to be stopped")

    # 5. The message RAM, before anything points at it.
    step("clear ram", off["std_filters"], mp["total_bytes"], 0,
         "clear every word of the message RAM in use")

    # 6 and 7. CCE, legal only while INIT is set.
    step("modify", REG["CCCR"], CCCR["CCE"], CCCR["CCE"], "open configuration")
    step("wait set", REG["CCCR"], CCCR["CCE"], 0,
         "wait for configuration to be open")

    # 8 and 9. The element sizes, and they are not optional: the reset value is
    # 8 byte elements while this layout assumes 64.
    step("write", REG["RXESC"], 0,
         (ESC_64B << 0) | (ESC_64B << 4) | (ESC_64B << 8),
         "receive elements are 64 byte")
    step("write", REG["TXESC"], 0, ESC_64B << 0,
         "transmit elements are 64 byte")

    # 10. Zero ACCEPTS, which is the opposite of what the name suggests.
    step("write", REG["GFC"], 0, 0,
         "accept every non-matching frame into receive FIFO 0")

    # 11 to 17. Byte offsets from the message RAM base, not absolute addresses.
    step("write", REG["SIDFC"], 0,
         (layout["std_filters"] << 16) | off["std_filters"],
         "standard filter list")
    step("write", REG["XIDFC"], 0,
         (layout["ext_filters"] << 16) | off["ext_filters"],
         "extended filter list")
    step("write", REG["RXF0C"], 0,
         (layout["rx_fifo0"] << 16) | off["rx_fifo0"], "receive FIFO 0")
    step("write", REG["RXF1C"], 0,
         (layout["rx_fifo1"] << 16) | off["rx_fifo1"], "receive FIFO 1")
    step("write", REG["RXBC"], 0, off["rx_buffers"],
         "receive buffers, none of them")
    step("write", REG["TXEFC"], 0,
         (layout["tx_event"] << 16) | off["tx_event"],
         "transmit event FIFO")
    step("write", REG["TXBC"], 0,
         (layout["tx_buffers"] << 24) | off["tx_buffers"],
         "transmit FIFO or queue")

    # 18 and 19. CCCR first, then TEST. CCCR.TEST is what makes TEST writable.
    step("modify", REG["CCCR"], clear | set_, set_, "the mode bits")
    step("write", REG["TEST"], 0, test,
         "the test register, writable only now")

    # 20. The timing, late, which is not where instinct puts it.
    step("write", REG["NBTP"], 0, nbtp, "nominal bit timing")

    # 21. Configuration closed, INIT still set.
    step("modify", REG["CCCR"], CCCR["CCE"], 0, "close configuration")

    # 22 and 23. And only now is it on the bus.
    step("modify", REG["CCCR"], CCCR["INIT"], 0, "start")
    step("wait clear", REG["CCCR"], CCCR["INIT"], 0,
         "wait until it is running")

    if len(s) > PLAN_MAX:
        return None, None
    return s, timing


# The configurations this chapter commits to, and the ones that must be refused.
# A plan builder that has never refused anything has not been tested.
CASES = [
    ("the first image: classic, 500 kbit/s from the 8 MHz HSE, on the bus",
     {"kernel_hz": 8_000_000, "bitrate": 500_000, "want_sp_permille": 800,
      "mode": "normal", "one_shot": False}),
    ("the same, in internal loopback, which needs no bus at all",
     {"kernel_hz": 8_000_000, "bitrate": 500_000, "want_sp_permille": 800,
      "mode": "loopback internal", "one_shot": False}),
    ("the same, in external loopback, which puts it on the wire",
     {"kernel_hz": 8_000_000, "bitrate": 500_000, "want_sp_permille": 800,
      "mode": "loopback external", "one_shot": False}),
    ("listening only, which receives and never acknowledges",
     {"kernel_hz": 8_000_000, "bitrate": 500_000, "want_sp_permille": 800,
      "mode": "listen only", "one_shot": False}),
    ("one shot, no automatic retransmission",
     {"kernel_hz": 8_000_000, "bitrate": 500_000, "want_sp_permille": 800,
      "mode": "normal", "one_shot": True}),
    ("the fallback rate, 250 kbit/s",
     {"kernel_hz": 8_000_000, "bitrate": 250_000, "want_sp_permille": 800,
      "mode": "normal", "one_shot": False}),
    ("REFUSE, a bit rate the 8 MHz HSE cannot divide into exactly",
     {"kernel_hz": 8_000_000, "bitrate": 666_667, "want_sp_permille": 800,
      "mode": "normal", "one_shot": False}),
    ("REFUSE, 2 Mbit/s nominal from 8 MHz is 4 quanta, below the floor",
     {"kernel_hz": 8_000_000, "bitrate": 2_000_000, "want_sp_permille": 800,
      "mode": "normal", "one_shot": False}),
]


def build():
    out = []
    for label, cfg in CASES:
        steps, timing = plan(cfg, mr.LAYOUT)
        out.append({
            "label": label,
            "config": dict(cfg, mode_code=MODE[cfg["mode"]]),
            "ok": steps is not None,
            "step_count": len(steps) if steps else 0,
            "timing": timing,
            "steps": steps,
        })
    return {
        "note": "Generated by tools/gen_initplan.py. Do not edit.",
        "registers": REG,
        "cccr_bits": CCCR,
        "ops": OPS,
        "modes": MODES,
        "plan_max": PLAN_MAX,
        "cases": out,
    }


def as_header(doc):
    """The same vectors as a C header, so the C test needs no JSON parser."""
    lines = [
        "/* Generated by tools/gen_initplan.py. Do not edit. */",
        "#ifndef JOINT_INITPLAN_VECTORS_H",
        "#define JOINT_INITPLAN_VECTORS_H",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        "typedef struct {",
        "    uint32_t    op;",
        "    uint32_t    reg;",
        "    uint32_t    mask;",
        "    uint32_t    value;",
        "} ip_step_t;",
        "",
        "typedef struct {",
        "    const char     *label;",
        "    uint32_t        kernel_hz;",
        "    uint32_t        bitrate;",
        "    uint32_t        want_sp_permille;",
        "    uint32_t        mode;",
        "    bool            one_shot;",
        "    bool            ok;",
        "    uint32_t        step_count;",
        "    const ip_step_t *steps;",
        "} ip_vector_t;",
        "",
    ]
    for i, c in enumerate(doc["cases"]):
        if not c["ok"]:
            continue
        lines.append(f"static const ip_step_t IP_STEPS_{i}[] = {{")
        for st in c["steps"]:
            lines.append("    {{ {op}u, 0x{reg:03X}u, 0x{mask:08X}u, "
                         "0x{value:08X}u }},   /* {name} */".format(**st))
        lines.append("};")
        lines.append("")
    lines.append("static const ip_vector_t IP_VECTORS[] = {")
    for i, c in enumerate(doc["cases"]):
        cfg = c["config"]
        lines.append(
            '    {{ "{label}", {k}u, {b}u, {sp}u, {mode}u, {os}, {ok}, {n}u, '
            "{steps} }},".format(
                label=c["label"], k=cfg["kernel_hz"], b=cfg["bitrate"],
                sp=cfg["want_sp_permille"], mode=cfg["mode_code"],
                os="true" if cfg["one_shot"] else "false",
                ok="true" if c["ok"] else "false", n=c["step_count"],
                steps=f"IP_STEPS_{i}" if c["ok"] else "0"))
    lines += [
        "};",
        "",
        "#define IP_VECTOR_COUNT "
        "((int) (sizeof IP_VECTORS / sizeof IP_VECTORS[0]))",
        "",
        "#endif /* JOINT_INITPLAN_VECTORS_H */",
        "",
    ]
    return "\n".join(lines)


def table(doc):
    """The first image's plan, step by step, for doc/node-order.md."""
    case = next(c for c in doc["cases"] if c["ok"])
    inv = {v: k for k, v in REG.items()}
    rows = ["| # | Operation | Register | Mask | Value | Why |",
            "|---|---|---|---|---|---|"]
    for n, st in enumerate(case["steps"], 1):
        if st["op_name"] == "clear ram":
            reg = "message RAM"
            mask = f"{st['mask']} bytes"
            value = f"from byte {st['reg']}"
        else:
            reg = f"`{inv.get(st['reg'], hex(st['reg']))}`"
            mask = f"`0x{st['mask']:08X}`" if st["mask"] else "none"
            value = f"`0x{st['value']:08X}`"
        rows.append(f"| {n} | {st['op_name']} | {reg} | {mask} | {value} | "
                    f"{st['name']} |")
    return "\n".join(rows)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="fail if the committed vectors differ from these")
    ap.add_argument("--table", action="store_true",
                    help="print the table for doc/node-order.md")
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
        print(f"initplan vectors match the generator ({len(doc['cases'])} "
              f"configurations, "
              f"{sum(1 for c in doc['cases'] if not c['ok'])} of them refused)")
        return 0

    VECTORS_JSON.parent.mkdir(parents=True, exist_ok=True)
    VECTORS_JSON.write_text(js, encoding="utf-8")
    VECTORS_H.write_text(hd, encoding="utf-8")
    print(f"wrote {VECTORS_JSON.name} and {VECTORS_H.name}: "
          f"{len(doc['cases'])} configurations, "
          f"{sum(1 for c in doc['cases'] if not c['ok'])} of them refused")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
