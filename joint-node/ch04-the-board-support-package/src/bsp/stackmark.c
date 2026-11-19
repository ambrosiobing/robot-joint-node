/* stackmark.c: the paint and the read back.
 *
 * Every direction in this file is the opposite of the intuitive one, because
 * the stack grows down. Lower address means deeper. The high-water mark is a
 * MINIMUM address, free space is BELOW it, and used space is ABOVE it.
 */
#include "stackmark.h"

/* From the linker script's `.stack (NOLOAD)` section. Addresses, not values. */
extern uint32_t _sstack;
extern uint32_t _estack;

/* Where the paint stopped. In .bss, so it is zero before the startup zeroes it
 * and zero after, which is why 0 means "never painted" and is a safe default:
 * a report cannot mistake an unpainted region for an unused one.
 *
 * `volatile` because the paint writes it and a later call in another
 * translation unit reads it, and -O1 has no reason to keep them in order
 * otherwise. */
static volatile uintptr_t g_ceiling;

uintptr_t stackmark_paint(void)
{
    const uintptr_t bottom = (uintptr_t) &_sstack;
    const uintptr_t top = (uintptr_t) &_estack;

    /* THE CALLER'S OWN FRAME, approximated by the address of a local. This is
     * `[inferred]` rather than exact: the compiler may place locals anywhere in
     * the frame, so this is a point INSIDE the frame rather than its lowest
     * address. The margin below it is what makes that approximation safe.
     *
     * Being called from a deep caller does NOT make this wrong, which an
     * earlier draft of this comment claimed: the ceiling is relative to the
     * CURRENT frame, so a deep call paints a smaller region and still never
     * touches the live frame. What a deep call costs is coverage, reported as
     * `painted_bytes`, not correctness. */
    uint32_t here = 0u;
    const uintptr_t frame = (uintptr_t) &here;

    /* Refuse rather than write, on anything that does not look like the
     * region the script describes. A paint that ran against a wrong pair of
     * symbols would corrupt whatever is actually there, and the symptom would
     * appear somewhere else entirely. */
    if (top <= bottom) {
        return 0u;              /* the region is empty or inverted */
    }
    if ((bottom % 4u) != 0u || (top % 4u) != 0u) {
        return 0u;              /* not word aligned, so the walk would not end */
    }
    if (frame <= bottom || frame > top) {
        return 0u;              /* this call is not running on that stack */
    }
    if (frame - bottom <= STACKMARK_MARGIN) {
        return 0u;              /* nothing to paint below the margin */
    }

    const uintptr_t ceiling = (frame - STACKMARK_MARGIN) & ~(uintptr_t) 3u;

    for (volatile uint32_t *p = (volatile uint32_t *) bottom;
         (uintptr_t) p < ceiling; p++) {
        *p = STACKMARK_PATTERN;
    }

    g_ceiling = ceiling;
    return ceiling;
}

bool stackmark_read(stackmark_t *out)
{
    if (out == 0) {
        return false;
    }
    const uintptr_t ceiling = g_ceiling;
    if (ceiling == 0u) {
        return false;           /* never painted; say so rather than guess */
    }

    const uintptr_t bottom = (uintptr_t) &_sstack;
    const uintptr_t top = (uintptr_t) &_estack;

    /* THE CONTIGUOUS RUN FROM THE BOTTOM is the measurement. The first word
     * that is no longer the pattern is the deepest the stack ever reached. */
    uint32_t run = 0u;
    const volatile uint32_t *p = (const volatile uint32_t *) bottom;
    while ((uintptr_t) p < ceiling && *p == STACKMARK_PATTERN) {
        run++;
        p++;
    }

    /* AND THE TOTAL, which is the check on it. A touched word that happens to
     * hold the pattern looks untouched and pushes the boundary up, reporting
     * more free space than there is. If every pattern word is in the run, no
     * such coincidence occurred inside the painted region. */
    uint32_t total = 0u;
    for (const volatile uint32_t *q = (const volatile uint32_t *) bottom;
         (uintptr_t) q < ceiling; q++) {
        if (*q == STACKMARK_PATTERN) {
            total++;
        }
    }

    out->sstack = bottom;
    out->estack = top;
    out->ceiling = ceiling;
    out->mark = bottom + 4u * run;

    out->region_bytes = (uint32_t) (top - bottom);
    out->painted_bytes = (uint32_t) (ceiling - bottom);
    out->used_bytes = (uint32_t) (top - out->mark);
    out->free_bytes = (uint32_t) (out->mark - bottom);

    out->intact_run = run;
    out->intact_total = total;
    out->trustworthy = (run == total);

    /* THE CLAMP. Every painted word still holds the pattern, so the stack never
     * entered the painted region and `mark` above is the ceiling rather than
     * anything the stack did. `used_bytes` is then the ceiling's depth and a
     * caller must read it as an upper bound on nothing: true usage is somewhere
     * in [0, region_bytes - painted_bytes]. */
    out->clamped = (run == out->painted_bytes / 4u);
    return true;
}
