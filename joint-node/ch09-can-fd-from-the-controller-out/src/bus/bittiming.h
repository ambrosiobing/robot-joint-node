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

/* ------------------------------------------------------- the register words
 *
 * A timing that has been solved still has to be written, and the two registers
 * that hold it pack the same four numbers into different places with different
 * widths. Settled Wednesday 7 October 2026 from the mainline Linux driver for
 * this exact peripheral, the Bosch M_CAN, in drivers/net/can/m_can/m_can.c.
 * RM0455 would say the same and st.com has never served it to this bench.
 *
 * Two properties of that source are worth stating, because they are why these
 * numbers are trusted without the reference manual.
 *
 * The field widths agree with the limits above, all eight of them. The nominal
 * prescaler field is bits 24:16, nine bits, and BT_NOMINAL says 1 to 512. The
 * data segment 2 field is bits 7:4, four bits, and BT_DATA says 1 to 16. Those
 * limits were taken from the field definitions when this chapter was written;
 * a second and independent source now agrees on every one.
 *
 * And the hardware stores each value minus one, which this header has claimed
 * since it was written and which the driver confirms by subtracting one from
 * the prescaler, the jump width and both segments before shifting them.
 *
 * NBTP, the nominal phase:   NSJW 31:25   NBRP 24:16   NTSEG1 15:8   NTSEG2 6:0
 * DBTP, the data phase:      TDC 23       DBRP 20:16   DTSEG1 12:8   DTSEG2 7:4
 *                            DSJW 3:0
 */
#define BT_NBTP_NSJW_SHIFT    25u
#define BT_NBTP_NSJW_BITS      7u
#define BT_NBTP_NBRP_SHIFT    16u
#define BT_NBTP_NBRP_BITS      9u
#define BT_NBTP_NTSEG1_SHIFT   8u
#define BT_NBTP_NTSEG1_BITS    8u
#define BT_NBTP_NTSEG2_SHIFT   0u
#define BT_NBTP_NTSEG2_BITS    7u

#define BT_DBTP_TDC           (1u << 23)
#define BT_DBTP_DBRP_SHIFT    16u
#define BT_DBTP_DBRP_BITS      5u
#define BT_DBTP_DTSEG1_SHIFT   8u
#define BT_DBTP_DTSEG1_BITS    5u
#define BT_DBTP_DTSEG2_SHIFT   4u
#define BT_DBTP_DTSEG2_BITS    4u
#define BT_DBTP_DSJW_SHIFT     0u
#define BT_DBTP_DSJW_BITS      4u

/* Pack a solved timing into its register word.
 *
 * Returns false, and writes nothing, when any of the four values will not fit
 * the field that must hold it. That is not a theoretical guard: the two phases
 * have different field widths, so a timing solved for the nominal phase and
 * packed as a data word is the exact mistake these functions exist to refuse.
 * A nominal timing at 80 MHz has a segment 1 of 127, and the data field is five
 * bits wide, so the truncated word would configure 2.6 Mbit/s instead of
 * 500 kbit/s and nothing would report an error.
 *
 * So the caller must check the return value. Silently truncating is how a
 * controller ends up at a bit rate nobody chose.
 */
bool bt_pack_nbtp(const bt_t *bt, uint32_t *out);

/* The data phase. `tdc` sets the transmitter delay compensation enable bit,
 * which is a separate decision from the timing and belongs to chapter 9 step 5;
 * the delay offset itself lives in TDCR and is not packed here. */
bool bt_pack_dbtp(const bt_t *bt, bool tdc, uint32_t *out);

/* Read the four values back out of a register word, so a configuration can be
 * verified against what was intended rather than against the fact of having
 * written it. The sample point and the quanta per bit are recomputed from the
 * segments, so an unpacked timing is directly comparable with a solved one.
 *
 * This is the half that makes a read-back worth doing. The driver this register
 * map came from writes CCCR and reads it back up to ten times before believing
 * it, and a volume that has already been caught by a register reading back its
 * reset value should do no less. */
void bt_unpack_nbtp(uint32_t word, bt_t *out);
void bt_unpack_dbtp(uint32_t word, bt_t *out);

#endif /* JOINT_BITTIMING_H */
