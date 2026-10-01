#include "jn_log.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* ctype takes an int that must be a value an unsigned char can hold, or EOF.
 * Handing it a plain char is undefined for any byte above 127, which an
 * interface name will never contain and a corrupt file very well might. */
static int is_space(char c)
{
    return isspace((unsigned char) c) != 0;
}

static int is_digit(char c)
{
    return isdigit((unsigned char) c) != 0;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

int jn_log_line(const jn_frame_t *f, const char *iface, char *out, size_t cap)
{
    char body[JN_FORMAT_MAX];
    unsigned long long sec;
    unsigned long long usec;
    int n;

    n = jn_frame_format(f, body, sizeof body);
    if (n < 0) {
        return n;
    }

    sec = (unsigned long long) (f->stamp_ns / JN_NS_PER_SEC);
    usec = (unsigned long long) ((f->stamp_ns % JN_NS_PER_SEC) / 1000ull);

    n = snprintf(out, cap, "(%llu.%06llu) %s %s", sec, usec, iface, body);
    if (n < 0 || (size_t) n >= cap) {
        return JN_E_BUF;
    }
    return n;
}

int jn_log_parse(const char *line, jn_frame_t *f, char *iface, size_t ifcap)
{
    const char *p = line;
    const char *tok;
    uint64_t sec = 0;
    uint64_t frac = 0;
    uint64_t scale = JN_NS_PER_SEC;
    uint32_t id = 0;
    unsigned nibble = 0u;
    unsigned flags = 0u;
    uint8_t data[JN_MAX_DLEN];
    size_t toklen;
    size_t idlen = 0;
    size_t bodylen = 0;
    int fdframe = 0;
    int half = 0;
    int v;
    int rc;

    while (is_space(*p)) {
        p++;
    }
    if (*p != '(') {
        return JN_E_PARSE;
    }
    p++;

    if (!is_digit(*p)) {
        return JN_E_PARSE;
    }
    while (is_digit(*p)) {
        sec = sec * 10u + (uint64_t) (*p - '0');
        p++;
    }
    if (*p != '.') {
        return JN_E_PARSE;
    }
    p++;
    if (!is_digit(*p)) {
        return JN_E_PARSE;
    }
    while (is_digit(*p)) {
        /* Past the ninth digit there is nothing left to place, so the rest is
         * dropped. A logger that prints picoseconds is not lying, but this
         * structure cannot hold them. */
        if (scale > 1u) {
            scale /= 10u;
            frac += (uint64_t) (*p - '0') * scale;
        }
        p++;
    }
    if (*p != ')') {
        return JN_E_PARSE;
    }
    p++;

    if (!is_space(*p)) {
        return JN_E_PARSE;
    }
    while (is_space(*p)) {
        p++;
    }

    tok = p;
    while (*p != '\0' && !is_space(*p)) {
        p++;
    }
    toklen = (size_t) (p - tok);
    if (toklen == 0u) {
        return JN_E_PARSE;
    }
    if (iface != NULL) {
        if (toklen + 1u > ifcap) {
            return JN_E_BUF;
        }
        memcpy(iface, tok, toklen);
        iface[toklen] = '\0';
    }

    if (!is_space(*p)) {
        return JN_E_PARSE;
    }
    while (is_space(*p)) {
        p++;
    }

    while ((v = hexval(*p)) >= 0) {
        if (idlen >= 8u) {
            return JN_E_PARSE;
        }
        id = (id << 4) | (uint32_t) v;
        idlen++;
        p++;
    }
    if (idlen == 0u) {
        return JN_E_PARSE;
    }

    if (*p != '#') {
        return JN_E_PARSE;
    }
    p++;
    if (*p == '#') {
        fdframe = 1;
        p++;
    }

    if (fdframe) {
        v = hexval(*p);
        if (v < 0) {
            return JN_E_PARSE;   /* a doubled separator with no flags digit */
        }
        flags = (unsigned) v;
        p++;
    }

    while ((v = hexval(*p)) >= 0) {
        if (half == 0) {
            nibble = (unsigned) v;
            half = 1;
        } else {
            if (bodylen >= JN_MAX_DLEN) {
                return JN_E_LEN;
            }
            data[bodylen++] = (uint8_t) ((nibble << 4) | (unsigned) v);
            half = 0;
        }
        p++;
    }
    if (half != 0) {
        return JN_E_PARSE;       /* an odd number of hex digits is half a byte */
    }

    while (is_space(*p)) {
        p++;
    }
    if (*p != '\0') {
        return JN_E_PARSE;
    }

    /* An identifier printed in more than three digits is an extended one. That
     * is a convention of the printed form rather than a fact about the frame,
     * and it is the convention the standard tools use: they pad an extended
     * identifier to eight digits and a standard one to three. */
    {
        unsigned ff = 0u;

        if (idlen > 3u) {
            ff |= JN_F_EXT;
        }
        if (fdframe) {
            ff |= JN_F_FD;
            if ((flags & JN_FD_BRS) != 0u) {
                ff |= JN_F_BRS;
            }
            if ((flags & JN_FD_ESI) != 0u) {
                ff |= JN_F_ESI;
            }
        }
        rc = jn_frame_init(f, id, data, bodylen, (uint8_t) ff);
        if (rc != JN_OK) {
            return rc;
        }
    }

    f->stamp_ns = sec * JN_NS_PER_SEC + frac;
    return JN_OK;
}

int jn_log_write(const char *path, const jn_frame_t *frames, size_t n,
                 const char *iface)
{
    char line[JN_LOG_MAX];
    FILE *fh;
    size_t i;
    int rc;

    fh = fopen(path, "w");
    if (fh == NULL) {
        return JN_E_SYS;
    }
    for (i = 0; i < n; i++) {
        rc = jn_log_line(&frames[i], iface, line, sizeof line);
        if (rc < 0) {
            fclose(fh);
            return rc;
        }
        if (fprintf(fh, "%s\n", line) < 0) {
            fclose(fh);
            return JN_E_SYS;
        }
    }
    /* The close is checked because a buffered write that failed has not failed
     * yet anywhere else. A recording that is silently short is worse than none. */
    if (fclose(fh) != 0) {
        return JN_E_SYS;
    }
    return (int) n;
}

int jn_log_read(const char *path, jn_frame_t *out, size_t cap)
{
    char line[JN_LOG_MAX];
    FILE *fh;
    size_t n = 0;
    int rc;

    fh = fopen(path, "r");
    if (fh == NULL) {
        return JN_E_SYS;
    }
    while (fgets(line, (int) sizeof line, fh) != NULL) {
        const char *p = line;

        while (is_space(*p)) {
            p++;
        }
        if (*p == '\0' || *p == '#') {
            continue;
        }
        if (n >= cap) {
            fclose(fh);
            return JN_E_BUF;
        }
        rc = jn_log_parse(line, &out[n], NULL, 0);
        if (rc != JN_OK) {
            fclose(fh);
            return rc;
        }
        n++;
    }
    fclose(fh);
    return (int) n;
}

int jn_log_drop(jn_frame_t *frames, size_t n, size_t every,
                size_t *dropped, size_t dcap)
{
    size_t i;
    size_t kept = 0;
    size_t ndrop = 0;

    if (every < 2u) {
        return JN_E_ARG;
    }
    for (i = 0; i < n; i++) {
        if ((i + 1u) % every == 0u) {
            if (dropped != NULL) {
                if (ndrop >= dcap) {
                    return JN_E_BUF;
                }
                dropped[ndrop] = i;
            }
            ndrop++;
        } else {
            if (kept != i) {
                frames[kept] = frames[i];
            }
            kept++;
        }
    }
    return (int) kept;
}
