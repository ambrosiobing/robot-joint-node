# Chapter 5. The encoder: quadrature in hardware, and one you generate

> **What the node gains:** Position  
> **Theme:** Timer encoder mode, index, velocity from differences, generating quadrature to test the decoder

> **Key facts**
>
> - **Adds to the node:** Position, and the first derived quantity in the volume: velocity. Also the first stand-in, and the chapter says so before it says anything else
> - **Peripherals:** One timer in encoder mode for the decode, a second timer generating two phase-shifted signals as the stand-in source, one capture channel for the index
> - **Depends on:** Chapter 2 for the period, chapter 3 for the time base and the capture technique, chapter 4 for the pin budget
> - **Real or modelled:** **Half real.** The decode path is real hardware and is exercised at full rate. The source is generated on the same board, so the position is a count rather than a measurement of anything that turns
> - **Difficulty:** 3 of 5
> - **Effort:** Three evenings of about three hours, one of them on the index
> - **Deliverable:** A hardware quadrature decoder with a software extension that survives direction changes, an index latched without a dedicated peripheral, a velocity estimate with its quantisation stated, and a generator that proves the decoder against a known count

## Why this chapter

A joint node without position is a timer with opinions. Everything from here depends on this chapter: the controller in chapter 16 acts on the error between a commanded position and this one, the state frame in chapter 11 carries it, and the tracking assertions in chapter 20 are computed from it.

There is no encoder on this bench, and this is the chapter where that stops being a detail and becomes a design decision. The honest options were to skip the subject, to buy a part, or to build the half that is real and generate the other half. This volume takes the third, for a reason worth stating: the decode path is where all the interesting firmware lives. Counting quadrature correctly across direction reversals, extending a sixteen-bit counter that can run backwards, latching an index without a peripheral that supports one, and getting a velocity out of a position that only changes in whole counts are all real problems with real solutions, and none of them needs a shaft to turn.

What is generated is the source. A second timer produces two square waves ninety degrees apart and an index pulse, those pins are wired back to the decoder's pins with two jumper wires, and the decoder sees quadrature that is indistinguishable from an encoder's except in one respect: it is perfect. A real encoder contributes mechanical eccentricity, electrical noise, missed edges at speed and a quadrature phase that is never exactly ninety degrees. The chapter says which of those the stand-in cannot exercise, and it injects the two that can be injected.

> [!NOTE]
> **A correction to chapter 1**
>
> Chapter 1's table of what is real and what is modelled put position in the fully real column. That was wrong, and this chapter is where it is corrected: position is half real, in the same sense that the actuator in chapter 7 is half real. The decode path is hardware and is exercised at full rate; the source is generated. The table in `doc/real-or-modelled.md` and the boot banner's modelled line are both updated here, which is what that file is for. A volume that discovers a boundary it drew in the wrong place and quietly moves it has lost the argument it was making.

## Prior art and what to reuse

| Source | What it gives | What it does not | Licence |
| --- | --- | --- | --- |
| The vendor's hardware abstraction driver for this family | The authoritative statement of what this part's encoder interface offers: exactly three encoder modes, named for which input's edges are counted. Everything in step 2 is checked against this header rather than against a tutorial | The mapping from those three modes to one, two and four counts per cycle is standard reference-manual semantics and was not read in the manual for this part. It is marked as such in the text | BSD-3-Clause |
| The vendor's own community answer of Saturday 14 March 2020 | The finding this chapter is shaped around, from a member of the vendor's staff: the hardware index feature is new, the first family to carry it is a different one, and it was not available when this family was designed | A forum answer, so it is corroborated below by the absence of the corresponding symbols in the vendor's own headers | forum post |
| The field oriented motor control project | Real robot code that reads encoders in the same loop that drives a bridge, including the velocity estimate and its filtering, and a permissively licensed driver for one common magnetic position sensor | Arduino shaped, and its velocity path makes different trade-offs from this one, which is worth comparing rather than copying | MIT |
| The sibling firmware volume, on filters and transforms | The filter this chapter's velocity estimate needs, already built and measured on this exact part | It is a filter chapter, not a position chapter | this series |
| A permissively licensed driver for one magnetic angle sensor | A clean driver, if that part is ever bought | **That sensor has no quadrature output and no serial peripheral interface**, so it cannot feed the timer's encoder interface at all. It is named here to stop a reader buying it for this purpose | MIT |

*Table 5.1. Prior art for chapter 5. The last row is a negative finding worth as much as the positive ones: a popular, inexpensive magnetic sensor that appears in a great many encoder tutorials cannot be used with this peripheral, and knowing that before ordering is worth more than a driver.*

What is left to write is the extension across direction reversals, the index without a peripheral for it, the velocity arithmetic and its stated error, and the generator. No published source was found that presents a quadrature generator built on the same part as the decoder for the purpose of testing it, so that part is this volume's own construction.

## What the node gains

Before this chapter the node knows when. After it, the node knows where: a position in counts that survives direction changes and counter wraps, an index that turns a relative count into an absolute one, and a velocity with an honest statement of how much of it is quantisation. What it does not gain is a measurement of anything mechanical, and the boot banner now says so.

## Parts from the inventory

| Part | Role | Interface |
| --- | --- | --- |
| NUCLEO-H7A3ZI-Q | Both halves: one timer generates the quadrature, another decodes it | Micro USB to the host |
| Two male-to-male jumper wires | The loopback from the generator pins to the decoder pins. This is the entire wiring of the chapter | Header to header |
| One more jumper wire | The index pulse, generator to capture input | Header to header |
| The power profiler, as a logic recorder | An optional external check that the generated waveform is what the firmware thinks it is | Two digital inputs |

*Table 5.2. Inventory items used in chapter 5. The bench has no rotary encoder, which is the subject of the note above and of the wiring figure below.*

## System architecture

![Figure 5.1. The decode path, the generated source, and the place a real encoder would plug in.](../figures/j05_arch.svg)

*Figure 5.1. The decode path, the generated source, and the place a real encoder would plug in. Everything solid is real hardware exercised at full rate. The dashed generator is on the same silicon as the decoder, which is the whole trick and also the whole limitation: it cannot produce the failures a real encoder produces, so those are injected deliberately in step 8.*

## Peripheral configuration

| Peripheral | Mode | Clock source | Pins and function | Interrupt and transfers |
| --- | --- | --- | --- | --- |
| Decode timer | Encoder interface, counting on both inputs | Peripheral bus timer clock | Two channels, A and B | Update interrupt, to extend the count |
| Same timer, third channel | Input capture on the index edge | As above | One channel, Z | Capture interrupt, low priority |
| Generator timer | Two compare outputs, one quarter period apart | Peripheral bus timer clock | Two pins, plus one for the index | Update interrupt, to emit the index |
| GPIO | Alternate function | Peripheral bus | Five pins, all on the free list in chapter 4 | None |

*Table 5.3. Peripheral configuration. Two timer instances are named as roles rather than by number, because which of this part's timers offer encoder mode, which are thirty-two bit, and which are still free once a shield is fitted are on chapter 4's confirm list. The selection rule is in step 1 and the chosen numbers go into the board description, not into this table.*

## Wiring

![Figure 5.2. Three jumper wires and a connector that is not there.](../figures/j05_wiring.svg)

*Figure 5.2. Three jumper wires and a connector that is not there. The loopback is the chapter's only wiring. The right half of the figure is what a real encoder brings and what would have to be checked before plugging one in, which is the paragraph to read before buying anything.*

Two cautions for the day an encoder is bought. Many industrial encoders are five volt parts and output five volt levels, and whether a given pin on this part tolerates that is a per-pin property in the datasheet rather than a property of the board: check the exact pins, not the family. And an encoder's index output is sometimes open collector, which needs a pull-up that the generated stand-in does not.

## Memory and timing budget

| Quantity | Budget | Measured | Margin |
| --- | --- | --- | --- |
| Flash, this chapter | 5 kB | not measured | not measured |
| Static memory, this chapter | 64 bytes | not measured | not measured |
| Decoder cost in the period handler | under 200 cycles | not measured | not measured |
| Maximum count rate decoded | 1 Mcount/s | not measured | not measured |
| Position error against the generator | 0 counts, exactly | not measured | not measured |
| Velocity quantisation at 1 rev/min | computed, see step 6 | computed | n/a |

*Table 5.4. The budget for chapter 5. The fifth row is the one that makes this chapter worth doing: because the source is generated, the number of counts that should have arrived is known exactly, so the acceptance criterion is equality rather than a tolerance.*

## Firmware design (UML)

![Figure 5.3. The three modules and what each is allowed to know.](../figures/j05_uml.svg)

*Figure 5.3. The three modules and what each is allowed to know. The decoder does not know that the source is generated, which is the property that lets a real encoder replace the generator later without touching it. The test harness knows both, which is why it can assert equality.*

The design rule here is the one that makes the stand-in honest. The decoder is written against a quadrature signal, not against the generator: it has no reference to it, no shared state with it, and no way to ask it anything. The harness above them both knows how many counts were generated and how many were decoded, and asserts they are equal. When a real encoder arrives, the generator is removed, the harness loses its reference count, and the decoder does not change at all.

## Data flow (ASCII)

```text
  generator timer                        decoder timer
  +---------------------------+          +--------------------------------+
  | CH1 ___|---|___|---|___    |  wire A  | TI1 -> encoder interface       |
  | CH2 _|---|___|---|___|_    |--------->| TI2 -> both edges of both      |
  |      quarter period apart  |  wire B  |         inputs counted         |
  |                            |--------->|   |                            |
  | CH3 index, one pulse per   |  wire Z  |   v                            |
  |     N counts               |--------->| CNT, 16 bit, counts up or down |
  +---------------------------+          |   |                            |
        |                                 |   +--> CH3 capture on index    |
        | the harness knows this number   |   |                            |
        v                                 |   v                            |
  expected_counts                        | extend to 64 bit, signed        |
        |                                 |   |                            |
        |                                 |   v                            |
        |                                position_counts ---> velocity     |
        |                                 |                     (step 6)   |
        +---------------> assert equal <--+                                |
                                          +--------------------------------+
```

## Repository layout

```text
joint-node/
  board/  nucleo_h7a3zi_q.yaml            # + five pins, with evidence
  doc/    real-or-modelled.md             # + corrected: position is half real
  src/
    bsp/     board.h + generated headers
    node/    identity.c state.h indicator.c selfcheck.c
    control/ period.c jitter.c loop.h
    time/    mono.c capture.c wall.c skew.c
    sense/
      encoder.c  encoder.h                # + this chapter: the decoder
      quadgen.c  quadgen.h                # + this chapter: the stand-in source
      velocity.c velocity.h               # + this chapter: and its error
    act/  bus/  mw/  safety/  update/
  test/
    test_encoder.c                        # + extension across reversal
    test_velocity.c                       # + quantisation, on the host
  host/  tools/  README.md
```

## Steps

**Step 1.** **Choose the two timers, and record the rule rather than the answer.** The decode timer needs encoder mode, a free pair of channels on free pins, and preferably a thirty-two bit counter. The generator needs two compare channels and does not care about width. Three of those properties are on chapter 4's confirm list for this part, so the selection rule goes in the text and the chosen instances go into the board description with their evidence.

```text
decode timer:    encoder mode required
                 32-bit counter preferred, 16-bit acceptable with the extension
                 two channels free after the shield in chapter 6 is fitted
                 a third channel free for the index capture
generator timer: two compare channels, any width
                 not TIM2 (the monotonic clock) and not TIM6 (the period)
```

**Step 2.** **Configure the encoder interface, and know which mode you chose.** This part offers exactly three encoder modes, and the vendor's own header for this family names all three: count on the edges of the first input, count on the edges of the second, or count on the edges of both. The third gives four counts per quadrature cycle and is what a joint node wants, because the resolution is free.

```c
/* encoder.c: the three modes this part offers, from the vendor's own header.
   TI12 counts both edges of both inputs: four counts per cycle. */
void encoder_init(void)
{
    TIM_Encoder_InitTypeDef enc = {
        .EncoderMode = TIM_ENCODERMODE_TI12,   /* the other two: TI1, TI2 */
        .IC1Polarity = TIM_ICPOLARITY_RISING,
        .IC1Selection = TIM_ICSELECTION_DIRECTTI,
        .IC1Filter = 4,                        /* see the pitfalls */
        .IC2Polarity = TIM_ICPOLARITY_RISING,
        .IC2Selection = TIM_ICSELECTION_DIRECTTI,
        .IC2Filter = 4,
    };
    HAL_TIM_Encoder_Init(&htim_enc, &enc);
    HAL_TIM_Encoder_Start(&htim_enc, TIM_CHANNEL_ALL);
}
```

The relationship between those three mode names and one, two or four counts per cycle is standard reference manual semantics. It was not read in the manual for this part during the research for this volume, so it is confirmed on the bench in step 7 by generating a known number of cycles and counting what arrives. That is a better check than a citation anyway.

![Figure 5.4. Quadrature, and where the counts come from.](../figures/j05_data.svg)

*Figure 5.4. Quadrature, and where the counts come from. Four states in a fixed order, and the order is the direction: a decoder does not need to know which way the shaft is turning, it only needs to know which state followed which. Every marked edge is one count in the mode this node uses, which is four per cycle and costs nothing extra. The index is the only thing in the whole signal that can recover an absolute position after a dropped edge.*

**Step 3.** **Extend the counter, and handle the direction.** This looks like chapter 3's extension and it is not, because an encoder counter runs both ways. An overflow can be an increment past the top or a decrement past zero, and the direction bit in the timer says which.

```c
static volatile int32_t g_wraps;          /* signed: it can go negative */

void ENCODER_TIM_IRQHandler(void)
{
    if (TIM_ENC->SR & TIM_SR_UIF) {
        TIM_ENC->SR = ~TIM_SR_UIF;
        /* The direction bit tells you which way the counter crossed. Getting
           this wrong costs one full counter range per reversal at the wrap,
           which is a position error of 65536 counts appearing from nowhere. */
        if (TIM_ENC->CR1 & TIM_CR1_DIR) g_wraps--;   /* counting down */
        else                            g_wraps++;   /* counting up   */
    }
}

int64_t encoder_position(void)
{
    int32_t w1, w2; uint32_t cnt;
    do { w1 = g_wraps; cnt = TIM_ENC->CNT; w2 = g_wraps; } while (w1 != w2);
    return ((int64_t) w2 << 16) + (int64_t) cnt;     /* 16-bit counter */
}
```

**Step 4.** **Latch the index in hardware, not in an interrupt handler.** This part has no hardware index support: the vendor states that the feature arrived with a later family and was not available when this one was designed, and the corresponding configuration type and function are simply absent from the headers for this part while being present for the family that has them. The interrupt route works and is late by whatever the interrupt latency is, which at speed is a position error. The better route uses the same technique as chapter 3: put the index on a third channel of the same timer, as an input capture, so the hardware records the counter value at the edge.

```c
/* The capture register holds CNT at the index edge, whenever the handler runs. */
void ENCODER_CAPTURE_IRQHandler(void)
{
    uint32_t at_index = TIM_ENC->CCR3;        /* the count when Z went high */
    g_index_seen = true;
    g_index_count = ((int64_t) g_wraps << 16) + at_index;
    g_offset = g_index_count;                 /* zero is now the index mark */
}
```

Whether a capture channel behaves this way while the same timer is in encoder mode is the one thing in this chapter that must be confirmed on the bench before it is trusted, and the confirmation is easy: generate the index at a known count and compare. It goes on chapter 4's confirm list with that test named.

**Step 5.** **Build the generator.** One timer, two compare channels at fifty per cent duty, the second delayed by a quarter of the period. Reversing the direction is a matter of swapping which channel leads.

```c
/* quadgen.c: two square waves a quarter period apart, and an index every N. */
void quadgen_set(int32_t counts_per_second, bool forward)
{
    uint32_t period = quadgen_period_for(counts_per_second);
    TIM_GEN->ARR  = period - 1u;
    TIM_GEN->CCR1 = period / 2u;                       /* channel A */
    TIM_GEN->CCR2 = forward ? (period / 4u)            /* B lags A  */
                            : (3u * period / 4u);      /* B leads A */
    g_expected_direction = forward ? +1 : -1;
}
```

**Step 6.** **Get a velocity, and state its error before quoting it.** Position changes in whole counts, so the difference over one control period is an integer. At a low speed that integer is zero or one, and the resulting velocity is mostly quantisation. The arithmetic is worth doing once, in the chapter, rather than discovering it in chapter 16.

```c
/* The counting method: counts in one period. Simple, and quantised. */
float velocity_counting(int64_t pos, int64_t prev, float dt)
{
    return (float) (pos - prev) / dt;        /* one count in 1 ms = 1000 c/s */
}

/* The timing method: the interval between edges, using the capture from
   chapter 3. Fine at low speed, useless at high speed when edges arrive
   faster than they can be timestamped. */
float velocity_timing(uint64_t t_now_us, uint64_t t_prev_us, int32_t counts)
{
    uint64_t dt_us = t_now_us - t_prev_us;
    return dt_us ? (float) counts * 1000000.0f / (float) dt_us : 0.0f;
}
```

With four thousand counts per revolution and a one millisecond period, one count per period is fifteen revolutions per minute. Below that speed the counting method reports either zero or fifteen and nothing in between, which is not a velocity, it is a quantiser. The node uses the counting method above a threshold and the timing method below it, and reports which one produced each figure.

**Step 7.** **Prove the decoder against a known count.** This is the acceptance test the generated source makes possible, and it is stronger than anything a real encoder would allow: the number of counts that should have arrived is known exactly, so the criterion is equality.

```bash
python host/encoder_sweep.py --rates 100,1000,10000,100000,1000000 --seconds 10
# rate      generated    decoded   error   direction
#    100         1000       1000       0   forward  PASS
#   1000        10000      10000       0   forward  PASS
#  10000       100000     100000       0   forward  PASS
# 100000      1000000    1000000       0   forward  PASS
#1000000     10000000    9999872    -128   forward  FAIL: input filter too slow
```

**Step 8.** **Inject the failures the generator cannot produce naturally.** A perfect source is the limitation of this whole approach, so the two failures that can be simulated are simulated deliberately: a missing edge, by suppressing one compare event, and a direction reversal exactly at the counter wrap, which is the case step 3 exists for.

```bash
python host/encoder_faults.py --drop-one-edge --at-count 32768
# decoded position after a dropped edge: off by 1 count, permanently: PASS
python host/encoder_faults.py --reverse-at-wrap
# position continuous across the reversal at the wrap: PASS
```

The first result is the important one and it is not a defect: a quadrature decoder cannot detect a missing edge, so the error is permanent until the next index pulse. That is exactly what the index is for, and it is why an incremental encoder without an index is a poor choice for a joint.

**Step 9.** **Correct the honesty table and the banner.** One row changes from real to half real, and the boot line that names modelled quantities gains its first entry.

```bash
joint-node-1  node 3  v0.5.0  built 2026-09-21
modelled: position source (generated quadrature, see chapter 5)
```

## Build, flash and debug

![Figure 5.5. Velocity from a position that only moves in whole counts.](../figures/j05_timing.svg)

*Figure 5.5. Velocity from a position that only moves in whole counts. Above, the counting method at three speeds: at the lowest it reports zero or one count per period and nothing in between. Below, the timing method on the same motion, using the capture timestamps from chapter 3. The crossover is a design decision and the node reports which method produced each figure.*

```bash
cmake --build build -j && probe-rs run --chip STM32H7A3ZITx build/firmware.elf
python host/encoder_sweep.py --rates 100,1000,10000,100000 --seconds 10
```

> [!NOTE]
> **When the position jumps by exactly 65536**
>
> The extension in step 3 counted a wrap in the wrong direction. It appears only when the shaft reverses within one counter range of the wrap point, which on a bench that only ever runs forward may never happen at all, and on a joint that oscillates around a position happens constantly. A second signature, a position that drifts by a few counts per reversal rather than jumping, is the input filter: too short and the decoder counts noise, too long and it misses genuine edges at speed. The sweep in step 7 is how the filter length gets chosen.

## Verification and acceptance criteria

- Decoded counts equal generated counts exactly, at every rate up to the one where the input filter begins to miss edges, and that rate is recorded.
- Reversing direction at an arbitrary point changes the sign of the velocity and leaves the position continuous.
- Reversing direction within one counter range of the wrap leaves the position continuous, proven by the injected case rather than by inspection.
- The index capture yields the same count every revolution, to zero counts, and the difference between the captured count and one read in the handler is reported as the latency that technique avoids.
- The velocity crossover is measured rather than assumed: the speed below which the counting method is quantisation is computed from the counts per revolution and the period, and the reported figure names its method.
- A dropped edge produces a permanent one-count error that the next index pulse corrects, and the node reports the correction rather than hiding it.
- The boot banner names the generated position source, and `doc/real-or-modelled.md` has been corrected.

## Variants

| Axis | Variant | What changes | Cost | Built in full in |
| --- | --- | --- | --- | --- |
| Decode | Timer encoder interface | The baseline. The counting is in hardware and costs the processor nothing at any speed | One timer, two pins | Here |
| Decode | Two interrupts and software | Works on any part with two spare pins, and the processor pays for every edge | At a million counts a second it is the whole processor | Nowhere. Named as what the hardware saves you |
| Resolution | One, two or four counts per cycle | The three modes this part offers. Four is free and is the default here | None | Here |
| Index | Input capture on a third channel | The count at the edge, in hardware, whenever the handler runs | One channel | Here |
| Index | External interrupt and read | The obvious route, and it is late by the interrupt latency, which at speed is a position error | Accuracy at speed | Here, as the comparison |
| Index | A hardware index in the timer | The clean answer, and this part does not have it: the vendor states it arrived with a later family | Not available | Nowhere, and the evidence is in step 4 |
| Velocity | Counting in a fixed period | Simple, and quantised at low speed | Resolution at low speed | Here |
| Velocity | Timing between edges | Fine at low speed, and it degrades where the first improves | Complexity, and a timestamp per edge | Here |
| Velocity | An observer over both | The usual industrial answer: a model that fuses position with a command | A plant model, which arrives in chapter 7 | Chapter 16 |
| Source | Generated on the same board | The stand-in. Exact, repeatable, and unable to produce a real encoder's failures | The honesty rule applies | Here |
| Source | A magnetic sensor over a serial bus | An angle rather than a count, and absolute rather than incremental | It does not feed this peripheral at all, so the decoder is unused | Nowhere. See the prior art table |

*Table 5.5. Variants for chapter 5. The two index rows are the pair worth reading together: the technique this part supports, and the one it does not, with the evidence for the second.*

## Pitfalls

- Extending the counter without consulting the direction bit. The error is one full counter range and it appears only on a reversal near the wrap.
- Choosing the input filter length by taste. Too short counts noise as motion; too long misses edges at speed. The sweep chooses it.
- Reading the counter and the wrap count without a retry. Chapter 3 made this mistake unavailable; the same discipline applies here.
- Reading the index in a handler and calling the counter value at that moment the index position. At a million counts a second, interrupt latency is tens of counts.
- Quoting a velocity at low speed without saying which method produced it. Below one count per period the counting method reports a number that is entirely an artefact of quantisation.
- Assuming a popular magnetic angle sensor can drive this peripheral. Several cannot: they provide an angle over a serial interface and no quadrature output at all.
- Buying a five volt encoder without checking the tolerance of the exact pins, which is a per-pin property in the datasheet.
- Forgetting that the loopback proves the decoder and not the sensing. The figure and the banner both say so, and the chapter says it three times because it is the one claim a reader might overstate.

## Best practices applied

- The stand-in is separated from the thing it stands in for by an interface, so the real part can replace it without touching the code under test.
- The acceptance criterion is equality rather than a tolerance, which is possible only because the source is known exactly, and that is the one advantage a generated source has over a real one.
- The failures the stand-in cannot produce are named, and the two that can be injected are injected.
- A claim taken from a forum answer is corroborated against the vendor's own headers before it is built on.
- A semantic detail not confirmed in the reference manual is tested on the bench instead, and the text says which it is.
- An earlier chapter's classification is corrected in the open, in the file that exists for it, rather than quietly adjusted.

## Stretch goals

- Drive the generator from a recorded trajectory rather than a constant rate, so the decoder is exercised with accelerations and reversals that resemble a joint rather than a test rig. Chapter 20 needs exactly this.
- Measure the maximum decode rate against input filter length and plot the two against each other. That figure does not appear to be published for this part.
- Add a second decoder instance on another timer and decode the same signal twice, which is the shape of a redundant position channel in a safety related design, and measure how often the two disagree.
- Emit the quadrature with a transfer engine instead of compare channels, so the generator can play an arbitrary pattern including deliberately malformed quadrature.

## Roadmap and next steps

Chapter 6 adds the inertial unit, which is the first sensor on this bench that measures something physical, and it uses chapter 3's capture technique to stamp its samples at the pin.

The published progression from here is the motor control project cited above: its encoder handling, its velocity filtering and its angle tracking are real robot code in the same idiom, and reading its velocity path next to this chapter's is the fastest way to see the trade-offs a shipping project makes. The vendor's timer application note is the other half and is currently an existence claim only in this volume: it could not be fetched during the research and is cited by number and title without anything being quoted from it.

## Portfolio evidence

- The sweep table, showing exact equality up to a measured rate and the rate at which the input filter starts to matter.
- The reversal-at-wrap test, which is the defect most quadrature code has and most test benches never exercise.
- The index latency comparison: the same index, captured in hardware and read in a handler, with the difference in counts at a stated speed.
- The corrected honesty table, and the boot banner naming its first modelled quantity. A reader who sees a project correct its own claim upward in rigour learns more about the author than a clean table would have told them.

## Sources

Normative references:

- Reference manual RM0455, for the encoder interface, the direction bit, the input filter settings and whether a capture channel may be used while the timer is in encoder mode.
- The datasheet for this part, for the five volt tolerance of the specific pins chosen.
- The vendor's timer application note, cited by number and title as an existence claim. It could not be fetched during the research for this volume and nothing is quoted from it.

Reusable implementations:

- The vendor's hardware abstraction driver for this family, BSD-3-Clause, whose header settles that this part offers exactly three encoder modes.  
  <https://github.com/STMicroelectronics/stm32h7xx-hal-driver>
- The field oriented motor control project, MIT, for encoder handling and velocity filtering in shipping robot code.  
  <https://github.com/simplefoc/Arduino-FOC>
- A permissively licensed driver for one magnetic angle sensor, MIT, named in the prior art table as the part that cannot drive this peripheral.  
  <https://github.com/RobTillaart/AS5600>

---

[Previous](04-the-board-support-package.md) &nbsp;&nbsp;|&nbsp;&nbsp; [Contents](../README.md) &nbsp;&nbsp;|&nbsp;&nbsp; [Next](06-the-inertial-unit-as-the-joints-inner-ear.md)
