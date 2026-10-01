/* Watch a bus, and optionally write what it saw to a file.
 *
 *     jn-listen vcan0
 *     jn-listen can0 --out run.log --seconds 10
 *     jn-listen can0 --classic
 *
 * Deliberately dumb about meaning. It prints identifiers and bytes, not named
 * signals, because the names belong to the protocol description in chapter 11
 * and this chapter is the socket layer underneath it. Keeping the two apart is
 * what lets this tool watch traffic from a node whose protocol it has never
 * been told.
 *
 * Lines are written to the file as they arrive rather than collected and
 * written at the end. A long recording interrupted at the wall socket then
 * still has everything up to the moment it stopped, which is usually the part
 * somebody wanted.
 */
#include "jn_bus.h"
#include "jn_log.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Set from a signal handler, so it is read once per loop and nothing else. The
 * two qualifiers are both needed: volatile because the loop must reread it, and
 * sig_atomic_t because a wider type can be written halfway. */
static volatile sig_atomic_t interrupted;

static void on_interrupt(int sig)
{
    (void) sig;
    interrupted = 1;
}

static void usage(void)
{
    fputs("usage: jn-listen [interface] [--out FILE] [--seconds N] [--classic]\n"
          "\n"
          "  interface   default vcan0\n"
          "  --out       also write a log file the standard tools can read\n"
          "  --seconds   stop after this long; without it, run until stopped\n"
          "  --classic   classic frames only, which truncates anything longer\n",
          stderr);
}

int main(int argc, char **argv)
{
    const char *iface = "vcan0";
    const char *outpath = NULL;
    char line[JN_LOG_MAX];
    double seconds = 0.0;
    uint64_t deadline = 0;
    unsigned long seen = 0;
    FILE *out = NULL;
    jn_frame_t f;
    int fd_mode = 1;
    int sock;
    int rc;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--classic") == 0) {
            fd_mode = 0;
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outpath = argv[++i];
        } else if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "-h") == 0
                   || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "jn-listen: %s is not an option I know\n", argv[i]);
            usage();
            return 2;
        } else {
            iface = argv[i];
        }
    }

    if (!jn_bus_available()) {
        fprintf(stderr, "jn-listen: %s\n", jn_bus_why_not());
        return 2;
    }

    sock = jn_bus_open(iface, fd_mode);
    if (sock < 0) {
        fprintf(stderr, "jn-listen: %s could not be opened: %s\n",
                iface, strerror(errno));
        return 1;
    }

    if (outpath != NULL) {
        out = fopen(outpath, "w");
        if (out == NULL) {
            fprintf(stderr, "jn-listen: %s could not be written: %s\n",
                    outpath, strerror(errno));
            jn_bus_close(sock);
            return 1;
        }
    }

    signal(SIGINT, on_interrupt);
    signal(SIGTERM, on_interrupt);

    if (seconds > 0.0) {
        deadline = jn_mono_ns() + (uint64_t) (seconds * 1e9);
    }

    /* A short poll rather than a blocking read, so an interrupt and a deadline
     * are both noticed within a fifth of a second of happening. */
    while (interrupted == 0) {
        if (deadline != 0 && jn_mono_ns() >= deadline) {
            break;
        }
        rc = jn_bus_recv(sock, &f, 200);
        if (rc == JN_E_TIMEOUT) {
            continue;
        }
        if (rc != JN_OK) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "jn-listen: %s: %s\n", jn_strerror(rc),
                    strerror(errno));
            break;
        }

        /* The arrival is stamped here rather than in the socket layer. The
         * kernel can hand over its own timestamp, and when that is wired up
         * this is the one line that changes. */
        f.stamp_ns = jn_now_ns();
        seen++;

        if (jn_log_line(&f, iface, line, sizeof line) < 0) {
            fprintf(stderr, "jn-listen: a frame would not fit a line\n");
            continue;
        }
        printf("%s\n", line);
        if (out != NULL) {
            fprintf(out, "%s\n", line);
        }
    }

    if (out != NULL) {
        if (fclose(out) != 0) {
            fprintf(stderr, "jn-listen: %s was not written cleanly: %s\n",
                    outpath, strerror(errno));
            jn_bus_close(sock);
            return 1;
        }
        fprintf(stderr, "jn-listen: %lu frames written to %s\n", seen, outpath);
    }
    jn_bus_close(sock);

    fprintf(stderr, "jn-listen: %lu frames seen on %s\n", seen, iface);
    return 0;
}
