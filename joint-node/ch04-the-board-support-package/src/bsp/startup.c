/* startup.c: the first two words, and getting from reset to main.
 *
 * Chapter 4's image floor, second piece. The linker script says where things
 * go; this is what the part runs when it is switched on.
 *
 * FOUR THINGS HERE FAIL WITHOUT A SYMPTOM, and three of them produce a board
 * that does nothing at all, which is the least informative failure available.
 *
 *   - THE INITIAL STACK POINTER IS _estack, NOT _sstack. The stack grows DOWN,
 *     so the pointer starts at the END of the region. Putting _sstack in the
 *     first word points it at the bottom, and the first push underflows out of
 *     the region into .bss, corrupting variables that have nothing to do with
 *     the code running. The two symbols differ by one character and both look
 *     like the right answer.
 *
 *   - THE SOURCE OF THE .data COPY IS _sidata, NOT _sdata. _sidata is where the
 *     initialised values are STORED, in flash; _sdata is where they GO, in
 *     DTCM. Copying _sdata to _sdata is a loop that does nothing and leaves
 *     every initialised variable holding whatever the RAM came up with, which
 *     on this part is not zero.
 *
 *   - NO LIBRARY FUNCTION MAY BE CALLED BEFORE THE COPY AND THE ZEROING ARE
 *     DONE. memcpy and memset are the obvious way to write both loops and they
 *     are the wrong way: a library whose own state lives in .data or .bss is
 *     being asked to run before its state exists. The loops below are written
 *     out for that reason and not for speed.
 *
 *   - THE TABLE MUST SURVIVE THE LINKER. It is referenced by nothing, so
 *     --gc-sections discards it unless the script KEEPs it, and an image
 *     without a vector table links and reports a sensible size. The script does
 *     KEEP it; `used` here stops the compiler discarding it first.
 *
 * `[unconfirmed]` ONLY THE SIXTEEN CORE VECTORS ARE HERE. The device interrupt
 * table for this part is not in this file, because its positions are a list of
 * numbers that has to be read from a checked source and has not been. Nothing
 * in this volume enables a device interrupt yet, so the table being short is
 * harmless TODAY and stops being harmless the moment one is enabled: the core
 * fetches the handler from past the end of the table and branches to whatever
 * is in the next words of flash. Before any interrupt is enabled, the table
 * needs its device entries from a source, one per row, checked.
 */
#include <stdint.h>

/* From the linker script. Addresses, not values: every one of these is a
 * symbol the script placed, and taking the address is the only correct way to
 * read it. */
extern uint32_t _estack;        /* the END of the stack region */
extern uint32_t _sidata;        /* where .data is STORED, in flash */
extern uint32_t _sdata;         /* where .data GOES, in DTCM */
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
int main(void);

/* A handler that stops. It is deliberately an infinite loop rather than a
 * reset: a part that resets on a fault boots, faults and resets forever, which
 * looks like a part that will not start, and the information about which fault
 * it was is gone. Stopped, a debugger can still be attached and the stacked
 * registers read. Once chapter 4 has a console and a region that survives a
 * reset, this is where the fault record goes. */
static void Default_Handler(void)
{
    for (;;) {
    }
}

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler_alias")));
void HardFault_Handler(void)
    __attribute__((weak, alias("Default_Handler_alias")));
void MemManage_Handler(void)
    __attribute__((weak, alias("Default_Handler_alias")));
void BusFault_Handler(void)
    __attribute__((weak, alias("Default_Handler_alias")));
void UsageFault_Handler(void)
    __attribute__((weak, alias("Default_Handler_alias")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler_alias")));
void DebugMon_Handler(void)
    __attribute__((weak, alias("Default_Handler_alias")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler_alias")));
void SysTick_Handler(void)
    __attribute__((weak, alias("Default_Handler_alias")));

/* An alias target has to have external linkage, so the static one above is
 * wrapped. Both exist so that the handler itself can stay static and the
 * aliases can still be overridden by a later chapter defining its own. */
void Default_Handler_alias(void);
void Default_Handler_alias(void)
{
    Default_Handler();
}

/* The table. Sixteen core entries, as uintptr_t rather than function pointers
 * so that the first one, which is an address and not a function, needs no cast
 * between pointer kinds.
 *
 * `used` stops the compiler discarding it, and the script's KEEP stops the
 * linker doing so. Both are needed: they are different passes. */
__attribute__((section(".isr_vector"), used))
const uintptr_t g_isr_vector[] = {
    (uintptr_t) &_estack,               /* 0: the END of the stack */
    (uintptr_t) Reset_Handler,          /* 1 */
    (uintptr_t) NMI_Handler,            /* 2 */
    (uintptr_t) HardFault_Handler,      /* 3 */
    (uintptr_t) MemManage_Handler,      /* 4 */
    (uintptr_t) BusFault_Handler,       /* 5 */
    (uintptr_t) UsageFault_Handler,     /* 6 */
    0u,                                 /* 7, reserved */
    0u,                                 /* 8, reserved */
    0u,                                 /* 9, reserved */
    0u,                                 /* 10, reserved */
    (uintptr_t) SVC_Handler,            /* 11 */
    (uintptr_t) DebugMon_Handler,       /* 12 */
    0u,                                 /* 13, reserved */
    (uintptr_t) PendSV_Handler,         /* 14 */
    (uintptr_t) SysTick_Handler,        /* 15 */
    /* The device interrupts begin at 16 and are not here. See the note at the
     * top: nothing enables one yet, and the list has to be read rather than
     * recalled before anything does. */
};

void Reset_Handler(void)
{
    /* The copy, FROM _sidata in flash TO _sdata in DTCM. Written out rather
     * than handed to memcpy, because memcpy's own state would be in the memory
     * being set up. */
    const uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* And the zeroing, for the same reason. */
    for (uint32_t *p = &_sbss; p < &_ebss; p++) {
        *p = 0u;
    }

    (void) main();

    /* main returning is not an error here and not success either: there is
     * nothing to return to. Stopping is the only honest thing, and stopping
     * where a debugger can see it beats resetting into the same place. */
    for (;;) {
    }
}
