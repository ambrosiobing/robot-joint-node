/* msgram.h: the message memory, laid out before anything else is configured.
 *
 * Chapter 9. A controller whose message memory has not been laid out accepts
 * every other configuration, reports no error, and never transmits. That is the
 * single most common way this peripheral appears broken, and it is why the
 * layout is computed and asserted rather than assumed.
 *
 * The sections do not share an element size. A standard filter is one word, an
 * extended filter and a transmit event are two, and a receive or transmit
 * element carrying the full sixty-four byte payload is eighteen: two of header
 * and sixteen of data. A single element_bytes field cannot describe that, and a
 * total computed as if it could is wrong by more than a third.
 */
#ifndef JOINT_MSGRAM_H
#define JOINT_MSGRAM_H

#include <stdbool.h>
#include <stdint.h>

#define MSGRAM_WORD 4u

/* Bytes per element, by section. */
#define MSGRAM_STD_FILTER_BYTES (1u * MSGRAM_WORD)
#define MSGRAM_EXT_FILTER_BYTES (2u * MSGRAM_WORD)
#define MSGRAM_TX_EVENT_BYTES   (2u * MSGRAM_WORD)
#define MSGRAM_FRAME_BYTES      (18u * MSGRAM_WORD)   /* 2 header + 16 data */

/* The most elements this part accepts in each section, and the memory it has.
 * These are the silicon vendor's own numbers, from the driver source. */
#define MSGRAM_MAX_STD_FILTERS 128u
#define MSGRAM_MAX_EXT_FILTERS  64u
#define MSGRAM_MAX_RX_FIFO0     64u
#define MSGRAM_MAX_RX_FIFO1     64u
#define MSGRAM_MAX_RX_BUFFERS   64u
#define MSGRAM_MAX_TX_EVENT     32u
#define MSGRAM_MAX_TX_BUFFERS   32u
#define MSGRAM_SIZE_BYTES    (10u * 1024u)

typedef struct {
    uint32_t std_filters;
    uint32_t ext_filters;
    uint32_t rx_fifo0;
    uint32_t rx_fifo1;
    uint32_t rx_buffers;
    uint32_t tx_event;
    uint32_t tx_buffers;
} msgram_layout_t;

/* The total a layout occupies, as an expression so it can be asserted at
 * compile time. Each section is multiplied by its own element size. */
#define MSGRAM_BYTES(l)                                   \
    ((l).std_filters * MSGRAM_STD_FILTER_BYTES            \
     + (l).ext_filters * MSGRAM_EXT_FILTER_BYTES          \
     + (l).rx_fifo0   * MSGRAM_FRAME_BYTES                \
     + (l).rx_fifo1   * MSGRAM_FRAME_BYTES                \
     + (l).rx_buffers * MSGRAM_FRAME_BYTES                \
     + (l).tx_event   * MSGRAM_TX_EVENT_BYTES             \
     + (l).tx_buffers * MSGRAM_FRAME_BYTES)

typedef struct {
    uint32_t std_filters_off, std_filters_bytes;
    uint32_t ext_filters_off, ext_filters_bytes;
    uint32_t rx_fifo0_off,    rx_fifo0_bytes;
    uint32_t rx_fifo1_off,    rx_fifo1_bytes;
    uint32_t rx_buffers_off,  rx_buffers_bytes;
    uint32_t tx_event_off,    tx_event_bytes;
    uint32_t tx_buffers_off,  tx_buffers_bytes;
    uint32_t total_bytes;
    uint32_t free_bytes;
} msgram_map_t;

/* The layout this node uses, and the assertion that it fits. The build fails
 * rather than the controller going quiet. */
extern const msgram_layout_t MSGRAM_LAYOUT;

/* Compute the section offsets. Returns false when a section asks for more
 * elements than the part accepts, or when the total does not fit, and the
 * caller must treat that as a reason not to start. */
bool msgram_compute(const msgram_layout_t *lay, msgram_map_t *out);

#endif /* JOINT_MSGRAM_H */
