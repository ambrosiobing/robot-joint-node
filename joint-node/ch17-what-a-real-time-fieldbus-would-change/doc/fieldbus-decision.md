# Fieldbus decision: what a deterministic fieldbus would change, and why it is not here

Chapter 17's only deliverable. Nothing was built for this chapter and nothing
could be: the decision is the work.

Every claim below carries where it came from, because a decision document whose
provenance is invisible is an opinion with a table around it:

| Marker | Means |
|---|---|
| `[quoted]` | a sentence copied from a document that was opened |
| `[read]` | read from a document that was opened, not quoted word for word |
| `[measured]` | measured in this volume, on this bench |
| `[unconfirmed]` | looked for and not settled, and written as open rather than guessed |

## What it would buy

| Gain | Figure | Source |
|---|---|---|
| Synchronisation across joints | one to two orders better than this volume achieves | `[read]` the implementation guide claims much better than 1 microsecond, and one controller vendor claims better than 100 nanoseconds |
| What this volume achieves instead | single-digit microseconds | `[measured]` chapter 3, in software, over the bus already here |
| How the gain is obtained | in hardware: 64 bit system time in nanosecond units, propagation delay measured, drift compensated | `[read]` |
| Process data ceiling | about 12.5 megabytes per second | `[read]` published, and generous for a joint |
| Integration with a robot framework | a drive profile the framework already speaks | `[read]` |

Single-digit microseconds in software is a good result and chapter 3 earned it.
It is still one to two orders short of what the hardware does, and the gap is
not closable by writing better software on this part.

## What it would cost

| Item | Cost | Source |
|---|---|---|
| Membership | free of charge | `[read]` |
| The vendor identifier | **free of charge**, and a machine builder integrating devices is explicitly not required to obtain one | `[read]` |
| The conformance test tool | **a paid annual subscription**, and its in-house use "is mandatory when selling the device to the market" | `[quoted]` from the implementation guide |
| Development effort | **six to eight weeks** to a working subdevice | `[read]` the technology group's own estimate, not this volume's |
| Parts for one node | a controller, a configuration memory, two physical layer devices, magnetics, connectors and passives | `[read]` |
| Assembly | not possible on this bench | `[measured]` there is no reflow here, and the packages are not hand solderable |

**Two assumptions are worth correcting, because nearly everyone arrives with
them.** The identifier people worry about is free, and not even required for a
machine builder integrating devices. The cost that actually bites is a recurring
subscription to a test tool, it attaches to **selling** rather than to building,
and almost nobody raises it.

## Why not here

The premise was settled from documents rather than from impressions, and both
findings are structural rather than matters of effort:

| Finding | Source |
|---|---|
| This part has no Ethernet controller | `[read]` chapter 4 established it from the **absence** of two interrupt vector slots in the vendor's own header, which is stronger evidence than a product page |
| This silicon vendor sells no subdevice controller at all | `[read]` |

The asymmetry behind both is one sentence, and it explains why a hobbyist can
write the controlling end in an evening and not the other end:

> "The only hardware requirement for an EtherCAT MainDevice is a standard
> Network Interface Controller (NIC, 100 Mbit/s full duplex)." `[quoted]`

The other end needs a dedicated controller chip, a configuration memory, and per
port a connector, magnetics, a physical layer device and passives, with two
ports minimum. `[read]`

So this is a boundary, not a gap in the work. No amount of firmware on this part
reaches the other side of it.

## What would change it

Three routes, and each is costed rather than waved at. None is built.

| Route | What changes | What it costs | Source |
|---|---|---|---|
| Keep this part, add a controller chip | the most instructive: the node stays and the bus arrives beside it | a chip, a memory, two physical layer devices, magnetics, connectors, and assembly this bench cannot do | `[read]` for the parts, `[measured]` for the assembly |
| A controller with a core inside it | the cleanest: one chip is both the participant and the processor | a different board and a different core, so fifteen chapters of this volume move | `[read]` for the part, and the fifteen chapters are this volume's own count |
| Soft, on programmable logic | flexible and protocol switchable | tied to one vendor's certified stack, and a different class of part altogether | `[read]` |

**One part is built for exactly this problem**, and its own feature list says so
without interpretation: three channel pulse width modulation, step and direction
control, incremental and Hall encoder interfaces, and an emergency stop input,
with both physical layer devices integrated, in an eighty pin package. That is a
joint node's entire peripheral set inside the bus controller. `[read]` from a
datasheet at version 1.09 of Thursday 12 December 2024.

The controller overview also shows which parts are current and which are not:
two parts that appear in older articles are absent from the current overview and
one product page returns not found, so a new board should not be designed around
them. `[read]`

## Licence findings, which will have moved since anyone last looked

These are the most perishable claims here and the most likely to matter.

| Component | Position | Source |
|---|---|---|
| The portable open controlling stack | GPLv3 since version 2.0.0 on Friday 11 July 2025, with a commercial option and a sentence saying a commercial product likely needs one. It was GPLv2 with a linking exception before that | `[read]` |
| Why automated checks miss that | the file is named `LICENSE.md`, so scanners report nothing and give false comfort | `[read]` |
| The kernel-space controlling stack | a GPLv2 file and an LGPLv2.1 file at the root, and the readme does not say which covers which half | `[unconfirmed]` the conventional split is assumed and could not be confirmed |
| The open participant stack | GPLv2 with a linking exception, which is the arrangement the controlling stack left behind | `[read]` the asymmetry is worth a sentence in any evaluation |
| The silicon vendor's stack | free of charge, gated behind membership, redistribution restricted. The gate is membership, not an identifier | `[read]` |

**The trap, and it generalises far beyond this bus.** The robot framework bridge
is permissively licensed, actively maintained, and lets devices be described in
parameter files instead of in C++ per device, which is genuinely good design.
It sits on the kernel-space stack: out-of-tree kernel modules under copyleft,
whose installation procedure requires **disabling secure boot** to load unsigned
modules. `[read]`

A permissive licence on the wrapper does not remove the copyleft from the stack
that gets deployed, and the secure boot requirement is a deployment decision
somebody senior should make rather than discover.

## The decision

This volume stops here, and says so rather than leaving a reader to wonder
whether it was tried and failed. The node on this bench speaks a bus that is
good enough for a joint. The bus serious machines use between a controller and
its axes needs hardware this part does not have and this vendor does not sell,
costs six to eight weeks by the estimate of the people who define it, and
carries a recurring obligation that attaches to selling.

What a reader can take from this chapter is the ability to tell a structural
limit from a lack of effort, and the numbers to argue either side of the
decision with somebody who has to sign for it.
