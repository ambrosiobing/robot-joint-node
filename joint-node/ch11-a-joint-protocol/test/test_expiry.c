/* The command state machine, and the clock wrap underneath it.
 *
 * Every case here is one a master can produce without meaning to: a repeated
 * frame, a late frame, a frame whose deadline sits the far side of the 32 bit
 * rollover. None of them needs a board.
 */
#include <stdio.h>
#include <string.h>

#include "command.h"

static int failures;

static void expect(int ok, const char *what)
{
    if (!ok) {
        failures++;
        printf("  %s\n", what);
    }
}

static command_t cmd(uint8_t seq, uint32_t valid_until)
{
    command_t c;
    memset(&c, 0, sizeof c);
    c.seq = seq;
    c.valid_until_us = valid_until;
    return c;
}

static void test_fresh_then_stale(void)
{
    cmd_tracker_t t;
    command_t c = cmd(7, 1000 + 5000);
    cmd_tracker_init(&t);

    expect(command_check(&t, &c, 1000) == CMD_FRESH, "a new command in window is not fresh");
    expect(command_check(&t, &c, 1100) == CMD_STALE, "the same sequence twice is not stale");

    c.seq = 8;
    expect(command_check(&t, &c, 1200) == CMD_FRESH, "a new sequence is not fresh");
}

static void test_expired(void)
{
    cmd_tracker_t t;
    command_t c = cmd(1, 5000);
    cmd_tracker_init(&t);

    expect(command_check(&t, &c, 6000) == CMD_EXPIRED, "a past deadline is not expired");

    /* An expired command must not move the sequence on, or the next genuine
     * frame carrying that sequence would be read as stale. */
    c.valid_until_us = 20000;
    expect(command_check(&t, &c, 6000) == CMD_FRESH,
           "a command rejected earlier blocked the same sequence later");
}

static void test_rejected_horizon(void)
{
    cmd_tracker_t t;
    command_t c = cmd(1, 1000 + CMD_MAX_HORIZON_US + 1);
    cmd_tracker_init(&t);

    expect(command_check(&t, &c, 1000) == CMD_REJECTED,
           "a deadline beyond the horizon is not rejected");

    c = cmd(2, 1000 + CMD_MAX_HORIZON_US);
    expect(command_check(&t, &c, 1000) == CMD_FRESH,
           "a deadline exactly at the horizon is not accepted");
}

static void test_wrap(void)
{
    /* now sits just below the rollover, the deadline just above it. The low 32
     * bits of the deadline are therefore a very small number, and a direct
     * comparison would read it as long past. */
    uint64_t now = 0xFFFFF000ull;
    uint32_t low = 0x00001000u;              /* 0x1_00001000 once expanded */
    cmd_tracker_t t;
    command_t c = cmd(1, low);
    cmd_tracker_init(&t);

    expect(cmd_expand_deadline(low, now) == 0x100001000ull,
           "a deadline across the rollover did not expand to the next epoch");
    expect(command_check(&t, &c, now) == CMD_FRESH,
           "a deadline across the rollover was not read as still ahead");

    /* And the other direction: now just above the rollover, deadline just
     * below it, which is genuinely in the past. */
    now = 0x100001000ull;
    low = 0xFFFFF000u;
    expect(cmd_expand_deadline(low, now) == 0xFFFFF000ull,
           "a deadline before the rollover did not expand to the previous epoch");
}

int main(void)
{
    test_fresh_then_stale();
    test_expired();
    test_rejected_horizon();
    test_wrap();

    if (failures) {
        printf("FAIL  %d problems\n", failures);
        return 1;
    }
    printf("ok  command state machine and the 32 bit wrap\n");
    return 0;
}
