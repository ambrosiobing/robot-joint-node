/* addrmap.c: read first, write once, and name the address before touching it.
 *
 * Thursday 12 November 2026, after the two register map probes. It exists to
 * close a gap of mine and to test a mechanism the previous capture suggested
 * without establishing.
 *
 * ---------------------------------------------------------------------------
 * THE GAP, WHICH IS MINE
 *
 * `fdcan_probe.c` reads `CREL` and `ENDN` only AFTER writing the clock enable.
 * It therefore shows that FDCAN answers once the bit is set and NOT that the
 * bit is what made it answer. The enable may have been unnecessary all along.
 * Two reads in the right place would have settled it and I did not put them
 * there.
 *
 * So here `CREL` and `ENDN` are read BEFORE anything is written, and read again
 * after, and the report states whether the answer changed.
 *
 * ---------------------------------------------------------------------------
 * THE MECHANISM, WHICH IS NOT ESTABLISHED
 *
 * `[measured]` Three pairs of RCC offsets exactly 0x60 apart behaved as one
 * register each: 0x0E0 and 0x140, 0x0E8 and 0x148, 0x0EC and 0x14C. That is a
 * fact about three pairs. One observation is not a mechanism, and a 0x60
 * periodic mirror of the whole block is only one of the readings consistent
 * with it.
 *
 * So four more pairs are read, chosen so that each tells something different:
 *
 *   0x000 and 0x060   the sharpest, because CR's value is already known from
 *                     the previous capture, 0x0000C025, so a mirror here is
 *                     unmistakable and so is its absence
 *   0x050 and 0x0B0   CDCCIP1R read 0x00000000 and CKGAENR is at 0x0B0 by both
 *                     halves of the header, so this pair distinguishes a mirror
 *                     from two registers that merely both read zero
 *   0x0B0 and 0x110   the same pair shifted, to see whether the mirror is
 *                     periodic rather than a single fold
 *   0x14C and 0x1AC   one step beyond the known pair: if the mirror repeats,
 *                     the enable bit written below appears here too
 *
 * `[measured]` 0x0E0 and 0x140 are read as well, as the KNOWN POSITIVE. A
 * capture in which the established pair stops mirroring is a capture to
 * distrust before reading anything else in it.
 *
 * ---------------------------------------------------------------------------
 * EVERY READ IS ANNOUNCED BEFORE IT HAPPENS, AND THAT IS NOT A STYLE CHOICE
 *
 * `[inferred]` All offsets read here lie inside base plus 0x3FF, which is the
 * one kilobyte slot this peripheral is allocated, so a read is expected to
 * return zero rather than to fault. That is an inference and not a source.
 *
 * If it is wrong, an unimplemented address raises a bus error, which on this
 * core escalates to a hard fault because the configurable faults are disabled
 * out of reset. The handler would then stop the part, and the capture would
 * simply END. A reader would see an incomplete list and not know which read
 * did it, because a line assembled in memory and printed at the end prints
 * nothing when the fault happens halfway through assembling it.
 *
 * So each offset is printed AND FLUSHED before the read that follows it, which
 * costs output and makes the last line of a truncated capture the culprit.
 *
 * AND THE FAULT HANDLERS SAY SO THEMSELVES. `HardFault_Handler` and
 * `BusFault_Handler` are overridden here, which is the first use in this volume
 * of the weak aliases the generated vector table was built to allow: a strong
 * definition under the vendor's own name wins. They print the last announced
 * offset and stop.
 *
 * `[unconfirmed]` Printing from a fault handler is not generally sound: the
 * console's state could be the thing that is broken. It is acceptable here
 * because the only fault this probe can plausibly raise is a bus error on a
 * read, which leaves the console untouched, and because a silent stop would
 * waste the whole capture. A fault from any other cause makes the printed line
 * evidence about where, not about what.
 *
 * ---------------------------------------------------------------------------
 * ONE WRITE, AND WHY THIS OFFSET
 *
 * `[measured]` The previous capture showed both candidates reaching the same
 * register, so the choice no longer carries the question it used to. 0x0EC is
 * used because the console already uses 0x0E0 and 0x0E8, and one convention
 * inside the volume is worth more than a coin toss.
 */
#include <stdint.h>

#include "console.h"
#include "stamp.h"

#define AM_TAG "jn-addrmap"

#define AM_FDCAN1_BASE   0x4000A000u
#define AM_FDCAN_CREL    0x000u
#define AM_FDCAN_ENDN    0x004u
#define AM_ENDN_EXPECTED 0x87654321u

#define AM_WRITE_OFFSET  0x0ECu
#define AM_FDCANEN       (1u << 8)

/* The widest offset read here. Everything must stay inside the peripheral's
 * one kilobyte slot, and this is checked by test_addrmap.py rather than
 * trusted, because a single digit here is the difference between a read that
 * returns zero and a read that stops the part. */
#define AM_MAX_OFFSET    0x3FFu

#define AM_ASSUMED_KERNEL_HZ 64000000u
#define AM_BAUD              115200u
#define AM_SPIN_LIMIT        100000u
#define AM_GAP_SPINS         2000000u

static console_t g_console;

/* The last offset ANNOUNCED, so a fault handler can name it. `volatile`
 * because the handler reads what the fault interrupted. */
static volatile uint32_t g_announced = 0xFFFFFFFFu;

int main(void);

static void line(const char *s)
{
    (void) console_write(&g_console, s);
}

static void hex(uint32_t v)
{
    (void) console_hex32(&g_console, v);
}

/* Announce, flush, then read. The flush is the point: it puts the offset on the
 * wire BEFORE the access that might not return. */
static uint32_t announced_read(uintptr_t base, uint32_t offset)
{
    g_announced = offset;
    line(" 0x");
    hex(offset);
    line("=");
    (void) console_flush(&g_console);

    const uint32_t v = *(volatile uint32_t *) (base + offset);

    line("0x");
    hex(v);
    (void) console_flush(&g_console);
    return v;
}

/* THE FIRST STRONG OVERRIDE OF A WEAK VECTOR IN THIS VOLUME. The generated
 * table declares every handler weak precisely so a chapter can do this. */
void HardFault_Handler(void);
void HardFault_Handler(void)
{
    line("\r\n" AM_TAG " FAULT hard, last announced offset 0x");
    hex(g_announced);
    line("\r\n" AM_TAG " the read above did not return, so that offset is not "
         "readable on this part\r\n");
    (void) console_flush(&g_console);
    for (;;) {
    }
}

void BusFault_Handler(void);
void BusFault_Handler(void)
{
    line("\r\n" AM_TAG " FAULT bus, last announced offset 0x");
    hex(g_announced);
    line("\r\n");
    (void) console_flush(&g_console);
    for (;;) {
    }
}

/* The pairs, as pairs, so the report cannot accidentally compare the wrong two
 * values. `why` is printed, because a table of numbers whose purpose lives only
 * in a comment is a table nobody can act on from a capture. */
typedef struct {
    uint32_t a;
    uint32_t b;
    const char *why;
} am_pair_t;

static const am_pair_t AM_PAIRS[5] = {
    { 0x0E0u, 0x140u, "known-positive, mirrored in the previous capture" },
    { 0x000u, 0x060u, "CR is 0x0000C025, so a mirror here is unmistakable" },
    { 0x050u, 0x0B0u, "CDCCIP1R read zero; CKGAENR at 0x0B0 is not zero" },
    { 0x0B0u, 0x110u, "the same pair shifted, to test periodicity" },
    { 0x14Cu, 0x1ACu, "one step past the known pair" },
};

static void gap(void)
{
    for (volatile uint32_t i = 0u; i < AM_GAP_SPINS; i++) {
    }
}

int main(void)
{
    if (!console_init(&g_console, CONSOLE_RCC_BASE, CONSOLE_GPIOD_BASE,
                      CONSOLE_USART3_BASE, AM_ASSUMED_KERNEL_HZ, AM_BAUD,
                      AM_SPIN_LIMIT)) {
        for (;;) {
        }
    }

    line("\r\n# " AM_TAG ": read first, write once, and name each address "
         "before touching it\r\n");
    line("# ");
    line(build_stamp());
    line("\r\n");
    (void) console_report(&g_console);
    line("# phase 1 writes NOTHING. every offset is printed and flushed before "
         "the read, so a truncated capture names the read that stopped it\r\n");

    /* --- phase 1a: FDCAN BEFORE any enable. This is the gap being closed. -- */
    line(AM_TAG " fdcan-before");
    const uint32_t crel_before = announced_read(AM_FDCAN1_BASE, AM_FDCAN_CREL);
    const uint32_t endn_before = announced_read(AM_FDCAN1_BASE, AM_FDCAN_ENDN);
    line(endn_before == AM_ENDN_EXPECTED ? " verdict answers-before-enable"
                                         : " verdict silent-before-enable");
    line("\r\n");

    /* --- phase 1b: the pairs, read only ---------------------------------- */
    for (unsigned i = 0u; i < 5u; i++) {
        line(AM_TAG " pair");
        const uint32_t a = announced_read(CONSOLE_RCC_BASE, AM_PAIRS[i].a);
        const uint32_t b = announced_read(CONSOLE_RCC_BASE, AM_PAIRS[i].b);
        line(a == b ? " same" : " differ");
        line(" why ");
        line(AM_PAIRS[i].why);
        line("\r\n");
        (void) console_flush(&g_console);
    }

    /* --- phase 2: THE ONE WRITE ------------------------------------------ */
    line(AM_TAG " write offset 0x");
    hex(AM_WRITE_OFFSET);
    line(" bit 8\r\n");
    (void) console_flush(&g_console);
    {
        volatile uint32_t *reg =
            (volatile uint32_t *) (CONSOLE_RCC_BASE + AM_WRITE_OFFSET);
        *reg |= AM_FDCANEN;
    }

    /* --- phase 3: the same pairs again, and FDCAN again ------------------ */
    for (unsigned i = 0u; i < 5u; i++) {
        line(AM_TAG " pair-after");
        const uint32_t a = announced_read(CONSOLE_RCC_BASE, AM_PAIRS[i].a);
        const uint32_t b = announced_read(CONSOLE_RCC_BASE, AM_PAIRS[i].b);
        line(a == b ? " same" : " differ");
        line("\r\n");
        (void) console_flush(&g_console);
    }

    line(AM_TAG " fdcan-after");
    const uint32_t crel_after = announced_read(AM_FDCAN1_BASE, AM_FDCAN_CREL);
    const uint32_t endn_after = announced_read(AM_FDCAN1_BASE, AM_FDCAN_ENDN);
    line("\r\n");

    /* --- the one sentence the whole image exists for --------------------- */
    line(AM_TAG " enable-necessary ");
    if (endn_before == AM_ENDN_EXPECTED && endn_after == AM_ENDN_EXPECTED) {
        line("no, the controller answered before the enable was written");
    } else if (endn_before != AM_ENDN_EXPECTED
               && endn_after == AM_ENDN_EXPECTED) {
        line("yes, it was silent before and answers after");
    } else if (endn_after != AM_ENDN_EXPECTED) {
        line("unknown, it does not answer even after the enable");
    } else {
        line("unknown");
    }
    line("\r\n");

    line(AM_TAG " crel-stable ");
    line(crel_before == crel_after ? "yes" : "no");
    line("\r\n");
    (void) console_flush(&g_console);

    uint32_t tick = 0u;
    for (;;) {
        tick++;
        line(AM_TAG " tick ");
        (void) console_dec(&g_console, tick);
        line(" bytes ");
        (void) console_dec(&g_console, g_console.bytes);
        line(" verdict ");
        line(console_verdict_name(g_console.verdict));
        line("\r\n");
        (void) console_flush(&g_console);
        gap();
    }
}
