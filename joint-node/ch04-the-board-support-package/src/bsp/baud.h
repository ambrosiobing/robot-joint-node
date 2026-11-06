/* baud.h: the console's divisor, as arithmetic with no register in sight.
 *
 * Chapter 4's image floor. Separate from console.c for the same reason chapter
 * 9 keeps bittiming.c separate from its board layer: the arithmetic can then be
 * checked against the generator's vectors on a host, and the only thing left
 * needing a board is whether the registers answer.
 *
 * It names no vendor header and does not need one. Nothing here knows what a
 * register is.
 *
 * `[kernel]` The arithmetic is the mainline Linux driver's, from
 * drivers/tty/serial/stm32-usart.c and stm32-usart.h.
 * Read on Sunday 25 October 2026. tools/gen_baud.py is the reference and
 * carries the citation in full; this is the same arithmetic in C.
 *
 * EVERY REFUSAL IS EXPRESSIBLE THROUGH THIS SIGNATURE, which is deliberate. The
 * generator refuses five different ways and each one has to be reachable here,
 * or the parity test would be asserting something this side cannot answer and
 * the mismatch would read as a defect in the code rather than in the contract.
 * So the return is a bool, the reason comes back through the struct, and the
 * five reasons are enumerated rather than left as one failure.
 */
#ifndef JOINT_BAUD_H
#define JOINT_BAUD_H

#include <stdbool.h>
#include <stdint.h>

/* `[kernel]` stm32_usart_presc_val, and USART_PRESC_MAX is 0b1011 which is the
 * last index of it. */
#define BAUD_PRESC_COUNT 12u

/* `[kernel]` Below 16 the driver switches to 8 times oversampling. This chapter
 * uses 16 times only, so this is a floor and not a preference. */
#define BAUD_USARTDIV_MIN 16u
#define BAUD_BRR_MAX      0xFFFFu

/* `[arithmetic]` A chosen limit, not a reading. The reasoning is in
 * tools/gen_baud.py: ten bit times, half a bit of margin, halved again because
 * both ends contribute. */
#define BAUD_ERROR_LIMIT_PERMILLE 25u

/* Why a solve was refused. Each one is reachable, and test_baud.py has a rule
 * that says so, because a refusal nothing reaches is a claim rather than a
 * behaviour. */
typedef enum {
    BAUD_OK = 0,
    BAUD_REFUSED_NOT_A_RATE,      /* a clock or a baud of zero */
    BAUD_REFUSED_DIVISOR_TOO_SMALL, /* under 16, which needs OVER8 */
    BAUD_REFUSED_ERROR_TOO_BIG,   /* past the limit above */
    BAUD_REFUSED_NO_PRESCALER,    /* past 16 bits with every prescaler */
    BAUD_REFUSED_NO_OUTPUT,       /* the caller passed no struct */
} baud_verdict_t;

typedef struct {
    uint32_t presc_index;
    uint32_t presc;
    uint32_t usartdiv;
    uint32_t brr;               /* what goes in the register */
    uint32_t actual_baud;       /* what the part will really produce */
    uint32_t error_permille;    /* rounded up, so a borderline case reads high */
    baud_verdict_t verdict;
} baud_t;

/* The prescaler values, in index order. Exposed so the console can write the
 * PRESC register from the same table the solve chose out of. */
extern const uint32_t BAUD_PRESC_VAL[BAUD_PRESC_COUNT];

/* Rounded to NEAREST, which is what the driver does. Truncating instead is a
 * silent error source: at 64 MHz and 115200 it is the difference between 556
 * and 555, and between 0.8 and 1.9 per thousand. Returns false on a zero
 * divisor rather than faulting. */
bool baud_round_closest(uint32_t n, uint32_t d, uint32_t *out);

/* The register word for a divisor. At 16 times oversampling this IS the
 * divisor, because the mantissa shift is four; the split only does work at 8
 * times, which this chapter does not use. */
uint32_t baud_brr_from_div(uint32_t usartdiv);

/* The smallest prescaler that lands a legal divisor. True only when `out`
 * carries a usable answer; on false `out->verdict` says which refusal it was,
 * so a console can print the reason rather than going quiet. */
bool baud_solve(uint32_t fck_hz, uint32_t baud, baud_t *out);

/* The refusal as a short stable token, for a console line and for a test to
 * assert on. "ok" when there was none. */
const char *baud_verdict_name(baud_verdict_t v);

#endif /* JOINT_BAUD_H */
