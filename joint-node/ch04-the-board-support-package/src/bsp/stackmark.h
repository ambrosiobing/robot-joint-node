/* stackmark.h: paint the unused stack, then read back how deep it went.
 *
 * `[unconfirmed]` THE 8 KB STACK IS A DECISION AND NOT A MEASUREMENT. The
 * linker script says so where it sets `_stack_size`, and names the remedy:
 * a high-water mark read back from a painted stack on the board, which needs a
 * console. The console exists and has been seen to work, so this is that
 * remedy.
 *
 * HOW IT WORKS, and the direction matters at every step. The stack grows DOWN
 * from `_estack` towards `_sstack`, so the deepest point ever reached is the
 * LOWEST address the stack pointer ever held.
 *
 *   1. `stackmark_paint` fills the region from `_sstack` upwards with a
 *      pattern, stopping a margin below the caller's own frame.
 *   2. Later, `stackmark_read` scans UPWARDS from `_sstack` and finds the first
 *      word that is no longer the pattern. Everything below that was never
 *      touched, and that boundary is the high-water mark.
 *
 * WHAT IT CANNOT SEE, and this is not a detail. Whatever the stack used BEFORE
 * the paint is invisible, because the paint starts from the current frame
 * downwards. `stackmark_paint` is therefore called as early as the startup can
 * call it, and `ceiling` reports where it stopped so a reader can see how much
 * was out of reach. The result is a bound on usage below the ceiling, not a
 * measurement of the deepest excursion the image ever made.
 *
 * AND IT CAN BE FOOLED IN THE OPTIMISTIC DIRECTION. A stack word that happens
 * to equal the pattern looks untouched, which pushes the boundary UP, reports
 * more free space and less usage. That is the wrong direction for a safety
 * figure, so two numbers are returned: the contiguous run of pattern words from
 * the bottom, and the TOTAL count of pattern words anywhere in the painted
 * region. If those differ, at least one pattern word sits above the boundary
 * and the boundary is suspect.
 */
#ifndef JOINT_STACKMARK_H
#define JOINT_STACKMARK_H

#include <stdbool.h>
#include <stdint.h>

/* The pattern. Neither half is zero, the two halves differ, and it is not a
 * plausible pointer into any region of this part: DTCM is 0x20000000, flash is
 * 0x08000000, the peripheral space is 0x40000000 and 0x58000000. A pattern that
 * looked like an address would be the likeliest value to appear on a stack by
 * accident, which is the one coincidence that matters here. */
#define STACKMARK_PATTERN 0xBEEF57ACu

/* How far below the caller's own frame the paint stops. Generous on purpose:
 * the cost of being wrong downwards is corrupting the live frame, and the cost
 * of being wrong upwards is a slightly smaller measured region. */
#define STACKMARK_MARGIN  256u

typedef struct {
    uintptr_t sstack;       /* bottom of the region, the lowest address */
    uintptr_t estack;       /* one past the top; the initial stack pointer */
    uintptr_t ceiling;      /* where the paint stopped, 0 if never painted */
    uintptr_t mark;         /* the high-water mark: lowest address ever used */

    uint32_t region_bytes;  /* estack - sstack */
    uint32_t painted_bytes; /* ceiling - sstack */
    uint32_t used_bytes;    /* estack - mark, so usage from the top down */
    uint32_t free_bytes;    /* mark - sstack, below the deepest point */

    uint32_t intact_run;    /* pattern words contiguous from the bottom */
    uint32_t intact_total;  /* pattern words anywhere in the painted region */
    bool     trustworthy;   /* intact_run == intact_total */
} stackmark_t;

/* Fill the unused stack with the pattern, from `_sstack` up to a margin below
 * this call's own frame. Returns the ceiling it reached, or 0 if the region
 * looked implausible and nothing was written.
 *
 * CALL THIS AS EARLY AS POSSIBLE and from the startup rather than from an
 * application, because everything the stack used before it is invisible.
 */
uintptr_t stackmark_paint(void);

/* Read the mark back. False when the paint never ran, so a report cannot
 * silently present an unpainted region as an unused one. */
bool stackmark_read(stackmark_t *out);

#endif /* JOINT_STACKMARK_H */
