/* A setpoint source: something for the node to listen to.
 *
 *     jn-setpoint vcan0 --rate 1000 --seconds 5
 *     jn-setpoint vcan0 --replay run.log
 *     jn-setpoint vcan0 --replay run.log --drop-every 3
 *
 * The payload here is opaque bytes on purpose. What a command frame means is
 * chapter 11's business; what this chapter owes is a source that emits at a
 * chosen rate, keeps its own schedule honestly, and can replay a file so that a
 * fault is reproducible.
 *
 * The rate it reports is the rate it achieved, not the rate it was asked for. A
 * generator that claims 1000 Hz while delivering 780 is the kind of instrument
 * that makes everything measured against it wrong, and it is the quiet sort of
 * wrong, because the number printed is the one that was typed in.
 */
#include "jn_bus.h"
#include "jn_log.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* About 18 MB of frames. Large enough for a minute at a thousand frames a
 * second, and refused rather than silently halved if a file is longer. */
#define REPLAY_MAX  200000u

static void usage(void)
{
    fputs("usage: jn-setpoint [interface] [--id N] [--rate HZ] [--seconds S]\n"
          "                   [--replay FILE] [--drop-every N]\n"
          "\n"
          "  interface      default vcan0\n"
          "  --id           identifier to send on, default 0x101\n"
          "  --rate         frames per second, default 100\n"
          "  --seconds      how long to send for, default 1\n"
          "  --replay       send the frames in this log file instead\n"
          "  --drop-every   with --replay, drop every nth frame on purpose\n",
          stderr);
}

/* Paced against a deadline rather than by sleeping the interval. Sleeping the
 * interval accumulates every scheduling delay; sleeping until the next deadline
 * does not, which is the difference between drifting a second a minute and not
 * drifting at all. */
static int generate(int sock, uint32_t id, double rate, double secs)
{
    uint64_t interval_ns;
    uint64_t begin;
    uint64_t elapsed;
    unsigned long count;
    unsigned long i;
    double achieved;
    int rc;

    if (rate <= 0.0 || secs <= 0.0) {
        fprintf(stderr, "jn-setpoint: a rate and a duration must both be"
                        " above zero\n");
        return 2;
    }

    count = (unsigned long) (rate * secs);
    if (count < 1ul) {
        count = 1ul;
    }
    interval_ns = (uint64_t) (1e9 / rate);

    begin = jn_mono_ns();
    for (i = 0; i < count; i++) {
        uint8_t payload[8];
        uint64_t target = begin + (uint64_t) i * interval_ns;
        uint64_t now = jn_mono_ns();
        jn_frame_t f;

        if (target > now) {
            jn_sleep_ns(target - now);
        }

        /* The counter, little endian, then four bytes left at zero. Opaque on
         * purpose: a receiver that reads meaning into this is reading meaning
         * this chapter never put there. */
        payload[0] = (uint8_t) (i & 0xFFul);
        payload[1] = (uint8_t) ((i >> 8) & 0xFFul);
        payload[2] = (uint8_t) ((i >> 16) & 0xFFul);
        payload[3] = (uint8_t) ((i >> 24) & 0xFFul);
        payload[4] = 0u;
        payload[5] = 0u;
        payload[6] = 0u;
        payload[7] = 0u;

        rc = jn_frame_init(&f, id, payload, sizeof payload, 0);
        if (rc != JN_OK) {
            fprintf(stderr, "jn-setpoint: %s\n", jn_strerror(rc));
            return 1;
        }
        rc = jn_bus_send(sock, &f);
        if (rc != JN_OK) {
            fprintf(stderr, "jn-setpoint: sending stopped after %lu frames:"
                            " %s (%s)\n", i, jn_strerror(rc), strerror(errno));
            return 1;
        }
    }
    elapsed = jn_mono_ns() - begin;

    if (elapsed == 0u) {
        fprintf(stderr, "jn-setpoint: %lu frames in no measurable time, so no"
                        " rate is being claimed\n", count);
        return 0;
    }

    achieved = (double) count * 1e9 / (double) elapsed;
    fprintf(stderr, "jn-setpoint: %lu frames in %.3f s on this interface:"
                    " %.1f Hz achieved against %.1f Hz asked for\n",
            count, (double) elapsed / 1e9, achieved, rate);
    if (achieved < rate * 0.95 || achieved > rate * 1.05) {
        fprintf(stderr, "jn-setpoint: that is more than five per cent off, so"
                        " do not quote the asked-for rate anywhere\n");
    }
    return 0;
}

static int replay(int sock, const char *path, unsigned long drop_every)
{
    jn_frame_t *frames;
    uint64_t base;
    uint64_t begin;
    size_t n;
    size_t i;
    int count;
    int rc = 0;

    frames = malloc((size_t) REPLAY_MAX * sizeof *frames);
    if (frames == NULL) {
        fprintf(stderr, "jn-setpoint: not enough memory for a replay buffer\n");
        return 1;
    }

    count = jn_log_read(path, frames, (size_t) REPLAY_MAX);
    if (count < 0) {
        fprintf(stderr, "jn-setpoint: %s could not be read: %s", path,
                jn_strerror(count));
        if (count == JN_E_BUF) {
            fprintf(stderr, " (more than %u frames)", (unsigned) REPLAY_MAX);
        } else if (count == JN_E_SYS) {
            fprintf(stderr, " (%s)", strerror(errno));
        }
        fputc('\n', stderr);
        free(frames);
        return 1;
    }
    n = (size_t) count;

    if (drop_every >= 2ul) {
        /* The positions are not collected. They are every nth by construction,
         * so a list of them would say nothing the count does not, and sizing an
         * array for the worst case would be the only hard part of asking. */
        int kept = jn_log_drop(frames, n, (size_t) drop_every, NULL, 0);

        if (kept < 0) {
            fprintf(stderr, "jn-setpoint: the drop was refused: %s\n",
                    jn_strerror(kept));
            free(frames);
            return 1;
        }
        fprintf(stderr, "jn-setpoint: dropping every %lu, which is %u of %u"
                        " frames, so the gap falls in the same place twice\n",
                drop_every, (unsigned) (n - (size_t) kept), (unsigned) n);
        n = (size_t) kept;
    } else if (drop_every == 1ul) {
        fprintf(stderr, "jn-setpoint: dropping every frame is not a gap;"
                        " --drop-every needs 2 or more\n");
        free(frames);
        return 2;
    }

    if (n == 0u) {
        fprintf(stderr, "jn-setpoint: %s holds no frames\n", path);
        free(frames);
        return 1;
    }

    /* The recorded gaps are reproduced, not the recorded absolute times. The
     * recording was made at some moment in the past and that moment is not
     * coming back; the spacing is the part worth repeating. */
    base = frames[0].stamp_ns;
    begin = jn_mono_ns();
    for (i = 0; i < n; i++) {
        if (frames[i].stamp_ns > base) {
            uint64_t target = begin + (frames[i].stamp_ns - base);
            uint64_t now = jn_mono_ns();

            if (target > now) {
                jn_sleep_ns(target - now);
            }
        }
        rc = jn_bus_send(sock, &frames[i]);
        if (rc != JN_OK) {
            fprintf(stderr, "jn-setpoint: sending stopped after %u frames:"
                            " %s (%s)\n", (unsigned) i, jn_strerror(rc),
                    strerror(errno));
            free(frames);
            return 1;
        }
    }

    fprintf(stderr, "jn-setpoint: replayed %u frames from %s\n",
            (unsigned) n, path);
    free(frames);
    return 0;
}

int main(int argc, char **argv)
{
    const char *iface = "vcan0";
    const char *replay_path = NULL;
    unsigned long drop_every = 0ul;
    uint32_t id = 0x101u;
    double rate = 100.0;
    double secs = 1.0;
    int sock;
    int status;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) {
            /* Base zero, so 0x101 and 257 both work and mean the same thing. */
            id = (uint32_t) strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--rate") == 0 && i + 1 < argc) {
            rate = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            secs = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            replay_path = argv[++i];
        } else if (strcmp(argv[i], "--drop-every") == 0 && i + 1 < argc) {
            drop_every = strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-h") == 0
                   || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "jn-setpoint: %s is not an option I know\n",
                    argv[i]);
            usage();
            return 2;
        } else {
            iface = argv[i];
        }
    }

    if (!jn_bus_available()) {
        fprintf(stderr, "jn-setpoint: %s\n", jn_bus_why_not());
        return 2;
    }

    sock = jn_bus_open(iface, 1);
    if (sock < 0) {
        fprintf(stderr, "jn-setpoint: %s could not be opened: %s\n",
                iface, strerror(errno));
        return 1;
    }

    if (replay_path != NULL) {
        status = replay(sock, replay_path, drop_every);
    } else {
        status = generate(sock, id, rate, secs);
    }

    jn_bus_close(sock);
    return status;
}
