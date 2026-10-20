/* element.c: the layout, and the identifier format that is not a function of
 * the identifier.
 *
 * Every refusal here is a frame that would otherwise go onto a wire carrying
 * something other than what the caller meant.
 */
#include "element.h"

/* Bytes the wire will carry for this frame, which is the length CODE's length
 * and not the caller's. Returns a NEGATIVE value when the frame cannot be sent
 * at all.
 *
 * The refusal is negative rather than zero because ZERO IS A LEGAL ANSWER: an
 * empty frame carries no payload and is a perfectly good frame. The first
 * version of this file returned 0 for both, and told the two apart by asking
 * whether the caller's length was also zero. That reads as equivalent and is
 * not: a zero length frame asking for a rate switch without the flexible format
 * is refused by frame_ok, yet its length is zero, so both callers below treated
 * the refusal as an empty frame and built one. No vector covered a refused
 * frame of zero length, which is why it survived. There is one now. */
static int32_t wire_bytes(const frame_t *f, int32_t *code_out)
{
    if (f == 0 || !frame_ok(f)) {
        return -1;
    }
    const int32_t code = frame_code_for_length(f->len, f->fd);
    if (code < 0) {
        return -1;
    }
    const int32_t bytes = frame_length_for_code((uint32_t) code, f->fd);
    if (bytes < 0) {
        return -1;
    }
    if (code_out != 0) {
        *code_out = code;
    }
    return bytes;
}

uint32_t elem_words_for(const frame_t *f)
{
    const int32_t bytes = wire_bytes(f, 0);
    if (bytes < 0) {
        return 0u;
    }
    /* Round up: a five byte payload still occupies two words, and the length
     * code guarantees bytes is one of the legal sizes, so this is exact. An
     * empty frame returns the two header words and no payload word, which is
     * what it occupies. */
    return ELEM_HEADER_WORDS + (((uint32_t) bytes + 3u) / 4u);
}

uint32_t elem_build_tx(const frame_t *f, bool extended, uint32_t marker,
                       bool want_event, uint32_t *words, uint32_t max_words)
{
    if (f == 0 || words == 0) {
        return 0u;
    }
    if (marker > ELEM_T1_MM_MASK) {
        return 0u;
    }

    int32_t code = 0;
    const int32_t bytes = wire_bytes(f, &code);
    if (bytes < 0) {
        return 0u;
    }

    const uint32_t need = ELEM_HEADER_WORDS + (((uint32_t) bytes + 3u) / 4u);
    if (need > max_words || need > ELEM_MAX_WORDS) {
        return 0u;
    }

    /* The identifier. The format is the caller's choice, not a function of the
     * value, because 0x123 standard and 0x123 extended are different frames and
     * a receiver filtering for one does not see the other. Refused when the
     * value will not fit the format asked for, rather than quietly promoting it
     * to the other format. */
    uint32_t w0;
    if (extended) {
        if (f->id > ELEM_ID_EXT_MASK) {
            return 0u;
        }
        w0 = ELEM_W0_XTD | (f->id & ELEM_ID_EXT_MASK);
    } else {
        if (f->id > ELEM_ID_STD_MASK) {
            return 0u;
        }
        w0 = (f->id & ELEM_ID_STD_MASK) << ELEM_ID_STD_SHIFT;
    }

    uint32_t w1 = ((uint32_t) code & ELEM_DLC_MASK) << ELEM_DLC_SHIFT;
    w1 |= (marker & ELEM_T1_MM_MASK) << ELEM_T1_MM_SHIFT;
    if (want_event) {
        w1 |= ELEM_T1_EFC;
    }
    if (f->fd) {
        w1 |= ELEM_W1_FDF;
        /* A rate switch without the flexible format is not a thing the
         * controller can send, so it is dropped rather than written. frame_ok
         * is where that is decided; this only refuses to encode it. */
        if (f->brs) {
            w1 |= ELEM_W1_BRS;
        }
    }

    words[0] = w0;
    words[1] = w1;

    /* The payload, four bytes to a word, least significant byte first, and
     * every byte the CODE asks for rather than every byte the caller gave.
     * Those are the same number under this chapter's exact-length policy, so
     * the zero fill below only ever reaches the odd bytes of a final partial
     * word. It is kept because the day that policy changes is the day it is
     * needed, and the test asserts the equality so the change is not silent. */
    for (uint32_t i = 0u; i < (uint32_t) bytes; i += 4u) {
        uint32_t w = 0u;
        for (uint32_t b = 0u; b < 4u; b++) {
            const uint32_t at = i + b;
            const uint32_t v = (at < f->len) ? f->data[at] : 0u;
            w |= v << (8u * b);
        }
        words[ELEM_HEADER_WORDS + (i / 4u)] = w;
    }

    return need;
}

bool elem_parse_rx(const uint32_t *words, uint32_t word_count, frame_t *out,
                   bool *extended, uint32_t *timestamp, bool *anmf)
{
    if (words == 0 || out == 0 || word_count < ELEM_HEADER_WORDS) {
        return false;
    }

    const uint32_t w0 = words[0];
    const uint32_t w1 = words[1];

    /* A remote frame carries no data and this chapter never sends one, so it is
     * refused rather than silently returned with a length that means something
     * else. */
    if (w0 & ELEM_W0_RTR) {
        return false;
    }

    const bool fd = (w1 & ELEM_W1_FDF) != 0u;
    const uint32_t code = (w1 >> ELEM_DLC_SHIFT) & ELEM_DLC_MASK;
    const int32_t bytes = frame_length_for_code(code, fd);
    if (bytes < 0) {
        return false;
    }

    const uint32_t need = ELEM_HEADER_WORDS + (((uint32_t) bytes + 3u) / 4u);
    if (need > word_count) {
        return false;       /* a short read would return a tail of whatever follows */
    }

    const bool xtd = (w0 & ELEM_W0_XTD) != 0u;
    out->id = xtd ? (w0 & ELEM_ID_EXT_MASK)
                  : ((w0 >> ELEM_ID_STD_SHIFT) & ELEM_ID_STD_MASK);
    if (extended != 0) {
        *extended = xtd;
    }
    out->fd = fd;
    out->brs = fd && ((w1 & ELEM_W1_BRS) != 0u);
    out->len = (uint32_t) bytes;

    for (uint32_t i = 0u; i < (uint32_t) bytes; i++) {
        const uint32_t w = words[ELEM_HEADER_WORDS + (i / 4u)];
        out->data[i] = (uint8_t) ((w >> (8u * (i % 4u))) & 0xFFu);
    }
    for (uint32_t i = (uint32_t) bytes; i < FRAME_FD_MAX; i++) {
        out->data[i] = 0u;
    }

    if (timestamp != 0) {
        *timestamp = w1 & ELEM_R1_RXTS_MASK;
    }
    if (anmf != 0) {
        *anmf = (w1 & ELEM_R1_ANMF) != 0u;
    }
    return true;
}
