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

        /* The register word, and then the same four numbers read back out of
         * it. A solved timing that cannot be written is not a solved timing,
         * and a word that does not unpack to what went in is an off-by-one in
         * a shift that staring at the arithmetic would never find. */
        uint32_t word = 0u;
        const bool packed = v->data_phase ? bt_pack_dbtp(&got, false, &word)
                                          : bt_pack_nbtp(&got, &word);
        if (!packed) {
            printf("  FAIL %s: solved, but will not fit its register\n",
                   v->label);
            failures++;
        } else {
            check_u32(v->label, "register word", word, v->register_word);

            bt_t back;
            memset(&back, 0, sizeof back);
            if (v->data_phase) {
                bt_unpack_dbtp(word, &back);
            } else {
                bt_unpack_nbtp(word, &back);
            }
            check_u32(v->label, "prescaler read back", back.prescaler,
                      got.prescaler);
            check_u32(v->label, "seg1 read back", back.seg1, got.seg1);
            check_u32(v->label, "seg2 read back", back.seg2, got.seg2);
            check_u32(v->label, "sjw read back", back.sjw, got.sjw);
            check_u32(v->label, "quanta read back", back.tq_per_bit,
                      got.tq_per_bit);
            check_u32(v->label, "sample point read back",
                      back.sample_point_permille, got.sample_point_permille);
        }
    }

    /* The packing has its own refusal path, and it is the one that matters
     * most, because the failure it prevents is silent. A nominal timing at
     * 80 MHz has a segment 1 of 127; the data field is five bits wide and
     * would truncate it to 31, configuring a bit rate nobody chose with no
     * error reported anywhere. */
    {
        bt_t wide;
        memset(&wide, 0, sizeof wide);
        uint32_t word = 0xDEADBEEFu;
        if (!bt_compute(80000000u, 500000u, 0.80f, &BT_NOMINAL, &wide)) {
            printf("  FAIL the 80 MHz nominal design point did not solve\n");
            failures++;
        } else if (bt_pack_dbtp(&wide, false, &word)) {
            printf("  FAIL a nominal timing packed as a data word was "
                   "accepted, and truncating it silently is the failure this "
                   "guard exists for\n");
            failures++;
        } else if (word != 0xDEADBEEFu) {
            printf("  FAIL a refused pack wrote to its output anyway\n");
            failures++;
        }

        /* And the other direction is legal, because a data timing is small.
         * Without this, the guard above might be refusing everything. */
        bt_t narrow;
        memset(&narrow, 0, sizeof narrow);
        if (!bt_compute(80000000u, 2000000u, 0.75f, &BT_DATA, &narrow)) {
            printf("  FAIL the 80 MHz data design point did not solve\n");
            failures++;
        } else {
            uint32_t plain = 0u, with_tdc = 0u;
            if (!bt_pack_nbtp(&narrow, &word)) {
                printf("  FAIL a data timing refused by the nominal packer, "
                       "which has wider fields\n");
                failures++;
            }
            /* The delay compensation bit is the only thing in DBTP that is not
             * a timing, so setting it must change exactly one bit. */
            if (bt_pack_dbtp(&narrow, false, &plain)
                && bt_pack_dbtp(&narrow, true, &with_tdc)
                && (plain ^ with_tdc) != BT_DBTP_TDC) {
                printf("  FAIL the TDC flag changed 0x%08X rather than only "
                       "0x%08X\n", (unsigned) (plain ^ with_tdc),
                       (unsigned) BT_DBTP_TDC);
                failures++;
            }
        }
    }

    if (failures) {
        printf("test_bittiming: %d failure(s) over %d vectors\n",
               failures, BT_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d vectors, %d of them refusals, both phases, "
           "every word packed and read back\n",
           BT_VECTOR_COUNT, refusals);
    return 0;
}
