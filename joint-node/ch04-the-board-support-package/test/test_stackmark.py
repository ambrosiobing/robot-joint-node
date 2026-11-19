#!/usr/bin/env python3
"""The painted stack, where every direction is the opposite of the intuitive one.

    python test/test_stackmark.py

The stack grows DOWN. Lower address means deeper, the high-water mark is a
MINIMUM address, free space is BELOW it and used space is ABOVE it. Nearly every
rule here exists because the opposite reading also produces a plausible number
of bytes.

The three that carry the most:

  **THE PAINT COMES AFTER THE .bss ZEROING.** stackmark keeps the ceiling it
  reached in a .bss variable, so painting first has the zeroing wipe it and
  every later read reports an unpainted region. That is a loud failure rather
  than a wrong number, which is why the variable lives in .bss in the first
  place, and the order is still part of the design.

  **THE SCAN RUNS UPWARD FROM THE BOTTOM.** Scanning down from the top would
  find the first word the stack has not yet reached on its CURRENT excursion,
  which is a reading of where the stack is now and not of where it has been.
  Both produce a number in the right range.

  **THE RUN AND THE TOTAL ARE BOTH COUNTED.** A touched word that happens to
  hold the pattern looks untouched, which pushes the boundary up and reports
  MORE free space than there is. That is the optimistic direction, so the
  coincidence has to be detectable: if every pattern word is inside the
  contiguous run, none occurred.

And the one about the application rather than the library:

  **A READING IS NOT AN EXPERIMENT.** The image reads the mark twice with a
  known consumption between and states the prediction before the result. An
  image printing one number produces a figure nothing can contradict.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
LIB = HERE / "src" / "bsp" / "stackmark.c"
HDR = HERE / "src" / "bsp" / "stackmark.h"
STARTUP = HERE / "src" / "bsp" / "startup.c"
APP = HERE / "src" / "app" / "stackdepth.c"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def strip_comments(code):
    code = re.sub(r"/\*.*?\*/", " ", code, flags=re.S)
    return re.sub(r"//[^\n]*", " ", code)


def main():
    lib = strip_comments(LIB.read_text(encoding="utf-8"))
    hdr = HDR.read_text(encoding="utf-8")
    start = strip_comments(STARTUP.read_text(encoding="utf-8"))
    app = strip_comments(APP.read_text(encoding="utf-8"))

    # 1. THE ORDER IN Reset_Handler: zero, then paint, then main.
    reset = start[start.find("void Reset_Handler(void)\n{"):]
    if not reset:
        fail("Reset_Handler was not found, so the order rules are vacuous")
    else:
        zero = reset.find("_ebss")
        paint = reset.find("stackmark_paint")
        callmain = reset.find("main(")
        if paint < 0:
            fail("Reset_Handler never calls stackmark_paint, so nothing paints "
                 "the stack and no image can report a mark")
        elif zero < 0:
            fail("the .bss zeroing was not found in Reset_Handler")
        elif not (zero < paint):
            fail(f"stackmark_paint is called BEFORE the .bss zeroing (paint at "
                 f"{paint}, zeroing at {zero}); the zeroing would wipe the "
                 f"ceiling and every read would report an unpainted region")
        elif not (paint < callmain):
            fail(f"stackmark_paint is called after main (paint at {paint}, "
                 f"main at {callmain}); everything the stack used in main "
                 f"would be invisible, which is the whole measurement")

    # 2. The ceiling is in .bss, so that zero means never painted. A static
    # with no initialiser, or one initialised to zero, both land there; an
    # initialiser that is not zero would put it in .data AND make the
    # never-painted case indistinguishable from a painted one.
    m = re.search(r"static\s+volatile\s+uintptr_t\s+(\w+)\s*(=\s*([^;]+))?;",
                  lib)
    if not m:
        fail("no static volatile uintptr_t holds the ceiling")
    else:
        if m.group(3) is not None and m.group(3).strip() not in ("0", "0u"):
            fail(f"the ceiling is initialised to {m.group(3).strip()!r}, which "
                 f"puts it in .data and makes 'never painted' look painted")
        if not re.search(r"\bvolatile\b", m.group(0)):
            fail("the ceiling is not volatile, and it is written in one "
                 "translation unit and read from another")

    # 3. THE SCAN RUNS UPWARD FROM THE BOTTOM. Checked on the loop, because
    # both directions produce a plausible byte count.
    # Found by its CONTENT and then read backwards to the `while`, because a
    # pattern for the condition itself has to cross the parenthesis of a cast:
    # `while ((uintptr_t) p < ceiling && ...)`. An earlier version used
    # `[^)]*` and could not match its own code.
    at = lib.find("run++")
    if at < 0:
        fail("nothing increments a run counter, so the contiguous run from the "
             "bottom is not being measured")
    else:
        w = lib.rfind("while", 0, at)
        cond = lib[w:at] if w >= 0 else ""
        if not cond:
            fail("the run counter is not inside a while loop")
        else:
            if "ceiling" not in cond:
                fail("the run scan is not bounded by the ceiling, so it would "
                     "read past the painted region into the live frame")
            if "STACKMARK_PATTERN" not in cond:
                fail("the run scan does not compare against the pattern")
            if "p++" not in lib[at:at + 120]:
                fail("the run scan does not advance upward; a downward scan "
                     "reads where the stack is NOW, not where it has been")
            if re.search(r"\bp--", lib[at:at + 120]):
                fail("the run scan advances DOWNWARD from the bottom, which "
                     "walks out of the region")
    # THE RUN SCAN'S OWN STARTING POINT, bound to the run scan and not to any
    # scan in the file. An earlier version searched the whole of stackmark.c
    # for a pointer initialised to `bottom`, and a mutation that started the
    # RUN scan at the ceiling stayed green because the TOTAL loop also starts
    # at `bottom` and satisfied the pattern. A rule that searches everywhere
    # cannot attribute what it finds.
    if at >= 0:
        decl = re.findall(r"\*\s*p\s*=\s*\(const volatile uint32_t \*\)\s*(\w+)",
                          lib[:at])
        if not decl:
            fail("the run scan's pointer is never initialised before the loop")
        elif decl[-1] != "bottom":
            fail(f"the run scan starts at {decl[-1]!r} and must start at the "
                 f"bottom of the region; starting at the ceiling measures "
                 f"where the stack is NOW, not where it has been")

    # 4. Both counted, and trustworthy compares them.
    for needed, why in (("intact_run", "the contiguous run from the bottom"),
                        ("intact_total", "the total count of pattern words"),
                        ("trustworthy", "the comparison between the two")):
        if needed not in lib:
            fail(f"{needed} is not computed, and it is {why}")
    if not re.search(r"trustworthy\s*=\s*\(?\s*run\s*==\s*total", lib):
        fail("trustworthy is not 'run == total', so a coincidental pattern "
             "word above the boundary would not be detectable")

    # 5. The pattern is not a plausible address on this part, and its halves
    # differ and are non-zero. An address-shaped pattern is the likeliest value
    # to appear on a stack by accident, which is the one coincidence that
    # matters.
    m = re.search(r"#define\s+STACKMARK_PATTERN\s+0x([0-9A-Fa-f]{8})u", hdr)
    if not m:
        fail("STACKMARK_PATTERN is not an eight digit hex constant")
    else:
        v = int(m.group(1), 16)
        hi, lo = v >> 16, v & 0xFFFF
        if hi == 0 or lo == 0:
            fail(f"half of STACKMARK_PATTERN is zero: 0x{v:08X}")
        if hi == lo:
            fail(f"the halves of STACKMARK_PATTERN are equal: 0x{v:08X}")
        for base, name in ((0x20000000, "DTCM"), (0x24000000, "AXI SRAM"),
                           (0x08000000, "flash"), (0x40000000, "APB"),
                           (0x58000000, "AHB4"), (0x00000000, "ITCM")):
            if (v & 0xFF000000) == base:
                fail(f"STACKMARK_PATTERN 0x{v:08X} looks like an address in "
                     f"{name}, which is the likeliest value to appear on a "
                     f"stack by accident")

    # 6. The margin is subtracted, so the ceiling is BELOW the frame.
    if not re.search(r"frame\s*-\s*STACKMARK_MARGIN", lib):
        fail("the ceiling is not the frame minus the margin; a ceiling at or "
             "above the frame paints over the live frame")
    if re.search(r"frame\s*\+\s*STACKMARK_MARGIN", lib):
        fail("the margin is ADDED to the frame, which paints over the frame "
             "that is in use")

    # 7. Every refusal in the paint returns 0, because 0 is what 'never
    # painted' means and a refusal that returned a ceiling would be read as a
    # successful paint.
    paint_fn = re.search(r"uintptr_t stackmark_paint\(void\)\s*\{(.*?)\n\}",
                         lib, re.S)
    if not paint_fn:
        fail("stackmark_paint was not found")
    else:
        body = paint_fn.group(1)
        guards = len(re.findall(r"\bif\s*\(", body))
        zeros = len(re.findall(r"return\s+0u\s*;", body))
        if guards < 4:
            fail(f"{guards} guards in the paint and at least four are needed: "
                 f"an inverted region, a misaligned bound, a frame outside the "
                 f"region, and no room below the margin")
        if zeros != guards:
            fail(f"{guards} guards but {zeros} 'return 0u', so a refusal may "
                 f"report a ceiling and be read as a successful paint")

    # 8. THE APPLICATION STATES A PREDICTION AND READS TWICE.
    if app.count("stackmark_read") < 2:
        fail("the image reads the mark fewer than twice, so the prediction "
             "cannot be tested and the figure is unfalsifiable")
    if not re.search(r"#define\s+SD_CONSUME_BYTES\s+\d+u", app):
        fail("the consumed size is not a named constant, so the prediction is "
             "not findable by somebody asking what this image claimed")
    if "predict" not in app:
        fail("the image never states its prediction before the result")
    for branch in ("METHOD BROKEN", "METHOD SUSPECT"):
        if branch not in app:
            fail(f"the verdict has no {branch!r} branch; a method that does "
                 f"not move the mark must say so rather than report a number")
    if not re.search(r"volatile\s+uint32_t\s+deep\s*\[", app):
        fail("the consumed array is not volatile, so -O1 may remove it and "
             "the prediction would test an array that was never allocated")
    # And the expected sum is computed, not typed.
    if re.search(r"expected\"?\s*,\s*32640", app):
        fail("the expected consume-sum is a literal; compute it from the word "
             "count so a change to the size cannot leave it stale")

    if failures:
        print(f"test_stackmark: {len(failures)} failure(s)")
        return 1
    print("ok  the paint runs after the zeroing and before main, the ceiling "
          "is in .bss, the scan runs upward from the bottom, run and total are "
          "both counted, the pattern is not address shaped, every refusal "
          "returns zero, and the image states a prediction it reads twice to "
          "test")
    return 0


if __name__ == "__main__":
    sys.exit(main())
