/* bittiming.c: the whole arithmetic, in one place, with no table of magic.
 *
 * The limits below are the widths of the register fields that hold these
 * numbers, read from the field definitions rather than from a summary of them.
 * A field of n bits holds 0 to 2^n - 1 and the hardware adds one, so the nine
 * bit nominal prescaler field means a prescaler of 1 to 512, and the five bit
 * data prescaler field means 1 to 32.
 *
 * The minimum quanta per bit is not a register limit. The registers allow three
 * quanta, one of synchronisation and one of each segment, and three quanta is
 * legal and useless: the sample point can only land on one of a handful of
 * positions, so it cannot be put where the design wants it, and the jump width
 * has almost nothing to work with. Eight is this volume's floor, chosen rather
 * than quoted, because the standard has not been read here. It is written as a
 * field so it can be lowered by anyone who has read it and disagrees.
 */
#include "bittiming.h"

#define BT_QUALITY_FLOOR_TQ 8u

const bt_limits_t BT_NOMINAL = {
    .name = "nominal",
    .prescaler_min = 1u, .prescaler_max = 512u,
    .seg1_min = 1u, .seg1_max = 256u,
    .seg2_min = 1u, .seg2_max = 128u,
    .sjw_max = 128u,
    .min_tq = BT_QUALITY_FLOOR_TQ,
};

const bt_limits_t BT_DATA = {
    .name = "data",
    .prescaler_min = 1u, .prescaler_max = 32u,
    .seg1_min = 1u, .seg1_max = 32u,
    .seg2_min = 1u, .seg2_max = 16u,
    .sjw_max = 16u,
    .min_tq = BT_QUALITY_FLOOR_TQ,
};

static uint32_t min_u32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

/* Round half away from zero on a non negative value, without pulling in the
 * maths library: the node is small and this is the only rounding it needs. */
static uint32_t round_u32(float v)
{
    return (uint32_t) (v + 0.5f);
}

bool bt_compute(uint32_t kernel_hz, uint32_t bitrate, float want_sp,
                const bt_limits_t *lim, bt_t *out)
{
    if (lim == 0 || out == 0 || kernel_hz == 0u || bitrate == 0u) {
        return false;
    }

    const uint32_t max_tq = 1u + lim->seg1_max + lim->seg2_max;

    /* The only float in this function, converted once and then left alone.
     * Everything below is integer, because the reference that generates the
     * vectors is Python, Python rounds half to even and C rounds half away from
     * zero, and one of the design points lands exactly on a half: 75 per cent
     * of 30 quanta is 22.5. A float comparison here would have made the two
     * sides disagree on that case and agree everywhere else, which is the
     * hardest kind of disagreement to find. */
    const uint32_t want_permille = round_u32(want_sp * 1000.0f);

    for (uint32_t pre = lim->prescaler_min; pre <= lim->prescaler_max; pre++) {
        if (kernel_hz % pre) {
            continue;                              /* inexact prescaler */
        }
        const uint32_t tq_hz = kernel_hz / pre;
        if (tq_hz % bitrate) {
            continue;                              /* inexact bit time  */
        }
        const uint32_t tq_per_bit = tq_hz / bitrate;
        if (tq_per_bit < lim->min_tq || tq_per_bit > max_tq) {
            continue;
        }

        const uint32_t before = (want_permille * tq_per_bit + 500u) / 1000u;
        if (before == 0u || before > tq_per_bit) {
            continue;
        }
        const uint32_t seg1 = before - 1u;         /* 1 tq is the sync */
        if (seg1 > tq_per_bit - 1u) {
            continue;
        }
        const uint32_t seg2 = tq_per_bit - seg1 - 1u;

        if (seg1 < lim->seg1_min || seg1 > lim->seg1_max) {
            continue;
        }
        if (seg2 < lim->seg2_min || seg2 > lim->seg2_max) {
            continue;
        }

        out->prescaler = pre;
        out->tq_per_bit = tq_per_bit;
        out->seg1 = seg1;
        out->seg2 = seg2;
        out->sjw = min_u32(seg2, lim->sjw_max);
        /* Integer per mille, so this agrees with the generated vectors exactly
         * rather than to within a float comparison nobody wrote down. */
        out->sample_point_permille =
            (uint32_t) (((uint64_t) (1u + seg1) * 1000u + tq_per_bit / 2u)
                        / tq_per_bit);
        return true;
    }

    return false;                                  /* caller must fail */
}
