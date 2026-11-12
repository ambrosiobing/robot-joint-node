# Which RCC register map this silicon has

Opened on Thursday 12 November 2026, while sourcing the clock enable bit for a
probe that was meant to read two FDCAN registers. The probe is still worth
building; this question has to be settled first, because the probe needs an
enable and there are two candidate addresses for it.

**This document records a contradiction inside one vendor file, says which side
each piece of evidence favours, and designs the experiment that settles it.**

**The experiment ran on Thursday 12 November 2026 and the question turned out to
be malformed: both candidate addresses reach the same register on this part.**
The contradiction stands as a documentation question and has no consequence for
these three registers. The result, what it did settle, and what it did not, are
in [the result section](#the-result-the-question-was-malformed-and-the-part-said-so)
below. Everything above it was written before the run.

## The contradiction, located field by field

Each field of `RCC_TypeDef` in ST's CMSIS device header carries its own offset
in its own comment (`Address offset: 0xNN`). Parsing the struct, computing each
field's offset from the declarations, and comparing against that comment gives:

| Header | Fields | Agree | Differ |
|---|---|---|---|
| `stm32h743xx.h` | 65 | 65 | **0** |
| `stm32h7a3xx.h` | 67 | 45 | **22** |

`stm32h7a3xxq.h`, the header for the Q variant this board carries, gives the
same layout as `stm32h7a3xx.h`.

The first divergence is at `RESERVED10`, declared `[31]` and carrying a comment
that is a verbatim copy of `RESERVED9`'s, `Reserved, 0xAC-0xAF  Address offset:
0xAC`. Every field after it is displaced by 0x60:

| Register | The struct's layout | ST's own comment | This volume uses |
|---|---|---|---|
| `RSR` | 0x130 | 0x0D0 | not used |
| `AHB3ENR` | 0x134 | 0x0D4 | not used |
| `AHB1ENR` | 0x138 | 0x0D8 | not used |
| `AHB2ENR` | 0x13C | 0x0DC | not used |
| `AHB4ENR` | 0x140 | **0x0E0** | 0x0E0 |
| `APB3ENR` | 0x144 | 0x0E4 | not used |
| `APB1LENR` | 0x148 | **0x0E8** | 0x0E8 |
| `APB1HENR` | 0x14C | **0x0EC** | the probe's question |
| `APB2ENR` | 0x150 | 0x0F0 | not used |

For `RSR` to sit at 0x0D0, `RESERVED10` would have to be `[7]` rather than
`[31]`: `CKGAENR` is at 0x0B0 by both accounts, so seven reserved words reach
0x0D0 and thirty-one reach 0x130.

**Everything below 0x0B4 is undisputed.** `CR` at 0x000 and `CDCCIP1R` at 0x050
are given the same offset by the struct and by the comments, which is what makes
a read-only baseline possible at all.

## Which side the evidence favours, and it is not one-sided

`[measured]` **For the comments, so 0x0E0.** This volume's console uses 0x0E0
and 0x0E8 and it works. `console_init` writes bit 3 at 0x0E0 and bit 18 at
0x0E8, reads both back and refuses if either is clear, then configures GPIOD
with read-modify-write plus a check that no other pin's bits moved, then
transmits. An unclocked GPIO port does not retain writes, and an unclocked USART
does not transmit. 5247 bytes arrived legibly on Thursday 12 November 2026.

`[inferred]` **For the struct, so 0x140.** Zephyr supports `nucleo_h7a3zi_q`
and reaches the RCC through this same CMSIS struct via its LL layer. A struct
displaced by 0x60 ought to make every clock enable on that board fail, and the
board is supported rather than broken.

Both cannot hold. The first is behaviour on this part and the second is an
inference about another project, which is why they are marked differently, but
the inference is not weak and it is not dismissed here.

## What has NOT been ruled out

`[unconfirmed]` That the console's success is explained by its RCC writes at
all. A reserved location may read back what was written, which would satisfy the
two read-back checks without enabling anything, and in that case something else
would have to have clocked GPIOD and USART3. Nothing in the image does, and
peripheral clocks are off after reset on this family, so no candidate has been
found. But "no candidate found" is not "ruled out", and the console's own
evidence is therefore strong rather than conclusive.

This is the reason the question is being put to the part instead of argued
further.

## The experiment

Two images from one source file, differing in exactly one write. Joseph's
decision, and the reason stands on its own: **an address whose meaning for this
silicon is unsettled must not be written to merely because it would have to be
reserved under the competing hypothesis.** That argument assumes the conclusion,
and writing there would change the state of the address under investigation. A
control must not move the subject.

| | Image A | Image B |
|---|---|---|
| Tag | `jn-fdcanprobe-a` | `jn-fdcanprobe-b` |
| Writes | offset 0x0EC only | offset 0x14C only |
| Absolute | `0x580244EC` | `0x5802454C` |
| On the authority of | ST's register comments | ST's `RCC_TypeDef` |

One source file with one `-D`, so the read-only baseline is identical by
construction rather than by maintenance. A build with neither `-D` is a compile
error, and `make fdcanprobe-a` proves that by compiling the file without one and
requiring a non-zero exit before it builds anything, because only a compiler can
establish it.

### Three things, kept apart

A set enable bit does not prove the controller's core has a usable clock, so the
report separates:

**1. The clock tree, read only, at undisputed offsets.** `CR` at 0x000 gives
`HSION` bit 0, `HSIRDY` bit 2, `HSIDIV` bits 4:3, `HSEON` bit 16, `HSERDY`
bit 17. `CDCCIP1R` at 0x050 gives `FDCANSEL` at bits 29:28.

`HSIDIV` is worth reading for its own sake. ST's `system_stm32h7xx.c` computes
the clock as `HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3)`, so this register
states whether the oscillator is undivided **without depending on the
terminal**. That is a second and independent reading of the figure the first
image bounded by legibility alone.

**2. The RCC peripheral enable**, at the disputed offsets. All six addresses,
both candidate triples, are read before and after, and one is written.

**3. FDCAN register access.** `CREL` at 0x000 and `ENDN` at 0x004, read only.
Nothing is written to FDCAN and nothing is configured.

### What each outcome means, written down before the run

`ENDN` reads `0x87654321`: the base address `0x4000A000` is confirmed on silicon
for the first time, from three agreeing sources, and whichever enable that image
wrote was sufficient.

`ENDN` reads `0x00000000`: two live hypotheses, and the report separates them.
Either that enable is not the one gating FDCAN, or the controller's core has no
clock. `[unconfirmed]` The meaning of `FDCANSEL`'s codes is not sourced here;
what is reported is the raw two-bit field together with `HSEON` and `HSERDY`, so
a reader can see whether the selected source is even running.

`ENDN` reads anything else: the window answers and is not this controller.

Reading all six addresses in both images also means a single capture shows which
triple the part implements, rather than only whether that image's own guess took.

## The result: the question was malformed, and the part said so

**Thursday 12 November 2026.** Both images ran. The two captures agree on every
read, which is the control the design wanted, and they agree on something the
design did not anticipate.

    jn-fdcanprobe-a enable before 0x0E0 0x00000008 0x0E8 0x00040000 0x0EC 0x00000000 0x140 0x00000008 0x148 0x00040000 0x14C 0x00000000
    jn-fdcanprobe-a enable after  0x0E0 0x00000008 0x0E8 0x00040000 0x0EC 0x00000100 0x140 0x00000008 0x148 0x00040000 0x14C 0x00000100

    jn-fdcanprobe-b enable before 0x0E0 0x00000008 0x0E8 0x00040000 0x0EC 0x00000000 0x140 0x00000008 0x148 0x00040000 0x14C 0x00000000
    jn-fdcanprobe-b enable after  0x0E0 0x00000008 0x0E8 0x00040000 0x0EC 0x00000100 0x140 0x00000008 0x148 0x00040000 0x14C 0x00000100

`[measured]` **THE TWO CANDIDATE ADDRESSES REACH THE SAME REGISTER.** Three
pairs, each exactly 0x60 apart, read and write identically:

| Pair | Distance | Before | After A wrote 0x0EC | After B wrote 0x14C |
|---|---|---|---|---|
| 0x0E0 and 0x140 | 0x60 | both `0x00000008` | unchanged | unchanged |
| 0x0E8 and 0x148 | 0x60 | both `0x00040000` | unchanged | unchanged |
| 0x0EC and 0x14C | 0x60 | both `0x00000000` | **both** `0x00000100` | **both** `0x00000100` |

Image A wrote 0x0EC and bit 8 appeared at 0x14C as well. Image B wrote 0x14C
and bit 8 appeared at 0x0EC as well. Neither image wrote the other address, so
this is the hardware and not the software.

The first two rows say the same thing about the console, which had written only
0x0E0 and 0x0E8: `0x00000008` is bit 3 at both ends of the first pair, and
`0x00040000` is bit 18 at both ends of the second.

**So the experiment cannot distinguish the two documented layouts, because on
this part they are not two layouts.** The contradiction in the header is real
and remains open AS A DOCUMENTATION QUESTION; it has no consequence for these
three registers on this silicon. That is a better outcome than either hypothesis
winning, and it is not the outcome the design was built to produce.

It also answers the uncomfortable version of the question. The console has been
enabling its clocks correctly all along, whichever half of that file one reads.

### What has NOT been established

`[unconfirmed]` **The mechanism.** One observation is not a mechanism. Three
pairs 0x60 apart behave as one register each; that is a fact about three pairs,
not a demonstrated mirroring of the whole block. A reader with a partial address
decode in mind and a reader with a 0x60 periodic alias in mind are both
consistent with this capture.

It is NOT a bus that returns the previously read word, and that is worth saying
because it is the cheap explanation. The reads happen in the order 0x0E0, 0x0E8,
0x0EC, 0x140, 0x148, 0x14C. If an unimplemented address returned the last value
on the bus, 0x140 would have returned 0x0EC's `0x00000000`. It returned
`0x00000008`, which is 0x0E0's value, so the address is mapped rather than
ignored.

`[unconfirmed]` **Whether the enable was necessary at all.** This is a gap in
the design and it is mine: the probe reads `CREL` and `ENDN` only AFTER the
write, so it shows that FDCAN answers once the bit is set and not that the bit
is what made it answer.

### The next experiment, built on Thursday 12 November 2026

`src/app/addrmap.c`, and it is written. One image, read only first, then one
write:

1. Read `CREL` and `ENDN` BEFORE any enable is written. That closes the gap
   above: if they already answer, the enable is not what grants access.
2. Read further pairs 0x60 apart to test whether the mirroring is periodic or
   confined to these three: 0x000 against 0x060, 0x050 against 0x0B0, 0x0B0
   against 0x110, and 0x14C against 0x1AC. All of these are reads.
3. Only then write the enable, and read everything again.

Reading 0x000 against 0x060 is the sharpest of those, because `CR` is at 0x000
and its value is known from this capture, so a mirror at 0x060 would be
unmistakable.

**It reads five pairs, and 0x0E0 against 0x140 is among them as the KNOWN
POSITIVE.** A capture in which the pair that already mirrored stops mirroring is
a capture to distrust before reading anything else in it.

**And a hazard the previous probe did not face.** This one reads addresses whose
meaning is unknown. `[inferred]` All of them lie inside base plus 0x3FF, the one
kilobyte slot this peripheral is allocated, so a read is expected to return zero
rather than to fault; that is an inference and not a source. If it is wrong, a
bus error escalates to a hard fault, the part stops, and the capture simply
ENDS.

A line assembled in memory and printed at the end prints nothing when the access
halfway through it never returns. So **every offset is printed AND FLUSHED
before the read that follows it**, which costs output and makes the last line of
a truncated capture the culprit. `test_addrmap.py` checks that the flush precedes
the dereference, collects every offset from the source and compares it against
the bound, and refuses any dereference outside the one announcing function.

`HardFault_Handler` and `BusFault_Handler` are overridden in that file, which is
the first use in this volume of the weak aliases the generated vector table was
built to allow: a strong definition under the vendor's own name wins. They print
the last announced offset and stop. `[unconfirmed]` Printing from a fault
handler is not generally sound, and it is acceptable here only because the one
fault this probe can plausibly raise is a bus error on a read, which leaves the
console untouched.

So a capture that ends mid-list is a result rather than a mishap.

## What the capture did settle

`[measured]` **The FDCAN1 base address, on silicon, for the first time in this
volume.**

    jn-fdcanprobe-a fdcan1 crel 0x32141218 endn 0x87654321 endn-expected 0x87654321 verdict base-confirmed
    jn-fdcanprobe-b fdcan1 crel 0x32141218 endn 0x87654321 endn-expected 0x87654321 verdict base-confirmed

`ENDN` reads `0x87654321`, which is the value chapter 9's plan predicts and the
only value in the whole configuration sequence that the part supplies rather
than the software. Three vendor sources agreed on `0x4000A000`; the part now
agrees with them.

`[measured]` **The controller's core release.** `CREL` is `0x32141218`, which
decodes by the field layout the mainline M_CAN driver uses:

| Field | Bits | Value |
|---|---|---|
| `REL` | 31:28 | 3 |
| `STEP` | 27:24 | 2 |
| `SUBSTEP` | 23:20 | 1 |
| `YEAR` | 19:16 | 4 |
| `MON` | 15:8 | `0x12`, BCD, so 12 |
| `DAY` | 7:0 | `0x18`, BCD, so 18 |

**M_CAN core release 3.2.1, dated 18 December 2014.** That is one of the four
questions this volume has been carrying since chapter 9 was written.
`[unconfirmed]` Which of the mainline driver's version dependent paths that
selects has not been read out of the driver yet, and is chapter 9's to source.

`[measured]` **The clock tree, from the registers rather than from legibility.**

    jn-fdcanprobe-a clocktree cr value 0x0000C025 hsion 1 hsirdy 1 hsidiv 0 hseon 0 hserdy 0
    jn-fdcanprobe-a clocktree cdccip1r value 0x00000000 fdcansel 0

`HSIDIV` reads 0, so the internal oscillator is undivided. ST's
`system_stm32h7xx.c` computes the clock as
`HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3)`, so this is a reading of the
64 MHz figure **from the part's own register**, independent of the terminal.
Part seven bounded that figure between 62.77 and 65.33 MHz by legibility alone;
this is a second and different kind of evidence for the same number.

`[measured]` **And the host interface answers without a core clock.** `HSEON`
and `HSERDY` are both 0, so the external oscillator is not running, and
`FDCANSEL` is 0 with `CDCCIP1R` entirely zero. `CREL` and `ENDN` answered
anyway. So reading the M_CAN host registers needs the peripheral bus clock and
nothing else, which means chapter 9's step 2 endianness check can run before any
kernel clock has been arranged.

`[unconfirmed]` What `FDCANSEL` 0 actually selects is still not sourced here. It
does not matter for the two reads above, and it will matter for anything that
makes the controller transmit.

## Why `ENDN` and not some other register

`[chapter]` It is step 2 of chapter 9's configuration plan, and the plan puts it
there for this reason: **it is the only register in the whole sequence whose
value the part supplies rather than the software.** A wrong base address accepts
every write and reads back zeroes, which is indistinguishable from a correct
base and an unconfigured controller. `ENDN` has one correct answer, so it is the
only falsifiable check on the address available before anything is configured.

`CREL` is read beside it because it names the M_CAN core release, which decides
which of the mainline driver's assumptions apply to this part, and because a
second non-zero register makes a reads-zero result harder to misread.

## Provenance

`[vendor]` `FDCANEN` is bit 8 of `APB1HENR`, from three independent sources:
Zephyr's `clocks = <&rcc STM32_CLOCK(APB1_2, 8)>` on `can@4000a000`, ST's
`RCC_APB1HENR_FDCANEN_Pos` which is 8, and the generated STM32H7 register
documentation.

`[vendor]` `FDCANSEL` is bits 29:28 of `CDCCIP1R`, from ST's
`RCC_CDCCIP1R_FDCANSEL_Pos` which is 28 with a two-bit mask, and Zephyr's
`FDCAN_SEL(val)` which names bits 29 and 28 of the same register.

`[vendor]` The `RCC_CR` bit positions above, from ST's own `_Pos` definitions.

`[vendor]` FDCAN1 registers at `0x4000A000` for `0x400` bytes, from Zephyr's
`reg = <0x4000a000 0x400>, <0x4000ac00 0x350>`, corroborated in chapter 9's
[`node-registers.md`](../ch09-can-fd-from-the-controller-out/doc/node-registers.md)
by NuttX's memory map and ST's HAL.

`[chapter]` `CREL` at 0x000, `ENDN` at 0x004 and the expected `0x87654321`, from
chapter 9's `tools/gen_initplan.py`. `test/test_fdcanprobe.py` imports that
generator and compares, rather than keeping a second copy, so the two chapters
cannot drift apart about the register map.

`[vendor]` The RCC base `0x58024400`, already in `src/bsp/console.h`.
