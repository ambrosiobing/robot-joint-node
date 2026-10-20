/* msgaddr.h: from an element index to the place that element lives.
 *
 * Chapter 9 stage two. msgram.h says where each section starts and how big it
 * is. element.h says what the words of one element mean. Neither says where
 * element seven of the transmit buffers is, and every access to the message
 * memory needs exactly that.
 *
 * It is four lines of arithmetic, which is why it is tempting to write those
 * four lines at each call site. THREE THINGS GO WRONG WHEN IT IS, and the
 * layout this chapter actually configures contains the worst of them.
 *
 *   - AN EMPTY SECTION HAS A PERFECTLY GOOD OFFSET. MSGRAM_N_RX_BUFFERS is 0 in
 *     this node, so the dedicated receive buffer section begins exactly where
 *     the transmit event section begins. Ask for receive buffer 0 and naive
 *     arithmetic hands back that offset without complaint. Write there and the
 *     first transmit event is overwritten; read there and an event comes back
 *     shaped like a frame. Neither is reported by anything.
 *
 *   - THE SECTIONS DO NOT SHARE AN ELEMENT SIZE. A standard filter is one word,
 *     an extended filter and a transmit event are two, a frame is eighteen.
 *     Index times the wrong size is an address inside a neighbouring element,
 *     so a frame is read starting partway through the one before it.
 *
 *   - AN OFFSET IS NOT AN ADDRESS. The start-address registers take offsets
 *     from the message memory base; a pointer needs the base added. The two are
 *     different numbers for the same element and both look reasonable. The
 *     register fields were the first place this chapter got that wrong, which
 *     is why the two are separate functions here with the word in their names
 *     rather than one function and a comment.
 *
 * The index is checked against THE LAYOUT'S count, not against the part's
 * maximum. The maximum is what the silicon accepts; the count is what was
 * configured, and memory past the count belongs to the next section whether the
 * part would have accepted a larger count or not.
 */
#ifndef JOINT_MSGADDR_H
#define JOINT_MSGADDR_H

#include <stdbool.h>
#include <stdint.h>

#include "msgram.h"

/* The seven sections, in the order the message memory holds them. The order is
 * not arbitrary: msgram_compute lays them out in it, and a section's offset
 * depends on every section before it. */
typedef enum {
    MSGRAM_SEC_STD_FILTERS = 0,
    MSGRAM_SEC_EXT_FILTERS,
    MSGRAM_SEC_RX_FIFO0,
    MSGRAM_SEC_RX_FIFO1,
    MSGRAM_SEC_RX_BUFFERS,
    MSGRAM_SEC_TX_EVENT,
    MSGRAM_SEC_TX_BUFFERS,
    MSGRAM_SEC_COUNT
} msgram_section_t;

/* The name of a section, for a message a person has to read. Never NULL: an
 * unknown section returns a string saying so rather than a null pointer that
 * turns a diagnostic into a fault. */
const char *msgaddr_section_name(msgram_section_t section);

/* Bytes one element of this section occupies, or 0 for an unknown section.
 *
 * An enum argument is range checked as UNSIGNED and only against the upper
 * bound. On ARM EABI a plain enum is unsigned, so `section < 0` is dead code
 * there and a -Werror failure besides. */
uint32_t msgaddr_element_bytes(msgram_section_t section);

/* How many elements of this section the layout configured, or 0 for an unknown
 * section. Zero is a real answer: this node configures no dedicated receive
 * buffers at all. */
uint32_t msgaddr_count(const msgram_layout_t *lay, msgram_section_t section);

/* The OFFSET FROM THE MESSAGE MEMORY BASE of element `index` of `section`.
 *
 * This is the number the start-address registers hold. It is not an address.
 *
 * Returns false, and leaves *out alone, when the section is unknown, when the
 * layout configured no elements of it, or when the index is at or past the
 * count. An empty section refuses every index INCLUDING ZERO, which is the
 * whole reason this function exists rather than a multiply at each call site:
 * the offset it would otherwise return is the first element of the next
 * section, and it is a plausible number that points at the wrong thing.
 */
bool msgaddr_offset(const msgram_layout_t *lay, const msgram_map_t *map,
                    msgram_section_t section, uint32_t index, uint32_t *out);

/* The ABSOLUTE ADDRESS of the same element, given the base of the message
 * memory. Refuses everything msgaddr_offset refuses, and also refuses when the
 * base plus the offset would wrap, because an address computed past the end of
 * the address space is not an address the caller can be allowed to write to.
 */
bool msgaddr_address(const msgram_layout_t *lay, const msgram_map_t *map,
                     msgram_section_t section, uint32_t index, uint32_t base,
                     uint32_t *out);

#endif /* JOINT_MSGADDR_H */
