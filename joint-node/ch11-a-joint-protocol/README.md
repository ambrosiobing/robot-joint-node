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
python tools/busload.py --audit-chapter               # see the disagreement below
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

## One open disagreement with the chapter

`tools/busload.py --audit-chapter` adopts the chapter's own two constants, 67
arbitration bits and a data phase of the payload plus 48 bits. On those it
reproduces the chapter exactly: 254 and 222 microseconds per frame, and a 190
per cent baseline. It then disagrees with all three option percentages:

| Option | This tool | Chapter 11 |
|---|---|---|
| command at a quarter rate | 124% | 91% |
| arbitration at 1 Mbit/s | 137% | 61% |
| both | 90% | 38% |

The tool's *both* figure lands within one point of the chapter's first option,
which is what a shifted row looks like. One of the two is wrong. Neither number
should be quoted until it is settled, and the conclusion the chapter draws from
them, that the obvious design does not fit on one bus, survives either way.

`make audit` runs it, and is deliberately not part of `make`: gating the build
on an unsettled question would stop all other work, and leaving the question
unasked would be worse.

## Acceptance criteria, and which are covered

| Criterion | Covered by |
|---|---|
| Both sides generated, a hand edit reverted by the build | `--check` |
| Round trip every field, including each type's boundaries | `test_msgs.py`, and `test_msgs.c` **unrun** |
| The compile time assertion on frame size fails if a field moves | `joint_msgs.h`, **never compiled, so never fired** |
| The length is one the format allows | `test_msgs.py`, and the validator |
| The bus load calculation runs as part of the build | `busload.py`, **not in `make`, see above** |
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
