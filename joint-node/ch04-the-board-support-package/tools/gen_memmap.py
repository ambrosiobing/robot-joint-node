#!/usr/bin/env python3
"""The part's memory map, and the linker script's MEMORY block from it.

    python tools/gen_memmap.py            regenerate the generated files
    python tools/gen_memmap.py --check    fail if the committed ones differ
    python tools/gen_memmap.py --table    print the table for doc/memory-map.md

Chapter 4 owes this volume an image floor, and this is the first piece of it:
chapter 9 established on Tuesday 20 October 2026 that nothing here has ever
produced a firmware image, and that the four things missing are a linker script,
a startup, a console and a cross build that links rather than compiles.

A LINKER SCRIPT WITH A WRONG REGION SIZE LINKS. That is the whole reason the
MEMORY block is generated rather than typed. An image whose RAM region is
declared larger than the part has links without complaint, reports its size
happily, and overruns at run time into whatever is past the end; one declared
smaller than the part has links too, and silently wastes the rest. Neither is an
error at build time and neither announces itself at boot.

So the regions are data here, each with the file it was read from, and the
script's MEMORY block is produced from them. A hand edit of the block fails
--check rather than surviving to be overwritten by somebody who did not know it
was there.

WHAT THIS CHAPTER WILL NOT DO IS GUESS. Three facts about this part were read
rather than recalled, and one of them is deliberately not used:

  * The flash size is 2048 KB at 0x08000000. [vendor] Zephyr's
    dts/arm/st/h7/stm32h7a3Xi.dtsi, which is the size variant file for this
    part, read on Tuesday 20 October 2026.

  * ITCM is 64 KB at 0x00000000 and DTCM is 128 KB at 0x20000000. [vendor]
    Zephyr's dts/arm/st/h7/stm32h7.dtsi, shared across the family.

  * The AXI SRAM is split into THREE nodes at contiguous addresses: 256 KB at
    0x24000000, then 384 KB at 0x24040000, then 384 KB at 0x240A0000. [vendor]
    Zephyr's dts/arm/st/h7/stm32h7a3.dtsi. Their addresses tile exactly, which
    this file checks arithmetically, so they LOOK like one megabyte region.

    THEY ARE NOT TREATED AS ONE. Whether the second and third are reachable
    without a clock or power enable is not something either file says, and an
    image that declares a 1024 KB region and gets 256 KB of it links, boots, and
    fails the first time it touches the upper three quarters. [vendor] Zephyr's
    own board file for this exact board chooses `zephyr,sram = &sram0`, the
    256 KB node alone, which is evidence that THAT one needs nothing extra and
    no evidence at all about the other two. So the script uses sram0 and the map
    records the rest as present and unused, with the question open.
"""
import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
LD_MEMORY = HERE / "src" / "bsp" / "memory.ld"
MEMMAP_H = HERE / "src" / "bsp" / "memmap.h"

K = 1024

# (name, origin, bytes, declarable_because, what it is, provenance)
#
# `declarable_because` is the column that matters, and it is a REASON rather
# than a boolean on purpose. A region recorded here is a fact about the part; a
# region the script declares to the linker is a promise that an image may put
# sections in it and they will be there at run time. Those are different claims,
# and the second one needs evidence.
#
# An empty reason means the region is recorded and not declared. A non-empty one
# has to carry a provenance marker, which test_memmap.py checks, because the
# first version of this table used a boolean and a mutation that flipped one
# character from False to True passed every rule in the file. Declaring the two
# 384 KB nodes now costs a sentence saying what makes them reachable without a
# clock or power enable, which is a claim somebody stands behind rather than a
# flag somebody flips.
REGIONS = [
    ("ITCM", 0x00000000, 64 * K, "",
     "instruction tightly coupled memory, and the boot alias lives here too",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7.dtsi"),
    ("FLASH", 0x08000000, 2048 * K,
     "[vendor] the part boots from it, so it is reachable before any clock is\n"
     "     configured at all",
     "program memory, and where the reset vector is fetched from",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3Xi.dtsi"),
    ("DTCM", 0x20000000, 128 * K,
     "[vendor] tightly coupled to the core rather than on a bus, so it needs no\n"
     "     peripheral clock; Zephyr's board file chooses it for the DTCM region",
     "data tightly coupled memory, no cache and no DMA reach",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7.dtsi"),
    ("AXISRAM", 0x24000000, 256 * K,
     "[vendor] Zephyr's board file for this exact board chooses this node as\n"
     "     zephyr,sram, which is evidence it needs nothing extra enabled",
     "the AXI SRAM node this board's own device tree chooses for the kernel",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi, and the board's chosen node"),
    ("AXISRAM2", 0x24040000, 384 * K, "",
     "contiguous with AXISRAM, and NOT USED: see the open question",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi"),
    ("AXISRAM3", 0x240A0000, 384 * K, "",
     "contiguous with AXISRAM2, and NOT USED: see the open question",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi"),
    ("SRAM_D2A", 0x30000000, 64 * K, "",
     "a different domain, so it has its own clock enable",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi"),
    ("SRAM_D2B", 0x30010000, 64 * K, "",
     "contiguous with SRAM_D2A, same domain",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi"),
    ("SRAM_D3", 0x38000000, 32 * K, "",
     "the always-on domain, which is what makes it useful for a fault record",
     "[vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi"),
]

# Where the part begins executing, and therefore where the vector table has to
# be. Not a choice: the core fetches the initial stack pointer and the reset
# vector from the first two words at the boot address.
BOOT_ADDRESS = 0x08000000

# The console, settled by the same board file rather than by assumption.
# [vendor] Zephyr boards/st/nucleo_h7a3zi_q/nucleo_h7a3zi_q.dts, whose chosen
# node names usart3 and whose pinctrl names usart3_tx_pd8 and usart3_rx_pd9.
#
# PORT D CARRIES BOTH THE CONSOLE AND THE BUS. Chapter 9 uses PD0 and PD1 for
# FDCAN1. These are PD8 and PD9. One port, two peripherals, one clock enable
# shared between them, so an image that configures either one by writing whole
# port registers rather than by read, modify, write takes the other's pins with
# it, and the symptom is a console that stops or a bus that goes quiet with
# nothing reported.
CONSOLE = {
    "peripheral": "USART3",
    "tx": "PD8",
    "rx": "PD9",
    "baud": 115200,
}


def used():
    """The regions the script declares, which are the ones with a reason."""
    return [r for r in REGIONS if r[3].strip()]


# The markers this volume accepts on a sourced claim, shared with chapter 10's
# checker. A reason carrying none of them is somebody's opinion.
PROVENANCE = ("[schematic]", "[wiki]", "[datasheet]", "[kernel]", "[measured]",
              "[inferred]", "[unconfirmed]", "[vendor]", "[chapter]",
              "[arithmetic]")


def kb(n):
    return n // K


def overlaps():
    """Every pair of regions that share an address, which must be none."""
    bad = []
    rows = sorted(REGIONS, key=lambda r: r[1])
    for a, b in zip(rows, rows[1:]):
        if a[1] + a[2] > b[1]:
            bad.append((a[0], b[0]))
    return bad


def contiguous_runs():
    """Runs of regions whose addresses tile exactly, as lists of names.

    Derived, not read: [arithmetic]. It is what makes the three AXI SRAM nodes
    look like one megabyte, and the reason the script does not treat them that
    way is that contiguous addresses are not evidence of a shared enable.
    """
    rows = sorted(REGIONS, key=lambda r: r[1])
    runs, cur = [], [rows[0][0]]
    for a, b in zip(rows, rows[1:]):
        if a[1] + a[2] == b[1]:
            cur.append(b[0])
        else:
            runs.append(cur)
            cur = [b[0]]
    runs.append(cur)
    return [r for r in runs if len(r) > 1]


def as_memory_ld():
    lines = [
        "/* Generated by tools/gen_memmap.py. Do not edit.",
        " *",
        " * The MEMORY block only. The SECTIONS block is in stm32h7a3.ld, which",
        " * includes this file, because where the regions are is a fact about the",
        " * part and what goes in them is a decision about the image.",
        " *",
        " * A region declared larger than the part has links, reports its size,",
        " * and overruns at run time. One declared smaller links too and wastes",
        " * the rest. Neither is a build error, which is why these numbers are",
        " * generated from doc/memory-map.md's own data rather than typed.",
        " */",
        "MEMORY",
        "{",
    ]
    width = max(len(r[0]) for r in used())
    for name, origin, size, _why, _what, _src in used():
        lines.append(
            f"    {name:<{width}} (rwx) : ORIGIN = 0x{origin:08X}, "
            f"LENGTH = {kb(size)}K")
    lines += [
        "}",
        "",
        "/* The boot address, as a LINKER symbol rather than a C macro, because",
        " * the script is what has to assert the vector table lands on it and a",
        " * script cannot read a header. The core fetches the initial stack",
        " * pointer from here and the reset vector from the four bytes after.",
        " */",
        f"_boot_address = 0x{BOOT_ADDRESS:08X};",
        "",
        "/* Recorded in doc/memory-map.md and deliberately NOT declared above:",
    ]
    for name, origin, size, why, what, _src in REGIONS:
        if not why.strip():
            lines.append(f" *   {name} at 0x{origin:08X}, {kb(size)}K, {what}")
    lines += [
        " */",
        "",
    ]
    return "\n".join(lines)


def as_header():
    lines = [
        "/* Generated by tools/gen_memmap.py. Do not edit. */",
        "#ifndef JOINT_MEMMAP_H",
        "#define JOINT_MEMMAP_H",
        "",
        "#include <stdint.h>",
        "",
        "/* Where the core fetches the initial stack pointer and the reset",
        " * vector. Not a choice. */",
        f"#define BOARD_BOOT_ADDRESS 0x{BOOT_ADDRESS:08X}u",
        "",
    ]
    for name, origin, size, why, _what, _src in REGIONS:
        tag = "" if why.strip() else "   /* recorded, not declared to the linker */"
        lines.append(f"#define BOARD_{name}_ORIGIN 0x{origin:08X}u{tag}")
        lines.append(f"#define BOARD_{name}_BYTES  {size}u")
    lines += [
        "",
        "/* The console, from this board's own device tree. Port D carries both",
        " * this and chapter 9's bus pins, PD0 and PD1, so neither may be",
        " * configured by writing a whole port register. */",
        f'#define BOARD_CONSOLE_PERIPHERAL "{CONSOLE["peripheral"]}"',
        f'#define BOARD_CONSOLE_TX_PIN "{CONSOLE["tx"]}"',
        f'#define BOARD_CONSOLE_RX_PIN "{CONSOLE["rx"]}"',
        f"#define BOARD_CONSOLE_BAUD {CONSOLE['baud']}u",
        "",
        "#endif /* JOINT_MEMMAP_H */",
        "",
    ]
    return "\n".join(lines)


def table():
    rows = ["| Region | Origin | Size | Declared, and why | What it is | "
            "Source |", "|---|---|---|---|---|---|"]
    for name, origin, size, why, what, src in sorted(REGIONS,
                                                     key=lambda r: r[1]):
        col = ("**yes**: " + " ".join(why.split())) if why.strip() else "no"
        rows.append(f"| `{name}` | `0x{origin:08X}` | {kb(size)} KB | "
                    f"{col} | {what} | {src} |")
    out = ["\n".join(rows), ""]

    runs = contiguous_runs()
    out.append("`[arithmetic]` Runs whose addresses tile exactly, derived here "
               "rather than read anywhere:")
    out.append("")
    for run in runs:
        total = sum(r[2] for r in REGIONS if r[0] in run)
        first = min(r[1] for r in REGIONS if r[0] in run)
        out.append(f"- {', '.join(run)}: `0x{first:08X}` for "
                   f"{kb(total)} KB with no gap")
    out.append("")
    out.append(f"Declared to the linker: "
               f"{sum(kb(r[2]) for r in used())} KB over {len(used())} "
               f"regions, of {sum(kb(r[2]) for r in REGIONS)} KB the part has.")
    return "\n".join(out)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--table", action="store_true")
    a = ap.parse_args(argv)

    ld, hd = as_memory_ld(), as_header()

    if a.table:
        print(table())
        return 0

    if a.check:
        bad = 0
        for path, want in ((LD_MEMORY, ld), (MEMMAP_H, hd)):
            have = path.read_text(encoding="utf-8") if path.exists() else ""
            if have.replace("\r\n", "\n") != want:
                print(f"{path.name} differs from the generator")
                bad += 1
        if bad:
            print("a hand edit does not survive here; regenerate instead")
            return 1
        print(f"memory map matches the generator ({len(REGIONS)} regions, "
              f"{len(used())} declared to the linker)")
        return 0

    LD_MEMORY.parent.mkdir(parents=True, exist_ok=True)
    LD_MEMORY.write_text(ld, encoding="utf-8")
    MEMMAP_H.write_text(hd, encoding="utf-8")
    print(f"wrote {LD_MEMORY.name} and {MEMMAP_H.name}: {len(REGIONS)} "
          f"regions, {len(used())} declared to the linker")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
