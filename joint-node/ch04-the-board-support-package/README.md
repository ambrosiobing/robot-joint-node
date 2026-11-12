# Chapter 4: the board support package

Mostly not written. What is here is the rule chapter 4 imposes on every chapter
after it, which is the part that has to exist first or it cannot exist at all.

## What runs

```bash
make                        # everything that needs no toolchain
make layers                 # chapter 4's rule over the whole joint-node tree
make pytest                 # the map, the startup, the vectors, by construct
make linkcheck              # needs arm-none-eabi-gcc: links a real image
make firstlight             # needs arm-none-eabi-gcc: THE IMAGE THAT PRINTS
make fdcanprobe-a           # the register map probe, writing ONLY 0x0EC
make fdcanprobe-b           # the same probe, writing ONLY 0x14C
make table                  # the tables in doc/memory-map.md, doc/console.md
                            # and doc/interrupts.md

python check_layers.py      # the rule on its own, which is what CI calls
python tools/gen_memmap.py  # regenerate memory.ld and memmap.h
python tools/gen_baud.py    # regenerate the baud vectors
python tools/check_image.py build/image.elf   # a linked image's first two words
python test/test_memmap.py  # the map, the script, and their agreement
python test/test_startup.py # src/bsp/startup.c's four silent mistakes
python tools/gen_vectors.py # regenerate src/bsp/vectors.c and vectors.h
python test/test_vectors.py # the 171 vectors, the offset, the reserved holes
python tools/gen_stamp.py --print            # the build stamp, as it will print
python test/test_firstlight.py               # the banner, the tag, the stamp
python test/test_fdcanprobe.py               # one write, and the boundary round it
python tools/gen_baud.py    # regenerate the baud vectors
python test/test_baud.py    # the divisor, its error, and every refusal reached
python test/test_console.py # the console's five silent mistakes, by construct
```

Nothing above the board layer may include a vendor header. The checker walks the
C in this repository, skips `src/bsp/`, and fails with the file and the include
when anything else names the silicon.

## Why this one check came before the rest of the chapter

The moment a control loop includes a peripheral header, the control loop is tied
to one silicon vendor and cannot be built on a host machine. Chapter 20 has to
build most of this node on a host with no hardware at all, and that is only
possible if the rule was kept from here onward rather than retrofitted at the
end. Retrofitting it means untangling every include in the tree at the point
where the deadline is closest.

So the rule ships before the thing it protects. It was confirmed to fail by
planting an include that breaks it.

It stopped passing trivially on Tuesday 20 October 2026, when the board layer
got its first inhabitant. The count now reads 53 C files with 51 above the board
layer, where for weeks it read "all of them above", and the two below are
chapter 9's `src/bsp/fdcan_mmio.c` and its header: the volatile accesses that
point that chapter's configuration executor at a real address. They arrived from
chapter 9 rather than from here, which is the right way round. The rule exists so
that the layer below can be written late and small, and the whole bus layer above
it was checked on a laptop for two weeks before it existed.

Worth noting what that file did NOT need: a vendor header. It is the one
directory allowed to include one, and reaching a register turned out to be a base
address, an offset and a volatile access, so pulling CMSIS in would have put a
vendor dependency under the only file permitted to have one and not requiring it.

## What chapter 4 still owes

Its actual deliverable is a board file another engineer can read, and none of it
exists:

- the pin map, as a description rather than as comments scattered through code
- the clock configuration, with the arithmetic that produces it
- the peripheral table: which peripheral, which mode, which clock, which pins,
  which interrupts and transfers

That file is the input to most later chapters, so it is the highest-value thing
missing from this tree. It is not started, and saying it is started would make
every chapter that cites it look further along than it is.

## The image floor, four of the four pieces built

Started Tuesday 20 October 2026, after chapter 9 established what was missing.

**The memory map and the linker script.** The `MEMORY` block is generated from
the map in `tools/gen_memmap.py`, because a region size typed by hand links
perfectly well and overruns at run time. Three of the nine regions are declared
to the linker and six are recorded and not, and the difference is evidence:
declaring a region is a promise that sections put there will be present at run
time, and recording one is only a fact about the part.

The case that matters is the AXI SRAM, which `[vendor]` Zephyr splits into three
nodes whose addresses tile into `[arithmetic]` one megabyte with no gap. Only
the first is declared, because the board's own device tree chooses that one and
says nothing about whether the other two need a clock or power enable. An image
that declared 1024 KB and got 256 KB would link, boot, and fail the first time
it touched the upper three quarters.

That field started as a boolean and a mutation flipping one character from
`False` to `True` passed every rule. It is a reason now, and a rule requires the
reason to carry a provenance marker, so declaring the 384 KB nodes costs a
sentence somebody has to stand behind.

**The startup, `src/bsp/startup.c`.** The `.data` copy and the `.bss`
zeroing. The vector table itself left this file on Monday 9 November 2026; see
below.
Four of its mistakes have no symptom and three of those produce a board that
does nothing at all, so each is checked by construct and each rule was proved
able to fail: `_sstack` where `_estack` belongs, a copy reading its own
destination, `memcpy` called before the memory it would use exists, and a table
discarded by `--gc-sections`.

`make linkcheck` links a real image and `tools/check_image.py` reads its first
two words back out of the `.elf`, which is the only check available without a
board.

The first link failed, and for a reason worth keeping: **GNU ld does not
concatenate adjacent string literals.** One of the four assertion messages was
wrapped the way C wraps a long string, which is a syntax error in a linker
script, and the only symptom was a line number pointing at the second half.
`test_startup.py` now counts the quotes inside every `ASSERT` and refuses more
than two, because the link is the one check this laptop cannot run.

The full account is in [`doc/memory-map.md`](doc/memory-map.md).

**The console's arithmetic, Sunday 25 October 2026.** The divisor has the same
shape as chapter 9's bit timing and fails the same way: wrong by a lot gives
obvious rubbish, wrong by two or three per cent gives output that is mostly
readable, and the natural reading of that is a loose wire rather than
arithmetic. So it is solved and checked the same way, in `tools/gen_baud.py`
with [`doc/console.md`](doc/console.md) carrying the register map and the table.

`[vendor]` The board's own device tree settles the console: USART3, PD8
transmit, PD9 receive, 115200. `[unconfirmed]` Its kernel clock does not, so the
table solves every plausible rate rather than picking one, and the first image
will print the clock it assumed next to the divisor it computed. A console
cannot check its own baud: it writes into a transmitter and never hears back, so
the terminal is the instrument and the printed numbers are what make legible
output mean something.

One rule in that file is about the rules. Five refusal reasons are written in
the solver and when they were first checked only three were reachable: the case
meant to trip the error limit was refused by the divisor floor first, and the
prescaler loop's exhaustion path had no case at all. A refusal nothing reaches
is a claim rather than a behaviour, so there is now a rule requiring each one to
be reached, and removing either new case makes that rule fail.

**The console itself, Friday 6 November 2026.** `src/bsp/console.c` and
`src/bsp/baud.c`. The arithmetic is separate so it can run on a host against the
same vectors the generator wrote; the console is nothing but registers, so it is
checked by construct instead, and each of its five quiet mistakes has a rule
proved able to fail.

Two of those rules are about ORDER, which no amount of reading the registers
afterwards would reveal: the status flags must be cleared BEFORE the byte, or a
transmission-complete flag left by the previous character makes a later flush
return before this byte has gone; and `CR1` must be cleared before `BRR`, or the
divisor is written to a running peripheral.

The sharpest rule guards somebody else's work. `MODER`, `OSPEEDR` and `PUPDR`
are one word for all sixteen pins of port D, and two of them are chapter 9's
bus, so every write to those registers must derive from the register's own prior
value and `init` refuses if any non-owned bit moved. A console that worked while
quietly returning `PD0` and `PD1` to inputs would have chapter 9 debugging a bus
this file broke.

**The device interrupt table, `tools/gen_vectors.py` and the generated
`src/bsp/vectors.c`.** Read on Monday 9 November 2026, which is what the
sixteen entry rule in `test_startup.py` was waiting for: all 171 entries, the 16
core exceptions and the 155 device positions, `[vendor]` from ST's own
`IRQn_Type` enum and corroborated against Zephyr's device tree for the four
positions this volume uses. The full reasoning and the table are in
[`doc/interrupts.md`](doc/interrupts.md).

Four things about that table fail silently, three of them checked by
`test/test_vectors.py` from the text and the fourth only by a compiler:

- **The device positions start at vector 16.** Off by one puts every handler one
  slot out, the image links, and nothing shows it until an interrupt fires. The
  rule checks four absolute vector numbers written out rather than computed,
  because a loop built from the same constant as the table agrees with itself.
- **21 of the 155 device positions are reserved holes, not a tail.** The first
  is 61. A table built from the list of named interrupts is 21 entries short and
  wrong above that point. The console's USART3 at 39 is below the first hole,
  which is the worst version of this: the peripheral brought up first is the one
  that would not reveal it.
- **The alignment depends on the length.** 171 entries is 684 bytes and needs
  1024 byte alignment; the sixteen entry table needed 128. The script said
  `ALIGN(512)`, correct for 64 bytes and too small for 684. A linker script
  cannot read a C header, so `test_startup.py` reads both numbers and refuses a
  mismatch.

The fourth is the one the text could not reach. **An alias target must be
defined in the same translation unit.** Every device handler is a weak alias so
that a chapter can override it by defining its own, and the first version aliased
all 134 to `Default_Handler_alias`, which `startup.c` defines and `vectors.c`
only declared. GCC resolves an alias within the file, so that is not a link time
question but `aliased to undefined symbol` at compile time, 134 times, after
every text based rule about those aliases had passed. `vectors.c` now defines a
local `Vector_Default_Handler` that calls through, and the rule checks for a
definition rather than a declaration. Checking that an alias is weak and names
the right target is the form; whether the target is in the same translation unit
is the fact, and `make linkcheck` is the only thing here that can tell.

It also gates a claim this chapter already made. `BOARD_HAS_ETHERNET` is 0 and
the proof cited is the vector table; positions 61 and 62 are reserved here and
are `ETH` and `ETH_WKUP` on the STM32H743, so the generator refuses if either
ever acquires a name.

## The image that prints, `src/app/first_light.c`

Built on Thursday 12 November 2026, once the floor under it was complete. It is
the first thing in this volume whose correctness cannot be seen from the host at
all.

**What it is for is one readable line.** That is a smaller claim than it sounds
and a larger one than anything here has made before, because readable text at
115200 is the first evidence that the assumed clock, the computed divisor, the
pin configuration, the startup's copy and zero, and the vector table's placement
are all simultaneously right. Not one of them can be confirmed from a laptop.

**It configures no clock tree, deliberately.** `[vendor]` The part comes out of
reset on its internal oscillator at 64 MHz: ST's own `system_stm32h7xx.c`
initialises `SystemCoreClock` and `SystemD2Clock` to `64000000` before any
configuration runs, and Zephyr's device tree for this family says
`hsi-div = <1>` with the comment `HSI RC: 64MHz, hsi_clk = 64MHz`. That is the
figure the console was already written to assume, so an image can print before
the clock configuration exists, and the clock configuration is a chapter 4
deliverable that is not started.

`[unconfirmed]` What neither vendor settles is which clock USART3's kernel input
actually takes after reset, because that depends on prescalers nothing here has
read back. **So readable text is the measurement.** A console cannot check its
own baud and the terminal is the instrument: `console_report` is printed FIRST
and carries the assumed figure, so a garbled capture is still evidence, provided
what was being tested was written down before it was tested.

### The banner has to name the image, because the board is shared

Two volumes work the same NUCLEO-H7A3ZI-Q, so whatever was flashed last is what
is on the part. Elsewhere on this bench a skipped flash printed a banner
identical to the expected one, character for character, twice in one evening,
because the banner carried only a name.

So the first line carries a tag no other project prints, `jn-firstlight`, and
`tools/gen_stamp.py` generates a build stamp into a translation unit that is
rewritten at every build:

    built Thursday 12 November 2026, 13:47:48 from ce3681e78085 tree 2 untracked

**Generated rather than `__DATE__`,** because those macros are fixed when their
own translation unit is compiled and not when the image is linked, so a build
whose only edit is elsewhere keeps an old stamp. That has reported a good image
as stale three times on this bench, and a guard with false alarms is worse than
no guard because what it throws away is real.

**And the dirty marker is computed rather than taken from `git describe
--dirty`,** which reads tracked modifications against the index and calls a tree
holding a brand new file clean. Every file in a new chapter starts untracked,
which is exactly when it matters. `test/test_firstlight.py` builds that state on
purpose and watches it detected, because a guard whose failing state nobody has
constructed is a guard nobody has seen work.

The heartbeat then prints `jn-firstlight tick N bytes M worst-spins W verdict V`,
with field names rather than bare numbers so a capture found later is
attributable, and with a monotonic count so a foreign reflash shows up as the
banner reappearing and the count restarting.

### It printed, and that answered the chapter's open question

**Thursday 12 November 2026.** The capture:

    # jn-firstlight ch04: the first image in this volume that prints
    # built Thursday 12 November 2026, 13:58:55 from 97f5241af2dd tree clean
    console: assumed kernel clock 64000000 Hz, presc 1, usartdiv 556,
             BRR 0x0000022C, rate 115108, error 1 per thousand
    jn-firstlight tick 1 bytes 459 worst-spins 457 verdict ok
    ...
    jn-firstlight tick 78 bytes 5213 worst-spins 1002 verdict ok

The stamp on the console is the stamp `make firstlight` printed, character for
character, so the capture is attributable to that build and not to whatever the
other volume last flashed. That is the whole reason the stamp exists.

`[measured]` **The kernel clock is bounded, and the prescaler chain is 1:1.**
5247 bytes arrived legibly at 115200 8N1 with the divisor at 556 and the
prescaler at 1, so the actual rate is `F_kernel / 556` and the receiver decoded
it. At a conservative two per cent receiver tolerance that puts `F_kernel`
between 62.77 and 65.33 MHz; even at the theoretical limit, half a bit time of
accumulated error at the stop bit of a ten bit frame, between 60.68 and
67.42 MHz. **32 MHz and 128 MHz are each excluded by about a factor of two**,
which was the question: whether a divider or multiplier sat between the 64 MHz
oscillator and this peripheral. The full reasoning is in
[`doc/console.md`](doc/console.md).

What is NOT established is that the clock is exactly 64.000 MHz, or anything
about the oscillator's trim. A ratio was bounded; no frequency was measured.

`[measured]` One figure came free. The transmit wait never exceeded 1002 reads
against a bound of 100000, steady from the third line on, so that bound carries
about a hundredfold margin at this clock. It was chosen by argument and is now
confirmed by a run.

## Which RCC register map this silicon has, `src/app/fdcan_probe.c`

**Opened on Thursday 12 November 2026 while sourcing one bit.** The probe was
meant to read two FDCAN registers; it needs a clock enable, and there turn out
to be two candidate addresses for it, because **ST's own header contradicts
itself.**

Parsing `RCC_TypeDef` and comparing each field's computed offset against the
`Address offset:` in that field's own comment:

| Header | Fields | Agree | Differ |
|---|---|---|---|
| `stm32h743xx.h` | 65 | 65 | **0** |
| `stm32h7a3xx.h` | 67 | 45 | **22** |

The first divergence is `RESERVED10`, declared `[31]` with a comment copied
verbatim from `RESERVED9`. Everything after it is displaced by 0x60, including
`AHB4ENR` (struct 0x140, comment 0x0E0), `APB1LENR` (0x148 against 0x0E8) and
`APB1HENR` (0x14C against 0x0EC).

`[measured]` **The console already uses the comments' offsets and works**, which
is behaviour on this part. `[inferred]` **Against that**, Zephyr supports this
board through that same struct, and a struct displaced by 0x60 ought to break
every clock enable on it. Both cannot hold, and arguing further is the error
this volume exists to avoid, so the part decides.

**Two images, one source file, one `-D` each, and exactly one write.** An
address whose meaning for this silicon is unsettled must not be written to
merely because it would have to be reserved under the competing hypothesis: that
assumes the conclusion, and writing there moves the subject. Image A writes only
offset 0x0EC, image B only 0x14C, and neither touches the other. The read-only
baseline is identical by construction because it is the same translation unit.

The report keeps three things apart, because a set enable bit does not prove the
controller's core has a usable clock: the clock tree at undisputed offsets
including `HSIDIV`, which states whether the oscillator is undivided **without
depending on the terminal**; the peripheral enable at the disputed offsets, all
six addresses read before and after; and the two FDCAN reads.

The contradiction, the evidence on both sides, what has not been ruled out, and
what each outcome will mean are in [`doc/register-map.md`](doc/register-map.md),
written before the run.

`test/test_fdcanprobe.py` imports chapter 9's own generator and compares `CREL`,
`ENDN` and the expected `0x87654321` against it rather than keeping a second
copy, so the two chapters cannot drift apart about the register map. Five
mutations were run against its rules and one survived: checking that `#error`
appears in the file is the form, and whether a build with neither `-D` actually
fails is the fact. The form check now tests the condition, and the fact is
checked by the Makefile, which compiles the file without a variant and requires
a non-zero exit before building anything.

### What the probe settled

`[measured]` **The FDCAN1 base address, on silicon, for the first time in this
volume.** `ENDN` reads `0x87654321`, the one value in chapter 9's whole
configuration sequence that the part supplies rather than the software. Three
vendor sources agreed on `0x4000A000`; the part now agrees with them.

`[measured]` **`CREL` is `0x32141218`: M_CAN core release 3.2.1, dated
18 December 2014.** One of the four board questions, answered.

`[measured]` **`HSIDIV` reads 0**, so the oscillator is undivided, read from the
part's own register rather than inferred from legible text. Part seven bounded
that figure by legibility alone; this is a different kind of evidence for the
same number.

`[measured]` **The host interface answers with no core clock.** `HSEON` and
`HSERDY` are both 0 and `CDCCIP1R` is entirely zero, yet `CREL` and `ENDN`
answered, so reading them needs the peripheral bus clock and nothing else.
Chapter 9's step 2 endianness check can therefore run before any kernel clock is
arranged.

**And the question the probe was built for was malformed.** Both candidate
offsets reach the same register: three pairs exactly 0x60 apart read and write
identically, in both images, and neither image wrote the other address. The
header's contradiction stands as a documentation question and has no consequence
for these three registers. `[unconfirmed]` The mechanism is not established, and
one observation is not a mechanism; the next experiment and the design gap it
closes are named in [`doc/register-map.md`](doc/register-map.md).

**Still missing: three board questions.** The stack high-water mark, whether one
`RXF0A` write releases a whole run of elements, and the loopback. The last two
need a configured controller.

## What chapter 9 established was missing, for the record

Found on Tuesday 20 October 2026 while trying to work out what chapter 9's first
image still needed. **Nothing in this volume has ever produced a firmware image,
and four things are absent, all of them chapter 4's rather than chapter 9's:**

| Missing | Why it is this chapter's |
|---|---|
| a linker script for the STM32H7A3ZI | the memory map is a board fact, and every chapter's image would use the same one |
| a startup: vector table, stack pointer, `.data` copy, `.bss` zero | the same for every image, and the vector table is a table of board facts |
| a console | every chapter from 9 onward reports its results through one, and chapter 9's first image has nothing to print through |
| a cross build that links rather than compiles | `make cross` in chapter 9 compiles its own translation units for the part, and stops there because there is nothing to link them against |

A cross compiler is available, and it was probed in both forms on
Tuesday 20 October 2026. `arm-none-eabi-gcc` 14.2.1 is on PATH in WSL on the
win11 skyhorizon laptop, under `-mcpu=cortex-m7 -mfpu=fpv5-d16
-mfloat-abi=hard -mthumb`:

| Probe | Result |
|---|---|
| compile | OK |
| link `-nostdlib` | OK |
| link `--specs=nosys.specs`, so newlib | OK, 4916 text, 1364 data, 808 bss |
| `-print-multi-directory` | `thumb/v7e-m+dp/hard`, double precision, right for this core |

**Newlib links, which decides how the console gets written.** It can be a
`_write` retarget onto newlib's `printf` rather than a bespoke string writer, and
the choice is now an argument about code size rather than about what is
installed. Nothing needs apt.

The first newlib probe FAILED, and that was the probe's fault rather than the
toolchain's: it named its entry point `Reset_Handler` and defined no `main`, so
`crt0.S` could not resolve `main`. Read as a missing package that is the wrong
conclusion, because the linker had already found `libc.a` and `crt0.o` under
`lib/thumb/v7e-m+dp/hard/`, which is proof the library is there. A newlib probe
has to define `main`.

So the toolchain is not what is missing. These four files are.

Until they exist, every chapter's claim about its C stops at "compiles for the
part", and chapter 9 says exactly that rather than implying more.
