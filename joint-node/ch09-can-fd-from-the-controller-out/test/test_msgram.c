/* test_msgram.c: the C layout against the generated vectors.
 *
 * The vectors come from tools/gen_msgram.py. Both sides are compared against
 * the vector file rather than against each other, so a disagreement names the
 * section and the number.
 *
 * Built in continuous integration with -std=c11 -Wall -Wextra -Werror. It is
 * never built on the win11 aquamarine authoring laptop, which has no compiler
 * that the house rules permit running.
 */
#include <stdio.h>
#include <string.h>

#include "msgram.h"
#include "vectors_msgram.h"

static int failures;

static void check(const char *section, const char *field,
                  uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("  FAIL %s: %s is %u, the vectors say %u\n",
               section, field, (unsigned) got, (unsigned) want);
        failures++;
    }
}

int main(void)
{
    msgram_map_t m;
    memset(&m, 0, sizeof m);

    if (!msgram_compute(&MSGRAM_LAYOUT, &m)) {
        printf("  FAIL the chapter's own layout was refused\n");
        return 1;
    }

    const uint32_t off[] = {
        m.std_filters_off, m.ext_filters_off, m.rx_fifo0_off, m.rx_fifo1_off,
        m.rx_buffers_off, m.tx_event_off, m.tx_buffers_off,
    };
    const uint32_t bytes[] = {
        m.std_filters_bytes, m.ext_filters_bytes, m.rx_fifo0_bytes,
        m.rx_fifo1_bytes, m.rx_buffers_bytes, m.tx_event_bytes,
        m.tx_buffers_bytes,
    };

    if (MSGRAM_VECTOR_COUNT != (int) (sizeof off / sizeof off[0])) {
        printf("  FAIL the vectors describe %d sections and the map has %d\n",
               MSGRAM_VECTOR_COUNT, (int) (sizeof off / sizeof off[0]));
        return 1;
    }

    for (int i = 0; i < MSGRAM_VECTOR_COUNT; i++) {
        const msgram_vector_t *v = &MSGRAM_VECTORS[i];
        check(v->name, "offset", off[i], v->offset);
        check(v->name, "bytes", bytes[i], v->bytes);
    }

    check("total", "bytes", m.total_bytes, MSGRAM_EXPECT_TOTAL_BYTES);
    check("total", "free", m.free_bytes, MSGRAM_EXPECT_FREE_BYTES);

    /* The sections must tile the space with no hole and no overlap: each one
     * starts where the previous ended. A hole is wasted memory and an overlap
     * is two sections writing the same words, which is the failure that looks
     * like random corruption rather than like a configuration mistake. */
    for (int i = 1; i < MSGRAM_VECTOR_COUNT; i++) {
        const uint32_t expect = off[i - 1] + bytes[i - 1];
        if (off[i] != expect) {
            printf("  FAIL %s starts at %u and the previous section ends at "
                   "%u\n", MSGRAM_VECTORS[i].name,
                   (unsigned) off[i], (unsigned) expect);
            failures++;
        }
    }

    /* Three ways of saying the same total have to agree: the function, the
     * run time macro over the struct, and the constant the compile time
     * assertion uses. The assertion cannot read the struct, because a const
     * object's members are not a constant expression in C, so it reads its own
     * macros instead. That is the gap where the two can drift apart, and this
     * is the only place it would ever be noticed. */
    if (MSGRAM_BYTES(MSGRAM_LAYOUT) != m.total_bytes) {
        printf("  FAIL the run time macro says %u and the function says %u\n",
               (unsigned) MSGRAM_BYTES(MSGRAM_LAYOUT),
               (unsigned) m.total_bytes);
        failures++;
    }
    if (MSGRAM_LAYOUT_BYTES != m.total_bytes) {
        printf("  FAIL the asserted constant says %u and the function says "
               "%u, so the build is guarding a layout that is not this one\n",
               (unsigned) MSGRAM_LAYOUT_BYTES, (unsigned) m.total_bytes);
        failures++;
    }

    /* Refusals. A layout checker that has never refused anything has not been
     * tested, and each of these fails for a different reason. */
    msgram_layout_t bad = MSGRAM_LAYOUT;
    bad.std_filters = MSGRAM_MAX_STD_FILTERS + 1u;
    if (msgram_compute(&bad, &m)) {
        printf("  FAIL too many standard filters was accepted\n");
        failures++;
    }

    bad = MSGRAM_LAYOUT;
    bad.tx_buffers = MSGRAM_MAX_TX_BUFFERS + 1u;
    if (msgram_compute(&bad, &m)) {
        printf("  FAIL too many transmit buffers was accepted\n");
        failures++;
    }

    const msgram_layout_t maxed = {
        MSGRAM_MAX_STD_FILTERS, MSGRAM_MAX_EXT_FILTERS, MSGRAM_MAX_RX_FIFO0,
        MSGRAM_MAX_RX_FIFO1, MSGRAM_MAX_RX_BUFFERS, MSGRAM_MAX_TX_EVENT,
        MSGRAM_MAX_TX_BUFFERS,
    };
    if (msgram_compute(&maxed, &m)) {
        printf("  FAIL every section at its maximum was accepted, and it does "
               "not fit\n");
        failures++;
    }

    if (failures) {
        printf("test_msgram: %d failure(s)\n", failures);
        return 1;
    }
    printf("ok  %d sections tile %u bytes with %u free, 3 layouts refused\n",
           MSGRAM_VECTOR_COUNT, (unsigned) MSGRAM_EXPECT_TOTAL_BYTES,
           (unsigned) MSGRAM_EXPECT_FREE_BYTES);
    return 0;
}
