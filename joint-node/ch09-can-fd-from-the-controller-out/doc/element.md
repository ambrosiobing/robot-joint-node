# The element: a frame as the message memory actually holds it

Chapter 9 stage two, Tuesday 20 October 2026. The sections are in
[message-memory.md](message-memory.md) and the length code is in
[frame-rules.md](frame-rules.md). Neither of those turns a frame into the words
a controller reads, and this page is that step.

An element is **two header words followed by the payload**. The header carries
the identifier, the format flags and the length code. The payload is packed four
bytes to a word, least significant byte first.

## Three things here fail silently

None of the three produces an error, a counter or a dropped frame. Each one puts
a frame on the wire that is not the frame anybody asked for, which is why this is
a separate file with vectors of its own rather than four lines inside a driver.

| Mistake | What happens | What it looks like on the bench |
|---|---|---|
| A standard identifier left in the extended position | the arbitration field becomes a different and much lower priority number | the node loses arbitrations it should win, and nothing anywhere reports a fault |
| The length confused with the length **code** | the right bytes move with the wrong count, or the wrong bytes with the right one | a receiver that reads 8 where 12 were sent, with no error flag at either end |
| The payload packed most significant byte first | every four byte group arrives reversed | data corruption, which is looked for in the wiring and in the transceiver before anybody suspects a shift |

## The identifier format is a parameter, not a deduction

Identifier `0x123` sent as a standard frame and identifier `0x123` sent as an
extended frame are **different frames on the wire**, and a receiver filtering for
one does not see the other. The first draft of `tools/gen_element.py` chose the
format from the magnitude of the number, which made every small extended
identifier unreachable; the frames still went out, they simply were not the
frames asked for. The vectors are what caught that, before any of this reached a
board.

The pair in the table below is the proof that the two are distinguishable:

| The same number, two formats | First word |
|---|---|
| `0x123` as a standard identifier | `0x048C0000` |
| `0x123` as an extended identifier | `0x40000123` |

A value that does not fit the format asked for is **refused** rather than
promoted to the other format, because a promotion sends a frame the caller did
not write.

## Nine bytes is refused here and padded at the other end

No length code stands for nine. `src/bus/element.c` refuses a nine byte frame.
The Raspberry Pi's SocketCAN pads it to twelve with three zero bytes, which
chapter 10 measured on the real bus.

Two ends of one bus with two policies, and the difference is deliberate at this
end: padding sends three bytes the caller never wrote, and a receiver has no way
to tell those from data. The chapter does not call the other end wrong; it
records that a frame crossing this bus can gain bytes, and that a reader of the
payload must not assume the length it sees is the length that was sent.

## Where the bit positions come from

`[kernel]` The mainline Linux driver for this peripheral, the Bosch M_CAN, in
`drivers/net/can/m_can/m_can.c`. The element format is Bosch's rather than any
one vendor's, so the driver is a better source here than a device summary would
be. Both length code fields were confirmed rather than inferred: the transmit
side defines its mask as bits 19:16, and the receive side has no such constant
and extracts the same field as `(word >> 16) & 0x0F`.

`src/bus/element.h` names no vendor header and must not, by chapter 4's rule.

## Zero is a legal answer, which is why a refusal is not zero

An empty frame is a perfectly good frame: two header words and no payload word.
So "no payload bytes" and "this frame is refused" are different answers, and the
first version of `element.c` returned the same value for both, telling them
apart afterwards by asking whether the caller's length was also zero.

That reads as equivalent and is not. A zero length frame that asks for a rate
switch without the flexible format is refused, and its length is zero as well,
so `elem_words_for` sized it at two words while `elem_build_tx` refused to write
it. A caller that sizes a buffer with one function and fills it with the other
depends on the two refusing the same frames.

No vector reached that input, because every refusal in the set had a payload.
There are two zero length cases now, one legal and one refused, and the rule
that catches it is stated against `frame_ok` rather than against the vectors:
with the generator mutated back to the old shape and the vectors regenerated to
agree with it, the vector comparison passes and the rule still fails.

`elem_words_for` is a function of **the frame alone**. A message marker above
255 and a buffer one word too small both refuse a build and leave the frame
legal, so the vectors carry both answers and the rule checks each against its
own question.

## The vectors

Generated by `python tools/gen_element.py --table`. Do not edit by hand.

Twenty-three frames, fifteen built and eight refused. The refusals are half the
value of the file: a layout that only ever encodes what it is given cannot tell
anybody what it will not encode.

| Case | Given | Identifier | Frame | On the wire | Words | First word | Second word |
|---|---|---|---|---|---|---|---|
| the chapter 10 test frame, standard id 0x123, four bytes classic | 4 bytes | standard | classic | **4 bytes** | 3 | `0x048C0000` | `0x00040000` |
| the chapter 10 setpoint frame, id 0x101, eight byte counter | 8 bytes | standard | classic | **8 bytes** | 4 | `0x04040000` | `0x00080000` |
| a standard id at its maximum, 0x7FF | 1 byte | standard | classic | **1 byte** | 3 | `0x1FFC0000` | `0x00010000` |
| identifier zero, which must not be mistaken for an empty element | 1 byte | standard | classic | **1 byte** | 3 | `0x00000000` | `0x00010000` |
| an extended identifier, which sits in different bits entirely | 4 bytes | extended | classic | **4 bytes** | 3 | `0x41ABCDEF` | `0x00040000` |
| an extended identifier at its maximum, 0x1FFFFFFF | 1 byte | extended | classic | **1 byte** | 3 | `0x5FFFFFFF` | `0x00010000` |
| 0x123 as a STANDARD identifier, the first of a matched pair | 1 byte | standard | classic | **1 byte** | 3 | `0x048C0000` | `0x00010000` |
| 0x123 as an EXTENDED identifier, the second of the pair | 1 byte | extended | classic | **1 byte** | 3 | `0x40000123` | `0x00010000` |
| zero length, which is a legal frame and writes no payload word | 0 bytes | standard | classic | **0 bytes** | 2 | `0x08000000` | `0x00000000` |
| zero length flexible with a rate switch, legal, and the contrast that makes the zero length refusal below mean something | 0 bytes | standard | flexible, rate switch | **0 bytes** | 2 | `0x08040000` | `0x00300000` |
| twelve bytes, the first length code that is not its own index | 12 bytes | standard | flexible | **12 bytes** | 5 | `0x0C000000` | `0x00290000` |
| forty-eight bytes, a code in the steeper part of the table | 48 bytes | standard | flexible | **48 bytes** | 14 | `0x0C040000` | `0x002E0000` |
| a full 64 byte flexible frame, the largest element | 64 bytes | standard | flexible, rate switch | **64 bytes** | 18 | `0x0C080000` | `0x003F0000` |
| a message marker and an event request, which live in the second word | 2 bytes | standard | classic | **2 bytes** | 3 | `0x10000000` | `0xA5820000` |
| a marker at its maximum, 255 | 1 byte | standard | classic | **1 byte** | 3 | `0x10040000` | `0xFF010000` |
| REFUSE, nine bytes flexible: no code stands for 9, and this end does not pad where the controller end does | 9 bytes | standard | flexible | refused | refused | refused | refused |
| REFUSE, thirty-three bytes flexible, for the same reason | 33 bytes | standard | flexible | refused | refused | refused | refused |
| REFUSE, nine bytes in a classic frame | 9 bytes | standard | classic | refused | refused | refused | refused |
| REFUSE, a rate switch without the flexible format | 1 byte | standard | classic, rate switch asked for | refused | refused | refused | refused |
| REFUSE, a message marker above 255 | 1 byte | standard | classic | refused | refused | refused | refused |
| REFUSE, zero length with a rate switch and no flexible format, the refusal whose own length is zero | 0 bytes | standard | classic, rate switch asked for | refused | refused | refused | refused |
| REFUSE, an identifier past 29 bits even as extended | 1 byte | extended | classic | refused | refused | refused | refused |
| REFUSE, an identifier past 11 bits asked for as standard | 1 byte | standard | classic | refused | refused | refused | refused |

