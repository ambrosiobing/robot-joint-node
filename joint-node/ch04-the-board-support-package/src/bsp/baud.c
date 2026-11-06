/* baud.c: the same arithmetic as tools/gen_baud.py, in C.
 *
 * Neither is compared against the other. Both are compared against
 * test/vectors_baud.json and test/vectors_baud.h, so a disagreement names the
 * rate rather than leaving somebody to guess which side moved.
 */
#include "baud.h"

const uint32_t BAUD_PRESC_VAL[BAUD_PRESC_COUNT] = {
    1u, 2u, 4u, 6u, 8u, 10u, 12u, 16u, 32u, 64u, 128u, 256u
};

bool baud_round_closest(uint32_t n, uint32_t d, uint32_t *out)
{
    if (out == 0 || d == 0u) {
        return false;
    }
    /* (n + d/2) / d, and the addition cannot overflow for any clock this part
     * has: the largest n here is a few hundred million and d/2 is at most half
     * of that again. A 64 bit intermediate would be free to add and would hide
     * the reasoning, so the reasoning is written instead. */
    *out = (n + (d / 2u)) / d;
    return true;
}

uint32_t baud_brr_from_div(uint32_t usartdiv)
{
    return ((usartdiv / 16u) << 4u) | (usartdiv % 16u);
}

static void refuse(baud_t *out, baud_verdict_t why)
{
    if (out == 0) {
        return;
    }
    out->presc_index = 0u;
    out->presc = 0u;
    out->usartdiv = 0u;
    out->brr = 0u;
    out->actual_baud = 0u;
    out->error_permille = 0u;
    out->verdict = why;
}

bool baud_solve(uint32_t fck_hz, uint32_t baud, baud_t *out)
{
    if (out == 0) {
        /* Nothing to report through, so the only honest thing is to say no.
         * The enumerator exists so a caller that passed a struct and got this
         * verdict knows it is a programming error rather than a rate. */
        return false;
    }
    if (fck_hz == 0u || baud == 0u) {
        refuse(out, BAUD_REFUSED_NOT_A_RATE);
        return false;
    }

    for (uint32_t index = 0u; index < BAUD_PRESC_COUNT; index++) {
        const uint32_t presc = BAUD_PRESC_VAL[index];

        uint32_t pres_clk = 0u;
        if (!baud_round_closest(fck_hz, presc, &pres_clk)) {
            refuse(out, BAUD_REFUSED_NOT_A_RATE);
            return false;
        }
        uint32_t usartdiv = 0u;
        if (!baud_round_closest(pres_clk, baud, &usartdiv)) {
            refuse(out, BAUD_REFUSED_NOT_A_RATE);
            return false;
        }

        if (usartdiv < BAUD_USARTDIV_MIN) {
            /* A larger prescaler makes it smaller still, so there is nothing
             * further to try. This would need 8 times oversampling, which this
             * chapter does not enter. */
            refuse(out, BAUD_REFUSED_DIVISOR_TOO_SMALL);
            return false;
        }

        const uint32_t brr = baud_brr_from_div(usartdiv);
        if (brr > BAUD_BRR_MAX) {
            continue;       /* a larger prescaler may bring it into range */
        }

        uint32_t actual = 0u;
        if (!baud_round_closest(pres_clk, usartdiv, &actual)) {
            refuse(out, BAUD_REFUSED_NOT_A_RATE);
            return false;
        }

        /* The error in parts per thousand, ROUNDED UP, so a case sitting on the
         * limit reads as over it rather than under. The generator reports a
         * fraction; this reports a whole number and rounds away from accepting,
         * because the two must never disagree about which side of the limit a
         * rate falls on. */
        const uint32_t diff = (actual > baud) ? (actual - baud)
                                              : (baud - actual);
        /* The multiply below would overflow 32 bits for a difference past
         * about 4.29 million. The reachable difference is at most a divisor's
         * worth of the rate, so this cannot happen for any clock this part
         * has; it is CHECKED rather than argued because an argument in a
         * comment is not a guard, and a 64 bit intermediate is not free here:
         * -nostdlib means no __aeabi_uldivmod to divide one with. */
        if (diff > (0xFFFFFFFFu / 1000u)) {
            refuse(out, BAUD_REFUSED_ERROR_TOO_BIG);
            out->actual_baud = actual;
            return false;
        }
        const uint32_t err = ((diff * 1000u) + (baud - 1u)) / baud;
        if (err > BAUD_ERROR_LIMIT_PERMILLE) {
            refuse(out, BAUD_REFUSED_ERROR_TOO_BIG);
            out->actual_baud = actual;      /* kept, so a console can print it */
            return false;
        }

        out->presc_index = index;
        out->presc = presc;
        out->usartdiv = usartdiv;
        out->brr = brr;
        out->actual_baud = actual;
        out->error_permille = err;
        out->verdict = BAUD_OK;
        return true;
    }

    refuse(out, BAUD_REFUSED_NO_PRESCALER);
    return false;
}

const char *baud_verdict_name(baud_verdict_t v)
{
    switch (v) {
    case BAUD_OK:                       return "ok";
    case BAUD_REFUSED_NOT_A_RATE:       return "not-a-rate";
    case BAUD_REFUSED_DIVISOR_TOO_SMALL: return "divisor-under-16";
    case BAUD_REFUSED_ERROR_TOO_BIG:    return "error-past-limit";
    case BAUD_REFUSED_NO_PRESCALER:     return "no-prescaler-fits";
    case BAUD_REFUSED_NO_OUTPUT:        return "no-output-struct";
    default:                            return "unknown";
    }
}
