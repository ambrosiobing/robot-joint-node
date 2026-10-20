/* fifo.h: the queue protocol, as arithmetic over status words.
 *
 * Chapter 9 stage two. msgaddr.h turns an index into a place. This file is
 * where the index comes from, and where the write that releases or launches an
 * element is composed.
 *
 * Nothing here touches a register. Every function takes a status word that was
 * read somewhere else and returns a number or a refusal, so the whole protocol
 * is checkable on a laptop with no controller on it. That matters more here
 * than anywhere else in the chapter, because on a board every mistake below
 * presents as a frame that is missing, duplicated or late, and never as an
 * error.
 *
 * THE ONE THAT WILL BITE. The receive acknowledge register takes AN INDEX. The
 * transmit add-request register takes A BIT PER BUFFER. They sit eight bytes
 * apart in purpose and are encoded in opposite ways:
 *
 *     RXF0A  <- 3            releases through element 3
 *     TXBAR  <- (1u << 3)    requests transmission of buffer 3
 *
 * Writing `1u << 3` to the acknowledge register acknowledges element 8, which
 * releases five elements nobody has read. Writing `3` to the add request
 * register requests buffers 0 and 1, one of which was never filled. Both writes
 * are accepted, neither errors, and the symptom is a frame gone missing or a
 * frame sent twice with contents that were never written. The two composers
 * below have the encoding in their names for that reason.
 *
 * THREE MORE, all silent.
 *
 *   - An empty FIFO still reports a get index, and so does a full one. Reading
 *     at the get index of an empty FIFO returns the element that was there
 *     before it was acknowledged, so the same frame is delivered twice and the
 *     second copy is indistinguishable from a real one.
 *
 *   - The indices wrap at THE CONFIGURED SIZE, not at the width of the field.
 *     The field holds six bits, so it can carry 63; this node configures 16
 *     receive elements. Walking past the end without wrapping addresses memory
 *     the layout never created, which msgaddr.c then refuses, but only if the
 *     walk asks it rather than doing the arithmetic itself.
 *
 *   - The message-lost flag is the only trace a dropped frame leaves, and it is
 *     sticky. Never reading it makes a loss invisible; reading it without
 *     clearing it makes one old loss look like a continuous fault.
 *
 * THIS FILE NAMES NO VENDOR HEADER, by chapter 4's rule. [kernel] The field
 * positions are from the mainline Linux driver for this peripheral, the Bosch
 * M_CAN, in drivers/net/can/m_can/m_can.c: RXFS_RFL BIT(25), RXFS_FF BIT(24),
 * RXFS_FPI_MASK GENMASK(21, 16), RXFS_FGI_MASK GENMASK(13, 8), RXFS_FFL_MASK
 * GENMASK(6, 0), TXFQS_TFQF BIT(21), TXFQS_TFQPI_MASK GENMASK(20, 16),
 * TXFQS_TFGI_MASK GENMASK(12, 8), TXFQS_TFFL_MASK GENMASK(5, 0).
 */
#ifndef JOINT_FIFO_H
#define JOINT_FIFO_H

#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------- the receive status word, RXFnS */
#define FIFO_RXFS_FFL_MASK   0x0000007Fu   /* fill level, bits 6:0 */
#define FIFO_RXFS_FGI_SHIFT  8u
#define FIFO_RXFS_FGI_MASK   0x3Fu         /* get index, bits 13:8 */
#define FIFO_RXFS_FPI_SHIFT  16u
#define FIFO_RXFS_FPI_MASK   0x3Fu         /* put index, bits 21:16 */
#define FIFO_RXFS_FF         (1u << 24)    /* FIFO full */
#define FIFO_RXFS_RFL        (1u << 25)    /* at least one message was lost */

/* ------------------------------------------ the transmit status word, TXFQS */
#define FIFO_TXFQS_TFFL_MASK  0x0000003Fu  /* free level, bits 5:0 */
#define FIFO_TXFQS_TFGI_SHIFT 8u
#define FIFO_TXFQS_TFGI_MASK  0x1Fu        /* get index, bits 12:8 */
#define FIFO_TXFQS_TFQPI_SHIFT 16u
#define FIFO_TXFQS_TFQPI_MASK  0x1Fu       /* put index, bits 20:16 */
#define FIFO_TXFQS_TFQF       (1u << 21)   /* FIFO or queue full */

typedef struct {
    uint32_t fill;        /* elements waiting to be read */
    uint32_t get_index;   /* where the oldest waiting element is */
    uint32_t put_index;   /* where the controller will write the next one */
    bool     full;
    bool     lost;        /* sticky: a frame was dropped at some point */
} fifo_rx_t;

typedef struct {
    uint32_t free;        /* elements free to be filled */
    uint32_t get_index;   /* the oldest element not yet transmitted */
    uint32_t put_index;   /* where to write the next frame */
    bool     full;
} fifo_tx_t;

/* Decode a receive status word against the number of elements THE LAYOUT
 * configured for that queue.
 *
 * Refuses when the word reports a fill level or an index the layout cannot
 * contain. That is not defensiveness about a well behaved peripheral: it is the
 * check that catches a layout and a controller that disagree about the size of
 * a queue, and the consequence of not catching it is reading an element in
 * another section's memory. `configured` of 0 refuses every word, because a
 * queue with no elements has no status worth decoding.
 */
bool fifo_rx_parse(uint32_t word, uint32_t configured, fifo_rx_t *out);

/* Decode a transmit status word the same way. */
bool fifo_tx_parse(uint32_t word, uint32_t configured, fifo_tx_t *out);

/* The index of the n-th element waiting, counting from 0 at the get index and
 * wrapping at `configured`.
 *
 * Refuses when n is at or past the fill level, which is what makes an empty
 * FIFO refuse index 0 rather than handing back the get index of an element that
 * was already released.
 */
bool fifo_rx_at(const fifo_rx_t *s, uint32_t configured, uint32_t n,
                uint32_t *index);

/* THE WORD TO WRITE TO THE ACKNOWLEDGE REGISTER after reading `n` elements
 * starting at the get index. It is AN INDEX, not a mask: the value released is
 * the index of the LAST element read, and acknowledging it releases every
 * element from the get index through that one.
 *
 * Refuses n of 0, because there is nothing to release and writing the get index
 * would release an element that has not been read, and refuses n past the fill
 * level, because that releases elements that have not arrived.
 */
bool fifo_rx_ack_index(const fifo_rx_t *s, uint32_t configured, uint32_t n,
                       uint32_t *index);

/* Where to write the next frame to transmit. Refuses when the queue is full or
 * reports no free element, because the put index is still a valid looking
 * number then and writing there overwrites a frame that has not been sent. */
bool fifo_tx_put(const fifo_tx_t *s, uint32_t *index);

/* THE WORD TO WRITE TO THE ADD REQUEST REGISTER to transmit the element at
 * `index`. It is A BIT PER BUFFER, not an index. Refuses an index at or past
 * `configured`, and refuses any index the register cannot address. */
bool fifo_tx_request_mask(uint32_t index, uint32_t configured, uint32_t *mask);

/* Whether the element at `index` is still waiting to be transmitted, read from
 * the request-pending register, which is also a bit per buffer. Refuses an
 * index at or past `configured`. */
bool fifo_tx_pending(uint32_t pending_word, uint32_t index,
                     uint32_t configured, bool *pending);

#endif /* JOINT_FIFO_H */
