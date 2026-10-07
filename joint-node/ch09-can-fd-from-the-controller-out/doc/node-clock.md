# The clock the node can actually reach

Settled Wednesday 7 October 2026, and it changed the plan twice in one sitting.

Every kernel clock in [bit-timing.md](bit-timing.md) up to this point was a
hypothetical: 80, 100, 60 and 40 MHz, chosen to show what the arithmetic does
and to find the design point. None of them asked the only question that decides
what the first image looks like, which is **what this part will let the FDCAN
peripheral be clocked from at all.**

## The answer is three sources, and one of them is free

`FDCANSEL`, a two bit field at bits 29:28 of `RCC_CDCCIP1R`, register offset
`0x50`:

| Value | Source | Where from |
|---|---|---|
| `00` | `HSE_CK`, and this is the **reset value** | ST's `stm32h7xx_hal_rcc_ex.h`, `RCC_FDCANCLKSOURCE_HSE` is `0x00000000U` |
| `01` | `PLL1_Q_CK` | the same header, `RCC_FDCANCLKSOURCE_PLL` is `RCC_CDCCIP1R_FDCANSEL_0` |
| `10` | `PLL2_Q_CK` | the same header, `RCC_FDCANCLKSOURCE_PLL2` is `RCC_CDCCIP1R_FDCANSEL_1` |

The field's position was taken independently from Zephyr, which encodes it as
`FDCAN_SEL(val) STM32_DT_CLOCK_SELECT((val), 29, 28, D2CCIP1R_REG)` with
`D2CCIP1R_REG 0x50`. `D2CCIP1R` and `CDCCIP1R` are the same register under two
naming conventions across the H7 family, at the same offset, and the two sources
agree on the bits.

Neither source is ST's reference manual. RM0455 would settle it directly and
st.com has not served a PDF to this bench in any attempt, so this is the same
route that settled `PD0` and `PD1`: the manufacturer's own published source
rather than the manufacturer's own prose. It is recorded as such rather than
dressed up as a datasheet reading.

**The consequence, and it is the useful part: there is no HSI option.** The part
boots on its 64 MHz internal oscillator, and 64 MHz solves the nominal phase
perfectly well on paper, 128 quanta at 79.7 per cent. It cannot be used. The
FDCAN peripheral cannot be clocked from HSI on this part at all, so the one
clock that needs no configuration whatsoever is not available to this
peripheral.

What is available with no configuration is the **selector**, because `00` is
already what reset leaves behind. The clock it points at still has to be
switched on.

## What HSE is on this board

The NUCLEO-H7A3ZI-Q takes its high-speed external clock from the on-board
ST-LINK debugger in **bypass** mode, at a nominal **8 MHz**. That is a board
fact, not a silicon one, and it is settled: ST's own division for this board is
8 over 4 times 280 over 2 to reach 280 MHz, which only makes sense from an 8 MHz
input.

**It is nominal and it has been measured, and the two differ.** The sibling
firmware volume measured it three times on Sunday 4 October 2026 and got
7990652, 7983868 and 7991850 Hz, a mean about **0.14 per cent below** 8 MHz.

That matters here more than it did there, because this chapter's whole doctrine
is that a bit rate one part in a thousand out works between two nodes wrong in
the same direction and fails against anything else. The arithmetic below is
exact against 8000000. The wire will carry something about 0.14 per cent slower,
and the controller at the other end is an MCP2518FD on a packaged oscillator
specified at plus or minus 20 ppm, which is 0.002 per cent. So the two ends are
not equally wrong, and the node is the inaccurate one by a factor of about
seventy.

**Whether 0.14 per cent is inside CAN's tolerance for this timing is not
answered here.** The standard has not been read on this bench, the tolerance
conditions depend on the jump width and the segment lengths, and quoting a
formula from memory is exactly the failure this volume keeps correcting. It is
an open question, and the honest version of it is in the list at the end.

## What 8 MHz buys

From the chapter's own reference, not by hand:

| Phase | Bit rate | Prescaler | tq a bit | seg1 | seg2 | SJW | Sample point |
|---|---|---|---|---|---|---|---|
| Nominal | 500 kbit/s | 1 | 16 | 12 | 3 | 3 | 81.3 per cent |
| Nominal | 250 kbit/s | 1 | 32 | 25 | 6 | 6 | 81.3 per cent |

Both are in the committed vectors, so the C reproduces them or the build fails.

**500 kbit/s at a prescaler of one.** 8 MHz divided by nothing is the quantum
rate, sixteen quanta make a bit, and sixteen quanta at 500 kbit/s is exactly
8 MHz. There is no remainder anywhere, which is what `bt_compute` demands.

**The sample point lands at 81.3 per cent and not at 80.** Eighty per cent of
sixteen quanta is 12.8, and a quantum cannot be split, so the sample point goes
to thirteen quanta in and the achieved figure is 81.3. `bt_compute` does not
refuse this: it demands an exact **bit rate** and reports whatever sample point
the quanta allow. That distinction is worth being clear about, because the
chapter's prose has described the calculator as refusing anything inexact, and
what it actually refuses is an inexact bit rate. An inexact sample point is
reported, not refused, and the reason is that the two ends of a CAN bus are not
required to sample at the same point while they are required to agree on the
bit rate.

**The controller end already sampled somewhere else anyway.** The Pi end
achieved exactly 80.0 per cent at 500 kbit/s, measured on Wednesday 7 October
2026. So the two ends will differ by 1.3 points. Chapter 9 had already written
down that the two ends would not agree on a sample point because 40 MHz and
80 MHz divide differently, and called it a finding rather than a fault. The
finding survives; only the numbers in it change, because the node end is 8 MHz
and not 80.

## What 8 MHz does not buy, and it is the interesting half

| Phase | Bit rate | Result |
|---|---|---|
| Data | 2 Mbit/s | **refused**, four quanta a bit |

Eight MHz divided by one, into 2 Mbit/s, is four quanta a bit. The registers
would hold it: three quanta is legal silicon. This volume's floor is eight,
chosen rather than quoted, because at three or four quanta the sample point can
only land in a handful of places and the jump width has almost nothing to work
with.

**So CAN FD's fast phase needs a PLL, whatever transceiver is fitted.** That is
a second reason chapter 13 waits, and it is independent of the first. The first
was a part: the loose SN65HVD230 at the node end does not specify loop delay
symmetry, which is the property a data phase depends on. This one is a clock,
and buying a TCAN3413 does not touch it.

| # | What chapter 13 waits for | Fixed by |
|---|---|---|
| 1 | a transceiver that specifies loop delay symmetry | a purchase |
| 2 | a kernel clock that gives at least eight quanta at 2 Mbit/s | `PLL1_Q` or `PLL2_Q`, which is the voltage scaling and PLL sequence the sibling volume has already solved |

Both have to be true at once, and neither was visible from the other. The
transceiver limit was found by reading three datasheets side by side. The clock
limit was found by putting the board's real clock through the chapter's own
calculator instead of a hypothetical one.

## What the first image therefore needs

Shorter than it looked this morning, when the assumption was that 80 MHz meant
the full 280 MHz sequence.

| Needed | Not needed |
|---|---|
| HSE on, in bypass mode | the PLL |
| `FDCANSEL` left at its reset `00` | voltage scaling, either step |
| `PD0` and `PD1` to alternate function 9 | the supply selection that leaves Run\* mode |
| FDCAN1 clock enabled, out of its reset init mode | flash access latency |
| the nominal bit timing registers, from `bt_compute` | the bus prescalers |
| a console to print what was written and read back | the system clock switch |

Every entry in the left column except the console is a register this chapter
has a value for. The console is the one borrowed idea: the sibling firmware
volume reaches the ST-LINK virtual COM port on `COM13`, and the pins it uses
are believed to be USART3 on `PD8` and `PD9` by Nucleo-144 convention,
corroborated by Zephyr's board description and now by the fact that an image
printed through it on Wednesday 7 October 2026.

## Still open

| # | Question | Why it matters |
|---|---|---|
| 1 | Is 0.14 per cent oscillator error inside CAN's tolerance for 16 quanta with a jump width of 3? | It decides whether the first image can be trusted against a third-party node, rather than only against the one controller on this bench |
| 2 | Is the HSE really 8 MHz on **this** board, or is the sibling volume's measurement the whole story? | The measurement was taken on the same board, so it probably is, but it was taken for a different purpose and the figure is being reused |
| 3 | Does `FDCANSEL` need the peripheral clock disabled while it changes? | Only matters if the image ever moves off HSE, which is chapter 13's problem rather than this one's |
| 4 | The 520 against 560 bit disagreement for a 64 byte frame | Chapter 9 step 7 and chapter 11 do not agree, and RM0455 is the only thing that settles it |

Number 1 is the one that would stop a result being published. The others can
wait, and number 3 does not arise until a PLL does.
