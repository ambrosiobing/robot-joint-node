/* test_fifo.c: the queue protocol against the generated vectors.
 *
 * The vectors come from tools/gen_fifo.py, which is the reference. Both sides
 * are compared against the vector file rather than against each other.
 *
 * This one links msgaddr and msgram as well, for a single rule: every index the
 * protocol hands out has to be an index the message memory can address. The two
 * modules are each correct on their own terms when the queue wraps at the width
 * of the status field instead of the size of the queue, and only a rule that
 * spans them notices.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written.
 */
#include <stdio.h>
#include <string.h>

#include "fifo.h"
#include "msgaddr.h"
#include "msgram.h"
#include "vectors_fifo.h"

static int failures;

static void check_u32(const char *label, const char *field,
                      uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("  FAIL %s: %s is %u, the vectors say %u\n",
               label, field, (unsigned) got, (unsigned) want);
        failures++;
    }
}

static void check_bool(const char *label, const char *field,
                       bool got, bool want)
{
    if (got != want) {
        printf("  FAIL %s: %s is %s, the vectors say %s\n", label, field,
               got ? "true" : "false", want ? "true" : "false");
        failures++;
    }
}

int main(void)
{
    /* ------------------------------------------------------- the status words */

    for (int i = 0; i < FV_PARSE_COUNT; i++) {
        const fv_parse_t *v = &FV_PARSE[i];

        if (v->is_rx) {
            fifo_rx_t s;
            memset(&s, 0, sizeof s);
            const bool ok = fifo_rx_parse(v->word, v->configured, &s);
            check_bool(v->label, "parsed", ok, v->ok);
            if (!ok || !v->ok) {
                continue;
            }
            check_u32(v->label, "fill", s.fill, v->fill);
            check_u32(v->label, "get index", s.get_index, v->get_index);
            check_u32(v->label, "put index", s.put_index, v->put_index);
            check_bool(v->label, "full", s.full, v->full);
            check_bool(v->label, "lost", s.lost, v->lost);
        } else {
            fifo_tx_t s;
            memset(&s, 0, sizeof s);
            const bool ok = fifo_tx_parse(v->word, v->configured, &s);
            check_bool(v->label, "parsed", ok, v->ok);
            if (!ok || !v->ok) {
                continue;
            }
            check_u32(v->label, "free level", s.free, v->free_level);
            check_u32(v->label, "get index", s.get_index, v->get_index);
            check_u32(v->label, "put index", s.put_index, v->put_index);
            check_bool(v->label, "full", s.full, v->full);
        }
    }

    /* -------------------------------------------------- reading and releasing */

    for (int i = 0; i < FV_RX_OP_COUNT; i++) {
        const fv_rx_op_t *v = &FV_RX_OPS[i];

        fifo_rx_t s;
        memset(&s, 0, sizeof s);
        if (!fifo_rx_parse(v->word, v->configured, &s)) {
            printf("  FAIL %s: the status word did not parse\n", v->label);
            failures++;
            continue;
        }

        uint32_t at = 0xFFFFFFFFu;
        const bool at_ok = fifo_rx_at(&s, v->configured, v->n, &at);
        check_bool(v->label, "element to read", at_ok, v->at_ok);
        if (at_ok && v->at_ok) {
            check_u32(v->label, "element index", at, v->at);
        }

        uint32_t ack = 0xFFFFFFFFu;
        const bool ack_ok = fifo_rx_ack_index(&s, v->configured, v->n + 1u,
                                              &ack);
        check_bool(v->label, "acknowledge", ack_ok, v->ack_ok);
        if (ack_ok && v->ack_ok) {
            check_u32(v->label, "acknowledge index", ack, v->ack);
        }

        /* The acknowledge that follows reading element n is the index of
         * element n itself, because the write releases through it. */
        if (at_ok && ack_ok && at != ack) {
            printf("  FAIL %s: read element %u and the acknowledge write is "
                   "%u, so the release does not include what was read\n",
                   v->label, (unsigned) at, (unsigned) ack);
            failures++;
        }
    }

    /* --------------------------------------------------- filling and sending */

    for (int i = 0; i < FV_TX_OP_COUNT; i++) {
        const fv_tx_op_t *v = &FV_TX_OPS[i];

        fifo_tx_t s;
        memset(&s, 0, sizeof s);
        if (!fifo_tx_parse(v->word, v->configured, &s)) {
            printf("  FAIL %s: the status word did not parse\n", v->label);
            failures++;
            continue;
        }

        uint32_t put = 0xFFFFFFFFu;
        const bool put_ok = fifo_tx_put(&s, &put);
        check_bool(v->label, "buffer to fill", put_ok, v->put_ok);
        if (put_ok && v->put_ok) {
            check_u32(v->label, "put index", put, v->put);
        }

        uint32_t mask = 0xFFFFFFFFu;
        const bool mask_ok = fifo_tx_request_mask(v->index, v->configured,
                                                  &mask);
        check_bool(v->label, "add request", mask_ok, v->mask_ok);
        if (mask_ok && v->mask_ok) {
            check_u32(v->label, "add request mask", mask, v->mask);
        }

        bool pending = false;
        const bool pend_ok = fifo_tx_pending(v->pending_word, v->index,
                                             v->configured, &pending);
        check_bool(v->label, "pending readable", pend_ok, v->pending_ok);
        if (pend_ok && v->pending_ok) {
            check_bool(v->label, "pending", pending, v->pending);
        }
    }

    /* ---------------------------------------------------------- the invariants */

    /* 1. THE ENCODINGS MUST NEVER AGREE. The acknowledge register takes an
     * index and the add request register takes a bit per buffer, so a value
     * written to the wrong register is always wrong and never accidentally
     * right. An index where the two coincided would let a swap pass by luck. */
    for (uint32_t index = 0u; index < FV_TX_BUFFERS; index++) {
        uint32_t mask = 0u;
        if (!fifo_tx_request_mask(index, FV_TX_BUFFERS, &mask)) {
            printf("  FAIL buffer %u has no add request mask\n",
                   (unsigned) index);
            failures++;
            continue;
        }
        if (mask == index) {
            printf("  FAIL buffer %u: the add request mask and the "
                   "acknowledge index are both %u, so writing one to the "
                   "other's register would look correct\n",
                   (unsigned) index, (unsigned) index);
            failures++;
        }
        if (mask != (1u << index)) {
            printf("  FAIL buffer %u: the add request mask is 0x%X and a bit "
                   "per buffer is 0x%X\n", (unsigned) index, (unsigned) mask,
                   (unsigned) (1u << index));
            failures++;
        }
    }

    /* 2. Exhaustive over every status word this queue can produce. Each field
     * must come back as it went in, and acceptance must depend on the full flag
     * agreeing with the fill level and on nothing else. */
    unsigned swept = 0u;
    for (uint32_t fill = 0u; fill <= FV_RX_FIFO0; fill++) {
        for (uint32_t get = 0u; get < FV_RX_FIFO0; get++) {
            for (uint32_t put = 0u; put < FV_RX_FIFO0; put++) {
                for (int f = 0; f < 2; f++) {
                    const bool full = (f != 0);
                    uint32_t word = fill & FIFO_RXFS_FFL_MASK;
                    word |= (get & FIFO_RXFS_FGI_MASK) << FIFO_RXFS_FGI_SHIFT;
                    word |= (put & FIFO_RXFS_FPI_MASK) << FIFO_RXFS_FPI_SHIFT;
                    if (full) {
                        word |= FIFO_RXFS_FF;
                    }
                    fifo_rx_t s;
                    memset(&s, 0, sizeof s);
                    const bool ok = fifo_rx_parse(word, FV_RX_FIFO0, &s);
                    const bool want = (full == (fill == FV_RX_FIFO0));
                    swept++;
                    if (ok != want) {
                        printf("  FAIL fill %u get %u put %u full %s: parsed "
                               "is %s and the flag and the level %s\n",
                               (unsigned) fill, (unsigned) get, (unsigned) put,
                               full ? "set" : "clear", ok ? "true" : "false",
                               want ? "agree" : "disagree");
                        failures++;
                        continue;
                    }
                    if (!ok) {
                        continue;
                    }
                    if (s.fill != fill || s.get_index != get ||
                        s.put_index != put || s.full != full) {
                        printf("  FAIL fill %u get %u put %u came back as "
                               "%u %u %u\n", (unsigned) fill, (unsigned) get,
                               (unsigned) put, (unsigned) s.fill,
                               (unsigned) s.get_index, (unsigned) s.put_index);
                        failures++;
                    }
                }
            }
        }
    }

    /* 3. Reading a FULL queue must visit every element exactly once. That is
     * the wrap stated as a property rather than as one example: a wrap at the
     * width of the field revisits some elements and never reaches others, and
     * every read succeeds. */
    for (uint32_t get = 0u; get < FV_RX_FIFO0; get++) {
        uint32_t word = FV_RX_FIFO0 | (get << FIFO_RXFS_FGI_SHIFT)
                        | (get << FIFO_RXFS_FPI_SHIFT) | FIFO_RXFS_FF;
        fifo_rx_t s;
        memset(&s, 0, sizeof s);
        if (!fifo_rx_parse(word, FV_RX_FIFO0, &s)) {
            printf("  FAIL a full queue with get index %u was refused\n",
                   (unsigned) get);
            failures++;
            continue;
        }
        bool seen[FV_RX_FIFO0];
        memset(seen, 0, sizeof seen);
        for (uint32_t n = 0u; n < FV_RX_FIFO0; n++) {
            uint32_t at = 0u;
            if (!fifo_rx_at(&s, FV_RX_FIFO0, n, &at)) {
                printf("  FAIL get index %u: a full queue refused element "
                       "%u\n", (unsigned) get, (unsigned) n);
                failures++;
                continue;
            }
            if (at >= FV_RX_FIFO0) {
                printf("  FAIL get index %u: element %u is index %u, past the "
                       "%u configured\n", (unsigned) get, (unsigned) n,
                       (unsigned) at, (unsigned) FV_RX_FIFO0);
                failures++;
                continue;
            }
            if (seen[at]) {
                printf("  FAIL get index %u: index %u was visited twice "
                       "reading one full queue\n", (unsigned) get,
                       (unsigned) at);
                failures++;
            }
            seen[at] = true;
        }
        for (uint32_t k = 0u; k < FV_RX_FIFO0; k++) {
            if (!seen[k]) {
                printf("  FAIL get index %u: index %u was never reached "
                       "reading a full queue\n", (unsigned) get,
                       (unsigned) k);
                failures++;
            }
        }
    }

    /* 4. Every index the protocol hands out must be an index the message memory
     * can address. The rule that spans two files: a wrap at the field width
     * produces indices msgaddr refuses, and each module is right on its own
     * terms. */
    {
        msgram_map_t map;
        memset(&map, 0, sizeof map);
        if (!msgram_compute(&MSGRAM_LAYOUT, &map)) {
            printf("  FAIL this node's layout was refused, so the cross check "
                   "below is unreachable\n");
            failures++;
        } else {
            for (uint32_t get = 0u; get < FV_RX_FIFO0; get++) {
                uint32_t word = FV_RX_FIFO0 | (get << FIFO_RXFS_FGI_SHIFT)
                                | (get << FIFO_RXFS_FPI_SHIFT)
                                | FIFO_RXFS_FF;
                fifo_rx_t s;
                memset(&s, 0, sizeof s);
                if (!fifo_rx_parse(word, FV_RX_FIFO0, &s)) {
                    continue;   /* already reported above */
                }
                for (uint32_t n = 0u; n < FV_RX_FIFO0; n++) {
                    uint32_t at = 0u, off = 0u;
                    if (!fifo_rx_at(&s, FV_RX_FIFO0, n, &at)) {
                        continue;
                    }
                    if (!msgaddr_offset(&MSGRAM_LAYOUT, &map,
                                        MSGRAM_SEC_RX_FIFO0, at, &off)) {
                        printf("  FAIL the protocol offers receive element %u "
                               "and the message memory refuses to address "
                               "it\n", (unsigned) at);
                        failures++;
                    }
                }
            }
            for (uint32_t index = 0u; index < FV_TX_BUFFERS; index++) {
                uint32_t off = 0u;
                if (!msgaddr_offset(&MSGRAM_LAYOUT, &map,
                                    MSGRAM_SEC_TX_BUFFERS, index, &off)) {
                    printf("  FAIL the protocol offers transmit buffer %u and "
                           "the message memory refuses to address it\n",
                           (unsigned) index);
                    failures++;
                }
            }
        }
    }

    /* 5. An empty queue refuses every n, including 0. This is the read that
     * returns a frame already delivered, and the get index of an empty queue is
     * an ordinary looking number. */
    for (uint32_t get = 0u; get < FV_RX_FIFO0; get++) {
        const uint32_t word = (get << FIFO_RXFS_FGI_SHIFT)
                              | (get << FIFO_RXFS_FPI_SHIFT);
        fifo_rx_t s;
        memset(&s, 0, sizeof s);
        if (!fifo_rx_parse(word, FV_RX_FIFO0, &s)) {
            printf("  FAIL an empty queue with get index %u was refused\n",
                   (unsigned) get);
            failures++;
            continue;
        }
        for (uint32_t n = 0u; n <= FV_RX_FIFO0; n++) {
            uint32_t at = 0u;
            if (fifo_rx_at(&s, FV_RX_FIFO0, n, &at)) {
                printf("  FAIL an empty queue with get index %u offered "
                       "element %u at index %u\n", (unsigned) get,
                       (unsigned) n, (unsigned) at);
                failures++;
            }
        }
        uint32_t ack = 0u;
        if (fifo_rx_ack_index(&s, FV_RX_FIFO0, 0u, &ack) ||
            fifo_rx_ack_index(&s, FV_RX_FIFO0, 1u, &ack)) {
            printf("  FAIL an empty queue with get index %u composed an "
                   "acknowledge\n", (unsigned) get);
            failures++;
        }
    }

    /* 6. A transmit queue refuses a put exactly when it is full. */
    for (uint32_t freel = 0u; freel <= FV_TX_BUFFERS; freel++) {
        uint32_t word = freel | (3u << FIFO_TXFQS_TFQPI_SHIFT);
        if (freel == 0u) {
            word |= FIFO_TXFQS_TFQF;
        }
        fifo_tx_t s;
        memset(&s, 0, sizeof s);
        if (!fifo_tx_parse(word, FV_TX_BUFFERS, &s)) {
            printf("  FAIL free level %u: a legal transmit word was refused\n",
                   (unsigned) freel);
            failures++;
            continue;
        }
        uint32_t put = 0u;
        const bool ok = fifo_tx_put(&s, &put);
        if (ok == (freel == 0u)) {
            printf("  FAIL free level %u: put is %s and the queue is %s\n",
                   (unsigned) freel, ok ? "offered" : "refused",
                   freel == 0u ? "full" : "not full");
            failures++;
        }
        if (ok && put >= FV_TX_BUFFERS) {
            printf("  FAIL free level %u: put index %u is past the %u "
                   "buffers configured\n", (unsigned) freel, (unsigned) put,
                   (unsigned) FV_TX_BUFFERS);
            failures++;
        }
    }

    /* 7. An index past what the layout configured is refused by both transmit
     * composers, even though the registers themselves could address it. */
    for (uint32_t index = FV_TX_BUFFERS; index < 40u; index++) {
        uint32_t mask = 0u;
        bool pending = false;
        if (fifo_tx_request_mask(index, FV_TX_BUFFERS, &mask)) {
            printf("  FAIL buffer %u was given an add request mask and the "
                   "layout configures %u\n", (unsigned) index,
                   (unsigned) FV_TX_BUFFERS);
            failures++;
        }
        if (fifo_tx_pending(0xFFFFFFFFu, index, FV_TX_BUFFERS, &pending)) {
            printf("  FAIL buffer %u was read from the pending word and the "
                   "layout configures %u\n", (unsigned) index,
                   (unsigned) FV_TX_BUFFERS);
            failures++;
        }
    }

    /* 8. Null arguments are refused rather than dereferenced. */
    {
        fifo_rx_t s;
        memset(&s, 0, sizeof s);
        uint32_t v = 0u;
        if (fifo_rx_parse(0u, FV_RX_FIFO0, 0) ||
            fifo_tx_parse(0u, FV_TX_BUFFERS, 0) ||
            fifo_rx_at(0, FV_RX_FIFO0, 0u, &v) ||
            fifo_rx_at(&s, FV_RX_FIFO0, 0u, 0) ||
            fifo_rx_ack_index(0, FV_RX_FIFO0, 1u, &v) ||
            fifo_tx_put(0, &v) ||
            fifo_tx_request_mask(0u, FV_TX_BUFFERS, 0)) {
            printf("  FAIL a null argument was accepted\n");
            failures++;
        }
    }

    if (failures) {
        printf("test_fifo: %d failure(s)\n", failures);
        return 1;
    }
    printf("ok  %d cases and %u status words swept, every full queue read "
           "visits all %u elements once, both encodings distinct\n",
           FV_PARSE_COUNT + FV_RX_OP_COUNT + FV_TX_OP_COUNT, swept,
           (unsigned) FV_RX_FIFO0);
    return 0;
}
