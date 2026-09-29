#!/usr/bin/env python3
"""House-style linter for the section sources.

    python lint.py                 check every sections/*.tex
    python lint.py sections/p07.tex

Checks prose (everything outside verbatim code environments) for: em and en
dashes and LaTeX -- / ---, non-ASCII characters, violent idioms, and the
required subsection skeleton.  Checks code blocks for non-ASCII and for lines
longer than the page can print.

The tooling-attribution scan is deliberately NOT here.  A rule that spells the
words it forbids would put those words into the repository it protects, which
is the opposite of what it is for, so that scan lives outside this repository
and is run by hand before anything is published.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DOC = {
    "slug":       "robot-joint-node",
    "stem_pfx":   "j",            # sections/jNN.tex are the chapter files
    "code_cols":  96,
    "ascii_cols": 112,
    "required": [
        "Why this chapter", "Prior art and what to reuse", "What the node gains",
        "Parts from the inventory", "System architecture", "Peripheral configuration",
        "Wiring", "Memory and timing budget", "Firmware design", "Data flow",
        "Repository layout", "Steps", "Build, flash and debug",
        "Verification and acceptance criteria", "Variants", "Pitfalls",
        "Best practices applied", "Stretch goals", "Roadmap and next steps",
        "Portfolio evidence", "Sources",
    ],
    "figures": ("arch", "wiring", "uml", "data", "timing"),
}

# This volume was written with a particular employer's advert open, and must not
# name it. A reader should be able to use the book for any robotics firmware
# application, and a named company dates the work to one vacancy.
FORBIDDEN_NAMES = re.compile(
    r"(?i)\b(neura|maira|mai-?ra|riederich|lara\s+robot|4ne1)\b")


if ROOT.name != DOC["slug"]:
    raise SystemExit(
        f"lint.py: DOC['slug'] is {DOC['slug']!r} but this file lives in "
        f"{ROOT.name!r}. Update the DOC block after copying the toolchain.")

VERB = re.compile(r"\\begin\{(asciiart|ccode|cppcode|rustcode|shellcode|pycode|makecode"
                  r"|dtscode|yamlcode|asmcode|ldcode|plaincode|lstlisting|verbatim)\}"
                  r"(.*?)\\end\{\1\}", re.S)
DASH = re.compile(r"(.{0,40})(\u2014|\u2013|(?<![-\w])---?(?![-\w>]))(.{0,40})")
VIOLENT = re.compile(r"\b(kill(?:s|ed|ing)?|attack(?:s|ed|ing)?|fight(?:s|ing)?|blame[ds]?"
                     r"|hurt(?:s|ing)?|destroy(?:s|ed|ing)?|suicid\w*|war against)\b", re.I)
MAXLEN = {"asciiart": DOC["ascii_cols"]}
MAXCODE = DOC["code_cols"]


def check(path):
    s = path.read_text(encoding="utf-8")
    problems = []
    codes = [(m.group(1), m.group(2)) for m in VERB.finditer(s)]
    prose = VERB.sub("\n", s)

    for m in DASH.finditer(prose):
        problems.append(f"dash: ...{m.group(1)}[{m.group(2)}]{m.group(3)}...".replace("\n", " "))
    bad = sorted({c for c in prose if ord(c) > 126})
    if bad:
        problems.append(f"non-ASCII in prose: {bad}")
    for m in VIOLENT.finditer(prose):
        problems.append(f"violent idiom: {m.group(0)!r} near {prose[max(0,m.start()-40):m.end()+40]!r}")
    for m in FORBIDDEN_NAMES.finditer(prose):
        problems.append(f"names an employer or product: {m.group(0)!r}; this "
                        f"volume stays generic so it serves any application")

    for env, code in codes:
        bad = sorted({c for c in code if ord(c) > 126})
        if bad:
            problems.append(f"non-ASCII in {env} block: {bad}")
        limit = MAXLEN.get(env, MAXCODE)
        for ln in code.splitlines():
            if len(ln) > limit:
                problems.append(f"{env} line {len(ln)} chars (max {limit}): {ln[:50]}...")

    pfx = DOC["stem_pfx"]
    if path.stem.startswith(pfx) and path.stem[len(pfx):].isdigit():
        for h in DOC["required"]:
            if h not in s:
                problems.append(f"missing subsection: {h}")
        for fig in DOC["figures"]:
            if f"{{{path.stem}_{fig}}}" not in s:
                problems.append(f"missing figure: {path.stem}_{fig}")
    return problems


def main(argv):
    files = [Path(a) if Path(a).is_absolute() else ROOT / a for a in argv] or \
            sorted((ROOT / "sections").glob("*.tex"))
    total = 0
    for f in files:
        pr = check(f)
        total += len(pr)
        if pr:
            print(f"== {f.name}: {len(pr)} problems")
            for p in pr[:25]:
                print("   " + p)
            if len(pr) > 25:
                print(f"   ... and {len(pr)-25} more")
        else:
            print(f"== {f.name}: clean")
    print("TOTAL PROBLEMS:", total)
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
