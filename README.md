# Twenty Chapters, One Robot Joint Node

One joint node for a robot arm, taken apart into twenty chapters and built on a
bench that has no robot arm on it. A NUCLEO-H7A3ZI-Q, a Raspberry Pi 4, three
sensor shields, one transceiver, and no motor.

**314 pages, 101 figures.** Deliverables, rebuilt with `python build.py`:

- `robot-joint-node.pdf` the book, built with pdflatex
- `robot-joint-node.html` the same content as one self-contained
  HTML file, with every figure inlined as SVG

By the end the node keeps a one kilohertz period and can prove it without a
probe, shares one clock across its sensors and its bus, reads a real encoder and
a real inertial unit, drives an actuator that is not there, speaks CAN-FD from
bit timing upward, survives a device older than itself on the same wire, joins a
robot framework as a participant, publishes standard message types with one
field honestly left empty, closes a loop and reports following error, stops
safely and latches, can be updated over the wire it already has, and is checked
nightly by a rig that turns a build red.

## The rule that runs through it

Three things a robotics firmware role asks for cannot be demonstrated on this
bench: a motor and drive stage, a force or torque sensor, and a real-time
Ethernet fieldbus. The volume names all three in chapter 1, designs around them
deliberately, and marks every figure accordingly.

- A **solid** outline is hardware that is present.
- A **dashed** outline is a model standing in for hardware that is not.
- A **dotted grey** block is hardware that is absent and explained rather than
  built.

No measurement taken through a dashed block is a measurement of anything
physical, every budget table carries a `Measured` column that reads *not
measured* until something has been, and a claim the research could not confirm
is written as a question rather than as an assertion.

That discipline produced more of the book than expected. Several of its most
useful paragraphs are of the form *this is not what everybody says*: that this
part's flash word is sixteen bytes and not thirty-two, that its backup registers
live in a different peripheral from its better known sibling's, that a widely
repeated middleware footprint comes from a commercial blog rather than from the
project, that a popular bootloader changed licence in 2025, and that the
collaborative robot technical specification is not withdrawn. Appendix C lists
all of them; appendix D lists the eight gaps the research could not close, which
are claimed as new work rather than dressed up as a survey.

## Source layout

| Path | What it is |
|---|---|
| `main.tex` | preamble, authoring macros, five parts |
| `tikz_preamble.tex` | shared TikZ and circuitikz styles, including the field bus, frame layout, control loop, joint and safe-state styles, and the three honesty styles |
| `sections/front.tex` | about, the honesty rule, the bench, how to read it |
| `sections/jNN.tex` | one file per chapter, 01 to 20 |
| `sections/appendix.tex` | the chapters at a glance, the honesty ledger, the corrections, the gaps, the open questions, the licence categories, the reference library |
| `figures/front_map.tex` | the dependency map |
| `figures/jNN_{arch,wiring,uml,data,timing}.tex` | five figures per chapter |
| `build.py` | figures to SVG, PDF, and the HTML converter |
| `lint.py` | house-style check |
| `crosscheck.py` | book-level consistency |
| `AUTHORING.md` | the contract every chapter follows, the honesty rule, the confirm-before-writing list, the variant matrix |
| `SOURCE.md` | the prior-art pool with verification marks, the four licence categories, the corrections, the claimable gaps, the open questions |
| `build/` | scratch output, gitignored, safe to delete |

Chapter files use a `j` prefix so that a cross-reference or a copied figure can
never silently resolve against a sibling volume's files.

## Checks

    python build.py --check sections/j07.tex   # one chapter alone, with its figures
    python lint.py                             # house rules over every chapter
    python crosscheck.py                       # book-level consistency
    python build.py --pdf                      # PDF only
    python build.py --html                     # figures and HTML only

`lint.py` checks prose for em and en dashes, non-ASCII characters, violent
idioms and the required subsection skeleton, and checks code blocks for
non-ASCII and for lines longer than the page can print. `crosscheck.py` checks
what per-chapter linting cannot see: the variant matrix, figure coverage,
cross-references, chapter titles against the authoring guide, and that every
date is written in full.

One exemption is worth knowing about. `\pubdate{April 2010}` marks a date a
publisher gives to month precision only. The house rule is that every date the
volume states carries weekday, day, month and year, and that rule cannot apply
to a day a publisher never published; writing one would be inventing a fact.
The macro makes the exemption explicit in the source, so every month-and-year
that is *not* wrapped is still reported as a defect.

The volume's identity lives in exactly one `DOC` block per tool file, and both
`build.py` and `lint.py` assert that the folder name matches it before
generating anything.

## Requirements

MiKTeX or TeX Live with pdflatex, latex, dvisvgm, circuitikz, tcolorbox and
listings; Python 3.10 or newer. No other Python package is needed to build.

## Licence

MIT, see `LICENSE`. The book cites a great deal of other people's work; every
chapter's Sources section records what was taken from where and under what
terms, and appendix F sets out the four licence categories the volume applies.
