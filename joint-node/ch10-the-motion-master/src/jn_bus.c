/* The feature macro has to come before every include, because -std=c11 hides
 * clock_gettime, nanosleep and poll behind it. Without this line the file
 * compiles with implicit declarations and links against nothing. */
#define _POSIX_C_SOURCE 200809L

#include "jn_bus.h"

#if defined(__linux__)

#include <errno.h>
#include <net/if.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int jn_bus_available(void)
{
    return 1;
}

const char *jn_bus_why_not(void)
{
    return NULL;
}

int jn_bus_open(const char *iface, int fd_mode)
{
    struct sockaddr_can addr;
    unsigned ifindex;
    int sock;
    int on = 1;
    int keep;

    if (iface == NULL || iface[0] == '\0') {
        return JN_E_ARG;
    }

    /* if_nametoindex rather than ioctl(SIOCGIFINDEX): it is the portable call,
     * it needs no struct ifreq, and it sets ENODEV when the interface is
     * absent, which is the common case and the one worth reporting clearly. */
    ifindex = if_nametoindex(iface);
    if (ifindex == 0u) {
        return JN_E_SYS;
    }

    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        return JN_E_SYS;
    }

    if (fd_mode) {
        if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                       &on, (socklen_t) sizeof on) < 0) {
            keep = errno;
            close(sock);
            errno = keep;
            return JN_E_SYS;
        }
    }

    memset(&addr, 0, sizeof addr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = (int) ifindex;
    if (bind(sock, (struct sockaddr *) &addr, (socklen_t) sizeof addr) < 0) {
        keep = errno;
        close(sock);
        errno = keep;
        return JN_E_SYS;
    }

    return sock;
}

int jn_bus_filter(int sock, const jn_filter_t *filters, size_t n)
{
    struct can_filter cf[JN_MAX_FILTERS];
    size_t i;

    if (n > (size_t) JN_MAX_FILTERS) {
        return JN_E_ARG;
    }
    for (i = 0; i < n; i++) {
        cf[i].can_id = filters[i].id;
        cf[i].can_mask = filters[i].mask;
    }
    if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER,
                   cf, (socklen_t) (n * sizeof cf[0])) < 0) {
        return JN_E_SYS;
    }
    return JN_OK;
}

int jn_bus_send(int sock, const jn_frame_t *f)
{
    uint8_t buf[JN_FD_MTU];
    ssize_t written;
    int n;

    n = jn_frame_encode(f, buf, sizeof buf);
    if (n < 0) {
        return n;
    }
    written = write(sock, buf, (size_t) n);
    if (written != (ssize_t) n) {
        /* A raw CAN socket writes one whole frame or nothing, so a short write
         * is not something to retry around; it means the request was wrong. */
        return JN_E_SYS;
    }
    return JN_OK;
}

int jn_bus_recv(int sock, jn_frame_t *f, int timeout_ms)
{
    uint8_t buf[JN_FD_MTU];
    ssize_t got;

    if (timeout_ms >= 0) {
        struct pollfd pfd;
        int rc;

        pfd.fd = sock;
        pfd.events = POLLIN;
        pfd.revents = 0;
        rc = poll(&pfd, 1, timeout_ms);
        if (rc < 0) {
            return JN_E_SYS;
        }
        if (rc == 0) {
            return JN_E_TIMEOUT;
        }
    }

    got = read(sock, buf, sizeof buf);
    if (got < 0) {
        return JN_E_SYS;
    }
    /* The number of bytes read is how the kernel says which kind of frame this
     * was: sixteen for a classic one, seventy two for a flexible-data one. */
    return jn_frame_decode(f, buf, (size_t) got);
}

void jn_bus_close(int sock)
{
    if (sock >= 0) {
        close(sock);
    }
}

static uint64_t from_timespec(const struct timespec *ts)
{
    return (uint64_t) ts->tv_sec * JN_NS_PER_SEC
         + (uint64_t) ts->tv_nsec;
}

uint64_t jn_now_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return 0;
    }
    return from_timespec(&ts);
}

uint64_t jn_mono_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return from_timespec(&ts);
}

void jn_sleep_ns(uint64_t ns)
{
    struct timespec req;
    struct timespec rem;

    req.tv_sec = (time_t) (ns / JN_NS_PER_SEC);
    req.tv_nsec = (long) (ns % JN_NS_PER_SEC);
    while (nanosleep(&req, &rem) != 0) {
        if (errno != EINTR) {
            return;
        }
        req = rem;
    }
}

#else   /* no SocketCAN here */

#include <time.h>

int jn_bus_available(void)
{
    return 0;
}

const char *jn_bus_why_not(void)
{
    return "this build has no SocketCAN, because SocketCAN is a Linux "
           "interface and this is not Linux. The frame layout and the log "
           "format are still tested; the socket is not. Run this part on the "
           "Pi 4, or on any Linux with the vcan module: "
           "sudo modprobe vcan; sudo ip link add dev vcan0 type vcan; "
           "sudo ip link set up vcan0";
}

int jn_bus_open(const char *iface, int fd_mode)
{
    (void) iface;
    (void) fd_mode;
    return JN_E_NOSYS;
}

int jn_bus_filter(int sock, const jn_filter_t *filters, size_t n)
{
    (void) sock;
    (void) filters;
    (void) n;
    return JN_E_NOSYS;
}

int jn_bus_send(int sock, const jn_frame_t *f)
{
    (void) sock;
    (void) f;
    return JN_E_NOSYS;
}

int jn_bus_recv(int sock, jn_frame_t *f, int timeout_ms)
{
    (void) sock;
    (void) f;
    (void) timeout_ms;
    return JN_E_NOSYS;
}

void jn_bus_close(int sock)
{
    (void) sock;
}

/* Good enough to compile and to order events in one run, which is all the
 * things that use it on this platform can do anyway. */
uint64_t jn_now_ns(void)
{
    return (uint64_t) time(NULL) * JN_NS_PER_SEC;
}

uint64_t jn_mono_ns(void)
{
    return (uint64_t) clock() * (JN_NS_PER_SEC / (uint64_t) CLOCKS_PER_SEC);
}

void jn_sleep_ns(uint64_t ns)
{
    (void) ns;
}

#endif
