/* link_probe.c: the smallest thing that makes the linker script answerable.
 *
 * A linker script nothing links against is unverified. This provides the `main`
 * the startup calls, and it deliberately puts one variable in .data and one in
 * .bss, because a script whose .data and .bss sections are both empty says
 * nothing about whether the copy and the zeroing have anywhere to point.
 *
 * `volatile` on both, so -O1 cannot fold them away and leave the sections empty
 * again, which would make this file look like it was doing something.
 *
 * It is not an image anybody flashes. It exists so that `make linkcheck` can
 * produce a real .elf whose first two words can be read back.
 */
#include <stdint.h>

#include "console.h"

/* In .data, because it has a non-zero initialiser. The value is distinctive so
 * that a copy that moved the wrong bytes shows up as this number being absent
 * from the image rather than as a zero that could have come from anywhere. */
volatile uint32_t g_initialised = 0xA5C3F00Du;

/* In .bss, because it has none. */
volatile uint32_t g_zeroed;

int main(void);

/* The console is REACHED from here rather than merely linked beside it. With
 * -ffunction-sections and --gc-sections, a console nothing calls is discarded
 * and the link would succeed while proving nothing about the file that was just
 * added. This never runs: the addresses are the real ones and this image is not
 * flashed anywhere.
 *
 * `[unconfirmed]` The 64 MHz is the HSI this family comes out of reset on, and
 * which clock USART3 actually takes has not been read. That is exactly what
 * console_report exists to print.
 */
static console_t g_console;

int main(void)
{
    g_zeroed = g_initialised;

    if (console_init(&g_console, CONSOLE_RCC_BASE, CONSOLE_GPIOD_BASE,
                     CONSOLE_USART3_BASE, 64000000u, 115200u, 100000u)) {
        (void) console_report(&g_console);
        (void) console_write(&g_console, "link probe: reached main\r\n");
        (void) console_flush(&g_console);
    }

    return (int) (g_zeroed & 0x7Fu);
}
