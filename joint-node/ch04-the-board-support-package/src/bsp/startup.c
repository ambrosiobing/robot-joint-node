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
 * `[vendor]` THE TABLE ITSELF IS NOT IN THIS FILE ANY MORE. It moved to the
 * generated `vectors.c` on Monday 9 November 2026, when the device positions
 * were read from a source: all 171 entries, the 16 core exceptions and the 155
 * device positions, from ST's own `IRQn_Type` enum and corroborated against
 * Zephyr's device tree. See `tools/gen_vectors.py` and `doc/interrupts.md`.
 *
 * Until then this file carried the 16 core vectors and nothing else, which was
 * correct exactly as long as nothing enabled a device interrupt and would have
 * stopped being correct the moment something did.
 *
 * WHAT STAYS HERE is the reset, the handlers the core exceptions name, and the
 * default they all alias. Those are hand written because each one is a decision
 * rather than a row: the infinite loop rather than a reset, the copy before the
 * zero, and the order of the two.
 */
#include <stdint.h>

/* From the linker script. Addresses, not values: every one of these is a
 * symbol the script placed, and taking the address is the only correct way to
 * read it. */
/* _estack is declared in vectors.c, which is the file that needs it: the
 * initial stack pointer is entry 0 of the table and this file no longer holds
 * the table. */
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

/* The table used to be here. It is in vectors.c now, generated, because 155
 * of its 171 entries are a list of positions rather than a list of decisions
 * and 21 of those positions are reserved holes that a hand written table gets
 * wrong by closing. `Default_Handler_alias` below is what every one of its
 * device entries aliases to, which is why it is not static. */

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
