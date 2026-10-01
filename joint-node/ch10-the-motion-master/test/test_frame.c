/* The frame layout, on any machine.
 *
 *     ./build/test_frame
 *
 * Nothing here opens a socket, so this runs wherever there is a compiler. What
 * it checks is the part that goes wrong silently: an identifier that overflowed
 * into the flag bits, a payload length that cannot exist, a flexible-data frame
 * decoded as a classic one because the socket option was never set, and a wire
 * buffer carrying bytes nobody meant to send.
 *
 * The last of those is the one C adds to the list. A buffer on the stack holds
 * whatever was there before, so a frame that writes only the bytes it knows
 * about will send the remains of the previous call, and only on some runs.
 */
#include "jn_frame.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define FAIL(...) do { \
        printf("  "); printf(__VA_ARGS__); printf("\n"); failures++; \
    } while (0)

static void check(int cond, const char *what)
{
    if (!cond) {
        FAIL("%s", what);
    }
}

static void test_sizes(void)
{
    uint8_t payload[24];
    uint8_t buf[JN_FD_MTU];
    jn_frame_t f;

    memset(payload, 0, sizeof payload);

    check(jn_frame_init(&f, 0x123, "\x01\x02", 2, 0) == JN_OK,
          "a two byte classic frame was refused");
    check(jn_frame_encode(&f, buf, sizeof buf) == (int) JN_CLASSIC_MTU,
          "a classic frame is not 16 bytes on the wire");

    check(jn_frame_init(&f, 0x123, payload, 24, JN_F_FD) == JN_OK,
          "a 24 byte flexible-data frame was refused");
    check(jn_frame_encode(&f, buf, sizeof buf) == (int) JN_FD_MTU,
          "a flexible-data frame is not 72 bytes on the wire");
}

static void test_round_trip(void)
{
    /* The fill byte differs per case so that a frame which came back as its
     * neighbour is caught rather than compared equal to itself. */
    static const struct {
        uint32_t id;
        size_t   len;
        uint8_t  flags;
        uint8_t  fill;
    } cases[] = {
        { 0x000,       0, 0,                                               0x00 },
        { 0x7FF,       8, 0,                                               0x10 },
        { 0x123,      24, JN_F_FD | JN_F_BRS,                              0x20 },
        { 0x1FFFFFFF, 64, JN_F_FD | JN_F_BRS | JN_F_ESI | JN_F_EXT,        0x30 },
        { 0x001,      12, JN_F_FD,                                         0xFF }
    };
    size_t c;

    for (c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        char printed[JN_FORMAT_MAX];
        char came_back[JN_FORMAT_MAX];
        uint8_t data[JN_MAX_DLEN];
        uint8_t buf[JN_FD_MTU];
        jn_frame_t sent;
        jn_frame_t got;
        size_t i;
        int n;
        int rc;

        for (i = 0; i < cases[c].len; i++) {
            data[i] = (uint8_t) (cases[c].fill + i);
        }

        if (jn_frame_init(&sent, cases[c].id, data, cases[c].len,
                          cases[c].flags) != JN_OK) {
            FAIL("case %u: a frame that can exist on a wire was refused",
                 (unsigned) c);
            continue;
        }
        n = jn_frame_encode(&sent, buf, sizeof buf);
        if (n < 0) {
            FAIL("case %u: encode refused a frame it had just accepted: %s",
                 (unsigned) c, jn_strerror(n));
            continue;
        }
        rc = jn_frame_decode(&got, buf, (size_t) n);
        if (rc != JN_OK) {
            FAIL("case %u: decode refused what encode wrote: %s",
                 (unsigned) c, jn_strerror(rc));
            continue;
        }

        if (!jn_frame_equal(&sent, &got)) {
            (void) jn_frame_format(&sent, printed, sizeof printed);
            (void) jn_frame_format(&got, came_back, sizeof came_back);
            FAIL("round trip changed %s into %s", printed, came_back);
        }
    }
}

static void test_identifier_flags(void)
{
    /* The extended bit lives above the identifier, so a standard frame and an
     * extended frame numbered alike are two different frames on the wire. */
    uint8_t buf[JN_FD_MTU];
    jn_frame_t std;
    jn_frame_t ext;
    jn_frame_t back;
    int n;

    check(jn_frame_init(&std, 0x123, NULL, 0, 0) == JN_OK,
          "an empty standard frame was refused");
    check(jn_frame_init(&ext, 0x123, NULL, 0, JN_F_EXT) == JN_OK,
          "an empty extended frame was refused");

    check(jn_frame_wire_id(&std) == 0x123u,
          "a standard identifier gained a flag bit");
    check(jn_frame_wire_id(&ext) == (0x123u | JN_EFF_FLAG),
          "an extended frame lost its flag bit");

    n = jn_frame_encode(&ext, buf, sizeof buf);
    check(n == (int) JN_CLASSIC_MTU,
          "an extended frame with no payload is not 16 bytes");
    check(jn_frame_decode(&back, buf, (size_t) JN_CLASSIC_MTU) == JN_OK,
          "an extended frame did not decode");
    check((back.flags & JN_F_EXT) != 0u,
          "the extended flag did not survive a round trip");
    check(back.id == 0x123u, "an extended identifier came back changed");

    (void) jn_frame_encode(&std, buf, sizeof buf);
    check(jn_frame_decode(&back, buf, (size_t) JN_CLASSIC_MTU) == JN_OK,
          "a standard frame did not decode");
    check((back.flags & JN_F_EXT) == 0u, "a standard frame came back extended");
}

static void test_refusals(void)
{
    uint8_t big[JN_MAX_DLEN];
    uint8_t neither[20];
    uint8_t small[JN_CLASSIC_MTU];
    jn_frame_t f;

    memset(big, 0, sizeof big);
    memset(neither, 0, sizeof neither);

    check(jn_frame_init(&f, 0x800, NULL, 0, 0) == JN_E_ID,
          "an identifier too wide for 11 bits was accepted");
    check(jn_frame_init(&f, 0x20000000, NULL, 0, JN_F_EXT) == JN_E_ID,
          "an identifier too wide for 29 bits was accepted");
    check(jn_frame_init(&f, 0x123, big, 9, 0) == JN_E_LEN,
          "a 9 byte classic frame was accepted");
    check(jn_frame_init(&f, 0x123, big, 9, JN_F_FD) == JN_E_LEN,
          "a 9 byte flexible-data frame was accepted");
    check(jn_frame_init(&f, 0x123, big, 63, JN_F_FD) == JN_E_LEN,
          "a 63 byte flexible-data frame was accepted");
    check(jn_frame_init(&f, 0x123, NULL, 0, JN_F_BRS) == JN_E_BRS,
          "a rate switch on a classic frame was accepted");
    check(jn_frame_init(&f, 0x123, NULL, 4, 0) == JN_E_ARG,
          "four bytes of payload were taken from a null pointer");
    check(jn_frame_decode(&f, neither, sizeof neither) == JN_E_LEN,
          "a 20 byte buffer was decoded as a frame");

    /* A caller who sized the buffer for a classic frame and then set the
     * flexible-data flag is a real mistake, and in C it is a stack overrun
     * rather than an exception, so the length is checked before the write. */
    check(jn_frame_init(&f, 0x123, big, 16, JN_F_FD) == JN_OK,
          "a 16 byte flexible-data frame was refused");
    check(jn_frame_encode(&f, small, sizeof small) == JN_E_BUF,
          "a 72 byte frame was written into a 16 byte buffer");

    /* Same again for the printed form: one byte short is still short. */
    {
        char tight[9];   /* "123#DEAD" needs nine bytes with its terminator */
        char tooshort[8];

        check(jn_frame_init(&f, 0x123, "\xde\xad", 2, 0) == JN_OK,
              "a two byte classic frame was refused");
        check(jn_frame_format(&f, tight, sizeof tight) == 8,
              "123#DEAD did not fit in nine bytes");
        check(jn_frame_format(&f, tooshort, sizeof tooshort) == JN_E_BUF,
              "123#DEAD was written into eight bytes");
    }
}

static void test_fd_flags_live_in_the_frame_not_the_identifier(void)
{
    uint8_t data[16];
    uint8_t buf[JN_FD_MTU];
    jn_frame_t f;
    jn_frame_t back;

    memset(data, 0, sizeof data);

    check(jn_frame_init(&f, 0x123, data, 16, JN_F_FD | JN_F_BRS) == JN_OK,
          "a 16 byte flexible-data frame with a rate switch was refused");
    check(jn_frame_encode(&f, buf, sizeof buf) == (int) JN_FD_MTU,
          "a 16 byte flexible-data frame is not 72 bytes");

    check((buf[5] & JN_FD_FDF) != 0u,
          "the flexible-data flag is not in the flags byte");
    check((buf[5] & JN_FD_BRS) != 0u,
          "the rate switch flag is not in the flags byte");
    check((jn_frame_wire_id(&f) & 0xE0000000u) == 0u,
          "a flexible-data flag leaked into the identifier word");

    check(jn_frame_decode(&back, buf, (size_t) JN_FD_MTU) == JN_OK,
          "a flexible-data frame did not decode");
    check((back.flags & JN_F_BRS) != 0u,
          "the rate switch did not survive a round trip");
}

static void test_printed_form(void)
{
    char out[JN_FORMAT_MAX];
    char want[JN_FORMAT_MAX];
    uint8_t twelve[12];
    jn_frame_t f;

    memset(twelve, 0, sizeof twelve);

    (void) jn_frame_init(&f, 0x123, "\xde\xad", 2, 0);
    (void) jn_frame_format(&f, out, sizeof out);
    if (strcmp(out, "123#DEAD") != 0) {
        FAIL("a classic frame printed as %s, not 123#DEAD", out);
    }

    /* Built rather than typed: twenty four zeros counted by eye is a test that
     * fails for the wrong reason. */
    memcpy(want, "123##1", 6);
    memset(want + 6, '0', 24);
    want[30] = '\0';

    (void) jn_frame_init(&f, 0x123, twelve, 12, JN_F_FD | JN_F_BRS);
    (void) jn_frame_format(&f, out, sizeof out);
    if (strcmp(out, want) != 0) {
        FAIL("a flexible-data frame printed as %s, not %s", out, want);
    }

    (void) jn_frame_init(&f, 0x456, "\x01", 1, JN_F_EXT);
    (void) jn_frame_format(&f, out, sizeof out);
    if (strcmp(out, "00000456#01") != 0) {
        FAIL("an extended frame printed as %s, not 00000456#01", out);
    }
}

static void test_the_whole_wire_is_written(void)
{
    uint8_t buf[JN_FD_MTU];
    uint8_t data[12];
    jn_frame_t f;
    size_t i;

    /* Dirtied first, on purpose. If the buffer started zeroed this test would
     * pass whether or not the code wrote those bytes. */
    memset(buf, 0xA5, sizeof buf);
    memset(data, 0x11, sizeof data);

    check(jn_frame_init(&f, 0x1A3, data, 12, JN_F_FD) == JN_OK,
          "a 12 byte flexible-data frame was refused");
    check(jn_frame_encode(&f, buf, sizeof buf) == (int) JN_FD_MTU,
          "a 12 byte flexible-data frame is not 72 bytes");

    check(buf[6] == 0u && buf[7] == 0u,
          "the two reserved bytes were left as they were found");
    for (i = 12u; i < JN_MAX_DLEN; i++) {
        if (buf[8u + i] != 0u) {
            FAIL("payload byte %u past the length is 0x%02X, not zero",
                 (unsigned) i, (unsigned) buf[8u + i]);
            break;
        }
    }
}

static void test_the_length_table(void)
{
    static const size_t refused[] = { 9, 10, 11, 13, 17, 25, 33, 49, 63, 65 };
    size_t i;

    for (i = 0; i < sizeof jn_fd_lengths; i++) {
        if (!jn_len_allowed((size_t) jn_fd_lengths[i])) {
            FAIL("%u is in the table and was refused",
                 (unsigned) jn_fd_lengths[i]);
        }
    }
    for (i = 0; i < sizeof refused / sizeof refused[0]; i++) {
        if (jn_len_allowed(refused[i])) {
            FAIL("%u bytes was allowed, and no frame can carry it",
                 (unsigned) refused[i]);
        }
    }
}

int main(void)
{
    test_sizes();
    test_round_trip();
    test_identifier_flags();
    test_refusals();
    test_fd_flags_live_in_the_frame_not_the_identifier();
    test_printed_form();
    test_the_whole_wire_is_written();
    test_the_length_table();

    if (failures != 0) {
        printf("FAIL  %d problems above\n", failures);
        return 1;
    }
    printf("ok  frame layout, identifier flags, length rules, printed form,"
           " and a fully written wire\n");
    return 0;
}
