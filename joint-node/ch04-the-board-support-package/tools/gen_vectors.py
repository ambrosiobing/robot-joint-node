#!/usr/bin/env python3
"""The device interrupt vector table for the STM32H7A3ZI, as data.

    python tools/gen_vectors.py            # write src/bsp/vectors.c and .h
    python tools/gen_vectors.py --check    # for the build
    python tools/gen_vectors.py --table    # the table in doc/interrupts.md

WHY THIS IS GENERATED RATHER THAN TYPED. The table is a list of 155 positions
in a fixed order, and every mistake available in it is silent:

  - A SHORT TABLE LINKS AND REPORTS A SENSIBLE SIZE. Until Monday
    9 November 2026 startup.c carried the sixteen core vectors and nothing
    else. That is correct exactly as long as no device interrupt is enabled,
    and the moment one is the core fetches a handler from past the end of the
    table and branches to whatever word of flash follows it.

  - OMITTING THE RESERVED POSITIONS SHIFTS EVERY LATER ONE. 134 of the 155
    device positions have a name and 21 do not, and the unnamed ones are
    scattered through the range rather than gathered at the end: the first
    gap is at 61. A table built from the list of NAMED interrupts is 21 entries
    short, and every position above the first gap lands on the wrong handler.
    The console's USART3 at 39 is below that gap and would survive, which is
    the worst possible version of this defect: the one peripheral this chapter
    brings up first is the one that would not reveal it.

  - THE TABLE'S ALIGNMENT DEPENDS ON ITS LENGTH. See ALIGN_BYTES below.

So the positions are data here, each with the vendor source it came from, and
the C is generated from them.

PROVENANCE. Two independent vendors, and they agree.

  `[vendor]` The 155 positions and their names are ST's own, from the
  `IRQn_Type` enum in the CMSIS device header for this exact part:
  https://raw.githubusercontent.com/STMicroelectronics/cmsis_device_h7/master/Include/stm32h7a3xx.h
  Read on Monday 9 November 2026 by parsing the enum rather than by reading it,
  because a table of 155 numbers must not pass through a paraphrase.

  `[vendor]` The positions of the peripherals this volume actually uses are
  corroborated against Zephyr's device tree, which is a different vendor
  reading the same silicon: CORROBORATED below, checked by this file. Zephyr's
  node for the bus is `can@4000a000`, which also agrees with the register base
  chapter 9 recorded from the same tree.

  `[vendor]` The alignment floor is ARM's: `SCB_VTOR_TBLOFF_Pos` is 7 in
  CMSIS `core_cm7.h`, so VTOR holds bits [31:7] of the base and the table must
  begin on a 128 byte boundary.
"""
import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
VECTORS_C = HERE / "src" / "bsp" / "vectors.c"
VECTORS_H = HERE / "src" / "bsp" / "vectors.h"

# The sixteen core entries, which are the architecture's rather than the part's
# and so are the same on every Cortex-M7. Entry 0 is the initial stack pointer
# and not a handler, which is why it is named separately in the C.
CORE = [
    (0, "_estack", "the END of the stack region, not a handler"),
    (1, "Reset_Handler", "reset"),
    (2, "NMI_Handler", "non maskable interrupt"),
    (3, "HardFault_Handler", "hard fault"),
    (4, "MemManage_Handler", "memory management fault"),
    (5, "BusFault_Handler", "bus fault"),
    (6, "UsageFault_Handler", "usage fault"),
    (7, None, "reserved"),
    (8, None, "reserved"),
    (9, None, "reserved"),
    (10, None, "reserved"),
    (11, "SVC_Handler", "supervisor call"),
    (12, "DebugMon_Handler", "debug monitor"),
    (13, None, "reserved"),
    (14, "PendSV_Handler", "pendable service call"),
    (15, "SysTick_Handler", "system tick"),
]

# `[vendor]` ST's IRQn_Type enum for the STM32H7A3, position and name, with the
# vendor's own one line description kept so a reader does not have to guess what
# a name abbreviates. Positions absent from this list are reserved; they are
# derived below rather than listed, because a second list is a second list to
# forget.
DEVICE = [
    (  0, "WWDG",
     "Window WatchDog Interrupt ( wwdg1_it, wwdg2_it)"),
    (  1, "PVD_PVM",
     "PVD/PVM through EXTI Line detection Interrupt"),
    (  2, "RTC_TAMP_STAMP_CSS_LSE",
     "Tamper, TimeStamp, CSS and LSE interrupts through the EXTI line"),
    (  3, "RTC_WKUP",
     "RTC Wakeup interrupt through the EXTI line"),
    (  4, "FLASH",
     "FLASH global Interrupt"),
    (  5, "RCC",
     "RCC global Interrupt"),
    (  6, "EXTI0",
     "EXTI Line0 Interrupt"),
    (  7, "EXTI1",
     "EXTI Line1 Interrupt"),
    (  8, "EXTI2",
     "EXTI Line2 Interrupt"),
    (  9, "EXTI3",
     "EXTI Line3 Interrupt"),
    ( 10, "EXTI4",
     "EXTI Line4 Interrupt"),
    ( 11, "DMA1_Stream0",
     "DMA1 Stream 0 global Interrupt"),
    ( 12, "DMA1_Stream1",
     "DMA1 Stream 1 global Interrupt"),
    ( 13, "DMA1_Stream2",
     "DMA1 Stream 2 global Interrupt"),
    ( 14, "DMA1_Stream3",
     "DMA1 Stream 3 global Interrupt"),
    ( 15, "DMA1_Stream4",
     "DMA1 Stream 4 global Interrupt"),
    ( 16, "DMA1_Stream5",
     "DMA1 Stream 5 global Interrupt"),
    ( 17, "DMA1_Stream6",
     "DMA1 Stream 6 global Interrupt"),
    ( 18, "ADC",
     "ADC1 and ADC2 global Interrupts"),
    ( 19, "FDCAN1_IT0",
     "FDCAN1 Interrupt line 0"),
    ( 20, "FDCAN2_IT0",
     "FDCAN2 Interrupt line 0"),
    ( 21, "FDCAN1_IT1",
     "FDCAN1 Interrupt line 1"),
    ( 22, "FDCAN2_IT1",
     "FDCAN2 Interrupt line 1"),
    ( 23, "EXTI9_5",
     "External Line[9:5] Interrupts"),
    ( 24, "TIM1_BRK",
     "TIM1 Break Interrupt"),
    ( 25, "TIM1_UP",
     "TIM1 Update Interrupt"),
    ( 26, "TIM1_TRG_COM",
     "TIM1 Trigger and Commutation Interrupt"),
    ( 27, "TIM1_CC",
     "TIM1 Capture Compare Interrupt"),
    ( 28, "TIM2",
     "TIM2 global Interrupt"),
    ( 29, "TIM3",
     "TIM3 global Interrupt"),
    ( 30, "TIM4",
     "TIM4 global Interrupt"),
    ( 31, "I2C1_EV",
     "I2C1 Event Interrupt"),
    ( 32, "I2C1_ER",
     "I2C1 Error Interrupt"),
    ( 33, "I2C2_EV",
     "I2C2 Event Interrupt"),
    ( 34, "I2C2_ER",
     "I2C2 Error Interrupt"),
    ( 35, "SPI1",
     "SPI1 global Interrupt"),
    ( 36, "SPI2",
     "SPI2 global Interrupt"),
    ( 37, "USART1",
     "USART1 global Interrupt"),
    ( 38, "USART2",
     "USART2 global Interrupt"),
    ( 39, "USART3",
     "USART3 global Interrupt"),
    ( 40, "EXTI15_10",
     "External Line[15:10] Interrupts"),
    ( 41, "RTC_Alarm",
     "RTC Alarm (A and B) through EXTI Line Interrupt"),
    ( 42, "DFSDM2",
     "DFSDM2 global Interrupt"),
    ( 43, "TIM8_BRK_TIM12",
     "TIM8 Break Interrupt and TIM12 global interrupt"),
    ( 44, "TIM8_UP_TIM13",
     "TIM8 Update Interrupt and TIM13 global interrupt"),
    ( 45, "TIM8_TRG_COM_TIM14",
     "TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt"),
    ( 46, "TIM8_CC",
     "TIM8 Capture Compare Interrupt"),
    ( 47, "DMA1_Stream7",
     "DMA1 Stream7 Interrupt"),
    ( 48, "FMC",
     "FMC global Interrupt"),
    ( 49, "SDMMC1",
     "SDMMC1 global Interrupt"),
    ( 50, "TIM5",
     "TIM5 global Interrupt"),
    ( 51, "SPI3",
     "SPI3 global Interrupt"),
    ( 52, "UART4",
     "UART4 global Interrupt"),
    ( 53, "UART5",
     "UART5 global Interrupt"),
    ( 54, "TIM6_DAC",
     "TIM6 global and DAC1&2 underrun error interrupts"),
    ( 55, "TIM7",
     "TIM7 global interrupt"),
    ( 56, "DMA2_Stream0",
     "DMA2 Stream 0 global Interrupt"),
    ( 57, "DMA2_Stream1",
     "DMA2 Stream 1 global Interrupt"),
    ( 58, "DMA2_Stream2",
     "DMA2 Stream 2 global Interrupt"),
    ( 59, "DMA2_Stream3",
     "DMA2 Stream 3 global Interrupt"),
    ( 60, "DMA2_Stream4",
     "DMA2 Stream 4 global Interrupt"),
    ( 63, "FDCAN_CAL",
     "FDCAN Calibration unit Interrupt"),
    ( 64, "DFSDM1_FLT4",
     "DFSDM Filter4 Interrupt"),
    ( 65, "DFSDM1_FLT5",
     "DFSDM Filter5 Interrupt"),
    ( 66, "DFSDM1_FLT6",
     "DFSDM Filter6 Interrupt"),
    ( 67, "DFSDM1_FLT7",
     "DFSDM Filter7 Interrupt"),
    ( 68, "DMA2_Stream5",
     "DMA2 Stream 5 global interrupt"),
    ( 69, "DMA2_Stream6",
     "DMA2 Stream 6 global interrupt"),
    ( 70, "DMA2_Stream7",
     "DMA2 Stream 7 global interrupt"),
    ( 71, "USART6",
     "USART6 global interrupt"),
    ( 72, "I2C3_EV",
     "I2C3 event interrupt"),
    ( 73, "I2C3_ER",
     "I2C3 error interrupt"),
    ( 74, "OTG_HS_EP1_OUT",
     "USB OTG HS End Point 1 Out global interrupt"),
    ( 75, "OTG_HS_EP1_IN",
     "USB OTG HS End Point 1 In global interrupt"),
    ( 76, "OTG_HS_WKUP",
     "USB OTG HS Wakeup through EXTI interrupt"),
    ( 77, "OTG_HS",
     "USB OTG HS global interrupt"),
    ( 78, "DCMI_PSSI",
     "DCMI and PSSI global interrupt"),
    ( 80, "RNG",
     "RNG global interrupt"),
    ( 81, "FPU",
     "FPU global interrupt"),
    ( 82, "UART7",
     "UART7 global interrupt"),
    ( 83, "UART8",
     "UART8 global interrupt"),
    ( 84, "SPI4",
     "SPI4 global Interrupt"),
    ( 85, "SPI5",
     "SPI5 global Interrupt"),
    ( 86, "SPI6",
     "SPI6 global Interrupt"),
    ( 87, "SAI1",
     "SAI1 global Interrupt"),
    ( 88, "LTDC",
     "LTDC global Interrupt"),
    ( 89, "LTDC_ER",
     "LTDC Error global Interrupt"),
    ( 90, "DMA2D",
     "DMA2D global Interrupt"),
    ( 91, "SAI2",
     "SAI2 global Interrupt"),
    ( 92, "OCTOSPI1",
     "OCTOSPI1 global interrupt"),
    ( 93, "LPTIM1",
     "LP TIM1 interrupt"),
    ( 94, "CEC",
     "HDMI-CEC global Interrupt"),
    ( 95, "I2C4_EV",
     "I2C4 Event Interrupt"),
    ( 96, "I2C4_ER",
     "I2C4 Error Interrupt"),
    ( 97, "SPDIF_RX",
     "SPDIF-RX global Interrupt"),
    (102, "DMAMUX1_OVR",
     "DMAMUX1 Overrun interrupt"),
    (110, "DFSDM1_FLT0",
     "DFSDM Filter1 Interrupt"),
    (111, "DFSDM1_FLT1",
     "DFSDM Filter2 Interrupt"),
    (112, "DFSDM1_FLT2",
     "DFSDM Filter3 Interrupt"),
    (113, "DFSDM1_FLT3",
     "DFSDM Filter4 Interrupt"),
    (115, "SWPMI1",
     "Serial Wire Interface 1 global interrupt"),
    (116, "TIM15",
     "TIM15 global Interrupt"),
    (117, "TIM16",
     "TIM16 global Interrupt"),
    (118, "TIM17",
     "TIM17 global Interrupt"),
    (119, "MDIOS_WKUP",
     "MDIOS Wakeup Interrupt"),
    (120, "MDIOS",
     "MDIOS global Interrupt"),
    (121, "JPEG",
     "JPEG global Interrupt"),
    (122, "MDMA",
     "MDMA global Interrupt"),
    (124, "SDMMC2",
     "SDMMC2 global Interrupt"),
    (125, "HSEM1",
     "HSEM1 global Interrupt"),
    (127, "DAC2",
     "DAC2 global Interrupt"),
    (128, "DMAMUX2_OVR",
     "DMAMUX2 Overrun interrupt"),
    (129, "BDMA2_Channel0",
     "BDMA2 Channel 0 global Interrupt"),
    (130, "BDMA2_Channel1",
     "BDMA2 Channel 1 global Interrupt"),
    (131, "BDMA2_Channel2",
     "BDMA2 Channel 2 global Interrupt"),
    (132, "BDMA2_Channel3",
     "BDMA2 Channel 3 global Interrupt"),
    (133, "BDMA2_Channel4",
     "BDMA2 Channel 4 global Interrupt"),
    (134, "BDMA2_Channel5",
     "BDMA2 Channel 5 global Interrupt"),
    (135, "BDMA2_Channel6",
     "BDMA2 Channel 6 global Interrupt"),
    (136, "BDMA2_Channel7",
     "BDMA2 Channel 7 global Interrupt"),
    (137, "COMP",
     "COMP global Interrupt"),
    (138, "LPTIM2",
     "LP TIM2 global interrupt"),
    (139, "LPTIM3",
     "LP TIM3 global interrupt"),
    (140, "UART9",
     "UART9 global interrupt"),
    (141, "USART10",
     "USART10 global interrupt"),
    (142, "LPUART1",
     "LP UART1 interrupt"),
    (143, "WWDG_RST",
     "Window Watchdog Event interrupt"),
    (144, "CRS",
     "Clock Recovery Global Interrupt"),
    (145, "ECC",
     "ECC diagnostic Global Interrupt"),
    (147, "DTS",
     "Digital Temperature Sensor Global Interrupt"),
    (149, "WAKEUP_PIN",
     "Interrupt for all 6 wake-up pins"),
    (150, "OCTOSPI2",
     "OctoSPI2 global interrupt"),
    (153, "GFXMMU",
     "GFXMMU global interrupt"),
    (154, "BDMA1",
     "BDMA1 for DFSM global interrupt"),
]

# `[vendor]` Zephyr's device tree for the same part, for the peripherals this
# volume names. A disagreement here is a refusal: two vendors reading the same
# silicon must produce the same number, and if they do not then this table is
# not a fact yet.
#
# From dts/arm/st/h7/stm32h7.dtsi on Monday 9 November 2026:
#   usart3: serial@40004800 { interrupts = <39 0>; }
#   fdcan1: can@4000a000  { interrupts = <19 0>, <21 0>, <63 0>; }
CORROBORATED = {
    "USART3": (39, "usart3: serial@40004800, interrupts = <39 0>"),
    "FDCAN1_IT0": (19, "fdcan1: can@4000a000, interrupts = <19 0>, ..."),
    "FDCAN1_IT1": (21, "fdcan1: can@4000a000, interrupts = ..., <21 0>, ..."),
    "FDCAN_CAL": (63, "fdcan1: can@4000a000, interrupts = ..., <63 0>"),
}

# `[vendor]` ABSENCE IN THIS TABLE IS A STATEMENT, NOT A SILENCE, and chapter 4
# relies on it: `BOARD_HAS_ETHERNET` is 0 and the proof the chapter cites is
# this table. So the claim is gated rather than asserted.
#
# Positions 61 and 62 are reserved on the STM32H7A3 and on the STM32H743, the
# better known sibling in the same family, they are:
#   ETH_IRQn      = 61   Ethernet global Interrupt
#   ETH_WKUP_IRQn = 62   Ethernet Wakeup through EXTI line Interrupt
# from Include/stm32h743xx.h in the same vendor repository, read on Monday
# 9 November 2026. Two headers from one vendor, and the difference between them
# is the fact: this part has no Ethernet, and the hole where the sibling's
# interrupt sits is where that shows.
#
# If either position ever acquires a name here, the chapter's Ethernet claim has
# to be revisited, which is what the check below forces.
ABSENT_HERE_PRESENT_ON_SIBLING = {
    61: ("ETH", "Ethernet global Interrupt, STM32H743"),
    62: ("ETH_WKUP", "Ethernet Wakeup through EXTI line Interrupt, STM32H743"),
}

CORE_ENTRIES = len(CORE)
TOP_DEVICE = max(p for p, _n, _c in DEVICE)
DEVICE_ENTRIES = TOP_DEVICE + 1
ENTRIES = CORE_ENTRIES + DEVICE_ENTRIES
TABLE_BYTES = ENTRIES * 4


def align_bytes(table_bytes):
    """The alignment the table needs, which DEPENDS ON ITS LENGTH.

    `[vendor]` The floor is 128: CMSIS core_cm7.h puts SCB_VTOR_TBLOFF at bit
    7, so VTOR cannot express a finer base than 128 bytes.

    `[unconfirmed]` Above that floor the architecture is documented to want the
    base aligned to a power of two at least as large as the table itself. That
    is asserted here from memory of the ARMv7-M reference and NOT from a source
    this bench can read, so it is marked and then SATISFIED anyway: over
    aligning costs padding in flash and nothing else, and the alternative is a
    table whose base is legal for the register and wrong for the core.

    Today the table also sits at the boot address, which is 0x08000000 and
    aligned to far more than this, so the directive is not what makes the image
    correct. What it does is stop the requirement being violated quietly if the
    table ever moves.
    """
    a = 128
    while a < table_bytes:
        a *= 2
    return a


ALIGN_BYTES = align_bytes(TABLE_BYTES)


def reserved_positions():
    named = {p for p, _n, _c in DEVICE}
    return [p for p in range(DEVICE_ENTRIES) if p not in named]


def by_position():
    """Every device position 0 to TOP_DEVICE, named or reserved."""
    named = {p: (n, c) for p, n, c in DEVICE}
    out = []
    for p in range(DEVICE_ENTRIES):
        if p in named:
            out.append((p, named[p][0], named[p][1]))
        else:
            out.append((p, None, "reserved"))
    return out


def problems():
    """Everything that must hold before a line of C is written."""
    bad = []

    seen = {}
    for p, n, _c in DEVICE:
        if p in seen:
            bad.append(f"position {p} is named twice, {seen[p]} and {n}")
        seen[p] = n
    names = [n for _p, n, _c in DEVICE]
    for n in set(names):
        if names.count(n) > 1:
            bad.append(f"the name {n} appears {names.count(n)} times")

    for n, (want, where) in CORROBORATED.items():
        got = [p for p, nn, _c in DEVICE if nn == n]
        if not got:
            bad.append(f"{n} is corroborated and is not in the table at all")
        elif got[0] != want:
            bad.append(f"{n} is position {got[0]} here and {want} in Zephyr "
                       f"({where}); two vendors disagree, so this is not a "
                       f"fact yet")

    for p, (sib, what) in ABSENT_HERE_PRESENT_ON_SIBLING.items():
        named = [n for pp, n, _c in DEVICE if pp == p]
        if named:
            bad.append(f"position {p} is named {named[0]} here and chapter 4 "
                       f"reads its absence as proof that this part has no "
                       f"{sib} ({what}); the claim and the table now disagree")

    # The whole point of the file. A table that is not exactly as long as the
    # highest position plus one has a hole or a tail, and both are silent.
    rows = by_position()
    if len(rows) != DEVICE_ENTRIES:
        bad.append(f"{len(rows)} device rows for {DEVICE_ENTRIES} positions")
    if TABLE_BYTES % 4 != 0:
        bad.append("the table is not a whole number of words")
    if ALIGN_BYTES < 128:
        bad.append(f"the alignment {ALIGN_BYTES} is below VTOR's 128 byte floor")
    if ALIGN_BYTES < TABLE_BYTES:
        bad.append(f"the alignment {ALIGN_BYTES} is smaller than the table's "
                   f"{TABLE_BYTES} bytes")
    return bad


def handler(name):
    return f"{name}_IRQHandler"


def as_header():
    res = reserved_positions()
    L = [
        "/* Generated by tools/gen_vectors.py. Do not edit. */",
        "#ifndef JOINT_VECTORS_H",
        "#define JOINT_VECTORS_H",
        "",
        "/* How long the table is, and what it must be aligned to. Both are",
        " * facts about the part rather than choices, and the linker script and",
        " * the table cannot be allowed to disagree about either: test_startup.py",
        " * reads these and the script's ALIGN and refuses a mismatch. */",
        f"#define BOARD_ISR_CORE_ENTRIES   {CORE_ENTRIES}u",
        f"#define BOARD_ISR_DEVICE_ENTRIES {DEVICE_ENTRIES}u",
        f"#define BOARD_ISR_ENTRIES        {ENTRIES}u",
        f"#define BOARD_ISR_BYTES          {TABLE_BYTES}u",
        f"#define BOARD_ISR_ALIGN          {ALIGN_BYTES}u",
        "",
        f"/* {len(DEVICE)} of the {DEVICE_ENTRIES} device positions carry a name;"
        f" {len(res)} are",
        " * reserved. The reserved ones are NOT at the end: the first is"
        f" {res[0] if res else 0}.",
        " * A table built from the named positions alone would be"
        f" {len(res)} entries short",
        " * and wrong for every position above that one. */",
        "",
    ]
    for p, n, c in DEVICE:
        L.append(f"#define BOARD_IRQ_{n} {p}u   /* {c} */")
    L += [
        "",
        "#endif /* JOINT_VECTORS_H */",
        "",
    ]
    return "\n".join(L)


def as_source():
    res = reserved_positions()
    L = [
        "/* Generated by tools/gen_vectors.py. Do not edit.",
        " *",
        " * The vector table, all " + str(ENTRIES) + " entries of it: the "
        + str(CORE_ENTRIES) + " core",
        " * exceptions, which are the architecture's, then the "
        + str(DEVICE_ENTRIES) + " device positions,",
        " * which are this part's.",
        " *",
        " * EVERY DEVICE HANDLER IS A WEAK ALIAS to the same default, so a file",
        " * elsewhere defines one simply by defining it under the vendor's own",
        " * name and the linker prefers the strong definition. Nothing in this",
        " * volume defines one yet.",
        " *",
        " * THE RESERVED POSITIONS HOLD A ZERO, both the architecture's five and",
        " * the part's twenty one, which is what ST's own startup file does. The",
        " * zero is not laziness and it is not a handler that was forgotten: a",
        " * reserved position is one nothing on this part is wired to, so it",
        " * cannot fire unless software pends it deliberately through the NVIC,",
        " * and nothing here does. If one ever does fire, a zero is the loud",
        " * outcome rather than the quiet one, because the core reads it as the",
        " * handler address and branches to 0 with the Thumb bit clear, which",
        " * faults. A default handler there would stop in a loop that looks",
        " * exactly like every other default handler stop.",
        " *",
        " * WHAT THE RESERVED POSITIONS ARE NOT IS OMITTED. They occupy their",
        " * entries. They are scattered through the range and not gathered at",
        " * the end, so a table that skipped them would put every later handler",
        " * at the wrong position, silently.",
        " */",
        '#include <stdint.h>',
        "",
        '#include "vectors.h"',
        "",
        "/* From the linker script, and an address rather than a value. */",
        "extern uint32_t _estack;",
        "",
        "/* Defined in startup.c, which owns the core handlers and the reset. */",
        "void Reset_Handler(void);",
        "void Default_Handler_alias(void);",
    ]
    for _p, n, _c in CORE[2:]:
        if n:
            L.append(f"void {n}(void);")
    L += [
        "",
        "/* The device handlers, under the vendor's own names so that a driver",
        " * written against any STM32 example defines the right symbol. */",
    ]
    for _p, n, _c in DEVICE:
        L.append(f"void {handler(n)}(void)")
        L.append('    __attribute__((weak, alias("Default_Handler_alias")));')
    L += [
        "",
        "/* uintptr_t and not a function pointer array, because entry 0 is the",
        " * initial stack pointer and not a function, and a cast between a data",
        " * address and a function pointer is the one cast C does not define. */",
        '__attribute__((section(".isr_vector"), used))',
        f"const uintptr_t g_isr_vector[{ENTRIES}] = {{",
        "    /* ---- the " + str(CORE_ENTRIES) + " core exceptions, the "
        "architecture's own ---- */",
    ]
    for p, n, c in CORE:
        if n is None:
            L.append(f"    0u,{'':30}/* {p}: {c} */")
        elif p == 0:
            L.append(f"    (uintptr_t) &{n},{'':14}/* {p}: {c} */")
        else:
            L.append(f"    (uintptr_t) {n},{'':{max(1, 30 - len(n))}}"
                     f"/* {p}: {c} */")
    L.append("    /* ---- the " + str(DEVICE_ENTRIES) + " device positions, "
             "this part's own ---- */")
    for p, n, c in by_position():
        if n is None:
            L.append(f"    0u,{'':38}/* {CORE_ENTRIES + p}: position {p}, "
                     f"{c} */")
        else:
            h = handler(n)
            pad = max(1, 38 - len(h))
            L.append(f"    (uintptr_t) {h},{'':{pad}}"
                     f"/* {CORE_ENTRIES + p}: position {p}, {c} */")
    L += [
        "};",
        "",
    ]
    return "\n".join(L)


def table_md():
    rows = ["| Position | Vector | Name | What the vendor calls it |",
            "|---|---|---|---|"]
    for p, n, c in by_position():
        rows.append(f"| {p} | {CORE_ENTRIES + p} | "
                    f"{'`' + handler(n) + '`' if n else 'reserved'} | {c} |")
    return "\n".join(rows)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--table", action="store_true")
    a = ap.parse_args(argv)

    bad = problems()
    if bad:
        for b in bad:
            print(f"  {b}")
        return 1

    if a.table:
        print(table_md())
        return 0

    src, hdr = as_source(), as_header()

    if a.check:
        n = 0
        for path, want in ((VECTORS_C, src), (VECTORS_H, hdr)):
            have = path.read_text(encoding="utf-8") if path.exists() else ""
            if have.replace("\r\n", "\n") != want:
                print(f"{path.name} differs from the generator")
                n += 1
        if n:
            print("a hand edit does not survive here; regenerate instead")
            return 1
        print(f"vector table matches the generator ({ENTRIES} entries, "
              f"{len(DEVICE)} named and {len(reserved_positions())} reserved, "
              f"{TABLE_BYTES} bytes aligned to {ALIGN_BYTES}, "
              f"{len(CORROBORATED)} positions corroborated by a second vendor, "
              f"{len(ABSENT_HERE_PRESENT_ON_SIBLING)} absent here and present "
              f"on the sibling)")
        return 0

    VECTORS_C.write_text(src, encoding="utf-8")
    VECTORS_H.write_text(hdr, encoding="utf-8")
    print(f"wrote {VECTORS_C.name} and {VECTORS_H.name}: {ENTRIES} entries, "
          f"{len(DEVICE)} named and {len(reserved_positions())} reserved, "
          f"{TABLE_BYTES} bytes aligned to {ALIGN_BYTES}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
