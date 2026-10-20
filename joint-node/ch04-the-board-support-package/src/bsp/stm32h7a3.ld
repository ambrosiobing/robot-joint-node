/* stm32h7a3.ld: what goes where, for the NUCLEO-H7A3ZI-Q.
 *
 * The MEMORY block is not here. It is generated into memory.ld from
 * tools/gen_memmap.py and included below, because where the regions are is a
 * fact about the part and what goes in them is a decision about the image. A
 * region size typed by hand links perfectly well and overruns at run time.
 *
 * THE FIRST TWO WORDS OF THE IMAGE ARE NOT A CHOICE. The core fetches the
 * initial stack pointer from the boot address and the reset vector from the
 * four bytes after it. An image whose vector table is anywhere else links, and
 * the part branches to whatever those two words happen to contain. There is an
 * ASSERT below that refuses that, because the symptom is a board that does
 * nothing at all and gives no clue which of twenty things is wrong.
 *
 * NO HEAP. Nothing in this volume allocates: chapter 9's bus layer is
 * arithmetic over caller-provided buffers, and the board accessors hold two
 * pointers. So there is no heap region, and no `end` symbol either, which is
 * what newlib's _sbrk needs, so an object that reaches malloc fails to LINK
 * rather than quietly taking memory from under the stack at run time. The note
 * at the end of this file says why that is enforcement by absence rather than
 * an assertion.
 *
 * `[unconfirmed]` THE STACK SIZE IS A DECISION, NOT A MEASUREMENT. 8 KB is
 * chosen because it is conventional and because this volume's deepest call
 * chain is shallow, not because anything has measured it. The number that would
 * settle it is a high-water mark read back from a painted stack on the board,
 * which needs a console, which is the piece after this one. Until then the
 * figure is a guess with a known method for replacing it.
 */

INCLUDE memory.ld

/* A decision, as above, and the one number in this file that is not read from a
 * vendor source. */
_stack_size = 8K;

ENTRY(Reset_Handler)

SECTIONS
{
    /* The vector table first, so that it lands on the boot address. */
    .isr_vector : ALIGN(512)
    {
        _sisr_vector = .;
        KEEP(*(.isr_vector))
        _eisr_vector = .;
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Unwind tables. Nothing in this volume throws, and -fno-exceptions is not
     * passed to C, so these are kept rather than discarded: a discarded section
     * that something references is a link error at the worst moment, and these
     * cost a few bytes of flash. */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    /* The initialised data, stored in flash and copied to DTCM by the startup.
     * _sidata is where it is stored; _sdata and _edata are where it goes. A
     * startup that confuses the two copies the wrong bytes and every
     * initialised variable comes up holding something else. */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCM

    /* The stack, at the top of DTCM, growing down towards .bss. The guard below
     * is the only thing standing between the two, and it is a build-time check
     * rather than a run-time one: a stack that grows into .bss corrupts
     * variables that are nowhere near the code doing it. */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + _stack_size;
        . = ALIGN(8);
        _estack = .;
    } > DTCM

    /* Kept, so that a map file names what came from where. */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* ---------------------------------------------------------- the refusals */

/* The vector table must be at the boot address. An image that links with it
 * anywhere else is an image the part will not start, and nothing about the
 * build says so. */
ASSERT(_sisr_vector == _boot_address,
       "the vector table is not at the boot address, so the part will fetch its stack pointer and reset vector from whatever is there instead")

/* And the flash region itself must begin at the boot address. The assertion
 * above would still pass if both had been moved together, which is the edit
 * somebody makes when adding a bootloader and is exactly when it matters. */
ASSERT(ORIGIN(FLASH) == _boot_address,
       "the flash region does not begin at the boot address")

/* Everything in DTCM has to fit, stack included. Without this the link
 * succeeds and the stack overlaps .bss. */
ASSERT(_estack <= ORIGIN(DTCM) + LENGTH(DTCM),
       "the stack does not fit in DTCM below the end of the region")

/* And the stack must not have been placed before .bss ends, which would mean
 * the two overlap in the other direction. */
ASSERT(_sstack >= _ebss,
       "the stack begins before .bss ends, so the two overlap")

/* NO HEAP, AND THE ENFORCEMENT IS NOT AN ASSERT. There is no way to assert that
 * a symbol is undefined, so the mechanism is absence: this script defines no
 * `end` symbol, which is what newlib's _sbrk needs, so an object that reaches
 * malloc fails to link with an undefined reference rather than taking memory
 * from under the stack at run time.
 *
 * That is testable in the direction that matters: link something that calls
 * malloc and the link fails. An ASSERT here saying "no heap" while checking
 * something else would read as a guarantee and be none, which is why there
 * isn't one. */
