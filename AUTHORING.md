# Authoring guide for the joint-node volume

Read `sections/j01.tex` and `figures/j01_*.tex` first: they are the reference for
tone, depth, structure and figure style. Every chapter must compile alone with

    python build.py --check sections/jNN.tex

and finish with `== RESULT: CLEAN`. Then `python lint.py sections/jNN.tex` must
print `clean`, and before a release `python crosscheck.py` must report no
cross-chapter problems and `python build.py --drift
../EmbeddedFirmware_NucleoH7_Top20/build.py` must report the toolchain
identical outside the `DOC` block.

A label must never sit on top of a symbol or another label. Render every figure
and look at it before moving on. The linter cannot see this.

---

## WHAT MAKES THIS VOLUME DIFFERENT

The two sibling volumes are collections: twenty projects that happen to share a
parts bin. **This one is a single machine, built up over twenty chapters.**
Chapter 1 has a board on a desk. Chapter 20 has a joint node that runs a control
loop at a fixed period, carries sensors, talks on a field bus to a motion
master, publishes itself to middleware, enters a safe state when something goes
wrong, can be updated without being touched, and is tested by a rig that fails
the build when tracking degrades.

Three consequences for every chapter:

1. **A chapter adds a layer, it does not stand alone.** The required section
   **What the node gains** says, in two or three sentences, what the node could
   not do before this chapter and can do after. The metadata line at the top of
   the chapter carries the same thing in a phrase.
2. **Nothing is thrown away.** Code written in chapter 5 is still running in
   chapter 20. If a chapter needs to replace an earlier decision, it says so and
   explains what changed, which is itself worth reading.
3. **The repository layout section shows the whole tree**, with this chapter's
   additions marked, not just the files this chapter touches. A reader should be
   able to see the node growing.

---

## THE PART TRAP, CORRECTED FOR THIS VOLUME

The sibling volume opens with a warning that this part is not the member of its
family that most material online was written for, and that copied clock and
memory configuration produces a board that does not boot. That warning stands.

**For the bus peripheral specifically it is wrong, and the correction is more
interesting than the warning.** A line-by-line comparison of the vendor's own
device headers for the two parts shows the bus controller is the same licensed
core at the same addresses with the same register bit positions: two instances,
the same base addresses, the same calibration unit, the same message RAM base,
and identical bit-timing field positions. Material written for the popular
sibling's bus peripheral is **directly reusable here** at register and library
level.

What actually differs sits outside the peripheral, and a chapter should name all
three precisely:

1. **The clock-selection register has a different name.** The two parts put the
   bus kernel clock selector at the same two bit positions in a register whose
   name reflects each part's domain naming. This is the single most likely line
   to fail to compile when copying, and it is a good concrete hook.
2. **The power and clock domains are named differently**, two on this part
   against three on the sibling. The vendor's own header carries a comment
   mapping one onto the other.
3. **The maximum core frequency differs**, which changes the arithmetic that
   lands a clean kernel clock, not the registers that consume it.

There is a fourth difference and it is at board level rather than silicon. The
upstream real-time operating system's board file for the sibling board enables
the bus controller and names its two pins. The board file for **this** board
mentions the bus peripheral **not once**, because nothing on the board wires it
to anything. The silicon has it, the board does not expose it, and the upstream
project reflects exactly that. That is the transceiver chapter's opening
paragraph, written for you by someone else's device tree.

**Rule for this volume: check the claim before repeating it.** Where the sibling
warning applies, say so. Where it does not, say that too, and say how it was
checked. The headers are public and permissively licensed, so a reader can
repeat the comparison.

---

## THE HONESTY RULE

Three things a robotics firmware role asks for cannot be demonstrated on this
bench, and the book says so plainly rather than quietly omitting them.

| Missing | Why | What the book does instead |
|---|---|---|
| A real-time Ethernet fieldbus | The part has **no Ethernet controller at all**, and a slave needs a dedicated slave chip | Chapter 17 explains what a slave needs in hardware, what an ESI file is, what the open master stack does, and what would have to be bought. It teaches the architecture without pretending to run it |
| A brushless motor and drive stage | No motor, no driver board, no current sensing | Chapter 7 builds the firmware side in full: the timer configuration, complementary outputs, dead time, and a fault input. The plant is a model in software, so the control loop is real and the torque is not |
| A force or torque sensor | Not on the bench | Chapter 8 explains what the signal is, what it is used for, what a real one costs and what it would connect to, and stands in with a derived estimate whose error is stated |

Each of those chapters ends with the same structure: what is real here, what is
modelled, and what an honest answer sounds like when someone asks whether you
have done it. A chapter that blurs that line has failed.

**Never present a modelled result as a measured one.** The budget tables carry a
Measured column and it stays "not measured" until it has been. A number that
came from a model is labelled "modelled"; a number that came from arithmetic is
labelled "computed".

---

## NO EMPLOYER OR PRODUCT NAME

This volume was written with a particular advert open. It must not name the
company, the robot product line, or the town. A reader should be able to use it
for any robotics firmware application, and a named employer dates the work to
one vacancy that will close.

The linter enforces this. Refer to duties generically: "a distributed node
network", "the motion master", "a robotics firmware role". Where a chapter maps
to something an advert would ask for, say so in the generic form, for example
"this is the chapter that answers a request for CAN-FD network management".

---

## The twenty chapters

Numbering is fixed. Titles may be polished but not renamed in spirit.

| NN | Title | What the node gains | Theme |
|----|-------|---------------------|-------|
| 01 | What a joint node is, and the bench that stands in for one | An identity and a bring-up | Architecture, the stand-in bench, what is real and what is modelled |
| 02 | The control period: 1 kHz you can prove | A heartbeat | Timer-driven period, jitter, measuring it without a scope |
| 03 | One clock for sensors, loop and bus | A shared time base | Timestamping, a monotonic tick, what to do without precision time hardware |
| 04 | The board support package, and a board file you can hand over | A documented hardware interface | Pin map, clock config, peripheral table, a board file another engineer can read |
| 05 | The encoder: quadrature in hardware, and one you generate | Position | Timer encoder mode, index, velocity from differences, generating quadrature to test the decoder |
| 06 | The inertial unit as the joint's inner ear | Motion sensing | Rate and acceleration, FIFO, watermark, timestamp alignment with the control period |
| 07 | The actuator you do not have: PWM, dead time, and a plant model | A command output and a simulated joint | Complementary PWM, dead time, fault input, a second-order plant in software |
| 08 | Force and torque: the signal you cannot buy | An estimate, honestly labelled | What the signal is, what it is for, what a real sensor needs, and the error of the stand-in |
| 09 | CAN-FD from the controller out: bit timing and the first frame | A voice | Bit timing for both phases, the transceiver, sample point, the first frame on a wire |
| 10 | The motion master: the bus on Linux | A listener and a commander | Socket layer on the host, the command line tools, a Python setpoint source |
| 11 | A joint protocol: state and command in sixty-four bytes | A vocabulary | Frame layout, identifier allocation and priority, what belongs in a state frame |
| 12 | Network management: heartbeat, node state, bus-off and recovery | Membership | Heartbeat, node state machine, error counters, bus-off detection and recovery |
| 13 | Two speeds on one wire, and why the old node errors | A diagnosis | Mixed classic and flexible-data traffic, tolerance, what a non-tolerant controller does |
| 14 | The node as a middleware participant, and the agent that hosts it | A place in the robot | The embedded middleware client, the agent on the host, transport and footprint |
| 15 | Joint state and joint command as messages | A standard shape | The standard message types, units and conventions, mapping frames to messages |
| 16 | The loop closed over the bus: setpoint in, state out, following error | A closed loop | Setpoint injection, the controller, following error as the figure of merit |
| 17 | What a real-time fieldbus would change, and why it is not on this bench | An honest boundary | Slave hardware, cycle time, the open master stack, what would have to be bought |
| 18 | Safe states, and the workspace sensor that triggers one | A way to stop | Safe-state machine, latching, entry actions, a presence sensor as a trigger |
| 19 | Update over the bus: a node you can reach but not touch | Maintainability | Transport for update, a bootloader that stays addressable, versioning across a fleet |
| 20 | The rig: injected faults, tracking error, and a build that fails | Proof | Fault injection, tracking regression, a report that turns a build red |

Cross-reference as `Chapter 5`; `\ref{sec:j05}` works.

### Non-duplication with the sibling volumes

The firmware volume next door already covers, on the same board, and this one
does not repeat them. Reference them instead.

- Toolchain, startup, linker script, printf. Chapter 4 here assumes it.
- Ring buffers, interrupt receive, direct memory access receive, framing and the
  checksum unit. Chapter 11 uses them and points there.
- Caches and coherency, and which transfer engine reaches which memory. Chapter
  9 cites it as the reason a bus buffer is placed where it is.
- Transforms and filters. Chapter 6 cites it for filtering.
- The generic operating system task set and latency measurement. Chapter 2 here
  is about the control period specifically, not about the scheduler in general.
- Low power. This node is mains powered and the subject does not arise.
- The generic hardware-in-the-loop rig. Chapter 20 here is the motion-specific
  rig: setpoints, trajectories and tracking error.

---

## Chapter structure

All headings in this order, `\subsection*{...}`.

```
\project{NN}{Title}{What the node gains, in a phrase}{Theme}
\begin{keyfacts} \item[Adds to the node] \item[Peripherals] \item[Depends on]
  \item[Real or modelled] \item[Difficulty] n of 5 \item[Effort] ...
  \item[Deliverable] ... \end{keyfacts}
\subsection*{Why this chapter}                 2 to 3 paragraphs
\subsection*{Prior art and what to reuse}      table: Source | What it gives | What it does not | Licence
\subsection*{What the node gains}              2 to 3 sentences: could not before, can after
\subsection*{Parts from the inventory}         table: Part | Role | Interface
\subsection*{System architecture}              \diagram{jNN_arch}{...}
\subsection*{Peripheral configuration}         table: Peripheral | Mode | Clock | Pins | IRQ and transfers
\subsection*{Wiring}                           \diagram{jNN_wiring}{...}
\subsection*{Memory and timing budget}         table: Quantity | Budget | Measured | Margin
\subsection*{Firmware design (UML)}            \diagram{jNN_uml}{...}
\subsection*{Data flow (ASCII)}                \begin{asciiart} ... max 112 columns
\subsection*{Repository layout}                the WHOLE tree, this chapter's additions marked
\subsection*{Steps}                            \begin{steps} 7 to 10 steps
\subsection*{Build, flash and debug}           \diagram{jNN_timing}{...} + commands
\subsection*{Verification and acceptance criteria}
\subsection*{Variants}                         table: Axis | Variant | What changes | Cost | Built in full in
\subsection*{Pitfalls}
\subsection*{Best practices applied}
\subsection*{Stretch goals}
\subsection*{Roadmap and next steps}           sourced from SOURCE.md, never invented
\subsection*{Portfolio evidence}
\subsection*{Sources}                          two lists: Normative references, Reusable implementations
```

The fifth figure slot is `jNN_data`, not `mem`: the data figure is a frame
layout, a message structure, a memory map, a register field or a state machine,
whichever the chapter is about.

Target length: **14 to 16 PDF pages**, 500 to 800 lines of LaTeX.

### Figure assignment, so nothing is drawn twice

Frame layouts in 11 and 13. Bus topology in 9 and 12. Control loop block diagram
in 16, with a simpler one in 7. Safe-state machine in 18. Memory map only in 19.
Register bit fields in 9 (bit timing) and 5 (encoder mode). Scheduling lanes in
2. Sequence diagrams in 14 and 19.

---

## Inventory

**The node.** NUCLEO-H7A3ZI-Q: Cortex-M7 at 280 MHz, 2 MB dual-bank flash,
1312 kB of declared SRAM plus tightly coupled instruction memory, **two FDCAN
controllers on chip and no transceiver on the board**, **no Ethernet controller
at all**. Verified settled facts: the 32.768 kHz crystal is fitted; the
high-speed clock comes from the on-board debugger in bypass mode at 8 MHz,
multiplied to 280; LEDs are green on PB0, yellow on PE1, red on PB14; the user
button is on PC13; the virtual serial port is USART3.

Memory map, settled from two independent machine-readable sources that agree:
flash 2048 kB at `0x0800 0000`; tightly coupled data memory 128 kB at
`0x2000 0000`, **which the main transfer engines cannot reach**; main SRAM
1024 kB at `0x2400 0000` in three banks; second domain 128 kB at `0x3000 0000`;
low-power domain 32 kB at `0x3800 0000`.

Five more facts settled from the vendor's own headers during the prior-art
sweep. Each is in `SOURCE.md` with the evidence, and each contradicts something
a reader will find elsewhere, so no chapter may quietly assume the opposite.

- **No Ethernet controller, proven by absence.** In `stm32h7a3xx.h` the
  interrupt list runs from stream 4 of the second transfer engine straight to the
  bus calibration entry: the two vector slots that carry Ethernet on the popular
  sibling are simply not there. **The upstream operating system's device tree for
  this part nevertheless leaves an Ethernet node in place**, inherited from the
  shared family file and never deleted. It is a modelling artefact. A reader who
  greps upstream will conclude the opposite, and chapter 04 says so.
- **The bus peripheral is in the read-only-memory bootloader.** This part can be
  reflashed over CAN-FD on two pins of port H with no bootloader of its own, at
  a fixed pair of bit rates, and it is polled first in the detection chain.
  Chapter 19 opens on this rather than pretending the problem is unsolved.
- **The flash word is 16 bytes and the sector is 8 kB**, against 32 bytes and
  128 kB on the sibling. The comment block in the vendor's own flash driver
  describes the sibling's geometry and is wrong here.
- **The backup registers are in the tamper peripheral, not the clock
  peripheral.** Code ported from the sibling will not build.
- **There is no hardware encoder index on this part**, on the vendor's own
  statement in its community forum and confirmed by the absence of the macros.
  The index is an external interrupt plus software, and that is chapter 05.

**The motion master.** Raspberry Pi 4, with 3B+ and 3 as alternatives.

**Sensors on hand.** X-NUCLEO-IKS4A1 and IKS5A1 inertial and environmental
shields, one at a time on the Arduino header, never two. X-NUCLEO-53L8A1 with an
8 by 8 multizone ranging sensor. A three-axis accelerometer breakout. Only one
shield is fitted at once.

**Instruments.** The power profiler as an ammeter with eight digital inputs. The
eight-channel data acquisition hat on a Pi at 100 kS/s aggregate. The board's own
timers and cycle counter. **There is no oscilloscope and no logic analyser**, and
**no soldering iron**.

**To be bought, and the book says why.** A CAN-FD capable transceiver module and
two terminators, which is the only electrical gap between the board and a bus.
A host adapter that presents a socket interface on Linux; the book states that a
Windows-only adapter cannot serve as the motion master and names what to check.

**Deliberately absent, see the honesty rule.** Motor, drive stage, force or
torque sensor, fieldbus slave controller, encoder.

---

## LaTeX subset, line lengths, figure conventions, writing rules

Identical to the firmware volume next door. In short: the listed environments
only; ASCII-only code at 96 columns and ASCII art at 112; no `\section`,
`\footnote`, `\includegraphics` or custom packages in `sections/`; a comma in a
note title breaks the box; figures at most 16 cm by 12 cm; solid fills only
because figures render through a path where transparency is dropped; computed
coordinates must be braced.

Writing: no em or en dashes, no `--` in prose, no violent idioms, no mention of
tooling, **no employer or product name**, dates in full as weekday, day, month
and year, every measurement names its instrument or says it has not been taken,
every dependency names its licence.

### Robotics figure styles added for this volume

Field bus: `busline` `busstub` `busnode` `busmaster` `buslistener` `busterm`
`busoff` `buslabel`, with `\busdrop{x}{node}`.
Frame layout: `fid` `fdata` `fcrc` `fstuff` `fbyte`, with
`\framefield{x}{w}{style}{label}` and `\framebits{x}{w}{count}`.
Control loop: `ctrlblk` `plantblk` `sensorblk` `sumj` `ctrlline` `fbline`
`ctrllabel`, with `\sumnode{name}{x}{y}{top}{bottom}`.
The joint: `linkbody` `jointpin` `jointarc`.
Safe states: `runstate` `degraded` `safestate` `latched` `faultedge`
`recoveredge`.

Everything from the two sibling volumes is still available: block diagrams, UML,
bench art, memory maps, register fields, clock trees, interrupt vectors, cache
diagrams, signal timing and scheduling lanes.

---

## Confirm before writing

The board facts above are settled. These are not, and are written as questions
for the bench rather than as assertions until checked against the reference
manual RM0455, the board manual MB1363, or the part's datasheet.

1. Which pins carry the two bus controllers, and whether either collides with a
   fitted shield. The usual candidates are on port D and port A; confirm in the
   configurator and the board manual.
2. The bus controller's message RAM: how much there is, how it is divided
   between filters, receive buffers and transmit buffers, and whether it must be
   configured before use. This differs across the family and is a common cause of
   a controller that never transmits.
3. Whether a bus buffer placed in tightly coupled memory is reachable by the
   controller. The sibling volume establishes that the main transfer engines
   cannot reach it; confirm what that means for this peripheral specifically.
4. Which timers offer encoder mode on this part, which are 32 bit, and whether
   the chosen one is free once a shield is fitted.
5. Which timer offers complementary outputs with a dead-time generator and a
   fault input, and on which pins.
6. The transceiver's data rate, once bought, and its suffix. A 1 Mbit part still
   gives full 64-byte frames and the improved checksum, because frame format is
   invisible to the transceiver; what it costs is only the switch to a faster
   data phase. The chapter states which part was used and what it allowed.

Four items that were on this list are now settled, and each answer changed a
chapter. They are kept here because the way they were settled is the method the
rest of the list should follow.

7. **The host adapter.** Settled, and it moved a purchase. The inexpensive
   analyser ships, for its flexible-data variant, a precompiled closed library
   with a vendor interface of its own, its own frame structure rather than the
   kernel's, and no licence file at all. The standard command-line tools cannot
   see it. An Apache-2.0 alternative from a robotics vendor presents the mainline
   kernel interface and does genuine flexible-data rates. Chapter 10 names both.
8. **The embedded middleware client.** Settled: this board is on neither support
   tier, the nearest listed board is a different silicon line, and the client's
   own bus transport is implemented for the host operating system only. So it is
   a porting job, and the bus transport is a custom one the author writes. The
   published memory figure in circulation covers the middleware core alone;
   chapter 14 prints what the cost is made of instead of a single number.
9. **The update transport.** Settled: both a field-bus application-layer
   specification and an automotive diagnostic standard cover it, the object
   indices are verified from two independent primary documents, and the two
   sources disagree about block against segmented transfer. Chapter 19 shows the
   disagreement rather than picking silently.
10. **Where the prior art ends.** Settled by the sweep itself: seven places where
    the literature has nothing to say are listed at the end of `SOURCE.md`. A
    chapter that reaches one of them says so and presents its own construction as
    the author's, never as a citation.

---

## Sources and licences

See `SOURCE.md` for the prior-art pool, the licence categories and the roadmap
material. Two rules apply everywhere: never present generated vendor code as
reviewed source, and never recommend a dependency without naming its licence in
the chapter that introduces it.

---

## The variant matrix

### Where each variant is built in full

Generated from the manuscript rather than maintained by hand, so the guide
cannot drift from the chapters. One row per variant axis, with every chapter
that claims to build a variant on that axis in full. `crosscheck.py` reads this
table and reports any axis where two chapters claim the same variant.

| Axis | Built in full in |
|---|---|
| Acquisition | chapter 6 |
| Adapter | chapter 10 |
| Addressing | chapter 19 |
| Alignment | chapter 6 |
| Anti-windup | chapter 16 |
| Architecture | chapter 18 |
| Board description | chapter 4 |
| Command | chapter 11, chapter 15 |
| Console | chapter 1 |
| Correction | chapter 3 |
| Coupling | chapter 7 |
| Dead time | chapter 7 |
| Decode | chapter 5 |
| Delivery | chapter 9 |
| Derivative | chapter 16 |
| Detector | chapter 8 |
| Effort | chapter 15 |
| Estimator | chapter 8 |
| Execution model | chapter 2 |
| Filtering | chapter 11 |
| Frame format | chapter 9 |
| Harness | chapter 20 |
| Host | chapter 14 |
| Host code | chapter 10 |
| Identifier | chapter 11 |
| Identity | chapter 1 |
| Index | chapter 5 |
| Injection | chapter 20 |
| Interface | chapter 10 |
| Language | chapter 4 |
| Layout | chapter 11 |
| Loader | chapter 19 |
| Loop placement | chapter 16 |
| Loopback | chapter 9 |
| Mapping | chapter 15 |
| Membership | chapter 12 |
| Modulation | chapter 7 |
| Node number | chapter 12 |
| Numeric type | chapter 16 |
| Observability | chapter 20 |
| Operating system | chapter 1 |
| Output | chapter 16 |
| Participant | chapter 13 |
| Placement | chapter 14 |
| Plant | chapter 7 |
| Processing | chapter 6 |
| Protection | chapter 7 |
| Rate | chapter 14 |
| Receive | chapter 13 |
| Recovery | chapter 12 |
| Redundancy | chapter 6 |
| Reporting | chapter 8, chapter 12 |
| Resolution | chapter 5 |
| Safe state | chapter 18 |
| Source | chapter 5, chapter 8 |
| State machine | chapter 18 |
| Storage | chapter 19 |
| Target | chapter 20 |
| Time | chapter 12 |
| Time source | chapter 3 |
| Timestamp | chapter 6 |
| Timing | chapter 10 |
| Transceiver | chapter 9 |
| Transfer | chapter 19 |
| Transport | chapter 14 |
| Trigger | chapter 18 |
| Units | chapter 11 |
| Velocity | chapter 5 |
| Watchdog | chapter 18 |
| Way out | chapter 13 |

---
