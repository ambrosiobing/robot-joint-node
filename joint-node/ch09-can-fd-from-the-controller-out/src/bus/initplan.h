/* initplan.h: the order the controller is configured in, as data rather than
 * as a function that does it.
 *
 * Chapter 9, stage two. The values were settled on Wednesday 7 October 2026 and
 * written up in doc/node-registers.md. The order was settled the same evening
 * and it is the part most likely to be got wrong, because three of the four
 * guesses about it were wrong and every one of those three fails silently:
 *
 *   - the message RAM must be cleared first, or an uninitialised element reads
 *     back as a parity error that presents as a bus fault
 *   - CCCR must be written before TEST, because CCCR.TEST is what makes TEST
 *     writable at all, so the other order leaves loopback off with every other
 *     bit reading back correctly
 *   - GFC of zero ACCEPTS every non-matching frame, so the value that looks
 *     like a safe default is the one that makes receive work
 *
 * A sequence that fails silently cannot be debugged on the board, so it is not
 * written as board code. It is produced here as a list of operations, by a
 * function with no hardware in it, and checked against a vector file on a host
 * that has no controller. src/bsp/ then walks the list.
 *
 * THIS FILE NAMES NO VENDOR HEADER AND MUST NOT. Chapter 4's rule is that the
 * silicon is reached only under src/bsp/, so that chapter 20 can build the node
 * on a host. An offset is a number; including a vendor header to learn it would
 * tie this file to one part for no gain.
 */
#ifndef JOINT_INITPLAN_H
#define JOINT_INITPLAN_H

#include <stdbool.h>
#include <stdint.h>

#include "bittiming.h"
#include "msgram.h"

/* The Bosch M_CAN register offsets, from the mainline Linux driver's own enum.
 * The peripheral is not ST's design, which is why a driver that runs on parts
 * from several vendors is a better source than any one vendor's summary. */
#define FDCAN_CREL   0x000u   /* core release, read to prove the block answers */
#define FDCAN_ENDN   0x004u   /* endianness, reads 0x87654321 */
#define FDCAN_DBTP   0x00Cu   /* data bit timing */
#define FDCAN_TEST   0x010u   /* test modes */
#define FDCAN_CCCR   0x018u   /* control, and the configuration gate */
#define FDCAN_NBTP   0x01Cu   /* nominal bit timing */
#define FDCAN_ECR    0x040u   /* error counters */
#define FDCAN_PSR    0x044u   /* protocol status */
#define FDCAN_IR     0x050u   /* interrupt flags, polled by this image */
#define FDCAN_GFC    0x080u   /* global filter */
#define FDCAN_SIDFC  0x084u   /* standard filter list */
#define FDCAN_XIDFC  0x088u   /* extended filter list */
#define FDCAN_RXF0C  0x0A0u   /* receive FIFO 0 */
#define FDCAN_RXF0S  0x0A4u   /* its status */
#define FDCAN_RXF0A  0x0A8u   /* its acknowledge */
#define FDCAN_RXBC   0x0ACu   /* receive buffers */
#define FDCAN_RXF1C  0x0B0u   /* receive FIFO 1 */
#define FDCAN_RXESC  0x0BCu   /* receive element size */
#define FDCAN_TXBC   0x0C0u   /* transmit buffers */
#define FDCAN_TXFQS  0x0C4u   /* its status */
#define FDCAN_TXESC  0x0C8u   /* transmit element size */
#define FDCAN_TXBAR  0x0D0u   /* add request, this is what sends */
#define FDCAN_TXEFC  0x0F0u   /* transmit event FIFO */

/* CCCR, bit by bit. */
#define FDCAN_CCCR_INIT  (1u << 0)
#define FDCAN_CCCR_CCE   (1u << 1)
#define FDCAN_CCCR_ASM   (1u << 2)
#define FDCAN_CCCR_CSA   (1u << 3)
#define FDCAN_CCCR_CSR   (1u << 4)
#define FDCAN_CCCR_MON   (1u << 5)
#define FDCAN_CCCR_DAR   (1u << 6)
#define FDCAN_CCCR_TEST  (1u << 7)
#define FDCAN_CCCR_FDOE  (1u << 8)
#define FDCAN_CCCR_BRSE  (1u << 9)
#define FDCAN_CCCR_NISO  (1u << 15)

/* TEST, the one bit this image uses. */
#define FDCAN_TEST_LBCK  (1u << 4)

/* CSR and CSA are a clock stop request and its acknowledgement, not settings.
 * A read-back comparison must mask them out or it will report a mismatch on a
 * bit that was never the caller's to set. */
#define FDCAN_CCCR_VOLATILE (FDCAN_CCCR_CSR | FDCAN_CCCR_CSA)

/* The element size encoding. 0x7 is 64 bytes of payload, and this chapter's
 * layout assumes exactly that: MSGRAM_FRAME_BYTES is eighteen words, two of
 * header and sixteen of data. The reset value is 8 byte elements, so leaving
 * these registers alone makes the layout wrong by more than a factor of four
 * and puts frames on top of one another. */
#define FDCAN_ESC_64B 0x7u

#define FDCAN_RXESC_F0DS_SHIFT 0u
#define FDCAN_RXESC_F1DS_SHIFT 4u
#define FDCAN_RXESC_RBDS_SHIFT 8u
#define FDCAN_TXESC_TBDS_SHIFT 0u

/* Element count fields, for the sections whose count is written beside the
 * offset. The offset occupies the low bits of each of these registers and is a
 * byte offset from the message RAM base, NOT an absolute address. */
#define FDCAN_SIDFC_LSS_SHIFT  16u
#define FDCAN_XIDFC_LSE_SHIFT  16u
#define FDCAN_RXF0C_F0S_SHIFT  16u
#define FDCAN_RXF1C_F1S_SHIFT  16u
#define FDCAN_RXBC_OFF_SHIFT    0u
#define FDCAN_TXEFC_EFS_SHIFT  16u
#define FDCAN_TXBC_TFQS_SHIFT  24u

/* What the node is for. The first image is a classic node at 500 kbit/s; the
 * other two modes exist because a bus is not needed to exercise either. */
typedef enum {
    FDCAN_MODE_NORMAL = 0,
    /* CCCR.TEST, CCCR.MON and TEST.LBCK together. MON is the bit that makes it
     * internal: it stops the transmitter driving the wire, so the frame goes
     * round inside the peripheral and nothing appears on the bus. */
    FDCAN_MODE_LOOPBACK_INTERNAL,
    /* `[inferred]` The same without MON, which should put the frame on the real
     * wire and read it back. The driver this register map came from implements
     * only the internal form, so this is reasoning from what MON does rather
     * than a reading, and it is visible on the board: the controller end either
     * sees the frame or does not. */
    FDCAN_MODE_LOOPBACK_EXTERNAL,
    /* CCCR.MON alone. Receives, never acknowledges. */
    FDCAN_MODE_LISTEN_ONLY,
} fdcan_mode_t;

typedef struct {
    uint32_t     kernel_hz;      /* what FDCANSEL actually selected */
    uint32_t     bitrate;        /* the nominal phase */
    uint32_t     want_sp_permille;
    fdcan_mode_t mode;
    bool         one_shot;       /* CCCR.DAR, no automatic retransmission */
} fdcan_config_t;

/* One operation. `kind` says what to do with the other three fields, and `name`
 * is there so an image can print the step it is on: a sequence that stops needs
 * to say where, and "step 11 of 24" is not an answer a reader can act on. */
typedef enum {
    FDCAN_OP_READ = 0,     /* read reg, report it, change nothing */
    FDCAN_OP_WRITE,        /* reg = value */
    FDCAN_OP_MODIFY,       /* reg = (reg & ~mask) | value, then read back */
    FDCAN_OP_WAIT_SET,     /* spin until every bit in mask reads 1 */
    FDCAN_OP_WAIT_CLEAR,   /* spin until every bit in mask reads 0 */
    FDCAN_OP_CLEAR_RAM,    /* zero the message RAM from `reg` to `mask`, bytes */
    FDCAN_OP_EXPECT,       /* read reg and require it to equal value */
} fdcan_op_t;

typedef struct {
    fdcan_op_t  op;
    uint32_t    reg;       /* register offset, or the first byte for CLEAR_RAM */
    uint32_t    mask;      /* bits, or one past the last byte for CLEAR_RAM */
    uint32_t    value;
    const char *name;
} fdcan_step_t;

/* The longest plan this file can produce. Asserted against the real count by
 * the test, so a plan that outgrows it fails on a host rather than overrunning
 * a buffer on the board. */
#define FDCAN_PLAN_MAX 32u

/* Build the plan. Returns false, having written nothing useful, when the bit
 * timing cannot be solved exactly, when it will not fit NBTP, when the message
 * memory layout does not fit the part, or when `max` is too small. Every one of
 * those is a reason not to start rather than something to work around.
 *
 * The timing that was solved is returned through `timing` so the image can
 * print it beside the word, because a reader who sees only 0x04000B02 cannot
 * tell 500 kbit/s from 250. */
bool fdcan_plan(const fdcan_config_t *cfg, const msgram_layout_t *layout,
                fdcan_step_t *steps, uint32_t max, uint32_t *count,
                bt_t *timing);

/* The name of an operation, for the same reason the steps carry names. */
const char *fdcan_op_name(fdcan_op_t op);

#endif /* JOINT_INITPLAN_H */
