# Which RCC register map this silicon has

Opened on Thursday 12 November 2026, while sourcing the clock enable bit for a
probe that was meant to read two FDCAN registers. The probe is still worth
building; this question has to be settled first, because the probe needs an
enable and there are two candidate addresses for it.

**This document records a contradiction inside one vendor file, says which side
each piece of evidence favours, and designs the experiment that settles it. It
does not conclude.**

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
