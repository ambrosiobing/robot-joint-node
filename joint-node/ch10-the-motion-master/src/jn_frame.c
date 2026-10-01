#include "jn_frame.h"

#include <string.h>

const uint8_t jn_fd_lengths[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};

static const char HEX[] = "0123456789ABCDEF";

const char *jn_strerror(int err)
{
    switch (err) {
    case JN_OK:        return "ok";
    case JN_E_ID:      return "the identifier does not fit its addressing mode";
    case JN_E_LEN:     return "not a length any frame can carry";
    case JN_E_BRS:     return "a classic frame has no rate switch to set";
    case JN_E_BUF:     return "the buffer is too small";
    case JN_E_PARSE:   return "not a frame";
    case JN_E_NOSYS:   return "this build has no SocketCAN";
    case JN_E_SYS:     return "the system call failed";
    case JN_E_ARG:     return "an argument no call could honour";
    case JN_E_TIMEOUT: return "nothing arrived inside the time allowed";
    default:           return "an error with no description, which is a defect here";
    }
}

int jn_len_allowed(size_t len)
{
    size_t i;

    for (i = 0; i < sizeof jn_fd_lengths; i++) {
        if ((size_t) jn_fd_lengths[i] == len) {
            return 1;
        }
    }
    return 0;
}

/* Little endian by hand. The wire is little endian whatever this host is, and
 * writing it out makes that a property of the code rather than of the machine
 * it happened to be compiled on. */
static void put_u32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t) (v & 0xFFu);
    p[1] = (uint8_t) ((v >> 8) & 0xFFu);
    p[2] = (uint8_t) ((v >> 16) & 0xFFu);
    p[3] = (uint8_t) ((v >> 24) & 0xFFu);
}

static uint32_t get_u32le(const uint8_t *p)
{
    return (uint32_t) p[0]
         | ((uint32_t) p[1] << 8)
         | ((uint32_t) p[2] << 16)
         | ((uint32_t) p[3] << 24);
}

int jn_frame_init(jn_frame_t *f, uint32_t id, const void *data, size_t len,
                  uint8_t flags)
{
    uint32_t limit = ((flags & JN_F_EXT) != 0u) ? JN_EFF_MASK : JN_SFF_MASK;

    if (f == NULL) {
        return JN_E_ARG;
    }
    if (id > limit) {
        return JN_E_ID;
    }
    if ((flags & JN_F_FD) != 0u) {
        if (!jn_len_allowed(len)) {
            return JN_E_LEN;
        }
    } else {
        if (len > 8u) {
            return JN_E_LEN;
        }
        if ((flags & JN_F_BRS) != 0u) {
            return JN_E_BRS;
        }
    }
    if (len > 0u && data == NULL) {
        return JN_E_ARG;
    }

    memset(f, 0, sizeof *f);
    f->id = id;
    f->len = (uint8_t) len;
    f->flags = flags;
    if (len > 0u) {
        memcpy(f->data, data, len);
    }
    return JN_OK;
}

uint32_t jn_frame_wire_id(const jn_frame_t *f)
{
    return f->id | (((f->flags & JN_F_EXT) != 0u) ? JN_EFF_FLAG : 0u);
}

int jn_frame_encode(const jn_frame_t *f, uint8_t *buf, size_t cap)
{
    int fd = (f->flags & JN_F_FD) != 0u;
    size_t mtu = fd ? (size_t) JN_FD_MTU : (size_t) JN_CLASSIC_MTU;
    unsigned wire_flags = 0u;

    if (cap < mtu) {
        return JN_E_BUF;
    }
    if (fd) {
        if (!jn_len_allowed(f->len)) {
            return JN_E_LEN;
        }
    } else if (f->len > 8u) {
        return JN_E_LEN;
    }

    /* The two reserved bytes and the unused tail of the payload are zeroed
     * rather than left as they were. A frame that carries whatever was last in
     * this buffer is the kind of bug that only shows up on another machine. */
    memset(buf, 0, mtu);

    put_u32le(buf, jn_frame_wire_id(f));
    buf[4] = f->len;

    if (fd) {
        wire_flags = JN_FD_FDF;
        if ((f->flags & JN_F_BRS) != 0u) {
            wire_flags |= JN_FD_BRS;
        }
        if ((f->flags & JN_F_ESI) != 0u) {
            wire_flags |= JN_FD_ESI;
        }
    }
    buf[5] = (uint8_t) wire_flags;

    memcpy(buf + 8, f->data, f->len);
    return (int) mtu;
}

int jn_frame_decode(jn_frame_t *f, const uint8_t *buf, size_t len)
{
    uint32_t raw;
    unsigned wire_flags;
    unsigned flags = 0u;
    uint8_t payload_len;
    int fd;

    if (len == (size_t) JN_FD_MTU) {
        fd = 1;
    } else if (len == (size_t) JN_CLASSIC_MTU) {
        fd = 0;
    } else {
        return JN_E_LEN;
    }

    raw = get_u32le(buf);
    payload_len = buf[4];
    wire_flags = buf[5];

    if (fd) {
        if (!jn_len_allowed(payload_len)) {
            return JN_E_LEN;
        }
    } else if (payload_len > 8u) {
        return JN_E_LEN;
    }

    if ((raw & JN_EFF_FLAG) != 0u) {
        flags |= JN_F_EXT;
    }
    if (fd) {
        flags |= JN_F_FD;
        if ((wire_flags & JN_FD_BRS) != 0u) {
            flags |= JN_F_BRS;
        }
        if ((wire_flags & JN_FD_ESI) != 0u) {
            flags |= JN_F_ESI;
        }
    }

    memset(f, 0, sizeof *f);
    f->id = raw & (((flags & JN_F_EXT) != 0u) ? JN_EFF_MASK : JN_SFF_MASK);
    f->len = payload_len;
    f->flags = (uint8_t) flags;
    memcpy(f->data, buf + 8, payload_len);
    return JN_OK;
}

int jn_frame_format(const jn_frame_t *f, char *out, size_t cap)
{
    int fd = (f->flags & JN_F_FD) != 0u;
    int ext = (f->flags & JN_F_EXT) != 0u;
    size_t idw = ext ? 8u : 3u;
    size_t need = idw + (fd ? 3u : 1u) + (size_t) f->len * 2u + 1u;
    size_t n = 0;
    size_t i;
    int shift;

    if (cap < need) {
        return JN_E_BUF;
    }

    for (shift = (int) (idw - 1u) * 4; shift >= 0; shift -= 4) {
        out[n++] = HEX[(f->id >> shift) & 0xFu];
    }

    out[n++] = '#';
    if (fd) {
        unsigned wf = 0u;

        /* The flexible-data flag itself is not printed. It is already said by
         * the doubled separator, and the tools do not print it either. */
        if ((f->flags & JN_F_BRS) != 0u) {
            wf |= JN_FD_BRS;
        }
        if ((f->flags & JN_F_ESI) != 0u) {
            wf |= JN_FD_ESI;
        }
        out[n++] = '#';
        out[n++] = HEX[wf & 0xFu];
    }

    for (i = 0; i < (size_t) f->len; i++) {
        out[n++] = HEX[(f->data[i] >> 4) & 0xFu];
        out[n++] = HEX[f->data[i] & 0xFu];
    }
    out[n] = '\0';
    return (int) n;
}

int jn_frame_equal(const jn_frame_t *a, const jn_frame_t *b)
{
    if (a->id != b->id || a->len != b->len || a->flags != b->flags) {
        return 0;
    }
    return memcmp(a->data, b->data, a->len) == 0;
}
