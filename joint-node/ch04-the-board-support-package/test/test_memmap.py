#!/usr/bin/env python3
"""The memory map, against its own arithmetic and against the linker script.

    python test/test_memmap.py

The generator's output is checked for drift by `--check`. These are the rules
that catch a map which is internally wrong, which a regenerated file would agree
with perfectly:

  - two regions that overlap, so an image can be linked into both
  - a region the script declares that the map does not record
  - the boot address somewhere other than the start of flash
  - a stack that does not fit, or that the script sized differently from here
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gen_memmap as g                                           # noqa: E402

HERE = Path(__file__).resolve().parent.parent
MEMORY_LD = HERE / "src" / "bsp" / "memory.ld"
SCRIPT_LD = HERE / "src" / "bsp" / "stm32h7a3.ld"
MEMMAP_H = HERE / "src" / "bsp" / "memmap.h"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def parse_memory_block(text):
    """The regions the script actually declares, read back out of the file.

    Read from the generated file rather than from the generator, because the
    rule being checked is that the two agree. Comparing the generator with
    itself would pass whatever the file said.
    """
    out = {}
    for m in re.finditer(
            r"^\s*(\w+)\s*\([rwx]+\)\s*:\s*ORIGIN\s*=\s*(0x[0-9A-Fa-f]+)\s*,"
            r"\s*LENGTH\s*=\s*(\d+)K\s*$", text, re.M):
        out[m.group(1)] = (int(m.group(2), 16), int(m.group(3)) * g.K)
    return out


def main():
    ld = MEMORY_LD.read_text(encoding="utf-8")
    script = SCRIPT_LD.read_text(encoding="utf-8")
    header = MEMMAP_H.read_text(encoding="utf-8")

    declared = parse_memory_block(ld)

    # 1. No two regions may overlap. An image linked into overlapping regions
    # links, and one section quietly lands on top of another.
    for a, b in g.overlaps():
        fail(f"{a} and {b} overlap, so a section placed in one can land on the "
             f"other")

    # 2. Every region the script declares must be one the map records as used,
    # with the same origin and the same size, and no others. This is the rule
    # the generator exists to keep, read back out of the file it wrote.
    want = {r[0]: (r[1], r[2]) for r in g.used()}
    if not g.used():
        fail("no region carries a reason, so the script declares nothing and "
             "no image can be linked")
    if declared != want:
        for name in sorted(set(declared) | set(want)):
            if declared.get(name) != want.get(name):
                fail(f"{name}: the script declares {declared.get(name)} and "
                     f"the map says {want.get(name)}")
    if not declared:
        fail("the script declares no regions at all, so the MEMORY block was "
             "not parsed and every rule below it is vacuous")

    # 3. A region recorded but NOT declared must not appear in the MEMORY
    # block, because that is the whole point of the distinction: the three AXI
    # SRAM nodes tile into what looks like one megabyte and only the first is
    # known to need no enabling.
    for name, _o, _s, why, _w, _src in g.REGIONS:
        if not why.strip() and name in declared:
            fail(f"{name} has no reason recorded and the script declares it "
                 f"anyway")

    # 3b. AND EVERY DECLARED REGION'S REASON MUST CARRY A PROVENANCE MARKER.
    # This rule exists because the first version of the table used a boolean,
    # and a mutation flipping one character from False to True passed every
    # other rule in this file: once a region is marked usable, the rule above
    # stops applying to it. The field is a reason now, so declaring the 384 KB
    # nodes costs a sentence about what makes them reachable, and a sentence
    # with no marker is somebody's opinion rather than a reading.
    for name, _o, _s, why, _w, _src in g.REGIONS:
        if not why.strip():
            continue
        if not any(mark in why for mark in g.PROVENANCE):
            fail(f"{name} is declared to the linker and its reason carries no "
                 f"provenance marker, so nothing says where the claim that it "
                 f"is reachable came from")

    # 4. The boot address must be the start of flash, in the map, in the
    # generated linker symbol and in the header. Three places, because the one
    # that is wrong is the one nobody checked.
    flash = next(r for r in g.REGIONS if r[0] == "FLASH")
    if g.BOOT_ADDRESS != flash[1]:
        fail(f"the boot address 0x{g.BOOT_ADDRESS:08X} is not the start of "
             f"flash 0x{flash[1]:08X}")
    m = re.search(r"_boot_address\s*=\s*(0x[0-9A-Fa-f]+)\s*;", ld)
    if not m:
        fail("memory.ld defines no _boot_address, and the script's assertion "
             "on it would be a link error rather than a check")
    elif int(m.group(1), 16) != g.BOOT_ADDRESS:
        fail(f"memory.ld says the boot address is {m.group(1)} and the map "
             f"says 0x{g.BOOT_ADDRESS:08X}")
    if f"0x{g.BOOT_ADDRESS:08X}u" not in header:
        fail("memmap.h does not carry the boot address")

    # 5. The script must assert the two facts a wrong vector table placement
    # would otherwise hide. Checked by construct, not by the word ASSERT
    # appearing somewhere: a check that matched its own comment would pass on a
    # file with the assertions deleted and the prose left behind.
    for needle, why in (
            (r"ASSERT\s*\(\s*_sisr_vector\s*==\s*_boot_address",
             "the vector table lands on the boot address"),
            (r"ASSERT\s*\(\s*ORIGIN\(FLASH\)\s*==\s*_boot_address",
             "the flash region begins at the boot address"),
            (r"ASSERT\s*\(\s*_estack\s*<=",
             "the stack fits inside its region"),
            (r"ASSERT\s*\(\s*_sstack\s*>=\s*_ebss",
             "the stack does not overlap .bss")):
        if not re.search(needle, script):
            fail(f"the script does not assert that {why}")

    # 6. The stack must fit in the region it is placed in, computed here rather
    # than taken from the script's own sentence about it.
    m = re.search(r"_stack_size\s*=\s*(\d+)K\s*;", script)
    if not m:
        fail("the script does not state a stack size, so the assertions about "
             "it cannot be read")
    else:
        stack = int(m.group(1)) * g.K
        dtcm = next(r for r in g.REGIONS if r[0] == "DTCM")
        if stack >= dtcm[2]:
            fail(f"the stack is {stack // g.K} KB and DTCM is "
                 f"{dtcm[2] // g.K} KB, leaving nothing for .data and .bss")
        if stack % 8 != 0:
            fail(f"the stack size {stack} is not a multiple of 8, and the "
                 f"procedure call standard wants an 8 byte aligned stack")

    # 7. The script must include the generated block rather than carry its own,
    # and must not declare a MEMORY block of its own.
    if "INCLUDE memory.ld" not in script:
        fail("the script does not include the generated MEMORY block")
    if re.search(r"^\s*MEMORY\s*$", script, re.M):
        fail("the script carries its own MEMORY block, which is the hand edit "
             "the generator exists to prevent")

    # 8. There must be no heap. The mechanism is the absence of the `end`
    # symbol newlib's _sbrk needs, so the rule is that the script does not
    # define it. A script that defined it would give malloc somewhere to go
    # without anybody deciding that.
    if re.search(r"^\s*(end|_end|PROVIDE\s*\(\s*end)\s*=", script, re.M):
        fail("the script defines an end symbol, which gives newlib's _sbrk a "
             "heap nobody decided to have")

    # 9. Every region must be a whole number of kilobytes and word aligned,
    # because the generated block states lengths in K and an origin that is not
    # word aligned faults on the first access.
    for name, origin, size, _why, _w, _src in g.REGIONS:
        if size % g.K:
            fail(f"{name} is {size} bytes, which is not a whole number of KB, "
                 f"and the MEMORY block states lengths in K")
        if origin % 4:
            fail(f"{name} begins at 0x{origin:08X}, which is not word aligned")

    # 10. The contiguous runs the map derives must really tile. This is the
    # claim that makes the three AXI SRAM nodes look like one region, so it is
    # the claim most worth checking rather than asserting in prose.
    for run in g.contiguous_runs():
        rows = [r for r in g.REGIONS if r[0] in run]
        rows.sort(key=lambda r: r[1])
        for a, b in zip(rows, rows[1:]):
            if a[1] + a[2] != b[1]:
                fail(f"{a[0]} and {b[0]} are reported as contiguous and "
                     f"0x{a[1]:08X} plus {a[2]} is not 0x{b[1]:08X}")

    if failures:
        print(f"test_memmap: {len(failures)} failure(s)")
        return 1
    runs = g.contiguous_runs()
    print(f"ok  {len(g.REGIONS)} regions, {len(declared)} declared to the "
          f"linker and matching the map, no overlaps, "
          f"{len(runs)} contiguous runs proved, every declared region "
          f"carrying a sourced reason, four refusals present")
    return 0


if __name__ == "__main__":
    sys.exit(main())
