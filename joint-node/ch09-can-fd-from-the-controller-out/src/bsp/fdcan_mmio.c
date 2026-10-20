/* fdcan_mmio.c: volatile accesses, range checks, and a clear that can fail.
 *
 * Every access is 32 bits wide. The message memory is written word by word
 * rather than with memset, because memset is free to use byte stores and
 * unaligned stores and this memory does not accept them.
 */
#include "fdcan_mmio.h"

static void fault(fdcan_mmio_t *m, const char *what, uint32_t offset)
{
    if (m == 0) {
        return;
    }
    if (m->faults == 0u) {
        /* The first one explains the rest, so only the first is kept. */
        m->fault_what = what;
        m->fault_offset = offset;
    }
    m->faults++;
}

/* A register offset is in range when it is word aligned and the whole word lies
 * inside the window. An offset of 0x400 is the other controller instance, not
 * an error the bus would report. */
static bool reg_ok(const fdcan_mmio_t *m, uint32_t reg)
{
    return (reg % 4u) == 0u && reg <= (m->regs_bytes - 4u);
}

static uint32_t mmio_read(void *ctx, uint32_t reg)
{
    fdcan_mmio_t *m = (fdcan_mmio_t *) ctx;
    if (m == 0) {
        return 0u;
    }
    if (!reg_ok(m, reg)) {
        fault(m, "register-out-of-window", reg);
        /* Zero rather than whatever is at the address. A read that reports a
         * neighbouring instance's register would be used, and the executor
         * compares read-backs, so a plausible wrong value is worse than an
         * obviously wrong one. */
        return 0u;
    }
    return m->regs[reg / 4u];
}

static void mmio_write(void *ctx, uint32_t reg, uint32_t value)
{
    fdcan_mmio_t *m = (fdcan_mmio_t *) ctx;
    if (m == 0) {
        return;
    }
    if (!reg_ok(m, reg)) {
        fault(m, "register-out-of-window", reg);
        return;     /* nothing is written, and the read back will disagree */
    }
    m->regs[reg / 4u] = value;
}

static void mmio_clear_ram(void *ctx, uint32_t from, uint32_t to)
{
    fdcan_mmio_t *m = (fdcan_mmio_t *) ctx;
    if (m == 0) {
        return;
    }
    /* `from` is the first byte and `to` is one past the last, which is how
     * initplan.c emits it. An empty range is refused rather than ignored: the
     * plan always has something to clear, so an empty range means the layout
     * arrived wrong and clearing nothing would leave the memory as found. */
    if ((from % 4u) != 0u || (to % 4u) != 0u) {
        fault(m, "clear-range-misaligned", from);
        return;
    }
    if (to <= from || to > m->msgram_bytes) {
        fault(m, "clear-range-out-of-memory", to);
        return;
    }

    const uint32_t first = from / 4u;
    const uint32_t last = (to / 4u) - 1u;

    /* THE PROBE, BEFORE THE CLEAR. Writing zeroes and reading zeroes back is
     * not evidence: a wrong base accepts every write and reads back zeroes,
     * which is indistinguishable from a memory that cleared perfectly. So a
     * pattern goes in first and has to come back. Safe here because the plan
     * clears this memory before any register points at it. */
    m->msgram[first] = FDCAN_MMIO_PROBE;
    m->msgram[last] = ~FDCAN_MMIO_PROBE;
    if (m->msgram[first] != FDCAN_MMIO_PROBE) {
        fault(m, "msgram-does-not-hold-a-pattern", from);
        return;
    }
    if (m->msgram[last] != ~FDCAN_MMIO_PROBE) {
        /* The two ends carry different patterns on purpose. A window that
         * aliases, so that every address answers with the same word, passes the
         * first check and fails this one. */
        fault(m, "msgram-aliases-or-is-shorter-than-claimed", to - 4u);
        return;
    }

    for (uint32_t w = first; w <= last; w++) {
        m->msgram[w] = 0u;
    }

    /* And read back the two ends again, because the clear itself is the step
     * the executor records as having worked whatever happened. */
    if (m->msgram[first] != 0u || m->msgram[last] != 0u) {
        fault(m, "msgram-did-not-clear", from);
    }
}

bool fdcan_mmio_init(fdcan_mmio_t *m,
                     uintptr_t regs_base, uint32_t regs_bytes,
                     uintptr_t msgram_base, uint32_t msgram_bytes)
{
    if (m == 0 || regs_base == 0u || msgram_base == 0u) {
        return false;
    }
    /* A misaligned base faults on the first access, which is loud, but it
     * happens after a whole configuration has been composed. Refusing here
     * costs two tests and names the argument. */
    if ((regs_base % 4u) != 0u || (msgram_base % 4u) != 0u) {
        return false;
    }
    if (regs_bytes < 4u || msgram_bytes < 4u) {
        return false;
    }
    if ((regs_bytes % 4u) != 0u || (msgram_bytes % 4u) != 0u) {
        return false;
    }
    /* The two windows must not overlap, in either order. An overlap means one
     * of the two bases is wrong, and the symptom is a register write that
     * changes a message element or the reverse, with nothing reported. */
    if (regs_base < msgram_base) {
        if (regs_base + regs_bytes > msgram_base) {
            return false;
        }
    } else {
        if (msgram_base + msgram_bytes > regs_base) {
            return false;
        }
    }

    m->regs = (volatile uint32_t *) regs_base;
    m->msgram = (volatile uint32_t *) msgram_base;
    m->regs_bytes = regs_bytes;
    m->msgram_bytes = msgram_bytes;
    m->faults = 0u;
    m->fault_offset = 0u;
    m->fault_what = 0;
    return true;
}

bool fdcan_mmio_io(fdcan_mmio_t *m, fdcan_io_t *io,
                   uint32_t retries, uint32_t spin_limit)
{
    if (m == 0 || io == 0 || m->regs == 0 || m->msgram == 0) {
        return false;
    }
    /* Refused here as well as in fdcan_run, so the failure names this file
     * when the mistake was made here. */
    if (spin_limit == 0u) {
        return false;
    }
    io->read = mmio_read;
    io->write = mmio_write;
    io->clear_ram = mmio_clear_ram;
    io->ctx = m;
    io->retries = retries;
    io->spin_limit = spin_limit;
    return true;
}

bool fdcan_mmio_ok(const fdcan_mmio_t *m)
{
    return m != 0 && m->faults == 0u;
}

const char *fdcan_mmio_fault(const fdcan_mmio_t *m, uint32_t *offset)
{
    if (m == 0) {
        return "no-mmio";
    }
    if (offset != 0) {
        *offset = m->fault_offset;
    }
    if (m->faults == 0u) {
        return "ok";
    }
    return m->fault_what != 0 ? m->fault_what : "unknown";
}
