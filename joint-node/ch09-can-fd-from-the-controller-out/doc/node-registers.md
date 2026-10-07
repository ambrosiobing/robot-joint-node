# The registers the first image writes

Settled Wednesday 7 October 2026. Together with
[node-clock.md](node-clock.md) this is everything the node's first image needs
except the initialisation order, which is the one thing still missing at the
end.

Nothing here came from RM0455. st.com has not served a PDF to this bench in any
attempt, so the route is the one that settled `PD0`, `PD1` and `FDCANSEL`: the
manufacturer's own published source, and where ST's own source is silent, the
mainline Linux driver for the same silicon. **The peripheral is not ST's
design.** It is the Bosch M_CAN, and `drivers/net/can/m_can/m_can.c` drives the
identical register block in dozens of parts from several vendors. That is a
better source than a vendor summary, because it is code that runs.

## Where it is

| Thing | Address | Where from |
|---|---|---|
| FDCAN1 registers | `0x4000A000`, a `0x400` window | Zephyr's device tree for `st,stm32h7-fdcan`, corroborated by NuttX's `stm32h7x3xx_memorymap.h` |
| Message RAM | `0x4000AC00` | the same two |
| Message RAM size | `0x2800`, which is 10240 bytes | ST's HAL, `FDCAN_MESSAGE_RAM_SIZE` |

**That 10240 agrees with what this chapter already had.** `msgram.h` has carried
`MSGRAM_SIZE_BYTES (10u * 1024u)` since it was written, taken from the driver
source, and ST's own HAL says `0x2800`. Two sources, one number, and the
chapter's `_Static_assert` has been guarding against the right limit all along.

**It is shared between the two FDCAN instances.** This chapter's layout uses
2432 bytes of it and leaves 7808, which is room for a second instance that this
volume does not use.

## The register map

The Bosch offsets, from the driver's own enum. The subset the first image
touches is marked.

| Offset | Register | First image? |
|---|---|---|
| `0x00` | `CREL`, core release | read it, to prove the block answers |
| `0x04` | `ENDN`, endianness | read it, it should read `0x87654321` |
| `0x0C` | `DBTP`, data bit timing | not yet, there is no data phase |
| `0x10` | `TEST`, test modes | **yes**, bit 4 is `LBCK`, loopback |
| `0x18` | `CCCR`, control | **yes**, this is the gate |
| `0x1C` | `NBTP`, nominal bit timing | **yes** |
| `0x40` | `ECR`, error counters | worth printing |
| `0x44` | `PSR`, protocol status | worth printing |
| `0x48` | `TDCR`, delay compensation | not yet, chapter 9 step 5 |
| `0x50` | `IR`, interrupt flags | **yes**, polled rather than enabled |
| `0x80` | `GFC`, global filter | **yes**, decide what unmatched frames do |
| `0x84` | `SIDFC`, standard filters | **yes** |
| `0x88` | `XIDFC`, extended filters | **yes** |
| `0xA0` | `RXF0C`, receive FIFO 0 | **yes** |
| `0xA4` | `RXF0S`, its status | **yes**, this is how a frame is noticed |
| `0xA8` | `RXF0A`, its acknowledge | **yes**, how a frame is released |
| `0xAC` | `RXBC`, receive buffers | **yes**, zero elements, but it is still written |
| `0xB0` | `RXF1C`, receive FIFO 1 | **yes** |
| `0xBC` | `RXESC`, receive element size | **yes**, and getting it wrong is subtle |
| `0xC0` | `TXBC`, transmit buffers | **yes** |
| `0xC4` | `TXFQS`, its status | **yes** |
| `0xC8` | `TXESC`, transmit element size | **yes** |
| `0xD0` | `TXBAR`, add request | **yes**, this is what sends |
| `0xF0` | `TXEFC`, transmit event FIFO | **yes** |

## `CCCR`, and the gate that has to be respected

| Bit | Name | What it does |
|---|---|---|
| 0 | `INIT` | the peripheral is stopped and configurable |
| 1 | `CCE` | configuration change enable |
| 2 | `ASM` | restricted operation |
| 3 | `CSA` | clock stop acknowledge, read only in effect |
| 4 | `CSR` | clock stop request |
| 5 | `MON` | bus monitoring, the receiver does not drive the bus |
| 6 | `DAR` | automatic retransmission disabled |
| 7 | `TEST` | the `TEST` register becomes writable |
| 8 | `FDOE` | CAN FD frames accepted |
| 9 | `BRSE` | bit rate switching accepted |
| 15 | `NISO` | the non-ISO frame format |

**`CCE` can only be set while `INIT` is set, and the driver refuses outright
rather than trying.** Its own words are "refusing to configure device when in
normal mode". So the order is `INIT` first, confirmed, then `CCE`, and not the
other way round.

**And the driver does not trust the write.** It writes `CCCR`, reads it back,
compares, and tries up to ten times before giving up, masking out `CSR` and
`CSA` from the comparison because those two are a request and its
acknowledgement rather than settings. That is the read-back rule this volume
already applies to everything else, arrived at independently by people driving
this exact peripheral. It is worth copying exactly, including the masking,
because a reader who compares all 32 bits will see a mismatch on a bit that was
never theirs to set.

## The two timing words, and the trap between them

| Register | Field | Bits | Width | Holds |
|---|---|---|---|---|
| `NBTP` | `NSJW` | 31:25 | 7 | jump width 1 to 128 |
| `NBTP` | `NBRP` | 24:16 | 9 | prescaler 1 to 512 |
| `NBTP` | `NTSEG1` | 15:8 | 8 | segment 1, 1 to 256 |
| `NBTP` | `NTSEG2` | 6:0 | 7 | segment 2, 1 to 128 |
| `DBTP` | `TDC` | 23 | 1 | delay compensation enable |
| `DBTP` | `DBRP` | 20:16 | 5 | prescaler 1 to 32 |
| `DBTP` | `DTSEG1` | 12:8 | 5 | segment 1, 1 to 32 |
| `DBTP` | `DTSEG2` | 7:4 | 4 | segment 2, 1 to 16 |
| `DBTP` | `DSJW` | 3:0 | 4 | jump width 1 to 16 |

**Every value is stored as itself minus one**, which `bittiming.h` has claimed
since it was written and which the driver confirms by subtracting one from the
prescaler, the jump width and both segments before shifting them.

**All eight of this chapter's limit constants are now independently
confirmed.** `BT_NOMINAL` and `BT_DATA` were filled in from the field
definitions when the chapter was written, with a note saying they came from the
widths rather than from a summary of the widths. The widths above come from a
different source entirely, and they agree on all eight: 512, 256, 128 and 128
for the nominal phase, 32, 32, 16 and 16 for the data phase. That closes a soft
spot nobody had flagged.

**The trap is that the two phases are not interchangeable, and the failure is
silent.** A nominal timing at 80 MHz has a segment 1 of 127. The data field is
five bits wide. Packed as a data word it truncates to 31, which configures a
completely different bit rate, and no register reports anything. So
`bt_pack_nbtp` and `bt_pack_dbtp` return a boolean and write nothing when a
value will not fit, and the caller has to check it. The test proves the guard by
packing a nominal timing as a data word and requiring a refusal, and that check
was watched turning red with the guard removed.

### The word the first image writes

From the chapter's own generator, not typed:

| Case | `NBTP` |
|---|---|
| 500 kbit/s from the 8 MHz HSE, the first image | `0x04000B02` |
| 250 kbit/s from the 8 MHz HSE, the fallback | `0x0A001805` |
| 500 kbit/s from 80 MHz, the design point a PLL would reach | `0x3E007E1F` |

`0x04000B02` reads as jump width 3, prescaler 1, segment 1 of 12, segment 2 of
3, which is sixteen quanta a bit at 8 MHz, which is 500 kbit/s. Every solved
case in [bit-timing.md](bit-timing.md) now carries its word, and both the C and
the Python pack it, unpack it and compare the result with what went in.

## The message RAM, and the question that had to be settled first

The start-address fields in `SIDFC`, `XIDFC`, `RXF0C`, `RXF1C`, `RXBC`, `TXEFC`
and `TXBC` hold **offsets from the message RAM base, not absolute addresses.**

That was worth settling rather than assuming, because both readings are
plausible and the wrong one puts every section at an address `0x4000AC00` too
high, which is outside the window. Two drivers say offsets, independently.
Zephyr is explicit about it, computing `addr = mram - mrba + offset`, subtracting
the base before writing. Linux writes its configured `.off` values straight into
the field.

**And this chapter's layout already produces exactly that.** `msgram_map_t`
carries `*_off` fields, `msgram_compute` starts them at 0 and accumulates byte
counts, and every offset it produces is word aligned:

| Section | Offset | Bytes |
|---|---|---|
| standard filters | 0 | 32 |
| extended filters | 32 | 32 |
| receive FIFO 0 | 64 | 1152 |
| receive FIFO 1 | 1216 | 576 |
| receive buffers | 1792 | 0 |
| transmit events | 1792 | 64 |
| transmit buffers | 1856 | 576 |
| total | | 2432, with 7808 free |

So `msgram_compute`'s output goes into the registers with no conversion. That
was not foresight: the layout was written before the register format was known,
and it happens to match because starting at zero and counting bytes is the
obvious thing to do. It is recorded here so that nobody later "fixes" it by
adding the base address.

**One thing that is easy to get wrong and is not an address.** `RXESC` and
`TXESC` encode the element size, and `0x7` means 64 bytes of payload. This
chapter's layout assumes 64 bytes: `MSGRAM_FRAME_BYTES` is eighteen words, two
of header and sixteen of data. **If those two registers are left at their reset
value the hardware will use 8 byte elements and the layout is wrong by a factor
of more than four**, silently, with frames landing on top of one another. So
they are written even in an image that only ever sends eight bytes.

## Still missing, and it is one thing

Everything above is a value. What is not settled is the **order**: the exact
initialisation sequence from reset to a peripheral that will transmit. The
pieces are known, `INIT`, then `CCE`, then the timing and the memory, then
`INIT` cleared, but the constraints between them are not, and nor is what has to
be true before `INIT` can be set at all.

| # | Question | How to settle it |
|---|---|---|
| 1 | The initialisation order, and what each step must wait on | the driver's probe and open paths, read end to end rather than quoted in pieces |
| 2 | Whether `TEST` needs `CCCR.TEST` set first, and whether `LBCK` alone gives internal or external loopback | ST's HAL distinguishes internal from external loopback as modes 3 and 4, so the raw bits differ by one more; `MON` is the likely second bit and that is a guess |
| 3 | Whether the message RAM has to be cleared before use | some M_CAN parts need it and some do not, and a stale element looks like a received frame |
| 4 | What `GFC` should do with unmatched frames | a filter configuration that rejects everything is indistinguishable from a dead bus |

Number 2 is the one that matters for the very first test, because internal
loopback is how the peripheral gets exercised with no bus at all, which is
exactly what [the controller end's part one](../ch10-the-motion-master/doc/first-light.md)
did before any wire existed.
