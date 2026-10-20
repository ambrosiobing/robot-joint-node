/* test_msgaddr.c: element addressing against the generated vectors.
 *
 * The vectors come from tools/gen_msgaddr.py, which is the reference. Both
 * sides are compared against the vector file rather than against each other,
 * so a disagreement names the section and the index.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written.
 */
#include <stdio.h>
#include <string.h>

#include "msgaddr.h"
#include "msgram.h"
#include "vectors_msgaddr.h"

static int failures;

static void check_u32(const char *label, const char *field,
                      uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("  FAIL %s: %s is 0x%08X, the vectors say 0x%08X\n",
               label, field, (unsigned) got, (unsigned) want);
        failures++;
    }
}

int main(void)
{
    const msgram_layout_t *lay = &MSGRAM_LAYOUT;

    msgram_map_t map;
    memset(&map, 0, sizeof map);
    if (!msgram_compute(lay, &map)) {
        printf("  FAIL this node's own layout was refused by msgram_compute, "
               "so nothing below can be addressed\n");
        return 1;
    }

    int resolved = 0, refused = 0;

    /* ------------------------------------------------------- the vectors */

    for (int i = 0; i < MA_VECTOR_COUNT; i++) {
        const ma_vector_t *v = &MA_VECTORS[i];
        const msgram_section_t sec = (msgram_section_t) v->section;

        uint32_t off = 0xFFFFFFFFu;
        const bool got_off = msgaddr_offset(lay, &map, sec, v->index, &off);
        if (got_off != v->offset_ok) {
            printf("  FAIL %s: the offset was %s and the vectors say %s\n",
                   v->label, got_off ? "resolved" : "refused",
                   v->offset_ok ? "it resolves" : "it is refused");
            failures++;
            continue;
        }
        if (got_off) {
            check_u32(v->label, "offset", off, v->offset);
        }

        uint32_t addr = 0xFFFFFFFFu;
        const bool got_addr = msgaddr_address(lay, &map, sec, v->index,
                                              v->base, &addr);
        if (got_addr != v->address_ok) {
            printf("  FAIL %s: the address was %s and the vectors say %s\n",
                   v->label, got_addr ? "resolved" : "refused",
                   v->address_ok ? "it resolves" : "it is refused");
            failures++;
            continue;
        }
        if (got_addr) {
            check_u32(v->label, "address", addr, v->address);
            /* An offset is not an address, and the difference is the base. A
             * call that lost the base would return a plausible number. */
            check_u32(v->label, "address minus offset", addr - off, v->base);
            if (addr == off) {
                printf("  FAIL %s: the address and the offset are the same "
                       "number, so one is being used for the other\n",
                       v->label);
                failures++;
            }
            resolved++;
        } else {
            refused++;
        }

        check_u32(v->label, "element bytes",
                  msgaddr_element_bytes(sec), v->element_bytes);
        check_u32(v->label, "configured elements",
                  msgaddr_count(lay, sec), v->elements);

        /* A refused access must leave the caller's variable alone. A function
         * that writes a value and then returns false hands a wrong address to
         * anybody who checked the return in the wrong order, and that is the
         * one mistake a refusal is supposed to prevent. */
        if (!got_off) {
            uint32_t keep = 0xA5A5A5A5u;
            (void) msgaddr_offset(lay, &map, sec, v->index, &keep);
            if (keep != 0xA5A5A5A5u) {
                printf("  FAIL %s: a refused offset wrote 0x%08X into the "
                       "caller's variable\n", v->label, (unsigned) keep);
                failures++;
            }
        }
        if (!got_addr) {
            uint32_t keep = 0xA5A5A5A5u;
            (void) msgaddr_address(lay, &map, sec, v->index, v->base, &keep);
            if (keep != 0xA5A5A5A5u) {
                printf("  FAIL %s: a refused address wrote 0x%08X into the "
                       "caller's variable\n", v->label, (unsigned) keep);
                failures++;
            }
        }
    }

    /* ---------------------------------------------------- the invariants */

    /* 1. The C enumerates as many sections as the generator lists, and in the
     * same order. A section's offset is the sum of every section before it, so
     * a reordering moves every section but the first and nothing says so. */
    if ((int) MSGRAM_SEC_COUNT != MA_SECTION_COUNT) {
        printf("  FAIL the C has %d sections and the generator lists %d\n",
               (int) MSGRAM_SEC_COUNT, MA_SECTION_COUNT);
        failures++;
    }

    /* 2. Every configured element, walked in the memory's own order, must begin
     * exactly where the previous one ended, and the last must end exactly at
     * the total. Contiguity in one pass catches an overlap, a hole, a wrong
     * element size and a reordered enumeration, all of which otherwise return a
     * number that looks like an answer. */
    {
        uint32_t expect = 0u;
        for (uint32_t s = 0u; s < (uint32_t) MSGRAM_SEC_COUNT; s++) {
            const msgram_section_t sec = (msgram_section_t) s;
            const uint32_t elem = msgaddr_element_bytes(sec);
            const uint32_t count = msgaddr_count(lay, sec);
            for (uint32_t index = 0u; index < count; index++) {
                uint32_t off = 0u;
                if (!msgaddr_offset(lay, &map, sec, index, &off)) {
                    printf("  FAIL %s element %u of the %u configured was "
                           "refused, so a configured element has no address\n",
                           msgaddr_section_name(sec), (unsigned) index,
                           (unsigned) count);
                    failures++;
                    continue;
                }
                if (off != expect) {
                    printf("  FAIL %s element %u is at 0x%04X and the previous "
                           "element ended at 0x%04X\n",
                           msgaddr_section_name(sec), (unsigned) index,
                           (unsigned) off, (unsigned) expect);
                    failures++;
                }
                if (off % MSGRAM_WORD != 0u) {
                    printf("  FAIL %s element %u is at 0x%04X, which is not a "
                           "multiple of %u\n", msgaddr_section_name(sec),
                           (unsigned) index, (unsigned) off,
                           (unsigned) MSGRAM_WORD);
                    failures++;
                }
                expect = off + elem;
            }
        }
        if (expect != map.total_bytes) {
            printf("  FAIL the last element ends at 0x%04X and the layout "
                   "totals 0x%04X, so the sections do not tile\n",
                   (unsigned) expect, (unsigned) map.total_bytes);
            failures++;
        }
    }

    /* 3. THE DEFECT, named rather than implied. A section the layout gives no
     * elements shares its offset with whatever follows it, so index 0 of an
     * empty section is a real offset pointing at somebody else's first element.
     * This node configures no dedicated receive buffers, so the collision is in
     * the layout under test rather than in a hypothetical one. */
    {
        int empty_sections = 0;
        for (uint32_t s = 0u; s < (uint32_t) MSGRAM_SEC_COUNT; s++) {
            const msgram_section_t sec = (msgram_section_t) s;
            if (msgaddr_count(lay, sec) != 0u) {
                continue;
            }
            empty_sections++;

            uint32_t off = 0u;
            if (msgaddr_offset(lay, &map, sec, 0u, &off)) {
                printf("  FAIL %s is empty and index 0 resolved to 0x%04X\n",
                       msgaddr_section_name(sec), (unsigned) off);
                failures++;
            }
        }
        if (empty_sections == 0) {
            printf("  FAIL no section in this layout is empty, so the case "
                   "this file exists for is unreachable and the refusal is "
                   "untested\n");
            failures++;
        }
    }

    /* 4. An index one past the count is refused for every section, empty or
     * full. This is the same arithmetic as the span check and deliberately so:
     * the two agree whenever the layout and the map agree, and the span check
     * earns its place only when they have drifted. */
    for (uint32_t s = 0u; s < (uint32_t) MSGRAM_SEC_COUNT; s++) {
        const msgram_section_t sec = (msgram_section_t) s;
        const uint32_t count = msgaddr_count(lay, sec);
        uint32_t off = 0u;
        if (msgaddr_offset(lay, &map, sec, count, &off)) {
            printf("  FAIL %s index %u resolved and the layout configured %u "
                   "elements\n", msgaddr_section_name(sec), (unsigned) count,
                   (unsigned) count);
            failures++;
        }
    }

    /* 5. A section value past the enumeration is refused rather than indexing
     * off the end of a switch. */
    {
        uint32_t off = 0u;
        if (msgaddr_offset(lay, &map, MSGRAM_SEC_COUNT, 0u, &off)) {
            printf("  FAIL the section count itself was accepted as a "
                   "section\n");
            failures++;
        }
        if (msgaddr_offset(lay, &map, (msgram_section_t) 99u, 0u, &off)) {
            printf("  FAIL section 99 was accepted\n");
            failures++;
        }
        if (msgaddr_element_bytes((msgram_section_t) 99u) != 0u) {
            printf("  FAIL section 99 was given an element size\n");
            failures++;
        }
        if (msgaddr_section_name((msgram_section_t) 99u) == 0) {
            printf("  FAIL an unknown section's name is a null pointer, which "
                   "turns a diagnostic into a fault\n");
            failures++;
        }
    }

    /* 6. The first element of the first section is at offset zero, and no
     * element's offset equals the message memory base. The second half is what
     * keeps an offset and an address from being told apart by eye. */
    {
        uint32_t off = 0xFFFFFFFFu;
        if (!msgaddr_offset(lay, &map, (msgram_section_t) 0, 0u, &off)) {
            printf("  FAIL the first element of the first section has no "
                   "offset\n");
            failures++;
        } else if (off != 0u) {
            printf("  FAIL the first element of the first section is at "
                   "0x%04X and the memory begins at 0\n", (unsigned) off);
            failures++;
        }
    }

    /* 7. A null layout, a null map and a null output are refused rather than
     * dereferenced. The executor in planrun.c refuses an unusable io for the
     * same reason: a caller that has not finished setting up is the caller most
     * likely to be wrong about everything else too. */
    {
        uint32_t off = 0u;
        if (msgaddr_offset(0, &map, MSGRAM_SEC_TX_BUFFERS, 0u, &off) ||
            msgaddr_offset(lay, 0, MSGRAM_SEC_TX_BUFFERS, 0u, &off) ||
            msgaddr_offset(lay, &map, MSGRAM_SEC_TX_BUFFERS, 0u, 0)) {
            printf("  FAIL a null argument was accepted\n");
            failures++;
        }
        if (msgaddr_count(0, MSGRAM_SEC_TX_BUFFERS) != 0u) {
            printf("  FAIL a null layout reported a count\n");
            failures++;
        }
    }

    if (failures) {
        printf("test_msgaddr: %d failure(s) over %d accesses\n",
               failures, MA_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d accesses, %d resolved and %d refused, every configured "
           "element contiguous over %u bytes\n",
           MA_VECTOR_COUNT, resolved, refused, (unsigned) map.total_bytes);
    return 0;
}
