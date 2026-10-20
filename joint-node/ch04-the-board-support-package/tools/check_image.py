#!/usr/bin/env python3
"""The first two words of a linked image, read back out of the image.

    python tools/check_image.py build/image.elf

Needs the cross toolchain, so this runs in WSL on win11 skyhorizon and in CI,
never on the win11 aquamarine authoring laptop.

WHY THIS EXISTS. test_startup.py checks that the SOURCE puts &_estack in slot 0
and Reset_Handler in slot 1. That is a check on what was written. It cannot see
what the linker did with it: a table discarded by --gc-sections, a section
placed somewhere other than the boot address, a symbol resolved to a different
definition. Every one of those produces an image that links and reports a
sensible size, and a part that does nothing when it is switched on.

So this reads the two words out of the .elf and compares them against the
symbols, which is the only check available without a board.

THE THUMB BIT IS NOT AN ERROR. The reset vector's low bit is set because the
core uses it to enter Thumb state, so the word is the handler's address with 1
added. A check that compared the raw word against the symbol would fail on a
perfectly good image, which is why the comparison masks it off and then insists
it was set: a reset vector with that bit CLEAR puts the core into ARM state,
which this part does not have, and the result is a fault on the first
instruction.
"""
import re
import subprocess
import sys
from pathlib import Path

PREFIX = "arm-none-eabi-"


def run(args):
    out = subprocess.run(args, capture_output=True, text=True)
    if out.returncode != 0:
        print(f"  FAIL {' '.join(args)} exited {out.returncode}")
        print(out.stderr.strip()[:400])
        return None
    return out.stdout


def symbols(elf):
    """Every symbol as name to address, from nm."""
    out = run([PREFIX + "nm", elf])
    if out is None:
        return None
    syms = {}
    for line in out.splitlines():
        m = re.match(r"^([0-9A-Fa-f]+)\s+\S\s+(\S+)$", line.strip())
        if m:
            syms[m.group(2)] = int(m.group(1), 16)
    return syms


def isr_words(elf, count=2):
    """The first `count` words of .isr_vector, little endian, from objdump."""
    out = run([PREFIX + "objdump", "-s", "-j", ".isr_vector", elf])
    if out is None:
        return None
    words, addr = [], None
    for line in out.splitlines():
        m = re.match(r"^\s*([0-9A-Fa-f]+)\s+((?:[0-9A-Fa-f]{2,8}\s+){1,4})",
                     line)
        if not m:
            continue
        if addr is None:
            addr = int(m.group(1), 16)
        for group in m.group(2).split():
            if len(group) != 8:
                continue
            # objdump prints each 4 byte group in memory order, so a little
            # endian word has to be byte swapped back.
            b = bytes.fromhex(group)
            words.append(int.from_bytes(b, "little"))
        if len(words) >= count:
            break
    return addr, words[:count]


def main(argv):
    if len(argv) != 1:
        print(__doc__.strip().splitlines()[2].strip())
        return 2
    elf = argv[0]
    if not Path(elf).exists():
        print(f"  FAIL {elf} does not exist, so nothing was linked")
        return 1

    failures = 0
    syms = symbols(elf)
    got = isr_words(elf)
    if syms is None or got is None:
        return 1
    addr, words = got
    if len(words) < 2:
        print("  FAIL .isr_vector holds fewer than two words, so the table was "
              "discarded or never placed")
        return 1

    boot = syms.get("_boot_address")
    estack = syms.get("_estack")
    reset = syms.get("Reset_Handler")

    for name, value in (("_boot_address", boot), ("_estack", estack),
                        ("Reset_Handler", reset)):
        if value is None:
            print(f"  FAIL the image defines no {name}")
            failures += 1
    if failures:
        return 1

    # 1. The section has to be AT the boot address. The script asserts this at
    # link time; checking it here as well costs nothing and catches a script
    # whose assertion was removed.
    if addr != boot:
        print(f"  FAIL .isr_vector is at 0x{addr:08X} and the boot address is "
              f"0x{boot:08X}, so the part will fetch its first two words from "
              f"somewhere else")
        failures += 1

    # 2. Word 0 is the initial stack pointer, and it must be the END of the
    # stack region. _sstack would be the bottom, and the first push would
    # underflow out of the region into .bss.
    if words[0] != estack:
        print(f"  FAIL word 0 is 0x{words[0]:08X} and _estack is "
              f"0x{estack:08X}; the stack grows down, so the pointer starts at "
              f"the END of the region")
        failures += 1

    # 3. Word 1 is the reset vector, which is Reset_Handler with the Thumb bit
    # set. Both halves of that matter and are checked separately so the message
    # names which one is wrong.
    if (words[1] & ~1) != reset:
        print(f"  FAIL word 1 is 0x{words[1]:08X} and Reset_Handler is at "
              f"0x{reset:08X}, so the part will branch somewhere else")
        failures += 1
    elif (words[1] & 1) == 0:
        print(f"  FAIL word 1 is 0x{words[1]:08X} with the Thumb bit clear, "
              f"which asks the core for ARM state that this part does not "
              f"have, and it faults on the first instruction")
        failures += 1

    if failures:
        print(f"check_image: {failures} failure(s)")
        return 1
    print(f"ok  .isr_vector at 0x{addr:08X}, the initial stack pointer is "
          f"_estack 0x{words[0]:08X}, the reset vector is Reset_Handler "
          f"0x{reset:08X} with the Thumb bit set")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
