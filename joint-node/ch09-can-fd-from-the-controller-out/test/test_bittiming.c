/* test_bittiming.c: the C arithmetic against the generated vectors.
 *
 * The vectors come from tools/gen_bt_vectors.py, which is the reference. The
 * two are never compared against each other: both are compared against the
 * vector file, so when they disagree the output says which case and which
 * field rather than leaving somebody to guess which side moved.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written. Continuous integration builds it with
 * -std=c11 -Wall -Wextra -Werror and runs it, which is where it first compiled
 * on Tuesday 6 October 2026. It has still never been near the board.
 */
#include <stdio.h>
#include <string.h>

#include "bittiming.h"
#include "vectors.h"

static int failures;

static void check_u32(const char *label, const char *field,
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
    int refusals = 0;

    for (int i = 0; i < BT_VECTOR_COUNT; i++) {
        const bt_vector_t *v = &BT_VECTORS[i];
        const bt_limits_t *lim = v->data_phase ? &BT_DATA : &BT_NOMINAL;
        const float want_sp = (float) v->want_sp_permille / 1000.0f;

        bt_t got;
        memset(&got, 0, sizeof got);
        const bool ok = bt_compute(v->kernel_hz, v->bitrate, want_sp, lim, &got);

        if (ok != v->ok) {
            printf("  FAIL %s: %s, and the vectors say %s\n", v->label,
                   ok ? "solved" : "refused",
                   v->ok ? "it solves" : "it is refused");
            failures++;
            continue;
        }
        if (!v->ok) {
            refusals++;
            continue;       /* a refusal has no numbers to compare */
        }

        check_u32(v->label, "prescaler", got.prescaler, v->prescaler);
        check_u32(v->label, "seg1", got.seg1, v->seg1);
        check_u32(v->label, "seg2", got.seg2, v->seg2);
        check_u32(v->label, "sjw", got.sjw, v->sjw);
        check_u32(v->label, "sample point", got.sample_point_permille,
                  v->sample_point_permille);

        /* The identity the whole scheme rests on: a bit is one quantum of
         * synchronisation plus the two segments, and nothing else. If this
         * ever fails, the arithmetic has a hole in it rather than an
         * off-by-one. */
        if (1u + got.seg1 + got.seg2 != got.tq_per_bit) {
            printf("  FAIL %s: 1 + %u + %u is not %u quanta\n", v->label,
                   (unsigned) got.seg1, (unsigned) got.seg2,
                   (unsigned) got.tq_per_bit);
            failures++;
        }
    }

    if (failures) {
        printf("test_bittiming: %d failure(s) over %d vectors\n",
               failures, BT_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d vectors, %d of them refusals, both phases\n",
           BT_VECTOR_COUNT, refusals);
    return 0;
}
