# joint-node

The node the volume describes, as code. New and small: the protocol, its
generator, the tools around it, and the tests that need no hardware.

The chapters are the specification. This tree is one reading of them, and where
the two disagree the chapter wins unless the code can show the chapter wrong,
which has happened once already and is recorded below.

## What runs today, and where

| Check | Needs | Runs on the authoring laptop |
|---|---|---|
| `python tools/gen_msgs.py proto/messages.yaml --check` | python, pyyaml | yes |
| `python tools/check_layers.py` | python | yes |
| `python test/test_msgs.py` | python | yes |
| `python test/test_gen.py` | python, pyyaml | yes |
| `make ctest` | a C compiler | **no**, there is none on that laptop |
| `make` | both | no |

The C tests are written and have never been run. They run on the Pi 4, or
anywhere with a compiler. Until then the honest statement is that the host
codec round trips and the node's codec is untested, not that the protocol is
proven.

The wrap arithmetic in `src/bus/command.c` was checked by hand against the same
cases `test/test_expiry.c` asserts, which is weaker than running it.

## One description, several outputs

```
proto/messages.yaml          the source of truth
  -> src/bus/joint_msgs.h    the node's types, constants and compile time checks
  -> src/bus/joint_msgs.c    the node's packer and unpacker
  -> host/joint_msgs.py      the host twin of the same layout
  -> proto/joint.dbc         for tooling that has never heard of this repository
  -> test/vectors.json       boundary values, for the python test
  -> test/vectors.h          the same values, for the C test
```

Generated files carry the description's hash in their banner and are not edited.
`--check` regenerates in memory and compares, so an edit fails the build. The C
and python sides are checked against the vectors rather than against each other,
so when they disagree the vector file names the field and the value.

The validator is as much the point as the generator. A layout whose fields
overlap, whose offsets leave a hole, whose total misses the declared length, or
whose length is not one the frame format allows, is refused before a byte is
emitted. `test/test_gen.py` breaks the description nine ways and confirms each
one is caught, because a validator nobody has tested has never rejected
anything.

## The rule that makes the rest possible

Nothing above `src/bsp/` includes a vendor header. `tools/check_layers.py`
enforces it and fails with the file and the include. Chapter 20 has to build
most of this node on a host, and that only works if the rule was kept from the
start rather than retrofitted at the end.

## One open disagreement with the chapter

`tools/busload.py --audit-chapter` reproduces chapter 11's per frame figures
exactly, 254 and 222 microseconds, and its 190 per cent baseline, because it
adopts the chapter's own two constants. It disagrees with all three of the
chapter's option percentages:

| Option | This tool | Chapter 11 |
|---|---|---|
| command at a quarter rate | 124% | 91% |
| arbitration at 1 Mbit/s | 137% | 61% |
| both | 90% | 38% |

The tool's *both* figure lands within one point of the chapter's first option,
which is what a shifted row looks like. One of the two is wrong. Neither number
should be quoted until it is settled, and the conclusion the chapter draws from
them, that the obvious design does not fit on one bus, survives either way.

## Not written yet

The filter table of chapter 11 step 8, the host listener and setpoint source of
chapter 10, and everything from chapter 12 onward. The tree names the
directories so the shape is visible, and empty is empty.
