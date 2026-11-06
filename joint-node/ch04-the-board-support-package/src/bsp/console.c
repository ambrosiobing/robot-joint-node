/* console.c: the registers, with every write read back and every wait bounded.
 *
 * `[vendor]` Register offsets from NuttX arch/arm/src/stm32h7/hardware/
 * stm32h7x3xx_rcc.h, _gpio.h and _uart.h, read Sunday 25 October 2026 and
 * Friday 6 November 2026. The RCC file is NAMED for the H743; NuttX itself maps
 * this part's family, the H7B3, to it, and Zephyr's device tree gives the same
 * two enable bit numbers independently. It is still not a reading of this
 * part's own manual, which is why both enable bits are read back below.
 */
#include "console.h"

/* ------------------------------------------------------------------ offsets */
#define RCC_AHB4ENR   (0x0E0u / 4u)     /* GPIOD is bit 3 */
#define RCC_APB1LENR  (0x0E8u / 4u)     /* USART3 is bit 18 */
#define RCC_GPIOD_EN  (1u << 3)
#define RCC_USART3_EN (1u << 18)

#define GPIO_MODER    (0x00u / 4u)
#define GPIO_OSPEEDR  (0x08u / 4u)
#define GPIO_PUPDR    (0x0Cu / 4u)
#define GPIO_AFRH     (0x24u / 4u)      /* pins 8 to 15, four bits each */

#define USART_CR1     (0x00u / 4u)
#define USART_CR2     (0x04u / 4u)
#define USART_CR3     (0x08u / 4u)
#define USART_BRR     (0x0Cu / 4u)
#define USART_ISR     (0x1Cu / 4u)
#define USART_ICR     (0x20u / 4u)
#define USART_TDR     (0x28u / 4u)
#define USART_PRESC   (0x2Cu / 4u)

#define CR1_UE        (1u << 0)
#define CR1_TE        (1u << 3)
#define ISR_TC        (1u << 6)
#define ISR_TXE       (1u << 7)

/* Everything the error path or a previous character can leave set, cleared
 * before each operation rather than only after a failure. A flag left set is
 * indistinguishable from the same flag set by the operation about to run. */
#define ICR_CLEAR_ALL ((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) \
                       | (1u << 4) | (1u << 6))

#define MODER_ALTERNATE 0x2u            /* 0b10, two bits per pin */
#define PUPDR_PULL_UP   0x1u            /* 0b01, two bits per pin */

static bool refuse(console_t *c, console_verdict_t why)
{
    if (c != 0) {
        c->verdict = why;
    }
    return false;
}

/* A two-bit field for one pin, as a mask and a value in place. */
static uint32_t two_bit_mask(uint32_t pin) { return 0x3u << (pin * 2u); }
static uint32_t two_bit_val(uint32_t v, uint32_t pin) { return v << (pin * 2u); }

/* A four-bit alternate-function field, for a pin in 8 to 15 within AFRH. */
static uint32_t af_mask(uint32_t pin) { return 0xFu << ((pin - 8u) * 4u); }
static uint32_t af_val(uint32_t v, uint32_t pin)
{
    return v << ((pin - 8u) * 4u);
}

bool console_init(console_t *c, uintptr_t rcc_base, uintptr_t gpio_base,
                  uintptr_t usart_base, uint32_t kernel_hz, uint32_t baud,
                  uint32_t spin_limit)
{
    if (c == 0) {
        return false;
    }
    c->rcc = 0;
    c->gpio = 0;
    c->usart = 0;
    c->kernel_hz = kernel_hz;
    c->spin_limit = spin_limit;
    c->worst_spins = 0u;
    c->bytes = 0u;
    c->verdict = CONSOLE_OK;

    if (rcc_base == 0u || gpio_base == 0u || usart_base == 0u) {
        return refuse(c, CONSOLE_BAD_ARGUMENT);
    }
    if ((rcc_base % 4u) != 0u || (gpio_base % 4u) != 0u
        || (usart_base % 4u) != 0u) {
        return refuse(c, CONSOLE_BAD_ARGUMENT);
    }
    /* A bound of none is the failure this file exists to avoid, and spelling it
     * as zero is too easy a mistake to honour. */
    if (spin_limit == 0u) {
        return refuse(c, CONSOLE_BAD_ARGUMENT);
    }

    /* THE RATE FIRST, BEFORE ANY REGISTER IS TOUCHED. An impossible rate then
     * leaves the part exactly as it was, rather than half configured with its
     * clocks on and a divisor nobody computed. */
    if (!baud_solve(kernel_hz, baud, &c->rate)) {
        return refuse(c, CONSOLE_RATE_REFUSED);
    }

    c->rcc = (volatile uint32_t *) rcc_base;
    c->gpio = (volatile uint32_t *) gpio_base;
    c->usart = (volatile uint32_t *) usart_base;

    /* ---------------------------------------------------- the clocks, read back */
    c->rcc[RCC_AHB4ENR] |= RCC_GPIOD_EN;
    if ((c->rcc[RCC_AHB4ENR] & RCC_GPIOD_EN) == 0u) {
        return refuse(c, CONSOLE_CLOCK_NOT_ENABLED);
    }
    c->rcc[RCC_APB1LENR] |= RCC_USART3_EN;
    if ((c->rcc[RCC_APB1LENR] & RCC_USART3_EN) == 0u) {
        return refuse(c, CONSOLE_CLOCK_NOT_ENABLED);
    }

    /* ------------------------------------------------------------- the pins */

    /* Snapshot the registers shared with every other pin on this port, so the
     * writes below can be checked for having moved only the two they own.
     * Chapter 9's bus is PD0 and PD1 in these same words. */
    const uint32_t moder_before = c->gpio[GPIO_MODER];
    const uint32_t ospeedr_before = c->gpio[GPIO_OSPEEDR];
    const uint32_t pupdr_before = c->gpio[GPIO_PUPDR];

    const uint32_t ours2 = two_bit_mask(CONSOLE_TX_PIN)
                           | two_bit_mask(CONSOLE_RX_PIN);

    /* Read, modify, write. Never an assignment: these words belong to sixteen
     * pins and two of them are somebody else's bus. */
    uint32_t moder = moder_before & ~ours2;
    moder |= two_bit_val(MODER_ALTERNATE, CONSOLE_TX_PIN);
    moder |= two_bit_val(MODER_ALTERNATE, CONSOLE_RX_PIN);
    c->gpio[GPIO_MODER] = moder;

    /* Low speed, deliberately. A 115200 edge needs nothing faster, and the
     * slowest setting is the quietest. */
    c->gpio[GPIO_OSPEEDR] = ospeedr_before & ~ours2;

    /* `[vendor]` The pinctrl file specifies bias-pull-up on PD8 and nothing on
     * PD9, so that is what is applied rather than a symmetric guess. A pull-up
     * on the transmit line holds it at the idle level before the peripheral
     * drives it, which stops a spurious start bit at power on. */
    uint32_t pupdr = pupdr_before & ~ours2;
    pupdr |= two_bit_val(PUPDR_PULL_UP, CONSOLE_TX_PIN);
    c->gpio[GPIO_PUPDR] = pupdr;

    /* AFRH covers pins 8 to 15, so this word cannot reach PD0 or PD1 at all.
     * That is a property of where the split falls rather than care taken here,
     * and the header says so. */
    uint32_t afrh = c->gpio[GPIO_AFRH];
    afrh &= ~(af_mask(CONSOLE_TX_PIN) | af_mask(CONSOLE_RX_PIN));
    afrh |= af_val(CONSOLE_PIN_AF, CONSOLE_TX_PIN);
    afrh |= af_val(CONSOLE_PIN_AF, CONSOLE_RX_PIN);
    c->gpio[GPIO_AFRH] = afrh;

    /* Did the two fields take, and did nothing else move? The second half is
     * the one that matters: a console that works while having quietly put the
     * bus pins back to inputs is the worst outcome available, because chapter
     * 9 would then be debugging a bus that this file broke. */
    if ((c->gpio[GPIO_MODER] & ours2) != (moder & ours2)) {
        return refuse(c, CONSOLE_REGISTER_NOT_TAKEN);
    }
    if ((c->gpio[GPIO_AFRH] & (af_mask(CONSOLE_TX_PIN)
                               | af_mask(CONSOLE_RX_PIN)))
        != (afrh & (af_mask(CONSOLE_TX_PIN) | af_mask(CONSOLE_RX_PIN)))) {
        return refuse(c, CONSOLE_REGISTER_NOT_TAKEN);
    }
    if ((c->gpio[GPIO_MODER] & ~ours2) != (moder_before & ~ours2)
        || (c->gpio[GPIO_OSPEEDR] & ~ours2) != (ospeedr_before & ~ours2)
        || (c->gpio[GPIO_PUPDR] & ~ours2) != (pupdr_before & ~ours2)) {
        return refuse(c, CONSOLE_PINS_DISTURBED);
    }

    /* ---------------------------------------------------------- the USART */

    /* Disabled while it is configured. BRR and PRESC are not writable with any
     * guarantee while the peripheral is enabled, and a divisor that half takes
     * is a rate nobody computed. */
    c->usart[USART_CR1] = 0u;

    /* 8N1: CR1.M1 and CR1.M0 both clear, parity off, CR2.STOP clear. Written
     * rather than left at reset, because an image that relies on a reset value
     * is an image that breaks the first time something runs before it. */
    c->usart[USART_CR2] = 0u;
    c->usart[USART_CR3] = 0u;

    /* The prescaler explicitly, from the same table the solve chose out of.
     * Its reset value happens to be divide by one, which is what makes leaving
     * it alone work by luck rather than by decision. */
    c->usart[USART_PRESC] = c->rate.presc_index;
    c->usart[USART_BRR] = c->rate.brr;

    if (c->usart[USART_BRR] != c->rate.brr
        || c->usart[USART_PRESC] != c->rate.presc_index) {
        return refuse(c, CONSOLE_REGISTER_NOT_TAKEN);
    }

    /* Every flag a previous life of this peripheral could have left set,
     * cleared before the transmitter is enabled. */
    c->usart[USART_ICR] = ICR_CLEAR_ALL;

    c->usart[USART_CR1] = CR1_TE | CR1_UE;
    if ((c->usart[USART_CR1] & (CR1_TE | CR1_UE)) != (CR1_TE | CR1_UE)) {
        return refuse(c, CONSOLE_REGISTER_NOT_TAKEN);
    }

    return true;
}

bool console_put(console_t *c, char ch)
{
    if (c == 0 || c->usart == 0 || c->spin_limit == 0u) {
        return refuse(c, CONSOLE_BAD_ARGUMENT);
    }

    /* CLEARED BEFORE THE BYTE, NOT AFTER. TC is left set by the previous
     * character, so a flush that waits for it would return at once, before
     * this byte had gone anywhere. The error flags go too: one left set is
     * indistinguishable from one this operation caused. */
    c->usart[USART_ICR] = ICR_CLEAR_ALL;

    /* TXE is a state and not an event: it reports that the transmit register
     * is free, so it cannot be stale. It can, however, never arrive, which is
     * what the bound is for. */
    uint32_t spins = 0u;
    while ((c->usart[USART_ISR] & ISR_TXE) == 0u) {
        spins++;
        if (spins >= c->spin_limit) {
            if (spins > c->worst_spins) {
                c->worst_spins = spins;
            }
            return refuse(c, CONSOLE_TX_TIMEOUT);
        }
    }
    if (spins > c->worst_spins) {
        c->worst_spins = spins;
    }

    c->usart[USART_TDR] = (uint32_t) (unsigned char) ch;
    c->bytes++;
    return true;
}

bool console_write(console_t *c, const char *s)
{
    if (c == 0 || s == 0) {
        return refuse(c, CONSOLE_BAD_ARGUMENT);
    }
    for (const char *p = s; *p != '\0'; p++) {
        if (!console_put(c, *p)) {
            return false;
        }
    }
    return true;
}

bool console_hex32(console_t *c, uint32_t v)
{
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4) {
        if (!console_put(c, digits[(v >> (unsigned) shift) & 0xFu])) {
            return false;
        }
    }
    return true;
}

bool console_dec(console_t *c, uint32_t v)
{
    /* Ten digits is the most a 32 bit value needs, and the buffer is on the
     * stack because there is no allocator and there is not going to be one. */
    char buf[10];
    uint32_t n = 0u;

    if (v == 0u) {
        return console_put(c, '0');
    }
    while (v != 0u && n < sizeof buf) {
        buf[n++] = (char) ('0' + (v % 10u));
        v /= 10u;
    }
    while (n != 0u) {
        if (!console_put(c, buf[--n])) {
            return false;
        }
    }
    return true;
}

bool console_flush(console_t *c)
{
    if (c == 0 || c->usart == 0 || c->spin_limit == 0u) {
        return refuse(c, CONSOLE_BAD_ARGUMENT);
    }
    /* TC is cleared by every put before its byte, so waiting for it here waits
     * for the LAST byte rather than reading a flag the first byte set. */
    uint32_t spins = 0u;
    while ((c->usart[USART_ISR] & ISR_TC) == 0u) {
        spins++;
        if (spins >= c->spin_limit) {
            if (spins > c->worst_spins) {
                c->worst_spins = spins;
            }
            return refuse(c, CONSOLE_TX_TIMEOUT);
        }
    }
    if (spins > c->worst_spins) {
        c->worst_spins = spins;
    }
    return true;
}

bool console_report(console_t *c)
{
    if (c == 0) {
        return false;
    }
    /* Deliberately dull and deliberately first. Every number a reader needs in
     * order to decide whether the clock assumption was right, printed by the
     * thing whose correctness depends on it. */
    bool ok = true;
    ok = ok && console_write(c, "console: assumed kernel clock ");
    ok = ok && console_dec(c, c->kernel_hz);
    ok = ok && console_write(c, " Hz, presc ");
    ok = ok && console_dec(c, c->rate.presc);
    ok = ok && console_write(c, ", usartdiv ");
    ok = ok && console_dec(c, c->rate.usartdiv);
    ok = ok && console_write(c, ", BRR 0x");
    ok = ok && console_hex32(c, c->rate.brr);
    ok = ok && console_write(c, ", rate ");
    ok = ok && console_dec(c, c->rate.actual_baud);
    ok = ok && console_write(c, ", error ");
    ok = ok && console_dec(c, c->rate.error_permille);
    ok = ok && console_write(c, " per thousand\r\n");
    ok = ok && console_write(c, "console: IF THIS LINE IS LEGIBLE the assumed "
                                "clock is right to within the error above\r\n");
    return ok;
}

const char *console_verdict_name(console_verdict_t v)
{
    switch (v) {
    case CONSOLE_OK:                 return "ok";
    case CONSOLE_BAD_ARGUMENT:       return "bad-argument";
    case CONSOLE_RATE_REFUSED:       return "rate-refused";
    case CONSOLE_CLOCK_NOT_ENABLED:  return "clock-not-enabled";
    case CONSOLE_REGISTER_NOT_TAKEN: return "register-not-taken";
    case CONSOLE_PINS_DISTURBED:     return "pins-disturbed";
    case CONSOLE_TX_TIMEOUT:         return "tx-timeout";
    default:                         return "unknown";
    }
}
