/* test_initplan.c: the C plan against the generated vectors.
 *
 * The vectors come from tools/gen_initplan.py, which is the reference. The two
 * are never compared against each other: both are compared against the vector
 * file, so when they disagree the output names the step and the field rather
 * than leaving somebody to guess which side moved.
 *
 * There is no C compiler on the win11 aquamarine authoring laptop, so this is
 * never built where it is written. It is built in WSL on JPTOUPM678 and in
 * continuous integration with -std=c11 -Wall -Wextra -Werror.
 */
#include <stdio.h>
#include <string.h>

#include "initplan.h"
#include "msgram.h"
#include "vectors_initplan.h"

static int failures;

static void check_u32(const char *label, int step, const char *field,
                      uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("  FAIL %s: step %d %s is 0x%08X, the vectors say 0x%08X\n",
               label, step, field, (unsigned) got, (unsigned) want);
        failures++;
    }
}

int main(void)
{
    int refusals = 0;

    for (int i = 0; i < IP_VECTOR_COUNT; i++) {
        const ip_vector_t *v = &IP_VECTORS[i];

        fdcan_config_t cfg;
        memset(&cfg, 0, sizeof cfg);
        cfg.kernel_hz = v->kernel_hz;
        cfg.bitrate = v->bitrate;
        cfg.want_sp_permille = v->want_sp_permille;
        cfg.mode = (fdcan_mode_t) v->mode;
        cfg.one_shot = v->one_shot;

        fdcan_step_t steps[FDCAN_PLAN_MAX];
        memset(steps, 0, sizeof steps);
        uint32_t count = 0u;
        bt_t timing;
        memset(&timing, 0, sizeof timing);

        const bool ok = fdcan_plan(&cfg, &MSGRAM_LAYOUT, steps,
                                   FDCAN_PLAN_MAX, &count, &timing);

        if (ok != v->ok) {
            printf("  FAIL %s: %s, and the vectors say %s\n", v->label,
                   ok ? "planned" : "refused",
                   v->ok ? "it plans" : "it is refused");
            failures++;
            continue;
        }
        if (!v->ok) {
            refusals++;
            /* A refusal must leave nothing behind for a caller to misread. */
            if (count != 0u) {
                printf("  FAIL %s: refused but reported %u steps\n",
                       v->label, (unsigned) count);
                failures++;
            }
            continue;
        }

        if (count != v->step_count) {
            printf("  FAIL %s: %u steps, the vectors say %u\n", v->label,
                   (unsigned) count, (unsigned) v->step_count);
            failures++;
            continue;       /* comparing further would print noise */
        }

        for (uint32_t s = 0u; s < count; s++) {
            const int n = (int) s + 1;
            check_u32(v->label, n, "op", (uint32_t) steps[s].op,
                      v->steps[s].op);
            check_u32(v->label, n, "register", steps[s].reg, v->steps[s].reg);
            check_u32(v->label, n, "mask", steps[s].mask, v->steps[s].mask);
            check_u32(v->label, n, "value", steps[s].value, v->steps[s].value);

            /* Every step has to say what it is for. A plan that stops at step
             * eleven and cannot say which register that was is not debuggable
             * on a board with one serial port. */
            if (steps[s].name == 0 || steps[s].name[0] == '\0') {
                printf("  FAIL %s: step %d has no name\n", v->label, n);
                failures++;
            }
            if (steps[s].op > FDCAN_OP_EXPECT) {
                printf("  FAIL %s: step %d has opcode %u, which is not one\n",
                       v->label, n, (unsigned) steps[s].op);
                failures++;
            }
        }
    }

    /* The array size the plan is written into is a compile time constant, and a
     * plan that outgrew it must refuse rather than overrun. Asked for one step
     * fewer than the real count, it has to say no. */
    {
        fdcan_config_t cfg;
        memset(&cfg, 0, sizeof cfg);
        cfg.kernel_hz = 8000000u;
        cfg.bitrate = 500000u;
        cfg.want_sp_permille = 800u;
        cfg.mode = FDCAN_MODE_NORMAL;

        fdcan_step_t steps[FDCAN_PLAN_MAX];
        uint32_t count = 0u;
        bt_t timing;
        memset(&timing, 0, sizeof timing);

        if (!fdcan_plan(&cfg, &MSGRAM_LAYOUT, steps, FDCAN_PLAN_MAX, &count,
                        &timing)) {
            printf("  FAIL the first image's own configuration did not plan\n");
            failures++;
        } else {
            const uint32_t full = count;
            uint32_t short_count = 0u;
            memset(steps, 0, sizeof steps);
            if (fdcan_plan(&cfg, &MSGRAM_LAYOUT, steps, full - 1u,
                           &short_count, &timing)) {
                printf("  FAIL a plan of %u steps fitted into %u, so the "
                       "bound is not enforced\n", (unsigned) full,
                       (unsigned) (full - 1u));
                failures++;
            } else if (short_count != 0u) {
                printf("  FAIL a refused plan reported %u steps\n",
                       (unsigned) short_count);
                failures++;
            }

            /* And the timing it reports must be the one it packed, or an image
             * would print 500 kbit/s beside a word that says something else.
             *
             * The plan is rebuilt here rather than reusing the array above,
             * because the refused call just overwrote part of it. Reading a
             * failed run's leftovers and calling them a result is the kind of
             * mistake this whole file exists to catch, so it would be a poor
             * place to make one. */
            memset(steps, 0, sizeof steps);
            count = 0u;
            memset(&timing, 0, sizeof timing);
            if (!fdcan_plan(&cfg, &MSGRAM_LAYOUT, steps, FDCAN_PLAN_MAX,
                            &count, &timing)) {
                printf("  FAIL the configuration planned once and then "
                       "refused\n");
                failures++;
            } else {
                uint32_t word = 0u;
                if (!bt_pack_nbtp(&timing, &word)) {
                    printf("  FAIL the reported timing does not pack\n");
                    failures++;
                } else {
                    bool found = false;
                    for (uint32_t s = 0u; s < count; s++) {
                        if (steps[s].op == FDCAN_OP_WRITE
                            && steps[s].reg == FDCAN_NBTP) {
                            found = true;
                            if (steps[s].value != word) {
                                printf("  FAIL NBTP step writes 0x%08X but "
                                       "the reported timing packs to "
                                       "0x%08X\n",
                                       (unsigned) steps[s].value,
                                       (unsigned) word);
                                failures++;
                            }
                        }
                    }
                    if (!found) {
                        printf("  FAIL no step writes NBTP\n");
                        failures++;
                    }
                }
                if (count != full) {
                    printf("  FAIL the same configuration planned %u steps "
                           "and then %u\n", (unsigned) full,
                           (unsigned) count);
                    failures++;
                }
            }
        }
    }

    if (failures) {
        printf("test_initplan: %d failure(s) over %d configurations\n",
               failures, IP_VECTOR_COUNT);
        return 1;
    }
    printf("ok  %d configurations, %d of them refused, every step matched "
           "and the bound holds\n", IP_VECTOR_COUNT, refusals);
    return 0;
}
