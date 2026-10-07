#!/usr/bin/env python3
"""The hardware findings have to keep their provenance and their verdicts.

    python tools/check_findings.py

Chapter 10's doc directory holds five documents that no program can verify for
truth: what a resistor's value is, where a net goes, whether a jumper cap is on.
A person has to open the schematic and look. So this checks the one thing a
program can check, which is whether each claim still says where it came from.

Five rules, and every one of them has cost something on this bench already:

  1. The headings each document promises are present. A findings page that has
     quietly lost "the open questions" is an inventory pretending to be
     complete.

  2. Every row of every table that has a Source column carries a provenance
     marker. A row without one is a claim whose origin has gone missing, and
     those survive three edits and then get quoted at somebody as fact. The
     rule keys on the column rather than on position: a table without a Source
     column is a summary, a map or a legend rather than a claim table, and each
     one is listed by name below, so adding a new one is a decision rather than
     an accident.

  3. Every marker that appears is one of the defined ones, and every one used on
     a page is explained by that page's legend. This catches a drifting
     vocabulary, which is how a provenance scheme dies: somebody writes [read]
     where the legend says [schematic], the two mean subtly different things,
     and within a month nobody can tell which rows were actually opened.

  4. Every verdict in the rewiring document is one of the four words that
     document defines. "Probably not" is not a verdict, it is a mood, and a mood
     cannot be reconsidered later because it never said why.

  5. Every [datasheet] row names the page it was read from. This rule was added
     after two figures in this chapter turned out to be typical values quoted as
     maximums, and after a third reversed a conclusion outright. In all three
     cases the claim was plausible and nothing in the row said which page to go
     and check. Four characters converts an argument into a lookup.

What this cannot check is whether any of it is still true. The schematic and the
datasheets are the authority, the bench is the tiebreaker, and the open
questions table is the honest list of what neither has been asked yet.
"""
import re
import sys
from pathlib import Path

DOC = Path(__file__).resolve().parent.parent / "doc"

FINDINGS = DOC / "board-findings.md"
NOTES = DOC / "datasheet-notes.md"
FIRSTLIGHT = DOC / "first-light.md"
REWIRING = DOC / "rewiring.md"
BRINGUP = DOC / "bench-bring-up.md"

MARKERS = (
    "[schematic]",
    "[wiki]",
    "[datasheet]",
    "[kernel]",
    "[measured]",
    "[inferred]",
    "[unconfirmed]",
    "[chapter]",
    "[arithmetic]",
)

VERDICTS = ("Do", "Do not", "Later", "Open")

REQUIRED = {
    FINDINGS: [
        "## How to read a line",
        "## 9. The complete 40-pin header map",
        "## 10. The terminal block",
        "## 11. Every link and jumper on the board",
        "## The open questions, as one list",
        "## How to re-derive all of this in about two minutes",
    ],
    NOTES: [
        "## How to read a line",
        "## A note on the diagrams in these pages",
        "# What the datasheets changed, as one list",
        "## What is still unread",
        "## The documents, with their addresses",
    ],
    FIRSTLIGHT: [
        "## How to read a line",
        "## The configuration under test",
        "## The termination jumpers, read off the board",
        "## What this run did not prove",
        "## Corrections this run forced",
    ],
    REWIRING: [
        "## The one that changes the plan",
        "## The zero ohm links",
        "## Termination, which is wiring rather than rewiring",
        "## Reflections on how the wrong readings happened",
        "## The order of work this leaves",
    ],
    BRINGUP: [
        "## Order of operations",
        "## What loopback proves, and what it does not",
        "## Still open",
    ],
}

# Per document, the tables that carry no Source column on purpose: legends,
# maps, summaries of corrections, and the document lists. Each is derived from
# rows that are themselves sourced elsewhere in the same file. Listing them by
# their header line means a new unsourced table fails rule 2 until somebody
# decides it belongs here.
CLAIM_DOCS = {
    FINDINGS: {
        "| Marker | Means |",
        "| Rail | Side | Where it comes from |",
        "| Header pin | BCM | Net on this board | Used by |",
        "| Overlay | Claims | Collides with | Severity |",
        "| Position | Net | What it is |",
        "| Scheme | Classic CAN | CAN FD |",
        "| # | Question | Why it matters | How to settle it |",
        "| Document | Where |",
        "| Document | Answers | Where |",
        "| End | Board | What it brings | What it lacks |",
    },
    FIRSTLIGHT: {
        "| Marker | Means |",
        "| Register | Range | Source |",
        "| Frame sent | `TX: packets` | `RX: packets` | Source |",
        "| Payload asked for | Legal CAN FD length? | `TX: bytes` increment | Source |",
        "| Not proved | Why | Source |",
        "| # | Before | After |",
    },
    NOTES: {
        "| Marker | Means |",
        "| Belief | Now | Why it matters |",
        "| # | Before | After | Where |",
        "| What | Why it is unread | What it would settle |",
        "| Document | Revision read | Address |",
    },
}

SEPARATOR = re.compile(r"^\|[\s:|-]+\|$")
BARE_MARKER = re.compile(r"\[([a-z]+)\](?!\()")

# A page reference, as "p8" or "p13". Deliberately loose about what precedes it,
# because a row may name a revision too, as in "SLOS346K p8".
PAGE_CITED = re.compile(r"\bp\d+\b")


def without_code(text):
    """Drop fenced code blocks.

    The vocabulary rule is about prose. Without this it reads a subscript as a
    marker: the findings page's worked example contains rows[key], and the first
    run of this checker duly reported [key] as an undefined provenance marker.
    That was the checker being wrong, not the document, so it is fixed here.
    """
    out = []
    fenced = False
    for line in text.split("\n"):
        if line.startswith("```"):
            fenced = not fenced
            continue
        if not fenced:
            out.append(line)
    return "\n".join(out)


def tables(lines):
    """Yield (header_line, [(lineno, row), ...]) for each Markdown table."""
    i = 0
    while i < len(lines):
        if (
            lines[i].startswith("|")
            and i + 1 < len(lines)
            and SEPARATOR.match(lines[i + 1].strip())
        ):
            header = lines[i].strip()
            rows = []
            j = i + 2
            while j < len(lines) and lines[j].startswith("|"):
                rows.append((j + 1, lines[j]))
                j += 1
            yield header, rows
            i = j
        else:
            i += 1


def check_headings(problems):
    for path, headings in REQUIRED.items():
        if not path.exists():
            problems.append(f"{path.name} is missing, and it is part of the deliverable")
            continue
        text = path.read_text(encoding="utf-8")
        for h in headings:
            if h not in text:
                problems.append(f"{path.name}: missing heading {h!r}")


def check_sources(problems):
    total = 0
    for path, exceptions in CLAIM_DOCS.items():
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8").split("\n")
        checked = 0
        for header, rows in tables(lines):
            if header in exceptions:
                continue
            if "Source" not in header:
                problems.append(
                    f"{path.name}: table {header!r} has no Source column and is "
                    "not listed as a deliberate exception in CLAIM_DOCS"
                )
                continue
            for lineno, row in rows:
                checked += 1
                if not any(m in row for m in MARKERS):
                    problems.append(
                        f"{path.name}:{lineno}: row has no provenance marker: "
                        f"{row.strip()[:72]}"
                    )
                elif "[datasheet]" in row and not PAGE_CITED.search(row):
                    problems.append(
                        f"{path.name}:{lineno}: [datasheet] row names no page: "
                        f"{row.strip()[:72]}"
                    )
        if checked == 0:
            problems.append(
                f"{path.name}: no claim rows were checked at all, which means "
                "the table parser stopped matching the document"
            )
        total += checked
    return total


def check_vocabulary(problems):
    for path in CLAIM_DOCS:
        if not path.exists():
            continue
        text = without_code(path.read_text(encoding="utf-8"))
        legend_start = text.find("| Marker | Means |")
        if legend_start < 0:
            problems.append(f"{path.name}: the marker legend table is gone")
            continue
        legend = text[legend_start : text.find("\n\n", legend_start)]
        for marker in sorted({f"[{m}]" for m in BARE_MARKER.findall(text)}):
            if marker not in MARKERS:
                problems.append(
                    f"{path.name}: {marker} looks like a provenance marker but "
                    "is not one of the defined ones"
                )
            elif f"`{marker}`" not in legend:
                problems.append(
                    f"{path.name}: {marker} is used but this page's legend does "
                    "not explain it"
                )


def check_verdicts(problems):
    if not REWIRING.exists():
        return 0
    lines = REWIRING.read_text(encoding="utf-8").split("\n")
    count = 0
    for lineno, line in enumerate(lines, 1):
        if not line.startswith("**Verdict:"):
            continue
        count += 1
        body = line[len("**Verdict:") :].strip().lstrip("*").strip()
        if not any(body.startswith(v) for v in VERDICTS):
            problems.append(
                f"{REWIRING.name}:{lineno}: verdict does not begin with one of "
                f"{VERDICTS}: {line.strip()[:72]}"
            )
    if count == 0:
        problems.append(f"{REWIRING.name}: no verdicts found, which cannot be right")
    return count


def main():
    problems = []
    check_headings(problems)
    rows = check_sources(problems)
    check_vocabulary(problems)
    verdicts = check_verdicts(problems)

    if problems:
        for p in problems:
            print(p)
        print(f"\n{len(problems)} problem(s)")
        return 1

    print(f"{rows} sourced claim rows checked, every one carries a marker")
    print("every [datasheet] row names the page it was read from")
    print(f"{verdicts} rewiring verdicts checked, every one is one of {VERDICTS}")
    print("chapter 10 hardware findings: shape and provenance intact")
    return 0


if __name__ == "__main__":
    sys.exit(main())
