/* first_light.c: the first image in this volume that prints.
 *
 * Chapter 4. Everything under it has existed and been checked for some time:
 * the memory map, the linker script, the startup, the 171 entry vector table,
 * the baud divisor and the console. All of it was checked WITHOUT a part, which
 * is most of what this volume can do and is not the same as having run.
 *
 * WHAT THIS IMAGE IS FOR, exactly: to put one readable line through the
 * ST-LINK's virtual COM port. That is a smaller claim than it sounds and a
 * bigger one than anything this volume has made so far, because readable text
 * at 115200 is the first evidence that the assumed clock, the computed divisor,
 * the pin configuration, the startup's copy and zero, and the vector table's
 * placement are all simultaneously right. None of them can be confirmed from
 * the host.
 *
 * `[measured]` AND THE CLOCK IT ASSUMES IS NOW BOUNDED, BY THIS IMAGE. The
 * console is told 64 MHz. Two vendors already agreed that this is the reset
 * state: ST's own system_stm32h7xx.c initialises SystemCoreClock and
 * SystemD2Clock to 64000000 before any configuration runs, and Zephyr's device
 * tree says `hsi-div = <1>` with the comment `HSI RC: 64MHz`. What neither
 * settled is which clock USART3's kernel input actually takes after reset,
 * because that depends on prescalers nothing had read back.
 *
 * READABLE TEXT IS THE MEASUREMENT, and on Thursday 12 November 2026 it was
 * taken: 5247 bytes arrived legibly at 115200 8N1 with no corruption. The
 * divisor in use was 556 with the prescaler at 1, so the actual rate is
 * F_kernel / 556, and the receiver decoded it. Taking a deliberately
 * conservative two per cent receiver tolerance that puts F_kernel between
 * 62.77 and 65.33 MHz; even at the theoretical limit, half a bit time of
 * accumulated error at the stop bit of a ten bit frame, it is between 60.68
 * and 67.42 MHz.
 *
 * SO THE PRESCALER CHAIN IS 1:1 AFTER RESET. 32 MHz and 128 MHz are both
 * excluded by about a factor of two, which is what the question was: whether
 * some divider or multiplier sat between the 64 MHz oscillator and this
 * peripheral. What is NOT established is that the clock is exactly 64.000 MHz,
 * nor anything about how well the internal oscillator is trimmed. A ratio was
 * bounded, not a frequency measured.
 *
 * A console still cannot check its own baud; the terminal is the instrument.
 * That is why console_report is printed FIRST and why the assumed figure is in
 * it: a garbled capture would have been evidence too, because the thing being
 * tested was written down before it was tested.
 *
 * This image configures no clock tree. It deliberately runs at whatever the
 * part comes out of reset on, because the clock configuration is a chapter 4
 * deliverable that is not started, and an image that needs it could not be
 * built yet. Running at reset speed is what makes a first light possible now.
 */
#include <stdint.h>

#include "console.h"
#include "stamp.h"

/* A TAG NO OTHER PROJECT ON THIS BENCH PRINTS. The one NUCLEO-H7A3ZI-Q is
 * shared between volumes, so the image on the part is whatever was flashed
 * last, and the first line of a capture is the check. Two other volumes print
 * `nucleo-h7a3-sampling` and `p13-rollcall`; nothing prints this. */
#define FL_TAG "jn-firstlight"

/* The console is told this and does not measure it. See the header comment. */
#define FL_ASSUMED_KERNEL_HZ 64000000u
#define FL_BAUD              115200u

/* A wait long enough that a bound being hit is a fault rather than a slow
 * transmitter. At 115200 one character is about 87 microseconds, and at the
 * reset clock one spin is a few cycles, so this is several characters' worth
 * of margin and nowhere near a visible hang. */
#define FL_SPIN_LIMIT        100000u

/* Between heartbeat lines. NOT a time: nothing here has configured a timer, so
 * this is a count of iterations and is reported as one. Calling it a delay in
 * milliseconds would be a measurement this image cannot make. */
#define FL_GAP_SPINS         2000000u

static console_t g_console;

int main(void);

static void gap(void)
{
    /* `volatile` so that -O1 cannot see the loop does nothing and remove it,
     * which would turn the heartbeat into an unreadable flood. */
    for (volatile uint32_t i = 0u; i < FL_GAP_SPINS; i++) {
    }
}

static void line(const char *s)
{
    (void) console_write(&g_console, s);
}

int main(void)
{
    if (!console_init(&g_console, CONSOLE_RCC_BASE, CONSOLE_GPIOD_BASE,
                      CONSOLE_USART3_BASE, FL_ASSUMED_KERNEL_HZ, FL_BAUD,
                      FL_SPIN_LIMIT)) {
        /* Nothing can be said, because the thing that would say it is what
         * refused. Stop rather than reset: a part that resets on a fault boots,
         * faults and resets forever, which looks like a part that will not
         * start, and `g_console.verdict` holds the reason for a debugger. */
        for (;;) {
        }
    }

    /* THE BANNER, AND ITS ORDER IS THE ARGUMENT.
     *
     * The report goes first because it names the assumption every later line
     * depends on. The stamp goes with it because the name alone cannot tell a
     * new image from the one the previous session flashed. */
    line("\r\n# " FL_TAG " ch04: the first image in this volume that prints\r\n");
    line("# ");
    line(build_stamp());
    line("\r\n");

    /* NO `# ` PREFIX IN FRONT OF THIS CALL. The first capture, on Thursday
     * 12 November 2026, printed `# console: assumed kernel clock ...` followed
     * by a bare `console: IF THIS LINE IS LEGIBLE ...`, because a prefix
     * written before a call only reaches that call's FIRST line and
     * console_report prints two. Both of its lines already begin with
     * `console: `, which is the field name that identifies them, so there was
     * nothing to add and one line to get wrong. */
    (void) console_report(&g_console);
    /* This line said "the clock above was assumed, never measured" until the
     * capture of Thursday 12 November 2026 made that false. As far as this code
     * knows the figure is still an assumption; what changed is that a reader of
     * the output now has somewhere to look for what it bounds. */
    line("# legible text here IS the measurement: it bounds the clock above to "
         "within a ratio, see doc/console.md\r\n");

    /* Then one line per heartbeat, with field names rather than bare numbers so
     * that a capture found later can be attributed to this image.
     *
     * `tick` is monotonic and starts at 1. A reflash resets the part, so the
     * banner reappears and the counter restarts: a reader that counts banner
     * lines and checks that every tick step is 1 detects a foreign reflash
     * without being told it happened. */
    uint32_t tick = 0u;
    for (;;) {
        tick++;
        line(FL_TAG " tick ");
        (void) console_dec(&g_console, tick);
        line(" bytes ");
        (void) console_dec(&g_console, g_console.bytes);
        line(" worst-spins ");
        (void) console_dec(&g_console, g_console.worst_spins);
        line(" verdict ");
        line(console_verdict_name(g_console.verdict));
        line("\r\n");

        /* Flushed every line, because the interesting capture is the one taken
         * from a part that is then reset or unplugged, and an unflushed
         * transmitter loses whatever was still in the shift register. */
        (void) console_flush(&g_console);

        gap();
    }
}
