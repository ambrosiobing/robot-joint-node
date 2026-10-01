/* The socket, and the only part of this chapter that needs a kernel.
 *
 * Everything in jn_frame.h and jn_log.h is byte layout and arithmetic and runs
 * anywhere. This file is kept separate so that the rest stays testable on a
 * machine with no CAN support at all, which is most machines.
 *
 * The interface name is the only thing that changes when real hardware arrives:
 *
 *     jn_bus_open("vcan0", 1)    the virtual interface, no hardware anywhere
 *     jn_bus_open("can0", 1)     the adapter, once it is seated and bound
 *
 * Nothing else in this chapter knows the difference, which is the point of
 * doing the host half first.
 *
 * On a build where SocketCAN does not exist every call here returns JN_E_NOSYS
 * instead of failing to compile, and jn_bus_why_not says so in a sentence. A
 * program that reports why it cannot run is more use than one that will not
 * build.
 */
#ifndef JN_BUS_H
#define JN_BUS_H

#include "jn_frame.h"

/* More than a four joint bus needs, and small enough to sit on the stack. */
#define JN_MAX_FILTERS  16u

typedef struct {
    uint32_t id;      /* compared against jn_frame_wire_id, flag bits included */
    uint32_t mask;    /* the bits of that identifier the kernel must match */
} jn_filter_t;

/* Non-zero when this build can open a CAN socket at all. Says nothing about
 * whether any interface is up. */
int jn_bus_available(void);

/* NULL when SocketCAN is available, otherwise a sentence explaining what is
 * missing and what to do about it. */
const char *jn_bus_why_not(void);

/* A raw CAN socket bound to one interface. Returns the descriptor, which is
 * non-negative, or a negative jn_err_t with errno left as the system set it.
 *
 * With fd_mode set the socket accepts flexible-data frames. Without it the
 * kernel truncates every one of them to a classic frame, silently, and the
 * payload above eight bytes is gone before any of this code sees it. */
int jn_bus_open(const char *iface, int fd_mode);

/* Accept only these identifier and mask pairs. Filtering here means the kernel
 * drops the rest before they reach this process: on a four joint bus that is
 * the difference between waking for every frame and waking for the three
 * addressed to this node. Passing n == 0 installs the kernel's empty filter,
 * which accepts nothing, so it is a mute button rather than a reset. */
int jn_bus_filter(int sock, const jn_filter_t *filters, size_t n);

/* Returns JN_OK, or a negative jn_err_t. */
int jn_bus_send(int sock, const jn_frame_t *f);

/* One frame, waiting at most timeout_ms for it; a negative timeout waits
 * forever. Returns JN_OK, JN_E_TIMEOUT when nothing arrived, or another
 * negative jn_err_t. The frame's stamp is left at zero: the caller knows
 * better than this layer which clock it wants to attribute the arrival to. */
int jn_bus_recv(int sock, jn_frame_t *f, int timeout_ms);

void jn_bus_close(int sock);

/* Two clocks, because the two jobs are different. The wall clock is what a
 * recording is stamped with, so a log made here lines up with one made by
 * another tool. The monotonic clock is what a schedule is paced against,
 * because the wall clock can step backwards and a generator that waits for a
 * deadline in the past emits a burst. */
uint64_t jn_now_ns(void);
uint64_t jn_mono_ns(void);

/* Sleeps at least this long, resuming after an interrupted sleep rather than
 * returning early. */
void jn_sleep_ns(uint64_t ns);

#endif /* JN_BUS_H */
