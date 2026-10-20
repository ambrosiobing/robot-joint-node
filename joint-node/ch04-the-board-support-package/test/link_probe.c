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

/* In .data, because it has a non-zero initialiser. The value is distinctive so
 * that a copy that moved the wrong bytes shows up as this number being absent
 * from the image rather than as a zero that could have come from anywhere. */
volatile uint32_t g_initialised = 0xA5C3F00Du;

/* In .bss, because it has none. */
volatile uint32_t g_zeroed;

int main(void);

int main(void)
{
    g_zeroed = g_initialised;
    return (int) (g_zeroed & 0x7Fu);
}
