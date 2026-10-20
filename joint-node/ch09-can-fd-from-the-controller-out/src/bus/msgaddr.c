/* msgaddr.c: the arithmetic, with the refusals that make it worth having.
 *
 * Every refusal here is an access that would otherwise land on memory belonging
 * to a different section, and no register anywhere reports that.
 */
#include "msgaddr.h"

const char *msgaddr_section_name(msgram_section_t section)
{
    switch (section) {
    case MSGRAM_SEC_STD_FILTERS: return "standard filters";
    case MSGRAM_SEC_EXT_FILTERS: return "extended filters";
    case MSGRAM_SEC_RX_FIFO0:    return "receive FIFO 0";
    case MSGRAM_SEC_RX_FIFO1:    return "receive FIFO 1";
    case MSGRAM_SEC_RX_BUFFERS:  return "dedicated receive buffers";
    case MSGRAM_SEC_TX_EVENT:    return "transmit events";
    case MSGRAM_SEC_TX_BUFFERS:  return "transmit buffers";
    case MSGRAM_SEC_COUNT:
    default:                     return "an unknown section";
    }
}

uint32_t msgaddr_element_bytes(msgram_section_t section)
{
    switch (section) {
    case MSGRAM_SEC_STD_FILTERS: return MSGRAM_STD_FILTER_BYTES;
    case MSGRAM_SEC_EXT_FILTERS: return MSGRAM_EXT_FILTER_BYTES;
    case MSGRAM_SEC_RX_FIFO0:    return MSGRAM_FRAME_BYTES;
    case MSGRAM_SEC_RX_FIFO1:    return MSGRAM_FRAME_BYTES;
    case MSGRAM_SEC_RX_BUFFERS:  return MSGRAM_FRAME_BYTES;
    case MSGRAM_SEC_TX_EVENT:    return MSGRAM_TX_EVENT_BYTES;
    case MSGRAM_SEC_TX_BUFFERS:  return MSGRAM_FRAME_BYTES;
    case MSGRAM_SEC_COUNT:
    default:                     return 0u;
    }
}

uint32_t msgaddr_count(const msgram_layout_t *lay, msgram_section_t section)
{
    if (lay == 0) {
        return 0u;
    }
    switch (section) {
    case MSGRAM_SEC_STD_FILTERS: return lay->std_filters;
    case MSGRAM_SEC_EXT_FILTERS: return lay->ext_filters;
    case MSGRAM_SEC_RX_FIFO0:    return lay->rx_fifo0;
    case MSGRAM_SEC_RX_FIFO1:    return lay->rx_fifo1;
    case MSGRAM_SEC_RX_BUFFERS:  return lay->rx_buffers;
    case MSGRAM_SEC_TX_EVENT:    return lay->tx_event;
    case MSGRAM_SEC_TX_BUFFERS:  return lay->tx_buffers;
    case MSGRAM_SEC_COUNT:
    default:                     return 0u;
    }
}

/* Where the section begins and how many bytes the map gave it. Returns false
 * for an unknown section. The byte span is read out alongside the offset
 * because the check below compares the two, which is what couples this file to
 * the map rather than to its own idea of how big a section should be. */
static bool section_span(const msgram_map_t *map, msgram_section_t section,
                         uint32_t *off, uint32_t *bytes)
{
    if (map == 0 || off == 0 || bytes == 0) {
        return false;
    }
    switch (section) {
    case MSGRAM_SEC_STD_FILTERS:
        *off = map->std_filters_off; *bytes = map->std_filters_bytes; return true;
    case MSGRAM_SEC_EXT_FILTERS:
        *off = map->ext_filters_off; *bytes = map->ext_filters_bytes; return true;
    case MSGRAM_SEC_RX_FIFO0:
        *off = map->rx_fifo0_off;    *bytes = map->rx_fifo0_bytes;    return true;
    case MSGRAM_SEC_RX_FIFO1:
        *off = map->rx_fifo1_off;    *bytes = map->rx_fifo1_bytes;    return true;
    case MSGRAM_SEC_RX_BUFFERS:
        *off = map->rx_buffers_off;  *bytes = map->rx_buffers_bytes;  return true;
    case MSGRAM_SEC_TX_EVENT:
        *off = map->tx_event_off;    *bytes = map->tx_event_bytes;    return true;
    case MSGRAM_SEC_TX_BUFFERS:
        *off = map->tx_buffers_off;  *bytes = map->tx_buffers_bytes;  return true;
    case MSGRAM_SEC_COUNT:
    default:
        return false;
    }
}

bool msgaddr_offset(const msgram_layout_t *lay, const msgram_map_t *map,
                    msgram_section_t section, uint32_t index, uint32_t *out)
{
    if (lay == 0 || map == 0 || out == 0) {
        return false;
    }
    /* Checked as unsigned and against the upper bound only. A plain enum is
     * unsigned on ARM EABI, so a test for negative is dead code there and a
     * -Werror failure with it. */
    if ((uint32_t) section >= (uint32_t) MSGRAM_SEC_COUNT) {
        return false;
    }

    const uint32_t elem = msgaddr_element_bytes(section);
    if (elem == 0u) {
        return false;
    }

    const uint32_t count = msgaddr_count(lay, section);
    if (count == 0u || index >= count) {
        /* An empty section refuses index 0 as well. Its offset is the offset of
         * whatever section follows, and that is the silent one: this node
         * configures no dedicated receive buffers, so receive buffer 0 would
         * otherwise address the first transmit event. */
        return false;
    }

    uint32_t off = 0u, bytes = 0u;
    if (!section_span(map, section, &off, &bytes)) {
        return false;
    }

    /* The element must lie wholly inside the span the MAP gave this section.
     * The layout's count and the map's byte span are computed in two places, so
     * comparing them here is what catches one of the two drifting: a count that
     * grew without the map being recomputed asks for memory the map handed to
     * the next section. */
    const uint32_t end = (index + 1u) * elem;
    if (end > bytes) {
        return false;
    }

    *out = off + (index * elem);
    return true;
}

bool msgaddr_address(const msgram_layout_t *lay, const msgram_map_t *map,
                     msgram_section_t section, uint32_t index, uint32_t base,
                     uint32_t *out)
{
    if (out == 0) {
        return false;
    }
    uint32_t off = 0u;
    if (!msgaddr_offset(lay, map, section, index, &off)) {
        return false;
    }
    /* A base of zero is refused. The message memory of this part is not at
     * address zero, and a zero base is what an uninitialised or forgotten
     * argument looks like; returning an offset dressed up as an address would
     * make that mistake survive to a write. */
    if (base == 0u) {
        return false;
    }
    if (off > (0xFFFFFFFFu - base)) {
        return false;       /* the sum would wrap, and that is not an address */
    }
    *out = base + off;
    return true;
}
