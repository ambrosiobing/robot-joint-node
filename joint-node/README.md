# joint-node

One directory per chapter. Each is its own software, runnable on its own, with
its own tests, its own dependencies and its own README.

```
ch04-the-board-support-package/
ch09-can-fd-from-the-controller-out/
ch10-the-motion-master/
ch11-a-joint-protocol/
ch17-what-a-real-time-fieldbus-would-change/
```

Five of the twenty. Chapter 9 carries everything that can be decided without a
controller answering, which turned out to be most of it: the bit timing, the
message memory layout and the arithmetic that addresses an element inside it,
the frame length code, the element format, the queue protocol, the
configuration order as data, the executor that walks it, and the accessors that
point that executor at a real address. What still needs the Nucleo is whether a
register answers at all, the loopback and the first frame. Its README says
which is which.

Chapter 4 carries the image floor the rest of the volume stands on, which
chapter 9 established on Tuesday 20 October 2026 was missing: a memory map
generated from vendor sources, a linker script, a startup, the device interrupt
vector table, the console's divisor and the console itself. **The volume's first
firmware image linked on Tuesday 20 October 2026** and has been relinked on
every change since. The vector table was the last piece of that floor and was
read from its sources on Monday 9 November 2026, all 171 entries. **The first
image that prints was built on Thursday 12 November 2026**, `src/app/first_light.c`,
which runs at the part's reset clock so that it needs no clock configuration and
carries a generated build stamp so that a capture can name the image that
produced it. **It printed on the same day**, which is the first time anything in
this volume has run on the part: 5247 bytes legible at 115200, the console's
stamp matching the build's character for character, and `[measured]` the USART
kernel clock bounded between 62.77 and 65.33 MHz, which excludes any factor of
two in the prescaler chain. What is still missing is the other three board
questions, which that image does not ask because it only prints.

**A contradiction in a vendor header is open as of Thursday 12 November 2026.**
ST's CMSIS device file for this part disagrees with itself about where the RCC
clock enable registers are: its `RCC_TypeDef` and its own per-field comments
differ by 0x60 for 22 registers, and the console works at the comments'
addresses while Zephyr supports this board through the struct. Chapter 4 carries
two probe images, differing in exactly one write, to let the part settle it.
See [`ch04/doc/register-map.md`](ch04-the-board-support-package/doc/register-map.md).

Chapter 17 contains **no code at all**, and that is the chapter rather than an
omission. Its subject is the boundary this volume stops at, so its deliverable
is a decision document: what a deterministic fieldbus would buy, what it would
cost in parts and weeks and obligations, why this part cannot reach it, and what
would change that. Every claim in it says where it came from, and a check keeps
it that way.

## Why one directory per chapter rather than one shared tree

The volume is organised around a single device, so the obvious move is a single
tree that every chapter adds to. That was tried and it is the wrong shape here.

A reader who wants to see what chapter 10 is cannot be asked to read nineteen
other chapters first, and a chapter that cannot be opened on its own cannot be
shown to anybody on its own either. A shared tree also makes every chapter
depend on every earlier one in practice, so chapter 14 cannot be started until
chapter 4 is finished, which is false and expensive.

The cost is duplication. Two chapters that both need to know a frame layout will
each carry one. That is accepted on purpose: the alternative is a shared library
that quietly becomes the thing all twenty chapters really are, and then there are
no chapters, only one program with twenty README files.

Where two chapters genuinely must agree, they agree through a **file**, not
through an import. Chapter 11's `proto/messages.yaml` is the description; another
chapter that needs those offsets copies the description and generates its own
code from it, so the agreement is visible in a diff rather than hidden in a path.

## The shape each chapter follows

```
chNN-slug/
  README.md          what it is, what runs, what needs hardware and what does not
  include/ src/      the software itself, when the chapter is C
  app/               the executables, when it has any
  test/              its own tests, runnable with no arguments
  tools/             anything it generates or checks with
  Makefile           how to build and how to test
  requirements.txt   pinned, and only where there is Python to pin
```

A chapter is done when its tests run and its README says honestly which of its
acceptance criteria they cover and which they do not.

The language is C, because the volume is about embedded Linux in C and a
chapter written in another language teaches the interface without teaching the
language. Python stays where it is the right tool for a build-time job and
never ships to a board: chapter 11 generates its C from a description, chapter 4
checks a layering rule and generates a linker script's MEMORY block from a
recorded memory map, and chapter 9 keeps a reference implementation of every
piece of arithmetic beside the C and compares both against committed vectors
rather than against each other. That last arrangement is why several defects in
this volume were found on a laptop with no compiler on it. A chapter with no
Python carries no `requirements.txt`, because an empty one implies a virtual
environment that nothing needs.

## What exists now

Counted on Sunday 8 November 2026, from the tree rather than from memory.

| Chapter | Language | Software | Runs on the authoring laptop |
|---|---|---|---|
| 04, the board support package | C and Python | the layer rule, the memory map and its generated MEMORY block, the linker script, the startup, the generated 171 entry vector table, the baud divisor, the console, the first image that prints and the RCC register map probe. 8 C files, 6 headers, 13 Python | the Python half, and the pure-arithmetic C half needs only a host compiler |
| 09, CAN FD from the controller out | C and Python | nine modules: bit timing, message memory, element addressing, the length code, the element format, the queue protocol, the configuration order, the executor, and the board accessors. 18 C files, 17 headers, 16 Python | the Python half only |
| 10, the motion master | C | frame layout, log format, socket, two tools, and five hardware documents | no, there is no compiler there |
| 11, a joint protocol | C, generated by Python | the codec, its validator, bus load | the Python half only |
| 17, what a real-time fieldbus would change | Python, one checker | **no code by design**, one decision document and a check that keeps it sourced | yes |

**Fifteen chapters have nothing.** That is the honest state and the table above
is the only place to read it. The count is five with software and fifteen
without, and chapter 17 is in the first group only in the sense that it has a
checker: its subject is a boundary, so a chapter with code there would be the
defect.

## What needs hardware, and what only needs Linux

Three of these chapters need no board at all, and two of those need no Linux
either. The distinction matters because it decides what can be finished tonight.

- **No compiler at all**: every generator and every Python test in chapters 4,
  9, 11 and 17. That is most of the checking in this tree, and it is deliberate:
  the authoring laptop has no C compiler by house rule, so anything that can be
  decided by arithmetic is written so it can be decided there.
- **Anywhere with a compiler**: chapter 11 in full, chapter 10's frame layout
  and log format, chapter 9's whole host half, chapter 4's baud divisor.
- **Linux, no hardware**: chapter 10's socket, against the virtual interface.
  `sudo modprobe vcan; sudo ip link add dev vcan0 type vcan; sudo ip link set up vcan0`
- **A cross compiler, still no board**: chapter 9's `make cross`, which compiles
  its bus and board layers for `cortex-m7`, and chapter 4's `make linkcheck`,
  which links a real image and reads its first two words back out of the `.elf`.
  Neither flashes anything.
- **A board**: whether a register answers at all, the loopback, the first frame,
  and every open question in chapter 9's register map. Nothing here claims
  otherwise.

There is no C compiler on the win11 aquamarine authoring laptop, so no C in this
tree is built there. It is built by the workflow in `.github/workflows/checks.yml`
and on the Pi 4, and the workflow is where the first information about a change
to any of it comes from.

One thing is still unproven anywhere, and chapter 10's README says so in its own
acceptance table: no setpoint rate has been measured, only reported at run time.

Chapter 10's socket test was in that list until Thursday 1 October 2026. The
runner image ships a kernel with no `vcan` module, so it reported a skip; the
workflow now installs `linux-modules-extra` for the running kernel first, and
the test is real. It sends a 24 byte flexible-data frame through the kernel and
gets every byte back.
