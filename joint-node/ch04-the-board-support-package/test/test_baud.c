/* test_baud.c: the C divisor against the generated vectors.
 *
 * The generator is the reference. Neither side is compared against the other:
 * both are compared against test/vectors_baud.h, so a disagreement names the
 * rate rather than leaving somebody to guess which side moved.
 *
 * EVERY REFUSAL IN THE ORACLE IS REACHABLE THROUGH THIS SIGNATURE, which was
 * checked rather than assumed. The generator refuses five ways; baud_solve
 * returns false with an enumerated verdict for each, so no case in the vectors
 * asks this side a question it cannot answer.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written.
 */
#include <stdio.h>
#include <string.h>

#include "baud.h"
#include "vectors_baud.h"

static int failures;

static void check(const char *label, const char *field,
                  uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("  FAIL %s: %s is %u, the vectors say %u\n",
               label, field, (unsigned) got, (unsigned) want);
        failures++;
    }
}

int main(void)
{
    int solved = 0, refused = 0;

    for (int i = 0; i < BV_VECTOR_COUNT; i++) {
        const bv_vector_t *v = &BV_VECTORS[i];

        baud_t r;
        memset(&r, 0, sizeof r);
        const bool ok = baud_solve(v->fck_hz, v->baud, &r);

        if (ok != v->ok) {
            printf("  FAIL %s: solved is %s and the vectors say %s (verdict "
                   "%s)\n", v->label, ok ? "true" : "false",
                   v->ok ? "true" : "false", baud_verdict_name(r.verdict));
            failures++;
            continue;
        }
        if (!ok) {
            refused++;
            /* A refusal has to carry a reason. A false with BAUD_OK in it
             * would be a function that failed without saying so. */
            if (r.verdict == BAUD_OK) {
                printf("  FAIL %s: refused with a verdict of ok\n", v->label);
                failures++;
            }
            continue;
        }
        solved++;

        if (r.verdict != BAUD_OK) {
            printf("  FAIL %s: solved with verdict %s\n", v->label,
                   baud_verdict_name(r.verdict));
            failures++;
        }
        check(v->label, "prescaler index", r.presc_index, v->presc_index);
        check(v->label, "usartdiv", r.usartdiv, v->usartdiv);
        check(v->label, "BRR", r.brr, v->brr);
        check(v->label, "achieved baud", r.actual_baud, v->actual_baud);

        /* The prescaler index has to name the value that was divided by, or
         * the console writes one prescaler to PRESC and computed with
         * another. */
        if (r.presc_index >= BAUD_PRESC_COUNT) {
            printf("  FAIL %s: prescaler index %u is past the table\n",
                   v->label, (unsigned) r.presc_index);
            failures++;
        } else if (r.presc != BAUD_PRESC_VAL[r.presc_index]) {
            printf("  FAIL %s: prescaler %u is not entry %u of the table, "
                   "which is %u\n", v->label, (unsigned) r.presc,
                   (unsigned) r.presc_index,
                   (unsigned) BAUD_PRESC_VAL[r.presc_index]);
            failures++;
        }

        /* And the achieved rate recomputed from the REGISTER WORD rather than
         * carried along, because a chain that reports the rate it meant to
         * produce would agree with itself. */
        uint32_t pres_clk = 0u, from_brr = 0u;
        if (!baud_round_closest(v->fck_hz, r.presc, &pres_clk)
            || !baud_round_closest(pres_clk, r.brr, &from_brr)) {
            printf("  FAIL %s: the register word could not be turned back into "
                   "a rate\n", v->label);
            failures++;
        } else if (from_brr != r.actual_baud) {
            printf("  FAIL %s: the register word gives %u and the solve says "
                   "%u\n", v->label, (unsigned) from_brr,
                   (unsigned) r.actual_baud);
            failures++;
        }
    }

    /* --------------------------------------------------------- the invariants */

    /* 1. At 16 times oversampling the register word IS the divisor. */
    for (uint32_t d = BAUD_USARTDIV_MIN; d <= 4096u; d++) {
        if (baud_brr_from_div(d) != d) {
            printf("  FAIL a divisor of %u composes to %u, and at 16 times "
                   "oversampling the two are the same number\n",
                   (unsigned) d, (unsigned) baud_brr_from_div(d));
            failures++;
            break;
        }
    }

    /* 2. Rounded to nearest, and the choice has to matter somewhere. */
    {
        uint32_t got = 0u;
        if (!baud_round_closest(64000000u, 115200u, &got) || got != 556u) {
            printf("  FAIL 64 MHz over 115200 rounds to 556 and this gave "
                   "%u\n", (unsigned) got);
            failures++;
        }
        if (64000000u / 115200u == 556u) {
            printf("  FAIL truncation gives the same answer here, so the "
                   "rounding rule is untested on this case\n");
            failures++;
        }
        if (baud_round_closest(1u, 0u, &got)) {
            printf("  FAIL a divisor of zero was accepted\n");
            failures++;
        }
    }

    /* 3. Every refusal verdict is distinct and named. Two enumerators sharing
     * a token would make a console line ambiguous about which happened. */
    {
        const baud_verdict_t all[] = {
            BAUD_OK, BAUD_REFUSED_NOT_A_RATE, BAUD_REFUSED_DIVISOR_TOO_SMALL,
            BAUD_REFUSED_ERROR_TOO_BIG, BAUD_REFUSED_NO_PRESCALER,
            BAUD_REFUSED_NO_OUTPUT,
        };
        const int n = (int) (sizeof all / sizeof all[0]);
        for (int a = 0; a < n; a++) {
            if (strcmp(baud_verdict_name(all[a]), "unknown") == 0) {
                printf("  FAIL verdict %d has no name\n", (int) all[a]);
                failures++;
            }
            for (int b = a + 1; b < n; b++) {
                if (strcmp(baud_verdict_name(all[a]),
                           baud_verdict_name(all[b])) == 0) {
                    printf("  FAIL verdicts %d and %d share the name %s\n",
                           (int) all[a], (int) all[b],
                           baud_verdict_name(all[a]));
                    failures++;
                }
            }
        }
    }

    /* 4. A null output is refused rather than written through. */
    if (baud_solve(64000000u, 115200u, 0)) {
        printf("  FAIL a null output struct was accepted\n");
        failures++;
    }

    /* 5. Every accepted rate anywhere in a sweep obeys all three limits. */
    unsigned swept = 0u;
    for (uint32_t fck = 1000000u; fck <= 200000000u; fck += 1000000u) {
        const uint32_t rates[] = { 9600u, 115200u, 1000000u };
        for (unsigned k = 0u; k < 3u; k++) {
            baud_t r;
            memset(&r, 0, sizeof r);
            swept++;
            if (!baud_solve(fck, rates[k], &r)) {
                continue;
            }
            if (r.usartdiv < BAUD_USARTDIV_MIN || r.brr > BAUD_BRR_MAX
                || r.error_permille > BAUD_ERROR_LIMIT_PERMILLE
                || r.brr != r.usartdiv) {
                printf("  FAIL %u from %u Hz was accepted with divisor %u, "
                       "BRR 0x%X and error %u\n", (unsigned) rates[k],
                       (unsigned) fck, (unsigned) r.usartdiv,
                       (unsigned) r.brr, (unsigned) r.error_permille);
                failures++;
            }
        }
    }

    if (failures) {
        printf("test_baud: %d failure(s) over %d rates\n",
               failures, BV_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d rates, %d solved and %d refused, %u swept, the register "
           "word is the divisor throughout\n",
           BV_VECTOR_COUNT, solved, refused, swept);
    return 0;
}
