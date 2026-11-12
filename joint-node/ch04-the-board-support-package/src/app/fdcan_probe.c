/* fdcan_probe.c: which register map this silicon actually has, and whether
 * the bus controller answers at the address three sources agree on.
 *
 * TWO IMAGES FROM THIS ONE FILE, A AND B, differing in exactly one write.
 * Joseph's decision on Thursday 12 November 2026, and the reason is sound: an
 * address whose meaning for this silicon is unsettled must not be written to
 * merely because it would have to be reserved under the competing hypothesis.
 * That argument assumes what the experiment is here to decide, and writing to
 * it would change the state of the very thing under investigation. A control
 * must not move the subject.
 *
 * So the baseline is READ ONLY and identical in both images, by construction:
 * one source file, one `-D`, and a build without that `-D` is a compile error
 * rather than a silent default, because an image whose variant is ambiguous
 * cannot be attributed at all.
 *
 * ---------------------------------------------------------------------------
 * THE CONTRADICTION THIS EXISTS TO SETTLE
 *
 * `stm32h7a3xx.h` disagrees with itself. Parsing its `RCC_TypeDef` and
 * comparing each field's computed offset against the `Address offset:` in that
 * field's OWN comment gives 67 fields, 45 agreeing and 22 differing. The first
 * divergence is `RESERVED10`, declared `[31]` and carrying a comment that is a
 * verbatim copy of `RESERVED9`'s. Every field after it is displaced by 0x60.
 * The same parser on `stm32h743xx.h` gives 65 fields, 65 agreeing, 0 differing.
 *
 *   register     the struct says   ST's own comment says
 *   AHB4ENR      0x140             0x0E0
 *   APB1LENR     0x148             0x0E8
 *   APB1HENR     0x14C             0x0EC
 *
 * `[measured]` The comments are what this volume's console already uses, and it
 * works: `console_init` writes bit 3 at 0x0E0 and bit 18 at 0x0E8, reads both
 * back, then configures GPIOD with read-modify-write plus a disturbance check,
 * and transmits. An unclocked GPIO port does not retain writes and an unclocked
 * USART does not transmit. 5247 bytes arrived on Thursday 12 November 2026.
 *
 * `[inferred]` Against that: Zephyr supports `nucleo_h7a3zi_q` and reaches the
 * RCC through this same struct via its LL layer, so a struct displaced by 0x60
 * ought to make every clock enable on that board fail.
 *
 * Both cannot hold. Continuing to argue is the error this volume exists to
 * avoid, so the part decides.
 *
 * ---------------------------------------------------------------------------
 * THREE THINGS, KEPT APART IN THE OUTPUT
 *
 * Joseph's distinction, and it matters because a set enable bit does not prove
 * the controller's core has a usable clock:
 *
 *   1. THE CLOCK TREE, read only, at offsets that are NOT in dispute. The first
 *      divergence above is at 0x0B4, so every register below that address is
 *      given the same offset by the struct and by the comments. `CR` at 0x000
 *      and `CDCCIP1R` at 0x050 are both in that undisputed range.
 *   2. THE RCC PERIPHERAL ENABLE, `APB1HENR.FDCANEN`, at the disputed offset.
 *      Six addresses are read before and after, and ONE is written.
 *   3. FDCAN REGISTER ACCESS, `CREL` and `ENDN`, read only.
 *
 * ---------------------------------------------------------------------------
 * WHAT EACH OUTCOME MEANS, written down before the run
 *
 * `ENDN` reads 0x87654321: the base address is confirmed on silicon for the
 * first time, and whichever enable this image wrote was sufficient.
 *
 * `ENDN` reads 0x00000000: two live hypotheses, and the report separates them.
 * Either the enable this image wrote is not the one that gates FDCAN, or the
 * controller's core has no clock. `[unconfirmed]` `FDCANSEL` selects that core
 * clock and its reset value's meaning is not sourced here; what IS reported is
 * the raw two bit field together with `HSEON` and `HSERDY`, so a reader can see
 * whether the selected source is even running.
 *
 * `ENDN` reads anything else: the window answers and is not this controller.
 *
 * ---------------------------------------------------------------------------
 * PROVENANCE
 *
 * `[vendor]` `FDCANEN` is bit 8 of `APB1HENR`, from three independent sources:
 * Zephyr's `clocks = <&rcc STM32_CLOCK(APB1_2, 8)>` for `can@4000a000`, ST's
 * `RCC_APB1HENR_FDCANEN_Pos` which is 8, and the generated STM32H7 register
 * documentation Joseph cited.
 *
 * `[vendor]` `FDCANSEL` is bits 29:28 of `CDCCIP1R`, from ST's
 * `RCC_CDCCIP1R_FDCANSEL_Pos` which is 28 with a two bit mask, and Zephyr's
 * `FDCAN_SEL(val)` which names bits 29 and 28 of the same register.
 *
 * `[vendor]` `RCC_CR`: `HSION` bit 0, `HSIRDY` bit 2, `HSIDIV` bits 4:3,
 * `HSEON` bit 16, `HSERDY` bit 17, from ST's own `_Pos` definitions. `HSIDIV`
 * is worth reading for its own sake: ST's `system_stm32h7xx.c` computes the
 * clock as `HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3)`, so this register
 * says whether the oscillator is undivided WITHOUT depending on the terminal.
 * That is a second, independent reading of the figure the first image bounded
 * by legibility alone.
 *
 * `[vendor]` FDCAN1 registers at 0x4000A000 for 0x400 bytes, from Zephyr's
 * `reg = <0x4000a000 0x400>, <0x4000ac00 0x350>` and corroborated in chapter
 * 9's `doc/node-registers.md` by NuttX's memory map and ST's HAL.
 *
 * `[chapter]` `CREL` at 0x000 and `ENDN` at 0x004, and the expected value
 * 0x87654321, from chapter 9's own `tools/gen_initplan.py`, where they are
 * steps 1 and 2 of the configuration plan for exactly this reason: `ENDN` is
 * the only register in the whole sequence whose value the part supplies rather
 * than the software, so it is the only falsifiable check on the base address.
 *
 * NOTHING HERE WRITES TO FDCAN, and nothing here configures the controller.
 * That is chapter 9's plan and it needs a bit timing, a message memory layout
 * and a transceiver. This reads two registers.
 */
#include <stdint.h>

#include "console.h"
#include "stamp.h"

/* ------------------------------------------------------------ the variant
 *
 * Set by the Makefile, never defaulted. An image that cannot say which of the
 * two candidates it wrote is an image whose capture means nothing, and the one
 * NUCLEO-H7A3ZI-Q is shared between volumes, so attribution is not optional.
 */
#if !defined(FP_VARIANT_A) && !defined(FP_VARIANT_B)
/* One line, no continuation: the quote rule in test_console.py sweeps for an
 * odd number of quotes per line, because GNU ld does not join adjacent string
 * literals and a wrapped message once cost a link-time syntax error. C would
 * join them, but the rule is right that the wrap is worth avoiding. */
#error "define FP_VARIANT_A or FP_VARIANT_B: an image must name its one write"
#endif
#if defined(FP_VARIANT_A) && defined(FP_VARIANT_B)
#error "FP_VARIANT_A and FP_VARIANT_B are exclusive: one write, one image"
#endif

#if defined(FP_VARIANT_A)
#define FP_TAG            "jn-fdcanprobe-a"
#define FP_WRITE_OFFSET   0x0ECu
#define FP_WRITE_WHENCE   "ST's own register comments"
#else
#define FP_TAG            "jn-fdcanprobe-b"
#define FP_WRITE_OFFSET   0x14Cu
#define FP_WRITE_WHENCE   "ST's RCC_TypeDef layout"
#endif

/* ------------------------------------------------------------- the addresses */
#define FP_FDCAN1_BASE    0x4000A000u
#define FP_FDCAN_CREL     0x000u
#define FP_FDCAN_ENDN     0x004u
#define FP_ENDN_EXPECTED  0x87654321u

#define FP_RCC_CR         0x000u      /* undisputed */
#define FP_RCC_CDCCIP1R   0x050u      /* undisputed */

/* The six disputed addresses, read before and after the one write. Both
 * candidates for all three enable registers, so the capture shows which
 * triple the part implements rather than only the one this image writes. */
static const uint32_t FP_WATCH[6] = {
    0x0E0u, 0x0E8u, 0x0ECu,           /* comments: AHB4ENR, APB1LENR, APB1HENR */
    0x140u, 0x148u, 0x14Cu,           /* struct:   the same three */
};

#define FP_FDCANEN        (1u << 8)

#define FP_ASSUMED_KERNEL_HZ 64000000u
#define FP_BAUD              115200u
#define FP_SPIN_LIMIT        100000u
#define FP_GAP_SPINS         2000000u

static console_t g_console;

int main(void);

static uint32_t rd(uintptr_t base, uint32_t offset)
{
    return *(volatile uint32_t *) (base + offset);
}

static void line(const char *s)
{
    (void) console_write(&g_console, s);
}

static void hex(uint32_t v)
{
    (void) console_hex32(&g_console, v);
}

static void dec(uint32_t v)
{
    (void) console_dec(&g_console, v);
}

/* `name 0xVALUE`, one field per call, so a capture is greppable by field. */
static void field_hex(const char *name, uint32_t v)
{
    line(" ");
    line(name);
    line(" 0x");
    hex(v);
}

static void field_dec(const char *name, uint32_t v)
{
    line(" ");
    line(name);
    line(" ");
    dec(v);
}

static void watch_line(const char *when)
{
    static const char *const names[6] = {
        "0x0E0", "0x0E8", "0x0EC", "0x140", "0x148", "0x14C",
    };
    line(FP_TAG " enable ");
    line(when);
    for (unsigned i = 0u; i < 6u; i++) {
        field_hex(names[i], rd(CONSOLE_RCC_BASE, FP_WATCH[i]));
    }
    line("\r\n");
    (void) console_flush(&g_console);
}

static void gap(void)
{
    for (volatile uint32_t i = 0u; i < FP_GAP_SPINS; i++) {
    }
}

int main(void)
{
    if (!console_init(&g_console, CONSOLE_RCC_BASE, CONSOLE_GPIOD_BASE,
                      CONSOLE_USART3_BASE, FP_ASSUMED_KERNEL_HZ, FP_BAUD,
                      FP_SPIN_LIMIT)) {
        for (;;) {
        }
    }

    line("\r\n# " FP_TAG ": which register map this silicon has, and whether "
         "the bus answers\r\n");
    line("# ");
    line(build_stamp());
    line("\r\n");
    (void) console_report(&g_console);
    line("# this image writes ONE enable, at offset 0x");
    hex(FP_WRITE_OFFSET);
    line(", on the authority of " FP_WRITE_WHENCE "\r\n");
    line("# it does not write the competing candidate, because that address's "
         "meaning is what is under test\r\n");

    /* --- 1 of 3: the clock tree, read only, undisputed offsets ----------- */
    const uint32_t cr = rd(CONSOLE_RCC_BASE, FP_RCC_CR);
    const uint32_t ccip = rd(CONSOLE_RCC_BASE, FP_RCC_CDCCIP1R);

    line(FP_TAG " clocktree cr");
    field_hex("value", cr);
    field_dec("hsion", (cr >> 0) & 1u);
    field_dec("hsirdy", (cr >> 2) & 1u);
    field_dec("hsidiv", (cr >> 3) & 3u);
    field_dec("hseon", (cr >> 16) & 1u);
    field_dec("hserdy", (cr >> 17) & 1u);
    line("\r\n");

    line(FP_TAG " clocktree cdccip1r");
    field_hex("value", ccip);
    field_dec("fdcansel", (ccip >> 28) & 3u);
    line("\r\n");
    (void) console_flush(&g_console);

    /* --- 2 of 3: the RCC peripheral enable, the disputed offsets --------- */
    watch_line("before");

    /* THE ONE WRITE. Read, modify, write: the register holds other
     * peripherals' enables and this must not clear them. */
    {
        volatile uint32_t *reg =
            (volatile uint32_t *) (CONSOLE_RCC_BASE + FP_WRITE_OFFSET);
        *reg |= FP_FDCANEN;
    }

    watch_line("after");

    /* --- 3 of 3: FDCAN register access, read only ------------------------ */
    const uint32_t crel = rd(FP_FDCAN1_BASE, FP_FDCAN_CREL);
    const uint32_t endn = rd(FP_FDCAN1_BASE, FP_FDCAN_ENDN);

    line(FP_TAG " fdcan1");
    field_hex("crel", crel);
    field_hex("endn", endn);
    field_hex("endn-expected", FP_ENDN_EXPECTED);
    line(endn == FP_ENDN_EXPECTED ? " verdict base-confirmed"
                                  : (endn == 0u ? " verdict reads-zero"
                                                : " verdict answers-wrongly"));
    line("\r\n");
    (void) console_flush(&g_console);

    /* The heartbeat, so the part is visibly alive after the one shot block and
     * so a foreign reflash shows up as the banner reappearing with the count
     * restarting. */
    uint32_t tick = 0u;
    for (;;) {
        tick++;
        line(FP_TAG " tick ");
        dec(tick);
        field_dec("bytes", g_console.bytes);
        field_dec("worst-spins", g_console.worst_spins);
        line(" verdict ");
        line(console_verdict_name(g_console.verdict));
        line("\r\n");
        (void) console_flush(&g_console);
        gap();
    }
}
