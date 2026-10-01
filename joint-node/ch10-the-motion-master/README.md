# Chapter 10: the motion master

The host side of the bus: the socket, the tools, and something for a node to
listen to. No node is involved and none is needed.

## What runs, and where

```bash
python test/test_frames.py      # anywhere
python test/test_record.py      # anywhere
python test/test_transport.py   # a real test on Linux, a clean skip elsewhere
```

The first two need nothing but Python. They cover the part that is wrong
silently: an identifier that overflowed into the flag bits, a payload length
that cannot exist on a wire, a flexible-data frame truncated to a classic one
because a socket option was never set.

The third needs Linux and the virtual interface. On the win11 aquamarine
authoring laptop it prints `skip` and exits zero, because this Python has no
`AF_CAN` at all. A skip that exits one trains people to ignore a red build, and
a skip that pretends to pass is worse, so it says which it did.

## The whole first half with no hardware

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Then two terminals:

```bash
python -m master.listen vcan0
python -m master.setpoint vcan0 --rate 200 --seconds 5
```

The only thing that changes when an adapter arrives is the interface name.
`Bus("vcan0")` becomes `Bus("can0")` and nothing else in this chapter knows the
difference. What the virtual interface cannot teach is timing, arbitration and
errors: frames on it have none of those, and no number taken here means anything
about a real bus.

## Record and replay

```bash
python -m master.listen vcan0 --out run.log --seconds 10
python -m master.setpoint vcan0 --replay run.log
python -m master.setpoint vcan0 --replay run.log --drop-every 3
```

The file is the format the standard logger writes, so a recording made here can
be read by tools that have never heard of this repository. Replay is what makes
a missing frame reproducible: chapter 11 demonstrates its sequence gap detection
by dropping a frame on purpose, and dropping the same frame twice is only
possible if the traffic came from a file.

## What this chapter does not do

It does not know what a frame means. Identifiers and bytes go in and out, and
the names belong to chapter 11's description. Keeping the two apart is what lets
these tools watch traffic from a node whose protocol they were never told.

`master/setpoint.py` reports the rate it achieved rather than the rate it was
asked for, and says so loudly when the two differ by more than five per cent. A
generator that claims 1000 Hz while delivering 780 makes everything measured
against it wrong.

## Acceptance criteria, and which are covered

| Criterion | Covered by |
|---|---|
| The socket layer works against the virtual interface | `test_transport.py`, on Linux only, **not yet run anywhere** |
| A recording can be read by the standard tools | `test_record.py`, against lines those tools produce |
| Replaying a file twice gives the same traffic twice | `test_record.py` |
| A frame of an impossible length is refused | `test_frames.py` |
| The setpoint source holds its rate | reported at run time, **never measured** |

The two marked not run need a Linux machine. Nothing in this chapter has been
exercised against a kernel yet.
