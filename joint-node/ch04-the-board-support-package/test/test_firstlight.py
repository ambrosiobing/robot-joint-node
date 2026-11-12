#!/usr/bin/env python3
"""The first printing image, and the build stamp that makes a capture nameable.

    python test/test_firstlight.py

There is no C compiler on the win11 aquamarine authoring laptop, so none of this
is built where it is written and every rule about the C here is about the text.
The stamp is different: it is Python, so it is driven directly, including the one
state it exists to detect.

WHAT THIS FILE IS ACTUALLY GUARDING. The image will be flashed to a board that
two volumes share, which means the output of a capture has to identify the image
that produced it or the capture proves nothing. Elsewhere on this bench a
skipped flash step printed a banner identical to the expected one character for
character, twice in one evening, because the banner carried only a name. So:

  - A TAG NO OTHER PROJECT PRINTS, so the first line of a capture is the check.
  - A BUILD STAMP, because a name cannot separate two builds of one source.
  - THE DIRTY MARKER COUNTS UNTRACKED FILES, which is the half
    `git describe --dirty` reports as clean, and is the usual state of a new
    chapter. This is the guard whose failing state is constructed below.
  - THE ASSUMED CLOCK PRINTED BEFORE ANYTHING DEPENDS ON IT, because the console
    cannot measure its own baud and readable text is the only evidence there is.
  - A MONOTONIC COUNTER, so a foreign reflash shows up as the banner reappearing
    and the count restarting.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE / "tools"))
import gen_stamp as st                                          # noqa: E402

APP = HERE / "src" / "app" / "first_light.c"
REPO = HERE.parent.parent

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

    # ---------------------------------------------------------- the stamp
    # 1. THE GUARD'S FAILING STATE, BUILT ON PURPOSE. An untracked file and
    # nothing else is a dirty tree, and it is the case the git option this
    # replaces gets wrong. If this ever returns None the marker is worthless
    # precisely when a chapter is new.
    only_untracked = "?? joint-node/ch04-the-board-support-package/src/app/x.c"
    got = st.dirty_from_status(only_untracked)
    if got is None:
        fail("a tree whose only change is an untracked file is reported clean, "
             "which is the defect this marker exists to avoid")
    elif "untracked" not in got:
        fail(f"an untracked-only tree is reported as {got!r}, which does not "
             f"say what is actually different")

    # And the other three states, so the reason is not simply always the same
    # string. A marker that says 'dirty' for a clean tree is as useless as one
    # that says 'clean' for a dirty one.
    if st.dirty_from_status("") is not None:
        fail("a clean tree is reported as dirty")
    if st.dirty_from_status(None) != "no-git":
        fail("a tree with no git at all is not distinguished from a clean one")
    both = st.dirty_from_status(" M a/b.c\n?? c/d.c")
    if not (both and "modified" in both and "untracked" in both):
        fail(f"a tree with both kinds of change reports {both!r} and should "
             f"name both")

    # 2. The line carries the fields a reader is told to look for, in a form
    # that is attributable rather than a row of bare values.
    line = st.stamp_line()
    for needed in ("built ", " from ", " tree "):
        if needed not in line:
            fail(f"the stamp line {line!r} has no {needed!r}")
    if not re.search(r"built \w+ \d{1,2} \w+ \d{4}, \d\d:\d\d:\d\d", line):
        fail(f"the stamp line does not carry a full date: {line!r}")

    # ------------------------------------------------------- the C image
    # 3. The tag is unique in this repository. A tag another project prints
    # cannot tell a reader which image is running, which is the entire job of
    # the first line.
    tag = re.search(r'#define\s+FL_TAG\s+"([^"]+)"', code)
    if not tag:
        fail("no FL_TAG is defined, so the capture has nothing to identify it")
    else:
        t = tag.group(1)
        hits = []
        for p in REPO.rglob("*"):
            if not p.is_file() or ".git" in p.parts or "build" in p.parts:
                continue
            if p.suffix not in (".c", ".h", ".py", ".md", ".tex", ".ld"):
                continue
            try:
                if t in p.read_text(encoding="utf-8", errors="ignore"):
                    hits.append(p.relative_to(REPO).as_posix())
            except OSError:
                continue
        # The image, this test, the chapter README and the document may all
        # name it. What must not happen is another project's source doing so.
        other = [h for h in hits
                 if not h.startswith("joint-node/ch04-the-board-support-package")
                 and not h.startswith("sections/")
                 and not h.startswith("chapters/")]
        if other:
            fail(f"the tag {t!r} also appears outside chapter 4, so the first "
                 f"line of a capture does not identify this image: {other}")

    # 4. THE ORDER. The report names the assumption every later line depends
    # on, so it must come before the heartbeat. Checked by position, because a
    # banner printed after the first tick is a banner a reader scrolls past.
    rep = code.find("console_report")
    tick = code.find("tick")
    if rep < 0:
        fail("console_report is never called, so the image prints numbers "
             "whose clock nobody can see")
    elif tick >= 0 and rep > tick:
        fail("console_report is called after the heartbeat starts, so the "
             "assumed clock arrives after the lines that depend on it")

    # 5. The stamp is actually printed, not merely linked. With
    # --gc-sections a stamp nothing calls is discarded and the image would
    # link, print its name, and be unattributable.
    if "build_stamp" not in code:
        fail("build_stamp is never called, so the banner cannot separate two "
             "builds of the same source")

    # 6. The assumed clock is a named constant. A literal inside the call is
    # the same number and is not findable by somebody asking what this image
    # assumed.
    if not re.search(r"#define\s+FL_ASSUMED_KERNEL_HZ", code):
        fail("the assumed kernel clock is not a named constant, so the one "
             "unmeasured figure in this image is not findable by name")
    if re.search(r"console_init\s*\([^;]*?\b64000000u\b", code, re.S):
        fail("console_init is passed the clock as a literal rather than "
             "through FL_ASSUMED_KERNEL_HZ")

    # 7. The gap is not described as a time. Nothing here has configured a
    # timer, so a figure in milliseconds would be a measurement this image
    # cannot make, and the volume's rule is that an unmeasured number does not
    # get a unit.
    for bad in re.findall(r"#define\s+FL_GAP\w*[^\n]*", code):
        if re.search(r"\b(ms|millisecond|microsecond|us|second|hz)\b", bad,
                     re.I):
            fail(f"the gap is given a time unit and nothing here measures "
                 f"time: {bad.strip()}")

    # 8. The counter is incremented before it is printed, so the first line
    # says tick 1 and a reader counting steps of one starts from a known
    # value rather than from zero on one image and one on the next.
    body = code[code.find("int main(void)\n{"):]
    inc = body.find("tick++")
    first_print = body.find("console_dec")
    if inc < 0:
        fail("the heartbeat has no counter, so a foreign reflash cannot be "
             "detected from the capture alone")
    elif first_print >= 0 and inc > first_print:
        fail("the counter is printed before it is incremented, so the first "
             "line reports tick 0")

    # 9. Flushed, because the useful capture is from a part that is then reset
    # or unplugged and an unflushed transmitter loses what was in flight.
    if "console_flush" not in code:
        fail("nothing is flushed, so the last line of any capture may be the "
             "one that never left the shift register")

    # 10. And it must not return. There is nothing to return to: the startup
    # calls main and loops, but an image whose main falls through has told the
    # reader nothing about why it stopped printing.
    if not re.search(r"for\s*\(\s*;\s*;\s*\)", body):
        fail("main can fall through, and a capture that simply stops is "
             "indistinguishable from a part that crashed")

    if failures:
        print(f"test_firstlight: {len(failures)} failure(s)")
        return 1
    print(f"ok  the tag is unique to chapter 4, the report precedes the "
          f"heartbeat, the stamp is printed, the dirty marker catches an "
          f"untracked-only tree, and no unmeasured number carries a unit")
    return 0


if __name__ == "__main__":
    sys.exit(main())
