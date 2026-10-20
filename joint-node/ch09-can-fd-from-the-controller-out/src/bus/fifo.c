/* fifo.c: the arithmetic, and the refusals that are the reason for the file.
 *
 * Every refusal here is a read of an element nobody wrote, a release of an
 * element nobody read, or a transmission of a buffer nobody filled. None of
 * those is reported by the controller.
 */
#include "fifo.h"

/* The widest value each index field can hold. A status word reporting more than
 * this did not come from this peripheral, and a layout cannot be larger. */
#define RX_INDEX_LIMIT (FIFO_RXFS_FGI_MASK + 1u)    /* 64 */
#define TX_INDEX_LIMIT (FIFO_TXFQS_TFGI_MASK + 1u)  /* 32 */

bool fifo_rx_parse(uint32_t word, uint32_t configured, fifo_rx_t *out)
{
    if (out == 0) {
        return false;
    }
    /* A queue with no elements has no status worth decoding. The layout this
     * chapter configures has one such queue, the dedicated receive buffers, and
     * msgaddr.c refuses to address it for the same reason. */
    if (configured == 0u || configured > RX_INDEX_LIMIT) {
        return false;
    }

    const uint32_t fill = word & FIFO_RXFS_FFL_MASK;
    const uint32_t get = (word >> FIFO_RXFS_FGI_SHIFT) & FIFO_RXFS_FGI_MASK;
    const uint32_t put = (word >> FIFO_RXFS_FPI_SHIFT) & FIFO_RXFS_FPI_MASK;

    /* The controller and the layout have to agree about how big this queue is.
     * When they do not, every index below addresses memory the layout gave to
     * another section, and the read succeeds. */
    if (fill > configured || get >= configured || put >= configured) {
        return false;
    }

    /* The full flag and the fill level are two statements of one fact, and a
     * word that makes them disagree is not describing a queue. Reading one and
     * trusting it while the other says otherwise is how a full queue gets
     * written to. */
    const bool full = (word & FIFO_RXFS_FF) != 0u;
    if (full != (fill == configured)) {
        return false;
    }

    out->fill = fill;
    out->get_index = get;
    out->put_index = put;
    out->full = full;
    out->lost = (word & FIFO_RXFS_RFL) != 0u;
    return true;
}

bool fifo_tx_parse(uint32_t word, uint32_t configured, fifo_tx_t *out)
{
    if (out == 0) {
        return false;
    }
    if (configured == 0u || configured > TX_INDEX_LIMIT) {
        return false;
    }

    const uint32_t freel = word & FIFO_TXFQS_TFFL_MASK;
    const uint32_t get = (word >> FIFO_TXFQS_TFGI_SHIFT) & FIFO_TXFQS_TFGI_MASK;
    const uint32_t put =
        (word >> FIFO_TXFQS_TFQPI_SHIFT) & FIFO_TXFQS_TFQPI_MASK;

    /* The transmit side reports FREE elements where the receive side reports
     * waiting ones. Reading one as the other inverts the test for room, so a
     * full queue looks empty and an empty one looks full; both are plausible
     * numbers and the field names are one letter apart. */
    if (freel > configured || get >= configured || put >= configured) {
        return false;
    }

    const bool full = (word & FIFO_TXFQS_TFQF) != 0u;
    if (full != (freel == 0u)) {
        return false;
    }

    out->free = freel;
    out->get_index = get;
    out->put_index = put;
    out->full = full;
    return true;
}

bool fifo_rx_at(const fifo_rx_t *s, uint32_t configured, uint32_t n,
                uint32_t *index)
{
    if (s == 0 || index == 0 || configured == 0u) {
        return false;
    }
    /* An empty FIFO refuses n of 0. Its get index is a perfectly good number
     * pointing at the element that was last released, and the controller does
     * not clear the memory, so a read there returns a frame that has already
     * been delivered once. */
    if (n >= s->fill) {
        return false;
    }
    if (s->get_index >= configured) {
        return false;
    }
    /* Wrapping at the CONFIGURED size, not at the width of the field. */
    *index = (s->get_index + n) % configured;
    return true;
}

bool fifo_rx_ack_index(const fifo_rx_t *s, uint32_t configured, uint32_t n,
                       uint32_t *index)
{
    if (s == 0 || index == 0) {
        return false;
    }
    /* Nothing read means nothing to release. Writing the get index here would
     * release an element that has not been looked at, and the frame is gone
     * with no record that it existed. */
    if (n == 0u || n > s->fill) {
        return false;
    }
    /* The index of the LAST element read, because acknowledging an index
     * releases every element from the get index through it.
     *
     * [kernel] The driver reads a run of elements, tracking the index itself,
     * and writes one acknowledge afterwards with the last index it read.
     * [inferred] That the write releases the whole run rather than one element
     * follows from the driver doing it, not from a statement anywhere that it
     * does. One board run settles it: read two frames, acknowledge the second
     * index only, and see whether the fill level falls by two or by one. Until
     * that run exists this function and the chapter both say inferred. */
    return fifo_rx_at(s, configured, n - 1u, index);
}

bool fifo_tx_put(const fifo_tx_t *s, uint32_t *index)
{
    if (s == 0 || index == 0) {
        return false;
    }
    /* A full queue still reports a put index, and it is the index of a frame
     * that has been requested and not yet sent. Writing there replaces a frame
     * already queued for transmission with a different one, and what goes on
     * the wire is whichever of the two won. */
    if (s->full || s->free == 0u) {
        return false;
    }
    *index = s->put_index;
    return true;
}

bool fifo_tx_request_mask(uint32_t index, uint32_t configured, uint32_t *mask)
{
    if (mask == 0) {
        return false;
    }
    if (configured == 0u || configured > TX_INDEX_LIMIT) {
        return false;
    }
    if (index >= configured) {
        return false;
    }
    /* A BIT PER BUFFER. The shift is what makes this different from the
     * acknowledge register, which takes the index itself. */
    *mask = 1u << index;
    return true;
}

bool fifo_tx_pending(uint32_t pending_word, uint32_t index,
                     uint32_t configured, bool *pending)
{
    if (pending == 0) {
        return false;
    }
    if (configured == 0u || configured > TX_INDEX_LIMIT) {
        return false;
    }
    if (index >= configured) {
        return false;
    }
    *pending = (pending_word & (1u << index)) != 0u;
    return true;
}
