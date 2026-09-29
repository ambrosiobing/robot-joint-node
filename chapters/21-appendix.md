# Appendix

## The twenty chapters at a glance

One page, so that a chapter can be chosen by what it gives rather than by its title. The last column is the one this volume cares about most.

|  | Title | What the node gains | Diff. | Evenings | Real, modelled or absent |
| --- | --- | --- | --- | --- | --- |
| 1 | What a joint node is, and the bench that stands in for one | An identity and a bring-up | 2 | 2 | Real, and it names all three absences |
| 2 | The control period: 1 kHz you can prove | A heartbeat | 3 | 2 | Real |
| 3 | One clock for sensors, loop and bus | A shared time base | 4 | 3 | Real |
| 4 | The board support package | A board file you can hand over | 3 | 3 | Real, and it proves two absences |
| 5 | The encoder | Position, exactly | 3 | 2 | Real, with a generated source |
| 6 | The inertial unit | An inner ear | 3 | 2 | Real |
| 7 | The actuator you do not have | A drive stage, in firmware | 4 | 3 | **Real firmware, modelled plant** |
| 8 | Force and torque | An estimate, honestly labelled | 3 | 2 | **Modelled, and labelled everywhere** |
| 9 | CAN-FD from the controller out | A wire | 4 | 3 | Real |
| 10 | The motion master | Somebody to talk to | 3 | 2 | Real |
| 11 | A joint protocol | A vocabulary | 4 | 3 | Real |
| 12 | Network management | A way to notice | 4 | 3 | Real |
| 13 | Two speeds on one wire | A diagnosis | 3 | 2 | Real, with a part bought to fail |
| 14 | The node as a middleware participant | A place in the robot | 5 | 4 | Real software, absent robot |
| 15 | Joint state and joint command as messages | A standard shape | 3 | 2 | Real, one field left empty |
| 16 | The loop closed over the bus | A closed loop | 5 | 4 | **Real controller, modelled plant** |
| 17 | What a real-time fieldbus would change | An honest boundary | 2 to 5 | 2 | **Neither: nothing is built** |
| 18 | Safe states, and the workspace sensor | A way to stop | 4 | 3 | Real trigger, modelled reaction |
| 19 | Update over the bus | Maintainability | 5 | 4 | Real |
| 20 | The rig | Proof | 5 | 5 | Both, deliberately |

*Table 1. The twenty chapters, their cost and their honesty class. Six of the twenty carry something modelled or absent, and every one of those six says so in its key facts, in its figures and in its budget table. Roughly sixty evenings in total, and two chapters need no hardware at all.*

## The honesty ledger

Everything in this volume that is not what it appears to be, in one table, so that a reader never has to hunt for the caveat.

| Thing | Class | Why | Where it is handled |
| --- | --- | --- | --- |
| A brushless motor and drive stage | **Absent** | No motor, no driver board, no current sensing on this bench | Chapter 7 builds the firmware in full and drives a model |
| A force or torque sensor | **Absent** | Not on the bench, and the cheap substitutes measure something else | Chapter 8 publishes a derived estimate with a seven term error budget |
| A real-time Ethernet fieldbus | **Structurally impossible** | This part has no Ethernet controller, and this silicon vendor sells no subdevice controller at all | Chapter 17 costs it out and refuses it |
| A brake | **Absent** | And so the second of two safe states is designed and never felt | Chapter 18, in logic only |
| An external watchdog | **Absent** | Both on-die watchdogs share the chip's fate | Chapter 18 says so rather than claiming independence |
| A second channel, and an assessment | **Absent** | Single channel reaches the lower integrity levels by architecture | Chapter 18's page titled what this node is not |
| An oscilloscope, a logic analyser, an external probe | **Absent since chapter 1** | Which is why every timing claim uses the part's own counters | Chapters 2 and 20 |
| A soldering iron | **Absent** | Which rules out three otherwise reasonable routes | Chapters 13, 17 and 19 |
| The plant in the control loop | **Modelled** | The loop closes through one dashed block and six solid ones | Chapter 16, on every figure |
| Position feedback in chapter 1's table | **Corrected in chapter 5** | Chapter 1 claimed it was real before the encoder existed | The correction is printed in chapter 5 rather than edited away |
| The robot the middleware would join | **Absent** | No framework runs on this bench; only the agent is proven | Chapter 14, drawn dotted |

*Table 2. The honesty ledger. The tenth row is the one worth pointing at in an interview: an internal inconsistency was found while writing chapter 5, and it was corrected in the open, in the chapter that found it, rather than repaired quietly in chapter 1.*

## Corrections carried into this volume

Claims in wide circulation that the research behind this volume found to be wrong. Each is printed as a correction in the chapter that meets it, and each is here in one place because a reader is more likely to meet the claim than the chapter.

### About this part

1. **The STM32H7A3ZI has no Ethernet controller**, proven from interrupt vector slots 61 and 62 being absent where its popular sibling has them, rather than from a product page.
2. **One upstream device tree leaves a network controller node in place** as an inherited modelling artefact, so reading that file gives the opposite answer to grepping the vendor header. Both were read.
3. **This part's flash geometry is not its sibling's**: a sixteen byte word and an eight kilobyte sector, against thirty-two and one hundred and twenty-eight. **The comment block in the vendor's own flash driver source describing the wider word is written for the sibling and is wrong here.**
4. **The backup registers live in the tamper peripheral on this part**, not in the real-time clock. Code ported from the sibling will not compile or will not work.
5. **The brown-out reset has four levels, not the three usually quoted**, and the programmable voltage detector has seven thresholds plus an eighth selection that compares an external analog input against the reference.
6. **The two voltage detectors share one interrupt line**, so a handler must read both status bits to tell them apart.
7. **The ROM bootloader already speaks the flexible-data bus**, on two specific pins, and its detection chain polls that bus first.
8. **This family has no hardware encoder index**, on the vendor's own statement. A later family was the first to have one.
9. **The part trap does not apply to the bus peripheral**: same core, same addresses, same bit timing fields, with three real differences named in chapter 9.

### About libraries, stacks and packages

1. **The vendor's EEPROM emulation package does not cover this family.** The string identifying it does not appear anywhere in the current revision of the relevant application note.
2. **That note was renamed.** Its current title is "How to use EEPROM emulation on STM32 MCUs", revision 11, Tuesday 18 March 2025. A second note frequently cited alongside it is about dual bank firmware update, not emulation.
3. **The motor control package is under the vendor's own licence**, not the permissive one usually assumed.
4. **The widely repeated middleware footprint of 32 kB of memory and 256 kB of flash comes from a commercial blog, not from the project.** The project's own published figure covers the constrained-environments client alone and is much narrower in scope.
5. **That middleware has no packaged binary for any current distribution**, and its original documentation host now redirects, with its deep links returning not-found.
6. **The portable open master stack moved to stronger copyleft on Friday 11 July 2025**, not away from it. Automated licence checks report nothing because its licence file has an unexpected extension.
7. **One popular bootloader is GPL-3.0, not the earlier version usually quoted.**
8. **The claim that a portable open bootloader cannot run on this family because of its write block size is contradicted by its current source**, whose assertion covers this part's sixteen byte word.
9. **One widely used controller library is permissively licensed only by a sentence in a readme file**, with no licence file, no identifier line and no metadata field, so scanners report none. Its other readme returns not-found.
10. **The filter library often recommended alongside it contains no controller at all.**
11. **One bus tooling project was renamed and then archived by its owner on Friday 7 May 2021**; the claim that a particular successor superseded it could not be confirmed.
12. **The download object is named "Program data"**, not the longer form usually written down.
13. **A separate vendor slave stack is gated behind membership, not behind a vendor identifier**, and the identifier itself is free of charge. The real cost is a mandatory recurring conformance test tool subscription.
14. **There is no applications processor in that family with an integrated subdevice controller**; the integrated part is a different device with a different core inside it.

### About standards and safety

1. **The industrial robot safety series is now titled "Robotics"**, not the older wording, and getting it wrong dates a document at a glance.
2. **The collaborative robot technical specification is not withdrawn.** It is published, was confirmed in 2022, and remains current until its replacement appears. Several secondary sources say otherwise and they are wrong.
3. **The machinery functional safety standard's 2021 edition lost its restriction to electrical technologies rather than gaining non-electrical coverage**, and both it and the electrical equipment standard carry amendments that a bare citation omits.
4. **The current coding guidelines edition is dated March 2025**, and the 2023 edition consolidated the 2012 one with its amendments, so the old form of citation is out of date.
5. **One silicon vendor's safety architecture claim is routinely quoted with its redundancy scheme changed**, which reverses the meaning of its second half.
6. **A certified kernel product is not a certified edition of the free one.** They share a functional foundation and are separate, redesigned products.
7. **Both titles in circulation for the two firmware update architecture documents are wrong**; the verified forms are in chapter 19.
8. **One timestamping standard's 2020 edition is superseded by its 2025 one**, and a bus specification number frequently cited for the same subject does not exist.
9. **One data link standard's current edition carries a different subtitle from its predecessor**, so the edition is part of the citation rather than an optional extra.

## Gaps this volume can claim

Stated as new work in the chapter that looked for them, never dressed up as a survey. Each was searched for deliberately, and each absence is a finding.

1. **No published source states that a safe state must be reachable from every other state**, and none treats fault latching as a named technique. Both are real and universally practised. *Chapter 18 presents them as engineering practice and attributes them to nobody.* The same absence covers heartbeat and liveness over a bus, fail-silent against fail-operational, and redundant state variables.
2. **No published, motion-specific example runs a rig in continuous integration and asserts on tracking quality.** The generic material is plentiful. *Chapter 20, and a reader who publishes one will have written something that does not exist.*
3. **No open software failure analysis tool is worth using.** The candidates carry six or seven commits between them. *Chapter 18 recommends a space agency's freely usable worksheets and a spreadsheet under version control.*
4. **No bus transport exists for the portable open bootloader or for its management protocol**, whose transports are wireless and two kinds of serial. *Chapter 19 names the gap and writes into it.*
5. **The embedded middleware's bus transport is for a full operating system only**, with no implementation for a small kernel anywhere. *Chapter 14 writes the six functions it needs.*
6. **No published article presents the in-memory histogram method for proving a control period with no probe.** *Chapter 2 presents it as the author's construction on two cited ideas.*
7. **No verified open embedded implementation in this language does textbook back-calculation anti-windup.** Clamping is everywhere; conditional integration is common. *Chapter 16 writes it and measures it against clamping on the same step.*
8. **No equivalent to the classic-frame safety communication layer was found for the flexible-data format.** *Chapter 18 names the gap and points at the untrusted-transport framing as the idea to verify.*

## Open questions for the bench

Nothing on this list is written as fact anywhere in the volume. Each is a question for a session with the reference manual, the board manual and the datasheet open, on a machine that can reach the vendor's site.

- **The datasheet document number for this part.** One number in circulation attaches to two different parts; the candidate is unconfirmed.
- **Flash endurance cycles and retention years.** Chapter 19's rotating log argument is parameterised so that it holds whatever the figure turns out to be.
- **Flash word program time and eight kilobyte sector erase time.** The conclusion that an erase never fits in the power-down path does not depend on the exact values.
- **Backup memory size, address and retention conditions**, checked against the reference manual rather than only against the headers.
- **The timestamp counter clause numbers, and the dead-time generator encoding.**
- **The bus controller's message memory on this part**: how much, how divided between filters and buffers, and whether it must be configured before use. This differs across the family and is a common cause of a controller that never transmits.
- **Whether a bus buffer placed in tightly coupled memory is reachable by the peripheral at all.**
- **Which timers are 32 bit**, and which offers complementary outputs with dead time and a fault input, on which pins.
- **Whether the two-times and four-times encoder mode mapping is as standard semantics suggests.**
- **The transceiver's data rate and suffix**, once bought.
- **Whether the vendor's safety library covers this part at all, and under what licence.** Chapter 18 uses nothing from it for exactly this reason.
- **The untrusted-transport standard family's number, title and edition**, for chapter 18's closing paragraph.
- **Two paywalled clauses**: one stop-category clause and one set of category and diagnostic coverage tables.
- **The harmonised adoption date of the 2025 robot safety standards**, reported at secondary level only.

## Licences, decided once

Four categories, applied consistently through the volume, and the reason every chapter checks a licence before writing code around a library rather than after.

| Category | Rule | What is in it here |
| --- | --- | --- |
| **A. Depend and vendor** | Permissive: copy it in, keep the headers, record the provenance | The simulator, the bus stacks, the bootloader that is portable, the fault backtrace and logging libraries, the fake framework, the unit test tools, the message definitions, the host control framework, the real-time helpers, the trace recorder's target-side source |
| **B. Depend, do not vendor** | Copyleft in a way that reaches a linked program, or a library boundary that matters | The host bus library, the bench control layer, the fixed-point controller, the kernel-space fieldbus master |
| **C. Read, do not copy** | Useful to read, tied to one vendor's silicon, or commercial beyond a point | The vendor middleware, the vendor safety library whose coverage of this part is unconfirmed, the event framework that is copyleft or commercial, the commercial trace visualiser |
| **D. No licence at all** | **All rights reserved.** Cite as reading, never vendor | Several community repositories with no licence file, one dormant since Monday 10 April 2017, and one bootloader whose licence path returns not-found |

*Table 3. The four categories. Category D is the one that catches people, because a repository with no licence file is not permissive by default, it is closed by default, and several of the most convenient-looking results for these subjects are in it.*

Three positions are worth stating separately because they changed a decision in this volume rather than merely being noted.

- **The most consequential single licence decision is in chapter 19.** An excellent, maintained bootloader solves that chapter's problem completely on this family and in both frame formats, and linking the joint node's firmware against it would make the whole firmware copyleft. The chapter states this before a reader writes any code around it.
- **A permissive wrapper does not describe the stack it deploys.** Chapter 17's fieldbus bridge is permissively licensed and installs on a copyleft out-of-tree kernel module, with a procedure that requires disabling secure boot. The check is one dependency level deeper than most people look.
- **One vendor archive ships three different licence statements**, between its release notes, its source file headers and a licence agreement document included in the same archive, the last of which forbids commercial distribution. A licence is what the files and the archive say together.

## The reference library

Everything cited anywhere in the volume, grouped by what it is for, with its licence where it has one. Every address was resolved once more before delivery, and this list is stamped with the date it was checked. A book that tells a reader to clone repositories owes them working addresses.

### The bus, the protocol and the host side

- ISO 11898-1:2024, third edition, May 2024, the data link standard. **Its subtitle differs from the 2015 edition's, so the edition is part of the citation.** Paywalled.
- The command line bus tools, dual licensed per file, whose load generator and replay utility carry chapters 10, 13 and 20.  
  <https://github.com/linux-can/can-utils>
- A permissively licensed bus stack implementing the safety data object wire format, Apache-2.0.  
  <https://github.com/CANopenNode/CANopenNode>
- The maintained bus-connected motion stack, Apache-2.0, which is the master side of the contract this volume's node sits opposite.  
  <https://github.com/ros-industrial/ros2_canopen>
- The raw frame wrapper and its message package, Apache-2.0, which defines a flexible-data frame type.  
  <https://github.com/autowarefoundation/ros2_socketcan>

### Middleware, messages and control

- The configurator integration utilities, Apache-2.0, and the client and agent, Apache-2.0.  
  <https://github.com/micro-ROS/micro_ros_stm32cubemx_utils>  
  <https://github.com/eProsima/Micro-XRCE-DDS-Client>
- The common interface definitions, Apache-2.0, and the control message package, BSD-3-Clause, for the message types and the six error codes.  
  <https://github.com/ros2/common_interfaces>
- The host control framework, Apache-2.0, and its real-time helper library, BSD-3-Clause.  
  <https://github.com/ros-controls/ros2_control>
- The best-fitting small controller, **MIT at repository level with no header in either source file**.  
  <https://github.com/pms67/PID>
- Astrom and Murray, *Feedback Systems*, chapter 10, section 10.4. **The book is the citation; the hosting site serves a broken certificate chain, so no link is given.**
- Koren, "Cross-Coupled Biaxial Computer Control for Manufacturing Systems", Journal of Dynamic Systems, Measurement, and Control volume 102, pages 265 to 272, 1980, for the distinction this volume relies on between following error and contouring error.

### Safety

- IEC 61508-4:2010, clause 3.1.13 for the safe state definition and clause 3.2.11 for the tool classification, both read from the official preview.
- ISO 13850:2015, clause 4.1.1, read directly, for the six requirements chapter 18's latch is written against.
- ISO 10218-1:2025 and ISO 10218-2:2025, February 2025, which for a joint is the type-C standard and therefore takes precedence.
- The space agency's software engineering and assurance handbook, topic 8.5, with four worksheet templates. **A United States government work with no copyright notice, and the safest material in this volume to adapt and publish.**
- The free performance level calculator, version 3.0.4 of Friday 31 October 2025. **Free to use and pass on; modification and re-hosting are not permitted.**

### Update, storage and the rig

- RFC 9019, April 2021, and RFC 9124, January 2022, both Informational and both free to redistribute, **which makes them the safest normative references in the volume**.  
  <https://www.rfc-editor.org/rfc/rfc9019>
- The portable open bootloader, Apache-2.0, port agnostic by its own porting guide and **without any bus transport**.  
  <https://github.com/mcu-tools/mcuboot>
- A power-loss-resilient filesystem, BSD-3-Clause, and a key-value store, Apache-2.0.  
  <https://github.com/littlefs-project/littlefs>
- The open simulator, which models this family including its bus controller and bridges both frame formats to a host virtual interface.  
  <https://github.com/renode/renode>
- A fault diagnosis library covering this core, MIT, needing no debugger, and a small logger with a flash backend, MIT.  
  <https://github.com/armink/CmBacktrace>  
  <https://github.com/armink/EasyLogger>
- A header-only fake framework, MIT, whose return sequences are unit level fault injection.  
  <https://github.com/meekrosoft/fff>
- ISO 9283:1998, second edition, Thursday 23 April 1998, for the assertion names chapter 20 borrows. **Its metric symbols could not be confirmed and are not printed.**
- Hsueh, Tsai and Iyer, "Fault injection techniques and tools", Computer volume 30, pages 75 to 82, 1997, and Natella and co-authors, "On Fault Representativeness of Software Fault Injection", IEEE Transactions on Software Engineering volume 39, pages 80 to 96, 2013.

### Link hygiene

Every address above and every address in the twenty chapters’ Sources sections was resolved once more before delivery. Several domains refuse automated checking, the silicon vendor's among them, so every document from those was opened by hand in a browser. **Checked: Monday 21 September 2026.**

Four categories of dead address were found during the research and are recorded here so that nobody spends an afternoon on them: four article slugs from one embedded engineering site, every documentation address on one operating system's retired estate, the deep links into one middleware project's original documentation host, and one bootloader repository's licence path.

---

[Previous](20-the-rig.md) &nbsp;&nbsp;|&nbsp;&nbsp; [Contents](../README.md)
