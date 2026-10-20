# Reaching the registers, and what a host can still prove about it

Chapter 9, Tuesday 20 October 2026. `src/bsp/fdcan_mmio.c` is the first file in
this volume **below** the board layer, so it is the first one allowed to name an
address. Everything in `src/bus/` is above it and may not, by chapter 4's rule,
which is what let the whole bus layer be checked on a laptop for two weeks
before this file existed.

## It needs no vendor header, and does not have one

`src/bsp/` is the one directory permitted to include a vendor header. This file
includes none, because `planrun.c` asks for a read, a write and a clear, and all
three are volatile accesses at a base plus an offset. Pulling CMSIS in for that
would put a vendor dependency under the only file allowed to have one and not
requiring it.

## Two bases, not one

`planrun.c` passes a **register offset** to read and write, and a **message
memory byte offset** to clear. Those are different windows:

| Window | Address | Size |
|---|---|---|
| FDCAN1 registers | `0x4000A000` | `0x400` |
| FDCAN1 message RAM | `0x4000AC00` | `0x2800`, 10240 bytes |

`[datasheet]` Both from [node-registers.md](node-registers.md), which took them
from Zephyr's device tree for `st,stm32h7-fdcan` and NuttX's
`stm32h7x3xx_memorymap.h`, and the size from ST's HAL `FDCAN_MESSAGE_RAM_SIZE`.

An implementation with one base clears the wrong memory, and every access still
succeeds. The two are `0x800` apart with **another controller instance's window
in between**, which is also why a register offset is range checked rather than
trusted: the window is `0x400`, so an offset of `0x400` or more reaches FDCAN2.
The configuration then appears to take, because it does take, on the wrong
controller.

## Two of the three callbacks cannot refuse

`fdcan_write_fn` and `fdcan_clear_fn` return void, so a range check here has
nowhere to report to. A refusal is recorded as a sticky fault and
`fdcan_mmio_ok` must be asked after a run.

This matters most for the clear, because **the clear is the one step in the whole
plan that cannot fail.** `planrun.c` calls `clear_ram` and records the step as
having worked, unconditionally. Every other step is read back; this one is not.
So a caller that checks only `fdcan_run`'s return value has not checked the
clear, and the chapter's own executor cannot tell it otherwise.

## Which is why the clear proves itself before it clears

Writing zeroes and reading zeroes back is not evidence. A wrong base accepts
every write and reads back zeroes, which is the same picture as a memory that
cleared perfectly. So the clear:

1. writes `0xA5C3F00D` to the first word of the range and its complement to the
   last, two different patterns on purpose, because a window that aliases so
   that every address answers with the same word would pass a single-pattern
   check,
2. reads both back and records a fault if either disagrees,
3. only then zeroes the range, word by word and never with `memset`, which is
   free to use byte and unaligned stores that this memory does not accept,
4. and reads the two ends again afterwards.

That turns the plan's one unfalsifiable step into one that can fail. It is safe
at that point in the sequence because the plan clears the message memory at step
5, before any register points at it.

## What the host run proves, and what it does not

The test runs **the real plan through the real executor through the real
accessors.** The only substitution is where the two bases point, and that goes
through the same argument the board will use rather than through a compile-time
switch.

**The control arm is a block of zeroes,** which is what a wrong base address
looks like from outside: every write accepted, every read returning nothing. The
plan has to stop, and it stops at step 2, the endianness register, the only
register in the sequence whose value the part supplies rather than the software:

```
flat zero array  ->  expect-mismatch | step 2 | endianness, the one register with one answer
                     wrote 0x87654321  read back 0x00000000
seeded array     ->  ok | steps done 23 of 23
```

That pair was run in the Python reference before the C test was written, because
a plain array is **not** a sufficient register model and assuming it was would
have produced a test that fails for the wrong reason. The two values the hardware
supplies, `ENDN` and `CCCR.INIT`, have to be seeded; leaving them out is the
control rather than a defect.

Guard words sit beside both windows, as **members of one struct** rather than as
separate arrays, because C guarantees member order and guarantees nothing about
where separate objects land. As four separate arrays the guards would prove
nothing: an access one word past the end could land in padding and the guards
would still read clean.

**What it does not prove.** Two of the probe's refusals, "does not hold a
pattern" and "aliases", are properties of a bus that misbehaves. Real memory
holds patterns and does not alias, so those paths are unreachable on a host and
stay `[unconfirmed]` until the board runs, exactly like the loopback itself. And
nothing here proves a register answers at all; only the part does that.

## Compiled for the part, not linked into an image

`make cross` compiles every translation unit in `src/bus/` and `src/bsp/` for
`cortex-m7` with `-mfpu=fpv5-d16 -mfloat-abi=hard -mthumb`, under the same
`-Wall -Wextra -Werror` the host build uses, so that a difference between the two
is the architecture and nothing else. It catches the class of mistake that is
silent on a host, chiefly that a plain enum is unsigned on ARM EABI, which makes
a test for a negative enumerator dead code there and a `-Werror` failure with
it. Both the plan and the addressing range check enumerators.

**Objects only.** There is no startup, no linker script and no console in this
volume, so there is no image to link. That floor is chapter 4's and it has no
code tree at all; its README now lists the four files and says so. The honest
claim for this chapter's C today is that it compiles for the part, and that is
what the chapter says.

The cross compiler itself is not what is missing. `arm-none-eabi-gcc` 14.2.1 is
on PATH in WSL on the win11 skyhorizon laptop and was probed in both forms on
Tuesday 20 October 2026: it compiles, it links a bare-metal Cortex-M7 hard-float
object with `-nostdlib`, it links newlib through `--specs=nosys.specs` as well,
and the multilib for this core resolves to `thumb/v7e-m+dp/hard`. Chapter 4's
README carries the figures and the one probe that failed for its own reasons.
