/* test_element.c: the C layout against the generated vectors.
 *
 * The vectors come from tools/gen_element.py, which is the reference. The two
 * are never compared against each other: both are compared against the vector
 * file, so a disagreement names the word and the frame rather than leaving
 * somebody to guess which side moved.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written.
 */
#include <stdio.h>
#include <string.h>

#include "element.h"
#include "msgram.h"
#include "vectors_element.h"

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
    int built = 0, refusals = 0;

    for (int i = 0; i < EL_VECTOR_COUNT; i++) {
        const el_vector_t *v = &EL_VECTORS[i];

        frame_t f;
        memset(&f, 0, sizeof f);
        f.id = v->id;
        f.len = v->len;
        f.fd = v->fd;
        f.brs = v->brs;
        for (uint32_t b = 0u; b < v->len && b < FRAME_FD_MAX; b++) {
            f.data[b] = v->data[b];
        }

        uint32_t words[ELEM_MAX_WORDS];
        memset(words, 0, sizeof words);
        const uint32_t n = elem_build_tx(&f, v->extended, v->marker,
                                         v->want_event, words,
                                         ELEM_MAX_WORDS);

        /* elem_words_for is a function of the FRAME, so it is checked for
         * every case including the refused ones, and it is checked BEFORE the
         * build, so a build that refuses cannot excuse a sizing that does not.
         * The two disagreed on a zero length frame refused for a reason other
         * than its length, and what that costs is a caller that sizes a buffer
         * with one and fills it with the other. A marker above 255 and a buffer
         * too small refuse a build and leave the frame legal, which is why the
         * vectors carry both answers rather than one. */
        check_u32(v->label, "elem_words_for", elem_words_for(&f),
                  v->words_for);
        if ((elem_words_for(&f) != 0u) != v->frame_ok) {
            printf("  FAIL %s: elem_words_for %s the frame, and frame_ok says "
                   "the frame is %s\n", v->label,
                   elem_words_for(&f) ? "sized" : "refused",
                   v->frame_ok ? "legal" : "illegal");
            failures++;
        }

        if ((n != 0u) != v->ok) {
            printf("  FAIL %s: %s, and the vectors say %s\n", v->label,
                   n ? "built" : "refused",
                   v->ok ? "it builds" : "it is refused");
            failures++;
            continue;
        }
        if (!v->ok) {
            refusals++;
            continue;
        }
        built++;

        if (n != v->word_count) {
            printf("  FAIL %s: %u words, the vectors say %u\n", v->label,
                   (unsigned) n, (unsigned) v->word_count);
            failures++;
            continue;       /* comparing further would print noise */
        }
        for (uint32_t w = 0u; w < n; w++) {
            char field[32];
            snprintf(field, sizeof field, "word %u", (unsigned) w);
            check_u32(v->label, field, words[w], v->words[w]);
        }

        /* And for a frame that did build, the sizing must equal what was
         * actually written, not merely be non-zero. */
        check_u32(v->label, "elem_words_for against the words written",
                  elem_words_for(&f), n);

        /* The round trip, including the format, because a standard and an
         * extended frame carrying the same number are different frames. */
        frame_t back;
        memset(&back, 0, sizeof back);
        bool xtd = false;
        uint32_t ts = 0xFFFFFFFFu;
        bool anmf = true;
        if (!elem_parse_rx(words, n, &back, &xtd, &ts, &anmf)) {
            printf("  FAIL %s: built but does not parse\n", v->label);
            failures++;
            continue;
        }
        check_u32(v->label, "id round trip", back.id, v->id);
        check_u32(v->label, "length round trip", back.len, v->parsed_len);
        if (xtd != v->extended) {
            printf("  FAIL %s: extended went in as %s and came back as %s\n",
                   v->label, v->extended ? "true" : "false",
                   xtd ? "true" : "false");
            failures++;
        }
        if (back.fd != v->fd || back.brs != v->brs) {
            printf("  FAIL %s: format flags did not survive the round trip\n",
                   v->label);
            failures++;
        }
        for (uint32_t b = 0u; b < v->len; b++) {
            if (back.data[b] != v->data[b]) {
                printf("  FAIL %s: byte %u went in as 0x%02X and came back as "
                       "0x%02X\n", v->label, (unsigned) b,
                       (unsigned) v->data[b], (unsigned) back.data[b]);
                failures++;
                break;
            }
        }

        /* A short read must be refused rather than returning a tail of
         * whatever followed the element in the message memory. */
        if (n > ELEM_HEADER_WORDS) {
            frame_t tail;
            memset(&tail, 0, sizeof tail);
            if (elem_parse_rx(words, n - 1u, &tail, 0, 0, 0)) {
                printf("  FAIL %s: an element one word short of its own "
                       "length code was parsed rather than refused\n",
                       v->label);
                failures++;
            }
        }
    }

    /* ------------------------------------------------------- the invariants */

    /* The same number in the two formats must produce different words. The
     * first draft chose the format from the magnitude, which made every small
     * extended identifier unreachable and did so silently. */
    {
        frame_t f;
        memset(&f, 0, sizeof f);
        f.id = 0x123u;
        f.len = 1u;
        f.data[0] = 1u;

        uint32_t a[ELEM_MAX_WORDS], b[ELEM_MAX_WORDS];
        memset(a, 0, sizeof a);
        memset(b, 0, sizeof b);
        const uint32_t na = elem_build_tx(&f, false, 0u, false, a,
                                          ELEM_MAX_WORDS);
        const uint32_t nb = elem_build_tx(&f, true, 0u, false, b,
                                          ELEM_MAX_WORDS);
        if (na == 0u || nb == 0u) {
            printf("  FAIL 0x123 did not build in both formats\n");
            failures++;
        } else if (a[0] == b[0]) {
            printf("  FAIL 0x123 standard and 0x123 extended produced the "
                   "same first word, so one of the two is unreachable\n");
            failures++;
        }
        if (nb != 0u && !(b[0] & ELEM_W0_XTD)) {
            printf("  FAIL an extended frame did not set the extended bit\n");
            failures++;
        }
        if (na != 0u && (a[0] & ELEM_W0_XTD)) {
            printf("  FAIL a standard frame set the extended bit\n");
            failures++;
        }
        if (na != 0u && (a[0] & ((1u << ELEM_ID_STD_SHIFT) - 1u)) != 0u) {
            printf("  FAIL a standard identifier was not shifted up\n");
            failures++;
        }
    }

    /* An identifier that does not fit the format asked for must be refused
     * rather than quietly promoted to the other format. */
    {
        frame_t f;
        memset(&f, 0, sizeof f);
        f.id = 0x800u;          /* twelve bits, too wide for standard */
        f.len = 1u;
        uint32_t w[ELEM_MAX_WORDS];
        memset(w, 0, sizeof w);
        if (elem_build_tx(&f, false, 0u, false, w, ELEM_MAX_WORDS) != 0u) {
            printf("  FAIL 0x800 was accepted as a standard identifier\n");
            failures++;
        }
        f.id = 0x20000000u;     /* thirty bits, too wide for extended */
        if (elem_build_tx(&f, true, 0u, false, w, ELEM_MAX_WORDS) != 0u) {
            printf("  FAIL 0x20000000 was accepted as an extended "
                   "identifier\n");
            failures++;
        }
    }

    /* The payload is little endian within each word, probed with four bytes
     * that are all different so a reversal cannot hide. */
    {
        frame_t f;
        memset(&f, 0, sizeof f);
        f.id = 1u;
        f.len = 4u;
        f.data[0] = 0x11u; f.data[1] = 0x22u;
        f.data[2] = 0x33u; f.data[3] = 0x44u;
        uint32_t w[ELEM_MAX_WORDS];
        memset(w, 0, sizeof w);
        if (elem_build_tx(&f, false, 0u, false, w, ELEM_MAX_WORDS) == 0u) {
            printf("  FAIL the endianness probe refused to build\n");
            failures++;
        } else if (w[ELEM_HEADER_WORDS] != 0x44332211u) {
            printf("  FAIL bytes 11 22 33 44 packed to 0x%08X, and little "
                   "endian is 0x44332211\n",
                   (unsigned) w[ELEM_HEADER_WORDS]);
            failures++;
        }
    }

    /* The largest element and the message memory must agree about how much
     * room a frame needs, or the layout reserves the wrong amount. */
    if (ELEM_MAX_WORDS * 4u != MSGRAM_FRAME_BYTES) {
        printf("  FAIL element.h sizes a frame at %u bytes and msgram.h at "
               "%u\n", (unsigned) (ELEM_MAX_WORDS * 4u),
               (unsigned) MSGRAM_FRAME_BYTES);
        failures++;
    }

    /* A buffer too small must be refused rather than overrun. */
    {
        frame_t f;
        memset(&f, 0, sizeof f);
        f.id = 1u;
        f.len = 8u;
        uint32_t w[ELEM_MAX_WORDS];
        memset(w, 0, sizeof w);
        const uint32_t full = elem_build_tx(&f, false, 0u, false, w,
                                            ELEM_MAX_WORDS);
        if (full == 0u) {
            printf("  FAIL an eight byte frame did not build\n");
            failures++;
        } else if (elem_build_tx(&f, false, 0u, false, w, full - 1u) != 0u) {
            printf("  FAIL a %u word element fitted into %u words\n",
                   (unsigned) full, (unsigned) (full - 1u));
            failures++;
        }
    }

    if (failures) {
        printf("test_element: %d failure(s) over %d frames\n",
               failures, EL_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d frames, %d built and %d refused, every round trip exact\n",
           EL_VECTOR_COUNT, built, refusals);
    return 0;
}
