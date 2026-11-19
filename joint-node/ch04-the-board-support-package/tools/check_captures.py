#!/usr/bin/env python3
"""Every quoted console line must exist in a capture on file.

    python tools/check_captures.py

WHY THIS EXISTS, and it is the only gate in this chapter written because of a
specific failure rather than a specific hazard.

On Thursday 12 November 2026 three pages in this repository said that reading
`CREL` and `ENDN` needs the peripheral bus clock and nothing else. A capture
taken the same evening refuted it: the enable bit read zero and the registers
answered anyway. **The tree carried the refuted sentence for seven days with
every gate green.**

No checker could have caught it, and not through weakness. `check_readmes.py`
compares a tree with itself. `lint.py` reads house style. The date sweep reads
dates. **A bench capture was not an input to any of them**, so the claim lived
in the tree and its refutation lived in a terminal transcript, and nothing
connected those two places except somebody remembering.

This connects them. It cannot tell whether a sentence of prose is true. What it
can do is refuse a page that QUOTES a console line which no capture contains,
which is the specific move that went wrong: I quoted from a transcript nobody
kept, and a quotation is not raw until it has been diffed against its source.

WHAT IT CHECKS

  1. Every line of prose that looks like console output appears VERBATIM in some
     capture file. Compared on the stripped line, so indentation inside a
     Markdown block is free and internal spacing is not.
  2. Each capture carries its header fields and the marker that separates the
     commentary from the verbatim text.
  3. Each capture's `Commit:` is a real commit in this repository. A capture
     naming a build that never existed is worse than no capture.

     THIS RULE IS HISTORY DEPENDENT, which cost a red run on Thursday
     19 November 2026: `actions/checkout@v4` fetches depth 1 by default, so in
     CI only the tip commit exists and all four captures looked wrong. The
     workflow now sets `fetch-depth: 0`, and this script refuses outright on a
     shallow clone rather than reporting four false diagnoses.
  4. Each capture's tag is defined by some `src/app/*.c`, and appears in the
     capture's own first line, so a capture is traceable to the image.
  5. A truncation line states how many lines it stands for, so a shortened
     capture cannot quietly drop content.
  6. And it REPORTS, rather than refuses, which images have a capture on file
     and which have never produced one. That is this volume's honesty rule for
     images made mechanical: an image with no capture has never run.
  7. And which captures PREDATE A CHANGE TO THEIR OWN IMAGE, because "has a
     capture" and "has a capture of the image on the bench today" are different
     facts and rule 6 alone cannot tell them apart. Reported rather than
     refused: a capture that prompted the change to its own image is the
     normal and desirable case, and two in this chapter are exactly that.

     The staleness test asks git whether the source file differs between the
     capture's commit and the working tree. It is guarded by a cat-file check
     first, because `git diff --quiet` over a path that does not exist at that
     commit answers NO CHANGE, which is indistinguishable from nothing having
     happened. That guard fired immediately on the first attempt: `commit:path`
     is root relative while `diff -- path` is relative to the working
     directory, so the two forms named different files until both calls were
     pinned to the repository root.
"""
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
CAPTURES = HERE / "doc" / "captures"
APPS = HERE / "src" / "app"

# The repository root, and this chapter's path inside it. Both git calls that
# name a file need the SAME root-relative path, and `commit:path` only accepts
# that form, so the directory is pinned rather than inherited.
TOP = Path(subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=HERE,
                          capture_output=True, text=True,
                          check=True).stdout.strip())
PREFIX = subprocess.run(["git", "rev-parse", "--show-prefix"], cwd=HERE,
                        capture_output=True, text=True,
                        check=True).stdout.strip()

# Prose this chapter owns. The published Markdown edition is generated from
# sections/, so it is not read here: a quotation that is wrong there is wrong in
# the source and will be caught at the source.
PROSE = [HERE / "README.md"] + sorted((HERE / "doc").glob("*.md"))

REQUIRED_HEADERS = ("# Capture:", "# Taken on", "# Image:", "# Commit:",
                    "# Asked:")
MARKER = "# ---- capture ----"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def tags_from_sources():
    """tag -> source file, from every `#define *_TAG "..."` under src/app."""
    out = {}
    for c in sorted(APPS.glob("*.c")):
        for m in re.finditer(r'#define\s+\w*_TAG\s+"([^"]+)"',
                             c.read_text(encoding="utf-8")):
            out[m.group(1)] = c.name
    return out


def capture_lines(path):
    """The verbatim half of one capture, as stripped lines."""
    text = path.read_text(encoding="utf-8")
    if MARKER not in text:
        return None
    body = text.split(MARKER, 1)[1]
    return [ln.strip() for ln in body.splitlines() if ln.strip()]


def looks_like_console(stripped, tags):
    """Is this line of prose a quoted console line?

    A line QUOTES the console when it begins with an image tag, or with `# `
    and then a tag, or with a field prefix the console itself emits. An inline
    mention in the middle of a sentence is not a quotation and is not checked,
    which is why the test is on the START of the line.
    """
    s = stripped
    if s.startswith("# "):
        s = s[2:].strip()
    if s.startswith("console: "):
        return True
    return any(s.startswith(t + " ") or s == t for t in tags)


def main():
    if not CAPTURES.is_dir():
        fail(f"{CAPTURES.relative_to(HERE)} does not exist, so no page can "
             f"quote a capture and be checked")
        print(f"check_captures: {len(failures)} failure(s)")
        return 1

    # A SHALLOW CLONE CANNOT ANSWER THE COMMIT QUESTION, and must say so
    # rather than answer it wrongly. `actions/checkout@v4` fetches depth 1 by
    # default, so only the tip commit exists and every capture naming an older
    # one would be reported as "this repository has no such commit". That
    # sentence would be false: the repository has it, the clone does not.
    #
    # Refusing rather than skipping is deliberate. A checker that quietly drops
    # a rule in CI is a checker that passes for the wrong reason, and this
    # chapter has already had one of those.
    shallow = subprocess.run(["git", "rev-parse", "--is-shallow-repository"],
                             cwd=HERE, capture_output=True, text=True)
    if shallow.stdout.strip() == "true":
        fail("this is a SHALLOW clone, so the commit named by each capture "
             "cannot be checked. That is a defect in the workflow and not in "
             "the captures: add 'fetch-depth: 0' to the checkout step of the "
             "job that runs this script.")
        print(f"check_captures: {len(failures)} failure(s)")
        return 1

    tags = tags_from_sources()
    if not tags:
        fail("no image tag is defined under src/app, so no capture can be "
             "traced to the image that produced it")

    files = sorted(CAPTURES.glob("*.txt"))
    if not files:
        fail("no capture files at all")

    # ---- the captures themselves -------------------------------------- #
    verbatim = {}
    stale = []          # stripped line -> set of capture names
    have_capture = set()   # tags with a capture on file
    for f in files:
        text = f.read_text(encoding="utf-8")
        for h in REQUIRED_HEADERS:
            if h not in text:
                fail(f"{f.name} has no {h!r} header, so it does not say what "
                     f"it is a capture OF")
        lines = capture_lines(f)
        if lines is None:
            fail(f"{f.name} has no {MARKER!r}, so nothing separates its "
                 f"commentary from the verbatim console text")
            continue

        m = re.search(r"# Capture:\s*(\S+)", text)
        tag = m.group(1) if m else None
        if tag is None:
            fail(f"{f.name} names no tag")
        elif tag not in tags:
            fail(f"{f.name} names the tag {tag!r} and no source under src/app "
                 f"defines it; a capture that cannot be traced to an image is "
                 f"not evidence about one")
        else:
            have_capture.add(tag)
            if not any(tag in ln for ln in lines[:2]):
                fail(f"{f.name} is a capture of {tag!r} and neither of its "
                     f"first two verbatim lines contains that tag, so the "
                     f"capture does not identify itself")

        m = re.search(r"# Commit:\s*([0-9a-f]{7,40})", text)
        if not m:
            fail(f"{f.name} names no commit, so the capture cannot be tied to "
                 f"a build")
        else:
            r = subprocess.run(["git", "cat-file", "-e", m.group(1) + "^{commit}"],
                               cwd=HERE, capture_output=True)
            if r.returncode != 0:
                fail(f"{f.name} names commit {m.group(1)} and this CLONE does "
                     f"not contain it. In a full clone that means the capture "
                     f"names a build that never existed.")
            elif tag in tags:
                # ---- AND HAS THE IMAGE CHANGED SINCE? ------------------- #
                #
                # A capture is evidence about the image that produced it, and
                # nothing more. If its own source has changed since the commit
                # the capture names, then the image on the bench today is not
                # the image that printed these lines, and reporting the tag as
                # "has run" would answer a question nobody asked.
                #
                # Two captures in this chapter are legitimately in that state:
                # jn-firstlight and jn-stackdepth were both corrected BECAUSE
                # of what their capture showed. That is the normal case and not
                # a failure, so this is reported rather than refused. What
                # would be a failure is a page claiming the current image was
                # measured, and the quotation rule above already catches that:
                # a changed console line is a quotation no capture contains.
                # ROOT-RELATIVE, and both git calls below run from the repo
                # root. `commit:path` is always root relative while
                # `diff -- path` is relative to the working directory, so the
                # two forms disagree unless the directory is pinned. They did
                # disagree on the first attempt, and the cat-file guard below
                # is what said so.
                src = PREFIX + "src/app/" + tags[tag]

                # THE PATH IS CHECKED BEFORE IT IS TRUSTED. `git diff --quiet`
                # over a path that does not exist at that commit reports NO
                # CHANGE, which is the same answer as "nothing happened". So
                # ask first whether git knows the file there; a silent zero
                # from a wrong path is the failure mode this whole file exists
                # to prevent.
                known = subprocess.run(
                    ["git", "cat-file", "-e", f"{m.group(1)}:{src}"],
                    cwd=TOP, capture_output=True)
                if known.returncode != 0:
                    fail(f"{f.name} names commit {m.group(1)[:7]}, and {src} "
                         f"does not exist in that commit, so no staleness "
                         f"check of this capture is possible")
                else:
                    d = subprocess.run(["git", "diff", "--quiet", m.group(1),
                                        "--", src], cwd=TOP,
                                       capture_output=True)
                    if d.returncode == 1:
                        stale.append((tag, f.name, m.group(1)[:7], src))
                    elif d.returncode != 0:
                        fail(f"git diff failed for {src} at {m.group(1)[:7]}, "
                             f"so the capture's staleness is unknown")

        for ln in lines:
            if ln.startswith("# ----"):
                if not re.match(r"#\s*----\s*\d+\s+further\b", ln):
                    fail(f"{f.name}: the truncation line {ln!r} does not say "
                         f"how many lines it stands for")
                continue
            verbatim.setdefault(ln, set()).add(f.name)

    # ---- THE RULE: every quoted console line is in a capture ----------- #
    quoted = 0
    for page in PROSE:
        if not page.exists():
            continue
        for n, raw in enumerate(page.read_text(encoding="utf-8").splitlines(), 1):
            s = raw.strip()
            if not s or not looks_like_console(s, tags):
                continue
            quoted += 1
            if s not in verbatim:
                fail(f"{page.name}:{n} quotes a console line that no capture "
                     f"contains:\n           {s[:120]}")

    # ---- and the honest report, which refuses nothing ------------------ #
    never = sorted(t for t in tags if t not in have_capture)

    if failures:
        print(f"check_captures: {len(failures)} failure(s)")
        return 1

    print(f"ok  {len(files)} captures, {len(verbatim)} verbatim lines, "
          f"{quoted} quotations in the prose all found, "
          f"{len(have_capture)} of {len(tags)} images have run")
    if never:
        print(f"    never run, no capture on file: {', '.join(never)}")
    if stale:
        print(f"    {len(stale)} capture(s) predate a change to their own "
              f"image, so they are evidence about an earlier build:")
        for tag, name, sha, src in sorted(stale):
            print(f"      {tag:18s} {name}  built at {sha}, {src} changed since")
        print("    That is the normal case when a capture is what prompted the "
              "change. A recapture is what closes it.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
