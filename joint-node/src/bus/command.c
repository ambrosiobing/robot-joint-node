#include "command.h"

#define WRAP        0x100000000ull      /* the low 32 bits roll over here */
#define HALF_WRAP   0x80000000ull

void cmd_tracker_init(cmd_tracker_t *t)
{
    t->last_seq = 0;
    t->have_seen_one = false;
}

uint64_t cmd_expand_deadline(uint32_t low32, uint64_t now_us)
{
    /* Put the low bits into the current epoch, then pick the neighbouring
     * epoch when that lands more than half a wrap away. A deadline is always
     * near now in a system with a hundred control periods of horizon, so
     * "nearest" is the right rule and the only ambiguous case is exactly half
     * a wrap out, which is thirty-five minutes and not a real command. */
    uint64_t candidate = (now_us & ~(WRAP - 1)) | (uint64_t) low32;

    if (candidate + HALF_WRAP < now_us)
        candidate += WRAP;
    else if (now_us + HALF_WRAP < candidate)
        candidate -= WRAP;

    return candidate;
}

cmd_state_t command_check(cmd_tracker_t *t, const command_t *c, uint64_t now_us)
{
    uint64_t deadline;

    if (t->have_seen_one && c->seq == t->last_seq)
        return CMD_STALE;

    deadline = cmd_expand_deadline(c->valid_until_us, now_us);

    if (now_us > deadline)
        return CMD_EXPIRED;

    if (deadline > now_us + CMD_MAX_HORIZON_US)
        return CMD_REJECTED;

    /* Only a command that is going to be acted on moves the sequence on. */
    t->last_seq = c->seq;
    t->have_seen_one = true;
    return CMD_FRESH;
}
