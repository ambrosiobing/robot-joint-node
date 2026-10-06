/* frame.h: the length code, and which frames the controller may be asked for.
 *
 * Chapter 9 step 4. The controller does not carry a length, it carries a four
 * bit code, and the code is not the length. Code 9 is twelve bytes, and the top
 * four codes step 24, 32, 48, 64. The same code also means different things in
 * the two formats: a classic frame has no length above eight and reads every
 * code above 8 as eight.
 *
 * A codec that gets this wrong does not fail loudly. It moves the right bytes
 * with the wrong count, or the wrong bytes with the right one, and the symptom
 * appears in whatever reads the payload.
 */
#ifndef JOINT_FRAME_H
#define JOINT_FRAME_H

#include <stdbool.h>
#include <stdint.h>

#define FRAME_CLASSIC_MAX 8u
#define FRAME_FD_MAX      64u

/* The three modes chapter 9 uses, in the order it uses them. Internal proves
 * the controller with no pins involved; external proves the pins with one
 * jumper wire; normal needs a transceiver and a second node. */
typedef enum {
    FRAME_MODE_INTERNAL_LOOPBACK,
    FRAME_MODE_EXTERNAL_LOOPBACK,
    FRAME_MODE_NORMAL,
} frame_mode_t;

typedef struct {
    uint32_t id;
    uint8_t  data[FRAME_FD_MAX];
    uint32_t len;
    bool     fd;    /* flexible-data format */
    bool     brs;   /* rate switch; meaningless without fd */
} frame_t;

/* Bytes a code stands for, in the given format. Returns -1 only for a code
 * outside 0 to 15; every code in range has a length in both formats. */
int32_t frame_length_for_code(uint32_t code, bool fd);

/* The code for a length, or -1 when the format cannot carry it.
 *
 * Returning -1 rather than the next code up is the point. Rounding up sends
 * bytes the caller never wrote; rounding down drops the tail. Both are silent. */
int32_t frame_code_for_length(uint32_t length, bool fd);

/* Whether this is a frame the controller may be asked to send. */
bool frame_ok(const frame_t *f);

#endif /* JOINT_FRAME_H */
