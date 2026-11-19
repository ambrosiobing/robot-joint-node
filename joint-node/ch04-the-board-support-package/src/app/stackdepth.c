/* stackdepth.c: what the 8 KB stack actually uses, and a prediction to test it.
 *
 * `[unconfirmed]` The linker script says, where it sets `_stack_size`, that
 * 8 KB is a decision and not a measurement, and names the remedy: a high-water
 * mark read back from a painted stack on the board, which needs a console. The
 * console exists and has been seen to work, so this is that remedy.
 *
 * ---------------------------------------------------------------------------
 * A READING IS NOT AN EXPERIMENT
 *
 * An image that paints the stack and prints one number produces a figure
 * nothing can contradict. If the painting were broken, or the scan ran in the
 * wrong direction, or the pattern were being written somewhere else entirely,
 * the output would still be a plausible number of bytes.
 *
 * So this image reads the mark TWICE with a known consumption between, and
 * states the prediction before the result:
 *
 *   mark A        after the console is up
 *   consume       a function with a 1024 byte volatile local array, touched
 *   mark B        again
 *   the claim     depth(B) >= 1024
 *
 * THE CLAIM WAS A DIFFERENCE AND IS NOW ABSOLUTE, corrected after the capture
 * of Thursday 19 November 2026 reported METHOD SUSPECT at 900 against 1024.
 * The method was right and the statistic was wrong: mark A was CLAMPED at the
 * paint ceiling, because the stack had never gone that deep, so used(A) was the
 * ceiling's depth rather than the stack's. Differencing a clamped figure
 * against a real one underestimates the movement by up to the invisible band,
 * which was 268 bytes, and 900 plus 268 is 1168. The absolute depth carries no
 * such term.
 *
 * `[inferred]` The inequality rather than an equality, because the frame also
 * holds the loop counter, the saved registers and whatever alignment the
 * compiler chooses, so the true figure is 1024 plus an unknown small amount.
 * A result BELOW 1024 means the method is wrong, not that the stack is
 * shallow. A result of exactly zero means the paint or the scan is not doing
 * anything at all, and the verdict says so in those words.
 *
 * ---------------------------------------------------------------------------
 * WHAT THE NUMBER IS AND IS NOT
 *
 * It is a bound on usage below the paint ceiling, which the report prints. It
 * is NOT the deepest excursion this image ever made: whatever the stack used
 * before `stackmark_paint` ran is invisible, which is why the startup calls it
 * as early as it can.
 *
 * And it is a bound for THIS image on THIS run. A different image with a deeper
 * call chain will report a different figure, which is the point of printing it
 * from every image rather than deciding `_stack_size` from one.
 */
#include <stdint.h>

#include "console.h"
#include "stackmark.h"
#include "stamp.h"

#define SD_TAG "jn-stackdepth"

/* The prediction's magnitude. A named constant because the whole claim is
 * stated in terms of it, and a literal buried in an array declaration is not
 * findable by somebody asking what this image predicted. */
#define SD_CONSUME_BYTES 1024u
#define SD_CONSUME_WORDS (SD_CONSUME_BYTES / 4u)

#define SD_ASSUMED_KERNEL_HZ 64000000u
#define SD_BAUD              115200u
#define SD_SPIN_LIMIT        100000u
#define SD_GAP_SPINS         2000000u

static console_t g_console;

int main(void);

static void line(const char *s)
{
    (void) console_write(&g_console, s);
}

static void field(const char *name, uint32_t v)
{
    line(" ");
    line(name);
    line(" ");
    (void) console_dec(&g_console, v);
}

static void field_hex(const char *name, uint32_t v)
{
    line(" ");
    line(name);
    line(" 0x");
    (void) console_hex32(&g_console, v);
}

static void report(const char *which, const stackmark_t *m)
{
    line(SD_TAG " ");
    line(which);
    field_hex("mark", (uint32_t) m->mark);
    field("used", m->used_bytes);
    field("free", m->free_bytes);
    field("run", m->intact_run);
    field("total", m->intact_total);
    line(m->trustworthy ? " trustworthy yes" : " trustworthy NO");
    /* CLAMPED means the figures above are the ceiling's and not the stack's,
     * and a reader who takes `used` as a measurement here is reading a bound. */
    line(m->clamped ? " clamped YES, used is the ceiling depth and true usage "
                      "is at most that"
                    : " clamped no, used is the stack's own boundary");
    line("\r\n");
    (void) console_flush(&g_console);
}

/* THE CONSUMER. `volatile` on the array so that -O1 cannot see that nothing
 * reads it afterwards and remove it, which would leave the prediction testing
 * an array that was never allocated. Touched in a loop for the same reason, and
 * the sum is returned so the loop itself cannot be dropped either. */
static uint32_t consume(void)
{
    volatile uint32_t deep[SD_CONSUME_WORDS];
    uint32_t sum = 0u;

    for (uint32_t i = 0u; i < SD_CONSUME_WORDS; i++) {
        deep[i] = i;
    }
    for (uint32_t i = 0u; i < SD_CONSUME_WORDS; i++) {
        sum += deep[i];
    }
    return sum;
}

static void gap(void)
{
    for (volatile uint32_t i = 0u; i < SD_GAP_SPINS; i++) {
    }
}

int main(void)
{
    if (!console_init(&g_console, CONSOLE_RCC_BASE, CONSOLE_GPIOD_BASE,
                      CONSOLE_USART3_BASE, SD_ASSUMED_KERNEL_HZ, SD_BAUD,
                      SD_SPIN_LIMIT)) {
        for (;;) {
        }
    }

    line("\r\n# " SD_TAG ": what the 8 KB stack uses, with a prediction to test "
         "the method\r\n");
    line("# ");
    line(build_stamp());
    line("\r\n");
    (void) console_report(&g_console);

    stackmark_t a;
    if (!stackmark_read(&a)) {
        line("# " SD_TAG " the stack was never painted, so there is nothing to "
             "read. the startup must call stackmark_paint AFTER zeroing .bss, "
             "because the zeroing clears the ceiling\r\n");
        (void) console_flush(&g_console);
        for (;;) {
        }
    }

    line(SD_TAG " region");
    field_hex("sstack", (uint32_t) a.sstack);
    field_hex("estack", (uint32_t) a.estack);
    field_hex("ceiling", (uint32_t) a.ceiling);
    field("region-bytes", a.region_bytes);
    field("painted-bytes", a.painted_bytes);
    field("above-ceiling-invisible", a.region_bytes - a.painted_bytes);
    line("\r\n");
    (void) console_flush(&g_console);

    report("mark-a", &a);

    /* THE PREDICTION IS ON THE ABSOLUTE DEPTH, not on a difference.
     *
     * The first version of this image predicted used(B) minus used(A) and the
     * capture of Thursday 19 November 2026 reported METHOD SUSPECT at 900
     * against 1024. The method was right and the statistic was wrong: mark A
     * was CLAMPED at the ceiling, so used(A) was the ceiling's depth rather
     * than the stack's, and differencing a clamped figure against a real one
     * underestimates the movement by up to the invisible band.
     *
     * The absolute depth has no such term. consume() holds a 1024 byte array,
     * so the deepest point must lie at least that far below the top whatever
     * happened before it. */
    line(SD_TAG " predict depth-at-b at-least ");
    (void) console_dec(&g_console, SD_CONSUME_BYTES);
    line(" because consume() holds that many bytes of volatile locals, and the "
         "movement only as an interval because mark-a may be clamped\r\n");
    (void) console_flush(&g_console);

    const uint32_t sum = consume();

    stackmark_t b;
    (void) stackmark_read(&b);
    report("mark-b", &b);

    /* THE ONE LINE THIS IMAGE EXISTS FOR, and it tests the absolute depth. */
    const uint32_t moved_low = (b.used_bytes > a.used_bytes)
                               ? (b.used_bytes - a.used_bytes) : 0u;
    const uint32_t invisible = a.region_bytes - a.painted_bytes;
    const uint32_t moved_high = a.clamped ? moved_low + invisible : moved_low;

    line(SD_TAG " verdict depth-at-b");
    field("bytes", b.used_bytes);
    field("predicted-at-least", SD_CONSUME_BYTES);
    field("moved-at-least", moved_low);
    field("moved-at-most", moved_high);
    if (b.clamped) {
        line(" METHOD BROKEN, the stack never entered the painted region even "
             "after allocating the array\r\n");
    } else if (b.used_bytes < SD_CONSUME_BYTES) {
        line(" METHOD SUSPECT, the deepest point is shallower than the array "
             "it allocated\r\n");
    } else {
        line(" method holds, so depth-at-b is the stack's own figure\r\n");
    }
    (void) console_flush(&g_console);

    /* AND THE FIGURE THE LINKER SCRIPT ASKED FOR, which is mark-a's and not
     * mark-b's: mark-b includes a 1024 byte array that exists only to test the
     * method. If mark-a was clamped, this image's real usage is at most the
     * invisible band and no narrower statement is available from one run. */
    line(SD_TAG " real-usage");
    if (a.clamped) {
        line(" at-most");
        field("bytes", invisible);
        line(" of");
        field("region", a.region_bytes);
        line(" because the stack never reached the paint ceiling\r\n");
    } else {
        line(" exactly");
        field("bytes", a.used_bytes);
        line(" of");
        field("region", a.region_bytes);
        line("\r\n");
    }
    (void) console_flush(&g_console);

    /* The sum is printed so that nothing in consume() can be discarded as
     * unused, and because a wrong sum would mean the array was not what it
     * claimed to be. `[arithmetic]` The words hold 0 to 255, so the sum is
     * 255*256/2 = 32640. */
    line(SD_TAG " consume-sum");
    field("got", sum);
    field("expected", (SD_CONSUME_WORDS - 1u) * SD_CONSUME_WORDS / 2u);
    line(sum == (SD_CONSUME_WORDS - 1u) * SD_CONSUME_WORDS / 2u
         ? " agree\r\n" : " DISAGREE, the array is not what it claims\r\n");
    (void) console_flush(&g_console);

    uint32_t tick = 0u;
    for (;;) {
        tick++;
        line(SD_TAG " tick ");
        (void) console_dec(&g_console, tick);
        field("bytes", g_console.bytes);
        line("\r\n");
        (void) console_flush(&g_console);
        gap();
    }
}
