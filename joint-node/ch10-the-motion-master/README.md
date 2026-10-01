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
```

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
