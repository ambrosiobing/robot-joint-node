/* frame.c: the length code in one place, so nothing else has to know it. */
#include "frame.h"

/* Code 0 to 15, as the flexible-data format reads them. The first nine are the
 * length itself; the rest are the only lengths above eight the format has. */
static const uint8_t FD_LENGTH_FOR_CODE[16] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 12u, 16u, 20u, 24u, 32u, 48u, 64u,
};

int32_t frame_length_for_code(uint32_t code, bool fd)
{
    if (code > 15u) {
        return -1;
    }
    if (fd) {
        return (int32_t) FD_LENGTH_FOR_CODE[code];
    }
    /* A classic frame reads every code above 8 as 8. This is not a courtesy:
     * it is what the format says, and a receiver that treats code 12 as 24
     * bytes on a classic frame reads sixteen bytes that were never sent. */
    return (int32_t) (code <= FRAME_CLASSIC_MAX ? code : FRAME_CLASSIC_MAX);
}

int32_t frame_code_for_length(uint32_t length, bool fd)
{
    if (!fd) {
        return length <= FRAME_CLASSIC_MAX ? (int32_t) length : -1;
    }
    for (uint32_t c = 0u; c < 16u; c++) {
        if ((uint32_t) FD_LENGTH_FOR_CODE[c] == length) {
            return (int32_t) c;
        }
    }
    return -1;
}

bool frame_ok(const frame_t *f)
{
    if (f == 0) {
        return false;
    }
    /* The rate switch is a flexible-data feature. A classic frame runs at one
     * rate for its whole length and has nowhere to record a second. */
    if (f->brs && !f->fd) {
        return false;
    }
    return frame_code_for_length(f->len, f->fd) >= 0;
}
