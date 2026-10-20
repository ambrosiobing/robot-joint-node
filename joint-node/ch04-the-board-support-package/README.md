# Chapter 4: the board support package

Mostly not written. What is here is the rule chapter 4 imposes on every chapter
after it, which is the part that has to exist first or it cannot exist at all.

## What runs

```bash
python check_layers.py
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

## And the image floor, which chapter 9 established is missing

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
