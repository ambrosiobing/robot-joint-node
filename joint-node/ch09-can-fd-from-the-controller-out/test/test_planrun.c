/* test_planrun.c: the executor against a register model, and against the log.
 *
 * The model here and the one in tools/gen_planrun.py are the same model, written
 * twice on purpose. Neither is compared against the other: both are compared
 * against test/vectors_planrun.json, so a disagreement names the bus access
 * rather than the symptom.
 *
 * What this exercises cannot be exercised on the board. A register that declines
 * a write does not announce it, which is the whole reason planrun.c reads
 * everything back; so the only place that rule can be shown to work is against a
 * register that declines on purpose.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written.
 */
#include <stdio.h>
#include <string.h>

#include "initplan.h"
#include "msgram.h"
#include "planrun.h"
#include "vectors_planrun.h"

static int failures;

/* ------------------------------------------------------------- the model
 *
 * 256 words, indexed by offset / 4. All zero except ENDN, which reads
 * 0x87654321, and CCCR, which starts with INIT set because the part comes out of
 * reset stopped. A write stores the word, with CCCR's CSR and CSA forced to zero
 * because they are a request and its acknowledgement rather than settings. Then
 * one deviation per scenario.
 */
#define MODEL_WORDS 256u
#define LOG_MAX     256u

typedef enum {
    SC_COOPERATIVE = 0,
    SC_ENDN_WRONG,
    SC_CCE_STUCK,
    SC_CCE_REVERTS,
    SC_INIT_STUCK,
    SC_TEST_DECLINES,
} scenario_t;

typedef struct {
    scenario_t scenario;
    uint32_t   mem[MODEL_WORDS];
    pr_access_t log[LOG_MAX];
    uint32_t   n;
    bool       overflow;
} model_t;

static void model_init(model_t *m, scenario_t scenario)
{
    memset(m, 0, sizeof *m);
    m->scenario = scenario;
    m->mem[FDCAN_ENDN / 4u] = (scenario == SC_ENDN_WRONG) ? 0x12345678u
                                                          : 0x87654321u;
    m->mem[FDCAN_CCCR / 4u] = FDCAN_CCCR_INIT;
}

static void note(model_t *m, uint32_t kind, uint32_t reg, uint32_t value)
{
    if (m->n >= LOG_MAX) {
        m->overflow = true;
        return;
    }
    m->log[m->n].kind = kind;
    m->log[m->n].reg = reg;
    m->log[m->n].value = value;
    m->n++;
}

static uint32_t model_read(void *ctx, uint32_t reg)
{
    model_t *m = (model_t *) ctx;
    const uint32_t v = (reg / 4u < MODEL_WORDS) ? m->mem[reg / 4u] : 0u;
    note(m, 0u, reg, v);
    /* A bit that does not latch: correct once, immediately after the write, and
     * gone by the next read. That is what turns a modify that succeeds into a
     * wait that never finishes, and nothing else in this file would find it. */
    if (m->scenario == SC_CCE_REVERTS && reg == FDCAN_CCCR) {
        m->mem[reg / 4u] = v & ~FDCAN_CCCR_CCE;
    }
    return v;
}

static void model_write(void *ctx, uint32_t reg, uint32_t value)
{
    model_t *m = (model_t *) ctx;
    note(m, 1u, reg, value);
    /* TEST is writable only while CCCR.TEST is set. A model that ignores writes
     * to it is exactly what a sequence writing TEST too early meets, and the
     * point is that the hardware says nothing about it. */
    if (reg == FDCAN_TEST && m->scenario == SC_TEST_DECLINES) {
        return;
    }
    if (reg == FDCAN_CCCR) {
        value &= ~FDCAN_CCCR_VOLATILE;
        if (m->scenario == SC_CCE_STUCK) {
            value &= ~FDCAN_CCCR_CCE;       /* the bit never sets */
        }
        if (m->scenario == SC_INIT_STUCK) {
            value |= FDCAN_CCCR_INIT;       /* the bit never clears */
        }
    }
    if (reg / 4u < MODEL_WORDS) {
        m->mem[reg / 4u] = value;
    }
}

static void model_clear(void *ctx, uint32_t from, uint32_t to)
{
    note((model_t *) ctx, 2u, from, to);
}

/* ---------------------------------------------------------------- the test */

static void check_u32(const char *scenario, const char *field,
                      uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("  FAIL %s: %s is 0x%08X, the vectors say 0x%08X\n",
               scenario, field, (unsigned) got, (unsigned) want);
        failures++;
    }
}

int main(void)
{
    /* Declared out here so the io checks at the end can reuse the last plan. */
    fdcan_step_t steps[FDCAN_PLAN_MAX];
    uint32_t count = 0u;

    for (int i = 0; i < PR_VECTOR_COUNT; i++) {
        const pr_vector_t *v = &PR_VECTORS[i];

        /* The configuration is per scenario now, because one of them needs
         * loopback: in normal mode TEST is written with zero and reads zero, so
         * a declined write there is indistinguishable from an obeyed one. */
        fdcan_config_t cfg;
        memset(&cfg, 0, sizeof cfg);
        cfg.kernel_hz = 8000000u;
        cfg.bitrate = 500000u;
        cfg.want_sp_permille = 800u;
        cfg.mode = (fdcan_mode_t) v->mode;
        cfg.one_shot = v->one_shot;

        bt_t timing;
        memset(&timing, 0, sizeof timing);
        memset(steps, 0, sizeof steps);
        count = 0u;
        if (!fdcan_plan(&cfg, &MSGRAM_LAYOUT, steps, FDCAN_PLAN_MAX, &count,
                        &timing)) {
            printf("  FAIL %s: its configuration did not plan\n", v->scenario);
            failures++;
            continue;
        }
        if (count != v->plan_steps) {
            printf("  FAIL %s: planned %u steps, the vectors say %u\n",
                   v->scenario, (unsigned) count, (unsigned) v->plan_steps);
            failures++;
            continue;
        }

        model_t m;
        model_init(&m, (scenario_t) v->scenario_code);

        fdcan_io_t io;
        memset(&io, 0, sizeof io);
        io.read = model_read;
        io.write = model_write;
        io.clear_ram = model_clear;
        io.ctx = &m;
        io.retries = PR_RETRIES;
        io.spin_limit = PR_SPIN_LIMIT;

        fdcan_run_t res;
        memset(&res, 0, sizeof res);
        const bool ok = fdcan_run(steps, count, &io, &res);

        if (ok != v->ok) {
            printf("  FAIL %s: run %s, the vectors say %s\n", v->scenario,
                   ok ? "completed" : "refused",
                   v->ok ? "it completes" : "it is refused");
            failures++;
            continue;
        }
        check_u32(v->scenario, "step", res.step, v->step);
        check_u32(v->scenario, "steps done", res.steps_done, v->steps_done);
        check_u32(v->scenario, "spins", res.spins, v->spins);
        check_u32(v->scenario, "read back", res.read_back, v->read_back);
        check_u32(v->scenario, "wrote", res.wrote, v->wrote);

        if (strcmp(fdcan_run_verdict(&res), v->verdict) != 0) {
            printf("  FAIL %s: verdict is %s, the vectors say %s\n",
                   v->scenario, fdcan_run_verdict(&res), v->verdict);
            failures++;
        }

        /* A failure has to name the step. "step 6 failed" sends a reader to the
         * source; "open configuration" sends them to the register. */
        if (!ok && (res.name == 0 || res.name[0] == '\0')) {
            printf("  FAIL %s: refused without naming the step\n", v->scenario);
            failures++;
        }

        if (m.overflow) {
            printf("  FAIL %s: the access log overflowed at %u\n", v->scenario,
                   (unsigned) LOG_MAX);
            failures++;
            continue;
        }
        if (m.n != v->access_count) {
            printf("  FAIL %s: %u bus accesses, the vectors say %u\n",
                   v->scenario, (unsigned) m.n, (unsigned) v->access_count);
            failures++;
            continue;       /* comparing the log entry by entry would be noise */
        }
        for (uint32_t a = 0u; a < m.n; a++) {
            if (m.log[a].kind != v->log[a].kind
                || m.log[a].reg != v->log[a].reg
                || m.log[a].value != v->log[a].value) {
                printf("  FAIL %s: access %u is (%u, 0x%03X, 0x%08X), the "
                       "vectors say (%u, 0x%03X, 0x%08X)\n", v->scenario,
                       (unsigned) a + 1u,
                       (unsigned) m.log[a].kind, (unsigned) m.log[a].reg,
                       (unsigned) m.log[a].value,
                       (unsigned) v->log[a].kind, (unsigned) v->log[a].reg,
                       (unsigned) v->log[a].value);
                failures++;
                break;      /* one is enough to identify where it diverged */
            }
        }
    }

    /* The io itself has to be refused when it cannot work, and a spin limit of
     * zero is the case worth naming: it reads as "no limit" and would make every
     * wait fail instead. Nothing may run. */
    {
        model_t m;
        model_init(&m, SC_COOPERATIVE);
        fdcan_io_t io;
        memset(&io, 0, sizeof io);
        io.read = model_read;
        io.write = model_write;
        io.clear_ram = model_clear;
        io.ctx = &m;
        io.retries = PR_RETRIES;
        io.spin_limit = 0u;

        fdcan_run_t res;
        memset(&res, 0, sizeof res);
        if (fdcan_run(steps, count, &io, &res)) {
            printf("  FAIL a spin limit of zero was accepted\n");
            failures++;
        } else if (res.step != 0u || m.n != 0u) {
            printf("  FAIL an unusable io ran %u accesses and reached step "
                   "%u\n", (unsigned) m.n, (unsigned) res.step);
            failures++;
        } else if (strcmp(fdcan_run_verdict(&res), "io-unusable") != 0) {
            printf("  FAIL an unusable io reported %s\n",
                   fdcan_run_verdict(&res));
            failures++;
        }

        /* And a missing function pointer, which is the same class of mistake and
         * would otherwise be a null call on the first step. */
        io.spin_limit = PR_SPIN_LIMIT;
        io.clear_ram = 0;
        memset(&res, 0, sizeof res);
        if (fdcan_run(steps, count, &io, &res)) {
            printf("  FAIL a null clear_ram was accepted\n");
            failures++;
        }
    }

    if (failures) {
        printf("test_planrun: %d failure(s) over %d scenarios\n",
               failures, PR_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d scenarios, every step, spin and bus access matched, and an "
           "unusable io ran nothing\n", PR_VECTOR_COUNT);
    return 0;
}
