# The source pool for the joint-node volume

Assembled Monday 21 September 2026 from four research sweeps. Every entry
carries one of three marks, and the mark is part of the entry:

- **verified**: the page, file or PDF was fetched and read, or read in a browser.
- **indexed only**: the item appeared in a search listing, the page could not be
  opened, and nothing about its contents is claimed.
- **could not confirm**: looked for, not found, or found in a form that
  contradicts the usual claim.

A chapter may cite an **indexed only** item as an existence claim, naming
document number and title and nothing more. A chapter may never quote one,
never summarise its contents, and never build a step on it.

**What this file does not do.** It names libraries, standards, repositories,
papers and their licences, because that is the whole point of it. It does not
name the employer, the robot product line or the town from the advert this
volume was written beside. That rule is in `AUTHORING.md` and the linter
enforces it. It applies to the book's prose, not to the software it depends on.

Three rules apply to every chapter:

1. **Name the licence in the chapter that introduces the dependency**, not only
   in an appendix. A reader decides what to depend on before writing code around
   it.
2. **Never present generated vendor code as reviewed source.** Name the
   peripherals and the constraints, then show the code written on top.
3. **Check the claim before repeating it.** Twenty-one claims in wide
   circulation turned out to be wrong, and they are listed at the end. Where the
   correction is more interesting than the claim, it becomes chapter material.

---

## Hosts that could not be read, and what follows

`st.com` timed out or reset on every automated attempt across all four sweeps,
including direct downloads. Its product pages did load in a browser. **Every
silicon fact in this volume was therefore settled from ST's own machine
readable sources on GitHub, from Internet Archive copies of ST PDFs, or from the
`stmcu.com.cn` mirror.** That is a stronger method than reading prose. What
remains unsettled is under "Open questions" at the end.

Also unreadable from this machine: `iso.org`, `webstore.ansi.org`, `sae.org`,
`misra.org.uk`, `pilz.com` and Microchip and Infineon product pages (403,
although their asset PDFs work), `docs.ros.org` and `roscon.ros.org` (anti-bot
interstitial, so the ROS 2 documentation was read from the `ros2_documentation`
repository instead), `ieeexplore.ieee.org`, `developer.arm.com` in part,
`autosar.org` and `www.cds.caltech.edu` (both serve broken certificate chains),
`fbswiki.org` (refused), `koopman.us` (refused) and `philkoopman.org` (does not
resolve; the live host is `users.ece.cmu.edu`).

---

## Licence categories, decided once

**Category A, safe to depend on and to vendor.**

Apache-2.0: MCUboot, CANopenNode, Lely core, `ros2_control`, `ros2_socketcan`
and `ros2_socketcan_msgs`, `ros2_canopen`, `ethercat_driver_ros2` (but read the
warning under chapter 17), the whole micro-ROS and eProsima Micro XRCE-DDS
stack, `rclc`, CMSIS-DSP, `mjbots/moteus`, Zephyr and its coredump and state
machine framework, Percepio `TraceRecorderSource`, FlashDB, Pigweed `pw_kvs`,
ST `cmsis-device-h7`.

MIT: Renode, `hsmcpp`, `kiishor/UML-State-Machine-in-C`, `amaiorano/hsm`,
Ceedling with Unity and CMock, `meekrosoft/fff`, CmBacktrace, EasyLogger,
SPIFFS, EasyFlash, `simplefoc/Arduino-FOC`, `pms67/PID`,
`lFatality/stm32_micro_ros_setup`, `RobTillaart/AS5600`, cantools,
`pylessard/python-udsoncan`, `mercedes-benz/odxtools`, `feaser/canfdshield`,
the MIT forks of `isotp-c`, ODrive v3.x (legacy only), FreeRTOS kernel,
`CanOpenSTM32`.

BSD-3-Clause: littlefs, ST `stm32h7xx-hal-driver`, ST
`stm32-util-eeprom-emulation`, `control_msgs`, `realtime_tools`,
`joshnewans/diffdrive_arduino`, `openxc/isotp-c`, `openxc/uds-c`.

Plus every United States government work: the NASA software assurance handbook
material and MIL-STD-1629A. Plus `semver.org` (CC BY 3.0), RFC 9019 and
RFC 9124, and Casini and co-authors, ECRTS 2019 (CC BY 3.0).

**Category B, safe to depend on, not to vendor and not to link.**

- **SOEM, GPLv3** since v2.0.0 of Friday 11 July 2025, dual licensed with a paid
  commercial option. The licence moved toward stronger copyleft, not away.
- **IgH EtherCAT Master (EtherLab), GPLv2** kernel modules with an LGPLv2.1
  userspace library; the README does not state which files fall on which side,
  so the split **could not be confirmed**.
- **SOES, GPLv2 with a linking exception.** Usable if the exception's source
  availability clause is honoured. Read it before relying on it.
- **OpenBLT, GPLv3** or paid commercial. The best off-the-shelf match to chapter
  19 and the most consequential licence decision in the volume.
- **wolfBoot, GPL-3.0** plus commercial, quoted per end product.
- **QP/C and QP/C++, GPLv3** plus commercial; SafeQP is commercial only.
- **`devcoons/iso15765-canbus`, AGPL-3.0.** The strongest copyleft in the pool.
- **The phryniszak bit timing calculator, AGPL-3.0.** Use the hosted page.
- **Caring Caribou, GPL-3.0**, active. **FAIL\*, GPL-3.0**, dormant since
  Wednesday 15 December 2021.
- **python-can, LGPL-3.0**; **Labgrid, LGPL-2.1**; **`mike-matera/FastPID`,
  LGPL-2.1.** Fine as host side dependencies, not to be statically linked into
  firmware.
- **`tttapa/Arduino-Filters` and `Arduino-Helpers`, GPL-3.0.** Cite the author's
  web pages freely, write the code fresh.
- **VESC (`vedderb/bldc`), GPL-3.0-or-later**, with a trap: no top level licence
  file, so automated scanners report no licence and give false comfort.
- **`jonas-merkle/AS5047P`, GPL-3.0.** Use SimpleFOC's MIT driver instead.
- **can-utils**, dual GPL-2.0-only and BSD-3-Clause per file. Invoking the
  binaries is unaffected; check the per file marker before copying source.
- **`open-fmea` and `openfmea`, both GPL-3.0**, and neither is mature.

**Category C, read, do not copy.** Vendor licences that survive redistribution.

ST **SLA0044 "Ultimate Liberty"** (rev 6, October 2025 text read) and
**SLA0048** (rev 4, March 2018 text read) both permit source redistribution with
notices retained, **but** each carries a clause restricting use and execution to
ST manufactured devices and a clause forbidding the software from being
subjected to open source terms, naming GPL, EPL, Apache, BSD and MIT
explicitly. A public repository with one permissive licence file at the top that
appears to cover the whole tree breaches the second clause. **Do not vendor
SLA0044 or SLA0048 files.** Reference them, or fetch them at build time, and
keep the repository's own licence scoped explicitly to the author's own code.

Also here: **X-CUBE-MCSDK** (SLA0048, and ST claims patents on parts of the
architecture), **X-CUBE-STL** (object code only, conditions of use bound,
licence terms **unverified**), **Beckhoff ET9300 Slave Stack Code** (free of
charge but gated behind ETG membership, redistribution restricted), **SAFERTOS**
(commercial, price on application), **Tracealyzer** the viewer (commercial),
**SISTEMA** (free to use and to pass on, **modification and re-hosting not
permitted**), and TI's safety package with its FMEDA and assessment reports
(available under non-disclosure only).

**Category D, no licence at all, therefore all rights reserved.**

`geekfactory/PID` (dead since Monday 10 April 2017), `fzxhub/can_tp`,
`ControlLTH/Jitterbug` (licence could not be determined), CANToolz (licence
type could not be confirmed), and `akospasztor/stm32-bootloader` (the host
reports no assertion and the licence path returns 404). Reported as negative
findings, never used.

**A fifth situation, and the one most likely to trip this book up.** The
`memfault-firmware-sdk` reads like a three clause permissive licence until its
additional clauses, which require the software to be used **only with Memfault's
own service and server**, plus a no reverse engineering clause. For an
independent public repository that is a licence breach regardless of
attribution. The Interrupt articles are free to read and are cited throughout;
the kit is not used, and chapter 20 says so in one paragraph.

**A sixth: share-alike on content rather than code.** Koopman's lecture slides
may be downloaded for personal and academic use with attribution, and his pages
state that posting them publicly is prohibited and that for-profit training use
needs permission and a fee. **His slides cannot be reproduced in this book.**
Cite and link only. The control tutorials at CTMS are CC BY-SA 4.0: paraphrase
and attribute, reproduce no text or figures.

**Paywalled standards.** Cite by number, title, edition and publication date.
Reproduce nothing where the document carries a no-reproduction notice. Prices
are recorded so a chapter can tell a reader what a decision costs.

---

# The pool, chapter by chapter

## Chapter 01. What a joint node is, and the bench that stands in for one

| Source | What it gives | What it does not | Licence |
|---|---|---|---|
| `ros-controls/ros2_control`, `mock_components/GenericSystem` **verified** | The published precedent for a node that exists to exercise the framework with no hardware attached: "provide ideal behavior by mirroring commands to their states". Parameters `calculate_dynamics` (Euler integration, so the mock moves rather than teleports), `position_state_following_offset` (inject a constant following error deliberately), `disable_commands`, `mock_sensor_commands`, `mock_gpio_commands`, `initial_value` | A mock, not a plant model. Asserts nothing about tracking quality; the offset parameter is a fault injector, not a metric | Apache-2.0 |
| Zephyr board page and device tree for `nucleo_h7a3zi_q` **verified** | The board's facts in machine readable form: STM32H7A3ZI, Cortex-M7, 280 MHz, 2 MB flash, about 1.4 MB SRAM, FDCAN with two instances, user manual UM2408, USB Micro-AB, SWD, Zio and morpho headers | No RJ45, no PHY, no MAC node. No transceiver | Apache-2.0 |
| `renode/renode` **verified**, 2,914 stars, last push Monday 21 September 2026 | A second bench needing no hardware. `platforms/cpus/stm32h7.repl` declares `cpu: CPU.CortexM` with `cpuType: "cortex-m7"` and two controllers modelled as the Bosch M_CAN IP: `fdcan1: CAN.MCAN @ sysbus 0x4000A000`, `fdcan2 @ 0x4000A400`, message RAM at `0x4000AC00`. Sibling files for H743, H747 and H753 | No platform file for the H7A3, and no motion metrics at all | MIT (the API reports NOASSERTION only because the file is not byte identical to the template) |

Left to write: the identity of the node, the boundary between what is real on
this bench and what is modelled, and the three-gap table the honesty rule
requires. No prior art exists for that table, which is the point of it.

## Chapter 02. The control period: 1 kHz you can prove

| Source | What it gives | Licence |
|---|---|---|
| FreeRTOS kernel `include/task.h` doxygen for `xTaskDelayUntil` **verified from the raw header** | The quotable definition: "Delay a task until a specified time. This function can be used by periodic tasks to ensure a constant execution frequency", against `vTaskDelay`, which blocks "for the specified number of ticks from the time vTaskDelay() is called". `pxPreviousWakeTime` is caller owned and updated inside the call. Gated by `INCLUDE_xTaskDelayUntil`. **The return value is a free overrun detector**: `pdFALSE` means the deadline had already passed | MIT |
| Koopman, CMU 18-348 Lecture 15, "Interrupt and Cyclic Task Response Timing", Monday 14 March 2016, at `users.ece.cmu.edu/~koopman/lectures/` **verified, read** | Slide 12 reproduces a "Bad Code on an RTOS" example, a loop of delay then work, annotated **"Delay for 100ms not same as 'run every' 100ms."** That single line justifies the chapter, and `HAL_Delay(1)` is the identical defect. Slide 13: response time is "max time until computation *starts* running" | **Cite and link only. Reproduction prohibited** |
| Ganssle, "Interrupt Latency" **verified** | "In a real system, interrupts come often. Latency varies depending on what other things are going on." Two measurement methods, the second needing no oscilloscope: the handler reads the timer count register, which keeps incrementing during the latency delay | article |
| Cervin, Henriksson, Lincoln and Arzen, "Jitterbug and TrueTime: Analysis Tools for Real-Time Control Systems", 2nd Workshop on Real-Time Tools, Copenhagen, August 2002 **verified, read** | The sentence to build on: "Digital control theory normally assumes equidistant sampling intervals and a negligible or constant control delay from sampling to actuation. However, this can seldom be achieved in practice." Jitterbug computes a quadratic performance criterion under timing conditions; TrueTime co-simulates plant, tasks and network | paper. The Lund tool page 404s, do not cite it. `ControlLTH/Jitterbug` licence **unknown**, category D |
| Schwarzmann and Kaeser, "On the Effect of Sampling-Time Jitter", arXiv:2509.04199, Thursday 4 September 2025 **verified** | The modern statement that jitter "effectively scales the system matrices", so a varying period perturbs the plant model rather than adding noise | preprint |
| Styger, "Cycle Counting on ARM Cortex-M with DWT", Monday 30 January 2017 **verified**, and SEGGER's knowledge base article on the same counter **verified** | Enable sequence: set bit 24 `TRCENA` in `DEMCR` at `0xE000EDFC`, then bit 0 `CYCCNTENA` in `DWT_CTRL` at `0xE0001000`; the counter is at `0xE0001004`. **Two traps**: the cycle counter is optional silicon, so show the runtime check; and a connected debugger usually sets `TRCENA` already, so the code works under the debugger and fails standalone | articles |
| Andrews, "Trace Cortex-M software with the Instrumentation Trace Macrocell", Wednesday 13 September 2017 **verified** | "writing to the ITM is an internal 32-bit register write in the Cortex-M processor", one store, cheap enough to leave in a 1 kHz handler | article. **But the output still needs a probe**, so it is out on this bench |

Wittenmark, Nilsson and Torngren, "Timing problems in real-time control
systems", American Control Conference 1995, is **indexed only**: do not print
page numbers.

**The centrepiece method, and it is the author's construction.** At 280 MHz a
cycle is 3.57 ns and one control period is exactly 280,000 cycles. Read
`DWT->CYCCNT` in the handler, subtract the previous value, bin `delta - 280000`
into a small histogram in RAM with running minimum and maximum, and dump it over
the bus. The histogram **is** the jitter distribution in 3.57 ns bins, with no
probe of any kind. **No published article presents this recipe.** Present it as
built on Ganssle's counter-read idea and Styger's enable sequence, not as a
citation.

Caveat for a callout box, **indexed only**: on a Cortex-M7 the cache, branch
prediction and long pipeline widen the spread between minimum and maximum
measured execution time far more than on an M4, and repeated measurements of
identical code differ.

**Negative finding to print: no ST application note on building a fixed-period
digital control loop exists.** AN4776, "General-purpose timer cookbook for STM32
microcontrollers", revision 3 of July 2019 with a revision 4 of February 2026,
is **indexed only** and is a peripheral guide that would not fill the gap
anyway.

## Chapter 03. One clock for sensors, loop and bus

**The finding that shapes the chapter: the FDCAN peripheral already
timestamps.** Verified from the ST community thread "How does FDCAN time
stamping work exactly on the STM32H7?". Registers `TSCC` (timestamp counter
configuration) and `TSCV` (timestamp counter value). Quoted: "On start of frame
reception/transmission the counter value is captured and stored into the
timestamp". **The capture point is the start-of-frame bit, not the interrupt**,
so interrupt latency is removed from the measurement entirely. The counter
clocks from the internal bit time or from TIM3. Two cautions from the same
thread: FDCAN and TIM3 sit in **different clock domains** (`fdcan_tq_ck` against
APB) and are not synchronised, with about 5 per cent drift observed; and the
counter is **16 bit and wraps within seconds**. The construction that works is
TIM3 as a 16 bit auto-reload timer extended on update events into a **stable 48
bit monotonic timer in software**, with correction in post-processing. The
register names come from the Bosch M_CAN IP. **RM0455 is the correct reference
manual for this part**; it was not fetched, so verify the clause numbers before
printing them.

| Source | What it gives | Licence |
|---|---|---|
| CiA 301 version 4.2.0, Monday 21 February 2011 **verified, 158 page PDF read** | **Clause 7.2.5**, quoted: the SYNC producer broadcasts periodically, the period is the communication cycle period parameter, and "There may be a time jitter in transmission by the SYNC producer corresponding approximately to the latency due to some other message being transmitted just before the SYNC." **The standard itself states that a sync frame's jitter is one message transmission time.** That is the hard floor without reception timestamping. Clause 7.2.5.3.1: the optional one byte counter, incremented per transmission, wrapping at the overflow value, reset to 1 on boot-up and on the transition out of stopped, which is how a node knows which cycle a sample belongs to. Clause 7.2.6: TIME, six bytes, given a very high priority identifier. Clause 7.1.6.5 TIME_OF_DAY: 48 bits, milliseconds after midnight plus days since Sunday 1 January 1984. Table 38: NMT at 000h, **SYNC at 080h**, **TIME at 100h**. Objects 1005h, 1006h, 1007h, 1012h, 1013h, 1019h. **Clause 7.5.2.16, object 1013h**, quoted: "The value is given in multiples of 1 µs", UNSIGNED32, PDO mappable, so it **wraps about every 71.6 minutes** and the node must handle it | free after registration, CiA copyright |
| CiA 603 version 1.1.0, "CAN network time management", Friday 1 September 2023, status DS **verified from the CiA listing** | The document most directly on this topic: timestamping on transmission and reception in both frame formats, described by CiA as usable in particular in AUTOSAR environments | members only. Cite by number and title, quote nothing |
| Gergeleit and Streich, "Implementing a Distributed High-Resolution Real-Time Clock using the CAN-Bus", 1st International CAN Conference, Mainz, September 1994 **verified, read** | About 20 microseconds accuracy, under 20 messages a second, one identifier. **The sentence to put in the book**, on why this works on CAN and not on Ethernet: bus length is always much shorter than one bit time, so every node sees the level at about the same moment, a property inherent to the arbitration method, "thus the delay on a CAN network can be approximated as zero (this is not true for other networks, like e.g. Ethernet)". Why the naive protocol fails: eight bytes at 125 kbit/s takes about a millisecond, and even at 1 Mbit/s the critical path is an order of magnitude above the target accuracy. **The refined protocol**: any node broadcasts an indication frame, every participant including the master timestamps its reception, then the master sends a second frame carrying its own timestamp for that indication, and each slave derives its correction. The critical path is reception to timestamp only, local to each node, so it is characterised once and subtracted. **Adaptive clocks**: never jump, because that gives non-monotonic time; speed up or slow down by leaving out or repeating single ticks | freely downloadable from CiA |
| Einspieler, Rathakrishnan, Prabhakara, Steinwender and Elmenreich, "High Accuracy Software-Based Clock Synchronization Over CAN", IEEE Transactions on Systems, Man, and Cybernetics: Systems, 2021 **verified, author's version read** | Quoted: "We show that an advanced timer module combined with additional system knowledge allows sub-microsecond precision and accuracies." About twenty times better than the 1994 result, **in software over ordinary CAN**, using an IEEE 1588 capable high resolution timer with **rate correction** that many off-the-shelf microcontrollers already carry. A ready made related work section. Its critique of the field: most approaches "solely correct the clock's offset but do not modify the clock's tick rate", so two events can share a local timestamp although their global times differ, which "must not be allowed in a real-time system"; and most do not investigate the communication path, the transceivers or the interrupt behaviour at all | IEEE copyright, cite and link |
| `rmw_microxrcedds` `rmw_microros/time_sync.h` **verified, the declaring header** | `rmw_uros_sync_session(timeout_ms)`, documented as "Synchronizes the session time using the NTP protocol"; `rmw_uros_epoch_synchronized()`; `rmw_uros_epoch_millis()` and `..._nanos()`, offset corrected, returning 0 if the session is uninitialised. **An NTP shaped round trip yielding an offset only**: no rate correction, no hardware assistance, accuracy bounded by round trip asymmetry, and over a CAN-FD transport it inherits arbitration delay, which is asymmetric by construction. Right for wall-clock epoch alignment, **wrong for aligning a 1 kHz period**, and the chapter says so plainly | Apache-2.0 |

Named, normative, and not used because the hardware is absent: **IEEE
1588-2019**, "IEEE Standard for a Precision Clock Synchronization Protocol for
Networked Measurement and Control Systems", approved Thursday 7 November 2019,
published Tuesday 16 June 2020, active, seven amendments. **IEEE 802.1AS**,
"Timing and Synchronization for Time-Sensitive Applications", whose **2020
edition is superseded by the 2025 edition**. **RFC 5905**, NTP version 4, June
2010, relevant only because the middleware session sync is NTP shaped.
**ISO 11898-4:2004**, time-triggered communication, **indexed only** but
corroborated by the 2021 paper above, which states that this hardware
timestamping "is exclusively implemented in the TTCAN transceivers, not readily
available in generic CAN modules", and that "not even CAN-FD as a successor of
CAN includes this technology".

**AUTOSAR time synchronisation is indexed only**, because `autosar.org` serves a
broken certificate chain. Real document identifiers seen in indexing:
`AUTOSAR_FO_PRS_TimeSyncOverCANProtocol` (Foundation R25-11, document 1089,
Thursday 27 November 2025) and `AUTOSAR_CP_SWS_TimeSyncOverCAN` (Classic
Platform R23-11). The mechanism, **indexed only**: module `CanTSyn`, a time
master sends SYNC then FUP, with OFS and OFNS offset messages, and CAN FD with
the extended message format merges the two offset messages into one. Mark that
paragraph as indexed only in the manuscript or drop it.

**There is no CiA 1301.** If a source mentions one, it is wrong.
**SAE J1939-71 could not be confirmed** and is the application layer in any
case, not a time synchronisation document.

## Chapter 04. The board support package, and a board file you can hand over

The two authoritative, fetchable, redistributable sources for this part are
`STMicroelectronics/cmsis-device-h7` (Apache-2.0) and
`STMicroelectronics/stm32h7xx-hal-driver` (BSD-3-Clause, v1.11.6). Both were
read directly and between them they settle the memory map, the power supervision
thresholds, the flash geometry and the backup domain.

**The headline board fact, proven twice from ST's own headers: the STM32H7A3ZI
has no Ethernet MAC.** In `stm32h7a3xx.h` there is no `ETH_TypeDef` and no
`ETH_IRQn`; the `IRQn_Type` list runs `DMA2_Stream4_IRQn = 60` and then jumps
straight to `FDCAN_CAL_IRQn = 63`, with vector slots 61 and 62 simply absent. In
`stm32h743xx.h` those same two slots are `ETH_IRQn = 61` and
`ETH_WKUP_IRQn = 62`, and `ETH_TypeDef` is present. Zephyr's board page and
device tree corroborate: no RJ45, no PHY, no MAC node.

**A trap worth documenting, because a reader grepping upstream will conclude the
opposite.** Zephyr's `dts/arm/st/h7/stm32h7a3.dtsi` ends with
`&mac { memory-regions = <&sram3>; };`, because it inherits the shared
`stm32h7.dtsi` and deletes `adc3` and `crc` but not `mac`. It is a modelling
artefact, not a capability.

**The licence mixture inside one vendor tree is itself chapter material.** The
verified component table in `STM32CubeWB/LICENSE.md` lists the HAL, the CMSIS
device headers, every board support driver and FatFS as BSD-3-Clause; the CMSIS
core as Apache-2.0; the USB device library, the touch sensing library and the
wireless stack as SLA0044; the audio library as SLA0047; and the bundled RTOS
under its own vendor licence. **A repository cannot be called permissively
licensed merely because of which vendor tree it came from.** Check each
component.

**No transceiver on the board.** Zephyr's page for the sibling H723ZG states it
outright: the board "does not have any onboard CAN transceiver. In order to use
the FDCAN bus on this board, an external CAN bus transceiver must be
connected." The H7A3 board's own file does not mention the bus peripheral once,
while the sibling H743 board's file enables it and names its two pins. The
silicon has it, the board does not expose it, and upstream reflects exactly
that. ST's own feature list, read in a browser, confirms a CAN FD header on the
board and "2x watchdogs (independent and window)".

## Chapter 05. The encoder: quadrature in hardware, and one you generate

**Two verified findings, and both are constraints.**

**The H7 defines exactly three encoder modes.** From
`stm32h7xx_hal_tim.h`: `TIM_ENCODERMODE_TI1`, `TIM_ENCODERMODE_TI2`,
`TIM_ENCODERMODE_TI12`. That is the whole list. The usual mapping of these to
two-times and four-times counting is standard reference manual semantics but
**was not read in RM0455 this session**: the macro list is verified, the mapping
is standard but unread.

**The STM32H7 has no hardware encoder index, and ST says so.** Two independent
confirmations. An ST staff answer on the ST community forum, Saturday 14 March
2020: "this is a new feature of the timer, and the STM32G4 is the first one to
embed it. It was not available when the F7 and H7 were designed." And the
headers themselves: the H7 has no `TIM_ENCODERINDEX_*` macro, no index
configuration structure and no `HAL_TIMEx_ConfigEncoderIndex`. The STM32G4
headers, by contrast, define `TIMEx_EncoderIndexConfigTypeDef` with polarity,
prescaler, filter, first index enable, position and direction fields. **Note the
`TIMEx_` prefix, not `TIM_ENCODERINDEX`, which is a common miswriting.** So the
index on this part is an external interrupt plus software, and that is the
chapter.

**Sensor caveat to print: the AS5600 has no quadrature output and no SPI**, so
it cannot feed the timer encoder peripheral. `RobTillaart/AS5600` is MIT and is
the right library if that part is used over its own interface. For the AS5047P,
**`jonas-merkle/AS5047P` is GPL-3.0**: use SimpleFOC's MIT `as5047` driver or
write fresh from the datasheet. **The ams-OSRAM product URLs for these parts are
dead**; the datasheets are now hosted by Infineon, effective Wednesday 1 July
2026.

Reusable code for the encoder idiom itself is in chapter 07's motor control
projects, which read encoders in the same loop that drives the bridge.

## Chapter 06. The inertial unit as the joint's inner ear

ST's register level driver collection for these sensors, permissively licensed
throughout behind a two-function porting shim, is established in the sibling
firmware volume and is not re-surveyed here. This chapter cites that volume for
the driver layer and spends its pages on timestamp alignment with the control
period, which is chapter 03's machinery applied to a sensor.

## Chapter 07. The actuator you do not have: PWM, dead time, and a plant model

| Source | What it gives | What it does not | Licence |
|---|---|---|---|
| `simplefoc/Arduino-FOC` **verified**, 3,034 stars, last push Wednesday 5 August 2026 | Real robot code: the field oriented idiom, bridge driving, encoder reading, and a controller that **measures elapsed time rather than assuming a period**, with overflow handling. The opposite design choice from chapter 16's fixed period controller, and the contrast is worth printing | Arduino shaped; its controller has no derivative filter | MIT |
| `mjbots/moteus` **verified** | A complete open brushless controller with a documented bus protocol, from a robotics vendor | Different silicon | Apache-2.0 |
| ODrive v3.x **verified** | A widely read controller design | Frozen, and classic CAN only | MIT |
| VESC (`vedderb/bldc`) **verified** | A large body of motor control practice | **GPL-3.0-or-later with no top level licence file**, so scanners report nothing. Reading only | GPL-3.0 |
| ST X-CUBE-MCSDK **verified as SLA0048** | ST's own motor control suite | **Not BSD-3-Clause**, contrary to common belief; use restricted to ST devices, mixed component licences, and ST claims patents on parts of the architecture. Category C | SLA0048 |
| TI InstaSPIN | | **Could not be verified at all**: the tool page 404s. Omit it | |

Left to write: the complementary output configuration with dead time and a fault
input on this part, and the second-order plant in software that stands in for a
joint that is not on the bench. The plant model is the author's. The dead-time
generator encoding is in RM0455 and is on the open questions list.

## Chapter 08. Force and torque: the signal you cannot buy

No prior art worth the name was found, and that is the finding. The chapter
states what the signal is, what it is for, what a real sensor requires, and the
error of the stand-in, and it labels the estimate honestly everywhere it
appears. One of the three gaps the honesty rule names.

## Chapter 09. CAN-FD from the controller out: bit timing and the first frame

**The correction that opens the chapter.** The sibling volume's warning that this
part differs from the popular member of its family is true at board, clock and
RCC-register level and **false at the bus peripheral level**. A diff of ST's own
device headers shows the same licensed Bosch M_CAN core at the same base
addresses, `0x4000A000` and `0x4000A400`, with the calibration unit at
`0x4000A800` and message RAM at `0x4000AC00`, and identical bit timing field
positions. Material written for the H743's FDCAN is directly reusable. Three
real differences: the clock selection register name, `RCC_CDCCIP1R_FDCANSEL`
here against `RCC_D2CCIP1R_FDCANSEL` there, **same bits 29:28**; the domain
naming, CD and SRD against D1, D2 and D3; and the maximum core frequency, 280
against 480 MHz, which changes the arithmetic that lands a clean kernel clock,
not the registers that consume it.

Frame format is confirmed at HAL level in `stm32h7xx_hal_fdcan.h`:
`FDCAN_FRAME_CLASSIC`, `FDCAN_FRAME_FD_NO_BRS`, `FDCAN_FRAME_FD_BRS`.

| Source | What it gives | Licence |
|---|---|---|
| ST **AN5348, "Introduction to FDCAN peripherals for STM32 MCUs"** **indexed only** | The peripheral introduction, by number and title. Nothing quoted | vendor document |
| `feaser/canfdshield` **verified** | A ready made transceiver shield in Arduino form factor for Nucleo-64 and Nucleo-144, with a TI TCAN3413DR transceiver. Same author as OpenBLT | MIT |
| The phryniszak bit timing calculator **verified** | Bit timing for both phases, sample point, and the arithmetic laid out | **AGPL-3.0**: use the hosted page, never vendor it |

**Buying advice that belongs in the chapter.** A one megabit transceiver still
gives full 64 byte frames and the improved checksum, because frame format is
invisible to the transceiver; what it costs is the switch to a faster data
phase. The part number does not tell you which you have: one family carries no
format marker in its name and is rated five megabit, while another separates one
megabit from five megabit parts by a single letter suffix in the same datasheet.
Read the suffix. The chapter states which part was used and what it allowed.

## Chapter 10. The motion master: the bus on Linux

| Source | What it gives | Licence |
|---|---|---|
| The kernel's SocketCAN documentation **verified** | The whole host side in one free document: `vcan` "offers a virtual local CAN interface" enabling transmission and reception "without real CAN controller hardware"; error frames through the ordinary filter mechanism via `CAN_RAW_ERR_FILTER`, with `CAN_ERR_TX_TIMEOUT` and `CAN_ERR_BUSOFF`; controller states ERROR-ACTIVE, ERROR-WARNING, ERROR-PASSIVE, BUS-OFF, STOPPED; counters for restarts, bus errors, arbitration lost, warnings, passive, bus-off and overrun; `struct canfd_frame` with `CANFD_MTU` of 72 bytes and separate arbitration and data bitrates; and **`restart-ms`**, quoted: "If set to a non-zero value, a restart of the CAN controller will be triggered automatically in case of a bus-off condition after the specified delay time in milliseconds" | GPL-2.0 documentation, quote briefly |
| `linux-can/can-utils` **verified**, 2,917 stars, last push Sunday 20 September 2026 | `cangen` with `-g` gap in milliseconds (default 200, so `-g 0` is a storm), `-c` burst count, **`-f` for FD frames**, `-b` bit rate switch, `-E` error state indicator, `-L` length generation mode; `canplayer` with `-I`, `-l` loop or infinite, `-t`, `-g`, `-n`, and **interface remapping such as `vcan2=can0`**; plus the ISO-TP tools `isotpsend`, `isotprecv`, `isotpsniffer`, `isotpdump`, `isotpserver`, `isotpperf`, `isotptun` | **Dual, per file**: the `LICENSES/` directory holds BSD-3-Clause, GPL-2.0-only and the syscall note. Invoking binaries is unaffected; check the SPDX tag before copying source |
| python-can **verified**, 1.6k stars | Host side library, FD supported, SocketCAN and `vcan0` documented | **LGPL-3.0**, category B |
| cantools **verified** | Database driven encoding and decoding, which is how chapter 11's protocol becomes readable on the host | MIT |
| `mjbots/fdcanusb` **verified** | An adapter presenting both the kernel interface and a serial port, with genuine FD rates | Apache-2.0 |

**The adapter finding, and it changes a purchase.** The inexpensive USB analyser
that appears on shopping lists ships, for its FD variant, a Linux archive that
is a precompiled closed shared library with a vendor API of its own. It defines
its own frame structure rather than the kernel's, it carries **no licence file
at all**, and it never mentions SocketCAN. Nothing in `can-utils` will see it.
For a book about mainline Linux, and for a public repository, it is the wrong
adapter regardless of the operating system list on the box.

## Chapter 11. A joint protocol: state and command in sixty-four bytes

CiA 301's pre-defined connection set gives the identifier allocation convention
and the priority argument, verified under chapter 03. `CANopenNode`
(Apache-2.0, 2,008 stars, last push Friday 10 July 2026) is the reference
implementation of the object dictionary and communication objects.
**ISO 15765-2:2016**, verified from ISO's own preview, carries what a 64 byte
frame needs: clause 6 "ISO 11898-1 CAN data link layer extension" with 6.1
comparing the two frame features and **6.3 "Additional requirements for CAN
FD"**; 9.2.1 and 9.2.2 for single frames at and above eight bytes; **9.5
"Transmit data link layer data length (TX_DL) configuration"** with 9.5.4 on
receiver determination; and 10.4 on the length code. That machinery is the
reason to cite the 2016 edition rather than the 2011 one. Its foreword also
confirms that part 3 was withdrawn and replaced by ISO 14229-3.

Ring buffers, interrupt reception, transfers and framing are the sibling
volume's subject. This chapter points there.

## Chapter 12. Network management: heartbeat, node state, bus-off and recovery

Object indices verified from `CANopenNode` source rather than from a paywalled
specification: **1001h** the error register, stated in `CO_Emergency.h` to
reside at that index and be computed from an internal bitfield of critical bits;
**1014h** the emergency identifier, with a message sent on each change of any
error condition; **1015h** its inhibit time, optional; **1016h** consumer
heartbeat time; **1017h** producer heartbeat time, confirmed twice, once in
`CO_NMT_Heartbeat.h` and once as `OD_H1017_PRODUCER_HB_TIME = 0x1017U` in
`CO_ODinterface.h`. The emergency frame is two bytes of error code, one byte of
error register, one byte of error condition index and four bytes of additional
information, with ranges 0x10xx generic, 0x20xx current, 0x30xx voltage, 0x40xx
temperature, 0x50xx hardware, 0x60xx software, 0x80xx monitoring, 0xFFxx device
specific. CiA's own page confirms the emergency object is broadcast and
"transmitted only once per error event".

The bus-off half comes from the SocketCAN documentation quoted under chapter 10.
`restart-ms` is exactly the knob for the disconnect-then-recover demonstration,
and it is free.

**The contrast worth printing.** The emergency object is event driven and sent
once. SAE J1939's **DM1, PGN 65226, "Active Diagnostic Trouble Codes", is
broadcast every 1 second** with the active fault list plus lamp status, a single
frame for zero or one codes and multi-frame beyond; DM2 is identical in
structure but carries previously active codes; DM3 to DM60 are request only. A
code is SPN (19 bits), FMI (5 bits), CM (1 bit) and an occurrence count (7
bits), and the header carries four two-bit lamp signals: protect, amber warning,
red stop, malfunction indicator. **A joint node probably wants both shapes.**
The standard is **SAE J1939-73, "Application Layer, Diagnostics", current
revision J1939-73_202609 issued Wednesday 9 September 2026**: pin the revision,
since five earlier ones exist. Paywalled; the structural description above comes
from a free secondary source, CSS Electronics, and is marked as such.

## Chapter 13. Two speeds on one wire, and why the old node errors

**ISO 11898-1:2024, "Road vehicles, Controller area network (CAN), Part 1: Data
link layer and physical coding sublayer", third edition, May 2024**, 82 pages,
about EUR 245. It supersedes the 2015 edition, **whose subtitle was different,
"Data link layer and physical signalling"**, so the edition must be cited, not
just the number. Scope now covers Classical CAN, CAN FD and CAN XL, with data
fields to 2,048 bytes, and splits the data link layer into LLC and MAC
sublayers. Two resellers disagree on the day in May, so write "May 2024".
**Paywalled and not fetched**, so the error counter and recovery text is taken
from the SocketCAN documentation instead, which names the same states and gives
the recovery mechanism in a free, quotable form.

Demonstration tooling: `cangen` with `-f` and `-b`, and `vcan`, both verified
under chapter 10.

## Chapter 14. The node as a middleware participant, and the agent that hosts it

**The honest headline: micro-ROS on this exact board is a porting job, not a
supported configuration.** The official hardware list has two tiers. Officially
supported, with long term support guaranteed: STM32L4 Discovery IoT, Olimex
STM32-E407, ROBOTIS OpenCR 1.0, Crazyflie 2.1, Renesas RA family. Community
supported: NUCLEO-F446ZE, NUCLEO-F746ZG, **NUCLEO-H743ZI**. **The
NUCLEO-H7A3ZI-Q appears in neither tier**, and the nearest entry is a different
silicon line.

| Source | What it gives | What it does not | Licence |
|---|---|---|---|
| `micro-ROS/micro_ros_stm32cubemx_utils` **verified**, 275 stars, last push Monday 15 September 2025, branches per distribution | The board agnostic route: a Docker image builds a **static library** that drops into a CubeMX or CubeIDE project. Sample mains for the default and UDP and embeddedRTPS paths, `colcon.meta`, and `extra_sources/` with time, allocator and transport shims | **Docker is mandatory.** Transports shipped, quoted from its README: "U(S)ART with DMA, U(S)ART with Interrupts, USB CDC, UDP". **CAN is not among them.** Carries the project's own "not ready for production use" notice | Apache-2.0 |
| eProsima Micro XRCE-DDS **Agent** **verified** | `MicroXRCEAgent canfd` is a first class transport on Linux, with the stated constraint that "the used interface must support CAN FD frames with a maximum payload of 64 bytes". Full transport list: `udp4`, `udp6`, `tcp4`, `tcp6`, `serial`, `multiserial`, `canfd`, `pseudoterminal` | | Apache-2.0 |
| Micro XRCE-DDS **Client** CAN transport **verified** | `include/uxr/client/profile/transport/can/can_transport.h` defines `UXR_CAN_TRANSPORT_MTU 63` with the comment that for CAN-FD the MTU is a fixed value, and `uxr_init_can_transport(transport, dev, can_id)` | **The implementation is POSIX only**: the source directory holds `can_transport.c` and `can_transport_posix.c` and nothing else. SocketCAN only. **On FreeRTOS this is a custom transport job** | Apache-2.0 |
| The custom transport API **verified** | `rmw_uros_set_custom_transport(framing_enabled, args, open_fn, close_fn, write_fn, read_fn)`, with `eprosima::uxr::CustomAgent` on the far side. `framing_enabled` false gives packet oriented mode, **which is what a CAN-FD frame wants**, and `RMW_UXRCE_TRANSPORT` accepts `custom` | | Apache-2.0 |
| `notblackmagic`, "Micro-ROS" **verified** | **The closest published precedent**: a port to a custom STM32H723 board, also Cortex-M7, covering toolchain, static library build, IDE integration and writing the transport functions | No flash, RAM or latency figures | article |
| `lFatality/stm32_micro_ros_setup` **verified**, 129 stars | An STM32F429ZI example with a DMA UART transport, and a companion video | Different part | MIT |

CAN and CAN FD arrived in Micro XRCE-DDS **v2.1**, announced on Wednesday 5
January 2022, with the Renesas RA family as reference hardware.

**The footprint question, answered honestly.** The only published figure with a
primary source covers the middleware layer alone: "less than 75 KB of Flash
memory and around 3 KB of RAM for a complete publisher and subscriber
application handling messages sizes on the order of 512 B". That is Micro
XRCE-DDS Client, **not** the full micro-ROS stack of rcl, rclc, rmw and type
support. **No official figure in text form for the complete client stack on a
Cortex-M7 was found**; the official benchmarking page publishes graphs with no
tables and targets a Cortex-M4 board. **The widely repeated "32 KB RAM and
256 KB flash minimum" comes from a commercial blog, not from the project: do not
print it as a micro-ROS figure.**

What **can** be said precisely is what the cost is made of. `rmw_microxrcedds`
"tries to fully rely on static memory assignations", and the compile time caps
are documented: transport selection, maximum nodes, publishers, subscriptions,
services and clients (4 each by default), history (8), stream history (4, a
power of two), name length caps, and graph support and dynamic allocations both
off by default. **That table is the honest answer to what the middleware costs:
a configurable static budget set in `colcon.meta`, plus the README's stated
requirement that "the micro-ROS task has more than 10 kB of stack", plus the
75 kB and 3 kB middleware core.** micro-ROS's RTOS set is FreeRTOS, Zephyr and
NuttX, so FreeRTOS is on the supported path.

**The agent on the Raspberry Pi, with honest status.** There is **no apt
binary**: the package index shows only version 0.0.1 from 2019 and "No version
for distro" against every current distribution. Three routes remain: a source
build through `micro_ros_setup` (Apache-2.0, 509 stars, last push Friday 18
September 2026, supporting Humble, Jazzy, Kilted and Rolling), which is the live
route; the Docker image, whose page **does not state architecture support**, so
verify arm64 before relying on it; and a snap whose stable channel has not been
updated since Thursday 9 June 2022 and which the store itself flags as stale.

**The specification.** OMG, "DDS For Extremely Resource Constrained
Environments", version 1.0, document `formal/20-02-01`, February 2020, status
Formal, **verified**. Supersedes the two beta versions of May 2018 and March
2019. **No version beyond 1.0 is listed.**

**Documentation host correction: `micro.ros.org` now redirects to
`micro.vulcanexus.org`, and every `micro.ros.org/docs/...` deep link returns
404.** Cite the new host or the markdown sources on GitHub.

Three things are the author's on this part: the CAN-FD custom transport, for
which **no FreeRTOS reference exists**; the H7 memory protection and cache
configuration, which the integration's own README flags for this family; and
cache coherency around DMA buffers, which is the sibling volume's subject and is
cited there.

## Chapter 15. Joint state and joint command as messages

All message definitions below were verified by reading the `.msg` files in
`ros2/common_interfaces`.

**`sensor_msgs/msg/JointState`**: a header, then `string[] name`,
`float64[] position`, `float64[] velocity`, `float64[] effort`. Its own comments
settle the units: position in radians or metres, velocity in radians or metres
per second, effort in newton metres or newtons. Two rules worth quoting: "the
header specifies the time at which the joint states were recorded. All the joint
states in one message have to be recorded at the same time", and the arrays are
optional but must be the same size or empty. **That timestamp rule is chapter
03's problem restated as a message contract.**

**`trajectory_msgs/msg/JointTrajectory`**: a header, `string[] joint_names`, and
an array of `JointTrajectoryPoint`, each carrying positions, velocities,
accelerations, effort and `time_from_start` as a duration, with units per joint
type in the comments.

**`control_msgs`** (BSD-3-Clause) adds the members a joint node actually needs:
`JointJog` for real time jog commands, `JointCommand` as a semantic alternative
to a bare float array, `JointTrajectoryControllerState`, `DynamicJointState`,
`SingleDOFState`, `MultiDOFStateStamped`, `GripperCommand`,
`AdmittanceControllerState`. Its action **`FollowJointTrajectory`** carries
`path_tolerance`, `goal_tolerance` and `goal_time_tolerance` in the goal, the
result codes `SUCCESSFUL = 0`, `INVALID_GOAL = -1`, `INVALID_JOINTS = -2`,
`OLD_HEADER_TIMESTAMP = -3`, `PATH_TOLERANCE_VIOLATED = -4`,
`GOAL_TOLERANCE_VIOLATED = -5`, and feedback giving desired, actual and error
points plus an index. **Those five error codes are a ready made vocabulary for
what a joint node reports when it cannot follow.**

For the concepts, the source of record is the `ros2/ros2_documentation`
repository, branch `jazzy`, since `docs.ros.org` refuses automated fetching. A
node is "a participant in the ROS 2 computational graph" and "each node should
do one logical thing"; connections form through "a distributed discovery
process". Topics are for "continuous data streams, like sensor data, robot
state", are anonymous and **strongly typed**, and the strong typing argument is
in a firmware engineer's own language: the type carries semantics and units and
"permits no alternative interpretations".

**The closest existing prior art to this book's whole architecture is
`ros-industrial/ros2_canopen`** (Apache-2.0, 289 stars, master serving Jazzy and
later plus a `humble` branch, `canopen_core` 0.3.4 released Sunday 24 May 2026).
Built on the Lely core libraries, its driver hierarchy runs
`NodeCanopenBaseDriver` (SDO, PDO, NMT), `NodeCanopenProxyDriver`, and
**`NodeCanopen402Driver`, which implements the CiA 402 motion profile with its
state machine and position, velocity and torque operation modes, exposed as
`ros2_control` hardware interfaces**. It is the maintained answer to "a CAN
connected joint appears as a hardware interface", and **this book's node is the
device side of that same contract**. Say so, and say what is left: this volume
builds the node, not the master.

At the raw frame level, `autowarefoundation/ros2_socketcan` (Apache-2.0, 194
stars, released into every current distribution) is a thin, well released
SocketCAN wrapper, and its companion `ros2_socketcan_msgs` version 1.4.0,
released Monday 7 September 2026, defines **`FdFrame`**, so CAN FD is covered.
It gives no semantics above the frame. `can_msgs` (BSD, from
`ros-industrial/ros_canopen`) defines `Frame`.

## Chapter 16. The loop closed over the bus: setpoint in, state out, following error

### The host side contract

`ros-controls/ros2_control`, Apache-2.0, about 1.0k stars, last commit Thursday
17 September 2026, **verified**.

The **controller manager** is "the main component in the ros2_control framework.
It manages lifecycle of controllers, access to the hardware interfaces and
offers services to the ROS-world". Its `update_rate` parameter is "the frequency
of controller manager's real-time update loop. This loop reads states from
hardware, updates controllers and writes commands to hardware", in hertz,
**default 100**. The main thread attempts `SCHED_FIFO` at **priority 50**,
requiring the user in a `realtime` group with `rtprio` and `memlock` limits
configured, and a real time kernel is recommended. That is the Raspberry Pi side
of this book's story, and the arithmetic is worth printing: the default host
loop runs at one tenth the node's rate.

A **hardware component** derives from `hardware_interface::SystemInterface`,
`ActuatorInterface` or `SensorInterface`, with lifecycle callbacks `on_init`,
`on_configure`, `on_cleanup`, `on_activate`, `on_deactivate`, `on_shutdown` and
`on_error`, exports through `export_state_interfaces()` and
`export_command_interfaces()`, and carries the real time pair
`read(const rclcpp::Time&, const rclcpp::Duration&)` and `write(...)`,
registered with `PLUGINLIB_EXPORT_CLASS`. **Caution: the rendered documentation
summary omits the lifecycle state argument; check the header before printing
signatures.**

**The interface names are constants and the list is worth reproducing**, from
`hardware_interface/types/hardware_interface_type_values.hpp`: `position`,
`velocity`, `acceleration`, `effort`, `torque`, `force`, `current`,
`temperature`, `proportional`, `integral`, `derivative`, the two integral clamp
limits, and `feedforward`. **The gain interfaces matter: the framework already
anticipates a joint node that exposes its controller gains over the wire.**

**Asynchronous components** (`is_async="true"` with a scheduling policy, thread
priority and affinity) are how a slow hardware read is kept out of the control
loop. **`joint_trajectory_controller`** accepts the command interface
combinations position; position and velocity; position, velocity and
acceleration; velocity; effort; and position and effort, and exposes both the
action and a fire-and-forget topic, publishing state as
`JointTrajectoryControllerState`. **`joint_state_broadcaster`** publishes
`sensor_msgs/msg/JointState` on `/joint_states`, omitting fields a joint does
not provide. **`realtime_tools`** (BSD-3-Clause) supplies `RealtimeBuffer`,
`RealtimePublisher`, priority helpers and an async function handler.

`joshnewans/diffdrive_arduino` (BSD-3-Clause, 139 stars) is the structural
template for a hardware interface talking to a microcontroller over serial:
**swap serial for CAN-FD and the shape is the same.**

### The controller itself

| Implementation | Discretisation and anti-windup, read from source | Licence and activity |
|---|---|---|
| `pms67/PID` | **The best fit.** Trapezoidal integral, `integrator += 0.5f*Ki*T*(error+prevError)`; a **band-limited derivative on the measurement**, sign inverted so a setpoint step produces no kick; **clamping anti-windup with a separate integrator limit** computed from the headroom the proportional term leaves. About 150 lines, two functions, finished | **MIT at repository level only: no licence header inside `PID.c` or `PID.h`.** Add one if you vendor them. 935 stars, last push Monday 24 July 2023 |
| CMSIS-DSP `arm_pid_f32` | The incremental form `y[n] = y[n-1] + A0*x[n] + A1*x[n-1] + A2*x[n-2]` with `A0 = Kp+Ki+Kd`, `A1 = -Kp-2Kd`, `A2 = Kd`, in f32, q31 and q15 | Apache-2.0, active. **The teachable foil: no anti-windup, no derivative filtering, no output saturation**, the period baked invisibly into the gains, and an unfiltered second difference on the error, which is maximum noise amplification plus full derivative kick |
| SimpleFOC `src/common/pid.cpp` | Trapezoidal integral, plain backward difference derivative on the error, no filter, integral clamp plus output clamp, **plus an output slew rate limit** the others lack and a joint wants. Measures elapsed time instead of assuming a period | MIT, active |
| `br3ttb/Arduino-PID-Library` | The famous one, and a licence lesson | **MIT by a sentence in `README.txt` only**: no licence file, no SPDX line, no field in `library.properties`, so the API reports none. `double` based, `millis()` based, the sample period is an `int` in milliseconds so **1 kHz is its finest**, no derivative filter. `README.md` 404s |
| `mike-matera/FastPID` | The integer and fixed point reference | **LGPL-2.1**, category B |
| `geekfactory/PID` | Nothing usable | **No licence anywhere**, category D, dead since Monday 10 April 2017. A negative finding |

Theory at firmware-engineer level, verified and free: **tttapa (Pieter Pas),
"PID Controllers"**, giving forward Euler for the integral, backward Euler for
the derivative "yielding a causal formulation", Tustin named as the
higher-order alternative, the filtered derivative as finite differences plus an
exponential moving average, and the derivative-kick argument (take the
derivative of the measurement, equivalent when the setpoint is constant). Its
companion implementation page adds the two practical moves: premultiply the
gains by the period at configuration time so the handler does no division, and
**anti-windup by conditional integration**. Neither page covers windup
systematically nor argues for a constant period; do not cite them for those.

**Astrom and Murray, *Feedback Systems*, chapter 10, section 10.4 "Integrator
Windup"** is the right citation and is **indexed only**, because the hosting
site serves a broken certificate chain: **cite the book, not a link.** Astrom
and Hagglund, *PID Controllers: Theory, Design and Tuning*, second edition, ISA,
is the standard back-calculation source and **no fetchable copy was found**.

**No verified open embedded C implementation does textbook back-calculation.**
If the chapter shows it, it is the author's code.

The figure of merit distinction comes from **Koren, "Cross-Coupled Biaxial
Computer Control for Manufacturing Systems", Journal of Dynamic Systems,
Measurement, and Control volume 102, pages 265 to 272, 1980**, verified, which
is the origin of contouring error as a quantity distinct from axis following
error.

### Executor and scheduling, for the chapter's closing argument

`ros2/rclc` is Apache-2.0. Its executor is "analogous to rclcpp's Executor class
for C++" with "features for implementing deterministic timing behavior",
supports trigger conditions ALL, ANY, ONE and user defined, implements **logical
execution time** semantics, and is **single threaded**. The bibliography it
publishes is a ready made reading list, verified from the micro-ROS
documentation source: Staschulat, Luetkebohle and Lange, "The rclc Executor",
EMSOFT 2020, pages 18 to 19; Staschulat, Lange and Dasari, "Budget-based
real-time Executor for Micro-ROS", arXiv:2105.05590, Wednesday 12 May 2021
(**verified directly**); **Casini, Blass, Luetkebohle and Brandenburg,
"Response-Time Analysis of ROS 2 Processing Chains under Reservation-Based
Scheduling", ECRTS 2019, LIPIcs volume 133, DOI 10.4230/LIPIcs.ECRTS.2019.6,
published Tuesday 2 July 2019, CC BY 3.0 and therefore quotable**; Henzinger,
Horowitz and Kirsch, "Giotto", EMSOFT 2001; Liu and Layland, Journal of the ACM
volume 20 number 1, pages 46 to 61, 1973; and Ernst and co-authors, "The Logical
Execution Time Paradigm", Dagstuhl Seminar 18092.

## Chapter 17. What a real-time fieldbus would change, and why it is not on this bench

**The chapter's premise is settled twice over, and it is stronger than
"difficult".** The STM32H7A3ZI has no Ethernet MAC (chapter 04), and **ST sells
no EtherCAT slave controller at all**: a text search of the EtherCAT Technology
Group's own ESC overview for both ST names returns zero hits. So this is a
structural boundary, not a matter of effort.

**The asymmetry is the whole chapter.** ETG.2200 states: "The only hardware
requirement for an EtherCAT MainDevice is a standard Network Interface
Controller (NIC, 100 Mbit/s full duplex)." A PC network card is enough to be a
master. A SubDevice needs a dedicated controller chip, an EEPROM, and per port a
connector, magnetics, a PHY and passives.

Primary documents, both **verified and read**: the technology pages at
`ethercat.org`, and **ETG.2200, "EtherCAT Implementation Guide", V3.2.0,
Wednesday 7 August 2024**.

**How a SubDevice works.** Processing is in hardware, on the fly: "Each EtherCAT
SubDevice reads the data addressed to it 'on the fly', and inserts its data in
the frame as the frame is moving downstream." The controller contains a
processing unit, auto-forwarder and loopback, FMMUs, SyncManagers, process data
RAM, an EEPROM interface and a distributed clocks unit. Two ports minimum, with
loops closed on unconnected ports. FMMUs "convert logical addresses into
physical addresses by the means of internal address mapping", bit wise, with
configuration registers from `0x0600`. SyncManagers arbitrate the dual ported
RAM in **buffered mode** (three buffers, cyclic process data, the consumer
always gets the latest consistent buffer) or **mailbox mode** (handshake, no
data loss, for acyclic protocols). Distributed clocks give 64 bit system time
with a 1 ns base unit and an epoch of 1 January 2000, with propagation delay
measurement, offset and drift compensation in hardware; ETG.2200 claims "much
better than 1 microsecond" and Beckhoff claims better than 100 ns for the
ET1100. The host microcontroller attaches through the process data interface,
whose control register `0x0140` encodes digital I/O, SPI slave, bridge, and
8 and 16 bit synchronous and asynchronous microcontroller modes. **A hard
ceiling worth quoting: "the internal PDI interface can achieve a maximum
throughput of approx. 12.5 Mbyte/s."** **An effort estimate from the ETG
itself**, section 4.3.1: "To develop a new running SubDevice system, operated by
a standard EtherCAT MainDevice, about 6-8 weeks are feasible to get a working
solution."

**The chips**, from the ETG's own "EtherCAT SubDevice Controller (ESC)
Overview", July 2025, **verified and fully extracted**. The ones that matter
here:

| Part | Vendor | Ports | DPRAM | SM | FMMU | Host interface |
|---|---|---|---|---|---|---|
| ET1100 | Beckhoff | 2 to 4 | 8 KB | 8 | 8 | serial and parallel, 8 and 16 bit |
| LAN9252 | Microchip | 2 plus optional MII | **4 KB** | **4** | **3** | host bus, SPI, QSPI |
| LAN9253 / LAN9254 | Microchip | 2 plus optional | 8 KB | 8 | 8 | as above, plus digital I/O on the 9254 |
| LAN9255 | Microchip | 2 plus optional | 8 KB | 8 | 8 | **integrated SAM E53 Cortex-M4F, 1 MB flash, 256 KB SRAM** |
| AX58100 | ASIX | 2 with integrated PHYs | 9 KB | 8 | 8 | SPI, parallel |
| XMC4800 | Infineon | 2 MII | 8 KB | 8 | 8 | internal, Cortex-M4 with up to 2 MB flash |

**`AX58100` is the most joint-friendly plain controller**, and its own feature
list says why, verbatim: "3-channel PWM Controller; Step and Direction
Controller; Incremental and Hall Encoder Interface; Emergency Stop Input", with
two integrated PHYs, 80 pin package, datasheet v1.09 of Thursday 12 December
2024. **The XMC4800 integrates the controller but not the PHYs**, so it still
needs two external PHYs and magnetics. **The LAN9252 is the small one**: three
FMMUs, four SyncManagers, 4 KB, per its own 332 page datasheet DS00001909C of
2024. **ET1200 and ESC20 are legacy**: not listed in the July 2025 overview and
the product page 404s. **There is no SAMA5 with an integrated controller**; the
Microchip part with an integrated core is the LAN9255. TI's AM64x and AM243x
reach EtherCAT through PRU firmware and a certified stack rather than a fixed
function block, which is a useful framing: soft, protocol switchable, tied to
that vendor's stack.

**Description files.** An **ESI** file is EtherCAT SubDevice Information, vendor
supplied XML against an ETG schema, declaring identity (vendor, product code,
revision), object dictionary, PDO mapping, SyncManager configuration and mailbox
protocols. The configuration tool reads all ESI files and produces an **ENI**,
EtherCAT Network Information, and it is the ENI the master executes: topology,
per device init commands, cyclic command list. The same information in binary
lives in the SII EEPROM. Specification numbers seen on fetched pages:
**ETG.2000 "EtherCAT SubDevice Information (ESI) Specification", V1.21, Monday
26 May 2025**; ETG.2010 for the EEPROM interface; ETG.2100 for the network
information; **ETG.5001 Modular Device Profile**; **ETG.6010, "Implementation
Directive for the CiA402 Drive Profile"**, which is a directive, not the profile
itself; ETG.5100 for safety; plus ETG.1000, 1003, 1004, 1005, 1020, 1030, 1300,
1500, 1510, 1700, 5003, 7000, 9000, 9001 and 9002. **Corrections: ETG.2000's
current title says SubDevice, not Slave; and the individual titles of the
ETG.1000 parts could not be confirmed.**

**Normative anchors**, verified from ETG.2200 and the technology page:
"ETG.1000 represents the IEC 61158 - Type 12 (EtherCAT)"; "Both EtherCAT and
Safety over EtherCAT are IEC-Standards (IEC 61158 and IEC 61784)"; and for a
joint, "the drive profile CiA402 (IEC61800-7-201) is mapped to EtherCAT this
way". **ISO 15745 is not mentioned anywhere and was not confirmed: do not cite
it.**

**Open stacks, and the licence story is the chapter's second half.**

- **SOEM**, `OpenEtherCATsociety/SOEM`, 2,085 stars, last push Tuesday 8
  September 2026. **`LICENSE.md` states: "This software is distributed under
  GPLv3"**, with a commercial option and the sentence "If you intend to use this
  stack in a commercial product, you likely need to buy a license." The change
  landed with **v2.0.0 on Friday 11 July 2025**, which also added an ENI parser
  generating compilable C. The prior arrangement was GPLv2 with a linking
  exception. The API reports NOASSERTION only because the file is `LICENSE.md`.
- **IgH EtherCAT Master (EtherLab)**, `gitlab.com/etherlab.org/ethercat`,
  default branch `stable-1.6`, last activity Saturday 19 September 2026. Root
  carries **`COPYING` = GPLv2** and **`COPYING.LESSER` = LGPLv2.1**; the
  conventional split is GPL kernel modules and LGPL userspace, but **the README
  does not state the split, so it could not be confirmed**. Kernel space master
  with native and generic network drivers, a userspace library and a command
  line tool. Needs out-of-tree kernel modules.
- **SOES**, `OpenEtherCATsociety/SOES`, 847 stars, last push Tuesday 8 April
  2025. **GPLv2 with a linking exception**, quoted in its licence file.
  **Asymmetry worth a sentence: SOES kept the arrangement SOEM left behind.**
  Gives mailbox, CoE with object dictionary, SDO, PDO mapping, FoE and "an
  address offset based HAL for easy ESC read/write access via any interface",
  which is how it bolts to a controller over SPI. Gives no controller, no ESI
  file, no vendor identifier and no conformance.
- **Beckhoff Slave Stack Code (ET9300)**: ANSI C, covers the state machine, CoE,
  AoE, EoE, FoE, synchronisation and **an example CiA 402 drive profile**.
  **Free of charge but gated behind ETG membership**, redistribution
  restricted. **Correction: the gate is membership, not a vendor identifier.**

**Cost, corrected.** ETG membership is free of charge and **"The Vendor ID is
free of charge."** Machine builders integrating EtherCAT devices are explicitly
**not required** to obtain one. **The real cost is the Conformance Test Tool, a
paid annual subscription, and ETG.2200 says its in-house use "is mandatory when
selling the device to the market."**

**The ROS 2 bridge, and its trap.** `ICube-Robotics/ethercat_driver_ros2`,
**Apache-2.0**, 335 stars, last push Monday 31 August 2026, default branch
`jazzy`: a `ros2_control` hardware interface where modules are described in
parameter files, so devices are assembled by configuration rather than C++ per
device. **But its installation procedure builds on the IgH master at
`stable-1.5` and requires disabling secure boot to load unsigned kernel
modules.** So it is a permissively licensed wrapper sitting on a GPLv2
out-of-tree kernel module. **The permissive licence on the wrapper does not
remove the GPL from the stack that gets deployed**, and that sentence belongs in
the chapter.

## Chapter 18. Safe states, and the workspace sensor that triggers one

### What a safe state is, from the definition itself

**IEC 61508-4:2010, Edition 2.0, April 2010, "Functional safety of
electrical/electronic/programmable electronic safety-related systems, Part 4:
Definitions and abbreviations"**, ISBN 978-2-88910-527-4. The official preview
was read at printed page 11, so this is **primary verified**.

**Clause 3.1.13, safe state: "state of the EUC when safety is achieved".** The
note adds that the equipment may have to pass through intermediate safe states,
and that **"for some situations a safe state exists only so long as the EUC is
continuously controlled"**. Neighbouring definitions: 3.1.11 safety, "freedom
from unacceptable risk"; 3.2.1 equipment under control.

**Two points for the chapter.** First, the definition is deliberately system
level and contains no engineering content. It does not say "power removed". What
counts as the safe state is an output of the risk assessment. Second, the note
is the load-bearing sentence for a robot joint: **for a horizontal or
non-backdrivable joint, removing torque is a safe state that persists with no
energy; for a vertical joint carrying a payload, removing torque is not a safe
state at all, because the axis descends.** There the safe state is brake engaged
with torque removed, reached through a controlled deceleration. **Torque-off and
brake-held are two different safe states**, and the drive standard names them
STO and SS1 while IEC 61508-4 names neither.

### Stop categories, performance levels and integrity levels, kept apart

| Axis | Document | What it specifies | Values |
|---|---|---|---|
| Stop **category** | IEC 60204-1 clause 9.2.2 | what the stopping action does to actuator power | 0, 1, 2 |
| **Performance Level** | ISO 13849-1 | how reliably the safety function is performed | a to e |
| **SIL** | IEC 62061 for machinery, IEC 61508 generally | the same question as a safety integrity level | 1 to 3, and 4 in IEC 61508 |

A well formed requirement names one value from the first column and one from the
second or third: "emergency stop shall be stop category 1, achieved with PL d per
ISO 13849-1". **There is no stop category 3 and no PL 1.** A category 0 stop
implemented with a single non-diagnosed contactor is still a category 0 stop, at
a poor performance level. **A cross-map table between PL and SIL is a
convenience, not a normative equivalence, and no primary verified mapping was
obtained, so the book prints none.**

The stop category definitions themselves sit behind the paywall. The wording is
identical across sources and was fetched from a secondary source: category 0 is
stopping by immediate removal of power to the machine actuators, an uncontrolled
stop; category 1 is a controlled stop with power available to achieve the stop,
then removal of power once stopped; category 2 is a controlled stop with power
remaining available. **Reliably attested, not primary verified**, and the
chapter says so.

**The mapping to the drive standard is the cleanest single fact in this area**,
from a verified vendor lexicon page: stop category 0 corresponds to **STO**,
category 1 to **SS1**, category 2 to **SS2**.

### The standards, correctly numbered and dated

| Standard | Title as published | Edition | Published | Price |
|---|---|---|---|---|
| IEC 61508-1 to -7 | Functional safety of electrical/electronic/programmable electronic safety-related systems, parts 1 to 7 | 2.0 | Friday 30 April 2010 | CHF 380 and up; part 1 is 127 pages, part 3 is CHF 405 |
| **IEC 62061:2021 + AMD1:2024** | **Safety of machinery, Functional safety of safety-related control systems** | 2.0 | Monday 22 March 2021, 304 pages | CHF 430 |
| **IEC 60204-1:2016 + AMD1:2021** | Safety of machinery, Electrical equipment of machines, Part 1: General requirements | 6.0 | Thursday 13 October 2016, 277 pages | paid |
| ISO 13849-1:2023 | Safety of machinery, Safety-related parts of control systems, Part 1: General principles for design | 4 | April 2023, 152 pages | paid |
| ISO 13849-2:2012 | Part 2: Validation | 2 | October 2012, 79 pages | paid, **stage 90.92, to be revised** |
| ISO 12100:2010 | Safety of machinery, General principles for design, Risk assessment and risk reduction | 1 | November 2010, 77 pages | paid, **stage 90.92** |
| **ISO 13850:2015** | Safety of machinery, Emergency stop function, Principles for design | 3 | cover prints Thursday 1 October 2015, catalogue says November 2015 | paid, 11 pages |
| **ISO 10218-1:2025** | **Robotics, Safety requirements, Part 1: Industrial robots** | 3 | February 2025, 95 pages | paid |
| **ISO 10218-2:2025** | Part 2: Industrial robot applications and robot cells | 2 | February 2025 | paid |
| ISO/TS 15066:2016 | Robots and robotic devices, Collaborative robots | first | Monday 15 February 2016, 33 pages | paid |
| IEC 61800-5-2:2016 | Adjustable speed electrical power drive systems, Part 5-2: Safety requirements, Functional | 2.0 | Monday 18 April 2016, 175 pages | CHF 380, **stability date 2026** |

Six corrections carried from this table:

1. **IEC 62061:2021 did not gain electrical coverage, it lost the restriction to
   it.** The 2005 first edition was titled "...functional safety of
   safety-related **electrical, electronic and programmable electronic** control
   systems"; the 2021 edition dropped that from the title and IEC's own change
   list says it was "extended to non-electrical technologies". It is now
   technology neutral. It also shifted from SILCL to the **maximum SIL** of a
   subsystem, introduced a functional safety plan, added a security reference and
   periodic testing, and added software use cases and independence requirements.
2. **Two of these carry amendments and a bare citation is incomplete in 2026**:
   IEC 60204-1:2016+AMD1:2021 and IEC 62061:2021+AMD1:2024.
3. **The ISO 10218 series is now titled "Robotics"**, not "Robots and robotic
   devices". Getting this wrong dates a manuscript at a glance.
4. **ISO/TS 15066 is not withdrawn.** ISO's catalogue shows it Published, stage
   90.92, "last reviewed and confirmed in 2022. Therefore this version remains
   current", to be replaced by ISO/AWI 15066-1, still under development. Its
   substance was folded into the 2025 robot standards, and several secondary
   sources say flatly that it was withdrawn. **They are wrong. Do not write
   "withdrawn".**
5. **ISO 13849-2 is changing its remit, not just its edition**: the draft reads
   "Part 2: Application of principles for the design and validation".
6. **ISO 13850:2015's normative references point at IEC 60204-1:2005**, now two
   editions behind. Worth a footnote.

**The type hierarchy, verified from ISO 13850's own introduction**: ISO 12100 is
the type-A standard, ISO 13850 is type-B2, ISO 10218 is type-C, and **where a
type-C standard differs the type-C provisions take precedence**. For a robot
joint, ISO 10218 wins.

**ISO 13850 clause 4.1.1, verified**: initiated by a single human action;
available and operational at all times; overrides all other functions and
operations in all modes without impairing other protective functions; maintained
until manually reset; no start command effective on the stopped operations;
reset by intentional human action, and the reset itself shall not initiate a
restart. **Clause 4.1.1.3: it is a complementary protective measure and shall
not substitute for safeguarding measures or other safety functions.** Clause
4.1.3 is titled "Stop categories" and sits on printed page 5, **beyond the end
of the free preview**, so the "categories 0 and 1 only" reading is cited by
clause number and attributed to a secondary source.

**ISO 13849-1:2023 abstract, verified**: it includes the design of software, and
applies to **high demand and continuous modes only, explicitly not to low
demand mode**. It does not specify which required performance level applies to a
given application. **The Categories, MTTF_D, DC and CCF machinery is inside the
paywalled body and could not be primary verified**: report it as standard
terminology, not as quoted text.

**IEC 61800-5-2:2016, verified**: requirements for safety-related power drive
systems within the IEC 61508 framework, with edition 2 replacing "safety
function" by **"safety sub-function"** throughout, which is why drive
documentation reads oddly. The sub-functions confirmed: **STO, SS1, SS2, SOS,
SLS, SLP**, plus SDI safe direction, SLI safely limited increment, SSR safe
speed range, SLA safely limited acceleration and SAR safe acceleration range,
each with a safely monitored variant. The distinction: a motion function
initiates a reaction on a limit violation, a monitoring function reports without
triggering one. **Two caveats: SOS is safe operating stop, often miswritten;
and SBC and SBT, the brake control and brake test functions, were not confirmed
on the page read, which matters because they are exactly what a gravity loaded
joint needs.**

**IEC 61508-3 is the part that touches software.** Its tool classification comes
from **IEC 61508-4 clause 3.2.11, read directly**: T1 generates no output
contributing to executable code (editors, requirements tools, configuration
control); T2 supports test or verification and can fail to reveal defects but
cannot introduce them (test harness generators, coverage tools, static
analysers); T3 generates output that contributes directly or indirectly to
executable code (compilers, especially optimising ones and those embedding a run
time package). **Primary verified, and the right frame for a section on
toolchain qualification.**

**MISRA, and the usual citation is out of date.** `misra.org.uk` returns 403, so
this rests on two independent secondary sources that agree: **MISRA C:2025,
"Guidelines for the use of the C language in critical systems", March 2025, is
current.** MISRA C:2023 exists (the month is not reliably confirmed, the year
is) and consolidated MISRA C:2012 with its amendments and corrigendum into one
document, adding C11 and C18. **"MISRA C:2012 with amendments" is no longer the
way to cite it.** MISRA Compliance:2020 is the compliance framework and is
mandatory from MISRA C:2023 onward. **MISRA C++:2023, "Guidelines for the use of
C++:17 in critical systems", October 2023**, merges the former MISRA and AUTOSAR
C++ guidance.

### Free artifacts worth more than the paywalled ones

- **TI SWAB013, "Industrial Functional Safety for C2000 Real-Time
  Microcontrollers"**, free, no registration, **verified**. It carries the
  distinction firmware engineers most often miss, in a per device table:
  **random hardware capability (SIL 2 on most parts) against systematic
  capability (SIL 3)**. Systematic capability is about the development process;
  random hardware capability is about diagnostic coverage and failure rates. It
  gives the architecture arithmetic in a line each: dual channel with hardware
  fault tolerance 1 reaches SIL 3 or Category 3 and 4 with PL e; single channel
  with fault tolerance 0 reaches SIL 2 or Category 2 with PL d. Then it names the
  mechanisms actually implemented in firmware: a windowed watchdog on an
  independent clock, background CRC of memory, memory self test, error correction
  and parity, lock registers on critical control registers, lockstep, safe state
  assertion through the output trip mechanism, a dedicated error status pin, and
  software test libraries with stated permanent fault coverage. It also names the
  sub-functions beyond safe torque off.
- **SISTEMA**, from IFA, free of charge, version 3.0.4 build 5 as of Friday 31
  October 2025, supporting ISO 13849-1:2023, **verified**. Models the control
  structure and computes the attained performance level. **Free to use and to
  pass on, but modification and re-hosting are not permitted.** Link it, do not
  vendor it.
- **Stolte, Ackermann, Graubohm, Jatzkowski, Klamann, Winner and Maurer, "A
  Taxonomy to Unify Fault Tolerance Regimes for Automotive Systems: Defining
  Fail-Operational, Fail-Degraded, and Fail-Safe", IEEE Transactions on
  Intelligent Vehicles volume 7 number 2, pages 251 to 262, 2022, DOI
  10.1109/TIV.2021.3129933**, verified through the preprint. Valuable because it
  documents that the literature is inconsistent and then fixes it, adding
  fail-degraded and fail-unsafe as distinct regimes, supplying a concise
  definition of the safe state, and noting that the automotive functional safety
  standard and the driving automation taxonomy define no fault tolerance regimes
  at all. IEEE copyright: cite and link, do not bundle.
- **Infineon AN1106, "FuSa in a Nutshell"**, version 1.0, Thursday 9 October
  2025, free, **verified**. Its section 3.2 on system faults, monitors and
  reactions is the fault reaction material. Its own cover says the use cases are
  "for training purposes only", so it is an analogy for a joint node, not an
  authority over it.

### On the target: watchdogs and the vendor's safety package

ST's own feature list confirms **two watchdogs, independent and window**. The
independent one is clocked from the low speed internal oscillator and survives a
stopped main clock; the window one faults on a refresh that is too **early** as
well as too late, which catches a runaway loop refreshing at the wrong rate.
**Both are on-die and share the chip's fate: a genuinely independent watchdog
for a safety argument is external**, and the chapter says so.

**X-CUBE-STL** is ST's IEC 61508 offering, read from ST's own page in a browser.
The package has four items: an MCU safety manual, a qualitative MCU FMEA, a
static FMEDA failure rate report, and the self test library itself. The library
is "a software-based diagnostic suite designed to detect random hardware
failures in STM32 application-independent core components (CPU + SRAM + flash
memory)", developed to a SIL 3 process, **application independent and compiler
independent because it is delivered as object code**, and certified by TUV
Rheinland. **The architecture claim, quoted correctly: "SIL2 safety functions
can be implemented with a single STM32 MCU; SIL3 safety functions implementation
requires two STM32 MCUs in an 1oo2 scheme."** Several secondary write-ups garble
this into 1oo1. **Not verified: the licence text, the distribution mechanism,
the current version, and whether the STM32H7A3 is covered at all.** That matters,
because an object-code-only library bound by conditions of use is not something
a book can tell readers to drop into a portfolio project. Category C, with the
licence marked open.

**RTOS.** The **FreeRTOS kernel is MIT** and its README says nothing about
safety certification: the plain kernel is uncertified and the book must not
imply otherwise. **SAFERTOS**, from WITTENSTEIN high integrity systems, "shares
a common functional foundation" with FreeRTOS but is a **separate, completely
redesigned product**, and the vendor's own framing is that customers prototype
on FreeRTOS and convert at the start of formal development. **It is not
"certified FreeRTOS".** Certified to IEC 61508 SIL 3 by TUV SUD, ISO 26262
ASIL D, IEC 62304, and designed for DO-178C to DAL A. **Cortex-M7 is not named
explicitly on the page read; confirm separately.** Royalty-free perpetual
commercial licence, three tiers, price on application.

### Safety over the bus

**EN 50325-5:2010, "Industrial communications subsystem based on ISO 11898 (CAN)
for controller-device interfaces, Part 5: Functional safety communication based
on EN 50325-4"**, verified from four national adoptions that agree (BS released
Thursday 30 September 2010, UNE Friday 1 October 2010, CSN Tuesday 1 March 2011,
and the DIN adoption). **This is a reseller catalogue, named as the source: four
independent national adoptions agreeing is good corroboration but it is not
CENELEC's own page.** Note the structure the title encodes: **EN 50325-4 is
CANopen itself and part 5 is the functional safety layer on top.** Cite them as
a pair.

**CiA 304** is the CiA specification this corresponds to, and **confirmation is
indirect**: CiA's own safety pages 404 and its CANopen page contains no mention
of the number. The number is corroborated by CANopenNode's repository and
documentation, which name "CANopen Safety, EN 50325-5, CiA304" together.
**CiA 304's own official title is not verified.**

The **SRDO** mechanism, **secondary only**: a safety-related data object is a
one-to-many periodic broadcast of **two frames**, the first carrying the data
and the second on a different identifier carrying it **bit-wise inverted**,
giving integrity without a heavyweight checksum and letting small
microcontrollers participate. There is no safety master. A SIL 3 approval claim
exists at secondary level and is a claim about the specification, not about any
implementation.

**`CANopenNode` implements it**: Apache-2.0, with `304/CO_SRDO.h`, `CO_SRDO.c`,
`CO_GFC.h` and `CO_GFC.c`, documented as "'PDO like' communication in
safety-relevant networks". It claims **MISRA C:2012 conformance with documented
exceptions**, which is a coding standard claim and **not a functional safety
certification**, and it makes no certification claim anywhere. It gives a
working, permissively licensed reference implementation of the wire format. It
gives no certification, no FMEDA, no safety manual and no assessed diagnostic
coverage.

**Two gaps to name rather than gloss over.** First, **EN 50325-5 is classic CAN,
not CAN FD**, and its title binds it to ISO 11898 and EN 50325-4; nothing in
this research established an equivalent published safety communication standard
for CANopen FD. State it as a gap. Second, the general framework for safety
communication over an arbitrary fieldbus is the **black channel** approach,
standardised in the **IEC 61784-3 family, whose number, title and edition were
not verified**, so do not print them without checking. The idea is exactly the
right frame for a host-to-joint link, and it is the answer to "how do I make my
own heartbeat protocol defensible": the safety layer detects corruption, loss,
repetition, resequencing, delay and masquerade, while the transport underneath
is untrusted.

### Two gaps in the literature, stated plainly

**No citable published source was found that states that a safe state must be
reachable from every other state, and none treats fault latching as a named
technique.** Both are real and widely practised. The chapter presents them as
engineering practice with the standards cited for the surrounding requirements,
and never attributes them to a source nobody has read. The same applies to
heartbeat and liveness over the bus, to fail-silent against fail-operational,
and to redundant state variables: real practice, present inside the annexes of
IEC 61508-7 and IEC 62061, **with no free primary source to cite**. Flag it
rather than papering over it.

### State machine implementations

| Project | Licence | Last activity | Note |
|---|---|---|---|
| `igor-krechetov/hsmcpp` | **MIT** | Wednesday 15 April 2026 | C++11, **a FreeRTOS dispatcher**, history, timers, parallel states, SCXML generation, PlantUML export, **MISRA C++ checks in CI**. For a public portfolio, the only actively maintained option with both a kernel dispatcher and coding standard checks |
| Zephyr SMF | **Apache-2.0** | mainline | entry, run and exit actions, hierarchy behind `CONFIG_SMF_ANCESTOR_SUPPORT`, instrumentation hooks. Its documentation mentions safety nowhere. Its sample is described as based on the standard teaching example, which gives a permissively licensed route to that material |
| `kiishor/UML-State-Machine-in-C` | MIT | Tuesday 30 November 2021 | 116 bytes for a flat machine, 424 for a hierarchical one. Dormant five years |
| `amaiorano/hsm` | MIT | not shown | thin, no kernel integration |
| QP/C and QP/C++ | **GPLv3 or paid commercial** | Friday 28 August 2026, v8.1.5 | Active objects plus hierarchical state machines, built for Cortex-M. **Linking the firmware against it makes the whole firmware GPLv3, application code included.** SafeQP, the certified variant, is commercial only |
| `QuantumLeaps/State-Oriented-Programming` | **ambiguous**: Apache-2.0 per the host, MIT per the source comments | 10 commits | resolve before reuse |

The teaching text, *Practical UML Statecharts in C/C++*, second edition,
Newnes, Wednesday 1 October 2008, ISBN 978-0750687065, has its full PDF offered
free by the author, **but no licence terms are stated there**: read it, do not
redistribute it.

### Failure modes and effects analysis

- **IEC 60812:2018, "Failure modes and effects analysis (FMEA and FMECA)",
  Edition 3.0, published Friday 10 August 2018**, CHF 380, ISBN
  978-2-8322-5915-3. It **cancels and replaces** the 2006 second edition and the
  title now covers both. **The free preview carries the complete table of
  contents**, read page by page, so the chapter can say precisely what the
  paywalled body holds: clause 5 as the method, from identifying failure modes to
  identifying actions; **Annex B.4 assigning criticality by risk priority number,
  with B.4.3 giving an alternative method**, so IEC **keeps** the risk priority
  number; **Annex E.2 "Software FMEA" at page 53**, whose figure E.1 is a general
  software failure model for a component software unit, plus E.6 on safety
  related control systems; and worked examples including a blood sugar calculator
  application, an automated train control system with safe and dangerous failures
  tabulated, and **tables F.8, F.9 and F.10 showing one remote control product
  analysed as system, design, process and maintenance analyses side by side**,
  which is the clearest published illustration of those variants found anywhere.
  **Page 2 of the preview carries the no-reproduction notice in full.**
- **MIL-STD-1629A, "Procedures for performing a failure mode, effects, and
  criticality analysis", Monday 24 November 1980, cancelled by Notice 3 on
  Tuesday 4 August 1998**, verified. Cancelled but ancestral, since the space
  agency's current handbook still builds on it, and as a United States government
  work **it is the one source here that can be quoted freely**.
- **NASA-HDBK-2203, the Software Engineering and Assurance Handbook, topic 8.5,
  "SW Failure Modes and Effects Analysis"**, verified. This is the answer to a
  request for contributions to this analysis: an eight step process explicitly
  based on the military standard; a **catalogue of software failure modes** that
  reads as if written for a bus connected joint node (out of range values, missing
  inputs, overwritten memory, data collisions, command omission, incorrect
  sequence, illegal commands, timing issues, and hardware and software interaction
  failures such as broken sensors and stuck valves); criticality categories 1,
  1R, 2, 2R and 3 mapped to safety risk levels 1 to 5; **four downloadable
  worksheet templates**; and a pairing with software fault tree analysis at topic
  8.07 as the top down counterpart. Its companion, **NASA-STD-8739.8, "Software
  Assurance and Software Safety"**, free, has a history of initial release
  Wednesday 28 July 2004, revision 1 Thursday 5 May 2005, **revision A Wednesday
  10 June 2020** (which absorbed and cancelled an earlier standard and moved
  guidance into the handbook) and **revision B Thursday 8 September 2022**. The
  structural point worth printing: the 2020 revision pushed the method out of the
  standard and into the handbook, so the standard sets the requirement and the
  handbook carries the method. Avoid the direct file path for revision B, whose
  cover still reads "This official draft has not been approved". **No copyright
  notice, United States government work: these are the safest artifacts in the
  volume to adapt into a public repository.** Attribute them.
- **The AIAG and VDA FMEA Handbook, 2019, ISBN 978-1-60534-367-9**, product page
  verified, members USD 81 and non-members USD 242. **Honest flag: the page did
  not confirm the seven step structure or the action priority table.** Those are
  consistently reported by independent secondary sources and are very likely
  correct, but the chapter states them as widely reported, or the handbook gets
  bought first. **The contrast is good chapter material: IEC 60812 Annex B.4
  retains the risk priority number and adds an alternative, while the automotive
  handbook reportedly drops it for an action priority lookup.** Two engineers
  working to different sector standards are told opposite things.
- **Open tooling: there is none worth using.** `dromation/open-fmea` (GPL-3.0, 6
  commits on a beta branch) and `Foundliew/openfmea` (GPL-3.0, 7 commits, no
  stars, risk priority number only) are immature; `ovitrac/FMECAengine`
  (CeCILL-B, 85 commits) is MATLAB and specific to mass transfer. **The
  recommendation is the agency's worksheets plus a spreadsheet under version
  control, not a tool.**

## Chapter 19. Update over the bus: a node you can reach but not touch

### The correction that opens the chapter

**The ROM bootloader already in this part speaks CAN-FD.** Verified from
**AN2606, revision 61, January 2024, section 53, Table 115**, read at a
fetchable mirror. The STM32H7A3 system bootloader exposes USART1, 2 and 3, I2C1,
2 and 3, SPI1, 2 and 3, USB DFU, **and FDCAN1 on PH13 for transmit and PH14 for
receive**, at 250 kbit/s nominal and 1,000 kbit/s data, with `FDCAN_FRAME_FD_BRS`
and the FDCAN clock fixed at 20 MHz from PLLQ. **Figure 65 shows FDCAN is polled
first in the detection chain.** So the board can be reflashed over CAN-FD with
no bootloader of the author's own, and that is both the chapter's opening and
the baseline a custom bootloader has to beat. A revision 70 dated February 2026
is **indexed only**: check the revision before printing pin numbers.

Same table, and it applies to any custom bootloader too: in system memory boot
mode "the IWDG prescaler is configured to its maximum value. It is periodically
refreshed to prevent watchdog reset (if the hardware IWDG option was previously
enabled by the user)."

### Flash geometry, which is not the H743's geometry

Verified from ST's headers and corroborated by two closed Zephyr bugs: the flash
word is **128 bits, 16 bytes**, the sector is **8 KB**, with 128 sectors a bank
and two banks. `FLASH_NB_32BITWORD_IN_FLASHWORD` is 4 here and 8 on the H743,
which has 256 bit words and 128 KB sectors. **The comment block in
`stm32h7xx_hal_flash.c` describing a wider word is written for the H743 and is
wrong for this part. Do not quote it.** The two bugs are worth citing by number:
issue 45568, opened Wednesday 11 May 2022, where a hardcoded 32 byte stride
corrupted data on this part; and issue 97593, opened Wednesday 15 October 2025,
on dual bank sector mapping.

**Error correction works in the author's favour.** Each bank exposes `SNECCERR`
and `DBECCERR` in `FLASH_SR`, plus `ECC_FA1` and `ECC_FA2` fail address
registers, with `HAL_FLASHEx_EccCorrectionCallback` and
`HAL_FLASHEx_EccDetectionCallback` under `USE_FLASH_ECC`. **A flash word
interrupted part way through programming reads back as an uncorrectable double
error, so a torn record is detectable rather than silently wrong.** The
corollary: bits inside an already written word cannot be reprogrammed, so a
validity flag must live in a separate word.

### Power loss, verified from the headers

**The brown-out reset has four levels, not the three usually claimed**:
`FLASH_OPTSR` BOR_LEV gives 1.6, 2.1, 2.4 and 2.7 V. **The programmable voltage
detector has seven thresholds** in `PWR_CR1` PLS, from 1.95 to 2.85 V in 0.15 V
steps, **plus an eighth selection that compares an external analog input on
PVD_IN against the reference**, which is the escape hatch when no built-in
threshold suits. The analog supply detector has four: 1.7, 2.1, 2.5 and 2.8 V.
**Correction: the two detectors share EXTI line 16.** Both `PWR_EXTI_LINE_PVD`
and `PWR_EXTI_LINE_AVD` are `EXTI_IMR1_IM16`, so the handler disambiguates by
reading PVDO and AVDO in `PWR_CSR1`. There is also a D2 domain copy of the EXTI
registers, so wakeup can be routed per power domain.

**The backup domain differs from the H743 and will break ported code.** Backup
SRAM is 4 KB at `0x38800000` (`SRD_BKPSRAM_BASE`) in the SRD domain behind an
AXI to AHB bridge. The backup registers are **32 registers, BKP0R to BKP31R,
128 bytes, and on this part they live in the TAMP peripheral, not the RTC**:
`RTC_TypeDef` here ends at CFGR with no backup registers, unlike the H743. Code
ported from an H743 that writes `RTC->BKPxR` will not compile or will not work.
Cross-check the backup SRAM size and retention conditions against RM0455.

**What fits in the holdup window.** One 16 byte word per program operation. Set
the detector below nominal 3.3 V but above the flash programming minimum, take
the falling edge on EXTI line 16, and write. **An erase is never possible in the
power-down path**: an 8 KB sector erase runs into milliseconds, orders of
magnitude beyond a word program. Pre-erase during normal operation and keep a
clean sector ready, so the power-down path only ever programs. Exact program and
erase times are datasheet figures that could not be fetched, and the design
conclusion does not depend on them.

**The endurance arithmetic, parameterised because the endurance figure could not
be confirmed.** One record is one 16 byte word; an 8 KB sector holds 512 records
before an erase. A rotating log survives **512 times** the per sector endurance,
while a naive read, erase and rewrite of one structure survives the endurance
figure alone. At a commonly cited but **unverified** 10,000 cycles that is
5,120,000 events against 10,000, which is roughly 140 years against 100 days at
100 power cycles a day. **That three order of magnitude gap is the entire
argument for a rotating log**, and rotating over several sectors multiplies it
again.

Published guidance, all verified and free. **Ganssle, "Brown-Out Reset, An
Update", Tuesday 3 June 2014**, the most on point: flash programming draws
roughly 10 mA for milliseconds, the supply can sag below minimum during a write,
and reading the supply with the converter is hard while the core is halted during
that write. **Ganssle, "The Perils of NMI", Embedded Systems Programming, April
1991**: the power fail interrupt, saving variables as the supply decays, and
clamping reset below a threshold so decaying logic cannot overwrite what was just
saved. His low power article of December 2014, revised through October 2019,
argues against relying on built-in brown-out detection in ultra low power
designs but **does not discuss flash writes at low voltage**: do not cite it for
that. From Memfault's Interrupt: **"Considerations when Building Embedded
Databases", Chris Merck, Wednesday 24 July 2024**, where the reserved empty page
during compaction comes from; **"A Schematic Review Checklist for Firmware
Engineers", Mark Schulte, Thursday 18 July 2024**, carrying a field account the
chapter reuses, of a connected product made unusable by a brown-out loop in
which the supply sagged during transmission, the device rebooted, and firmware
started a flash write and a transmission at once, thousands of times over; and
**"Device Firmware Update Cookbook", Francois Baldassari, Tuesday 23 June
2020**, whose rule is to write the data first and commit by writing the header
pattern last.

Academic work, all verified through Crossref. **Mementos**: Ransford, Sorber and
Fu, ASPLOS 2011. **Hibernus**: Balsamo and co-authors, IEEE Embedded Systems
Letters volume 7 issue 1, pages 15 to 18, 2015, DOI 10.1109/LES.2014.2371494.
**Alpaca**: Maeng, Colin and Lucia, OOPSLA at SPLASH 2017, presented Friday 27
October 2017. **Correction: Chinchilla is the system name, not the title**; the
paper is **"Adaptive Dynamic Checkpointing for Safe Efficient Intermittent
Computing", Maeng and Lucia, USENIX OSDI 2018, pages 129 to 144**. Three more
worth having: **TFFS, "A Transactional Flash File System for Microcontrollers",
Gal and Toledo, USENIX ATC 2005**, the most directly relevant, giving atomicity
over arbitrary operation sequences on NOR flash in a few hundred bytes of RAM;
Umesh and Mittal, "A survey of techniques for intermittent computing", Journal
of Systems Architecture volume 112 article 101859, 2021; and **Ratchet**, Van
Der Woude and Hicks, USENIX OSDI 2016, pages 17 to 32. **One editorial caution
to print: that literature targets energy harvesting devices that lose power
constantly. A joint node loses power rarely. Borrow the techniques, not the cost
model.**

### Non-volatile configuration

| Library | Licence | Latest | Note |
|---|---|---|---|
| littlefs | BSD-3-Clause | v2.11.3, Wednesday 25 March 2026 | power loss resilient, dynamic wear levelling, copy on write metadata, bounded RAM. **Not a key-value API**: keys are paths and each value costs directory plus file metadata. **No published footprint** |
| FlashDB | Apache-2.0 | 2.2.0, Monday 23 March 2026 | key-value plus a time series store on raw flash or a file, multiple partitions. **The only entry with published per object footprints**, from its own README |
| EasyFlash | MIT | 4.1.0, Sunday 12 April 2020 | ROM about 6 KB, RAM about 0.1 KB, but its own README points to FlashDB as successor. Superseded |
| Pigweed `pw_kvs` | Apache-2.0 | consumed at head | log structured, append only, integrated wear levelling, redundancy with fallback on a corrupted read, garbage collection, size and redundancy changeable after shipping, about 1.0 kB of metadata RAM for 50 entries with redundancy 2. **C++ only, no C API**, and it needs the Pigweed build system. The repository now redirects to a new organisation |
| Zephyr NVS | Apache-2.0 | 4.4.2, Friday 7 August 2026 | numeric identifier to data in a circular buffer. No string keys, no namespaces, at least two sectors, power of two sector size, and **the CRC is not checked on a partial read** |
| Zephyr ZMS | Apache-2.0 | since 4.0.0 | also covers memories that overwrite without erase, 64 bit addressing, CRC32, optional cache at 8 bytes per entry. **No migration tool from NVS** |
| SPIFFS | MIT | 0.3.7, Monday 17 July 2017 | dormant |
| ESP-IDF NVS | Apache-2.0 | v5.3.6, Wednesday 16 September 2026 | **ESP32 only, cannot run here.** Include only as a design comparison; its published costs are instructive, about 22 KB RAM per 1 MB partition and about 5.5 KB per 1,000 keys, and its docs say plainly it is unsuitable for event logs or frequently rewritten data |
| Mbed KVStore and TDBStore | Apache-2.0 | 6.17.0, Tuesday 8 October 2024 | **Mbed OS reached end of life in July 2026 and its documentation estate redirects away. Do not cite an `os.mbed.com` URL.** Historical reference only |

**The ST situation, settled by a dedicated verification pass.**

- **AN3969, "EEPROM emulation in STM32F40x/STM32F41x microcontrollers", Doc ID
  022108 Rev 1, October 2011**, confirmed by reading the PDF cover. Document
  identifier DM00036065, firmware STSW-STM32066.
- **AN4894 is confirmed but was renamed.** Its current title is **"How to use
  EEPROM emulation on STM32 MCUs", Rev 11, Tuesday 18 March 2025**; earlier
  revisions were titled "EEPROM emulation techniques and software for STM32
  microcontrollers", and the revision history of Rev 11 records the rename.
  Document identifier DM00311483. **Cite the current title, and label the older
  one as the pre-rename title if it is used.**
- **AN4767 is a real note about a different subject: "On-the-fly firmware update
  for dual bank STM32 microcontrollers", Rev 3, May 2019**, document DM00230416,
  covering STM32L0 Cat.5, STM32L4 and STM32G4 Cat.3. It is not an emulation
  note. It cites AN4894, which is probably where the confusion started. **The
  Chinese mirror's translated title is a paraphrase, not ST's English title.**
- **X-CUBE-EEPROM exists, and it does not cover this family.** Read through the
  Internet Archive. The families covered, from the archived page and AN4894
  Rev 11 Table 1, are STM32C0, G0, G4, **H5**, L4, L4+, L5, U0, U3, U5, WB and
  WL. **The string "H7" does not occur anywhere in AN4894 Rev 11.** The only "H"
  part is the H5, which uses a high cycle data area this part does not have.
  Dual bank mode is an explicit driver option on L4, L4+, G4, U3, U5 and H5, and
  AN4894 recommends putting critical routines in one bank and the emulation area
  in the other, **but that is L4 and G4 and U5 and H5 class flash, not H7
  flash**. Versions verified up to V4.0.0 of Monday 29 November 2021; later ones
  clearly exist. **There is no ST GitHub mirror of this package**, and the
  third party uploads that exist are not an ST distribution channel.
- **`STMicroelectronics/X-CUBE-EEPRMA1` is a different product**, drivers for
  external M24xx and M95xx EEPROM chips, not internal flash emulation. The names
  differ by one letter group; do not confuse them.
- **`STMicroelectronics/stm32-util-eeprom-emulation` is BSD-3-Clause end to
  end**, verified from `LICENSE.md`, created Friday 23 January 2026, v2.0.0
  Friday 13 March 2026, v2.1.0 Friday 12 June 2026. FLITF and NVM algorithms,
  power loss recovery, CRC for corruption detection and BCH error correction,
  and pluggable flash, CRC and ECC driver interfaces with templates. **Its
  release notes list the STM32C5 series only.** It is family agnostic by design,
  so the flash driver for this part is the author's work. **A chapter, not a
  blocker.**
- **A licence hazard worth printing because it is real, not a misreading.** The
  X-CUBE-EEPROM V3.0.0 release notes say the package is licensed under a three
  clause permissive licence and point at ST's SLA0048 URL; the individual C
  files in the same package carry headers pointing at the canonical permissive
  text; **and the same archive ships a PDF titled "Software License Agreement
  (Liberty V2)", dated Wednesday 16 November 2011, which says the licensee "may
  not sell, assign, sublicense, lease, rent or otherwise distribute" the
  software commercially.** Three statements, one package.

**Wear levelling on this part.** Page swapping is ST's classic active-page plus
receive-page scheme. Rotating records append fixed size entries until the sector
fills. Log structured storage appends and garbage collects into a pre-erased
sector. **On the H7A3 the rotating record approach is unusually cheap: 512
records of 16 bytes per 8 KB sector, because the erase granularity is sixteen
times smaller than on the H743.**

### The update protocol

Object indices verified from **MicroControl AN1211, "CANopen Bootloader
Integration", Rev. 01**, read in full as a PDF, and corroborated by **CAN in
Automation's own iCC 2012 proceedings paper, Christian Keydel, "Security aspects
in CANopen bootloaders"**, also read in full:

| Index | Verified name |
|---|---|
| **1F50h** | **Program data** (not "download program data") |
| 1F51h | Program control, sub-values 00h stop, 01h start, 02h reset, 03h clear |
| 1F56h | Program software identification |
| 1F57h | Flash status identification |

Specifications, verified from both documents' reference lists: **CiA 301
V4.2.0**, **CiA 302-3 "CANopen additional application layer functions, Part 3:
Configuration and program download" (DSP V4.1.0)**, and **CiA 305 "Layer setting
services (LSS) and protocols" v3.0.0**. A third party confirmation of the part
number comes from a Kconfig help text: `CONFIG_CANOPEN_PROGRAM_DOWNLOAD` reads
"according to the CiA 302-3 (draft) specification". **AN1211's body text says
"CiA 302-2" once while its own reference list says 302-3: treat the body mention
as a typographical error.** Licence position, verified at CiA: PAS and TR
documents are free after registration, **DS and DSP are members only, and the
CiA 302 parts are DSP**. CiA 301 is PAS.

**A real disagreement worth showing the reader.** AN1211's captured trace moves
the image by **SDO block download** ("Init block download", "Write data, block
1..45", "Block download end"). The CiA conference paper argues **against** block
transfer in a bootloader: it needs back to back frame buffering in a polled
driver, adds RAM and significant ROM, and segmented access must be implemented
anyway, so "SDO segmented transfer should be the preferred choice for the DOMAIN
entries". Two credible sources, opposite conclusions, in the same problem space.

**The automotive alternative**, verified from ISO's own previews.
**ISO 14229-1:2020, "Road vehicles, Unified diagnostic services (UDS), Part 1:
Application layer", third edition, February 2020**: clause headings confirmed
verbatim, 14.2 RoutineControl (31h), clause 15 "Upload download functional unit"
with 15.2 RequestDownload (34h), 15.3 RequestUpload (35h), 15.4 TransferData
(36h), 15.5 RequestTransferExit (37h), 15.6 RequestFileTransfer (38h), and
**clause 17 "Non-volatile server memory programming process"** with 17.2.1 on
downloading application software and data and 17.3.2 on software, data
identification and fingerprints. **That clause is the model a fleet master
follows.** Its transport, ISO 15765-2:2016, is quoted under chapter 11. Both
paywalled with ISO copyright notices on the cover.

**Open transport implementations**, all verified: `lishen2/isotp-c` (MIT, last
push Thursday 11 April 2024, 287 stars); `openxc/isotp-c` (BSD-3-Clause, Monday
16 August 2021, 362 stars, the original, dormant); `driftregion/isotp-c` (MIT, a
rewrite of that lineage); `SimonCahill/isotp-c` (MIT, last push Sunday 20
September 2026, **the most active of the family**); `fzxhub/can_tp` (**no licence
stated**, a hard blocker); and `devcoons/iso15765-canbus` (**AGPL-3.0**, 199
stars, last push Monday 22 June 2026, the most problematic licence in the pool:
name it, never vendor it). None of the permissive ones claims CAN FD in its
README.

**The kernel's own `can-isotp` is better than all of them for the host side**,
verified, with the quotable line: "ISO-TP can be used both on CAN CC (aka
Classical CAN) and CAN FD (CAN with Flexible Datarate) based networks." Three
socket option groups: `CAN_ISOTP_OPTS`, `CAN_ISOTP_RECV_FC` (block size, minimum
separation time, maximum wait frames), and **`CAN_ISOTP_LL_OPTS` (MTU,
transmission payload length, CAN FD flags)**. That last group is what lets the
master drive a 64 byte link. **The claim that it entered mainline at 5.10 could
not be confirmed**: say "in mainline Linux" without a version.

**Stacks.** `CANopenNode`, Apache-2.0, 2,008 stars, last push Friday 10 July
2026: its README lists CiA 301, 303, 304, 305 and 309, **and CiA 302-3 is
absent**; its `301/CO_ODinterface.h` enumeration stops at 0x1A00, so **1F50h and
1F51h are not defined in the stack**. It does carry LSS master and slave with
fast scan. `CanOpenSTM32` (MIT, verified from its licence file although the host
metadata says otherwise, 524 stars, last push Wednesday 9 September 2026) has
the same gap. `canopen-python/canopen` (MIT, 567 stars) has NMT master, SDO
client and PDO but **no program download, no CiA 302 and no bootloader
mention**, so the download sequence would be written on top of its SDO client.
**Lely core** (Apache-2.0) is **the only stack claiming CiA 302 coverage**,
though it says "portions of" and does not itemise part 3.

**UDS and diagnostics on the host**: `pylessard/python-udsoncan` (MIT, 731
stars, last push Monday 21 September 2026) is the natural partner to the kernel
transport; `openxc/uds-c` (BSD-3-Clause, 828 stars) is permissive but dormant
five years; `mercedes-benz/odxtools` (MIT, 315 stars) handles diagnostic
descriptions.

**Bootloaders.**

- **OpenBLT**, `feaser/openblt`, 973 stars, mirror last pushed Saturday 19
  September 2026. Its homepage lists **RS232, CAN, CAN FD, TCP/IP, USB, Modbus
  RTU** and SD card, and it works on STM32. **The closest off-the-shelf match to
  this chapter, and GPLv3 or paid commercial: linking the joint node firmware
  against it makes the firmware GPLv3.** The most consequential single licence
  decision in the volume, and the chapter states it before a reader writes code
  around it. Its GitHub presence is a read-only daily mirror of SVN.
- **MCUboot**, `mcu-tools/mcuboot`, **Apache-2.0**, 2,122 stars, last push
  Friday 18 September 2026. Verified **port agnostic** from its own porting
  guide: a port supplies a flash map, the `flash_area_*` API and a
  `mcuboot_config.h`, and the common code "does not directly access contents of
  that object and never modifies it". It does not require Zephyr. Primary and
  secondary slots, three upgrade strategies (swap using scratch, swap using
  offset, overwrite), a 32 byte header with magic `0x96f3b83d` and version
  fields, and a trailer whose 16 byte magic encodes the maximum supported write
  alignment. **On whether it runs here, answered carefully:** current
  `bootutil_public.h` carries
  `_Static_assert(MCUBOOT_BOOT_MAX_ALIGN >= 8 && MCUBOOT_BOOT_MAX_ALIGN <= 32, ...)`,
  which **covers this part's 16 byte word**. The older claim that it cannot run
  on STM32H7 because the minimum write exceeds eight bytes **could not be
  verified from any fetchable primary source and is contradicted by the current
  source**. **No published footprint figures were found, so quote none**, and
  support on this exact board is unconfirmed: present it as portable in
  principle and as a bring-up exercise.
- **Neither MCUboot nor Zephyr's mcumgr and SMP has any CAN transport.** SMP's
  transports are verified as Bluetooth LE, UART with console framing, and raw
  UART only. **The bus half is the author's to write, and that is precisely the
  gap this chapter fills.**
- **wolfBoot**, **GPL-3.0** plus commercial, 537 stars, last push Thursday 17
  September 2026, with commercial licences quoted at USD 7,500 per end product.
  **Correction: it is GPL-3.0, not GPLv2.**
- `akospasztor/stm32-bootloader`, 1,061 stars, last push Saturday 12 September
  2026: **licence could not be confirmed**, the host reports no assertion and the
  licence path 404s. Its demonstration is an SD card and FAT32 loader on an
  STM32L496AG; CAN is not mentioned and this family is not listed. A readable
  reference, not a bus loader.
- **Memfault, "From Zero to main(): How to Write a Bootloader from Scratch",
  Francois Baldassari, Tuesday 13 August 2019**, free to read: extracting the
  stack pointer and program counter from the application vector table, the `msr`
  and `bx` instructions, reboot loop detection and memory relocation.
  **Verified gap: it does not cover image validation, signature verification or
  checksums.** Pair it with the CiA paper for that half.
- **X-CUBE-SBSFU: nothing confirmed.** The ST page timed out on every attempt
  and `STMicroelectronics/x-cube-sbsfu` returns 404. **Do not assert STM32H7
  support for it.**

**Requirements a bus bootloader must meet, from the two primary documents read
in full.** These are requirements those documents state, not requirements
invented here.

The bootloader is itself a complete CANopen application: it needs "a basic NMT
state machine and heartbeat generator" plus a small object dictionary, and "for
all but one" entry expedited SDO suffices; it sends its own boot-up message, and
AN1211's trace shows it answering on 581h to requests on 601h. **It always
executes first**, so the reset vector points at it at all times; on parts with
fixed vector locations the paper recommends vector mirroring, and on this part,
where VTOR is programmable, the bootloader sets the application's vector base
before jumping instead. Node addressing: AN1211's tool takes the node identifier
as a mandatory argument and defaults the master to 127, while **CiA 305 layer
setting services** are the standard route for an unconfigured node and
CANopenNode implements both sides with fast scan; **AN1211's own bootloader
implements no such server**, so this is a design choice rather than a given. The
bit rate is fixed or at most a small tested set, and **ST's ROM bootloader fixes
250 kbit/s and 1 Mbit/s, so a custom bootloader differs from it unless matched
deliberately**. **No interrupts, use polling**: vector mirroring makes hardware
vectors unusable unless every vector routes through a decider, interrupts remove
determinism, and most parts cannot execute handlers during flash programming
anyway, with hardware filtering admitting only two 11 bit identifiers, the SDO
request at 600h plus node identifier and NMT at 000h. **For a FreeRTOS
application that is a genuine architectural break to explain: the bootloader is
a polled superloop, not a task set.** After a plain reset with no keyword
pattern, the bootloader waits a bounded period for specific incoming traffic,
for example an SDO read of its own node identifier, and otherwise starts the
application: always reachable after power up, at the cost of always delaying
application start. It **always computes the application checksum** and never
executes on a mismatch, with a table based CRC recommended and a simpler
checksum acceptable where code space is tight. A **keyword pattern at a RAM
location excluded from startup initialisation** carries the handoff in both
directions, and the reason belongs in the book: a direct jump leaves peripherals
initialised by the bootloader, and applications developed and tested without a
bootloader present "can fail for subtle reasons that can be very difficult to
track down", often surfacing only in the field. Forced entry by writing 00h to
`[1F51h, 1]` is the standard way in, and **AN1211 cautions that implementing
1F51h inside the application would let any tool on the bus stop the application
and says it is not recommended for security reasons**, recommending a physical
input pin for an application that passes its checksum but does not communicate.
Every received block is checked to lie wholly inside the application flash
boundaries, with out of range writes rejected and an SDO abort generated, and
the bootloader's own area is erase and program protected where the hardware
allows. Errors are reported through **object 1003h**, the predefined error
field, with a CiA 301 code in the low 16 bits and free information in the upper
16, and the paper's suggested assignment is 6100h bootloader checksum wrong,
6200h application checksum wrong, 6300h EEPROM checksum wrong.

**One certification note worth printing**: the CiA paper's author certified
bootloader code to a sector scheme requiring MISRA C conformance, and records
that the 1998 rules against casting to and from pointers and against `continue`
and `break` made generic table driven object dictionary implementations
impractical, forcing a fresh coding effort.

### Versioning and fleet inventory

**Semantic Versioning 2.0.0** is **CC BY 3.0** and freely quotable. **RFC 9019,
"A Firmware Update Architecture for Internet of Things", Informational, April
2021**, and **RFC 9124, "A Manifest Information Model for Firmware Updates in
Internet of Things (IoT) Devices", Informational, January 2022**, both verified
at `rfc-editor.org`, both free to redistribute, **which makes them the safest
normative references in the whole volume**. Both titles differ from the ones in
circulation.

The verified fleet sweep: for each node identifier, read **1018h** sub 1 to 4
for vendor, product code, revision and serial; **1F56h** for the version; and
**1F57h** for flash state. AN1211's trace also shows the master reading **1000h**
device type and **1008h** manufacturer device name, where the value "Boot"
identifies bootloader mode.

## Chapter 20. The rig: injected faults, tracking error, and a build that fails

### The simulator, which is the most useful finding in this chapter

**Renode models this microcontroller family including the bus controller**, per
the platform file quoted under chapter 01. Its host integration provides a
**SocketCAN bridge** (`CreateSocketCANBridge`, defaulting to `vcan0`) carrying
classic and FD frames between the simulation and a host virtual interface,
**Linux host only**. Its test runner, `renode-test my_test.robot`, starts Renode
headless, opens a Robot Framework server on port 9999, emits HTML and XML
reports, runs in parallel with `-j`, and snapshots failures under
`RENODE_CI_MODE`. Related and useful: Antmicro's post of Thursday 16 July 2026
on the STM32H753 reference platform, and `antmicro/stm32h7-renode-reference-platform`
(Apache-2.0, KiCad only, two CAN terminals on the board).

What it does not give: **no motion metrics of any kind**, and no fault injection
section in its documentation index.

**QEMU is the wrong tool and the chapter says so.** Its ARM system target page
names M-profile support for Cortex-M0, M4 and M33 and four STM32 boards, all F1,
F2 and F4 class. **No STM32H7 machine, and CAN is not mentioned on the page at
all.**

### The harness

| Tool | Licence | Role |
|---|---|---|
| Robot Framework | Apache-2.0 | the test language Renode speaks natively. Structure and reporting, not measurement |
| Labgrid | **LGPL-2.1**, 528 stars, last push Friday 11 September 2026 | serial and SSH drivers, power switch and reset drivers, USB bootstrap, SD and USB multiplexers, a remote layer. The right layer for power cycle, flash, open console. Gives nothing about trajectories |
| pytest with python-can and pyserial | mixed, python-can **LGPL-3.0** | **the honest recommendation**, and more defensible than `pytest-embedded` (MIT), whose value is concentrated in services for another vendor's silicon and most of which would be reimplemented on an ST target |
| Ceedling with Unity and CMock | MIT, v1.1.9, last push Sunday 20 September 2026 | the unit half: the trajectory interpolator, the controller step, the frame packer |
| `meekrosoft/fff` | MIT, 938 stars | header only fakes. `SET_RETURN_SEQ` and `custom_fake` make a driver call fail on the third invocation, which is unit level fault injection |

**`ros2_control`'s `mock_components/GenericSystem` is the closest published
analogue** to the whole rig, and is described under chapter 01.

### What to assert, and where the names come from

**ISO 9283:1998, "Manipulating industrial robots, Performance criteria and
related test methods", second edition, published Thursday 23 April 1998** (the
British adoption is dated August 1998). **The title and number are correct as
usually given.** Its characteristics map almost one to one onto this chapter's
assertions: pose accuracy and repeatability, **position stabilisation time**,
**position overshoot**, **path accuracy and path repeatability**, cornering
deviations, path velocity characteristics, and drift of pose characteristics. A
verified practical description gives the test cube as the largest fitting the
workspace, with accuracy and repeatability measured at five configurations over
**30 cycles**, and its own caveat that five configurations is thin and
manufacturers commonly use a hundred or more. **The metric symbols could not be
confirmed from a fetchable page: do not print them as verified.** Paywalled,
roughly EUR 245.

**ISO 230-4:2022, "Test code for machine tools, Part 4: Circular tests for
numerically controlled machine tools", published Tuesday 22 February 2022**,
superseding the 2005 and 1996 editions. Specifies bi-directional circular error,
mean bi-directional radial error, and circular and radial error of paths from
two simultaneous linear axes. The free companion is **Renishaw's "Ballbar
testing explained"**, verified, which gives circular deviation as a single
overall indicator and makes the point the chapter needs: **large radius circles
expose geometry errors, small radius circles are more sensitive to servo
mismatch or lag.** That page does not cite the standard by number and does not
enumerate backlash or reversal signatures: do not attribute those to it.

**The defensible assertion set**: maximum absolute following error, root mean
square tracking error, settling time, overshoot, steady state error, and control
loop period jitter. ISO 9283 gives published names for stabilisation time,
overshoot and path accuracy. **Loop period jitter has no standards backing in
either document** and is presented as an engineering assertion.

Academic, all verified through Crossref: **Isermann, Schaffnit and Sinsel,
"Hardware-in-the-loop simulation for the design and testing of engine-control
systems", Control Engineering Practice volume 7, pages 643 to 653, 1999, DOI
10.1016/s0967-0661(98)00205-6**, the canonical citation; **Bacic, "On
Hardware-in-the-Loop Simulation", Proceedings of the 44th IEEE Conference on
Decision and Control, pages 3194 to 3198, DOI 10.1109/cdc.2005.1582653**;
**Koren 1980**, cited under chapter 16, for the tracking against contouring
distinction; and **Matinnejad, Nejati, Briand and Bruckmann, "Test Generation
and Test Prioritization for Simulink Models with Dynamic Behavior", IEEE
Transactions on Software Engineering volume 45, pages 919 to 944, 2019, DOI
10.1109/tse.2018.2811489**, with the earlier **"Automated test suite generation
for time-continuous Simulink models", ICSE 2016, pages 595 to 606, DOI
10.1145/2884781.2884797**, which are the closest published work to failing a
build on a regression in continuous output quality rather than in code, and
which reason explicitly over time-continuous signals rather than pass or fail
assertions.

**A gap this chapter can claim: no published, motion-specific example was found
that runs a rig in continuous integration and asserts on tracking quality.** The
practitioner articles that surfaced are generic.

### Fault injection

Academic foundations, all verified through Crossref: **Arlat and co-authors,
"Fault injection for dependability validation: a methodology and some
applications", IEEE Transactions on Software Engineering volume 16, pages 166 to
182, 1990, DOI 10.1109/32.44380**, with the 1989 conference paper at FTCS-19;
**Carreira, Madeira and Silva, "Xception", volume 24, pages 125 to 136, 1998,
DOI 10.1109/32.666826**; **Hsueh, Tsai and Iyer, "Fault injection techniques and
tools", Computer volume 30, pages 75 to 82, 1997, DOI 10.1109/2.585157, the
classic survey, cite this first**; **Natella, Cotroneo and Madeira, "Assessing
Dependability with Software Fault Injection", ACM Computing Surveys volume 48,
2016, DOI 10.1145/2841425**, the modern survey; **Natella and co-authors, "On
Fault Representativeness of Software Fault Injection", volume 39, pages 80 to
96, 2013, DOI 10.1109/tse.2011.124**, directly relevant to this book's honesty
problem, namely whether the faults injected are the faults that occur;
**Aidemark and co-authors, "GOOFI", DSN 2001, pages 83 to 88**, with the same
group's brake-by-wire evaluation, DSN 2002, pages 210 to 215, which is motion
adjacent; and **Rodriguez and co-authors, "MAFALDA", EDCC-3, LNCS, pages 143 to
160, 1999**.

Tooling, verified: **`danceos/fail` (FAIL\*)**, **GPL-3.0**, last commit
Wednesday 15 December 2021, dormant, 42 stars, with backends for two simulators
and for OpenOCD on real hardware, injecting at instruction level, memory
accesses, interrupts and timer events. **`can-utils` is the practical toolkit**,
with `cangen` and `canplayer` as described under chapter 10, and the
**interface remapping** in `canplayer` is the mechanism for replaying a captured
real bus trace onto a virtual bus in continuous integration. **Caring Caribou**,
`CaringCaribou/caringcaribou`, **GPL-3.0**, last commit Friday 12 June 2026, 950
stars, with `uds`, `xcp`, `fuzzer`, `listener`, `send`, `dump`, `uds_fuzz` and
`doip` modules and a fuzzer doing random generation, bit-mask brute force,
mutation and log replay: **the most maintained CAN fuzzer verified**.
**CANToolz** is dormant since Wednesday 24 February 2021 and **its licence type
could not be confirmed**. **Correction: `ericevenchick/CANard` redirects to
`pyvit`, which was archived by its owner on Friday 7 May 2021**; no statement in
the repository says python-can superseded it, so cite python-can and mention the
older project historically.

**Zephyr's ztest integrates `fff` and ships fake drivers configured through the
device tree and Kconfig, including a fake CAN driver**, plus fake EEPROM, LEDs,
PWM, regulators, RTC and stepper. **No dedicated Zephyr fault injection
subsystem was found**: report it as mocking yes, named framework no. **Fault
injection in Renode could not be confirmed either.**

**ISO 26262-6:2018, "Road vehicles, Functional safety, Part 6: Product
development at the software level", published Monday 17 December 2018**, about
EUR 245, and **ISO 26262-11:2018, "Guidelines on application of ISO 26262 to
semiconductors"**, same date, informative in character: both part numbers and
titles confirmed through resellers. **The specific table number could not be
confirmed and sources disagree between Table 7 and Table 10, so print no table
number.** Safe to say, attributed to secondary sources rather than to the
standard: fault injection testing appears among the methods for software unit
verification, recommended at ASIL A and B and highly recommended at ASIL C and
D, alongside requirements based testing, interface testing and resource usage
testing.

### Observability with no probe

This bench has no oscilloscope, no logic analyser and no debug probe, so the
topic has to be answered from inside the part.

The free published guidance is Memfault's Interrupt, all verified and free to
read: **"How to debug a HardFault on an ARM Cortex-M MCU", Chris Coleman,
Wednesday 20 November 2019**, giving CFSR at `0xE000ED28` and HFSR at
`0xE000ED2C`, stack frame and register recovery at exception entry, six worked
scenarios, MPU faults, imprecise bus errors, and recovery from some faults
without rebooting, which is what the fault handler section is built on; **"A
Guide to Watchdog Timers for Embedded Systems", Chris Coleman, Tuesday 18
February 2020**, hardware and task level watchdogs and reading reset reason
registers, which **does not** develop the uninitialised RAM technique in depth;
and **"Monitoring Fleet Health with Heartbeat Metrics", Tyler Hoffman,
Wednesday 2 September 2020**, counters, timed counters and gauges reset each
interval, with the argument that pre-aggregated periodic statistics beat raw logs
on a constrained device, which is **the intellectual basis for the periodic
diagnostic frame**. **Correction: four slugs in circulation return 404** and are
not cited: `reboot-reason-tracking`, `cortex-m-watchdog-timers`,
`device-firmware-observability`, and the `/tags/mcu/` page.

**The licence trap, stated plainly in the chapter**, is the
`memfault-firmware-sdk` position described in the licence categories above. The
linker technique itself is documented openly in Memfault's own docs (collect
named sections bracketed by start and end symbols), and placement in a `.noinit`
region is **indexed only**.

**The licence clean answers**, all verified:

- **Zephyr coredump**, **Apache-2.0**. Backends: logging to a serial console,
  **UDP logging to a remote collector**, a flash partition, or null. Capture
  scopes: minimal (exception thread stack and thread struct), all threads, or
  linker RAM, the default, spanning `_image_ram_start[]` to `_image_ram_end[]`
  **including data, noinit and BSS**. Host tools include a serial log parser, a
  UDP receiver, and **`coredump_gdbserver.py`, a GDB server that serves the
  captured dump**, which is the strongest verified answer to postmortem analysis
  with no probe.
- **`armink/CmBacktrace`**, **MIT**, 2,176 stars, last push Thursday 21 May
  2026. Detects asserts, hard faults, MemManage, bus and usage faults on
  Cortex-M0, M3, M4 and **M7**, across IAR, Keil and GCC, prints a diagnosed
  cause and a call stack, needs **no debugger**, sends output to a console, and
  can persist to flash through EasyFlash so the trace survives a restart, with
  addresses resolved offline using `addr2line`. **The single best licence clean
  fit for this bench.**
- **`armink/EasyLogger`**, **MIT**, 4.8k stars, ROM under 1.6 K and RAM under
  0.3 K, thread safe, asynchronous and buffered modes, tag, level and keyword
  filtering, hexdump, and a flash backend with **no file system required**.
- **`percepio/TraceRecorderSource`**, **Apache-2.0**, 82 stars, last commit
  Wednesday 17 December 2025, release v4.11.0. **The target side recorder
  library is genuinely permissive and safe to vendor**: task execution, ISR
  activity, event timing, state changes, heap and stack monitoring, state
  machine tracking. **Tracealyzer, the host visualiser, is commercial.** Present
  it as an open recorder with a commercial viewer. FreeRTOS+Trace is the older
  name for the same lineage.
- **A licence nuance that contradicts the usual assumption**: SEGGER's RTT
  recorder source, fetched from the Zephyr module mirror, is **effectively a one
  clause BSD**, so the target side source is redistributable. The restriction
  lives in the use licence: a free Friendly License for non-commercial,
  hobbyist, student and educational use, a Commercial-use License otherwise, and
  the statement that "Source code redistribution is not permitted under standard
  licenses". **But the blocker here is hardware, not licence: SEGGER's own
  knowledge base confirms RTT works "despite a J-Link connected via the standard
  debug port to the target", so RTT needs a J-Link**, which this bench does not
  have. The escape hatch, reflashing the on-board ST-LINK with J-Link firmware,
  is a probe by another name.
- **ITM and SWO** are out for the same reason, per chapter 02. The Arm
  architecture reference manual could not be fetched, so this is stated as
  engineering fact rather than cited to a document nobody opened.

**The probe-free stack this chapter actually builds**: CmBacktrace for fault
diagnosis, EasyLogger for the log, a noinit RAM region with a magic number and a
CRC for the trace buffer, the heartbeat metrics pattern for counters, and a
CANopen EMCY plus a J1939 DM1 shaped periodic frame as the transport. **That
answers the topic without a single probe and without a single problematic
licence.**

**AUTOSAR DEM and DET: not verified at all.** Omit or research separately.

---

## Roadmap and next steps: the sourced pool

Every chapter's closing section draws from here and never invents.

**After chapters 02 and 03.** The 2021 clock synchronisation paper's related work
section gives the next two steps in order: correct the tick rate, not only the
offset; and characterise the transceiver and the interrupt behaviour rather than
assuming them. The kernel's delay-until return value is the free first step
toward overrun accounting. Koopman's lecture index names the follow-on decks,
and his current writing has moved to a subscription newsletter (**indexed
only**) while his older blog is live but archival, last posted Wednesday 1 May
2024.

**After the bus chapters, 09 to 13.** The CiA conference proceedings are the
published progression for this subject and are freely downloadable. ISO 15765-2's
FD clauses point at the next capability, a payload larger than one frame. The
SocketCAN documentation names the next tools in its own text.

**After the middleware chapters, 14 to 17.** The `rclc` bibliography above is the
reading list, and the response time analysis paper is CC BY 3.0 so it can be
quoted. `ros2_canopen`'s CiA 402 driver is the next concrete build: implement the
device side of the profile the driver already speaks. For the fieldbus, ETG.2200
section 4.3 is the published route from nothing to a working device, including
its own six to eight week estimate, and the ESC overview is the shopping list.

**After the safety chapter, 18.** The free industrial functional safety document
names the mechanisms in a firmware engineer's own order, and SISTEMA is the next
thing to run once a structure exists. The taxonomy paper's degraded regime is the
next design step beyond a binary safe state. IEC 61784-3 and the black channel
idea is the next thing to read, and to verify first.

**After the update chapter, 19.** RFC 9019 and RFC 9124 are the architecture and
manifest progression, free to redistribute, and are what a reader should read
next. MCUboot's porting guide is the concrete next build. The CiA paper's
security section is the next concern after the mechanism works.

**After the rig chapter, 20.** Renode's test runner documentation, ISO 9283's
characteristics list, and the software fault injection survey are the three
published directions: more of the bench in software, more of the assertions from
a standard, and a better answer to whether the injected faults are
representative.

---

## Corrections carried into the manuscript

Claims in wide circulation that these sweeps found to be wrong. Each is either
printed as a correction in its chapter or quietly avoided.

1. **The part trap does not apply to the FDCAN peripheral**: same M_CAN core,
   same addresses, same bit timing fields. Three real differences, named in
   `AUTHORING.md` and chapter 09.
2. **The STM32H7A3ZI has no Ethernet MAC**, proven from interrupt vector slots
   61 and 62 being absent where the H743 has them.
3. **Zephyr's `stm32h7a3.dtsi` leaves a `&mac` node in place** as an inherited
   modelling artefact. Grepping upstream gives the opposite answer.
4. **This part's flash geometry is not the H743's**, and the comment block in
   ST's own flash driver describing a wider word is written for the H743.
5. **PVD and AVD share EXTI line 16.**
6. **The backup registers live in TAMP on this part, not in the RTC.**
7. **The brown-out reset has four levels, not three.**
8. **The ROM bootloader already speaks FDCAN**, on PH13 and PH14, and is polled
   first.
9. **The STM32H7 has no hardware encoder index**, on ST's own statement; the
   G4 was the first, and the type is `TIMEx_EncoderIndexConfigTypeDef`.
10. **The AS5600 has no quadrature output and no SPI**, so it cannot feed the
    timer encoder peripheral.
11. **X-CUBE-EEPROM exists but does not cover STM32H7**, AN4894 was renamed to
    "How to use EEPROM emulation on STM32 MCUs", and **AN4767 is about dual bank
    firmware update, not emulation**.
12. **X-CUBE-MCSDK is SLA0048, not BSD-3-Clause.**
13. **ST says SIL 3 needs two MCUs in a 1oo2 scheme**, not 1oo1.
14. **SAFERTOS is not "certified FreeRTOS"**; it is a separate product sharing a
    functional foundation.
15. **Both firmware update RFC titles in circulation are wrong**; the verified
    forms are above.
16. **wolfBoot is GPL-3.0, not GPLv2.**
17. **`github.com/jakeluo/isotp` does not exist**, returning 404 from the API.
18. **SOEM moved to GPLv3 on Friday 11 July 2025**, toward stronger copyleft,
    not away from it.
19. **There is no SAMA5 with an integrated EtherCAT controller**; the integrated
    part is the LAN9255 with a SAM E53.
20. **The ISO 10218 series is now titled "Robotics"**, and **ISO/TS 15066 is not
    withdrawn**, contrary to several secondary sources.
21. **IEC 62061:2021 lost its restriction to electrical technologies rather than
    gaining coverage**, and both it and IEC 60204-1:2016 carry amendments.
22. **MISRA C:2025 is current**; "MISRA C:2012 with amendments" is the pre-2023
    form.
23. **IEEE 802.1AS-2020 is superseded by the 2025 edition**, and **there is no
    CiA 1301**: the timestamping document is CiA 603 v1.1.0 of Friday 1
    September 2023.
24. **The download object is named "Program data"**, not "download program
    data".
25. **The claim that MCUboot cannot run on this family because of write block
    size is contradicted by its current source.**
26. **`br3ttb/Arduino-PID-Library` is MIT only by a sentence in `README.txt`**,
    so scanners report no licence; its `README.md` 404s.
27. **`tttapa/Arduino-Filters` contains no PID controller.**
28. **`CANard` is now `pyvit` and was archived on Friday 7 May 2021**; the claim
    that a particular project superseded it could not be confirmed.
29. **Four Interrupt article slugs and every `os.mbed.com` documentation URL are
    dead**, as are the `micro.ros.org` deep links: that host moved to
    `micro.vulcanexus.org`.
30. **micro-ROS has no apt binary for any current distribution**, and its
    published 75 KB and 3 KB figure covers Micro XRCE-DDS Client alone. **The
    "32 KB RAM and 256 KB flash" figure comes from a commercial blog, not the
    project.**
31. **The Beckhoff slave stack is gated behind ETG membership, not behind a
    vendor identifier**, and the vendor identifier itself is free of charge. The
    real cost is the mandatory conformance test tool subscription.
32. **TI InstaSPIN could not be verified at all**: omit it.

---

## Gaps this volume can claim honestly

Stated as new work in the chapter that looked for them, never dressed up as a
survey.

1. **No published source states that a safe state must be reachable from every
   other state**, and none treats fault latching as a named technique. Chapter
   18. The same absence covers heartbeat and liveness over the bus, fail-silent
   against fail-operational, and redundant state variables.
2. **No published motion-specific example runs a rig in continuous integration
   and asserts on tracking quality.** Chapter 20.
3. **No open software failure analysis tool is worth using.** Chapter 18
   recommends the NASA worksheets and a spreadsheet under version control.
4. **No CAN transport exists for MCUboot or for mcumgr and SMP**, and the
   micro-ROS CAN client is POSIX only. Chapter 19 writes the first; chapter 14
   writes the second.
5. **No published article presents the in-memory histogram method for proving a
   control period with no probe.** Chapter 02 presents it as the author's
   construction on two cited ideas.
6. **No verified open embedded implementation does textbook back-calculation
   anti-windup.** Chapter 16's, if shown, is the author's.
7. **No equivalent to EN 50325-5 was found for CAN FD.** Chapter 18 names the
   gap and points at the black channel idea as the frame to verify.

---

## Open questions carried from the research

Not written as fact anywhere until checked on a machine that can reach st.com,
or against RM0455, UM2408 and the datasheet.

- **The datasheet number for this part.** DS12923 appears to be wrong, attaching
  to the STM32H745 and H747; DS13195 is the candidate and is unconfirmed.
- Flash endurance cycles and retention years. The rotating log argument is
  parameterised so that it holds whatever the figure is.
- Flash word program time and 8 KB sector erase time. The conclusion that an
  erase never fits in the power-down path does not depend on the exact values.
- Backup SRAM size, address and retention conditions, cross-checked against
  RM0455 rather than only the headers.
- The TSCC and TSCV clause numbers in RM0455, and the dead-time generator
  encoding.
- FDCAN message RAM on this part: how much, how divided between filters, receive
  buffers and transmit buffers, and whether it must be configured before use.
  This differs across the family and is a common cause of a controller that
  never transmits.
- Whether an FDCAN buffer placed in tightly coupled memory is reachable by the
  peripheral.
- Which timers are 32 bit, and which offers complementary outputs with dead time
  and a fault input, on which pins.
- Whether the two-times and four-times mapping of the three encoder modes is as
  standard semantics suggests.
- The transceiver's data rate once bought, and its suffix.
- Whether X-CUBE-STL covers the STM32H7A3, and under what licence.
- Whether MCSDK 6.4.2 supports H7.
- The current version number of X-CUBE-EEPROM, and whether AN3969 has a revision
  later than October 2011.
- **IEC 61784-3**, number, title and edition, for the black channel section.
- ISO 13850 clause 4.1.3's actual text, and ISO 13849-1's Category and
  diagnostic coverage tables, both behind paywalls.
- The EN adoption of ISO 10218:2025 in the Official Journal, reported at
  secondary level as Monday 7 September 2026.

---

## Link hygiene before delivery

Every address cited anywhere in the volume is resolved once more at the end, and
this file is stamped with the date it was checked. A book that tells a reader to
clone repositories owes them working addresses. Several domains that matter here
refuse automated checking, st.com among them, so every document from those is
opened by hand in a browser before the volume goes out.

Checked: **Monday 21 September 2026.**
