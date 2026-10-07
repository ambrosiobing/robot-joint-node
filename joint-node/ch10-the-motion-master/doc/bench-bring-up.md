# Bringing the bus up on the bench: decisions before wiring

What was settled before a wire was connected, and how. Written Tuesday 7 October
2026, the day after the parts arrived, so that the next person does not repeat
the questions.

This page is the sequence. Three companions carry the detail, and if you only have
time for one, the first is the one that stops an evening being lost:

- [board-findings.md](board-findings.md) is the full inventory, device by
  device, with a source marker on every line: the header map, the terminal
  order, every link and jumper, every number, and the open questions.
- [datasheet-notes.md](datasheet-notes.md) is what the manufacturers specify,
  with a page number on every figure. It is where two of the findings on this
  page were corrected.
- [rewiring.md](rewiring.md) asks what of all that should be changed, and
  answers mostly no, with the reasons.
- [first-light.md](first-light.md) is what the hardware actually did when this
  sequence was run, with photographs. It corrected four things on this page.

Everything here concerns the controller end: a Raspberry Pi 4B with a Waveshare
WS-28164. The node end, the NUCLEO-H7A3ZI-Q, is chapter 9 and has no firmware
yet, so the bus has one node until that exists.

## The adapter, read from the schematic rather than guessed

**The board carries two CAN controllers, not one.** This is the single fact that
reshapes the bring-up, because it means the Pi never supplies a controller and
the two channels are independent.

| Channel | Controller | Transceiver | Bus | Interrupt | Clock |
|---|---|---|---|---|---|
| CAN FD | MCP2518FDT-H/SL | MCP2562FD | SPI0 chip select 1 | GPIO 24 | 40 MHz oscillator, plus or minus 20 ppm |
| Classic CAN | MCP2515T-I/SO | SN65HVD230DR | SPI0 chip select 0 | GPIO 23 | 16 MHz crystal |

The terminal block carries both separately, labelled `CAN FD` with `H`, `L`, `G`
and `CAN` with `H`, `L`, `G`, alongside `DC7-36V`, RS232 and two RS485 channels.
In the schematic's own numbering the classic pair is positions 1 and 2 and the
CAN FD pair is positions 4 and 5; the full fifteen position order is in
[board-findings.md](board-findings.md), and the silkscreen is what you read when
you turn the screw.

Four questions were put to the schematic rather than to the bench. All four had
answers, and one of them was not a question anybody had thought to ask.

**Does the isolated side need the external supply? No.** `U12` is a
**B0505LS-1W**, an isolated 5 V to 5 V, 1 W converter fed from the Pi's own 5 V
rail, producing the isolated rail `5VB`. `U4`, an `RT9193-33PB`, derives `3V3B`
from it. The `DC7-36V` terminal exists so the whole assembly can be powered from
an industrial supply instead; it is not part of the isolation barrier. USB power
to the Pi is enough.

This mattered because the failure it would have caused is a quiet one: the
controller probes perfectly over SPI, because the controller sits on the Pi's
side of the barrier, and the bus stays dead. That failure looks like firmware.

**Which resistor is the termination, and which jumper selects it?** `R16` with
`J1` for CAN FD, `R6` with `J2` for classic CAN, `R17` with `J4` and `R52` with
`J3` for the two RS485 channels. Each jumper has three positions and the
silkscreen prints `120R` and `NC` beside its terminal. `120R` is terminated.

Termination belongs at the two ends of the bus and nowhere else. With two nodes
both ends carry it; a third node in the middle must not.

**Read off the board on Wednesday 7 October 2026: both CAN caps are on `120R`**,
so both channels are terminated as they stand. The photograph and the reading
are in [first-light.md](first-light.md).

**Which interrupt goes where?** `R35`, a fitted 0 ohm link, puts the classic
channel's interrupt on GPIO 23, and `R36`, also fitted, puts the CAN FD
channel's on GPIO 24. Each has an unfitted alternative, `R21` to GPIO 22 and
`R37` to GPIO 13. So the device tree lines below are not a convention, they
match two resistors, and the alternatives need a soldering iron.

**Two statements that an earlier reading of this schematic got wrong**, both
corrected on Wednesday 7 October 2026 and both recorded in full in
[rewiring.md](rewiring.md), because the shape of the mistake is more useful than
the fix. `R36` does not choose between GPIO 23 and GPIO 24; those belong to two
different channels and two different link pairs. And `R32`, `R33` and `R34` do
not route anything: they are 1K pull ups on the serial expander's `RESET`,
`IRQ` and `I2C/SPI` pins. The SPI routing on this board is hard wired and cannot
be changed at all.

A third correction follows immediately below, and it has a different cause: the
schematic was read right and the **part** was reasoned about without opening its
datasheet. Six wrong readings came out of one day's work, four of the first kind
and two of the second, and [rewiring.md](rewiring.md) sets out both patterns.

**The surprise: `R3` is 1k on the SN65HVD230's `Rs` pin.** That pin selects the
transceiver's mode, so the value matters. The first reading of it, written earlier
the same day, was that 1k puts the part in slew limited mode and slows the classic
channel.

**Opening the datasheet reversed that.** TI selects the mode by the voltage on
`Rs`: above 0.75 VCC is standby, 10 kohm to 100 kohm to ground is slope control,
and below 1 V is high speed with no slope control at all. 1k is a tenth of the
fast end of that range, so the part is in **high speed** mode and the channel is
not slowed at all. Its loop delay is the best row in the table, 115 ns and 135 ns
maximum. The full correction, with page numbers, is in
[datasheet-notes.md](datasheet-notes.md) section 1.

What survives is the caution rather than the finding: `Rs` pulled **high** is
standby, and on this particular part that stops the driver while the receiver
keeps working, so a far end node sees a healthy listener that never speaks. And
it still says nothing about the loose SN65HVD230 module intended for the
NUCLEO-H7A3ZI-Q end, whose own `Rs` arrangement has not been read.

## The card

Raspberry Pi OS was not used. The image is cloud-init based, which decides where
every setting goes:

- `user-data` holds the account, SSH and packages
- `network-config` holds the wireless credentials as netplan
- `custom.toml` is **Raspberry Pi OS only** and is inert here

That distinction cost a round of work. A check that looked for `custom.toml` and
`firstrun.sh`, found neither, and concluded the image had not been customised was
wrong: Raspberry Pi Imager had written the cloud-init files instead. **Check
which image you have before checking for its artefacts.**

Two lines go in `config.txt`, under `[all]`:

```
dtoverlay=mcp251xfd,spi0-1,interrupt=24
dtparam=spi=on
```

No `oscillator=` is passed because the driver's default is 40 MHz and the board
fits a 40 MHz clock. For the classic channel the line is
`dtoverlay=mcp2515,spi0-0,oscillator=16000000,interrupt=23`, and that one does
need its `oscillator=` because 16 MHz is not the default.

A small correction worth carrying: the 40 MHz part, `Y2`, is a **packaged
oscillator**, not a crystal. It has four pins, an output enable, and a printed
tolerance of plus or minus 20 ppm. The tolerance is the useful half, because it
is a number the bit timing budget can quote instead of assume.

**Verify the overlay exists in the image before booting.** An overlay a
`dtoverlay=` line names but which is not present is accepted silently and does
nothing:

```bash
ls /boot/firmware/overlays/ | grep -i mcp251
```

`mcp251xfd.dtbo` was confirmed present on Tuesday 6 October 2026. The running
system turned out to be **Debian trixie**, not Bookworm as this line first said;
see [first-light.md](first-light.md).

## Enabling everything is the wrong first move

A first attempt enabled every bus the Pi has: `i2c1` through `i2c6`, `spi1`
through `spi5`, `i2c_vc`, `i2s`. Five of those touch pins this board uses,
and two of the five are fatal:

| Overlay | Claims | Which is | Verdict |
|---|---|---|---|
| `i2c4` | GPIO 6, 7 | GPIO 7 is SPI0 CE1, the MCP2518FD chip select | Fatal |
| `i2c6` | GPIO 22, 23 | GPIO 23 is the MCP2515 interrupt | Fatal for the classic channel |
| `spi5-1cs` | GPIO 12 to 15 | GPIO 14, 15 are the serial console | Loses the console |
| `spi3-1cs` | GPIO 0 to 3 | GPIO 0, 1 reach an unfitted footprint | Harmless here |
| `i2c_vc` | GPIO 0, 1 | the same unfitted footprint | Harmless here |

`i2c4` alone takes the chip select the CAN FD controller sits on, so the CAN FD
interface would never appear however correct everything else was.

**The last two rows are a correction.** They were first written down as
conflicts with the HAT ID EEPROM. There is no HAT ID EEPROM on this board:
`U14`, its address resistors and its decoupling are all marked `NC` on the
schematic and none of them is fitted. So GPIO 0 and GPIO 1 run to an empty
footprint, and those two overlays take nothing this board needs. The conclusion
is unchanged and the reason was wrong, which is worth fixing on its own, because
a right answer with a wrong reason gets applied wrongly to the next board.

The same finding explains something else. Without an ID EEPROM the board can
never be auto detected, so every `dtoverlay=` line has to be written by hand.
That is why the wiki hands you a configuration block instead of saying it just
works.

The working set is the one that leaves those pins alone: `enable_uart=1`,
`dtparam=i2c_arm=on`, `dtparam=spi=on`, `dtoverlay=w1-gpio`, and the
`mcp251xfd` line. Add interfaces one at a time afterwards, because the cost of
enabling them all at once is that a failure has fourteen possible causes and no
way to tell them apart.

## Order of operations

**Boot once with the adapter off.** If the board does not appear on the network,
that separates the card and the credentials from the adapter. One power cycle
buys an unambiguous answer.

**Then fit the adapter with nothing else on the header.** It is a 40-pin board,
so it cannot share the Pi with the data acquisition HAT, the Explorer700, the
display or any of the modems.

**Check the ribbon stripe if an extension cable is used.** The red stripe is
conductor 1, and on a Pi 4B pin 1 is the end of the header nearest the USB-C
socket. The board's connector is keyed; the Pi's header is bare pins and can
take a reversed cable, which puts 5 V and ground on the wrong pins.

**Then, and only then, ask whether the controller is alive:**

```bash
dmesg | grep -i -e mcp251 -e spi -e can
```

A probe that fails on a register or clock read means the controller is not
answering over SPI, which is seating, chip select or crystal, and nothing
further down the stack.

## What loopback proves, and what it does not

```bash
sudo ip link set can0 type can bitrate 500000 dbitrate 2000000 fd on loopback on
sudo ip link set up can0
ip -details link show can0
```

This proves the controller, the SPI link, the overlay and that both bit rates
are achievable on that part. It proves nothing about the transceiver, the
termination, the cable or the far node, because in loopback the frame never
reaches a pin. That is the same distinction chapter 9 draws between internal and
external loopback, and it is why the chapter does them in that order.

The last command prints the sample points the kernel chose. They will not match
chapter 9's 80.0 and 75.0 per cent, because the MCP2518FD runs from a 40 MHz
crystal and the NUCLEO-H7A3ZI-Q from an 80 MHz kernel clock, so the two have
different achievable sets. Record both; the comparison is a finding rather than
a fault.

## Still open

**The far node cannot transmit.** Chapter 9's `bittiming`, `msgram` and `frame`
are host code. There is no `fdcan.c`, no mode selection and no send path, so
until those exist this is a bus with one node.

A one node bus is not a small limitation. Nothing acknowledges a frame, so every
transmission fails and the controller retries, which is physics rather than a
fault and is the classic confusing first hour of a CAN bring up. It was observed
exactly that way on Wednesday 7 October 2026 before the wires went in: the
transmitter sat in `ERROR-PASSIVE` with a transmit error counter of 128, which is
sixteen failed attempts, while `TX: packets` stayed at 0. See
[first-light.md](first-light.md) part two.

There is a way around it that needs no firmware and no parts, and it came out
of reading the schematic rather than out of planning: **this board can be its
own two node bus. Done on Wednesday 7 October 2026, and it works.** It carries
two complete controllers with separate oscillators, separate transceivers and
separate terminal positions, so joining terminal position 1 to 4 and 2 to 5
with two jumper wires gives a real bus with real arbitration, real error
frames, real termination and a real acknowledgement. It has to run classic,
because the MCP2515 cannot do flexible data frames, so it proves nothing about
CAN FD itself. [rewiring.md](rewiring.md) sets out what it does and does not
buy, and recommends it.

**And when both channels are up, do not assume which interface is which.**
`can0` and `can1` are handed out in the order the drivers probe, which nothing
in the device tree fixes. One command settles it, and the answer is worth
writing down with the date:

```bash
for i in /sys/class/net/can*; do
  echo "$i -> $(basename $(readlink -f $i/device/driver))"
done
```

**The bit rate switch is capped at the NUCLEO-H7A3ZI-Q end**, and the reason is
not the obvious one. The loose transceiver is rated for 1 Mbit signalling, but
by loop delay it is the **fastest** of the three transceivers on this bench.
What it does not specify is loop delay **symmetry**, which is the property a
CAN FD data phase actually depends on. So chapter 13 waits for a part whose
datasheet states that figure, and the named candidate is the TCAN3413. The
comparison is in [datasheet-notes.md](datasheet-notes.md) section 5. The
adapter's own CAN FD channel is not the limit.

## How these answers were obtained

The schematic PDF carries a text layer, so it can be parsed rather than read as
a picture. That one trick is what made every finding above cheap, and it is
written up properly, with the worked code and the two traps it has, under
"How to re-derive all of this in about two minutes" in
[board-findings.md](board-findings.md).

The document list lives there too, deliberately in one place rather than two, so
that it cannot drift between copies. The short version: there is no user guide,
the wiki is the manual, the schematic answers more than the wiki does, and three
of the parts on this board have no datasheet in the vendor's own resource list.
