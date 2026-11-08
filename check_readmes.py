#!/usr/bin/env python3
"""Every README still describes its own chapter, and the indexes list them all.

    python check_readmes.py

**Why this is a check and not a habit.** On Sunday 8 November 2026 an audit of
every README in this repository found four kinds of drift, all of them from
software that had been added without the prose that announces it:

  * `joint-node/`, the entire software tree, was absent from the front page's
    repository layout table
  * `joint-node/README.md`'s state table had three rows and omitted chapter 9
    entirely, while describing chapter 4 as "the layer rule only"
  * the same file said the message memory layout was "not there yet" weeks after
    it was there, and that seventeen chapters had nothing when it was fifteen
  * chapter 10's README described its measurement document as three runs on one
    day when it had grown to five across two weeks

Not one of those was a wrong decision. Each was a README that nobody updated
because nothing made them. A rule that depends on remembering is not a rule, so
this is the rule: a generator, a test or a chapter that exists and is not named
in the prose that indexes it fails the build.

What this cannot check is whether the prose is TRUE. It checks that every piece
of software is mentioned somewhere a reader would look, which is the drift that
actually happens; a sentence that describes a generator wrongly still needs a
person. The counts in a README are not checked either, for the same reason: a
check that parsed them would be asserting its own arithmetic.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
TREE = ROOT / "joint-node"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def chapter_dirs():
    if not TREE.is_dir():
        return []
    return sorted(d for d in TREE.iterdir()
                  if d.is_dir() and re.match(r"^ch\d\d-", d.name))


def main():
    chapters = chapter_dirs()
    if not chapters:
        print("no chapter directories under joint-node/, so nothing to check "
              "and this result means nothing")
        return 1

    front = ROOT / "README.md"
    index = TREE / "README.md"

    # 1. The front page must point at the software tree at all. It did not,
    # for as long as the tree has existed.
    if not front.exists():
        fail("there is no front page README")
    else:
        text = front.read_text(encoding="utf-8")
        if "joint-node/" not in text:
            fail("the front page README never mentions joint-node/, which is "
                 "the whole software tree")

    # 2. Every chapter directory is listed in the tree's own index, and the
    # index lists no chapter that does not exist.
    if not index.exists():
        fail("joint-node/ has no README to index its chapters")
        print(f"check_readmes: {len(failures)} failure(s)")
        return 1
    index_text = index.read_text(encoding="utf-8")

    for d in chapters:
        if d.name not in index_text:
            fail(f"{d.name} exists and joint-node/README.md does not list it")
    for named in sorted(set(re.findall(r"\bch\d\d-[a-z0-9-]+", index_text))):
        if not (TREE / named).is_dir():
            fail(f"joint-node/README.md lists {named} and there is no such "
                 f"directory")

    # 3. EVERY CHAPTER'S STATE IS IN THE STATE TABLE. The table is where a
    # reader goes to find out what exists, so a chapter missing from it is
    # invisible however good its own README is. Chapter 9 was missing from it
    # while carrying nine modules.
    rows = re.findall(r"^\|\s*(\d\d)\s*,", index_text, re.M)
    for d in chapters:
        number = d.name[2:4]
        if number not in rows:
            fail(f"chapter {number} has a directory and no row in "
                 f"joint-node/README.md's state table, so a reader cannot see "
                 f"that it exists")

    # 4. Each chapter has its own README, and every generator and test in it is
    # named there. This is the one that catches the drift as it happens: a new
    # tools/gen_*.py or test/test_*.py is software a reader cannot discover
    # unless the README says it is there.
    for d in chapters:
        own = d / "README.md"
        if not own.exists():
            fail(f"{d.name} has no README")
            continue
        own_text = own.read_text(encoding="utf-8")

        scripts = sorted(
            [p for p in (d / "tools").glob("*.py")] +
            [p for p in (d / "test").glob("*.py")] +
            [p for p in d.glob("*.py")])
        for s in scripts:
            if s.name not in own_text:
                fail(f"{d.name}/{s.relative_to(d).as_posix()} exists and "
                     f"{d.name}/README.md does not name it")

        # And the shipping C, which is what the chapter actually is. A module
        # added without a word in the README is a module nobody finds.
        for sub in ("src/bus", "src/bsp"):
            for c in sorted((d / sub).glob("*.c")):
                if c.name not in own_text:
                    fail(f"{d.name}/{sub}/{c.name} exists and "
                         f"{d.name}/README.md does not name it")

    if failures:
        print(f"check_readmes: {len(failures)} failure(s)")
        return 1

    scripts = sum(len(list((d / "tools").glob("*.py")))
                  + len(list((d / "test").glob("*.py")))
                  + len(list(d.glob("*.py"))) for d in chapters)
    sources = sum(len(list((d / "src" / "bus").glob("*.c")))
                  + len(list((d / "src" / "bsp").glob("*.c")))
                  for d in chapters)
    print(f"ok  {len(chapters)} chapters, each with a README and a row in the "
          f"state table, {scripts} generators and tests and {sources} shipping "
          f"C files all named in the prose that indexes them")
    return 0


if __name__ == "__main__":
    sys.exit(main())
