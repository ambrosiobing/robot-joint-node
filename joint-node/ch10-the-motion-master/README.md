# Chapter 10: the motion master

The host side of the bus: the socket, the tools, and something for a node to
listen to. No node is involved and none is needed.

All C. This chapter was written in Python first, which was the wrong language
for it: the skill being built here is embedded Linux in C, and a Python
demonstration of SocketCAN teaches the kernel interface without teaching the
language the node will actually be written in. The port kept the one thing the
Python version had got right, which is the line between the part that needs a
kernel and the part that does not.

```
include/jn_frame.h   src/jn_frame.c    the frame on the wire, no kernel at all
include/jn_log.h     src/jn_log.c      the log format, no kernel at all
include/jn_bus.h     src/jn_bus.c      the socket, the only Linux-only file
app/jn_listen.c                        watch a bus, optionally record it
app/jn_setpoint.c                      emit at a chosen rate, or replay a file
test/test_frame.c  test/test_log.c  test/test_bus.c
doc/                                   the hardware, before any of it is wired
tools/check_findings.py                keeps the hardware pages honest
```

## The hardware, decided before it was wired

The code above is the host side of the bus and needs no hardware at all. The
hardware it will eventually run against is a Raspberry Pi 4B carrying a
Waveshare WS-28164, and five documents in `doc/` record what was established
about it, in the order you would want them:

| Document | What it is for |
|---|---|
| [doc/board-findings.md](doc/board-findings.md) | The inventory. Every device, every link, every jumper, the full 40-pin header map, the terminal order and the numbers, with a source marker on every line and the open questions listed |
| [doc/datasheet-notes.md](doc/datasheet-notes.md) | What the manufacturers specify, part by part, with a page number on every figure. Ten datasheets read; the three ST documents would not download |
| [doc/rewiring.md](doc/rewiring.md) | The argument. Everything that could be changed, each with a verdict and a reason, plus the two patterns behind six wrong readings |
| [doc/bench-bring-up.md](doc/bench-bring-up.md) | The sequence. Card, configuration, order of operations, and what loopback does and does not prove |
| [doc/first-light.md](doc/first-light.md) | **The measurements.** Three runs on Wednesday 7 October 2026 with photographs: internal loopback, the board wired to itself as a real two node bus, then this chapter's own tools finding that bus's ceiling. 278,513 frames, zero errors, and the volume's first measured frame length |

The first four were written before anything was wired, which is the whole point,
and the fifth is what happened when it was. Reading
the schematic's text layer took about two minutes per question and answered
several that had been queued for the bench, including one nobody had thought to
ask: there is a 1k resistor on the classic transceiver's slope control pin.

Then the datasheets were read, and six of the conclusions drawn from the
schematic alone turned out to be wrong, that one included. Both rounds are kept
visible rather than tidied away, because the difference between them is the
lesson: a schematic gives you the value, and only the datasheet tells you what
the value does.

**On diagrams.** Every figure in these documents is drawn here from the facts and
cites the vendor figure it corresponds to by number and page. Datasheet artwork
belongs to its manufacturer; the facts in it do not, and a redrawn figure can
show this board rather than a generic one.

```bash
python tools/check_findings.py
```

That checks what a program can check about those documents: that the headings
they promise are present, that all 430 claim rows still carry a provenance
marker, that **every `[datasheet]` row names the page it was read from**, that
the marker vocabulary has not drifted, and that every rewiring verdict is one of
the four words the document defines.

The page rule is the one worth explaining, because it was added in response to a
defect rather than designed in. Two figures in these pages turned out to be
typical values presented as maximums, and a third reversed a conclusion outright.
In all three cases the claim read plausibly and nothing in the row said which
page to go and check. Four characters converts an argument into a lookup.

It cannot check whether any of it is true. The schematic and the datasheets are
the authority, the bench is the tiebreaker, and that is why the open questions
table exists.

## What runs, and where

```bash
make                 # build everything and run all three tests
make test            # the tests alone
./build/test_frame   # anywhere there is a compiler
./build/test_log     # anywhere there is a compiler
./build/test_bus     # a real test on Linux, a clean skip elsewhere
```

The first two need nothing but a C11 compiler. They cover the part that goes
wrong silently: an identifier that overflowed into the flag bits, a payload
length that cannot exist on a wire, a flexible-data frame truncated to a classic
one because a socket option was never set.

`test_frame` also checks something the Python version could not have got wrong.
A wire buffer on the stack holds whatever was there before it, so a frame that
writes only the bytes it knows about will send the remains of the previous call,
and only on some runs. The test dirties the buffer with `0xA5` first and then
reads the two reserved bytes and the whole unused tail of the payload. If they
are not zero, the encoder is leaking.

`test_bus` needs Linux and the virtual interface. Everywhere else it prints
`skip` with the reason and exits zero. A skip that exits one trains people to
ignore a red build, and a skip that prints `ok` is worse, so it says which of
the two happened.

## The whole first half with no hardware

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Then two terminals:

```bash
./build/jn-listen vcan0
./build/jn-setpoint vcan0 --rate 200 --seconds 5
./build/jn-setpoint vcan0 --rate 200 --seconds 5 --payload alternating
```

The only thing that changes when an adapter arrives is the interface name.
`jn_bus_open("vcan0", 1)` becomes `jn_bus_open("can0", 1)` and nothing else in
this chapter knows the difference. What the virtual interface cannot teach is
timing, arbitration and errors: frames on it have none of those, and no number
taken here means anything about a real bus.

## Record and replay

```bash
./build/jn-listen vcan0 --out run.log --seconds 10
./build/jn-setpoint vcan0 --replay run.log
./build/jn-setpoint vcan0 --replay run.log --drop-every 3
```

The file is the format the standard logger writes, so a recording made here can
be read by tools that have never heard of this repository. Replay is what makes
a missing frame reproducible: chapter 11 demonstrates its sequence gap detection
by dropping a frame on purpose, and dropping the same frame twice is only
possible if the traffic came from a file.

Timestamps are carried as whole nanoseconds rather than as a floating point
number of seconds. Six printed digits of a 2026 timestamp is about seventeen
significant figures, which a double does not have, so the Python version lost the
last digit or two on every round trip through a file. The line still carries
microseconds, because that is what the format has, and `test_log` compares at
microsecond resolution rather than pretending otherwise.

## What this chapter does not do

It does not know what a frame means. Identifiers and bytes go in and out, and
the names belong to chapter 11's description. Keeping the two apart is what lets
these tools watch traffic from a node whose protocol they were never told.

`--payload` is the one place that line gets interesting. The four modes are
equally meaningless to a receiver and differ only in run structure, which is
exactly what CAN's bit stuffing charges for. So a choice this chapter insists is
semantically empty turns out to change how many frames a second the bus will
carry, by about ten per cent. Opaque bytes are not free bytes.

`jn-setpoint` reports the rate it achieved rather than the rate it was asked
for, and says so plainly when the two differ by more than five per cent. A
generator that claims 1000 Hz while delivering 780 makes everything measured
against it wrong, and it is the quiet sort of wrong, because the number printed
is the one that was typed in.

## Acceptance criteria, and which are covered

| Criterion | Covered by |
|---|---|
| A frame of an impossible length is refused | `test_frame`, run in the workflow |
| The wire buffer carries nothing but the frame | `test_frame`, buffer dirtied first |
| A recording can be read by the standard tools | `test_log`, against lines those tools produce |
| Replaying a file twice gives the same traffic twice | `test_log` |
| The socket layer works against the virtual interface | `test_bus`, run in the workflow against `vcan0` |
| A kernel filter drops what it should and nothing else | `test_bus`, by a second receive that must time out |
| The setpoint source holds its rate | reported at run time, **never measured** |

There is no C compiler on the win11 aquamarine authoring laptop, so nothing in
this directory has ever been built there. It is built by the workflow in
`.github/workflows/checks.yml` and on the Pi 4.

`test_bus` ran on the board itself for the first time on Wednesday 7 October 2026,
and `jn-listen` and `jn-setpoint` were pointed at a real two node CAN bus the
same day: 1000 frames sent, 1000 received, 1000 logged, and then a rate ramp that
found the bus ceiling. See [doc/first-light.md](doc/first-light.md) part three.

`test_bus` is a real test as of Thursday 1 October 2026 and no longer a skip.
GitHub's runner image ships a kernel with no `vcan` module, so the first
`modprobe vcan` fails; the workflow then installs `linux-modules-extra` for the
running kernel and the second `modprobe` succeeds. The interface comes up with
an MTU of 72, which is the flexible-data size, and the test sends a 24 byte
flexible-data frame through the kernel and gets every byte back. That is the
socket option, the 72 byte layout and the filter all exercised by a kernel
rather than asserted.

What the virtual interface still cannot prove is anything about a wire. It has
no arbitration, no bit timing and no error frames, so the Pi 4 with the isolated
adapter remains the only way to learn whether the bus works, as opposed to
whether this code speaks to a kernel correctly.

And there is a nearer step than waiting for the node end, which came out of
reading the adapter's schematic rather than out of planning. The WS-28164
carries **two** complete CAN controllers, each with its own clock, transceiver
and terminal positions, so two jumper wires turn the board into a genuine two
node bus: real arbitration, real error frames, real termination, and a frame
that is actually acknowledged by somebody. It has to run classic, because the
second controller is an MCP2515, so it proves nothing about flexible data
frames. Everything else these tools do, it proves.
[doc/rewiring.md](doc/rewiring.md) sets out what that buys and what it does not,
and recommends it.
