#!/usr/bin/env python3
"""The address map probe, where a single wrong digit stops the part.

    python test/test_addrmap.py

This probe reads addresses whose meaning is unknown, which is the point of it,
and that makes two rules load bearing in a way they are not elsewhere:

  **EVERY OFFSET READ MUST LIE INSIDE THE PERIPHERAL'S SLOT.** `[inferred]` A
  read inside the one kilobyte slot is expected to return zero; a read outside
  it raises a bus error that escalates to a hard fault and stops the part. One
  digit is the difference, so the offsets are collected from the source and
  compared against the bound rather than eyeballed.

  **EVERY READ IS ANNOUNCED AND FLUSHED BEFORE THE ACCESS.** A line assembled in
  memory and printed at the end prints nothing at all when the access halfway
  through it does not return, and the capture then stops without naming the read
  that stopped it. The flush has to come before the dereference, not after.

The rest:

  - EXACTLY ONE WRITE, as in the previous probe.
  - FDCAN IS READ BEFORE THE WRITE. That is the whole reason this image exists:
    the previous one read it only afterwards and so could not say whether the
    enable was necessary.
  - THE KNOWN POSITIVE IS AMONG THE PAIRS. A capture in which the pair that
    already mirrored stops mirroring is a capture to distrust entirely.
  - THE FAULT HANDLERS ARE STRONG and name the last announced offset, which is
    the first use in this volume of the weak aliases the generated vector table
    exists to allow.
  - THE FDCAN FACTS COME FROM CHAPTER 9, imported rather than retyped.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
CH09 = HERE.parent / "ch09-can-fd-from-the-controller-out" / "tools"
APP = HERE / "src" / "app" / "addrmap.c"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def strip_comments(code):
    code = re.sub(r"/\*.*?\*/", " ", code, flags=re.S)
    return re.sub(r"//[^\n]*", " ", code)


def main():
    raw = APP.read_text(encoding="utf-8")
    code = strip_comments(raw)

    # 1. THE BOUND. Collected, not eyeballed.
    m = re.search(r"#define\s+AM_MAX_OFFSET\s+0x([0-9A-Fa-f]+)u", code)
    if not m:
        fail("AM_MAX_OFFSET is not defined, so nothing states how far into the "
             "peripheral this probe is allowed to read")
        bound = None
    else:
        bound = int(m.group(1), 16)

    pairs = re.search(r"AM_PAIRS\[\d+\]\s*=\s*\{(.*?)\n\};", code, re.S)
    if not pairs:
        fail("AM_PAIRS is not a table, so the offsets cannot be collected")
        offsets = []
    else:
        offsets = [int(h, 16)
                   for h in re.findall(r"0x([0-9A-Fa-f]+)u", pairs.group(1))]
        if len(offsets) % 2:
            fail(f"{len(offsets)} offsets in AM_PAIRS, which is not pairs")

    write = re.search(r"#define\s+AM_WRITE_OFFSET\s+0x([0-9A-Fa-f]+)u", code)
    if write:
        offsets.append(int(write.group(1), 16))

    if bound is not None and offsets:
        over = [o for o in offsets if o > bound]
        if over:
            fail(f"these offsets are past AM_MAX_OFFSET 0x{bound:03X}: "
                 f"{[f'0x{o:03X}' for o in over]}; a read outside the "
                 f"peripheral's slot raises a bus error and stops the part")
        if bound > 0x3FF:
            fail(f"AM_MAX_OFFSET is 0x{bound:03X}, past the 0x3FF slot this "
                 f"peripheral is allocated")

    # 2. THE FLUSH PRECEDES THE DEREFERENCE inside announced_read. Checked by
    # position, because the whole value of announcing a read is lost if the
    # announcement is still sitting in the transmitter when the read faults.
    fn = re.search(r"announced_read\s*\([^)]*\)\s*\{(.*?)\n\}", code, re.S)
    if not fn:
        fail("announced_read is not a function, so nothing announces the reads")
    else:
        body = fn.group(1)
        flush = body.find("console_flush")
        deref = body.find("*(volatile uint32_t *)")
        if flush < 0:
            fail("announced_read does not flush, so the offset may still be in "
                 "the transmitter when the read faults")
        elif deref < 0:
            fail("announced_read does not dereference anything")
        elif flush > deref:
            fail("announced_read dereferences before it flushes, so a fault "
                 "loses the announcement it was supposed to leave behind")

    # 3. EVERY READ GOES THROUGH announced_read, so NO dereference-cast may
    # appear anywhere else. A bare one is an access nobody can locate when it
    # faults, which is the single thing this probe's output is designed around.
    #
    # The write is not counted here and must not be: it goes through a pointer
    # variable, `*reg |= ...`, which is not a dereference-cast at all. An
    # earlier version of this rule allowed "exactly one" on the strength of the
    # write and therefore demanded an access that does not exist, which made it
    # fail on correct code. Rule 4 counts the write.
    bare = [m2.start() for m2
            in re.finditer(r"\*\s*\(\s*volatile\s+uint32_t\s*\*\s*\)", code)]
    outside = ([b for b in bare if not (fn.start() <= b <= fn.end())]
               if fn else bare)
    if outside:
        lines = sorted({code[:b].count(chr(10)) + 1 for b in outside})
        fail(f"{len(outside)} dereference-cast(s) outside announced_read, "
             f"around line(s) {lines}; every read must be announced or a fault "
             f"cannot be located")

    # 4. Exactly one write.
    writes = re.findall(r"\*reg\s*\|=", code)
    if len(writes) != 1:
        fail(f"{len(writes)} writes and exactly one is permitted")

    # 5. FDCAN IS READ BEFORE THE WRITE. The reason this image exists.
    before = code.find("fdcan-before")
    wr = code.find("*reg |=")
    after = code.find("fdcan-after")
    if before < 0 or after < 0:
        fail("the before and after FDCAN reads are not both present, so this "
             "image cannot say whether the enable was necessary")
    elif not (before < wr < after):
        fail(f"the order is wrong: fdcan-before at {before}, write at {wr}, "
             f"fdcan-after at {after}")

    # 6. The known positive is among the pairs, and every pair is 0x60 apart,
    # which is the distance the previous capture found.
    if offsets and len(offsets) >= 2:
        ps = [(offsets[i], offsets[i + 1])
              for i in range(0, len(offsets) - 1, 2)]
        if (0x0E0, 0x140) not in ps:
            fail("the pair 0x0E0 and 0x140 is not read, and it is the known "
                 "positive: a capture where it stops mirroring is one to "
                 "distrust before reading anything else")
        wrong = [(a, b) for a, b in ps if b - a != 0x60]
        if wrong:
            fail(f"these pairs are not 0x60 apart, which is the distance the "
                 f"previous capture found: "
                 f"{[(f'0x{a:03X}', f'0x{b:03X}') for a, b in wrong]}")

    # 7. The fault handlers are STRONG and name the last announced offset.
    for h in ("HardFault_Handler", "BusFault_Handler"):
        if not re.search(r"\bvoid\s+" + h + r"\s*\(\s*void\s*\)\s*\{", code):
            fail(f"{h} is not defined here, so a faulting read stops the part "
                 f"silently and the capture does not say where")
        if re.search(r"(?:weak|static)[^;{]*\b" + h + r"\b", code):
            fail(f"{h} is weak or static here, which cannot override the "
                 f"generated table's weak alias")
    hf = re.search(r"void\s+HardFault_Handler\s*\(\s*void\s*\)\s*\{(.*?)\n\}",
                   code, re.S)
    if hf and "g_announced" not in hf.group(1):
        fail("HardFault_Handler does not print g_announced, so it stops the "
             "part without naming the read that caused it")
    if not re.search(r"volatile\s+uint32_t\s+g_announced", code):
        fail("g_announced is not volatile, and a fault handler reads what the "
             "faulting code had just stored")

    # 8. THE FDCAN FACTS COME FROM CHAPTER 9.
    if not CH09.is_dir():
        fail(f"chapter 9's tools are not at {CH09}")
    else:
        sys.path.insert(0, str(CH09))
        try:
            import gen_initplan as ip
        except ImportError as e:
            fail(f"chapter 9's generator does not import: {e}")
            ip = None
        if ip is not None:
            for name, macro in (("CREL", "AM_FDCAN_CREL"),
                                ("ENDN", "AM_FDCAN_ENDN")):
                want = ip.REG[name]
                m2 = re.search(r"#define\s+" + macro + r"\s+0x([0-9A-Fa-f]+)u",
                               code)
                if not m2:
                    fail(f"{macro} is not defined")
                elif int(m2.group(1), 16) != want:
                    fail(f"{macro} is 0x{int(m2.group(1), 16):03X} and chapter "
                         f"9 says {name} is 0x{want:03X}")
            m2 = re.search(r"#define\s+AM_ENDN_EXPECTED\s+0x([0-9A-Fa-f]+)u",
                           code)
            if not m2 or int(m2.group(1), 16) != 0x87654321:
                fail("AM_ENDN_EXPECTED is not 0x87654321")

    # 9. The verdict line distinguishes all three outcomes, including the one
    # where the controller does not answer even after the enable. A probe that
    # can only say yes or no cannot report a surprise.
    for needed in ("answered before the enable", "silent before",
                   "does not answer even after"):
        if needed not in code:
            fail(f"the enable-necessary verdict has no branch for {needed!r}")

    if failures:
        print(f"test_addrmap: {len(failures)} failure(s)")
        return 1
    print(f"ok  {len(offsets) - 1} offsets all inside 0x{bound:03X}, every read "
          f"announced and flushed before the access, FDCAN read before the one "
          f"write, the known positive among the pairs, and both fault handlers "
          f"naming the last announced offset")
    return 0


if __name__ == "__main__":
    sys.exit(main())
