/* The socket, where there is one.
 *
 *     ./build/test_bus
 *
 * On a machine with no SocketCAN, or with the interface down, this reports that
 * it was skipped and why, and exits zero. A skipped test that exits one trains
 * people to ignore a red build; a skipped test that prints "ok" is worse. It
 * says which of the two happened.
 *
 * With the virtual interface up it is a real test: two sockets on one
 * interface, a frame sent on one and received on the other, which is the whole
 * first half of the chapter working with no hardware anywhere.
 *
 *     sudo modprobe vcan
 *     sudo ip link add dev vcan0 type vcan
 *     sudo ip link set up vcan0
 */
#include "jn_bus.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define IFACE "vcan0"

static int failures;

#define FAIL(...) do { \
        printf("  "); printf(__VA_ARGS__); printf("\n"); failures++; \
    } while (0)

static int skipped(const char *reason)
{
    printf("skip  %s\n", reason);
    return 0;
}

/* One frame out and the same frame back, which is the only claim this file is
 * really making. The timeout is generous: a virtual interface delivers in
 * microseconds, and anything near a second means something else is wrong. */
static void exchange(int tx, int rx, const jn_frame_t *sent, const char *what)
{
    jn_frame_t got;
    char a[JN_FORMAT_MAX];
    char b[JN_FORMAT_MAX];
    int rc;

    rc = jn_bus_send(tx, sent);
    if (rc != JN_OK) {
        FAIL("%s: sending failed (%s, errno %s)", what, jn_strerror(rc),
             strerror(errno));
        return;
    }
    rc = jn_bus_recv(rx, &got, 1000);
    if (rc != JN_OK) {
        FAIL("%s: receiving failed (%s)", what, jn_strerror(rc));
        return;
    }
    if (!jn_frame_equal(sent, &got)) {
        (void) jn_frame_format(sent, a, sizeof a);
        (void) jn_frame_format(&got, b, sizeof b);
        FAIL("%s: sent %s and received %s", what, a, b);
    }
}

int main(void)
{
    uint8_t twentyfour[24];
    jn_frame_t sent;
    jn_frame_t got;
    int rx;
    int tx;
    int rc;

    if (!jn_bus_available()) {
        return skipped(jn_bus_why_not());
    }

    rx = jn_bus_open(IFACE, 1);
    if (rx < 0) {
        printf("skip  " IFACE " could not be opened (%s). Bring it up with: "
               "sudo modprobe vcan; "
               "sudo ip link add dev " IFACE " type vcan; "
               "sudo ip link set up " IFACE "\n", strerror(errno));
        return 0;
    }
    tx = jn_bus_open(IFACE, 1);
    if (tx < 0) {
        printf("skip  a second socket on " IFACE " could not be opened (%s)\n",
               strerror(errno));
        jn_bus_close(rx);
        return 0;
    }

    memset(twentyfour, 0x5A, sizeof twentyfour);

    /* A classic frame first, then a flexible-data one. The socket option that
     * makes the second possible is the one thing that is silently wrong when
     * it is missing: without it the kernel hands over sixteen bytes and the
     * rest of the payload is gone before this code sees it. */
    (void) jn_frame_init(&sent, 0x123, "\x01\x02\x03", 3, 0);
    exchange(tx, rx, &sent, "a classic frame");

    (void) jn_frame_init(&sent, 0x321, twentyfour, 24, JN_F_FD | JN_F_BRS);
    exchange(tx, rx, &sent, "a 24 byte flexible-data frame");

    /* A filter the kernel applies. Everything but 0x123 is dropped before this
     * process is woken at all, which on a four joint bus is the difference
     * between waking for every frame and waking for the ones addressed here. */
    {
        jn_filter_t only_123;

        only_123.id = 0x123u;
        only_123.mask = JN_SFF_MASK;

        rc = jn_bus_filter(rx, &only_123, 1u);
        if (rc != JN_OK) {
            FAIL("installing a filter failed (%s, errno %s)",
                 jn_strerror(rc), strerror(errno));
        } else {
            (void) jn_frame_init(&sent, 0x456, "\xff", 1, 0);
            (void) jn_bus_send(tx, &sent);
            (void) jn_frame_init(&sent, 0x123, "\xaa", 1, 0);
            (void) jn_bus_send(tx, &sent);

            rc = jn_bus_recv(rx, &got, 1000);
            if (rc != JN_OK) {
                FAIL("a filtered socket received nothing (%s)",
                     jn_strerror(rc));
            } else if (got.id != 0x123u) {
                FAIL("a filtered socket received 0x%X", (unsigned) got.id);
            }

            /* And now nothing more. This is the half that proves the filter
             * dropped 0x456 rather than merely delivering it second. */
            rc = jn_bus_recv(rx, &got, 200);
            if (rc == JN_OK) {
                FAIL("a filtered socket went on to receive 0x%X",
                     (unsigned) got.id);
            } else if (rc != JN_E_TIMEOUT) {
                FAIL("the second receive failed for another reason (%s)",
                     jn_strerror(rc));
            }
        }
    }

    jn_bus_close(tx);
    jn_bus_close(rx);

    if (failures != 0) {
        printf("FAIL  %d problems above\n", failures);
        return 1;
    }
    printf("ok  two sockets on " IFACE ", classic and flexible-data,"
           " and a kernel filter\n");
    return 0;
}
