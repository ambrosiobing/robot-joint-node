# Chapter 11: a joint protocol

State and command in sixty-four bytes. One description, several generated
outputs, and the generated files are not edited.

This chapter adds no hardware at all, which is why it was written first.

## What runs

```bash
python tools/gen_msgs.py proto/messages.yaml          # regenerate
python tools/gen_msgs.py proto/messages.yaml --check  # for the build
python test/test_msgs.py                              # the host codec
python test/test_gen.py                               # the validator
python tools/busload.py --audit-chapter               # the chapter's figures, checked
make                                                  # needs a C compiler
```

Everything except `make` runs on the win11 aquamarine authoring laptop. There is
no C compiler there, so the C half is written and has never been through one.

## One description, several outputs

```
proto/messages.yaml
  -> src/bus/joint_msgs.h    types, constants, compile time checks
  -> src/bus/joint_msgs.c    the packer and unpacker
  -> host/joint_msgs.py      the host twin of the same layout
  -> proto/joint.dbc         for tooling that has never heard of this repository
  -> test/vectors.json       boundary values, for the Python test
  -> test/vectors.h          the same values, for the C test
```

Each generated file carries the description's hash in its banner. `--check`
regenerates in memory and compares, so a hand edit fails the build rather than
surviving it. The C and Python sides are checked against the vectors rather than
against each other, so when they disagree the vector file names the field and
the value.

## The validator is as much the point as the generator

A layout whose fields overlap, whose offsets leave a hole, whose total misses the
declared length, or whose length is not one the frame format allows, is refused
before a byte is emitted. `test_gen.py` breaks the description nine ways and
confirms each one is caught, because a validator nobody has tested has never
rejected anything.

## The bus load audit, and the disagreement it settled

`tools/busload.py --audit-chapter` adopts the chapter's own two constants, 67
arbitration bits and a data phase of the payload plus 48 bits. On those it
reproduces the chapter exactly: 254 and 222 microseconds per frame, and a 190
per cent baseline.

Until Tuesday 6 October 2026 it disagreed with all three option percentages, and
the chapter was the one that was wrong:

| Option | Chapter, until 6 October | Correct |
|---|---|---|
| command at a quarter rate | 91% | 124%, still does not fit |
| arbitration at 1 Mbit/s | 61% | 137%, still does not fit |
| both | 38% | 90% |

What settled it is that the chapter prints those figures inside a block that
claims to be this program's output, naming the exact command. Running that
command does not produce them, so they were not two opinions about a model: one
side was a transcript, and a transcript either matches or does not.

It mattered more than a rounding difference. At 91, 61 and 38 all three
mitigations fit and the reader is offered a choice. In fact neither rescues the
design on its own, and only the combination fits, at 90 per cent, which is tight
rather than comfortable. The error ran in the direction that flattered the
design.

The audit is part of the build now rather than an excused failure, so the two
cannot drift apart again without a push going red.

## Acceptance criteria, and which are covered

| Criterion | Covered by |
|---|---|
| Both sides generated, a hand edit reverted by the build | `--check` |
| Round trip every field, including each type's boundaries | `test_msgs.py`, and `test_msgs.c` **unrun** |
| The compile time assertion on frame size fails if a field moves | `joint_msgs.h`, **never compiled, so never fired** |
| The length is one the format allows | `test_msgs.py`, and the validator |
| The bus load calculation runs as part of the build | `busload.py`, in `make` since 6 October 2026 |
| An expired command moves the node to its named policy | `command.c` and `test_expiry.c`, **unrun** |
| A command from beyond the horizon is rejected | `test_expiry.c`, **unrun** |
| The filters accept exactly three identifiers | **not written** |
| Sequence gap detection reports loss | **not written**; chapter 10's replay tool is ready for it |

Five of nine are covered and proven. Two are written and have never been
compiled. Two are not written.

The wrap arithmetic in `src/bus/command.c`, which reconstructs a 64 bit deadline
from the 32 bits the frame carries, was checked by hand against the cases
`test_expiry.c` asserts. That is weaker than running it and is the first thing to
re-check on a machine with a compiler.
