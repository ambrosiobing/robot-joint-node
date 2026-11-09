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
VECTORS = HERE / "src" / "bsp" / "vectors.c"
VECTORS_H = HERE / "src" / "bsp" / "vectors.h"
SCRIPT = HERE / "src" / "bsp" / "stm32h7a3.ld"

# The rules below split across two files as of Monday 9 November 2026: the ones
# about the TABLE read vectors.c, which is generated, and the ones about the
# RESET read startup.c, which is not. A test that kept reading startup.c for the
# table would have passed every table rule vacuously, because the table is not
# there any more, and rule 2 is the only one that would have objected.

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
    """The initialiser list of the table, as a list of expression strings.

    The length is declared now, `g_isr_vector[171]`, so the pattern allows a
    bound. A declared bound is worth having: a table with fewer initialisers
    than its bound is zero filled by C and links silently, and the count below
    is what catches that.
    """
    m = re.search(r"g_isr_vector\s*\[\s*(\d*)\s*\]\s*=\s*\{(.*?)\}\s*;",
                  code, re.S)
    if not m:
        return None, None
    bound = int(m.group(1)) if m.group(1) else None
    return bound, [e.strip() for e in m.group(2).split(",") if e.strip()]


def declared(name, header):
    """One #define from the generated header, as an int."""
    m = re.search(r"#define\s+" + re.escape(name) + r"\s+(\d+)u?", header)
    return int(m.group(1)) if m else None


def main():
    raw = STARTUP.read_text(encoding="utf-8")
    code = strip_comments(raw)
    script = strip_comments(SCRIPT.read_text(encoding="utf-8"))
    table_code = strip_comments(VECTORS.read_text(encoding="utf-8"))
    header = VECTORS_H.read_text(encoding="utf-8")

    bound, entries = vector_entries(table_code)
    want_entries = declared("BOARD_ISR_ENTRIES", header)
    want_align = declared("BOARD_ISR_ALIGN", header)
    want_bytes = declared("BOARD_ISR_BYTES", header)

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

        # 2. AS MANY ENTRIES AS THE GENERATOR SAYS, no more and no fewer.
        # Fewer and the core fetches a handler from past the end of the table.
        #
        # This rule used to be "exactly sixteen", which was the chapter's way
        # of making the addition of device vectors deliberate rather than
        # accidental. It was deliberate on Monday 9 November 2026: the 155
        # device positions came from ST's IRQn_Type enum, corroborated against
        # Zephyr's device tree, and the count now comes from the header the
        # same generator writes rather than from a number typed here.
        if want_entries is None:
            fail("BOARD_ISR_ENTRIES is not in vectors.h, so there is nothing "
                 "to check the table's length against")
        elif len(entries) != want_entries:
            fail(f"the table has {len(entries)} initialisers and the "
                 f"generator declares {want_entries}; C zero fills the "
                 f"difference and the image links")
        if bound is None:
            fail("g_isr_vector has no declared bound, so a short initialiser "
                 "list is not a compile error anywhere")
        elif want_entries is not None and bound != want_entries:
            fail(f"g_isr_vector is declared [{bound}] and the generator says "
                 f"{want_entries}")

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
            if not re.search(r"\bvoid\s+" + re.escape(name) + r"\s*\(",
                             table_code):
                fail(f"vector {n} names {name} and nothing in vectors.c "
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
    if not re.search(r"section\s*\(\s*\"\.isr_vector\"\s*\)", table_code):
        fail("the table is not placed in .isr_vector, so the script's rule "
             "about where it goes does not apply to it")
    if not re.search(r"\bused\b", table_code):
        fail("the table is not marked used, so the compiler may discard it "
             "before the linker ever sees it")

    # 8c. THE SCRIPT'S ALIGN MUST BE THE ONE THE TABLE'S LENGTH REQUIRES, and
    # this is the rule that could not have existed while the table was sixteen
    # entries long, because 512 was then generous rather than wrong.
    #
    # A linker script cannot read a C header, so the number cannot be generated
    # into it the way the MEMORY block is generated from the map. Two files hold
    # it and this is what stops them drifting. It bit immediately: the script
    # said ALIGN(512), which was correct for 64 bytes of table and is too small
    # for 684.
    m = re.search(r"\.isr_vector\s*:\s*ALIGN\s*\(\s*(\d+)\s*\)", script)
    if not m:
        fail("the .isr_vector section has no ALIGN, so its base is whatever "
             "the previous section left and VTOR cannot express it")
    elif want_align is None:
        fail("BOARD_ISR_ALIGN is not in vectors.h, so the script's ALIGN has "
             "nothing to agree with")
    else:
        got = int(m.group(1))
        if got != want_align:
            fail(f"the script aligns .isr_vector to {got} and the table's "
                 f"{want_bytes} bytes need {want_align}; the alignment "
                 f"depends on the length and these are two separate files")
        if got & (got - 1):
            fail(f"the alignment {got} is not a power of two")
        if got < 128:
            fail(f"the alignment {got} is below VTOR's 128 byte floor, which "
                 f"is CMSIS core_cm7.h putting SCB_VTOR_TBLOFF at bit 7")
    if not re.search(r"KEEP\s*\(\s*\*\s*\(\s*\.isr_vector\s*\)\s*\)", script):
        fail("the script does not KEEP .isr_vector, so --gc-sections discards "
             "the table and the image still links")

    # 8b. NO ASSERT MESSAGE MAY BE SPLIT ACROSS TWO STRING LITERALS. GNU ld
    # does not concatenate adjacent string literals the way C does, so a
    # message wrapped the way C wraps one is a SYNTAX ERROR at link time.
    #
    # That cost a round trip to WSL on Tuesday 20 October 2026: the first
    # assertion's message was split over two lines, the other three were not,
    # and the only symptom was "syntax error" naming the line of the second
    # half. The link is the one check this laptop cannot run, so the class
    # belongs here where it can be caught before the push.
    #
    # Matched on the raw text rather than the comment-stripped copy, because
    # the construct is a quoted string and stripping comments leaves those
    # alone.
    raw_script = SCRIPT.read_text(encoding="utf-8")
    for m in re.finditer(r"ASSERT\s*\((.*?)\)\s*$", raw_script, re.S | re.M):
        body = m.group(1)
        if len(re.findall('"', body)) > 2:
            head = body.splitlines()[0].strip()[:60]
            fail("an ASSERT message is split across more than one string "
                 "literal, which GNU ld reads as a syntax error rather than "
                 "concatenating: ASSERT(" + head)

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
    print(f"ok  {want_entries} vectors aligned to {want_align}, the stack "
          f"pointer at the end of its region, the copy from where it is "
          f"stored, no library call before the memory exists")
    return 0


if __name__ == "__main__":
    sys.exit(main())
