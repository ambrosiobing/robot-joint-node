#!/usr/bin/env python3
"""The vector table against its generator, and against what makes it correct.

    python test/test_vectors.py

Comparing the C against the generator is the cheap half and `--check` already
does it. What this file is for is the set of claims that would still be false if
the generator and its output agreed perfectly:

  - THE DEVICE POSITIONS START AT VECTOR 16. Position 0 is WWDG and it is the
    seventeenth word of the image, because the sixteen core exceptions come
    first. Off by one here puts every handler one slot out, and the image links,
    and the console still works because nothing calls a handler until an
    interrupt is enabled. This is the defect this file exists for.

  - THE RESERVED POSITIONS ARE PRESENT AND ARE NOT AT THE END. 21 of the 155
    device positions have no name, the first at 61, and a table that omitted
    them would be 21 entries short with every later handler displaced.

  - EVERY DEVICE HANDLER IS WEAK. If one were strong, a chapter defining its own
    USART3_IRQHandler would get a duplicate symbol error rather than the
    override the weak alias is there to allow.

  - THE ALIAS TARGET IS DEFINED IN THIS SAME FILE. GCC resolves an alias
    attribute within a translation unit, so a weak alias to a symbol that is
    only declared here does not compile at all: `aliased to undefined symbol`.
    This file originally aliased to `Default_Handler_alias`, which startup.c
    defines, and every handler failed. The rule below checks for a DEFINITION
    in vectors.c and not merely a declaration.

    That is the one rule here that was added because a compiler found what the
    text rules could not. Checking that an alias is weak and names the default
    is checking the form; whether the target is in the same translation unit is
    the fact.

There is no C compiler on the win11 aquamarine authoring laptop, so none of this
is built where it is written. Every rule here is therefore about the text.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE / "tools"))
import gen_vectors as g                                         # noqa: E402

VECTORS_C = HERE / "src" / "bsp" / "vectors.c"
VECTORS_H = HERE / "src" / "bsp" / "vectors.h"
STARTUP = HERE / "src" / "bsp" / "startup.c"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def entries(code):
    m = re.search(r"g_isr_vector\s*\[\s*\d+\s*\]\s*=\s*\{(.*?)\n\};",
                  code, re.S)
    if not m:
        return None
    out = []
    for line in m.group(1).splitlines():
        line = re.sub(r"/\*.*?\*/", "", line).strip()
        if not line:
            continue
        for e in line.split(","):
            e = e.strip()
            if e:
                out.append(e)
    return out


def main():
    code = VECTORS_C.read_text(encoding="utf-8")
    header = VECTORS_H.read_text(encoding="utf-8")
    startup = STARTUP.read_text(encoding="utf-8")

    ent = entries(code)
    if ent is None:
        fail("no g_isr_vector initialiser was found, so every rule below is "
             "vacuous")
        print(f"test_vectors: {len(failures)} failure(s)")
        return 1

    if len(ent) != g.ENTRIES:
        fail(f"{len(ent)} entries in the C and {g.ENTRIES} in the generator")

    # 1. THE OFFSET. Device position p is vector 16 + p, and this is checked
    # against the vendor's own name rather than against a count, because a
    # count that is off by one is still a count.
    for p, name, _c in g.by_position():
        v = g.CORE_ENTRIES + p
        if v >= len(ent):
            fail(f"position {p} should be vector {v} and the table has only "
                 f"{len(ent)} entries")
            break
        want = "0u" if name is None else f"(uintptr_t) {g.handler(name)}"
        if ent[v] != want:
            fail(f"vector {v} is {ent[v]!r} and position {p} wants {want!r}; "
                 f"if every row is wrong by the same amount the device table "
                 f"starts at the wrong offset")
            break

    # 2. The four positions a second vendor confirmed, by absolute vector
    # number, written out rather than computed in a loop. The loop above would
    # pass if g.CORE_ENTRIES itself were wrong, because it uses the same
    # constant to build the expectation that the C was generated from.
    for name, vec in (("USART3", 55), ("FDCAN1_IT0", 35),
                      ("FDCAN1_IT1", 37), ("FDCAN_CAL", 79)):
        want = f"(uintptr_t) {name}_IRQHandler"
        if vec >= len(ent):
            fail(f"{name} should be vector {vec} and the table is shorter")
        elif ent[vec] != want:
            fail(f"vector {vec} is {ent[vec]!r} and Zephyr's device tree puts "
                 f"{name} there; 16 core exceptions plus its position is "
                 f"{vec}")

    # 3. The reserved positions are present, and not gathered at the end.
    res = g.reserved_positions()
    if not res:
        fail("no reserved positions at all, which contradicts the vendor enum")
    else:
        if res[0] >= g.DEVICE_ENTRIES - len(res):
            fail(f"the reserved positions start at {res[0]} and there are "
                 f"{len(res)} of them in {g.DEVICE_ENTRIES}, so they could be "
                 f"a tail; the whole argument for generating this table is "
                 f"that they are holes")
        for p in res:
            v = g.CORE_ENTRIES + p
            if v < len(ent) and ent[v] != "0u":
                fail(f"position {p} is reserved and vector {v} is {ent[v]!r}")
                break

    # 4. Every device handler is weak, and aliases the one target.
    for _p, name, _c in g.DEVICE:
        h = g.handler(name)
        m = re.search(re.escape(h) + r"\s*\(\s*void\s*\)\s*"
                      r"__attribute__\s*\(\s*\(\s*weak\s*,\s*"
                      r'alias\s*\(\s*"([^"]+)"\s*\)', code)
        if not m:
            fail(f"{h} is not declared as a weak alias, so a chapter defining "
                 f"its own would collide with it instead of overriding it")
            break
        if m.group(1) != "Vector_Default_Handler":
            fail(f"{h} aliases {m.group(1)!r} rather than the local default")
            break

    # 5. THE ALIAS TARGET IS DEFINED IN vectors.c, not merely declared there.
    # A DECLARATION is what this file had on Monday 9 November 2026 and GCC
    # refused all 134 aliases, because it resolves an alias within the
    # translation unit rather than leaving it to the linker. The body is what
    # makes this a definition, so the pattern requires the brace.
    if not re.search(r"\bvoid\s+Vector_Default_Handler\s*\(\s*void\s*\)\s*\{",
                     code):
        fail("vectors.c declares but does not DEFINE Vector_Default_Handler; "
             "GCC resolves an alias inside one translation unit, so every "
             "device handler would fail with 'aliased to undefined symbol'")
    if re.search(r"\bstatic\s+void\s+Vector_Default_Handler\b", code):
        fail("Vector_Default_Handler is static, and a global weak alias "
             "should not point at a symbol with internal linkage")

    # And it must reach the one place that decides what a default handler does,
    # rather than quietly becoming a second empty loop nobody maintains.
    m = re.search(r"void\s+Vector_Default_Handler\s*\(\s*void\s*\)\s*\{"
                  r"(.*?)\}", code, re.S)
    if m and "Default_Handler_alias" not in m.group(1):
        fail("Vector_Default_Handler does not call through to startup.c's "
             "default, so there are now two defaults and one of them has no "
             "fault record and no comment explaining it")

    if not re.search(r"\bvoid\s+Default_Handler_alias\s*\(\s*void\s*\)\s*\{",
                     startup):
        fail("startup.c does not define Default_Handler_alias, which the "
             "trampoline in vectors.c calls")
    if re.search(r"\bstatic\s+void\s+Default_Handler_alias\b", startup):
        fail("Default_Handler_alias is static, so vectors.c cannot call it")

    # 6. The alignment, from its own two rules rather than from the number.
    if g.ALIGN_BYTES & (g.ALIGN_BYTES - 1):
        fail(f"the alignment {g.ALIGN_BYTES} is not a power of two")
    if g.ALIGN_BYTES < 128:
        fail(f"the alignment {g.ALIGN_BYTES} is below VTOR's 128 byte floor")
    if g.ALIGN_BYTES < g.TABLE_BYTES:
        fail(f"the alignment {g.ALIGN_BYTES} is smaller than the table's "
             f"{g.TABLE_BYTES} bytes")
    if g.ALIGN_BYTES // 2 >= g.TABLE_BYTES and g.ALIGN_BYTES > 128:
        fail(f"the alignment {g.ALIGN_BYTES} is more than twice the table's "
             f"{g.TABLE_BYTES} bytes, so it is not the smallest that works "
             f"and the flash padding is waste nobody chose")

    # 7. The header's declarations agree with the C they were written beside.
    for name, want in (("BOARD_ISR_ENTRIES", g.ENTRIES),
                       ("BOARD_ISR_CORE_ENTRIES", g.CORE_ENTRIES),
                       ("BOARD_ISR_DEVICE_ENTRIES", g.DEVICE_ENTRIES),
                       ("BOARD_ISR_BYTES", g.TABLE_BYTES),
                       ("BOARD_ISR_ALIGN", g.ALIGN_BYTES)):
        m = re.search(r"#define\s+" + name + r"\s+(\d+)u", header)
        if not m:
            fail(f"{name} is not in vectors.h")
        elif int(m.group(1)) != want:
            fail(f"{name} is {m.group(1)} in the header and {want} in the "
                 f"generator")

    # 8. And the per-interrupt constants, which are what a driver will use to
    # enable one. A wrong one here enables the wrong interrupt, which is
    # exactly as quiet as a wrong vector.
    for _p, name, _c in g.DEVICE:
        m = re.search(r"#define\s+BOARD_IRQ_" + re.escape(name) + r"\s+(\d+)u",
                      header)
        if not m:
            fail(f"BOARD_IRQ_{name} is not in vectors.h")
            break
    for name, (want, where) in g.CORROBORATED.items():
        m = re.search(r"#define\s+BOARD_IRQ_" + re.escape(name) + r"\s+(\d+)u",
                      header)
        if m and int(m.group(1)) != want:
            fail(f"BOARD_IRQ_{name} is {m.group(1)} and Zephyr says {want} "
                 f"({where})")

    if failures:
        print(f"test_vectors: {len(failures)} failure(s)")
        return 1
    print(f"ok  {g.ENTRIES} vectors, {len(g.DEVICE)} named and {len(res)} "
          f"reserved holes, device position 0 at vector {g.CORE_ENTRIES}, "
          f"{len(g.CORROBORATED)} confirmed by a second vendor, "
          f"{g.TABLE_BYTES} bytes aligned to {g.ALIGN_BYTES}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
