# Rewiring: what could be changed, and what should be

The previous page, [board-findings.md](board-findings.md), is an inventory. This
one is the argument. Having found every link, strap, jumper and spare pin, the
honest next question is whether any of them should move, and the honest answer
for most of them is no. The interesting part is the reasons, because a reason
survives into the next project and a verdict does not.

One principle runs through all of it, and it is worth putting first so that the
rest reads as an application of it rather than as a series of moods.

> **On a bench with no soldering iron, a change in copper is not a change, it
> is a commitment.** Configuration can be undone in a text editor at no cost.
> A zero ohm link that has been lifted stays lifted. So the bar for touching
> copper is not "would this be slightly better", it is "is this the only way to
> get something I actually need".

There is a second, quieter principle that took a while to earn:

> **The board's defaults are somebody's tested path.** Every fitted link on
> this board matches the configuration block on the vendor's wiki. When the
> defaults and the documented configuration agree, departing from either means
> leaving the only combination anybody has confirmed works. That is sometimes
> right. It is never free.

Each proposal below carries a verdict. The vocabulary is small on purpose:

| Verdict | Means |
|---|---|
| **Do** | Worth doing, and the reason it is worth it is stated |
| **Do not** | Worth not doing, and the reason is stated so it can be reconsidered |
| **Later** | Right in principle, blocked on something named |
| **Open** | Genuinely undecided, with the thing that would decide it named |

---

## The one that changes the plan

### Wire the board to itself and get a real two node bus today

**Verdict: Do.**

This is the proposal that came out of reading the inventory, and it was not
obvious beforehand, so it is worth setting out carefully.

The WS-28164 carries **two complete CAN controllers**, each with its own
oscillator, its own transceiver and its own pair of terminal positions. The CAN
FD channel is an MCP2518FD with an MCP2562FD on terminal positions 4 and 5. The
classic channel is an MCP2515 with an SN65HVD230 on terminal positions 1 and 2.
They share nothing except the SPI bus, the supply and the ground.

A CAN FD controller can be configured to send classic frames. So if you join
the two channels with two short wires, you have a **genuine two node CAN bus**:

```
terminal position 1  (H1, classic high)  to  terminal position 4  (H2, FD high)
terminal position 2  (L1, classic low)   to  terminal position 5  (L2, FD low)
```

Both jumper caps to `120R`, on `J1` and `J2`, because the two channels are now
the two ends of a very short bus.

What that buys, and this is the part worth dwelling on, is everything the
virtual interface cannot do:

| Thing | On `vcan0` | On this two node bus |
|---|---|---|
| Frames arrive | yes | yes |
| Bit timing is real | no | **yes** |
| Arbitration between two transmitters | no | **yes** |
| Error frames when something disagrees | no | **yes** |
| A transceiver, a termination and a wire | no | **yes** |
| Transmit acknowledgement from another node | **no** | **yes** |

That last row is the one that matters most and is easiest to overlook. On a
one node bus, nothing acknowledges a frame, so every transmission fails and the
controller retries forever. It is the classic first hour of a CAN bring up, and
on a single node bus it is not a fault, it is physics. A second node removes the
whole category.

What it costs: two jumper wires, two jumper caps, and about five minutes.

What it does **not** prove, and this list is as important as the one above:

- **Nothing about CAN FD.** The MCP2515 is a classic CAN 2.0B controller. It
  cannot do flexible data frames at all, so the bus has to run classic. The
  rate switch, the 64 byte payload and the four bit length code are all
  untouched by this.
- **Nothing about isolation.** Both nodes sit on the same isolated rail and the
  same `SGND`. There is no ground offset between them because they are the same
  ground.
- **Nothing about cable length, reflections or real world noise**, because the
  wire is 40 millimetres of jumper.
- **Nothing about the node end.** The Nucleo and its firmware are still exactly
  as far away as they were.

So this is not a substitute for the real bus. It is a strictly better substitute
for `vcan0`, available immediately, with no soldering and no parts. Chapters 10,
11, 12 and 19 can leave the virtual interface for it, and chapter 9's step 5 and
chapter 13 cannot.

The device tree for it is both wiki lines rather than one:

```
dtparam=spi=on
dtoverlay=mcp2515,spi0-0,oscillator=16000000,interrupt=23
dtoverlay=mcp251xfd,spi0-1,interrupt=24
```

And this is the configuration where the three numbering schemes bite, so the
interface identity check from the findings page stops being pedantry and becomes
a required step:

```bash
for i in /sys/class/net/can*; do
  echo "$i -> $(basename $(readlink -f $i/device/driver))"
done
```

**One honest caveat about the order of work.** Bringing up two interfaces at
once is exactly what the findings page argues against, because a failure then
has two places to be. The resolution is sequence, not principle: bring up the
CAN FD channel alone first and confirm internal loopback, then add the classic
channel, then join the wires. Three steps, each with one new thing in it.

---

## The zero ohm links

All six movable links need a soldering iron. There is no soldering iron on this
bench, and acquiring one is already ranked behind a multimeter in the bench's
own list of what to buy next. So every verdict in this section would be the same
even if the engineering were different, and it is worth separating the two
reasons rather than letting the missing tool do all the work.

### Move the CAN FD interrupt from GPIO 24 to GPIO 13

Lift `R36`, fit `R37`.

**Verdict: Do not.**

The engineering reason, which stands on its own: GPIO 24 is wanted by nothing
else. Nothing in this volume, nothing in the twenty projects, and nothing on
the bench competes for it. A move would free a pin that is not in demand and
would cost the agreement between this board and every configuration example
published for it. Changing a working assignment to no benefit is how a board
becomes undocumented.

The tooling reason: no iron.

### Move the classic CAN interrupt from GPIO 23 to GPIO 22

Lift `R35`, fit `R21`.

**Verdict: Do not.**

There is a real argument for this one, which is why it gets more than a line.
GPIO 23 collides with the `i2c6` overlay, and freeing it would let `i2c6` be
enabled alongside the classic CAN channel.

The argument fails on demand. Nothing on this bench wants `i2c6`. The Pi has
`i2c1` on GPIO 2 and 3, which this board does not touch at all, and that bus is
enough for every I2C part in the inventory. Moving a working interrupt to
unblock an overlay nobody needs is effort spent against an imagined
requirement.

And it would move the interrupt onto GPIO 22, which is wanted by `i2c6` as
well. The conflict would be relocated rather than removed.

### Move the serial expander interrupt from GPIO 25 to GPIO 26

Lift `R42`, fit `R53`.

**Verdict: Do not.** The RS485 channels are not used by this volume at all, so
this is a change to a path that is switched off.

### Lift `R43` and `R44` to isolate the board from the Pi's 5 V

**Verdict: Do not, and understand why before agreeing.**

These two fitted zero ohm links are the board's entire supply path from the Pi.
Lifting them has a real use: it separates the two 5 V rails, so the board can be
powered from its DC terminal without that supply reaching the Pi, and the Pi can
be powered from USB-C without the board drawing from it.

But lifting them means the board has **no supply at all** unless the DC terminal
is connected. The convenience that makes this bench work is that one USB-C cable
brings up the Pi, both CAN controllers, both transceivers and the isolated side.
Trading that for a separation nobody has asked for is the wrong direction.

The case where it would be right is a permanent installation fed from an
industrial supply, where back feeding a Pi's 5 V rail through two zero ohm links
is a thing a reviewer would object to. That is not this bench, and when it is,
the board will be in an enclosure and somebody will have an iron.

### Change the RS485 direction control links

`R10` against `R11`, and `R18` against `R22`.

**Verdict: Do not.** Same reason as the RS485 interrupt: an unused path.

---

## The one fitted component that is genuinely tempting

### Replace `R3`, the 1k on the SN65HVD230's `Rs` pin, with a link to ground

**Verdict: Do not. Open as a measurement.**

`R3` is the finding nobody was looking for, and it has exactly the shape that
invites a fix. The board's classic CAN transceiver is deliberately slew limited.
Grounding `Rs` would give it its full speed edges, shorten its loop delay by
something like thirty nanoseconds, and remove a number from the bit timing
budget that currently cannot be stated precisely.

Four reasons not to, in increasing order of how much they would persuade a
reviewer:

1. **No iron.** Mechanical, and the least interesting.
2. **It is not a fault.** A board sold with screw terminals, isolation and a
   DIN rail enclosure, slew limiting its classic channel, is making a
   considered choice about emissions on long industrial wiring. Treating a
   deliberate design decision as a defect because it complicates one
   calculation is backwards.
3. **The channel it affects is not the one this volume uses.** Chapter 9's bus
   is CAN FD, through `U6`, a different part. `R3` is on the classic channel.
   Even the two node proposal above runs that channel at 500 kbit/s, where a
   bit is 2000 nanoseconds long and a hundred nanoseconds of loop delay is five
   per cent of a bit. It is not close to a limit.
4. **It is a better measurement than a modification.** The interesting question
   is not "can the edges be made faster", it is "how much does a 1k on `Rs`
   actually cost in loop delay". The datasheet gives the grounded case and the
   10k case and leaves 1k between them. An oscilloscope would answer it in one
   capture, and there is no oscilloscope on this bench either, which makes it a
   clean entry on the instruments list rather than a half answered question.

So the verdict is to leave it fitted and record it as an `[unconfirmed]` number
with a named way to settle it. Which is what the findings page does.

---

## Configuration, where the bar is much lower

### Enable both CAN channels at once

**Verdict: Later, after each works alone.**

Covered under the two node proposal above. The sequence is CAN FD alone, then
classic alone or added, then both with the interface identity check run and its
output written down.

### Enable the RS485 channels

**Verdict: Do not, for now.**

The wiki's configuration block has six lines. Four of them bring up RS485 and
RS232. Leaving those four out of a first bring up is not timidity, it is the
whole method: when the bus does not come up, the number of candidate causes
should be one.

There is no reason in principle not to add them later. `sc16is7xx` is in
mainline, the board straps the part into SPI mode with a fitted 1K pull up, and
the channels have their own terminal positions, terminations and protection.
They are simply not what this volume is about, and the two SPI buses do not
interfere with each other.

### Enable the RS232 channel

**Verdict: Do not. This one has a cost that is easy to miss.**

The RS232 channel uses header pins 8 and 10, which are GPIO 14 and GPIO 15,
which are the Pi's serial console. There is no way to have both. Enabling the
RS232 channel means giving up the console, and on this bench the console is
already the subject of a separate recorded problem, so losing it for a second
and unrelated reason would be genuinely confusing six months from now.

Nothing in this volume needs RS232. The console wins, and the decision costs
nothing because there was no competition.

### Enable every interface, as a first attempt did

**Verdict: Do not, and the findings page has the table.**

Two of the five overlays in that attempt are fatal to this board: `i2c4` takes
GPIO 7, which is the CAN FD chip select, and `i2c6` takes GPIO 23, which is the
classic CAN interrupt. With `i2c4` enabled, the CAN FD interface never appears
no matter how correct everything else is.

Worth noting what the correction was worth. The original conclusion was right,
the reasoning behind two of its five rows was wrong, and finding that out
required reading the header map rather than reasoning about it. A right answer
with a wrong reason will be applied wrongly to the next board.

### Power the board from the DC terminal instead of the Pi's USB-C

**Verdict: Do not, and there is one thing to establish first.**

The board does not need it. `U12` is fed from the Pi's 5 V, so the isolated side
comes up on USB-C alone, and that is the finding that removed a whole class of
imagined problem from this bring up.

Before anyone ever does connect it, the open question from the findings page has
to be closed: whether the DC supply feeds the Pi back through `R43` and `R44`.
If it does, and the topology says it probably does, then connecting the DC
terminal while USB-C is plugged in puts two supplies in parallel on one 5 V
rail. That is a thing to do deliberately or not at all.

The test is one power cycle with USB-C out and DC in. It takes a minute, and it
would turn an `[inferred]` row into a `[measured]` one.

### Switch the serial expander to I2C mode

`R34` straps `U10` pin 9 high, selecting SPI.

**Verdict: Do not.** It is a fitted strap, so it needs an iron, and SPI is what
the mainline overlay line expects. There is no benefit on offer.

### Fit a HAT ID EEPROM so the board is auto detected

`U14`, `R45` through `R49`, `R54` and `C43` are all present as footprints and
all unfitted.

**Verdict: Do not, and it is less attractive than it sounds.**

Auto detection would replace a hand written `dtoverlay=` line with a blob
programmed into an EEPROM. For a product that is a clear improvement. For a book
whose subject is what the device tree does, the hand written line **is** the
documentation, and hiding it inside an EEPROM would make the chapter worse.

---

## Termination, which is wiring rather than rewiring

Not a modification, because the caps move by hand, and the most frequent source
of confusion on a new CAN bus, so it earns a section.

The rule: **120 ohms at the two ends of the bus, and nowhere else.**

| Arrangement | `J1`, CAN FD | `J2`, classic | A third node |
|---|---|---|---|
| Pi plus Nucleo, two nodes | `120R` | not in use | |
| The two node self bus above | `120R` | `120R` | |
| Pi, Nucleo, plus a listener in the middle | `120R` | not in use | **`NC`** |
| Pi and Nucleo, listener at one end | `NC` if the listener terminates | | `120R` |

Three terminations on a two node bus is the usual mistake, and it does not
produce a clean failure. It produces a bus that works at 125 kbit/s, works
mostly at 500 kbit/s, and falls apart in the data phase at 2 Mbit/s, which is
the hardest possible symptom to attribute.

**And the thing a document cannot tell you**: where the caps are right now. A
jumper position is a state, not a specification. `J1` either has its cap on
`120R` or it does not, and the only instrument that can read it is a pair of
eyes. That is why "look at `J1` and write down what you see" is a step in the
bring up and not a line in a table.

---

## The node end, where the decision is a purchase

### Use the loose SN65HVD230 at the Nucleo end

**Verdict: Do, with its ceiling stated.**

It is the transceiver that is on the bench, it is a 3.3 V part which matches the
STM32's pins, and it makes the bus real for chapters 9, 10, 11, 12 and 19. Its
rated 1 Mbit/s is comfortably above the planned 500 kbit/s arbitration rate.

### Buy an FD rated transceiver for the Nucleo end

**Verdict: Later, and it is the only thing chapter 13 is waiting for.**

Chapter 13's subject is two speeds on one wire. A bus runs at the rate every
node can manage, and the loose SN65HVD230 is rated 1 Mbit/s with no rate switch.
The HAT's own CAN FD channel is not the constraint. `U6` is an FD rated part.

So the constraint is one component at one end, and no amount of configuration
moves it. On the bench's own list of what to buy, this sits behind a multimeter,
which unblocks more acceptance tests across more projects, and ahead of most
other things.

**The named candidate is the TCAN3413**, a 3.3 V CAN FD transceiver, which is
the part the feaser CAN FD shield design uses:

```
https://www.ti.com/lit/ds/symlink/tcan3413.pdf
```

Three things to check in that datasheet before ordering, because they are the
three that decide whether it drops in where the SN65HVD230 currently sits:

1. **The supply.** A 3.3 V single supply part needs no second rail from the
   Nucleo, which is the whole reason it is the candidate rather than a 5 V
   transceiver.
2. **The loop delay.** This is the number chapter 9 step 5 wants for transmitter
   delay compensation, and the reason the SN65HVD230's 1k `Rs` resistor is an
   open question. A part whose loop delay is specified plainly at the data rate
   in use would close that question by replacing it.
3. **Whether it has a slope control pin at all.** The SN65HVD230's `Rs` pin is
   the source of two separate unknowns on this bench. A part without one has
   one fewer way to look dead while reading perfectly.

It is a loose chip rather than a board, so a module or a breakout matters as
much as the part: there is no soldering iron here, so an unmounted device in a
small surface mount package is not usable, however correct it is.

### Run the whole bus classic, at 500 kbit/s, and drop CAN FD

**Verdict: Do not.**

It would work, it would be simpler, and it is the wrong book. The volume's
subject is CAN FD from the controller out, and chapter 9's bit timing, message
memory and frame length code all exist because flexible data frames have a
second bit rate, a 64 byte payload and a four bit length code that is not a
length. Classic CAN has none of those.

The two node self bus above is classic, and that is an acknowledged limitation
of a stop gap rather than a change of subject.

---

## What the spare pins could become

Seven GPIO lines are untouched by this board: GPIO 2, 3, 4, 5, 6, 12 and 27.
GPIO 2 and 3 are the Pi's main I2C bus, so an accelerometer or a ranging sensor
would fit there electrically with no conflict at all.

**Verdict: Later, blocked on a 2 by 20 stacking header.**

The board is a 40-pin HAT. It covers the header. Electrical freedom is not
physical access, and the stacking header that would provide it is not on this
bench. It is already on the list of things to buy, where another project also
wants it.

Worth recording as a capability rather than a plan, because knowing that seven
pins and a whole I2C bus survive this HAT is the sort of thing that makes a
later design possible instead of impossible.

---

## Reflections on how the three wrong readings happened

Three statements on this bench turned out to be wrong, and all three were
corrected by the same method. The pattern is more useful than the corrections.

| What was believed | What is true | How it went wrong |
|---|---|---|
| `R32`, `R33`, `R34` route the controllers to SPI0 and the expander to SPI1 | They are 1K pull ups on the SC16IS752's `RESET`, `IRQ` and `I2C/SPI` pins. The SPI routing is hard wired | A schematic block titled **SPI SELECTION** was read as if it selected. It has no resistors in it at all |
| `Y2` is a 40 MHz crystal | It is a packaged 40 MHz oscillator, 3.3 V, plus or minus 20 ppm, with an output enable pin | A two pin crystal was the expected thing in that position, so the four pin symbol was not looked at |
| `i2c_vc` and `spi3-1cs` conflict with the HAT's ID EEPROM | There is no ID EEPROM. Every part in that block is marked `NC` | The footprint was seen and fitment was not checked |
| `R36` straps `CAN1_INT` to either `D23` or `D24` | `R36` links `CAN1_INT` to `D24`. The alternative is `R37` to `D13`. `D23` belongs to `R35` and the **other** channel | Two link pairs in one block were read as one pair, which merged two channels into one sentence |

The common shape: **a name was trusted over a reading.** "SELECTION" promised a
choice, the crystal position promised a crystal, a footprint promised a part,
and a block containing `D23` and `D24` promised they were alternatives for the
same signal.

Each correction cost two minutes with the text layer. Each belief, left alone,
would have cost an evening at the bench, and two of them would have produced
symptoms that looked like software.

So the practice this leaves behind, which is the thing most worth carrying to
the next board:

1. **Read the designator, its value, and the pin it lands on.** Three facts, in
   that order. Any one of them alone invites a story.
2. **Check fitment, not just presence.** `NC` is a value.
3. **Count the link pairs before describing any of them.** A selection block
   with four resistors is probably two choices, not one.
4. **Prefer a reading to an expectation, even when the expectation is
   reasonable.** A two pin crystal in that spot was a good guess. It was still
   a guess, and the drawing was right there.

And the encouraging half, because this is not a cautionary tale. Reading a
schematic through its text layer turned out to be fast, repeatable and
checkable, and it answered more questions than anybody expected to be
answerable without touching the board: the supply topology, every termination
designator, every interrupt option, the whole header map, the terminal order,
and a slope control resistor nobody had thought to ask about. The method is in
the findings page under "How to re-derive all of this in about two minutes", and
it is genuinely worth trying on the next unfamiliar board before any other step.

## The order of work this leaves

1. Look at `J1`, `J2`, and the ribbon stripe. Write down what you see.
2. Boot with the HAT off, to separate the card from the adapter.
3. Fit the HAT alone, enable the CAN FD channel alone, read `dmesg`.
4. Internal loopback on the CAN FD channel, and record the sample points.
5. Add the classic channel, run the interface identity check, record the answer.
6. Join positions 1 to 4 and 2 to 5, both caps on `120R`, and run chapter 10's
   tools against a real two node bus.
7. Settle the three unread nets, `U6` `STBY`, `U7` `EN1` and `EN2`, and `Y2`
   `OE`, from the schematic drawing.
8. Confirm `PD0` and `PD1` against UM2408 before any wire enters the Nucleo.

Steps 1 through 6 need nothing that is not already here. Step 7 needs ten
minutes. Step 8 is the gate on the real bus, and it is a reading, not a
purchase.
