/* bittiming.h: one bit, divided into quanta, for each of the two phases.
 *
 * Chapter 9. The node computes its timing from the kernel clock it reads back
 * at boot and refuses to start if either phase cannot be solved exactly,
 * because a bit rate that is one part in a thousand out works between two nodes
 * that are wrong in the same direction and fails against anything else.
 *
 * The two phases do not share limits. The nominal phase carries the
 * arbitration, where every node on the wire must agree, and its register fields
 * are wide; the data phase only has to survive one transmitter and its fields
 * are much narrower. A data prescaler above 32 cannot be written at all. That
 * is why the limits are an argument here rather than a set of constants: one
 * set of constants cannot be right for both phases, and the set that is right
 * for the nominal phase silently accepts data timings the hardware will not
 * hold.
 */
#ifndef JOINT_BITTIMING_H
#define JOINT_BITTIMING_H

#include <stdbool.h>
#include <stdint.h>

/* The limits of one phase, as the values the fields hold rather than as the
 * raw field contents. The hardware stores each of these minus one, which is a
 * detail for whoever writes the register and a trap for whoever reads a
 * configuration back without remembering it. */
typedef struct {
    const char *name;
    uint32_t prescaler_min, prescaler_max;
    uint32_t seg1_min, seg1_max;
    uint32_t seg2_min, seg2_max;
    uint32_t sjw_max;
    uint32_t min_tq;        /* see the floor note in the source */
} bt_limits_t;

extern const bt_limits_t BT_NOMINAL;
extern const bt_limits_t BT_DATA;

typedef struct {
    uint32_t prescaler;
    uint32_t tq_per_bit;
    uint32_t seg1;          /* quanta before the sample point, after the sync */
    uint32_t seg2;          /* quanta after it */
    uint32_t sjw;
    uint32_t sample_point_permille;   /* 800 is 80.0 per cent */
} bt_t;

/* Solve one phase. Returns false when no exact solution exists, and the caller
 * must treat that as a reason not to start rather than as something to round
 * past. want_sp is a fraction: 0.80f for eighty per cent. */
bool bt_compute(uint32_t kernel_hz, uint32_t bitrate, float want_sp,
                const bt_limits_t *lim, bt_t *out);

#endif /* JOINT_BITTIMING_H */
