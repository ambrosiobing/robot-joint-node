/* The log format, and the deliberate gap.
 *
 *     ./build/test_log [scratch-file]
 *
 * A recording is only useful if the file is the one the standard tools write,
 * and if reading it twice gives the same frames twice. The second property is
 * what makes a dropped frame reproducible, and a dropped frame is how chapter
 * 11's gap detection is demonstrated.
 *
 * The known lines below were written against the output of the standard logger
 * rather than against this code's own output. A test that compares a program
 * with itself only proves it is consistent.
 */
#include "jn_log.h"

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

/* 1696118400.123456 seconds, as whole nanoseconds. Written as the arithmetic
 * rather than as the digits, so the constant can be checked by reading it. */
#define STAMP  (1696118400ull * JN_NS_PER_SEC + 123456000ull)

static void test_line_shapes(void)
{
    static const uint8_t d_dead[2] = { 0xDE, 0xAD };
    static const uint8_t d_one[1]  = { 0x01 };
    static const uint8_t d_11[12]  = {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
    };
    static const struct {
        const char    *line;
        uint32_t       id;
        const uint8_t *want;
        size_t         len;
        int            fd;
        int            brs;
        const char    *iface;
    } known[] = {
        { "(1696118400.123456) vcan0 123#DEAD",   0x123, d_dead, 2, 0, 0, "vcan0" },
        { "(1696118400.123456) vcan0 7FF#",       0x7FF, NULL,   0, 0, 0, "vcan0" },
        { "(1696118400.123456) can0 00000456#01", 0x456, d_one,  1, 0, 0, "can0"  }
    };
    char fdline[JN_LOG_MAX];
    char iface[16];
    jn_frame_t f;
    size_t k;

    for (k = 0; k < sizeof known / sizeof known[0]; k++) {
        int rc = jn_log_parse(known[k].line, &f, iface, sizeof iface);

        if (rc != JN_OK) {
            FAIL("%s: refused (%s)", known[k].line, jn_strerror(rc));
            continue;
        }
        if (f.id != known[k].id) {
            FAIL("%s: identifier came back 0x%X", known[k].line,
                 (unsigned) f.id);
        }
        if ((size_t) f.len != known[k].len) {
            FAIL("%s: payload came back %u bytes, expected %u",
                 known[k].line, (unsigned) f.len, (unsigned) known[k].len);
        } else if (known[k].len > 0u
                   && memcmp(f.data, known[k].want, known[k].len) != 0) {
            FAIL("%s: payload bytes came back different", known[k].line);
        }
        if (((f.flags & JN_F_FD) != 0u) != known[k].fd) {
            FAIL("%s: the flexible-data flag came back wrong", known[k].line);
        }
        if (((f.flags & JN_F_BRS) != 0u) != known[k].brs) {
            FAIL("%s: the rate switch came back wrong", known[k].line);
        }
        if (strcmp(iface, known[k].iface) != 0) {
            FAIL("%s: the interface came back %s", known[k].line, iface);
        }
        if (f.stamp_ns != STAMP) {
            FAIL("%s: the stamp came back %llu, expected %llu",
                 known[k].line, (unsigned long long) f.stamp_ns,
                 (unsigned long long) STAMP);
        }
    }

    /* The flexible-data line is assembled rather than typed. Twelve repeated
     * bytes is twenty four hex digits plus one flags digit, and miscounting
     * them by eye gives a test that fails for a reason of its own. */
    memcpy(fdline, "(1696118400.123456) vcan0 1A3##1", 32);
    memset(fdline + 32, '1', 24);
    fdline[56] = '\0';

    if (jn_log_parse(fdline, &f, iface, sizeof iface) != JN_OK) {
        FAIL("the flexible-data line was refused: %s", fdline);
    } else {
        check(f.id == 0x1A3u, "the flexible-data identifier came back wrong");
        check((f.flags & JN_F_FD) != 0u,
              "a doubled separator did not make a flexible-data frame");
        check((f.flags & JN_F_BRS) != 0u,
              "the flags digit 1 did not set the rate switch");
        check(f.len == 12u, "twenty four hex digits did not make twelve bytes");
        check(memcmp(f.data, d_11, 12) == 0,
              "the flexible-data payload came back different");
    }

    /* An identifier printed in more than three digits is an extended one. */
    if (jn_log_parse("(1.0) can0 00000456#01", &f, NULL, 0) != JN_OK) {
        FAIL("an eight digit identifier was refused");
    } else {
        check((f.flags & JN_F_EXT) != 0u,
              "an eight digit identifier did not come back extended");
    }
    if (jn_log_parse("(1.0) can0 456#01", &f, NULL, 0) != JN_OK) {
        FAIL("a three digit identifier was refused");
    } else {
        check((f.flags & JN_F_EXT) == 0u,
              "a three digit identifier came back extended");
    }
}

static void test_the_printed_line(void)
{
    char line[JN_LOG_MAX];
    jn_frame_t f;
    int n;

    (void) jn_frame_init(&f, 0x123, "\xde\xad", 2, 0);
    f.stamp_ns = STAMP;

    n = jn_log_line(&f, "vcan0", line, sizeof line);
    if (n < 0) {
        FAIL("writing a line failed: %s", jn_strerror(n));
    } else if (strcmp(line, "(1696118400.123456) vcan0 123#DEAD") != 0) {
        FAIL("printed %s", line);
    }

    /* A short buffer is reported rather than quietly truncated, because a
     * truncated line is a line another tool will refuse. */
    {
        char tiny[10];
        check(jn_log_line(&f, "vcan0", tiny, sizeof tiny) == JN_E_BUF,
              "a 34 character line was written into ten bytes");
    }
}

static void test_round_trip_through_a_file(const char *path)
{
    jn_frame_t frames[11];
    jn_frame_t back[11];
    uint8_t twentyfour[24];
    size_t i;
    int n;

    for (i = 0; i < 10u; i++) {
        uint8_t data[8];
        size_t len = i % 8u;
        size_t k;

        for (k = 0; k < len; k++) {
            data[k] = (uint8_t) i;
        }
        if (jn_frame_init(&frames[i], (uint32_t) (0x200u + i),
                          data, len, 0) != JN_OK) {
            FAIL("frame %u, which the test built, was refused", (unsigned) i);
            return;
        }
        frames[i].stamp_ns = 1000ull * JN_NS_PER_SEC + (uint64_t) i * 1000000ull;
    }

    memset(twentyfour, 0, sizeof twentyfour);
    if (jn_frame_init(&frames[10], 0x300, twentyfour, 24,
                      JN_F_FD | JN_F_BRS) != JN_OK) {
        FAIL("the flexible-data frame the test built was refused");
        return;
    }
    frames[10].stamp_ns = 1001ull * JN_NS_PER_SEC;

    n = jn_log_write(path, frames, 11u, "vcan0");
    if (n != 11) {
        FAIL("writing eleven frames to %s returned %d (%s)",
             path, n, jn_strerror(n));
        return;
    }
    n = jn_log_read(path, back, sizeof back / sizeof back[0]);
    if (n != 11) {
        FAIL("reading %s back returned %d (%s)", path, n, jn_strerror(n));
        return;
    }

    for (i = 0; i < 11u; i++) {
        if (!jn_frame_equal(&frames[i], &back[i])) {
            FAIL("frame %u came back different", (unsigned) i);
        }
        /* The line carries microseconds, so the comparison is made at the
         * resolution the format actually has. Comparing nanoseconds here would
         * be testing the format for something it never claimed. */
        if (frames[i].stamp_ns / 1000ull != back[i].stamp_ns / 1000ull) {
            FAIL("the stamp on frame %u came back %llu, not %llu",
                 (unsigned) i,
                 (unsigned long long) (back[i].stamp_ns / 1000ull),
                 (unsigned long long) (frames[i].stamp_ns / 1000ull));
        }
    }
}

static void test_reading_twice_gives_the_same_frames(const char *path)
{
    jn_frame_t frames[5];
    jn_frame_t first[5];
    jn_frame_t second[5];
    size_t i;
    int n;

    for (i = 0; i < 5u; i++) {
        uint8_t b = (uint8_t) i;

        (void) jn_frame_init(&frames[i], 0x100, &b, 1, 0);
        frames[i].stamp_ns = ((uint64_t) i + 1ull) * JN_NS_PER_SEC;
    }
    if (jn_log_write(path, frames, 5u, "vcan0") != 5) {
        FAIL("could not write the five frame file");
        return;
    }

    n = jn_log_read(path, first, 5u);
    if (n != 5) {
        FAIL("the first read returned %d", n);
        return;
    }
    n = jn_log_read(path, second, 5u);
    if (n != 5) {
        FAIL("the second read returned %d", n);
        return;
    }
    for (i = 0; i < 5u; i++) {
        if (!jn_frame_equal(&first[i], &second[i])
                || first[i].stamp_ns != second[i].stamp_ns) {
            FAIL("two reads of one file differ at frame %u", (unsigned) i);
        }
    }
}

static void test_comments_and_blank_lines(const char *path)
{
    jn_frame_t back[4];
    FILE *fh;
    int n;

    fh = fopen(path, "w");
    if (fh == NULL) {
        FAIL("could not open %s", path);
        return;
    }
    fprintf(fh, "# a recording can be annotated\n");
    fprintf(fh, "\n");
    fprintf(fh, "(1.000000) vcan0 123#DEAD\n");
    fprintf(fh, "   \n");
    fprintf(fh, "(2.000000) vcan0 124#BEEF\n");
    fclose(fh);

    n = jn_log_read(path, back, sizeof back / sizeof back[0]);
    if (n != 2) {
        FAIL("a file with a comment and two blank lines read %d frames, not 2",
             n);
        return;
    }
    check(back[0].id == 0x123u && back[1].id == 0x124u,
          "the two frames either side of the blank lines came back wrong");
}

static void test_deliberate_gap(void)
{
    static const uint8_t expect[7] = { 0, 1, 3, 4, 6, 7, 9 };
    jn_frame_t frames[10];
    size_t dropped[4];
    size_t i;
    int kept;

    for (i = 0; i < 10u; i++) {
        uint8_t b = (uint8_t) i;

        /* The payload is the original position, which is what makes the kept
         * frames identifiable after the array has been compacted. */
        (void) jn_frame_init(&frames[i], 0x100, &b, 1, 0);
        frames[i].stamp_ns = (uint64_t) i * JN_NS_PER_SEC;
    }

    kept = jn_log_drop(frames, 10u, 3u, dropped,
                       sizeof dropped / sizeof dropped[0]);
    if (kept != 7) {
        FAIL("dropping every third of ten left %d, expected 7", kept);
        return;
    }
    check(dropped[0] == 2u && dropped[1] == 5u && dropped[2] == 8u,
          "the dropped positions were not 2, 5 and 8");

    for (i = 0; i < 7u; i++) {
        if (frames[i].data[0] != expect[i]) {
            FAIL("kept frame %u carries %u, expected %u", (unsigned) i,
                 (unsigned) frames[i].data[0], (unsigned) expect[i]);
        }
    }

    check(jn_log_drop(frames, 7u, 1u, NULL, 0) == JN_E_ARG,
          "dropping every frame was accepted as a gap");
    check(jn_log_drop(frames, 7u, 0u, NULL, 0) == JN_E_ARG,
          "dropping every nothingth frame was accepted as a gap");
}

static void test_refusals(void)
{
    static const char *bad[] = {
        "not a log line at all",
        "(1696118400.123456) vcan0 123#DEA",          /* half a byte */
        "(1696118400.123456) vcan0 1A3##",            /* no flags digit */
        "1696118400.123456 vcan0 123#DEAD",           /* no brackets */
        "(1696118400) vcan0 123#DEAD",                /* no fraction */
        "(1696118400.123456) vcan0 #DEAD",            /* no identifier */
        "(1696118400.123456) vcan0 123DEAD",          /* no separator */
        "(1696118400.123456) vcan0 123#DEAD extra",   /* something after it */
        ""
    };
    jn_frame_t f;
    size_t i;

    for (i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        if (jn_log_parse(bad[i], &f, NULL, 0) == JN_OK) {
            FAIL("accepted \"%s\", and it should not have", bad[i]);
        }
    }

    /* An interface name that will not fit is reported rather than cut short.
     * A name cut short names a different interface. */
    {
        char tiny[3];

        check(jn_log_parse("(1.0) vcan0 123#DEAD", &f, tiny, sizeof tiny)
                  == JN_E_BUF,
              "a five character interface name fitted into three bytes");
    }
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "jn_test.log";

    test_line_shapes();
    test_the_printed_line();
    test_round_trip_through_a_file(path);
    test_reading_twice_gives_the_same_frames(path);
    test_comments_and_blank_lines(path);
    test_deliberate_gap();
    test_refusals();

    remove(path);

    if (failures != 0) {
        printf("FAIL  %d problems above\n", failures);
        return 1;
    }
    printf("ok  log format, file round trip, repeatable reads,"
           " and a deliberate gap\n");
    return 0;
}
