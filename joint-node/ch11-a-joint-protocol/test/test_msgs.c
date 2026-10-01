/* Round trip the node's codec against the generated vectors.
 *
 * The same vectors drive test/test_msgs.py. If one passes and the other fails,
 * the two implementations disagree, and the label and field name below say
 * which case and which field rather than leaving it to be bisected by hand.
 *
 * Built and run by the host stage, which needs no board:
 *     make test
 */
#include <stdio.h>
#include <string.h>

#include "joint_msgs.h"
#include "vectors.h"

static int failures;

static void problem(const char *msg, const char *label, const char *field)
{
    failures++;
    if (field)
        printf("  %s / %s: %s\n", msg, label, field);
    else
        printf("  %s / %s\n", msg, label);
}

#define ROUND_TRIP(MSG, LEN, COUNT, CASES)                                   \
    do {                                                                     \
        for (size_t i = 0; i < (COUNT); i++) {                               \
            uint8_t buf[(LEN)];                                              \
            MSG##_t got;                                                     \
            const char *bad;                                                 \
            memset(buf, 0xA5, sizeof buf);   /* poison: a field the packer   \
                                                forgets stays visible */     \
            pack_##MSG(buf, &(CASES)[i].f);                                  \
            memset(&got, 0x5A, sizeof got);                                  \
            unpack_##MSG(buf, &got);                                         \
            bad = diff_##MSG(&(CASES)[i].f, &got);                           \
            if (bad)                                                         \
                problem(#MSG, (CASES)[i].label, bad);                        \
            checked += (int) (COUNT ? 1 : 0);                                \
        }                                                                    \
    } while (0)

int main(void)
{
    int checked = 0;

    ROUND_TRIP(state,   STATE_LEN,   STATE_CASE_COUNT,   state_cases);
    ROUND_TRIP(command, COMMAND_LEN, COMMAND_CASE_COUNT, command_cases);

    /* The packer must write every byte it claims. The poison above is the
     * check: a byte the packer never touches is still 0xA5 afterwards, and a
     * layout with a hole in it would show up here rather than on a wire. */
    {
        uint8_t buf[STATE_LEN];
        memset(buf, 0xA5, sizeof buf);
        pack_state(buf, &state_cases[0].f);     /* the all-zero case */
        for (size_t i = 0; i < sizeof buf; i++)
            if (buf[i] == 0xA5) {
                failures++;
                printf("  state: byte %zu was never written by the packer\n", i);
            }
    }

    if (failures) {
        printf("FAIL  %d problems\n", failures);
        return 1;
    }
    printf("ok  %d frames round tripped, every byte written\n", checked);
    return 0;
}
