/* initplan.c: the sequence, built once, with nothing in it that touches a bus.
 *
 * The order follows the mainline Linux driver for this peripheral. Where this
 * file departs from it, the departure is commented, because "I did it in a
 * different order and it worked" is not something a later reader can check.
 */
#include "initplan.h"

/* A small helper so the body below reads as a sequence rather than as struct
 * initialisation. Returns false once the array is full, and every caller
 * accumulates that into one flag, so an overrun is a refusal rather than a
 * write past the end. */
static bool emit(fdcan_step_t *steps, uint32_t max, uint32_t *n,
                 fdcan_op_t op, uint32_t reg, uint32_t mask, uint32_t value,
                 const char *name)
{
    if (*n >= max) {
        return false;
    }
    steps[*n].op = op;
    steps[*n].reg = reg;
    steps[*n].mask = mask;
    steps[*n].value = value;
    steps[*n].name = name;
    (*n)++;
    return true;
}

const char *fdcan_op_name(fdcan_op_t op)
{
    switch (op) {
    case FDCAN_OP_READ:       return "read";
    case FDCAN_OP_WRITE:      return "write";
    case FDCAN_OP_MODIFY:     return "modify";
    case FDCAN_OP_WAIT_SET:   return "wait set";
    case FDCAN_OP_WAIT_CLEAR: return "wait clear";
    case FDCAN_OP_CLEAR_RAM:  return "clear ram";
    case FDCAN_OP_EXPECT:     return "expect";
    default:                  return "unknown";
    }
}

/* The mode bits, as one mask to clear and one value to set. Returned together
 * because a mode change is a read, modify, write: setting loopback without
 * clearing monitoring would leave whatever the previous mode left behind, and
 * the reset value is not the only state this can be entered from. */
static void mode_bits(fdcan_mode_t mode, bool one_shot,
                      uint32_t *clear, uint32_t *set)
{
    /* Everything this function is responsible for, cleared first. FDOE and BRSE
     * are in here because the first image is classic: a controller left in FD
     * mode by a previous image would otherwise stay there. */
    *clear = FDCAN_CCCR_TEST | FDCAN_CCCR_MON | FDCAN_CCCR_ASM
           | FDCAN_CCCR_DAR | FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE
           | FDCAN_CCCR_NISO;
    *set = 0u;

    switch (mode) {
    case FDCAN_MODE_LOOPBACK_INTERNAL:
        *set |= FDCAN_CCCR_TEST | FDCAN_CCCR_MON;
        break;
    case FDCAN_MODE_LOOPBACK_EXTERNAL:
        *set |= FDCAN_CCCR_TEST;
        break;
    case FDCAN_MODE_LISTEN_ONLY:
        *set |= FDCAN_CCCR_MON;
        break;
    case FDCAN_MODE_NORMAL:
    default:
        break;
    }

    if (one_shot) {
        *set |= FDCAN_CCCR_DAR;
    }
}

bool fdcan_plan(const fdcan_config_t *cfg, const msgram_layout_t *layout,
                fdcan_step_t *steps, uint32_t max, uint32_t *count,
                bt_t *timing)
{
    if (cfg == 0 || layout == 0 || steps == 0 || count == 0 || timing == 0) {
        return false;
    }
    *count = 0u;

    /* Everything that can be refused is refused before the first step is
     * emitted, so a plan that exists is a plan that can be run. */
    msgram_map_t map;
    if (!msgram_compute(layout, &map)) {
        return false;
    }

    const float want_sp = (float) cfg->want_sp_permille / 1000.0f;
    if (!bt_compute(cfg->kernel_hz, cfg->bitrate, want_sp, &BT_NOMINAL, timing)) {
        return false;
    }

    uint32_t nbtp = 0u;
    if (!bt_pack_nbtp(timing, &nbtp)) {
        return false;
    }

    uint32_t clear = 0u, set = 0u;
    mode_bits(cfg->mode, cfg->one_shot, &clear, &set);

    /* The test register is only writable while CCCR.TEST is set, so the value
     * written to it depends on the mode in the same way CCCR does. */
    const uint32_t test = (cfg->mode == FDCAN_MODE_LOOPBACK_INTERNAL
                           || cfg->mode == FDCAN_MODE_LOOPBACK_EXTERNAL)
                        ? FDCAN_TEST_LBCK : 0u;

    bool ok = true;

    /* 1 and 2. Prove the register block answers before configuring it. A wrong
     * base address reads back as zeroes, and zeroes look like a peripheral that
     * is merely unconfigured. ENDN is the better of the two for that, because it
     * has one correct value and no other. */
    ok = ok && emit(steps, max, count, FDCAN_OP_READ, FDCAN_CREL, 0u, 0u,
                    "core release, and which M_CAN version this part carries");
    ok = ok && emit(steps, max, count, FDCAN_OP_EXPECT, FDCAN_ENDN, 0xFFFFFFFFu,
                    0x87654321u, "endianness, the one register with one answer");

    /* 3 and 4. INIT, and then confirmed. The driver this follows refuses to
     * configure a peripheral that is not in INIT rather than trying, so the
     * confirmation is not optional politeness. */
    ok = ok && emit(steps, max, count, FDCAN_OP_MODIFY, FDCAN_CCCR,
                    FDCAN_CCCR_INIT, FDCAN_CCCR_INIT, "stop the peripheral");
    ok = ok && emit(steps, max, count, FDCAN_OP_WAIT_SET, FDCAN_CCCR,
                    FDCAN_CCCR_INIT, 0u, "wait for it to be stopped");

    /* 5. The message RAM, before anything points at it. The driver's reason is
     * parity and ECC errors when reading an element that was never written, and
     * that failure presents as a receive error on a bus that is working. */
    ok = ok && emit(steps, max, count, FDCAN_OP_CLEAR_RAM,
                    map.std_filters_off, map.total_bytes, 0u,
                    "clear every word of the message RAM in use");

    /* 6 and 7. CCE, which is only legal while INIT is set. */
    ok = ok && emit(steps, max, count, FDCAN_OP_MODIFY, FDCAN_CCCR,
                    FDCAN_CCCR_CCE, FDCAN_CCCR_CCE, "open configuration");
    ok = ok && emit(steps, max, count, FDCAN_OP_WAIT_SET, FDCAN_CCCR,
                    FDCAN_CCCR_CCE, 0u, "wait for configuration to be open");

    /* 8 and 9. The element sizes, and they are not optional. The reset value is
     * 8 byte elements while this chapter's layout assumes 64, so an image that
     * leaves these alone lays its sections out more than four times too far
     * apart and the hardware writes them on top of one another. */
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_RXESC, 0u,
                    (FDCAN_ESC_64B << FDCAN_RXESC_F0DS_SHIFT)
                    | (FDCAN_ESC_64B << FDCAN_RXESC_F1DS_SHIFT)
                    | (FDCAN_ESC_64B << FDCAN_RXESC_RBDS_SHIFT),
                    "receive elements are 64 byte");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_TXESC, 0u,
                    (FDCAN_ESC_64B << FDCAN_TXESC_TBDS_SHIFT),
                    "transmit elements are 64 byte");

    /* 10. GFC of zero ACCEPTS non-matching frames into receive FIFO 0. The name
     * suggests a gate to open and the value suggests a safe default; it is the
     * other way round, and since no filter element is written by this image,
     * every frame is non-matching. A filter configuration that rejects
     * everything is indistinguishable from a dead bus. */
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_GFC, 0u, 0u,
                    "accept every non-matching frame into receive FIFO 0");

    /* 11 to 16. Where each section lives. Every one of these holds a BYTE
     * OFFSET from the message RAM base and not an absolute address, which two
     * independent drivers confirm and which msgram_compute already produces, so
     * nothing is added here.
     *
     * A DEPARTURE WORTH NAMING: the driver's own configuration path does not
     * write SIDFC and XIDFC where this does. It configures them elsewhere, and
     * that elsewhere was not read. Their position here is a choice, placed with
     * the other memory registers because that is where they belong by subject.
     * If a filter list ever misbehaves, this is the line to doubt first. */
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_SIDFC, 0u,
                    (layout->std_filters << FDCAN_SIDFC_LSS_SHIFT)
                    | map.std_filters_off, "standard filter list");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_XIDFC, 0u,
                    (layout->ext_filters << FDCAN_XIDFC_LSE_SHIFT)
                    | map.ext_filters_off, "extended filter list");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_RXF0C, 0u,
                    (layout->rx_fifo0 << FDCAN_RXF0C_F0S_SHIFT)
                    | map.rx_fifo0_off, "receive FIFO 0");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_RXF1C, 0u,
                    (layout->rx_fifo1 << FDCAN_RXF1C_F1S_SHIFT)
                    | map.rx_fifo1_off, "receive FIFO 1");
    /* Written even though this layout asks for zero receive buffers, because
     * the reset value points somewhere and somewhere is not nowhere. */
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_RXBC, 0u,
                    map.rx_buffers_off, "receive buffers, none of them");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_TXEFC, 0u,
                    (layout->tx_event << FDCAN_TXEFC_EFS_SHIFT)
                    | map.tx_event_off, "transmit event FIFO");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_TXBC, 0u,
                    (layout->tx_buffers << FDCAN_TXBC_TFQS_SHIFT)
                    | map.tx_buffers_off, "transmit FIFO or queue");

    /* 17 and 18. CCCR first, then TEST, and that order is the whole reason this
     * is a list. CCCR.TEST is what makes TEST writable, so writing TEST first
     * writes to a locked register: it fails silently, loopback stays off, and
     * every other bit reads back exactly as intended. */
    ok = ok && emit(steps, max, count, FDCAN_OP_MODIFY, FDCAN_CCCR,
                    clear | set, set, "the mode bits");
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_TEST, 0u, test,
                    "the test register, writable only now");

    /* 19. The timing, late, which is not where instinct puts it. The driver
     * writes it after the memory and the mode bits and this follows, because
     * following working code beats rationalising a different order. */
    ok = ok && emit(steps, max, count, FDCAN_OP_WRITE, FDCAN_NBTP, 0u, nbtp,
                    "nominal bit timing");

    /* 20. Configuration closed, and INIT still set. */
    ok = ok && emit(steps, max, count, FDCAN_OP_MODIFY, FDCAN_CCCR,
                    FDCAN_CCCR_CCE, 0u, "close configuration");

    /* 21 and 22. And only now is the peripheral on the bus. Separating this
     * from everything above means a configuration that fails leaves a stopped
     * controller rather than a running and wrongly configured one. */
    ok = ok && emit(steps, max, count, FDCAN_OP_MODIFY, FDCAN_CCCR,
                    FDCAN_CCCR_INIT, 0u, "start");
    ok = ok && emit(steps, max, count, FDCAN_OP_WAIT_CLEAR, FDCAN_CCCR,
                    FDCAN_CCCR_INIT, 0u, "wait until it is running");

    if (!ok) {
        *count = 0u;
        return false;
    }
    return true;
}
