/* test_frame.c: the C length code against the generated vectors.
 *
 * Built in continuous integration, and on the WSL side of win11 skyhorizon,
 * with -std=c11 -Wall -Wextra -Werror. Never built on the authoring laptop.
 */
#include <stdio.h>

#include "frame.h"
#include "vectors_frame.h"

static int failures;

static void check(const char *what, int32_t got, int32_t want)
{
    if (got != want) {
        printf("  FAIL %s: got %d, the vectors say %d\n",
               what, (int) got, (int) want);
        failures++;
    }
}

int main(void)
{
    char label[96];

    for (int i = 0; i < FRAME_CODE_VECTOR_COUNT; i++) {
        const frame_code_vector_t *v = &FRAME_CODE_VECTORS[i];
        snprintf(label, sizeof label, "code %u, classic", (unsigned) v->code);
        check(label, frame_length_for_code(v->code, false), v->classic_length);
        snprintf(label, sizeof label, "code %u, flexible-data",
                 (unsigned) v->code);
        check(label, frame_length_for_code(v->code, true), v->fd_length);
    }

    for (int i = 0; i < FRAME_LENGTH_VECTOR_COUNT; i++) {
        const frame_length_vector_t *v = &FRAME_LENGTH_VECTORS[i];
        snprintf(label, sizeof label, "length %u, classic",
                 (unsigned) v->length);
        check(label, frame_code_for_length(v->length, false), v->classic_code);
        snprintf(label, sizeof label, "length %u, flexible-data",
                 (unsigned) v->length);
        check(label, frame_code_for_length(v->length, true), v->fd_code);
    }

    /* The round trip, which is the property the payload actually depends on:
     * a length that has a code must come back as the same length, never as a
     * neighbouring one. */
    int round_trips = 0;
    for (uint32_t len = 0u; len <= FRAME_FD_MAX; len++) {
        const int32_t code = frame_code_for_length(len, true);
        if (code < 0) {
            continue;
        }
        const int32_t back = frame_length_for_code((uint32_t) code, true);
        if (back != (int32_t) len) {
            printf("  FAIL %u bytes became code %d and came back as %d\n",
                   (unsigned) len, (int) code, (int) back);
            failures++;
        }
        round_trips++;
    }
    if (round_trips != 16) {
        printf("  FAIL %d lengths round tripped and the format has 16\n",
               round_trips);
        failures++;
    }

    /* A code above 8 means two different things in the two formats, and that
     * is the trap. Assert the difference rather than leaving it implied. */
    for (uint32_t code = FRAME_CLASSIC_MAX + 1u; code <= 15u; code++) {
        if (frame_length_for_code(code, false) != (int32_t) FRAME_CLASSIC_MAX) {
            printf("  FAIL classic code %u did not clamp to 8\n",
                   (unsigned) code);
            failures++;
        }
        if (frame_length_for_code(code, true) <= (int32_t) FRAME_CLASSIC_MAX) {
            printf("  FAIL flexible-data code %u is not above 8\n",
                   (unsigned) code);
            failures++;
        }
    }

    /* A code outside the four bits it lives in. */
    check("code 16, classic", frame_length_for_code(16u, false), -1);
    check("code 16, flexible-data", frame_length_for_code(16u, true), -1);

    int refused = 0;
    for (int i = 0; i < FRAME_VECTOR_COUNT; i++) {
        const frame_vector_t *v = &FRAME_VECTORS[i];
        frame_t f = { .id = 0x123u, .len = v->length, .fd = v->fd,
                      .brs = v->brs };
        const bool ok = frame_ok(&f);
        if (ok != v->ok) {
            printf("  FAIL %s: %s, the vectors say %s\n", v->label,
                   ok ? "accepted" : "refused",
                   v->ok ? "accepted" : "refused");
            failures++;
        }
        if (!v->ok) {
            refused++;
            continue;
        }
        snprintf(label, sizeof label, "%s code", v->label);
        check(label, frame_code_for_length(v->length, v->fd), v->code);
    }

    if (!frame_ok(0)) {
        /* A null frame is refused, which is the one case the vectors cannot
         * describe. */
    } else {
        printf("  FAIL a null frame was accepted\n");
        failures++;
    }

    if (failures) {
        printf("test_frame: %d failure(s)\n", failures);
        return 1;
    }
    printf("ok  16 codes both formats, 65 lengths, 16 round trips, "
           "%d frames with %d refused\n", FRAME_VECTOR_COUNT, refused);
    return 0;
}
