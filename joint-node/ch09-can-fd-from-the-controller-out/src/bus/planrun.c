/* planrun.c: the executor. Every write verified, every wait bounded.
 *
 * The two rules below are the whole file, and both exist because the failure
 * they prevent is silent on real hardware.
 *
 * EVERY WRITE IS READ BACK. Three registers in this sequence can decline a
 * write without saying so: CCE declines while INIT is clear, TEST declines while
 * CCCR.TEST is clear, and a wrong peripheral base accepts every write and reads
 * back the reset value. Comparing the read-back against what was intended costs
 * one bus read per step and converts all three into a named failure.
 *
 * The risk the rule carries is the opposite one: a register that legitimately
 * does not read back what was written would make the image refuse a
 * configuration that is in fact correct. Every register this plan writes is a
 * configuration register that reads back while CCE is set, so the rule holds as
 * written; if one turns out not to, the fix is to learn which bits lie and mask
 * those, NOT to stop comparing.
 *
 * EVERY WAIT IS BOUNDED AND COUNTED. `while (!(reg & bit));` on a board with one
 * serial port and no debugger produces a blank console, which is
 * indistinguishable from a part that never started. A bound turns it into a step
 * number and a spin count.
 */
#include "planrun.h"

/* Bits that are a request and its acknowledgement rather than settings, so a
 * read-back comparison must ignore them. Only CCCR has any: comparing all 32
 * bits there reports a mismatch on a bit that was never the caller's to set,
 * which is a false failure and worse than none, because it trains a reader to
 * ignore the check. */
static uint32_t volatile_bits(uint32_t reg)
{
    return (reg == FDCAN_CCCR) ? FDCAN_CCCR_VOLATILE : 0u;
}

static void record(fdcan_run_t *out, uint32_t step, const fdcan_step_t *s,
                   uint32_t wrote, uint32_t read_back, uint32_t spins, bool ok)
{
    out->step = step;
    out->steps_done = step > 0u ? step - 1u : 0u;
    out->name = s != 0 ? s->name : "";
    out->op = s != 0 ? s->op : FDCAN_OP_READ;
    out->reg = s != 0 ? s->reg : 0u;
    out->wrote = wrote;
    out->read_back = read_back;
    out->spins = spins;
    out->ok = ok;
}

const char *fdcan_run_verdict(const fdcan_run_t *out)
{
    if (out == 0) {
        return "no-result";
    }
    if (out->ok) {
        return "ok";
    }
    if (out->step == 0u) {
        return "io-unusable";
    }
    switch (out->op) {
    case FDCAN_OP_EXPECT:     return "expect-mismatch";
    case FDCAN_OP_MODIFY:     return "modify-refused";
    case FDCAN_OP_WAIT_SET:   return "wait-set-timeout";
    case FDCAN_OP_WAIT_CLEAR: return "wait-clear-timeout";
    case FDCAN_OP_WRITE:      return "write-not-read-back";
    default:                  return "failed";
    }
}

bool fdcan_run(const fdcan_step_t *steps, uint32_t count,
               const fdcan_io_t *io, fdcan_run_t *out)
{
    if (out == 0) {
        return false;
    }
    record(out, 0u, 0, 0u, 0u, 0u, false);

    if (steps == 0 || io == 0 || io->read == 0 || io->write == 0
        || io->clear_ram == 0) {
        return false;
    }
    /* A spin limit of zero would make every wait fail, and a limit of none is
     * the bug this file exists to prevent, so neither reading is honoured. */
    if (io->spin_limit == 0u) {
        return false;
    }

    for (uint32_t i = 0u; i < count; i++) {
        const fdcan_step_t *s = &steps[i];
        const uint32_t n = i + 1u;

        switch (s->op) {
        case FDCAN_OP_READ: {
            const uint32_t got = io->read(io->ctx, s->reg);
            record(out, n, s, 0u, got, 0u, true);
            break;
        }

        case FDCAN_OP_EXPECT: {
            const uint32_t got = io->read(io->ctx, s->reg);
            const bool ok = ((got & s->mask) == (s->value & s->mask));
            record(out, n, s, s->value, got, 0u, ok);
            if (!ok) {
                return false;
            }
            break;
        }

        case FDCAN_OP_WRITE: {
            io->write(io->ctx, s->reg, s->value);
            const uint32_t got = io->read(io->ctx, s->reg);
            const uint32_t ignore = volatile_bits(s->reg);
            const bool ok = ((got & ~ignore) == (s->value & ~ignore));
            record(out, n, s, s->value, got, 1u, ok);
            if (!ok) {
                return false;
            }
            break;
        }

        case FDCAN_OP_MODIFY: {
            /* Read, modify, write, read back, and try again if it did not take.
             * The retry is not optimism: a register can accept a write on the
             * second attempt. The bound is what makes one that never accepts it
             * a report rather than a hang. */
            uint32_t attempts = 0u;
            uint32_t want = 0u, got = 0u;
            bool ok = false;
            const uint32_t ignore = volatile_bits(s->reg);

            while (attempts < io->retries) {
                const uint32_t cur = io->read(io->ctx, s->reg);
                want = (cur & ~s->mask) | (s->value & s->mask);
                io->write(io->ctx, s->reg, want);
                got = io->read(io->ctx, s->reg);
                attempts++;
                if ((got & ~ignore) == (want & ~ignore)) {
                    ok = true;
                    break;
                }
            }
            record(out, n, s, want, got, attempts, ok);
            if (!ok) {
                return false;
            }
            break;
        }

        case FDCAN_OP_WAIT_SET:
        case FDCAN_OP_WAIT_CLEAR: {
            const bool want_set = (s->op == FDCAN_OP_WAIT_SET);
            uint32_t spins = 0u;
            uint32_t got = 0u;
            bool ok = false;

            while (spins < io->spin_limit) {
                got = io->read(io->ctx, s->reg);
                spins++;
                const bool all_set = ((got & s->mask) == s->mask);
                const bool all_clear = ((got & s->mask) == 0u);
                if (want_set ? all_set : all_clear) {
                    ok = true;
                    break;
                }
            }
            record(out, n, s, 0u, got, spins, ok);
            if (!ok) {
                return false;
            }
            break;
        }

        case FDCAN_OP_CLEAR_RAM: {
            /* `reg` is the first byte and `mask` is one past the last, which is
             * how initplan emits it. The caller owns the walk because the stride
             * and the access width belong to the bus, not to the plan. */
            io->clear_ram(io->ctx, s->reg, s->mask);
            record(out, n, s, 0u, 0u, 0u, true);
            break;
        }

        default:
            /* An opcode this executor does not know is a refusal, not a skip.
             * Skipping a step in an ordered sequence is how the rest of a
             * configuration gets written into a peripheral that is not
             * listening, with every one of those writes appearing to work. */
            record(out, n, s, 0u, 0u, 0u, false);
            return false;
        }
    }

    /* Every step completed. `step` holds the last one so a caller can print how
     * far it got either way, and ok distinguishes the two. */
    out->ok = true;
    out->steps_done = count;
    return true;
}
