# The queues: where an index comes from, and what releases an element

Chapter 9 stage two, Tuesday 20 October 2026. The layout and the addressing are
in [message-memory.md](message-memory.md) and the element format is in
[element.md](element.md). This page is the protocol: how the node learns that a
frame has arrived, which element it is in, and what write releases it or sends
one.

Nothing in `src/bus/fifo.c` touches a register. Every function takes a status
word that was read elsewhere and returns a number or a refusal, so the whole
protocol is checkable on a laptop with no controller present. That matters more
here than anywhere else in the chapter: **on a board every mistake below
presents as a frame missing, duplicated or late, and never as an error.**

## The one that will bite

The receive acknowledge register takes **an index**. The transmit add-request
register takes **a bit per buffer**.

```
RXF0A  <-  3            releases through element 3
TXBAR  <-  1 << 3       requests transmission of buffer 3
```

Writing `1 << 3` to the acknowledge register releases through element 8, which
loses five elements nobody read. Writing `3` to the add-request register
requests buffers 0 and 1, one of which was never filled. Both writes are
accepted. Neither errors.

The two composers in `fifo.h` carry the encoding in their names for that reason:
`fifo_rx_ack_index` and `fifo_tx_request_mask`. And one rule says the two
encodings can never produce the same number, so a swap is always wrong rather
than accidentally right on some index and wrong on the rest.

## Three more, all silent

**An empty queue still reports a get index.** So does a full one. The get index
of an empty queue points at the element that was there before it was
acknowledged, and the controller does not clear the memory, so a read returns a
frame that has already been delivered once and the second copy is
indistinguishable from a real one. `fifo_rx_at` therefore refuses element 0 of
an empty queue, which is the only refusal in the file that looks like an
off-by-one and is not.

**The indices wrap at the configured size, not at the width of the field.** The
field holds six bits and can carry 63. This node configures sixteen receive
elements. A wrap at 64 addresses memory the layout never created, which
`msgaddr.c` refuses, but only if the walk asks it rather than doing the
arithmetic itself. That is the rule that spans the two files, and it is the only
reason a wrong wrap is caught at all: each module is correct on its own terms.

**The message-lost flag is the only trace a dropped frame leaves,** and it is
sticky. Never reading it makes a loss invisible. Reading it without clearing it
makes one old loss look like a continuous fault.

## What is inferred, and the run that would settle it

`[kernel]` The driver reads the get index out of `RXF0S` and writes that same
value, not a mask and not the index plus one, into `RXF0A`. It reads a run of
elements while tracking the index itself, and writes **one** acknowledge
afterwards carrying the last index read.

`[inferred]` That one acknowledge releases the whole run rather than a single
element follows from the driver doing it, not from any statement that it does.

The board run that settles it: read two frames, acknowledge the second index
only, and see whether the fill level falls by two or by one. Until that run
exists, `fifo.c`, this page and the chapter all say inferred. If it falls by
one, `fifo_rx_ack_index` is wrong and the node leaks an element per batch.

## What the checks proved

`[arithmetic]` The sweep is exhaustive over every status word a sixteen element
queue can produce: fill level 0 to 16, get index and put index each 0 to 15,
the full flag both ways, which is 8704 words. Each field must come back as it
went in, and a word is accepted exactly when the full flag agrees with the fill
level and refused otherwise. A word that makes those two disagree is not
describing a queue, and trusting either one alone is how a full queue gets
written to.

Reading a full queue must visit every element exactly once, from every one of
the sixteen possible get indices. That is the wrap stated as a property rather
than as one example.

Three mutations, each with the vectors regenerated to agree with it so that
only the rules could object:

| Mutation | Failures | What objected |
|---|---|---|
| Wrap at the width of the field, 64, instead of the queue size | 135 | the full-queue permutation rule, and 120 of them from the message memory refusing to address the index |
| Compose the acknowledge as a bit mask instead of an index | 5 | the relation between the element read and the acknowledge written |
| Let an empty queue read at its get index | 16 | the empty-queue rule, at every get index |

The counts are from the runs' own output.

### The status words

| Case | Side | Word | Configured | Waiting or free | Get | Put | Full | Lost |
|---|---|---|---|---|---|---|---|---|
| receive FIFO 0 empty, which still reports a get index | receive | `0x00000000` | 16 | 0 | 0 | 0 | no | no |
| one frame waiting at the start of receive FIFO 0 | receive | `0x00010001` | 16 | 1 | 0 | 1 | no | no |
| three waiting near the end, the put index already wrapped | receive | `0x00000D03` | 16 | 3 | 13 | 0 | no | no |
| receive FIFO 0 full, sixteen waiting, flag and level agreeing | receive | `0x01000010` | 16 | 16 | 0 | 0 | yes | no |
| two waiting and the sticky lost flag set, which is the only trace a dropped frame leaves | receive | `0x02090702` | 16 | 2 | 7 | 9 | no | yes |
| receive FIFO 1, eight elements, half full | receive | `0x00060204` | 8 | 4 | 2 | 6 | no | no |
| REFUSE, a fill level of 17 in a queue of 16, so the controller and the layout disagree about the size | receive | `0x00010011` | 16 | refused | refused | refused | refused | refused |
| REFUSE, a get index of 16 in a queue of 16, which addresses the next section | receive | `0x00001001` | 16 | refused | refused | refused | refused | refused |
| REFUSE, the full flag set with three waiting | receive | `0x01030003` | 16 | refused | refused | refused | refused | refused |
| REFUSE, sixteen waiting in a queue of 16 and the full flag clear | receive | `0x00000010` | 16 | refused | refused | refused | refused | refused |
| REFUSE, a queue the layout configured with no elements | receive | `0x00000000` | 0 | refused | refused | refused | refused | refused |
| every transmit buffer free | transmit | `0x00000008` | 8 | 8 | 0 | 0 | no | no |
| three frames queued of eight buffers | transmit | `0x00030005` | 8 | 5 | 0 | 3 | no | no |
| the transmit queue full, flag and free level agreeing | transmit | `0x00230300` | 8 | 0 | 3 | 3 | yes | no |
| REFUSE, nine free in a queue of eight | transmit | `0x00000009` | 8 | refused | refused | refused | refused | refused |
| REFUSE, the full flag set with two free | transmit | `0x00260002` | 8 | refused | refused | refused | refused | refused |
| REFUSE, no free elements and the full flag clear | transmit | `0x00030300` | 8 | refused | refused | refused | refused | refused |
| REFUSE, a transmit queue of 33, past what the index field can address | transmit | `0x00000001` | 33 | refused | refused | refused | refused | refused |

### Reading and releasing

| Case | Get | Waiting | n | Element to read | Acknowledge write |
|---|---|---|---|---|---|
| read the first of three waiting, at get index 13 | 13 | 3 | 0 | 13 | `0x0D` (index 13) |
| read the second of the three, at index 14 | 13 | 3 | 1 | 14 | `0x0E` (index 14) |
| read the third of the three, index 15, the last element of the queue | 13 | 3 | 2 | 15 | `0x0F` (index 15) |
| read the third of three from get index 14, WHICH WRAPS to index 0 | 14 | 3 | 2 | 0 | `0x00` (index 0) |
| REFUSE, the fourth of three waiting | 13 | 3 | 3 | refused | refused |
| REFUSE, the first element of an EMPTY queue, whose get index is a perfectly good number pointing at an element already released | 5 | 0 | 0 | refused | refused |
| read the last element of a full queue | 0 | 16 | 15 | 15 | `0x0F` (index 15) |

### Filling and sending

| Case | Buffer to fill | Add request write | Still pending |
|---|---|---|---|
| write the next frame into an empty queue and request buffer 0 | 0 | `0x01` (bit 0) | no |
| write into a queue with three already in it, buffer 3 | 3 | `0x08` (bit 3) | no |
| buffer 7, the last one this layout configures | 7 | `0x80` (bit 7) | no |
| REFUSE to write into a full queue, whose put index belongs to a frame already requested and not yet sent | refused | `0x08` (bit 3) | yes |
| REFUSE buffer 8 of the eight configured | 0 | refused | refused |
| REFUSE buffer 31, which the register can address and the layout cannot | 0 | refused | refused |

