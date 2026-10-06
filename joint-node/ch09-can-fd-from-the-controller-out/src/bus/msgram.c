/* msgram.c: an explicit layout, computed once and asserted against the size. */
#include "msgram.h"

/* Chapter 9 step 3. Sixteen receive elements on the first queue is the only
 * number here chosen for a reason rather than for roundness: at one kilohertz
 * with four joints reporting, sixteen is four control periods of slack before
 * anything is dropped, which is enough to survive a late handler and not so
 * much that a stale frame is ever acted on. */
/* Built from the macros rather than from literals, so the struct and the
 * compile time assertion in the header cannot say different numbers. */
const msgram_layout_t MSGRAM_LAYOUT = {
    .std_filters = MSGRAM_N_STD_FILTERS,
    .ext_filters = MSGRAM_N_EXT_FILTERS,
    .rx_fifo0    = MSGRAM_N_RX_FIFO0,
    .rx_fifo1    = MSGRAM_N_RX_FIFO1,
    .rx_buffers  = MSGRAM_N_RX_BUFFERS,
    .tx_event    = MSGRAM_N_TX_EVENT,
    .tx_buffers  = MSGRAM_N_TX_BUFFERS,
};

bool msgram_compute(const msgram_layout_t *lay, msgram_map_t *out)
{
    if (lay == 0 || out == 0) {
        return false;
    }

    if (lay->std_filters > MSGRAM_MAX_STD_FILTERS) return false;
    if (lay->ext_filters > MSGRAM_MAX_EXT_FILTERS) return false;
    if (lay->rx_fifo0    > MSGRAM_MAX_RX_FIFO0)    return false;
    if (lay->rx_fifo1    > MSGRAM_MAX_RX_FIFO1)    return false;
    if (lay->rx_buffers  > MSGRAM_MAX_RX_BUFFERS)  return false;
    if (lay->tx_event    > MSGRAM_MAX_TX_EVENT)    return false;
    if (lay->tx_buffers  > MSGRAM_MAX_TX_BUFFERS)  return false;

    uint32_t off = 0u;

    out->std_filters_off = off;
    out->std_filters_bytes = lay->std_filters * MSGRAM_STD_FILTER_BYTES;
    off += out->std_filters_bytes;

    out->ext_filters_off = off;
    out->ext_filters_bytes = lay->ext_filters * MSGRAM_EXT_FILTER_BYTES;
    off += out->ext_filters_bytes;

    out->rx_fifo0_off = off;
    out->rx_fifo0_bytes = lay->rx_fifo0 * MSGRAM_FRAME_BYTES;
    off += out->rx_fifo0_bytes;

    out->rx_fifo1_off = off;
    out->rx_fifo1_bytes = lay->rx_fifo1 * MSGRAM_FRAME_BYTES;
    off += out->rx_fifo1_bytes;

    out->rx_buffers_off = off;
    out->rx_buffers_bytes = lay->rx_buffers * MSGRAM_FRAME_BYTES;
    off += out->rx_buffers_bytes;

    out->tx_event_off = off;
    out->tx_event_bytes = lay->tx_event * MSGRAM_TX_EVENT_BYTES;
    off += out->tx_event_bytes;

    out->tx_buffers_off = off;
    out->tx_buffers_bytes = lay->tx_buffers * MSGRAM_FRAME_BYTES;
    off += out->tx_buffers_bytes;

    if (off > MSGRAM_SIZE_BYTES) {
        return false;
    }

    out->total_bytes = off;
    out->free_bytes = MSGRAM_SIZE_BYTES - off;
    return true;
}
