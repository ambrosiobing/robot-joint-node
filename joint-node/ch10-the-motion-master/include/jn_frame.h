/* The frame as it sits on the wire, with no socket and no kernel header.
 *
 * This half of the chapter is deliberately portable: it builds on any C11
 * compiler, so the layout, the identifier flags and the length rules can be
 * tested on a machine that has never heard of SocketCAN. The socket lives in
 * jn_bus.h and is the only part that needs Linux.
 *
 * The buffer layouts are the kernel's own:
 *
 *   struct can_frame     16 bytes:  id u32, len u8, pad u8, res0 u8, res1 u8, data[8]
 *   struct canfd_frame   72 bytes:  id u32, len u8, flags u8, res0 u8, res1 u8, data[64]
 *
 * They are written and read one byte at a time rather than by copying a struct,
 * so nothing here depends on this compiler's padding or on the host byte order.
 * The identifier carries three flag bits above the number itself, which is why
 * an identifier is never compared without masking it first.
 */
#ifndef JN_FRAME_H
#define JN_FRAME_H

#include <stddef.h>
#include <stdint.h>

/* Flags that ride in the top bits of the identifier word on the wire. */
#define JN_EFF_FLAG  0x80000000u   /* 29 bit identifier rather than 11 */
#define JN_RTR_FLAG  0x40000000u   /* remote request, no payload */
#define JN_ERR_FLAG  0x20000000u   /* an error frame from the driver, not a node */

#define JN_SFF_MASK  0x000007FFu
#define JN_EFF_MASK  0x1FFFFFFFu

/* Flags in the flexible-data frame's own byte. */
#define JN_FD_BRS    0x01u         /* the data phase runs at the faster rate */
#define JN_FD_ESI    0x02u         /* the sender is error passive */
#define JN_FD_FDF    0x04u         /* this is a flexible-data frame */

/* Flags in jn_frame_t. These are this library's own and not the wire's: the
 * wire splits the same information across the identifier word and the flags
 * byte, and keeping one set in the structure means the caller never has to
 * remember which lives where. */
#define JN_F_FD      0x01u
#define JN_F_BRS     0x02u
#define JN_F_ESI     0x04u
#define JN_F_EXT     0x08u

#define JN_CLASSIC_MTU  16u
#define JN_FD_MTU       72u
#define JN_MAX_DLEN     64u

/* Enough for the longest printed frame: eight identifier digits, two separator
 * characters, one flags digit, 128 payload digits and the terminator. */
#define JN_FORMAT_MAX  140u

/* Timestamps are carried as whole nanoseconds rather than as a floating point
 * number of seconds. Six printed digits of a 2026 timestamp is about seventeen
 * significant figures, which a double does not have, so a double round trip
 * loses the last digit or two. Integers lose nothing. */
#define JN_NS_PER_SEC  1000000000ull

/* Above eight bytes the payload length jumps. A frame is built to land on one
 * of these, and a length between them cannot be put on a wire at all. */
extern const uint8_t jn_fd_lengths[16];

typedef enum {
    JN_OK        =  0,
    JN_E_ID      = -1,   /* identifier too wide for its addressing mode */
    JN_E_LEN     = -2,   /* a length no frame can carry */
    JN_E_BRS     = -3,   /* a rate switch asked for on a classic frame */
    JN_E_BUF     = -4,   /* the caller's buffer is too small */
    JN_E_PARSE   = -5,   /* text that is not a frame */
    JN_E_NOSYS   = -6,   /* this build has no SocketCAN */
    JN_E_SYS     = -7,   /* the system call failed, errno says how */
    JN_E_ARG     = -8,   /* an argument no call could honour */
    JN_E_TIMEOUT = -9    /* nothing arrived inside the time allowed */
} jn_err_t;

/* A sentence, not a code. Every path that returns a negative value is meant to
 * be reportable without the caller owning a table of its own. */
const char *jn_strerror(int err);

typedef struct {
    uint32_t id;               /* the number alone: 11 or 29 bits, no flags */
    uint8_t  len;              /* payload bytes actually present */
    uint8_t  flags;            /* JN_F_* above */
    uint8_t  data[JN_MAX_DLEN];
    uint64_t stamp_ns;         /* 0 when the frame carries no timestamp */
} jn_frame_t;

/* Non-zero when the length is one a flexible-data frame may carry. */
int jn_len_allowed(size_t len);

/* Fill a frame, refusing anything that could not exist on a wire. Returns
 * JN_OK, or a negative jn_err_t with the frame left untouched. The stamp is
 * zeroed; set it afterwards if the frame has one. */
int jn_frame_init(jn_frame_t *f, uint32_t id, const void *data, size_t len,
                  uint8_t flags);

/* The identifier as it appears on the wire, with the extended bit set when the
 * frame is extended. This is the value a kernel filter is compared against, and
 * the reason a standard frame and an extended frame numbered alike are two
 * different frames. */
uint32_t jn_frame_wire_id(const jn_frame_t *f);

/* Write the frame into buf. Returns the number of bytes written, 16 or 72, or a
 * negative jn_err_t. */
int jn_frame_encode(const jn_frame_t *f, uint8_t *buf, size_t cap);

/* Read a frame out of a buffer the kernel handed over. The length decides which
 * layout it is, which is exactly how the kernel tells them apart. */
int jn_frame_decode(jn_frame_t *f, const uint8_t *buf, size_t len);

/* The shape the standard tools print, so a line from here and a line from
 * candump can be compared with no translation step between them:
 *
 *     123#DEAD              a classic frame
 *     1A3##1112233          a flexible-data frame, rate switch set
 *     00000456#01           an extended frame, always eight digits
 *
 * Returns the length written, not counting the terminator, or a negative
 * jn_err_t. JN_FORMAT_MAX bytes is always enough. */
int jn_frame_format(const jn_frame_t *f, char *out, size_t cap);

/* Non-zero when the identifier, the flags and every payload byte match. The
 * stamp is deliberately not compared: two recordings of one frame differ in
 * when they were taken and in nothing else. */
int jn_frame_equal(const jn_frame_t *a, const jn_frame_t *b);

#endif /* JN_FRAME_H */
