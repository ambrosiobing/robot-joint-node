# Chapter 9: bit timing, computed and refused

One bit, divided into time quanta, for each of the two phases. The node computes
its timing from the kernel clock it reads back at boot and refuses to start if
either phase cannot be solved exactly.

This is the arithmetic half of chapter 9. It needs no board, no transceiver and
no bus, which is why it was written first. The loopback, the message memory
layout and the first frame need the Nucleo and are not here yet.

The hardware at the other end of that bus is written up in chapter 10, and three
numbers from it belong to this chapter's arithmetic. The controller end's clock
is a packaged 40 MHz oscillator specified at plus or minus 20 ppm, which is a
printed tolerance rather than an assumed one. The transceiver loop delays that
step 5's delay compensation needs are collected there, with the one that is
still unknown marked as unknown. And the two ends will not agree on a sample
point, because 40 MHz and 80 MHz divide differently, which is a finding and not
a fault. See
[../ch10-the-motion-master/doc/board-findings.md](../ch10-the-motion-master/doc/board-findings.md),
in particular the section on the numbers this bus will be judged on.

## What runs

```bash
python tools/gen_bt_vectors.py            # regenerate the bit timing vectors
python tools/gen_bt_vectors.py --check    # for the build
python tools/gen_bt_vectors.py --table    # the table in doc/bit-timing.md
python test/test_bittiming.py             # the reference and its invariants

python tools/gen_msgram.py                # regenerate the layout vectors
python tools/gen_msgram.py --check        # for the build
python tools/gen_msgram.py --audit-chapter  # against chapter 9's budget row
python tools/gen_msgram.py --table        # the table in doc/message-memory.md
python test/test_msgram.py                # the layout and its invariants

python tools/gen_frame_vectors.py         # regenerate the frame vectors
python tools/gen_frame_vectors.py --check # for the build
python tools/gen_frame_vectors.py --table # the table in doc/frame-rules.md
python test/test_frame.py                 # the length code and its round trip

python tools/gen_initplan.py              # regenerate the configuration order
python tools/gen_initplan.py --check      # for the build
python tools/gen_initplan.py --table      # the table in doc/node-order.md
python test/test_initplan.py              # the order, and its seven rules

python tools/gen_planrun.py               # regenerate the executor's scenarios
python tools/gen_planrun.py --check       # for the build
python tools/gen_planrun.py --table       # the table in doc/node-order.md
python test/test_planrun.py               # six register models, five verdicts

make                                      # needs a C compiler
```

On the WSL side of win11 skyhorizon, `bing@JPTOUPM678`, the whole thing
including the C runs in seconds:

```bash
cd ~/src/robot-joint-node/joint-node/ch09-can-fd-from-the-controller-out; make
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

**And what it refuses is the bit rate, not the sample point.** That distinction
was left vague here until Wednesday 7 October 2026 and it matters: `bt_compute`
demands that the prescaler divide the kernel clock exactly and that the quantum
rate divide the bit rate exactly, then it reports whichever sample point the
quanta allow. An inexact sample point is reported, never refused. The reason is
that two ends of a CAN bus must agree on the bit rate and are not required to
sample at the same point, and the node's own first image lands on 81.3 per cent
rather than 80 for exactly that reason. See [`doc/node-clock.md`](doc/node-clock.md).

Five of the eighteen vectors are refusals:

| Refused because | Case |
|---|---|
| the bit rate does not divide the quantum rate | 666667 bit/s from 80 MHz |
| no prescaler divides the clock into the rate | 2 Mbit/s from 33 MHz |
| too few quanta a bit for a usable sample point | 2 Mbit/s from 6 MHz, 3 quanta |
| fewer quanta than the registers can hold | 40 Mbit/s from 80 MHz, 2 quanta |
| the same floor, at the clock the board can actually reach | 2 Mbit/s from 8 MHz, 4 quanta |

The first four fail for four different reasons, which was the original point of
the list. The fifth repeats the third's mechanism on purpose, because it is not
a probe of the rule but a design point: it is the data phase this chapter's
budget asks for, at the only kernel clock the first image can reach.

The first draft of that list had three cases labelled REFUSE and only one of
them refused. Two were legal timings with a disapproving name on them. They were
corrected rather than quietly dropped, because the point of the list is that the
refusal path has been exercised.

## The node's clock, which is not one of the hypothetical ones

Settled Wednesday 7 October 2026, and it shortened the first image considerably.

`FDCANSEL` offers three sources and no more: HSE, `PLL1_Q` and `PLL2_Q`. **There
is no HSI option**, so the 64 MHz the part boots on cannot clock this peripheral
at all, and the 8 MHz HSE from the on-board debugger is the only source that
needs no PLL. `00`, which selects it, is already the reset value.

Eight MHz solves 500 kbit/s at a prescaler of one, sixteen quanta a bit, sample
point 81.3 per cent. It does **not** solve a 2 Mbit/s data phase: four quanta a
bit is below this volume's floor of eight. So CAN FD's fast phase needs a PLL
whatever transceiver is fitted, which is a second and independent reason chapter
13 waits, the first being a transceiver that does not specify loop delay
symmetry.

The full argument, the sources for the register field, the measured HSE
deviation of about 0.14 per cent and the open question that deviation raises are
in [`doc/node-clock.md`](doc/node-clock.md).

## The registers, and the one trap that fails silently

Also Wednesday 7 October 2026. A timing that has been solved still has to be
written, and `bt_compute` stopped one step short of that until now.

`bt_pack_nbtp` and `bt_pack_dbtp` produce the register word, and
`bt_unpack_nbtp` and `bt_unpack_dbtp` read the four numbers back out of it, so a
configuration can be verified against what was intended rather than against the
fact of having written it. Every solved vector now carries its word, and both
implementations pack it, unpack it and compare.

**The two phases pack into different widths, and mixing them up is silent.** A
nominal timing at 80 MHz has a segment 1 of 127; the data field is five bits
wide and would truncate it to 31, configuring a bit rate nobody chose with
nothing reported anywhere. So both packers return a boolean and write nothing
when a value will not fit, and the caller must check it. The test packs a
nominal timing as a data word and requires the refusal, and that check was
watched turning red with the guard removed.

**The field widths confirm all eight of this chapter's limit constants**, from a
source that is not the one they came from. `BT_NOMINAL` and `BT_DATA` were
filled in from the register field definitions when this chapter was written. The
widths in the mainline driver for this peripheral agree on every one: 512, 256,
128 and 128 for the nominal phase, 32, 32, 16 and 16 for the data phase. That
closes a soft spot nobody had flagged.

## Stage two: the order, as data rather than as board code

Thursday 8 October 2026. The values were settled the day before. The order is the
other half, and it is the half that cannot be debugged on the board, because three
of the four things guessed about it were wrong and all three fail silently.

So the order is not written as a function that configures a controller. It is
produced as **data** by `fdcan_plan` in `src/bus/initplan.c`: twenty three
operations, each with a register, a mask, a value and a name, built by a function
that touches no hardware and names no vendor header. `src/bsp/` will walk the
list. Chapter 4's rule gets this for free, which is the rule's whole point.

```
python tools/gen_initplan.py --table      # the plan, step by step
python test/test_initplan.py              # the order and its seven rules
```

The plan is checked twice, and the second check is the one worth having:

| Check | Catches |
|---|---|
| it matches the committed vectors | drift, and a hand edit of either side |
| it obeys seven ordering rules | a sequence that is **wrong**, which no vector file can see, because vectors regenerated from a wrong generator agree with it perfectly |

That second claim was proved rather than asserted. Four mutations were applied to
the reference, **the vector file was regenerated each time so that it agreed with
the mutation**, and the test went red on the invariant alone: the message RAM
clear removed, `TEST` written before `CCCR`, `MON` dropped from internal loopback,
and `INIT` released before `CCE` was closed. The third was reported by the
distinct-plans rule, which noticed internal and external loopback had become the
same thing, rather than by the `MON` rule, which would also have fired.

`FDCAN_PLAN_MAX` bounds the array the plan is written into, and the C test asks
for one step fewer than the plan needs and requires a refusal, so a plan that
outgrows the bound fails on a host rather than overrunning a buffer on the board.

### And walking it, which is where the other two silent failures live

`fdcan_run` in `src/bus/planrun.c` executes the plan under two rules.

**Every write is read back.** `CCE` declines while `INIT` is clear, `TEST`
declines while `CCCR.TEST` is clear, and a wrong peripheral base accepts every
write and reads back the reset value. A sequence that writes and moves on reports
success in all three cases.

**Every wait is bounded and counted.** An unbounded spin on a board with one
serial port produces a blank console, which is indistinguishable from a part that
never started. A spin limit of zero is refused rather than read as "no limit".

Neither rule can be exercised on hardware, because a register that declines a
write does not announce it, which is the entire problem. So the hardware is a
256 word model with one deviation per scenario, **written twice**, once in the
generator and once in the C test, and both are compared against the recorded
sequence of bus accesses rather than against each other.

Six scenarios, six distinct outcomes, five distinct verdicts. The one that
justifies the read-back rule is `test declines`: it needs loopback, because in
normal mode `TEST` is written with zero and reads zero, so a declined write there
is indistinguishable from an obeyed one. In loopback only the read-back notices.

Three more mutations, each removing one rule and each with the vectors regenerated
to agree, and all three turned the test red. The instructive one is removing
modify verification: the run does not then succeed, it reaches the final wait and
fails there instead, four steps late and pointing at the wrong register. A test
that only asked whether the run failed would have passed it.

The sequence, the three steps that are not obvious from it, the six scenarios and
the list of what `src/bsp/` still has to contain are in
[`doc/node-order.md`](doc/node-order.md).

The addresses, the full register map, the `CCCR` gate, the message RAM
addressing question and **the sixteen step initialisation order** are in
[`doc/node-registers.md`](doc/node-registers.md). **The peripheral is not ST's
design**, it is the Bosch M_CAN, and the mainline Linux driver for it is code
that runs rather than a vendor summary, which is why it is the source here.

Four things that had been open there closed the same evening, and three of the
answers were not what the guesses said. The message RAM has to be **cleared**
before anything else, word by word, or an uninitialised buffer reads back as a
parity error that looks like a bus fault. Internal loopback is **three** bits and
not one, `CCCR.TEST`, `CCCR.MON` and `TEST.LBCK`, and `MON` is the one that keeps
the frame off the wire. `CCCR` must be written **before** `TEST`, because
`CCCR.TEST` is what makes `TEST` writable at all, so the obvious order fails
silently. And `GFC` = `0` **accepts** every non-matching frame rather than
rejecting it, which is the opposite way round from what the name suggests.

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
| Either phase refused when it cannot be solved exactly | 5 of 18 vectors, all checked to refuse |
| The design points hit their stated sample points exactly | `test_bittiming.py`, asserted, not eyeballed |
| A bit is the sync quantum plus the two segments | asserted in both tests |
| The result fits the registers that must hold it | `test_bittiming.py`, against the field widths |
| A hand edit of the vectors does not survive | `--check` |
| A solved timing packs into the register that must hold it | `bt_pack_nbtp` and `bt_pack_dbtp`, every solved vector |
| A packed word reads back as what went into it | `bt_unpack_nbtp` and `bt_unpack_dbtp`, six fields per vector |
| A timing too wide for its field is refused, not truncated | asserted in both tests, and watched failing with the guard removed |
| The configuration order is checked without hardware | `fdcan_plan`, 8 configurations, 2 of them refused |
| The order is checked for being right, not only unchanged | seven rules in `test_initplan.py`, each watched failing against agreeing vectors |
| A plan that outgrows its array refuses rather than overruns | `test_initplan.c`, asked for one step too few |
| Every write is read back, and a declined one is caught | `fdcan_run`, and the `test declines` scenario where only the read-back notices |
| Every wait is bounded, and a timeout names its step | `fdcan_run`, and the `cce reverts` scenario, which stops at the limit |
| Both executor rules are load bearing | three mutations, each removing one, each watched failing against agreeing vectors |
| The same arithmetic in C | `bittiming.c` and `test_bittiming.c`, compiled and run in CI |
| The message memory laid out rather than assumed | `msgram.c`, asserted at compile time and checked in both tests |
| The layout fits, and its sections tile with no hole or overlap | `test_msgram.py` and `test_msgram.c` |
| A layout the part would refuse is refused here | 3 of the 4 layout cases, each for a different reason |
| The three frames step 4 sends are legal, and the rest are refused | `test_frame.py` and `test_frame.c` |
| A length the format cannot carry has no code | all 65 lengths, both formats |
| A length that has a code comes back as itself | the 16 round trips |
| The loopback itself and the first frame | **not here yet**, they need the board |

## The length code, which is the other silent one

Chapter 9 step 4 sends three frames to itself: classic, flexible-data without
the rate switch, and flexible-data with it. All three have to be built before
any can be sent, and the part of that provable on a host is the length code.

The controller carries a **four bit code, not a length**. Code 9 is twelve
bytes, and the top codes step 24, 32, 48, 64. Worse, the same code means
different things in the two formats: a classic frame has no length above eight
and reads every code above 8 as 8. A codec that gets this wrong does not fail
loudly, it moves the right bytes with the wrong count or the wrong bytes with
the right one, and the symptom appears in whatever reads the payload.

So `frame_code_for_length` returns a refusal rather than the nearest code.
Rounding up sends bytes the caller never wrote; rounding down drops the tail.
Both are silent, and nine bytes is not a flexible-data length at all.

The full table is in [`doc/frame-rules.md`](doc/frame-rules.md), generated
rather than typed.

## The message memory, and the budget row it corrected

A controller whose message memory has not been laid out accepts every other
configuration, reports no error, and never transmits. It is the single most
common way this peripheral appears broken, so the layout is computed and
asserted rather than written down.

The sections do not share an element size, and that is where the chapter's
budget row went wrong. A standard filter is one word, an extended filter and a
transmit event are two, and a receive or transmit element carrying the full
sixty-four byte payload is eighteen: two of header and sixteen of data. The
chapter's own layout from step 3 therefore occupies **2432 bytes**, not the
"under 2 kB" its budget table claimed until Tuesday 6 October 2026. The sixteen
receive elements of the first queue are 1152 bytes on their own.

Nothing about the design was at fault. Every section is inside the counts the
part accepts, and 2432 of the 10240 bytes leaves 7808 free. The budget row was a
round number nobody had multiplied out, and `--audit-chapter` is gating now so
the two cannot drift apart again.

The element sizes and the element-count maxima are the silicon vendor's own, read
from the driver source rather than from a summary of it.
