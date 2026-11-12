#!/usr/bin/env python3
"""The register map probe, and the boundary it must not cross.

    python test/test_fdcanprobe.py

This file guards an experimental design rather than an implementation, and the
rule that matters most is the narrowest one:

  **EXACTLY ONE RCC WRITE, AND IT IS THE VARIANT'S OWN OFFSET.** The probe
  exists to find out which of two register maps this silicon implements. Writing
  to the competing candidate would change the state of the address whose meaning
  is under test, so a second write anywhere in the file is a defect in the
  experiment and not merely untidy code. Joseph drew that line on Thursday
  12 November 2026, against my own argument that the other address "must be
  reserved anyway", which assumed the conclusion.

The rest:

  - THE BASELINE IS READ BEFORE THE WRITE, or it is not a baseline.
  - BOTH TRIPLES ARE WATCHED, so one capture shows which map the part has rather
    than only whether this image's own guess took.
  - NOTHING IS WRITTEN TO FDCAN AT ALL. Two register reads; configuring the
    controller is chapter 9's plan and needs a bit timing and a transceiver.
  - THE VARIANT IS NEVER DEFAULTED. An image that cannot say which candidate it
    wrote produces a capture nobody can attribute, and the board is shared.
  - THE FDCAN FACTS MATCH CHAPTER 9'S GENERATOR, imported here rather than
    retyped, so the two chapters cannot drift apart about the register map.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
CH09 = HERE.parent / "ch09-can-fd-from-the-controller-out" / "tools"
PROBE = HERE / "src" / "app" / "fdcan_probe.c"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def strip_comments(code):
    code = re.sub(r"/\*.*?\*/", " ", code, flags=re.S)
    return re.sub(r"//[^\n]*", " ", code)


def main():
    raw = PROBE.read_text(encoding="utf-8")
    code = strip_comments(raw)

    # 1. THE BOUNDARY. Every write through a volatile pointer, counted. The
    # probe writes one RCC register and must write nothing else, so a count of
    # anything but one is a failure whatever the extra write touches.
    deref_writes = re.findall(
        r"\*reg\s*\|=|\*\s*\(\s*volatile[^;]*?\)\s*[^;]*?=", code)
    total = len(deref_writes)
    if total != 1:
        fail(f"{total} writes through a volatile pointer, and the experiment "
             f"permits exactly one: writing the competing candidate would "
             f"change the state of the address under test")

    # And it must be the variant's own offset, by name rather than by literal,
    # so the two images cannot both write the same place.
    if "FP_WRITE_OFFSET" not in code:
        fail("the write does not go through FP_WRITE_OFFSET, so the variant "
             "switch does not control what is written")
    # Both offset literals appear in FP_WATCH and in the variant block, which
    # is correct and is why the check above is on the WRITE and not on whether
    # a literal occurs in the file.
    m = re.search(r"\*reg\s*\|=\s*(\w+)\s*;", code)
    if not m:
        fail("the one write is not a read-modify-write of a named bit, and "
             "this register holds other peripherals' enables")
    elif m.group(1) != "FP_FDCANEN":
        fail(f"the write sets {m.group(1)} rather than FP_FDCANEN")

    # 2. NOTHING IS WRITTEN TO FDCAN. The base must appear only in reads.
    for hit in re.finditer(r"FP_FDCAN1_BASE", code):
        ctx = code[max(0, hit.start() - 80):hit.end() + 80]
        if re.search(r"FP_FDCAN1_BASE[^;]*?(?:\|=|=[^=])", ctx) \
                and "rd(" not in ctx:
            fail("something writes through FP_FDCAN1_BASE, and this probe "
                 "reads two registers and configures nothing")
            break

    # 3. The baseline precedes the write, or it is not a baseline.
    before = code.find('watch_line("before")')
    write = code.find("*reg |=")
    after = code.find('watch_line("after")')
    if before < 0 or after < 0:
        fail("the before and after reads are not both present")
    elif not (before < write < after):
        fail(f"the order is wrong: before at {before}, write at {write}, "
             f"after at {after}; a baseline read after the write is not a "
             f"baseline")

    # 4. Both triples are watched, so one capture distinguishes the two maps.
    watch = re.search(r"FP_WATCH\[6\]\s*=\s*\{(.*?)\}", code, re.S)
    if not watch:
        fail("FP_WATCH is not a six element table")
    else:
        got = set(re.findall(r"0x([0-9A-Fa-f]{3})u", watch.group(1)))
        want = {"0E0", "0E8", "0EC", "140", "148", "14C"}
        missing = {w for w in want if w.upper() not in {g.upper() for g in got}}
        if missing:
            fail(f"FP_WATCH does not cover {sorted(missing)}, so the capture "
                 f"cannot show which register map the part implements")

    # 5. The variant is never defaulted, and the two are exclusive.
    #
    # THIS IS THE FORM AND NOT THE FACT, and a mutation proved it: replacing
    # `#if !defined(A) && !defined(B)` with `#if 0` left the `#error` in the
    # file and this rule green. So the CONDITION is now checked, not merely the
    # presence of an `#error` somewhere.
    #
    # The fact is that a build with neither -D must fail, and only a compiler
    # can establish that. `make fdcanprobe-a` and `-b` compile this file with
    # neither variant defined and REQUIRE a non-zero exit before building
    # anything, which is where the real guard lives because that is where the
    # compiler is.
    def errors_inside(condition):
        """Is there an `#error` inside the block this condition opens?

        BELONGING, NOT ADJACENCY. An earlier version of this rule required the
        `#error` on the line immediately after the `#if`, and a four line
        comment placed between them turned it red on correct code. A rule that
        depends on formatting rather than on structure produces false alarms,
        and a guard with false alarms is worse than no guard because what it
        throws away is real.
        """
        m = re.search(condition, raw)
        if not m:
            return False
        rest = raw[m.end():]
        stop = re.search(r"^\s*#\s*(endif|else|elif)", rest, re.M)
        block = rest[:stop.start()] if stop else rest
        return re.search(r"^\s*#\s*error", block, re.M) is not None

    if not errors_inside(r"#if\s*!\s*defined\s*\(\s*FP_VARIANT_A\s*\)\s*&&\s*"
                         r"!\s*defined\s*\(\s*FP_VARIANT_B\s*\)"):
        fail("no #error inside a block guarded by "
             "'!defined(FP_VARIANT_A) && !defined(FP_VARIANT_B)', so a build "
             "with neither could still produce an unattributable image")
    if not errors_inside(r"#if\s*defined\s*\(\s*FP_VARIANT_A\s*\)\s*&&\s*"
                         r"defined\s*\(\s*FP_VARIANT_B\s*\)"):
        fail("nothing refuses a build with both variants defined")

    # 6. THE FDCAN FACTS COME FROM CHAPTER 9, not from a second copy here.
    if not CH09.is_dir():
        fail(f"chapter 9's tools are not at {CH09}, so the register facts in "
             f"this probe cannot be checked against the chapter that owns them")
    else:
        sys.path.insert(0, str(CH09))
        try:
            import gen_initplan as ip
        except ImportError as e:
            fail(f"chapter 9's generator does not import: {e}")
            ip = None
        if ip is not None:
            for name, macro in (("CREL", "FP_FDCAN_CREL"),
                                ("ENDN", "FP_FDCAN_ENDN")):
                want = ip.REG[name]
                m = re.search(r"#define\s+" + macro + r"\s+0x([0-9A-Fa-f]+)u",
                              code)
                if not m:
                    fail(f"{macro} is not defined")
                elif int(m.group(1), 16) != want:
                    fail(f"{macro} is 0x{int(m.group(1), 16):03X} and chapter "
                         f"9's generator says {name} is 0x{want:03X}")
            # And the one value the part supplies rather than the software.
            m = re.search(r"#define\s+FP_ENDN_EXPECTED\s+0x([0-9A-Fa-f]+)u",
                          code)
            if not m:
                fail("FP_ENDN_EXPECTED is not defined, so the only "
                     "falsifiable check on the base address is missing")
            elif int(m.group(1), 16) != 0x87654321:
                fail(f"FP_ENDN_EXPECTED is 0x{int(m.group(1), 16):08X} and the "
                     f"endianness register answers 0x87654321")

    # 7. The three sections Joseph asked to be kept apart are all reported, and
    # the enable section does not stand in for the clock section: a set enable
    # bit does not prove the controller's core has a usable clock.
    for needed, why in (
            ("clocktree cr", "the oscillator state, at an undisputed offset"),
            ("clocktree cdccip1r", "the core clock selector"),
            ("hsidiv", "whether the oscillator is undivided, read from the "
                       "part rather than inferred from legible text"),
            ("fdcansel", "which source the controller's core is given"),
            ("hserdy", "whether that source is actually running"),
            ("fdcan1", "the two register reads")):
        if needed not in code:
            fail(f"the report has no {needed!r}, which is {why}")

    # 8. The tag differs between the two variants, or no capture can be
    # attributed to one of them.
    tags = set(re.findall(r'#define\s+FP_TAG\s+"([^"]+)"', code))
    if len(tags) != 2:
        fail(f"{len(tags)} distinct tags are defined and there must be two, "
             f"one per variant: {sorted(tags)}")

    if failures:
        print(f"test_fdcanprobe: {len(failures)} failure(s)")
        return 1
    print("ok  one RCC write at the variant's own offset and no other, the "
          "baseline read before it, both register maps watched, nothing "
          "written to FDCAN, and the FDCAN facts taken from chapter 9")
    return 0


if __name__ == "__main__":
    sys.exit(main())
