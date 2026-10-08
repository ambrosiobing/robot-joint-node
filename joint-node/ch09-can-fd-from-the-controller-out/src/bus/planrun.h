/* planrun.h: walking the plan, and refusing rather than hanging.
 *
 * Chapter 9 stage two. initplan.c says what to do and in what order. This says
 * how to do it, and it is a separate file for the same reason the plan is data:
 * two more failures live here and both are silent.
 *
 *   - A write that is never read back. Three registers in this sequence can
 *     decline a write in silence: CCE declines while INIT is clear, TEST
 *     declines while CCCR.TEST is clear, and a wrong base address accepts every
 *     write and reads back zeroes. A sequence that writes and moves on reports
 *     success in all three cases.
 *
 *   - A wait with no bound. Every ready flag in this sequence is a flag the
 *     hardware may never set. `while (!(reg & bit));` on a board with one
 *     serial port and no debugger produces a blank console and nothing else,
 *     which is indistinguishable from a part that never started.
 *
 * So every modify is read back and compared, every wait is counted and bounded,
 * and a failure names the step rather than returning a code. The hardware is
 * reached through three function pointers, so this file compiles and runs on a
 * host against a register model, which is where its own failure paths are
 * exercised.
 *
 * THIS FILE NAMES NO VENDOR HEADER AND MUST NOT, by chapter 4's rule. It does
 * not know what a register is; it knows how to ask somebody else to read one.
 */
#ifndef JOINT_PLANRUN_H
#define JOINT_PLANRUN_H

#include <stdbool.h>
#include <stdint.h>

#include "initplan.h"

/* How the hardware is reached. `ctx` is passed through untouched, so the host
 * test can hang a register model off it and src/bsp/ can hang a base address. */
typedef uint32_t (*fdcan_read_fn)(void *ctx, uint32_t reg);
typedef void     (*fdcan_write_fn)(void *ctx, uint32_t reg, uint32_t value);
typedef void     (*fdcan_clear_fn)(void *ctx, uint32_t from, uint32_t to);

typedef struct {
    fdcan_read_fn  read;
    fdcan_write_fn write;
    fdcan_clear_fn clear_ram;
    void          *ctx;

    /* How many times a modify is retried before it is called a failure. The
     * driver this sequence follows uses ten, and the reason for retrying at all
     * is that a register can take a write on the second attempt; the reason for
     * a limit is that one which never takes it must be reported. */
    uint32_t retries;

    /* How many reads a wait may take before it is a failure. Zero would make
     * every wait fail, which is why fdcan_run refuses a zero rather than
     * treating it as unlimited: a bound of none is the bug this exists to
     * prevent, and spelling it as 0 is too easy a mistake to honour. */
    uint32_t spin_limit;
} fdcan_io_t;

/* What happened, whether it worked or not.
 *
 * `wrote` and `read_back` are both recorded because printing both is what turns
 * a failed configuration into a readable fault: a wrong base address reads back
 * the reset value, a reserved field reads back zero, and a field that moved
 * between parts reads back something that is neither. A caller that prints only
 * "step 6 failed" has thrown away the part that identifies which. */
typedef struct {
    uint32_t    step;        /* 1 based; 0 when nothing ran at all */
    uint32_t    steps_done;  /* how many completed before this one */
    const char *name;        /* the failing step's own words */
    fdcan_op_t  op;
    uint32_t    reg;
    uint32_t    wrote;
    uint32_t    read_back;
    uint32_t    spins;       /* reads a wait took, or attempts a modify took */
    bool        ok;
} fdcan_run_t;

/* Walk the plan. True only when every step completed.
 *
 * On failure it stops at once and leaves `out` describing the step that failed,
 * which matters because the sequence is ordered: continuing past a refused
 * configuration change would write the rest of the configuration into a
 * peripheral that is not listening, and every one of those writes would appear
 * to succeed.
 *
 * Returns false immediately, with `out->step` 0, when the io is unusable: a null
 * function pointer, or a spin limit of zero. */
bool fdcan_run(const fdcan_step_t *steps, uint32_t count,
               const fdcan_io_t *io, fdcan_run_t *out);

/* Why a run failed, as a short stable token rather than a sentence, so a console
 * line can be grepped and a test can assert on it. "ok" when it did not fail. */
const char *fdcan_run_verdict(const fdcan_run_t *out);

#endif /* JOINT_PLANRUN_H */
