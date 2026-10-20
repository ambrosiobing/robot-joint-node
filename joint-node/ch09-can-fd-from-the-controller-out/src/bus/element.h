/* element.h: a frame as the message memory actually holds it.
 *
 * Chapter 9 stage two. frame.h knows the length code. msgram.h knows where the
 * sections live. Neither turns a frame into the words the controller reads, and
 * that is this file.
 *
 * An element is two header words followed by the payload. The header carries the
 * identifier, the format flags and the length CODE, never the length. The
 * payload is packed four bytes to a word, least significant byte first.
 *
 * THREE THINGS HERE FAIL SILENTLY, which is why they are a separate file with
 * their own vectors rather than four lines inside a driver.
 *
 *   - The identifier sits in different bits depending on the format. A standard
 *     identifier is shifted up by eighteen; an extended one is not. Putting an
 *     eleven bit identifier in the extended position sends a frame whose
 *     arbitration field is a different and much lower priority number, which
 *     does not error, it just loses every arbitration it should win.
 *
 *   - The length written is a CODE, and only some lengths have one. Nine bytes
 *     has no code, so this chapter REFUSES nine bytes rather than padding them
 *     to twelve. The controller end does pad: chapter 10 measured nine bytes
 *     arriving as twelve with three zero bytes on the real bus. Two ends of one
 *     bus with two policies, and the difference is deliberate at this end,
 *     because padding sends bytes the caller never wrote.
 *
 *   - The payload is little endian within each word. Getting that backwards
 *     delivers every four byte group reversed, which looks like data corruption
 *     rather than like a layout error.
 *
 * THIS FILE NAMES NO VENDOR HEADER AND MUST NOT, by chapter 4's rule. The bit
 * positions below are from the mainline Linux driver for this peripheral, the
 * Bosch M_CAN, because the element format is Bosch's rather than any one
 * vendor's.
 */
#ifndef JOINT_ELEMENT_H
#define JOINT_ELEMENT_H

#include <stdbool.h>
#include <stdint.h>

#include "frame.h"

/* ------------------------------------------------------- the first word, T0 and R0 */
#define ELEM_W0_ESI        (1u << 31)   /* error state indicator */
#define ELEM_W0_XTD        (1u << 30)   /* extended identifier */
#define ELEM_W0_RTR        (1u << 29)   /* remote frame */

/* An extended identifier occupies bits 28 to 0. A standard identifier is eleven
 * bits and sits at 28 to 18, so it is shifted up by eighteen. */
#define ELEM_ID_EXT_MASK   0x1FFFFFFFu
#define ELEM_ID_STD_MASK   0x000007FFu
#define ELEM_ID_STD_SHIFT  18u

/* ------------------------------------------------------ the second word, T1 and R1 */
#define ELEM_T1_MM_SHIFT   24u          /* message marker, transmit only */
#define ELEM_T1_MM_MASK    0xFFu
#define ELEM_T1_EFC        (1u << 23)   /* store a transmit event, transmit only */
#define ELEM_W1_FDF        (1u << 21)   /* flexible data format */
#define ELEM_W1_BRS        (1u << 20)   /* bit rate switch */
#define ELEM_DLC_SHIFT     16u
#define ELEM_DLC_MASK      0x0Fu

#define ELEM_R1_ANMF       (1u << 31)   /* accepted non matching frame, receive only */
#define ELEM_R1_RXTS_MASK  0x0000FFFFu  /* receive timestamp, receive only */

/* Two header words, then the payload packed four bytes to a word. A sixty-four
 * byte payload is sixteen words, so the largest element is eighteen, which is
 * the figure msgram.h uses for its frame sections. The two agree on purpose and
 * the test checks that they still do. */
#define ELEM_HEADER_WORDS  2u
#define ELEM_MAX_WORDS     (ELEM_HEADER_WORDS + (FRAME_FD_MAX / 4u))

/* Build a transmit element.
 *
 * Writes the two header words and as many payload words as the LENGTH CODE
 * requires. Under this chapter's exact-length policy that is always the
 * caller's own length rounded up to a whole word, because a length with no code
 * is refused rather than padded. The fill past the caller's data is therefore
 * defensive: it only ever writes the odd bytes of a final partial word, and the
 * test asserts that the code's length equals the caller's so that this stops
 * being true loudly if the policy ever changes.
 *
 * Returns the number of WORDS written, or 0 when the frame is refused. A caller
 * that ignores a 0 and transmits anyway sends whatever was in the buffer.
 *
 * `extended` chooses the identifier format, and it is a parameter rather than
 * something deduced from the value because THE TWO ARE DIFFERENT FRAMES EVEN
 * FOR THE SAME NUMBER. Identifier 0x123 sent standard and identifier 0x123 sent
 * extended are distinct on the wire and a receiver filtering for one will not
 * see the other. Deducing the format from the magnitude, which the first draft
 * of this file did, makes the small extended identifiers unreachable and the
 * mistake is invisible: the frame goes out, it simply is not the frame asked
 * for. Refused when the value does not fit the format chosen.
 *
 * `marker` is the message marker echoed back in the transmit event, 0 to 255.
 * `want_event` sets EFC so that the transmission is recorded in the event FIFO.
 */
uint32_t elem_build_tx(const frame_t *f, bool extended, uint32_t marker,
                       bool want_event, uint32_t *words, uint32_t max_words);

/* Parse a receive element.
 *
 * `word_count` is how many words the caller actually read. The function works
 * out how many the element needs from its own length code and refuses rather
 * than reading past what it was given, because a short read returns a frame
 * whose tail is whatever followed it in the message memory.
 *
 * `extended` may be NULL, but a caller that discards it cannot tell a standard
 * frame from an extended one carrying the same number, and those are different
 * frames.
 *
 * `timestamp` may be NULL. `anmf`, when not NULL, reports whether the frame was
 * accepted because nothing matched rather than because a filter did, which is
 * the normal case in this chapter since no filter element is ever written.
 */
bool elem_parse_rx(const uint32_t *words, uint32_t word_count, frame_t *out,
                   bool *extended, uint32_t *timestamp, bool *anmf);

/* How many words an element carrying this frame occupies, or 0 if the FRAME is
 * refused. Separate from building one so a caller can size a buffer or walk a
 * FIFO without composing anything.
 *
 * A function of the frame alone. A message marker above 255 and a buffer one
 * word too small both make elem_build_tx refuse while leaving the frame itself
 * legal, so this reports a size for them; what it refuses is what frame_ok
 * refuses, and nothing else.
 *
 * ZERO IS A LEGAL ANSWER one layer down, which is the trap: an empty frame
 * occupies the two header words and no payload word. The first version of
 * element.c returned 0 from its internal helper for both "no payload" and
 * "refused", and told them apart afterwards by asking whether the caller's
 * length was zero too. A zero length frame refused for any other reason then
 * came back SIZED here while elem_build_tx refused it, which is the exact
 * disagreement a caller sizing with one and filling with the other cannot
 * survive. The internal refusal is negative now, and the vectors carry a legal
 * empty frame and a refused one so the pair is reachable. */
uint32_t elem_words_for(const frame_t *f);

#endif /* JOINT_ELEMENT_H */
