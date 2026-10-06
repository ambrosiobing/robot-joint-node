# Chapter 9: bit timing, computed and refused

One bit, divided into time quanta, for each of the two phases. The node computes
its timing from the kernel clock it reads back at boot and refuses to start if
either phase cannot be solved exactly.

This is the arithmetic half of chapter 9. It needs no board, no transceiver and
no bus, which is why it was written first. The loopback, the message memory
layout and the first frame need the Nucleo and are not here yet.

## What runs

```bash
python tools/gen_bt_vectors.py            # regenerate the vectors
python tools/gen_bt_vectors.py --check    # for the build
python tools/gen_bt_vectors.py --table    # the table in doc/bit-timing.md
python test/test_bittiming.py             # the reference and its invariants
make                                      # needs a C compiler
```

Everything except `make` runs on the win11 aquamarine authoring laptop. There is
no C compiler there, and the house rule is that one is never run there even
though Qt ships one, so the C half is never built where it is written.

It is built in continuous integration, which compiled it for the first time on
Tuesday 6 October 2026 with `-std=c11 -Wall -Wextra -Werror` and ran it against
the same vectors the Python half uses. Both halves are proven now. Neither has
been near the board, which is a different claim and is made nowhere.

## Why exact, and why it refuses

A bit rate that is one part in a thousand out works between two nodes that are
wrong in the same direction, and fails against anything else. So the prescaler
has to divide the kernel clock with no remainder, the quantum rate has to divide
the bit rate the same way, and anything else is refused rather than rounded into
place.

Four of the fifteen vectors are refusals, each for a different reason:

| Refused because | Case |
|---|---|
| the bit rate does not divide the quantum rate | 666667 bit/s from 80 MHz |
| no prescaler divides the clock into the rate | 2 Mbit/s from 33 MHz |
| too few quanta a bit for a usable sample point | 2 Mbit/s from 6 MHz, 3 quanta |
| fewer quanta than the registers can hold | 40 Mbit/s from 80 MHz, 2 quanta |

The first draft of that list had three cases labelled REFUSE and only one of
them refused. Two were legal timings with a disapproving name on them. They were
corrected rather than quietly dropped, because the point of the list is that the
refusal path has been exercised.

## The two phases do not share limits

This is the part that is easy to get wrong, and the chapter's printed snippet
gets it wrong: it uses one set of constants for both phases. One set cannot be
right for both. The fields that hold these numbers are much narrower in the data
phase, and the limits that suit the nominal phase silently accept data timings
the hardware cannot hold at all.

| Phase | Prescaler | seg1 | seg2 | jump width |
|---|---|---|---|---|
| Nominal | 1 to 512 | 1 to 256 | 1 to 128 | 1 to 128 |
| Data | 1 to 32 | 1 to 32 | 1 to 16 | 1 to 16 |

Those are the widths of the register fields, taken from the field definitions
rather than from a summary of them. A field of n bits holds 0 to 2^n - 1, and
the hardware adds one, so a nine bit prescaler field is a prescaler of 1 to 512.
That is why `bt_compute` takes the limits as an argument.

The eight quanta floor is **not** one of those limits. The registers allow three
quanta, and three is legal and useless. Eight is this volume's choice, written
as a field so that anyone who has read the standard, which has not been read
here, can lower it with a reason.

## The clock is not an arbitrary choice

Chapter 9's budget table asks for 500 kbit/s at an eighty per cent sample point
and 2 Mbit/s at seventy five. An 80 MHz kernel clock hits both exactly, with a
prescaler of one in each phase:

| Phase | tq a bit | seg1 | seg2 | Sample point |
|---|---|---|---|---|
| Nominal, 500 kbit/s | 160 | 127 | 32 | exactly 80 per cent |
| Data, 2 Mbit/s | 40 | 29 | 10 | exactly 75 per cent |

100 MHz does not: the data phase lands on 76 per cent, because 25 quanta cannot
be divided three quarters of the way along. 60 MHz lands on 76.7. The full table
is in [`doc/bit-timing.md`](doc/bit-timing.md), generated rather than typed.

## One reference, two implementations, one vector file

```
tools/gen_bt_vectors.py    the reference, and the generator
  -> test/vectors.json     every case and its expected answer
  -> test/vectors.h        the same, so the C test needs no JSON parser

src/bus/bittiming.{h,c}    the node's own copy, in C
test/test_bittiming.py     the reference against the vectors and the invariants
test/test_bittiming.c      the C against the same vectors
```

The C and the Python are never compared against each other. Both are compared
against the vector file, so when they disagree the output says which case and
which field rather than leaving somebody to guess which side moved.

`--check` regenerates in memory and compares, so a hand edit of either generated
file fails the build rather than surviving it.

One trap worth recording, because it would have produced exactly one wrong row
and fourteen right ones. Python rounds half to even and C rounds half away from
zero, and one design point lands exactly on a half: seventy five per cent of 30
quanta is 22.5, where Python would have chosen 22 and C 23. The decision is
integer on both sides now, and the only float is the sample point fraction the
caller passes in, converted once before anything is decided.

## Acceptance criteria, and which are covered

| Criterion | Covered by |
|---|---|
| Both phases computed from the kernel clock | `bt_compute`, both limit sets |
| Either phase refused when it cannot be solved exactly | 4 of 15 vectors, all checked to refuse |
| The design points hit their stated sample points exactly | `test_bittiming.py`, asserted, not eyeballed |
| A bit is the sync quantum plus the two segments | asserted in both tests |
| The result fits the registers that must hold it | `test_bittiming.py`, against the field widths |
| A hand edit of the vectors does not survive | `--check` |
| The same arithmetic in C | `bittiming.c` and `test_bittiming.c`, compiled and run in CI |
| The message memory layout, the loopback, the first frame | **not here yet**, they need the board |
