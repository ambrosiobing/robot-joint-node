#!/usr/bin/env python3
"""The startup, checked by construct for the four mistakes with no symptom.

    python test/test_startup.py

None of this runs the code. It reads startup.c and the linker script for the
constructs that have to be right, because three of the four produce a board that
does nothing at all and the fourth produces variables holding the wrong values.
A board that does nothing gives no clue which of twenty things is wrong, so the
checks happen here where they can name the line.

Each rule matches THE CONSTRUCT and not the prose beside it. A check whose needle
appears in its own comment matches itself, and would pass on a file with the code
deleted and the comments left behind.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
STARTUP = HERE / "src" / "bsp" / "startup.c"
SCRIPT = HERE / "src" / "bsp" / "stm32h7a3.ld"

CORE_VECTORS = 16

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def strip_comments(text):
    """Comments out, so no rule can match the prose explaining it."""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def body_of(code, name):
    """One function's body, by brace counting from its definition."""
    m = re.search(r"\b" + re.escape(name) + r"\s*\([^)]*\)\s*\{", code)
    if not m:
        return None
    i = m.end() - 1
    depth = 0
    for j in range(i, len(code)):
        if code[j] == "{":
            depth += 1
        elif code[j] == "}":
            depth -= 1
            if depth == 0:
                return code[i + 1:j]
    return None


def vector_entries(code):
    """The initialiser list of the table, as a list of expression strings."""
    m = re.search(r"g_isr_vector\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", code, re.S)
    if not m:
        return None
    return [e.strip() for e in m.group(1).split(",") if e.strip()]


def main():
    raw = STARTUP.read_text(encoding="utf-8")
    code = strip_comments(raw)
    script = strip_comments(SCRIPT.read_text(encoding="utf-8"))

    entries = vector_entries(code)

    # 1. THE FIRST WORD IS _estack, NOT _sstack. The stack grows down, so the
    # pointer starts at the end of the region. _sstack points at the bottom and
    # the first push underflows into .bss.
    if entries is None:
        fail("no vector table initialiser was found, so every rule about it "
             "below is vacuous")
    else:
        if not re.search(r"&\s*_estack", entries[0]):
            fail(f"the first vector entry is {entries[0]!r} and the initial "
                 f"stack pointer has to be &_estack, the END of the region")
        if re.search(r"&\s*_sstack", entries[0]):
            fail("the first vector entry is &_sstack, the BOTTOM of the stack, "
                 "so the first push will underflow out of the region")

        # 2. Sixteen core entries, no more and no fewer. Fewer and the core
        # fetches a handler from past the end; more means device vectors have
        # been added, which this chapter has not read from a source yet.
        if len(entries) != CORE_VECTORS:
            fail(f"the table has {len(entries)} entries and the sixteen core "
                 f"vectors are {CORE_VECTORS}; adding device entries means "
                 f"their positions were read from a checked source, which has "
                 f"not happened")

        # 3. Every slot is either a reserved zero or a handler that exists.
        for n, e in enumerate(entries):
            if re.fullmatch(r"0u?", e):
                continue
            m = re.search(r"\b(\w+)\s*$", e)
            if not m:
                fail(f"vector {n} is {e!r}, which is neither a zero nor a name")
                continue
            name = m.group(1)
            # A leading underscore is a linker symbol rather than a handler,
            # and rule 1 is what judges whether the right one is in slot 0.
            # Reporting it here as an undeclared handler names the wrong
            # defect, which sends a reader looking for a missing function.
            if name.startswith("_"):
                continue
            if not re.search(r"\bvoid\s+" + re.escape(name) + r"\s*\(", code):
                fail(f"vector {n} names {name} and nothing in this file "
                     f"declares it, so the linker will take it from wherever "
                     f"it finds one")

    # 4. THE COPY READS _sidata AND WRITES _sdata. _sidata is where the values
    # are stored in flash; _sdata is where they go. A loop reading _sdata does
    # nothing and leaves every initialised variable holding whatever the RAM
    # came up with.
    reset = body_of(code, "Reset_Handler")
    if reset is None:
        fail("Reset_Handler was not found, so the rules about it are vacuous")
    else:
        if not re.search(r"=\s*&\s*_sidata\b", reset):
            fail("Reset_Handler never takes the address of _sidata, so the "
                 "initialised data is not copied from where it is stored")
        if not re.search(r"=\s*&\s*_sdata\b", reset):
            fail("Reset_Handler never takes the address of _sdata, so the "
                 "initialised data has nowhere to go")
        if re.search(r"src\s*=\s*&\s*_sdata\b", reset):
            fail("the copy reads _sdata, which is its own destination, so the "
                 "loop moves nothing")

        # 5. .bss is zeroed, between _sbss and _ebss.
        if not (re.search(r"&\s*_sbss\b", reset)
                and re.search(r"&\s*_ebss\b", reset)):
            fail("Reset_Handler does not walk _sbss to _ebss, so .bss is left "
                 "holding whatever the RAM came up with")

        # 6. NO LIBRARY CALL BEFORE THE MEMORY IS SET UP. memcpy and memset are
        # the obvious way to write both loops and are the wrong way: a library
        # whose own state lives in .data or .bss would be running before its
        # state exists.
        before_main = reset.split("main(")[0]
        for bad in re.findall(r"\b(mem\w+|str\w+|printf|malloc|calloc)\s*\(",
                              before_main):
            fail(f"Reset_Handler calls {bad} before the copy and the zeroing "
                 f"are finished, and that library's own state is in the memory "
                 f"being set up")

        # 7. main is called, and the handler does not return.
        if "main(" not in reset:
            fail("Reset_Handler never calls main")
        if not re.search(r"for\s*\(\s*;\s*;\s*\)", reset):
            fail("Reset_Handler can return, and there is nothing to return to")

    # 8. The table has to survive two separate passes: the compiler, which
    # discards an object nothing references, and the linker with --gc-sections.
    if not re.search(r"section\s*\(\s*\"\.isr_vector\"\s*\)", code):
        fail("the table is not placed in .isr_vector, so the script's rule "
             "about where it goes does not apply to it")
    if not re.search(r"\bused\b", code):
        fail("the table is not marked used, so the compiler may discard it "
             "before the linker ever sees it")
    if not re.search(r"KEEP\s*\(\s*\*\s*\(\s*\.isr_vector\s*\)\s*\)", script):
        fail("the script does not KEEP .isr_vector, so --gc-sections discards "
             "the table and the image still links")

    # 9. The entry point the script names must exist here.
    m = re.search(r"ENTRY\s*\(\s*(\w+)\s*\)", script)
    if not m:
        fail("the script names no entry point")
    elif not re.search(r"\bvoid\s+" + re.escape(m.group(1)) + r"\s*\(",
                       code):
        fail(f"the script's entry point {m.group(1)} is not defined in the "
             f"startup")

    if failures:
        print(f"test_startup: {len(failures)} failure(s)")
        return 1
    print(f"ok  {CORE_VECTORS} core vectors, the stack pointer at the end of "
          f"its region, the copy from where it is stored, no library call "
          f"before the memory exists")
    return 0


if __name__ == "__main__":
    sys.exit(main())
