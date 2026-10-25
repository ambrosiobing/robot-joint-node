# Chapter 4: the board support package

Mostly not written. What is here is the rule chapter 4 imposes on every chapter
after it, which is the part that has to exist first or it cannot exist at all.

## What runs

```bash
make                        # everything that needs no toolchain
make layers                 # chapter 4's rule over the whole joint-node tree
make pytest                 # the memory map and the startup, by construct
make linkcheck              # needs arm-none-eabi-gcc: links a real image
make table                  # the table in doc/memory-map.md

python check_layers.py      # the rule on its own, which is what CI calls
python tools/gen_memmap.py  # regenerate memory.ld and memmap.h
python test/test_memmap.py  # the map, the script, and their agreement
python test/test_startup.py # the startup's four silent mistakes
python tools/gen_baud.py    # regenerate the baud vectors
python test/test_baud.py    # the divisor, its error, and every refusal reached
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

## The image floor, two of the four pieces built

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

**The startup.** Sixteen core vectors, the `.data` copy, the `.bss` zeroing.
Four of its mistakes have no symptom and three of those produce a board that
does nothing at all, so each is checked by construct and each rule was proved
able to fail: `_sstack` where `_estack` belongs, a copy reading its own
destination, `memcpy` called before the memory it would use exists, and a table
discarded by `--gc-sections`.

`make linkcheck` links a real image and reads its first two words back out of
the `.elf`, which is the only check available without a board.

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

**Still missing: `console.c` itself, and the device interrupt table.** `[vendor]` The
board's own device tree settles what the console is, USART3 on PD8 and PD9 at
115200, so that piece is next. The device vector positions have to be read from
a checked source before anything enables an interrupt, and the checker refuses a
table that is not exactly sixteen entries so that adding them is deliberate.

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
