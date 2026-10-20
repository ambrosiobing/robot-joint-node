/* test_fdcan_mmio.c: the board accessors, pointed at arrays instead of silicon.
 *
 * This runs THE REAL PLAN through THE REAL EXECUTOR through THE REAL ACCESSORS.
 * The only substitution is where the two bases point, which is the one thing a
 * host cannot provide, and it is substituted through the same argument the board
 * will use rather than through a compile-time switch.
 *
 * WHAT THIS CANNOT PROVE, stated here rather than left to be assumed. Two of the
 * message memory probe's refusals, "does not hold a pattern" and "aliases", are
 * properties of a bus that misbehaves. Real memory holds patterns and does not
 * alias, so those two paths are unreachable on a host and stay [unconfirmed]
 * until the board runs, exactly like the loopback itself. Everything else here
 * is reachable and is exercised.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written.
 */
#include <stdio.h>
#include <string.h>

#include "bittiming.h"
#include "fdcan_mmio.h"
#include "initplan.h"
#include "msgram.h"
#include "planrun.h"

static int failures;

static void fail(const char *what)
{
    printf("  FAIL %s\n", what);
    failures++;
}

/* The two windows, as arrays. Sized from the part's own numbers so the range
 * checks under test are the same ones the board will apply. */
#define REG_WORDS    (FDCAN1_REGS_BYTES / 4u)
#define MSGRAM_WORDS (FDCAN1_MSGRAM_BYTES / 4u)

/* Guard words either side of each window, and they are MEMBERS OF ONE STRUCT
 * rather than separate objects. C guarantees that struct members are laid out in
 * declaration order, and uint32_t arrays need no padding between them, so the
 * guards really do sit next to the window. As four separate static arrays they
 * would prove nothing: the compiler may place them anywhere, so an access one
 * word past the end could land in padding and the guards would still read
 * clean. On the board the memory past this window belongs to another controller
 * instance, so an overrun has somewhere real to go and must be caught here.
 */
#define GUARD_PATTERN 0xFEEDFACEu
#define GUARD_WORDS 4u

static struct {
    uint32_t guard_a[GUARD_WORDS];
    uint32_t regs[REG_WORDS];
    uint32_t guard_b[GUARD_WORDS];
    uint32_t msgram[MSGRAM_WORDS];
    uint32_t guard_c[GUARD_WORDS];
} arena;

#define regs   (arena.regs)
#define msgram (arena.msgram)

static void arena_reset(void)
{
    memset(&arena, 0, sizeof arena);
    for (unsigned i = 0u; i < GUARD_WORDS; i++) {
        arena.guard_a[i] = GUARD_PATTERN;
        arena.guard_b[i] = GUARD_PATTERN;
        arena.guard_c[i] = GUARD_PATTERN;
    }
}

static void guards_intact(const char *where)
{
    for (unsigned i = 0u; i < GUARD_WORDS; i++) {
        if (arena.guard_a[i] != GUARD_PATTERN ||
            arena.guard_b[i] != GUARD_PATTERN ||
            arena.guard_c[i] != GUARD_PATTERN) {
            printf("  FAIL %s: a guard word beside one of the two windows was "
                   "overwritten, so an access ran past its window\n", where);
            failures++;
            return;
        }
    }
}

/* The two values THE HARDWARE provides and no array can. Seeding them is what
 * makes the arrays a register model rather than a block of zeroes; leaving them
 * out is the control below. */
static void seed_hardware_values(void)
{
    regs[FDCAN_ENDN / 4u] = 0x87654321u;
    regs[FDCAN_CCCR / 4u] = FDCAN_CCCR_INIT;
}

static bool build_plan(fdcan_step_t *steps, uint32_t max, uint32_t *count)
{
    fdcan_config_t cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.kernel_hz = 8000000u;           /* the 8 MHz HSE, the only PLL-free source */
    cfg.bitrate = 500000u;
    cfg.want_sp_permille = 800u;
    cfg.mode = FDCAN_MODE_LOOPBACK_INTERNAL;
    cfg.one_shot = false;

    bt_t timing;
    memset(&timing, 0, sizeof timing);
    return fdcan_plan(&cfg, &MSGRAM_LAYOUT, steps, max, count, &timing);
}

int main(void)
{
    fdcan_step_t steps[FDCAN_PLAN_MAX];
    uint32_t count = 0u;

    memset(steps, 0, sizeof steps);
    if (!build_plan(steps, FDCAN_PLAN_MAX, &count) || count == 0u) {
        printf("  FAIL the plan this chapter configures does not plan, so "
               "nothing below can run\n");
        return 1;
    }

    /* ------------------------------------------------- init refuses the wrong */

    {
        fdcan_mmio_t m;
        memset(&m, 0, sizeof m);

        if (fdcan_mmio_init(&m, 0u, FDCAN1_REGS_BYTES,
                            (uintptr_t) msgram, sizeof msgram)) {
            fail("a register base of zero was accepted");
        }
        if (fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                            0u, sizeof msgram)) {
            fail("a message memory base of zero was accepted");
        }
        if (fdcan_mmio_init(&m, ((uintptr_t) regs) + 1u, FDCAN1_REGS_BYTES,
                            (uintptr_t) msgram, sizeof msgram)) {
            fail("a misaligned register base was accepted");
        }
        if (fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                            ((uintptr_t) msgram) + 2u, sizeof msgram)) {
            fail("a misaligned message memory base was accepted");
        }
        if (fdcan_mmio_init(&m, (uintptr_t) regs, 6u,
                            (uintptr_t) msgram, sizeof msgram)) {
            fail("a register window that is not a whole number of words was "
                 "accepted");
        }
        if (fdcan_mmio_init(&m, (uintptr_t) regs, 0u,
                            (uintptr_t) msgram, sizeof msgram)) {
            fail("a register window of no bytes was accepted");
        }
        /* Overlapping windows, both orders. One of the two bases being wrong is
         * how a register write comes to change a message element. */
        if (fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                            ((uintptr_t) regs) + 4u, 64u)) {
            fail("overlapping windows were accepted, message memory above");
        }
        if (fdcan_mmio_init(&m, ((uintptr_t) msgram) + 4u, 64u,
                            (uintptr_t) msgram, sizeof msgram)) {
            fail("overlapping windows were accepted, registers above");
        }
        /* And the one that must be accepted, so the refusals above are not
         * simply a function that always says no. */
        if (!fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                             (uintptr_t) msgram, sizeof msgram)) {
            fail("two separate aligned windows were refused");
        }
    }

    /* ---------------------------- THE CONTROL: a wrong base must be detected */

    /* An array of zeroes is what a wrong base address looks like from the
     * outside: every write is accepted and every read returns nothing. The plan
     * has to stop at the endianness register, which is the only register in the
     * sequence whose value the part supplies rather than the software. If this
     * arm passes, the sequence cannot tell a working controller from no
     * controller at all. */
    arena_reset();
    {
        fdcan_mmio_t m;
        fdcan_io_t io;
        fdcan_run_t out;
        memset(&m, 0, sizeof m);
        memset(&io, 0, sizeof io);
        memset(&out, 0, sizeof out);

        if (!fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                             (uintptr_t) msgram, sizeof msgram) ||
            !fdcan_mmio_io(&m, &io, 10u, 1000u)) {
            fail("the control arm could not be set up");
        } else if (fdcan_run(steps, count, &io, &out)) {
            fail("the plan completed against a block of zeroes, so a wrong "
                 "base address would configure nothing and report success");
        } else {
            if (strcmp(fdcan_run_verdict(&out), "expect-mismatch") != 0) {
                printf("  FAIL the control stopped with verdict %s and a wrong "
                       "base address reads back zero, which is an expect "
                       "mismatch\n", fdcan_run_verdict(&out));
                failures++;
            }
            if (out.reg != FDCAN_ENDN) {
                printf("  FAIL the control stopped at register 0x%03X and the "
                       "register that detects a wrong base is ENDN at "
                       "0x%03X\n", (unsigned) out.reg, (unsigned) FDCAN_ENDN);
                failures++;
            }
            if (out.read_back != 0u) {
                printf("  FAIL the control read back 0x%08X where a block of "
                       "zeroes reads 0\n", (unsigned) out.read_back);
                failures++;
            }
        }
        guards_intact("the control arm");
    }

    /* ------------------------------- the arm that must pass, and then its own */

    arena_reset();
    seed_hardware_values();
    {
        fdcan_mmio_t m;
        fdcan_io_t io;
        fdcan_run_t out;
        memset(&m, 0, sizeof m);
        memset(&io, 0, sizeof io);
        memset(&out, 0, sizeof out);

        if (!fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                             (uintptr_t) msgram, sizeof msgram) ||
            !fdcan_mmio_io(&m, &io, 10u, 1000u)) {
            fail("the working arm could not be set up");
        } else if (!fdcan_run(steps, count, &io, &out)) {
            printf("  FAIL the plan stopped at step %u of %u, %s, verdict %s, "
                   "wrote 0x%08X read back 0x%08X\n",
                   (unsigned) out.step, (unsigned) count,
                   out.name != 0 ? out.name : "",
                   fdcan_run_verdict(&out), (unsigned) out.wrote,
                   (unsigned) out.read_back);
            failures++;
        } else {
            if (out.steps_done + 1u != count && out.steps_done != count) {
                printf("  FAIL the plan reported %u steps done of %u\n",
                       (unsigned) out.steps_done, (unsigned) count);
                failures++;
            }
            /* AND THEN ASK THE ACCESSORS. fdcan_run can return true while the
             * accessors refused something, because the executor records every
             * clear as having worked whatever happened: it is the one step in
             * the plan that cannot fail. A caller that checks only the run has
             * not checked the clear. */
            uint32_t where = 0u;
            if (!fdcan_mmio_ok(&m)) {
                printf("  FAIL the plan completed and the accessors recorded "
                       "%u fault(s), first %s at 0x%04X\n",
                       (unsigned) m.faults, fdcan_mmio_fault(&m, &where),
                       (unsigned) where);
                failures++;
            }
            if (strcmp(fdcan_mmio_fault(&m, &where), "ok") != 0) {
                fail("a clean run does not report its fault as ok");
            }
        }

        /* The message memory the plan cleared must be zero across the range the
         * layout uses, and the guards must be untouched. */
        for (uint32_t w = 0u; w < MSGRAM_LAYOUT_BYTES / 4u; w++) {
            if (msgram[w] != 0u) {
                printf("  FAIL word %u of the message memory is 0x%08X after "
                       "the clear\n", (unsigned) w, (unsigned) msgram[w]);
                failures++;
                break;
            }
        }
        guards_intact("the working arm");
    }

    /* ------------------------------- the clear, with something to clear away */

    arena_reset();
    seed_hardware_values();
    {
        fdcan_mmio_t m;
        fdcan_io_t io;
        memset(&m, 0, sizeof m);
        memset(&io, 0, sizeof io);
        if (!fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                             (uintptr_t) msgram, sizeof msgram) ||
            !fdcan_mmio_io(&m, &io, 10u, 1000u)) {
            fail("the clear arm could not be set up");
        } else {
            /* Fill the whole window, so a clear that covers less than it claims
             * leaves evidence rather than agreeing with a zeroed array. */
            for (uint32_t w = 0u; w < MSGRAM_WORDS; w++) {
                msgram[w] = 0xDEADBEEFu;
            }
            const uint32_t used = MSGRAM_LAYOUT_BYTES;
            io.clear_ram(io.ctx, 0u, used);

            if (!fdcan_mmio_ok(&m)) {
                uint32_t where = 0u;
                printf("  FAIL clearing 0 to %u recorded %s at 0x%04X\n",
                       (unsigned) used, fdcan_mmio_fault(&m, &where),
                       (unsigned) where);
                failures++;
            }
            for (uint32_t w = 0u; w < used / 4u; w++) {
                if (msgram[w] != 0u) {
                    printf("  FAIL word %u is 0x%08X and was inside the range "
                           "cleared\n", (unsigned) w, (unsigned) msgram[w]);
                    failures++;
                    break;
                }
            }
            /* And the words PAST the range must still hold the fill. A clear
             * that runs long is invisible otherwise, and on the board the
             * memory past this layout belongs to the other controller. */
            for (uint32_t w = used / 4u; w < MSGRAM_WORDS; w++) {
                if (msgram[w] != 0xDEADBEEFu) {
                    printf("  FAIL word %u is 0x%08X and is past the range "
                           "cleared, so the clear ran long\n",
                           (unsigned) w, (unsigned) msgram[w]);
                    failures++;
                    break;
                }
            }
            guards_intact("the clear arm");
        }
    }

    /* ------------------------------------------ the accessors' own refusals */

    arena_reset();
    seed_hardware_values();
    {
        fdcan_mmio_t m;
        fdcan_io_t io;
        memset(&m, 0, sizeof m);
        memset(&io, 0, sizeof io);
        if (!fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                             (uintptr_t) msgram, sizeof msgram) ||
            !fdcan_mmio_io(&m, &io, 10u, 1000u)) {
            fail("the refusal arm could not be set up");
        } else {
            /* An offset at the end of the window is the NEXT controller
             * instance on this part, not reserved space, so it must be refused
             * rather than written. */
            const uint32_t beyond = FDCAN1_REGS_BYTES;
            io.write(io.ctx, beyond, 0x11111111u);
            if (fdcan_mmio_ok(&m)) {
                printf("  FAIL a write at offset 0x%03X, which is the next "
                       "controller instance, was not recorded as a fault\n",
                       (unsigned) beyond);
                failures++;
            }
            uint32_t where = 0u;
            if (strcmp(fdcan_mmio_fault(&m, &where),
                       "register-out-of-window") != 0) {
                printf("  FAIL an out of window write recorded %s\n",
                       fdcan_mmio_fault(&m, &where));
                failures++;
            }
            if (where != beyond) {
                printf("  FAIL the fault names offset 0x%04X and the write was "
                       "at 0x%04X\n", (unsigned) where, (unsigned) beyond);
                failures++;
            }
            if (io.read(io.ctx, beyond) != 0u) {
                fail("an out of window read returned something other than "
                     "zero");
            }

            const uint32_t faults_before = m.faults;
            io.write(io.ctx, 2u, 0x22222222u);      /* misaligned */
            if (m.faults == faults_before) {
                fail("a misaligned register offset was not recorded");
            }

            /* Clear ranges. The signature returns void, so each of these is
             * detectable only through the sticky count. */
            const struct { uint32_t from, to; const char *why; } bad[] = {
                { 0u, 0u, "an empty range" },
                { 64u, 32u, "a range that runs backwards" },
                { 0u, FDCAN1_MSGRAM_BYTES + 4u, "a range past the memory" },
                { 2u, 64u, "a misaligned start" },
                { 0u, 66u, "a misaligned end" },
            };
            for (unsigned i = 0u; i < sizeof bad / sizeof bad[0]; i++) {
                const uint32_t before = m.faults;
                io.clear_ram(io.ctx, bad[i].from, bad[i].to);
                if (m.faults == before) {
                    printf("  FAIL %s, %u to %u, was not recorded as a "
                           "fault\n", bad[i].why, (unsigned) bad[i].from,
                           (unsigned) bad[i].to);
                    failures++;
                }
            }
            /* The first fault is the one kept, and the later ones must not
             * overwrite it, because the first is the one that explains the
             * rest. */
            if (strcmp(fdcan_mmio_fault(&m, &where),
                       "register-out-of-window") != 0) {
                fail("a later fault overwrote the first one's reason");
            }
            guards_intact("the refusal arm");
        }
    }

    /* ------------------------------------------------------- unusable io */

    {
        fdcan_mmio_t m;
        fdcan_io_t io;
        memset(&m, 0, sizeof m);
        memset(&io, 0, sizeof io);
        if (!fdcan_mmio_init(&m, (uintptr_t) regs, FDCAN1_REGS_BYTES,
                             (uintptr_t) msgram, sizeof msgram)) {
            fail("setup for the unusable io arm failed");
        } else {
            if (fdcan_mmio_io(&m, &io, 10u, 0u)) {
                fail("a spin limit of zero was accepted, and a bound of none "
                     "is the failure the executor exists to prevent");
            }
            if (fdcan_mmio_io(0, &io, 10u, 1000u) ||
                fdcan_mmio_io(&m, 0, 10u, 1000u)) {
                fail("a null argument was accepted");
            }
        }
        if (fdcan_mmio_ok(0)) {
            fail("a null accessor set reported itself healthy");
        }
        if (strcmp(fdcan_mmio_fault(0, 0), "no-mmio") != 0) {
            fail("a null accessor set does not say so");
        }
    }

    if (failures) {
        printf("test_fdcan_mmio: %d failure(s)\n", failures);
        return 1;
    }
    printf("ok  %u plan steps walked through the real accessors, a block of "
           "zeroes refused at ENDN, %u bytes cleared and the words past them "
           "untouched\n",
           (unsigned) count, (unsigned) MSGRAM_LAYOUT_BYTES);
    return 0;
}
