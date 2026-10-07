# The datasheets, read: every number that bears on this bus

This is the companion to [board-findings.md](board-findings.md). That page says
what is on the boards. This one says what the manufacturers specify about those
parts, with a page number on every line, so that any figure used anywhere in
this volume can be traced to a page somebody opened.

It is long. That is the point of it. You are not meant to read it front to back;
you are meant to be able to find the one number you need and see where it came
from. Each part gets its own section, and each section ends with what the
datasheet changed about a decision, because a number that changes nothing is
trivia.

Read on **Wednesday 7 October 2026**, all documents fetched from the addresses in
the table at the end.

## How to read a line

Same markers as the findings page, with one addition that this page enforces:
**every `[datasheet]` row names the page it was read from.** A specification
without a page reference is a recollection, and recollections on this bench have
been wrong often enough to be worth the extra four characters.

| Marker | Means |
|---|---|
| `[datasheet]` | Read from the manufacturer's document, with the page given |
| `[schematic]` | Read from the WS-28164 schematic's text layer |
| `[inferred]` | Concluded from the above, with the reasoning given so you can disagree |
| `[unconfirmed]` | Not established. No wiring decision may rest on one of these |

## A note on the diagrams in these pages

Every diagram in this chapter's documents is **drawn here, from the facts**, and
cites the vendor figure it corresponds to by number and page.

That is a deliberate choice and worth one paragraph, because the obvious
alternative is to paste the manufacturer's own figures in. Datasheet artwork is
the manufacturer's copyrighted work. Facts about a part, that `STBY` high means
standby or that a loop delay is 115 ns, are not anybody's property and can be
stated freely. So the honest arrangement is to redraw from the facts and point at
the original, which has three further advantages beyond the obvious one:

1. A redrawn figure can show **this** board. TI's Figure 9 shows a generic loop
   delay measurement. What is actually wanted here is where the isolation
   barrier falls on the WS-28164, which no vendor figure contains.
2. A citation survives. An image pasted into a repository is a snapshot of one
   revision. "SLOS346K page 8, Figure 9" still finds the right thing in the next
   revision, or tells you plainly that it moved.
3. A reader who opens the original beside the redrawing is doing the thing this
   whole chapter is arguing for.

Where a vendor figure is the clearest possible explanation of something, the
text says so and gives its number, so you can go and look at it.

---

# 1. SN65HVD230, the classic CAN transceiver

Two of these matter: `U3` on the WS-28164, and the loose Waveshare board
intended for the NUCLEO-H7A3ZI-Q end.

**Document: TI SLOS346K, March 2001, revised February 2011.** Note the
revision letter. An earlier note on this bench cited SLOS346O; the copy read
here is **K**, and where a number differs between revisions the letter is what
tells you which one you are holding `[datasheet]` p1.

## The headline specification

| Quantity | Value | Source |
|---|---|---|
| Supply | 3.3 V | `[datasheet]` p1 |
| Standard | ISO 11898 | `[datasheet]` p2 |
| Signalling rate | up to 1 Mbps | `[datasheet]` p2 |
| Bus common mode range | -2 V to 7 V | `[datasheet]` p2 |
| Common mode transient withstand | plus or minus 25 V | `[datasheet]` p2 |
| Protection features | cross wire, loss of ground, overvoltage, overtemperature | `[datasheet]` p2 |
| Pin 5, `Vref` | a VCC/2 voltage reference, available | `[datasheet]` p2 |
| Supply current, driving | 10 mA typical, 17 mA maximum, dominant or recessive | `[datasheet]` p7 |
| Supply current, standby | 370 microamps typical, 600 maximum | `[datasheet]` p7 |

## The part number matters, and the board uses the one that keeps its receiver

Table 1 of the datasheet separates three parts that share a pinout
`[datasheet]` p2:

| Part | What pin 8 high does | `Vref` pin | Source |
|---|---|---|---|
| **SN65HVD230** | **Standby. Driver off, receiver stays active** | yes | `[datasheet]` p2 |
| SN65HVD231 | Sleep. Driver **and** receiver off | yes | `[datasheet]` p2 |
| SN65HVD232 | No standby and no sleep, pins 5 and 8 are no connect | no | `[datasheet]` p2 |

`U3` on the WS-28164 is marked **SN65HVD230DR** `[schematic]`, so it is the
standby variant. The practical consequence: if pin 8 were ever pulled high, the
board would still **receive** while transmitting nothing. That is a worse failure
than going completely silent, because a bus analyser at the far end sees a
healthy listener and no traffic.

## The correction: `R3` at 1k does not slope limit anything

This is the most important thing on this page, because it reverses a finding
written down earlier in this chapter.

Pin 8 is `Rs`, and the mode is selected by the **voltage** on it, not simply by
whatever resistor is fitted. Table 6, TRANSCEIVER MODES `[datasheet]` p3:

| Condition on `Rs` | Operating mode | Source |
|---|---|---|
| V(`Rs`) greater than 0.75 VCC | Standby | `[datasheet]` p3 |
| **10 kohm to 100 kohm to ground** | **Slope control** | `[datasheet]` p3 |
| **V(`Rs`) less than 1 V** | **High speed, no slope control** | `[datasheet]` p3 |

And the slope control range is stated explicitly: 10 kohm achieves a 15 V per
microsecond slew rate, 100 kohm achieves 2 V per microsecond `[datasheet]` p2.

`R3` is **1 kohm** `[schematic]`. That is a tenth of the fast end of the
specified range. Since the slope is proportional to the pin's output current
`[datasheet]` p2, a resistor ten times smaller develops a tenth of the voltage
for the same current, which puts `Rs` firmly in the "less than 1 V" row
`[inferred]`.

**So the board's classic transceiver is in high speed mode**, not slew limited.
`R3` is a ground connection through a small resistor, which is a common and
sensible way to strap a mode pin while leaving the option open.

Here is the mode window as a line, with the fitted value marked. Redrawn from
the conditions in Table 6, `[datasheet]` p3:

```
V(Rs):  0 V ─────────── 1 V ──────────────────── 0.75·VCC ──── VCC
             HIGH SPEED  │   (unspecified gap)        │  STANDBY
        ▲                │                            │
        │                └── slope control lives here,
        │                    set by 10 k … 100 k to ground
        │
     R3 = 1 k sits here, well inside HIGH SPEED
        (and a tenth of the fast end of the slope-control range)
```

### What that does to the loop delay

The loop delay is the number chapter 9 step 5 needs for transmitter delay
compensation, and it depends entirely on which mode the part is in. The full
table, DEVICE SWITCHING CHARACTERISTICS `[datasheet]` p8:

| `Rs` arrangement | Recessive to dominant, typ | max | Dominant to recessive, typ | max | Source |
|---|---|---|---|---|---|
| **V(`Rs`) = 0 V, which is the mode this board is in** | **70 ns** | **115 ns** | **100 ns** | **135 ns** | `[datasheet]` p8 |
| 10 kohm to ground | 105 ns | 175 ns | 155 ns | 185 ns | `[datasheet]` p8 |
| 100 kohm to ground | 535 ns | 920 ns | 830 ns | 990 ns | `[datasheet]` p8 |

**Two numbers in an earlier note on this bench were typicals quoted as
maximums.** It said the 10 kohm case raises the delays "to about 105 and 155",
and 100 kohm "to about 535 and 830". Those four figures are all typicals. The
maximums are 175, 185, 920 and 990, and a timing budget is built from maximums
`[datasheet]` p8.

So the figure to use for this board's classic channel is **115 ns and 135 ns
maximum**, the grounded row, and that is the best case of the three. The channel
is not handicapped at all.

Driver switching, for completeness `[datasheet]` p7:

| Quantity | V(`Rs`) = 0 V | 10 kohm | 100 kohm | Source |
|---|---|---|---|---|
| Propagation delay low to high, typ and max | 35, 85 ns | 70, 125 ns | 500, 870 ns | `[datasheet]` p7 |
| Propagation delay high to low, typ and max | 70, 120 ns | 130, 180 ns | 870, 1200 ns | `[datasheet]` p7 |
| Pulse skew, typ | 35 ns | 60 ns | 370 ns | `[datasheet]` p7 |
| Differential output rise time, typ | 50 ns | 120 ns | 800 ns | `[datasheet]` p7 |
| Differential output fall time, typ | 55 ns | 125 ns | 825 ns | `[datasheet]` p7 |

Receiver switching: propagation delay 35 ns typical and 50 ns maximum both
ways, pulse skew 10 ns, output rise and fall 1.5 ns `[datasheet]` p8.

## The bus side, for the termination and loading argument

| Quantity | Value | Source |
|---|---|---|
| Dominant differential output | 1.5 V minimum, 2 V typical, 3 V maximum | `[datasheet]` p7 |
| Recessive differential output, loaded | -120 to 12 mV | `[datasheet]` p7 |
| Receiver positive going threshold | 750 to 900 mV | `[datasheet]` p8 |
| Receiver negative going threshold | 500 to 650 mV | `[datasheet]` p8 |
| Receiver hysteresis | 100 mV | `[datasheet]` p8 |
| Differential input resistance | 40 to 100 kohm, 70 typical | `[datasheet]` p8 |
| Single pin input resistance | 20 to 50 kohm, 35 typical | `[datasheet]` p8 |
| `CANH`, `CANL` input capacitance | 32 pF | `[datasheet]` p8 |
| Differential input capacitance | 16 pF | `[datasheet]` p8 |
| Short circuit output current | plus or minus 250 mA | `[datasheet]` p7 |

The 70 kohm differential input resistance is why node count matters to
termination and why a receiver does not meaningfully load a terminated bus: two
120 ohm resistors in parallel are 60 ohms, and a few 70 kohm receivers across
that change nothing `[inferred]`.

And the receiver thresholds explain a thing worth knowing about dominant and
recessive. The wake condition from standby is a bus differential above 900 mV
typical `[datasheet]` p2, which is the same 900 mV as the positive going
threshold. The part has one notion of "dominant" and uses it consistently.

## What this datasheet changed

| Belief | Now | Why it matters |
|---|---|---|
| `R3` at 1k slews the classic channel and raises its loop delay | `R3` at 1k is high speed mode; the loop delay is the best of the three rows | Removed a worry and an `[unconfirmed]` number from the bit timing budget |
| 10 kohm raises delay "to about 105 and 155" | 105 and 155 are typicals; the maximums are 175 and 185 | A budget built from typicals is not a budget |
| The slope control resistor could be any value | TI specifies 10 kohm to 100 kohm, and 1k is outside it | Explains the board designer's choice as a strap, not a filter |

---

# 2. MCP2562FD, the CAN FD transceiver on the HAT

`U6`, the part the bus this volume cares about actually drives.

**Document: Microchip DS20005284A, the MCP2561FD and MCP2562FD datasheet.** It
was not in Waveshare's own resource list, which is why it carried the most
`[unconfirmed]` rows until now.

| Quantity | Value | Source |
|---|---|---|
| `VDD` range | **4.5 V to 5.5 V** | `[datasheet]` p9 |
| `VIO` range | **1.8 V to 5.5 V** | `[datasheet]` p9 |
| Standards | ISO 11898-2 and ISO 11898-5 | `[datasheet]` p1 |
| Operation claimed | 2 Mbps and 8 Mbps | `[datasheet]` p1 |
| Loop delay symmetry guaranteed to | **5 Mbps** | `[datasheet]` p3 |
| Standby current | 5 microamps typical | `[datasheet]` p1 |
| `STBY` low | **Normal mode.** Driver operational | `[datasheet]` p3 |
| `STBY` high | **Standby.** Transmitter and the high speed part of the receiver disabled | `[datasheet]` p3 |
| Wake up requires | both `VDD` and `VIO` in valid range | `[datasheet]` p3 |

Timing, from the AC characteristics `[datasheet]` p13:

| Parameter | Typical | Maximum | Source |
|---|---|---|---|
| `tTXD-BUSON`, TXD low to bus dominant | 65 ns | not specified | `[datasheet]` p13 |
| `tTXD-BUSOFF`, TXD high to bus recessive | 90 ns | not specified | `[datasheet]` p13 |
| `tBUSON-RXD`, bus dominant to RXD | 60 ns | not specified | `[datasheet]` p13 |
| `tBUSOFF-RXD`, bus recessive to RXD | 65 ns | not specified | `[datasheet]` p13 |
| `tTXD-RXD`, propagation delay TXD to RXD | 90 ns | 120 ns | `[datasheet]` p13 |
| `tTXD-RXD`, the other edge | 120 ns | 180 ns | `[datasheet]` p13 |
| Loop delay symmetry at 2 Mbps | 450, 485, 550 ns | min, typ, max | `[datasheet]` p13 |
| Loop delay symmetry at 5 Mbps | 160, 185, 220 ns | min, typ, max | `[datasheet]` p13 |
| Loop delay symmetry at 8 Mbps | 85, 105, 140 ns | min, typ, max | `[datasheet]` p13 |

## The split supply, now confirmed rather than inferred

The findings page inferred that `U6`'s `VDD` is on the isolated 5 V rail `5VB`
because the schematic text layer carried no label on that pin. The datasheet
settles it by elimination: **`VDD` must be 4.5 to 5.5 V** `[datasheet]` p9, and
the only rail on the isolated side of the barrier in that range is `5VB`. A
3.3 V `VDD` would be out of specification by more than a volt `[inferred]`.

`VIO` is read directly as `3V3B` from the schematic `[schematic]`, and the
datasheet's 1.8 to 5.5 V range accommodates it `[datasheet]` p9. So the part is
doing exactly what the split supply exists for: a 5 V bus driver with 3.3 V
digital pins that match the isolator facing them.

## The `STBY` question, mostly answered by a different datasheet

The findings page lists "what is `U6` `STBY` tied to" as the highest value open
question, because `STBY` high disables the transmitter while the controller reads
perfectly `[datasheet]` p3. The schematic's text layer carries no net on that
pin.

Two datasheets together now make the answer very likely. The MCP2518FD has a pin
named `INT0/GPIO0/XSTBY`, and a register bit `XSTBYEN` whose documented purpose
is to let the controller "automatically control the standby pin of the
transceiver" `[datasheet]` MCP2518FD p19 and p74. So a designer **could** route
controller to transceiver standby.

On this board they cannot, and the reason is countable. The transceiver sits on
the isolated side and the controller does not, so any standby signal would have
to cross the barrier. The barrier crossing for the CAN section is `U7`, a
four channel isolator, and all four channels are accounted for: `TXD_0` and
`RXD_0` for the classic channel, `TXD_1` and `RXD_1` for the CAN FD channel
`[schematic]`. **There is no spare channel for a standby line** `[inferred]`.

Therefore `U6` `STBY` is strapped locally on the isolated side, and since the
board functions as a CAN FD adapter it must be strapped **low**, which is Normal
mode `[inferred]`. The question moves from "this could silently break the bus"
to "this is almost certainly a tie to `SGND`, worth confirming by eye on the
drawing". That is a real reduction in risk, and it came from counting isolator
channels rather than from finding the net.

---

# 3. MCP2518FD, the CAN FD controller

`U5`. The kernel's `mcp251xfd` driver owns this part, so nothing in this volume
programs it directly. These figures are here to bound what the channel can do.

| Quantity | Value | Source |
|---|---|---|
| Message RAM | **2 KB**, message objects located in it | `[datasheet]` p1, p63 |
| SPI clock | up to **20 MHz** | `[datasheet]` p1 |
| SPI modes | **0,0 or 1,1**, 8-bit operation | `[datasheet]` p67 |
| FIFOs | **31**, each configurable as transmit or receive | `[datasheet]` p1 |
| Filter and mask objects | **32**, flexible | `[datasheet]` p1 |
| Arbitration bit rate | up to **1 Mbps** | `[datasheet]` p1 |
| Data bit rate | up to **8 Mbps** | `[datasheet]` p1 |
| `VDD` | **2.7 to 5.5 V** | `[datasheet]` p1, p76 |
| Clock input | 40 MHz crystal or resonator | `[datasheet]` p77 |
| Internal PLL | 10x, for multiplying a 4 MHz clock | `[datasheet]` p17, p73 |
| Transceiver standby control | `INT0/GPIO0/XSTBY` pin, enabled by `XSTBYEN` | `[datasheet]` p19, p74 |

## Two things worth noticing

**The arbitration rate ceiling is 1 Mbps and the data rate ceiling is 8 Mbps**
`[datasheet]` p1. That is not a quirk of this part, it is what CAN FD is: a slow
arbitration phase where every node must hear every other node within one bit,
and a fast data phase where only two nodes matter. The planned 500 kbit/s and
2 Mbit/s sit comfortably inside both.

**The clock is specified as a crystal or resonator, and this board fits neither.**
`Y2` is a packaged oscillator with `VDD`, `OUT`, `OE` and `GND`, printed
`40MHz(5032) 3.3V plus or minus 20ppm` `[schematic]`, driving `OSC1` through a
33 ohm series resistor `R15` `[schematic]`. Feeding a crystal oscillator input
from a driven clock is ordinary and generally what the `OSC1`-only connection is
for, but it is worth saying that the datasheet's characterisation assumes a
crystal `[datasheet]` p77, and that the oscillator's own `OE` pin introduces a
failure mode a crystal does not have `[unconfirmed]`.

What the packaged part buys in return is the **plus or minus 20 ppm** figure
`[schematic]`, which is printed on the board rather than assumed about a crystal
whose tolerance the schematic does not state.

---

# 4. MCP2515, the classic CAN controller

`U2`. Relevant to this volume only through the two node self bus proposal in
[rewiring.md](rewiring.md), and it is the part that makes that bus classic only.

| Quantity | Value | Source |
|---|---|---|
| Protocol | **CAN V2.0B at 1 Mb/s**, classic only | `[datasheet]` p1 |
| Transmit buffers | **three**, with prioritisation and abort | `[datasheet]` p1 |
| Receive buffers | **two**, with prioritised message storage | `[datasheet]` p1, p3 |
| Acceptance filters | **six** 29-bit filters | `[datasheet]` p1 |
| Acceptance masks | **two** | `[datasheet]` p1 |
| SPI clock | up to **10 MHz** | `[datasheet]` p1 |
| Supply | 2.7 V to 5.5 V | `[datasheet]` p1, p70 |
| Time quantum | **TQ = 2 x (BRP + 1) / FOSC** | `[datasheet]` p42 |
| Base time quantum | twice the oscillator period | `[datasheet]` p38 |

## The time quantum, worked for this board

The formula is the useful part, because it says what bit rates the classic
channel can actually hit. `Y1` is 16 MHz `[schematic]`, so the oscillator period
is 62.5 ns and the base time quantum, at `BRP` = 0, is **125 ns**
`[datasheet]` p38 and p42.

At the planned 500 kbit/s a bit is 2000 ns, which is **16 TQ exactly** at
`BRP` = 0. Sixteen time quanta is a comfortable, conventional bit: one for
synchronisation leaves fifteen to distribute across the two phase segments, and
a sample point at 75 or 80 per cent lands on a whole number of quanta. No
rounding, no compromise.

That is worth stating plainly because it is the kind of thing that is usually
discovered the hard way: the self bus proposal runs at 500 kbit/s partly because
chapter 9 chose it, and partly because **16 MHz divides into it exactly**
`[inferred]`.

Note also that classic CAN's 1 Mb/s ceiling `[datasheet]` p1 is not what limits
the self bus. 500 kbit/s is half of it.

---

# 5. TCAN3413, the candidate for the node end

The part named for the buy that unblocks chapter 13. **Document: TI, TCAN341x
3.3 V CAN FD Transceivers With Standby Mode and plus or minus 58 V Bus
Standoff.**

| Quantity | Value | Source |
|---|---|---|
| Supply | **3.3 V single supply** | `[datasheet]` p1, p3 |
| `VIO` | present on the **TCAN3413 only**, absent on the TCAN3414 | `[datasheet]` p3 |
| `VIO` levels supported | 1.8 V, 2.5 V or 3.3 V logic | `[datasheet]` p1 |
| Data rate | up to **8 Mbps**, characterised at 2, 5 and 8 Mbps | `[datasheet]` p1 |
| Bus standoff | plus or minus 58 V | `[datasheet]` p1 |
| Total loop delay, recessive to dominant | **95 ns typical, 180 ns maximum** | `[datasheet]` p8 |
| Total loop delay, dominant to recessive | **120 ns typical, 180 ns maximum** | `[datasheet]` p8 |
| Propagation delay symmetry | short and symmetrical, by design | `[datasheet]` p1 |

Pin functions, Table 4-1 `[datasheet]` p3:

| Pin | Name | Type | Note | Source |
|---|---|---|---|---|
| 1 | `TXD` | digital input | CAN transmit data, **integrated pull up** | `[datasheet]` p3 |
| 2 | `GND` | ground | | `[datasheet]` p3 |
| 3 | `VCC` | supply | 3.3 V | `[datasheet]` p3 |
| 4 | `RXD` | digital output | tri-stated when the device is powered off | `[datasheet]` p3 |
| 5 | `SHDN` | digital input | high puts the device in ultra low power shutdown | `[datasheet]` p3 |
| 5 | `VIO` | supply | I/O supply, **TCAN3413 only**, in place of `SHDN` | `[datasheet]` p3 |
| 6 | `CANL` | bus | | `[datasheet]` p3 |
| 7 | `CANH` | bus | | `[datasheet]` p3 |
| 8 | `STB` | digital input | standby mode control, **integrated pull up** | `[datasheet]` p3 |

## Three things to check before ordering, answered

The rewiring page listed three. All three now have answers.

**Does it need a second rail?** No. Single 3.3 V supply `[datasheet]` p1. On the
TCAN3413 specifically, pin 5 is `VIO` rather than `SHDN` `[datasheet]` p3, so if
you want the simplest possible wiring, the **TCAN3414** is the variant with
`SHDN` and no `VIO`, and both are 3.3 V parts. Which variant to buy is therefore
a real choice and not a detail.

**What is the loop delay?** 95 and 120 ns typical, **180 ns maximum both ways**
`[datasheet]` p8. Specified plainly, at a named data rate, with no resistor
dependent modes to reason about.

**Does it have a slope control pin?** **No.** Pin 8 is `STB`, standby, and pin 5
is either `SHDN` or `VIO` depending on variant `[datasheet]` p3. There is no
`Rs`. That removes the entire class of question that `R3` opened on the
SN65HVD230.

But note the thing that replaces it: **`STB` has an integrated pull up**
`[datasheet]` p3. Left floating, the part is in **standby**, which is the
"reads perfectly, transmits nothing" failure. On the SN65HVD230 a floating pin 8
is also standby. So the trap does not go away, it changes shape, and either part
needs its mode pin deliberately tied.

## Why chapter 13 really needs this part, which is not what was written before

Put the three transceivers side by side. All values are maximums where a maximum
is specified, because that is what a timing budget uses.

| Part | Supply | Loop delay r to d | d to r | Rated | Symmetry specified | Source |
|---|---|---|---|---|---|---|
| SN65HVD230, `Rs` grounded | 3.3 V | 115 ns | 135 ns | 1 Mbps | **no** | `[datasheet]` SLOS346K p8, p2 |
| MCP2562FD | `VDD` 5 V, `VIO` 1.8 to 5.5 V | 120 ns | 180 ns | 8 Mbps | **yes, to 5 Mbps** | `[datasheet]` DS20005284A p13, p3 |
| TCAN3413 | 3.3 V | 180 ns | 180 ns | 8 Mbps | **yes** | `[datasheet]` p8, p1 |

Look at the first column of numbers. **The SN65HVD230 is the fastest of the
three.** 115 and 135 nanoseconds beats both FD rated parts.

So the reason chapter 13 cannot run on it is **not** that it is slow. The reason
is the last column. CAN FD's data phase depends on the delay being
**symmetrical**, because a bit that comes back 60 ns longer than it went out
eats into the next bit's sample point, and at 2 Mbit/s there are only 500 ns to
spend. Microchip states the case directly: the MCP2562FD "guarantees Loop Delay
Symmetry in order to support the higher data rates required for CAN FD"
`[datasheet]` DS20005284A p1, and gives the symmetry window as a specified
number at 2, 5 and 8 Mbps `[datasheet]` p13. TI's part is sold on "short and
symmetrical propagation delays" `[datasheet]` TCAN3413 p1.

The SN65HVD230 specifies **pulse skew**, 35 ns typical with `Rs` grounded
`[datasheet]` SLOS346K p7, but not a loop delay symmetry figure, and it is rated
for a signalling rate of 1 Mbps rather than for a data phase at all
`[datasheet]` p2.

**That is the honest statement of the limit, and it is better than the old one**,
which said the part was too slow. A reader who believed "too slow" might
reasonably try 2 Mbit/s and find it half works, which is the most expensive
possible outcome. A reader who understands that the part does not specify
symmetry knows that it may work on a short bench wire at room temperature and
is not a result.

---

# 6. SC16IS752, the serial expander

Not used by this volume. Three numbers are here because the part shares SPI1 and
an interrupt pin with the CAN section, and because one of them is a trap.

| Quantity | Value | Source |
|---|---|---|
| Function | dual UART with an I2C or SPI host interface | `[datasheet]` p1 |
| FIFOs | **64 bytes**, transmit and receive | `[datasheet]` p1 |
| Maximum SPI clock | **4 Mbit/s** | `[datasheet]` p2 |
| The SC16IS**762** instead supports | 15 Mbit/s SPI, and IrDA SIR to 1.152 Mbit/s | `[datasheet]` p1, p2 |
| RS485 feature | driver direction control, with inversion | `[datasheet]` p2 |

**The 4 Mbit/s SPI ceiling is the trap.** The board fits the **752**
`[schematic]`, not the 762, and the two differ in essentially nothing else
`[datasheet]` p1. So an overlay or a driver that clocks SPI1 above 4 MHz is out
of specification on this board, and the part that would have tolerated it is a
different part number. If the RS485 channels are ever brought up and behave
intermittently, this is the first number to check.

The board straps the part into SPI mode with `R34`, a fitted 1 kohm pull up on
pin 9, `I2C/SPI` `[schematic]`, with `R32` and `R33` as 1 kohm pull ups on
`RESET` and `IRQ` `[schematic]`.

---

# 7. BCM2711, the host side

The Raspberry Pi 4's own peripheral document, which is what turns the overlay
conflict table from a recollection into a reading.

| Quantity | Value | Source |
|---|---|---|
| GPIO 7 alternate function 0 | **`SPI0_CE1_N`** | `[datasheet]` p79 |
| GPIO 8 alternate function 0 | `SPI0_CE0_N` | `[datasheet]` p79 |
| SPI1 chip select 0, MISO, MOSI, SCLK | on the GPIO 16 to 21 group | `[datasheet]` p79, p81 |
| SPI master maximum clock | 125 MHz with a 250 MHz core clock | `[datasheet]` p18, p19 |
| Practical SPI ceiling | lower, because the I/O pads cannot keep up | `[datasheet]` p19 |
| SPI FIFO depth | shallow, with no DMA support on these masters | `[datasheet]` p18 |

**GPIO 7's alternate function 0 is `SPI0_CE1_N`** `[datasheet]` p79, and the
CAN FD controller's chip select is `SPI0_CE1` `[schematic]`. That is the
conflict with the `i2c4` overlay, now read from the Pi's own document rather
than recalled. It is the single fatal row in the overlay table and it deserved
a citation.

The 4 Mbit/s ceiling on the SC16IS752 and the 20 MHz ceiling on the MCP2518FD
are both far below what the SPI master can produce `[datasheet]` p18, so on this
board the host is never the limit. Which peripheral sets the clock ceiling is the
sort of thing worth knowing before blaming the wrong end.

---

# 8. Raspberry Pi 4, the power path question

| Quantity | Value | Source |
|---|---|---|
| 5 V input voltage, absolute maximum | -0.5 V to **6.0 V** | `[datasheet]` p8 |
| GPIO default drive strength | 8 mA | `[datasheet]` p9 |
| GPIO maximum drive strength | 16 mA | `[datasheet]` p9 |
| Output low current, at `VO` = 0.4 V | 7 mA | `[datasheet]` p9 |
| Output high current, at `VO` = 2.3 V | 7 mA | `[datasheet]` p9 |

The first row is the relevant one, and it is quietly informative. The board's
5 V header pins are specified as an **input voltage** with an absolute maximum
`[datasheet]` p8. A rail described that way is one the designers expect
something else may drive, which is consistent with the reading in the findings
page that an add-on board with its own supply feeds the Pi through the header
`[inferred]`.

It does not prove it. The absolute maximum of a rail says nothing about the
direction of any particular circuit, and the thing that settles the WS-28164's
power path is still one power cycle with USB-C out and the DC terminal in. But
it does mean the arrangement is contemplated rather than abusive, and that is
worth knowing before anybody worries about it.

---

# What the datasheets changed, as one list

| # | Before | After | Where |
|---|---|---|---|
| 1 | `R3` at 1k slew limits the classic channel | 1k is high speed mode; TI's slope control range starts at 10 kohm | Section 1 |
| 2 | Loop delays "about 105 and 155" at 10 kohm | Those are typicals; the maximums are 175 and 185 | Section 1 |
| 3 | `U6` `VDD` on `5VB` was an inference from the drawing | `VDD` must be 4.5 to 5.5 V, so no other isolated rail is possible | Section 2 |
| 4 | `U6` `STBY` unknown, highest risk open question | Almost certainly tied low, because the isolator has no spare channel to carry a standby signal | Section 2 |
| 5 | Chapter 13 waits because the SN65HVD230 is too slow | It is the **fastest** of the three. It waits because the part does not specify loop delay symmetry | Section 5 |
| 6 | TCAN3413's three questions open | All three answered, and the variant choice turns out to matter | Section 5 |
| 7 | The overlay conflict table was a recollection | GPIO 7 alternate function 0 is `SPI0_CE1_N`, read from BCM2711 | Section 7 |
| 8 | 500 kbit/s on the classic channel was chapter 9's choice | 16 MHz also divides into it exactly, 16 TQ at `BRP` = 0 | Section 4 |

Item 5 is the one to carry away. The old reason was wrong in a way that would
have produced a confident wrong experiment, and the new reason explains why a
bench test might appear to succeed and still prove nothing.

## What is still unread

| What | Why it is unread | What it would settle |
|---|---|---|
| STM32H7A3ZI datasheet | **st.com served no response from this bench**, three attempts, HTTP 000 | Whether `PD0` and `PD1` are FDCAN1, and the alternate function number |
| RM0455, the reference manual | the same | The 520 against 560 bit disagreement, and the FDCAN register detail |
| UM2408, the Nucleo-144 board manual | the same | Which connector pin carries `PD0` and `PD1` |
| B0505LS-1W | no manufacturer datasheet located | The isolation rating of the barrier |
| SI8642ED-B-IS | not fetched | The isolator's rated data rate and isolation voltage |
| `U16`, the RS232 isolator | the part number is not printed on the schematic | What it is |

**The ST failure is a reproducible bench fact, not bad luck.** Three documents,
three attempts each, all returning HTTP 000 with no response body. It matches
what this bench has recorded before. The fallback that has worked is ST's
published header repositories for register and pin definitions, which answer the
alternate function question even though they are not the datasheet.

So the four `[unconfirmed]` rows that gate putting a wire into the
NUCLEO-H7A3ZI-Q remain exactly where they were, and that gate stays closed.
That is the correct outcome: the documents that would open it are named, the
route that has worked before is named, and nothing has been guessed in the
meantime.

## The documents, with their addresses

| Document | Revision read | Address |
|---|---|---|
| SN65HVD230 | SLOS346K, February 2011 | `https://files.waveshare.com/upload/8/82/SN65HVD230.pdf` |
| MCP2561FD and MCP2562FD | DS20005284A | `https://ww1.microchip.com/downloads/en/DeviceDoc/20005284A.pdf` |
| MCP2518FD | as published | `https://files.waveshare.com/upload/c/c3/MCP2518FDT.pdf` |
| MCP2515 | as published | `https://files.waveshare.com/upload/8/83/MCP2515.pdf` |
| TCAN3413 and TCAN3414 | as published | `https://www.ti.com/lit/ds/symlink/tcan3413.pdf` |
| SC16IS752 | as published | `https://files.waveshare.com/upload/a/ad/SC16IS752_datasheet.pdf` |
| SP3481 and SP3485 | as published | `https://files.waveshare.com/upload/3/36/SP3481_SP3485.pdf` |
| SP3232EEN | as published | `https://files.waveshare.com/wiki/RS232-485-422-TO-CAN/SP3232EEN.pdf` |
| BCM2711 peripherals | as published | `https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf` |
| Raspberry Pi 4 datasheet | as published | `https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-datasheet.pdf` |
| WS-28164 schematic | as published | `https://files.waveshare.com/wiki/RS232-RS485-CAN-Board/RS232_RS485_CAN_Board_Sch.pdf` |
| STM32H7A3ZI datasheet | **not served** | `https://www.st.com/resource/en/datasheet/stm32h7a3zi.pdf` |
| RM0455 | **not served** | `https://www.st.com/resource/en/reference_manual/rm0455-stm32h7a3b3-and-stm32h7b0-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf` |
| UM2408 | **not served** | `https://www.st.com/resource/en/user_manual/um2408-stm32h7-nucleo144-boards-mb1363-stmicroelectronics.pdf` |

No datasheet is committed to this repository. They are the manufacturers' to
distribute, the addresses above are their own, and a page number plus a revision
letter is a more durable reference than a copy that silently goes stale. The two
RS485 and RS232 documents are listed for completeness and nothing in this
volume reads them.
