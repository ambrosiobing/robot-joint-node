/* A command is valid until a time, not forever.
 *
 * This is the small idea that changes the failure mode of the whole system. A
 * master that stops transmitting produces a joint that stops, and it stops at
 * a time the master chose rather than a time this file's author guessed.
 *
 * Nothing here includes a vendor header, so it builds on a host and chapter
 * 20's first stage can exercise it with no board present.
 */
#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdbool.h>

#include "joint_msgs.h"

typedef enum {
    CMD_FRESH = 0,   /* new, and inside its validity window           */
    CMD_STALE,       /* the sequence number is the one already seen   */
    CMD_EXPIRED,     /* its validity window has passed                */
    CMD_REJECTED     /* valid so far ahead that the master's clock is suspect */
} cmd_state_t;

/* How far ahead a master may legitimately place a deadline. A command valid
 * for the next hour is not a generous master, it is a master whose clock is
 * wrong, and accepting it would hide that until the joint stopped responding
 * to anything newer. */
#define CMD_MAX_HORIZON_US  100000u     /* 100 ms, a hundred control periods */

typedef struct {
    uint8_t last_seq;
    bool    have_seen_one;
} cmd_tracker_t;

void cmd_tracker_init(cmd_tracker_t *t);

/* The frame carries the low 32 bits of a deadline and the node's clock is 64
 * bits wide, so the deadline is reconstructed against now rather than compared
 * to it directly. Exposed because it is the part worth testing on its own. */
uint64_t cmd_expand_deadline(uint32_t low32, uint64_t now_us);

/* Classify a received command. The tracker is updated only when the verdict is
 * CMD_FRESH: a stale or malformed command must not move the sequence on, or a
 * single corrupt frame would make the next genuine one look stale too. */
cmd_state_t command_check(cmd_tracker_t *t, const command_t *c, uint64_t now_us);

#endif /* COMMAND_H */
