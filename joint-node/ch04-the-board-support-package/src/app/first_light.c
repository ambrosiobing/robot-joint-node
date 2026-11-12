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
 * `[unconfirmed]` AND IT ASSUMES A CLOCK IT CANNOT MEASURE. The console is told
 * 64 MHz. Two vendors agree that this is the reset state: ST's own
 * system_stm32h7xx.c initialises SystemCoreClock and SystemD2Clock to 64000000
 * before any configuration runs, and Zephyr's device tree for this family says
 * `hsi-div = <1>` with the comment `HSI RC: 64MHz, hsi_clk = 64MHz`. What
 * neither settles is which clock USART3's kernel input actually takes after
 * reset, because that depends on prescalers nothing here has read back.
 *
 * SO READABLE TEXT IS THE MEASUREMENT. A console cannot check its own baud; the
 * terminal is the instrument. If the lines below arrive legibly then the
 * assumption printed in them was close enough, and if they arrive as rubbish
 * then the assumption is wrong and the report line names what was assumed. That
 * is why console_report is printed FIRST and why the assumed figure is in it:
 * a garbled capture is still evidence, provided the thing being tested was
 * written down before it was tested.
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
    line("\r\n# ");
    (void) console_report(&g_console);
    line("# this text being legible IS the test: the clock above was assumed, "
         "never measured\r\n");

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
