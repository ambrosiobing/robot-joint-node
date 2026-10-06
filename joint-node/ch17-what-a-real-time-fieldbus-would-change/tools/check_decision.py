#!/usr/bin/env python3
"""The decision document has to keep its shape and its provenance.

    python tools/check_decision.py

Chapter 17 builds nothing. Its deliverable is one document, so the only thing
that can be checked mechanically is whether the document still says what a
decision document has to say. Two rules, and both have cost something somewhere
before:

  1. The four headings the chapter specifies are all present. A decision that
     has quietly lost "what would change it" is an excuse rather than a
     decision.

  2. Every row of every claim table carries a provenance marker. The chapter's
     key facts say nothing here is built and everything is read, quoted, priced
     or decided, and that the chapter says which of its claims come from
     documents it opened and which it could not confirm. A row without a marker
     is a claim whose source has gone missing, and it is the kind of thing that
     survives three edits and then gets quoted at somebody.

This checks shape, not truth. No program can tell whether a subscription is
still annual; a person has to reopen the document and look, and the licence
section says as much.
"""
import re
import sys
from pathlib import Path

DOC = Path(__file__).resolve().parent.parent / "doc" / "fieldbus-decision.md"

REQUIRED_HEADINGS = [
    "## What it would buy",
    "## What it would cost",
    "## Why not here",
    "## What would change it",
]

MARKERS = ("[quoted]", "[read]", "[measured]", "[unconfirmed]")

# The legend table defines the markers, so it is the one table whose rows are
# not themselves claims. Named rather than guessed at by position.
LEGEND_HEADING = "| Marker | Means |"


def main():
    if not DOC.exists():
        print(f"{DOC} is missing, and it is the whole deliverable")
        return 1

    text = DOC.read_text(encoding="utf-8")
    problems = []

    for h in REQUIRED_HEADINGS:
        if h not in text:
            problems.append(f"missing heading: {h}")

    lines = text.split("\n")
    in_legend = False
    rows = unmarked = 0

    for i, line in enumerate(lines, 1):
        if line.strip() == LEGEND_HEADING:
            in_legend = True
            continue
        if in_legend:
            # The legend ends at the first line that is not part of its table.
            if not line.startswith("|"):
                in_legend = False
            continue
        if not line.startswith("|"):
            continue
        if re.match(r"^\|[\s:|-]+\|$", line):
            continue                      # the delimiter row
        if line.startswith("| ---") or set(line) <= set("| -"):
            continue
        # A header row names columns rather than making a claim.
        if re.search(r"\|\s*(Source|What it costs|Means)\s*\|", line):
            continue
        rows += 1
        if not any(m in line for m in MARKERS):
            unmarked += 1
            problems.append(f"line {i}: a claim with no provenance marker: "
                            f"{line.strip()[:72]}")

    if rows == 0:
        problems.append("no claim rows at all, so the check proves nothing")

    if problems:
        for p in problems:
            print(f"  {p}")
        print(f"check_decision: {len(problems)} problem(s)")
        return 1

    print(f"ok  4 headings, {rows} claim rows, every one of them sourced")
    return 0


if __name__ == "__main__":
    sys.exit(main())
