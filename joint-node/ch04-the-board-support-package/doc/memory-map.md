# The memory map, the linker script, and what makes a region declarable

Chapter 4's image floor, first piece, Tuesday 20 October 2026. Chapter 9
established that nothing in this volume had ever produced a firmware image and
that four things were missing: a linker script, a startup, a console and a cross
build that links. This page is the first two.

## A linker script with a wrong region size links

That is the whole reason the `MEMORY` block is generated rather than typed. A
RAM region declared larger than the part has links without complaint, reports
its size happily, and overruns at run time into whatever is past the end. One
declared smaller links too, and silently wastes the rest. Neither is a build
error and neither announces itself at boot.

So the regions are data in `tools/gen_memmap.py`, each with the file it was read
from, and `src/bsp/memory.ld` is produced from them. A hand edit of the block
fails `--check`.

## Declaring a region is a different claim from recording one

A region recorded here is a fact about the part. A region **declared to the
linker** is a promise that an image may put sections in it and they will be
there at run time. The second needs evidence, and three of the nine have it.

The case that matters is the AXI SRAM. `[vendor]` Zephyr splits it into three
nodes whose addresses tile exactly: 256 KB at `0x24000000`, then 384 KB at
`0x24040000`, then 384 KB at `0x240A0000`. `[arithmetic]` That is one megabyte
with no gap, which this chapter's own checker proves rather than asserts.

**They are not declared as one.** Whether the second and third are reachable
without a clock or power enable is not something those files say, and an image
that declares 1024 KB and gets 256 KB of it links, boots, and fails the first
time it touches the upper three quarters. `[vendor]` Zephyr's board file for
this exact board chooses `zephyr,sram = &sram0`, the 256 KB node alone, which is
evidence about that node and no evidence at all about the other two.

### The boolean that certified itself

The first version of the table used a `used` boolean, and a mutation flipping
one character from `False` to `True` passed every rule in the checker. Once a
region is marked usable, the rule forbidding the declaration of unusable
regions stops applying to it. One character, no evidence, no complaint.

So the field is a **reason** now, empty meaning not declared, and a rule
requires every declared region's reason to carry a provenance marker. Declaring
the 384 KB nodes costs a sentence saying what makes them reachable. The control:
a reason reading "it is probably fine" is refused by name, and declaring the AXI
SRAM as one 1024 KB region is refused by the overlap rule.

| Region | Origin | Size | Declared, and why | What it is | Source |
|---|---|---|---|---|---|
| `ITCM` | `0x00000000` | 64 KB | no | instruction tightly coupled memory, and the boot alias lives here too | [vendor] Zephyr dts/arm/st/h7/stm32h7.dtsi |
| `FLASH` | `0x08000000` | 2048 KB | **yes**: [vendor] the part boots from it, so it is reachable before any clock is configured at all | program memory, and where the reset vector is fetched from | [vendor] Zephyr dts/arm/st/h7/stm32h7a3Xi.dtsi |
| `DTCM` | `0x20000000` | 128 KB | **yes**: [vendor] tightly coupled to the core rather than on a bus, so it needs no peripheral clock; Zephyr's board file chooses it for the DTCM region | data tightly coupled memory, no cache and no DMA reach | [vendor] Zephyr dts/arm/st/h7/stm32h7.dtsi |
| `AXISRAM` | `0x24000000` | 256 KB | **yes**: [vendor] Zephyr's board file for this exact board chooses this node as zephyr,sram, which is evidence it needs nothing extra enabled | the AXI SRAM node this board's own device tree chooses for the kernel | [vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi, and the board's chosen node |
| `AXISRAM2` | `0x24040000` | 384 KB | no | contiguous with AXISRAM, and NOT USED: see the open question | [vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi |
| `AXISRAM3` | `0x240A0000` | 384 KB | no | contiguous with AXISRAM2, and NOT USED: see the open question | [vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi |
| `SRAM_D2A` | `0x30000000` | 64 KB | no | a different domain, so it has its own clock enable | [vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi |
| `SRAM_D2B` | `0x30010000` | 64 KB | no | contiguous with SRAM_D2A, same domain | [vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi |
| `SRAM_D3` | `0x38000000` | 32 KB | no | the always-on domain, which is what makes it useful for a fault record | [vendor] Zephyr dts/arm/st/h7/stm32h7a3.dtsi |

`[arithmetic]` Runs whose addresses tile exactly, derived here rather than read anywhere:

- AXISRAM, AXISRAM2, AXISRAM3: `0x24000000` for 1024 KB with no gap
- SRAM_D2A, SRAM_D2B: `0x30000000` for 128 KB with no gap

Declared to the linker: 2432 KB over 3 regions, of 3424 KB the part has.

## The startup, and four mistakes with no symptom

Three of the four produce a board that does nothing at all, which is the least
informative failure available. `test/test_startup.py` checks each by construct,
and each rule was proved able to fail.

| Mistake | What happens | Caught by |
|---|---|---|
| `_sstack` in the first word instead of `_estack` | the stack grows down, so the pointer must start at the END of the region; at the bottom the first push underflows into `.bss` and corrupts variables unrelated to the code running | the first vector entry is read and compared |
| the `.data` copy reading `_sdata` instead of `_sidata` | `_sidata` is where the values are stored in flash and `_sdata` is where they go; a loop from `_sdata` to `_sdata` moves nothing and every initialised variable holds whatever the RAM came up with | the copy's source expression |
| `memcpy` or `memset` for the two loops | a library whose own state lives in `.data` or `.bss` is asked to run before that memory exists | any library call in `Reset_Handler` before `main` |
| the table discarded | nothing references it, so `--gc-sections` removes it unless the script `KEEP`s it, and the image still links and reports a sensible size | `used` in the source and `KEEP` in the script, both, because they are different passes |

`[unconfirmed]` **Only the sixteen core vectors exist.** The device interrupt
positions for this part are a list of numbers that has to be read from a checked
source and has not been. Nothing in this volume enables a device interrupt yet,
so a short table is harmless today and stops being harmless the moment one is
enabled: the core fetches the handler from past the end of the table and
branches into the next words of flash. The checker refuses a table that is not
exactly sixteen entries, so adding device vectors is a deliberate act that comes
with reading their positions.

`[unconfirmed]` **The 8 KB stack is a decision, not a measurement.** The number
that would settle it is a high-water mark read back from a painted stack on the
board, which needs a console, which is the piece after this one.

## GNU ld does not concatenate adjacent string literals

Found by the first link, Tuesday 20 October 2026, and it cost a round trip to
WSL. The script's four assertions carry messages, and the first one's message
was wrapped the way C wraps a long string:

```
ASSERT(_sisr_vector == _boot_address,
       "the vector table is not at the boot address, so the part will fetch its "
       "stack pointer and reset vector from whatever is there instead")
```

C concatenates those two literals. **The linker script language does not.** The
link failed with `stm32h7a3.ld:142: syntax error`, naming the line of the second
half, and nothing in the message says what the rule is. The other three messages
were single strings, which is why exactly one line failed and the cause looked
arbitrary.

The message is one literal now, long line and all. And `test_startup.py` gained
a rule that counts the quotes inside every `ASSERT` and refuses more than two,
because the link is the one check the authoring laptop cannot run: without that
rule the class is only ever caught in WSL or in CI, which is two pushes away
from where it was written. The control is the split put back, and the rule names
the assertion it is in.

## What a host can prove about an image, and what it cannot

`make pytest` checks the source. It cannot see what the linker did with it, and
a table discarded, a section placed off the boot address or a symbol resolved to
a different definition all produce an image that links and reports a sensible
size.

So `make linkcheck` links a real image from the startup and `test/link_probe.c`,
with `--gc-sections` on deliberately so that the `KEEP` is under test, and
`tools/check_image.py` reads the first two words back out of the `.elf` and
compares them against `nm`'s symbols.

The Thumb bit is why that comparison is not a plain equality. The reset vector
is `Reset_Handler`'s address with the low bit set, because the core uses it to
enter Thumb state. A check comparing the raw word against the symbol would fail
on a perfectly good image, so the rule masks the bit off, compares, and then
insists it was set: a reset vector with it clear asks for ARM state this part
does not have and faults on the first instruction.

**What none of this proves** is that the part runs the image. There is no
console yet, so an image that boots and an image that faults in `Reset_Handler`
look identical from outside. That is the next piece, and `[vendor]` the board's
own device tree already settles what it is: USART3, PD8 transmit, PD9 receive,
115200.

**And one finding for chapter 9 rather than for here.** The console is on
`PD8` and `PD9`; chapter 9's bus is on `PD0` and `PD1`. One port, two
peripherals, one clock enable shared between them. An image that configures
either by writing whole port registers rather than by read, modify, write takes
the other's pins with it, and the symptom is a console that stops or a bus that
goes quiet with nothing reported.
