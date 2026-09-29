# Twenty Chapters, One Robot Joint Node

One joint node for a robot arm, taken apart into twenty chapters and built on a
bench that has no robot arm on it. A NUCLEO-H7A3ZI-Q, a Raspberry Pi 4, three
sensor shields, one transceiver, and no motor.

**314 pages, 101 figures, 20 chapters.** Each chapter stands on its own: it
states what it adds to the node, what it depends on, what is real and what is
only modelled, and it ends in something measurable.

**Read it.** The whole volume is in [`chapters/`](chapters/) as Markdown
with its figures beside it. Start with
[About this volume](chapters/00-about-this-volume.md), or take a chapter
from the table below.

**Or build it.** The PDF and a single self-contained HTML file come from
the same source and stay local:

    python build.py --chapter 7

That writes `chapter-07-the-actuator-you-do-not-have.pdf` and a matching
self-contained `.html` with its five figures inlined.

**Contents**
[Read it](chapters/) ·
[The rule](#the-rule-that-runs-through-it) ·
[The twenty chapters](#the-twenty-chapters) ·
[Building](#building) ·
[Checks](#checks) ·
[Layout](#repository-layout) ·
[Requirements](#requirements) ·
[Licence](#licence)

## The rule that runs through it

Three things a robotics firmware role asks for cannot be demonstrated on this
bench: a motor and drive stage, a force or torque sensor, and a real-time
Ethernet fieldbus. The book names all three in chapter 1, designs around them
deliberately, and marks every figure accordingly.

| In a figure | Means |
|---|---|
| A **solid** outline | hardware that is present |
| A **dashed** outline | a model standing in for hardware that is not |
| A **dotted grey** block | hardware that is absent and explained rather than built |

No measurement taken through a dashed block is a measurement of anything
physical. Every budget table carries a `Measured` column that reads *not
measured* until something has been. A claim the research could not confirm is
written as a question rather than as an assertion.

That discipline produced more of the book than expected. Several of its most
useful paragraphs are of the form *this is not what everybody says*: that this
part's flash word is sixteen bytes and not thirty-two, that its backup registers
live in a different peripheral from its better known sibling's, that a widely
repeated middleware footprint comes from a commercial blog rather than from the
project, that a popular bootloader changed licence in 2025, and that the
collaborative robot technical specification is not withdrawn. Appendix C lists
them all; appendix D lists the eight gaps the research could not close, which
are claimed as new work rather than dressed up as a survey.

## The twenty chapters

Difficulty is 1 to 5. Effort is in evenings. Chapter 1 gates everything; after
that the reading order is mostly preference, and the dependency map in the front
matter draws the parts that are not.

| # | Chapter | What the node gains | Diff. | Effort |
|---|---|---|---|---|
| 1 | [What a joint node is, and the bench that stands in for one](chapters/01-what-a-joint-node-is.md) | An identity and a bring-up | 2/5 | Two evenings |
| 2 | [The control period: 1 kHz you can prove](chapters/02-the-control-period.md) | A heartbeat | 3/5 | Two evenings |
| 3 | [One clock for sensors, loop and bus](chapters/03-one-clock-for-sensors.md) | A shared time base | 3/5 | Two evenings |
| 4 | [The board support package, and a board file you can hand over](chapters/04-the-board-support-package.md) | A documented hardware interface | 2/5 | Two evenings |
| 5 | [The encoder: quadrature in hardware, and one you generate](chapters/05-the-encoder.md) | Position | 3/5 | Three evenings |
| 6 | [The inertial unit as the joint's inner ear](chapters/06-the-inertial-unit-as-the-joints-inner-ear.md) | Motion sensing | 3/5 | Three evenings |
| 7 | [The actuator you do not have: PWM, dead time, and a plant model](chapters/07-the-actuator-you-do-not-have.md) | A command output and a simulated joint | 4/5 | Four evenings |
| 8 | [Force and torque: the signal you cannot buy](chapters/08-force-and-torque.md) | An estimate, honestly labelled | 4/5 | Three evenings |
| 9 | [CAN-FD from the controller out: bit timing and the first frame](chapters/09-can-fd-from-the-controller-out.md) | A voice | 4/5 | Three evenings |
| 10 | [The motion master: the bus on Linux](chapters/10-the-motion-master.md) | A listener and a commander | 3/5 | Two evenings |
| 11 | [A joint protocol: state and command in sixty-four bytes](chapters/11-a-joint-protocol.md) | A vocabulary | 3/5 | Three evenings |
| 12 | [Network management: heartbeat, node state, bus-off and recovery](chapters/12-network-management.md) | Membership | 4/5 | Three evenings |
| 13 | [Two speeds on one wire, and why the old node errors](chapters/13-two-speeds-on-one-wire.md) | A diagnosis | 3/5 | Two evenings |
| 14 | [The node as a middleware participant, and the agent that hosts it](chapters/14-the-node-as-a-middleware-participant.md) | A place in the robot | 5/5 | Four evenings |
| 15 | [Joint state and joint command as messages](chapters/15-joint-state-and-joint-command-as-messages.md) | A standard shape | 3/5 | Two evenings |
| 16 | [The loop closed over the bus: setpoint in, state out, following error](chapters/16-the-loop-closed-over-the-bus.md) | A closed loop | 5/5 | Four evenings |
| 17 | [What a real-time fieldbus would change, and why it is not on this bench](chapters/17-what-a-real-time-fieldbus-would-change.md) | An honest boundary | 2/5 | Two evenings |
| 18 | [Safe states, and the workspace sensor that triggers one](chapters/18-safe-states.md) | A way to stop | 4/5 | Three evenings |
| 19 | [Update over the bus: a node you can reach but not touch](chapters/19-update-over-the-bus.md) | Maintainability | 5/5 | Four evenings |
| 20 | [The rig: injected faults, tracking error, and a build that fails](chapters/20-the-rig.md) | Proof | 5/5 | Five evenings |

Every chapter has the same twenty-one sections: why it exists, the prior art and
what is taken from it, what the node gains, the parts it uses, a system
architecture figure, the peripheral configuration, the wiring, a memory and
timing budget, a software design in UML, a data-flow sketch, a repository
layout, numbered steps with real commands and real code, measurable acceptance
criteria, the variants it touches, pitfalls, best practices, stretch goals, a
sourced roadmap, the evidence to publish, and its sources.

## Building

    python build.py --chapter 7      one chapter, PDF and self-contained HTML
    python build.py --chapters       all twenty, one file each
    python build.py                  the whole book, PDF and one HTML file
    python mdbuild.py                the Markdown edition, chapters and figures

Built output is not committed, with one deliberate exception. The PDF and the
HTML are artefacts of this source, they are regenerated in a couple of minutes,
and keeping them out of the history keeps the repository small and every
published file traceable to the commit it came from. The figures are the
exception: they are committed as SVG, because the Markdown edition cannot draw
a single diagram in a browser without them.

## Checks

    python lint.py                             house rules over every chapter
    python crosscheck.py                       book-level consistency
    python build.py --check sections/j07.tex   compile one chapter and report on it

`lint.py` checks prose for em and en dashes, non-ASCII characters, violent
idioms and the required section skeleton, and checks code blocks for non-ASCII
and for lines longer than the page can print. `crosscheck.py` checks what
per-chapter linting cannot see: the variant matrix, figure coverage,
cross-references, chapter titles against the authoring guide, and that every
date is written in full. Both run on every push, see
[`.github/workflows/checks.yml`](.github/workflows/checks.yml).

One exemption is worth knowing about. `\pubdate{April 2010}` marks a date a
publisher gives to month precision only. The house rule is that every date the
book states carries weekday, day, month and year, and that rule cannot apply to
a day a publisher never published; writing one would be inventing a fact. The
macro makes the exemption explicit in the source, so every month-and-year that
is *not* wrapped is still reported as a defect.

## Repository layout

| Path | What it is |
|---|---|
| `chapters/NN-title.md` | the Markdown edition, one file per chapter, generated from `sections/` |
| `figures/NAME.svg` | every figure rendered, committed so the Markdown draws in a browser |
| `sections/jNN.tex` | one file per chapter, 01 to 20 |
| `sections/front.tex` | about, the honesty rule, the bench, how to read it |
| `sections/appendix.tex` | the chapters at a glance, the honesty ledger, the corrections, the gaps, the open questions, the licence categories, the reference library |
| `figures/jNN_{arch,wiring,uml,data,timing}.tex` | five figures per chapter |
| `figures/front_map.tex` | the dependency map |
| `main.tex` | preamble, authoring macros, five parts |
| `tikz_preamble.tex` | shared TikZ and circuitikz styles, including the field bus, frame layout, control loop, joint and safe-state styles, and the three honesty styles |
| `mdbuild.py` | the Markdown converter, which reuses `build.py`'s parser |
| `build.py` | figures to SVG, PDF, per-chapter builds, and the HTML converter |
| `lint.py` | house-style check |
| `crosscheck.py` | book-level consistency |
| `AUTHORING.md` | the contract every chapter follows, the honesty rule, the confirm-before-writing list, the variant matrix |
| `SOURCE.md` | the prior-art pool with verification marks, the four licence categories, the corrections, the claimable gaps, the open questions |
| `CONTENTS.md` | the chapter table, generated, which the table above follows |
| `build/` | scratch output, ignored, safe to delete |

Chapter files use a `j` prefix so that a cross-reference or a copied figure can
never silently resolve against a sibling volume's files. The book's identity
lives in exactly one `DOC` block per tool file, and both `build.py` and
`lint.py` refuse to run if the folder name stops matching it.

## Requirements

MiKTeX or TeX Live with `pdflatex`, `latex`, `dvisvgm`, `circuitikz`,
`tcolorbox` and `listings`; Python 3.10 or newer. No Python package outside the
standard library is needed.

## Licence

MIT, see [`LICENSE`](LICENSE). The book cites a great deal of other people's
work: every chapter's Sources section records what was taken from where and
under what terms, and appendix F sets out the four licence categories the book
applies to its own dependencies.
