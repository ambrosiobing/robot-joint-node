/* fdcan_io.h: how the silicon is reached, and nothing else.
 *
 * Chapter 9 stage two. This file holds three function pointers and the struct
 * that carries them, and it exists as its own file because TWO modules now ask
 * somebody else to read a register: planrun.c, which walks the configuration
 * plan, and txrx.c, which moves one frame in or out of the message memory.
 *
 * Before this, the type lived in planrun.h. A transfer does not run a plan, so
 * a transfer including the plan executor's header to learn what a register
 * accessor looks like would have been a dependency written the wrong way round
 * and then excused in a comment. Splitting it costs one file and says the true
 * thing: both modules depend on an accessor, neither depends on the other.
 *
 * Nothing moved but the text. The three signatures and the struct are character
 * for character what planrun.h carried, which is what makes the existing
 * vectors a proof of the move rather than a hope: nine test suites compare
 * against committed vectors and all of them still pass.
 *
 * THIS FILE NAMES NO VENDOR HEADER AND MUST NOT, by chapter 4's rule. It does
 * not know what a register is; it describes asking somebody else to read one.
 */
#ifndef JOINT_FDCAN_IO_H
#define JOINT_FDCAN_IO_H

#include <stdbool.h>
#include <stdint.h>

/* How the hardware is reached. `ctx` is passed through untouched, so the host
 * test can hang a register model off it and src/bsp/ can hang a base address. */
typedef uint32_t (*fdcan_read_fn)(void *ctx, uint32_t reg);
typedef void     (*fdcan_write_fn)(void *ctx, uint32_t reg, uint32_t value);
/* The clear RETURNS A BOOL where the other two do not, and the asymmetry is
 * deliberate rather than untidy.
 *
 * A refused WRITE is already expressible: the executor reads every write back,
 * so a write that did not take is caught by the comparison that follows it. A
 * refused CLEAR had nowhere to go at all. This callback returned void, so an
 * implementation that found the range misaligned, backwards or past the end of
 * the memory could do nothing but ignore it, and the executor recorded the step
 * as having worked. That made the clear THE ONE STEP IN THE WHOLE PLAN THAT
 * COULD NOT FAIL, and the board layer had to carry a sticky fault that a caller
 * was expected to remember to ask about afterwards.
 *
 * Changed on Monday 9 November 2026. A refusal that an implementation can
 * detect and cannot report is a signature to change, not a case to work
 * around. */
typedef bool     (*fdcan_clear_fn)(void *ctx, uint32_t from, uint32_t to);

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

#endif /* JOINT_FDCAN_IO_H */
