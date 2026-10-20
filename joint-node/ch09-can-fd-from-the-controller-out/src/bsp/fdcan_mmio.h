/* fdcan_mmio.h: the three function pointers planrun.c asks for, pointed at
 * real memory.
 *
 * Chapter 9. This is the first file in this volume that is BELOW the board
 * layer, so it is the first one allowed to name an address. Everything in
 * src/bus/ is above it and may not, by chapter 4's rule, which is what lets the
 * whole bus layer be checked on a laptop.
 *
 * It still names no vendor header. It does not need one: planrun.c asks for a
 * read, a write and a clear, and all three are volatile accesses at a base plus
 * an offset. Pulling CMSIS in for that would put a vendor dependency under the
 * one file that is allowed to have one and does not require it.
 *
 * TWO BASES, NOT ONE, and that is the whole reason this file is not four lines.
 * planrun passes a REGISTER OFFSET to read and write, and a MESSAGE MEMORY BYTE
 * OFFSET to clear. The register block and the message memory are different
 * windows at different addresses, 0x800 bytes apart with another controller
 * instance's window in between, so an implementation with one base clears the
 * wrong memory and every access still succeeds.
 *
 *   FDCAN1 registers     0x4000A000, a 0x400 window
 *   FDCAN1 message RAM   0x4000AC00, 0x2800 bytes
 *
 * `[datasheet]` Both from doc/node-registers.md, which took them from Zephyr's
 * device tree for `st,stm32h7-fdcan` and NuttX's `stm32h7x3xx_memorymap.h`, and
 * the size from ST's HAL `FDCAN_MESSAGE_RAM_SIZE`.
 *
 * AN OFFSET PAST THE REGISTER WINDOW LANDS IN THE OTHER INSTANCE. The window is
 * 0x400 and the next window begins there, so a register offset of 0x400 or more
 * reads and writes FDCAN2 rather than failing. The configuration then appears to
 * take, because it does take, on the wrong controller. Both offsets are
 * therefore range checked here rather than trusted.
 *
 * TWO OF THE THREE CALLBACKS CANNOT REFUSE. `fdcan_write_fn` and
 * `fdcan_clear_fn` return void, so a range check here has nowhere to report to.
 * A refusal is recorded as a sticky fault instead, and `fdcan_mmio_ok` must be
 * asked after a run. A caller that checks only `fdcan_run`'s return value will
 * not hear about a refused clear, because the executor records every clear as
 * having worked: it is the one step in the whole plan that cannot fail.
 *
 * WHICH IS WHY THE CLEAR PROVES ITSELF FIRST. Writing zeroes and reading zeroes
 * back is not evidence, because a wrong base accepts every write and reads back
 * zeroes, which is the same picture as a perfect clear. So the clear writes a
 * pattern to the first and last word of the range, reads both back, and records
 * a fault if either disagrees, BEFORE zeroing anything. That makes the one
 * unfalsifiable step in the plan falsifiable, and it is safe at that point in
 * the sequence because the plan clears the message memory before any register
 * points at it.
 */
#ifndef JOINT_FDCAN_MMIO_H
#define JOINT_FDCAN_MMIO_H

#include <stdbool.h>
#include <stdint.h>

#include "planrun.h"

/* `[datasheet]` doc/node-registers.md. Passed to fdcan_mmio_init rather than
 * used directly, so the host test can point the same code at an array. */
#define FDCAN1_REGS_BASE    0x4000A000u
#define FDCAN1_REGS_BYTES   0x400u
#define FDCAN1_MSGRAM_BASE  0x4000AC00u
#define FDCAN1_MSGRAM_BYTES 0x2800u

/* The pattern the clear proves itself with. Chosen so that neither half is zero
 * and the two halves differ, because a bus that returns the low half twice and
 * one that works would look identical under a repeating pattern. */
#define FDCAN_MMIO_PROBE 0xA5C3F00Du

typedef struct {
    volatile uint32_t *regs;
    volatile uint32_t *msgram;
    uint32_t regs_bytes;
    uint32_t msgram_bytes;

    /* Sticky, because two of the three callbacks return void. Counted rather
     * than flagged so that a console line can say how many accesses were
     * refused, and the first one's offset and reason are kept because the first
     * is the one that explains the rest. */
    uint32_t faults;
    uint32_t fault_offset;
    const char *fault_what;
} fdcan_mmio_t;

/* Point the accessors at memory. Both bases must be word aligned and both sizes
 * must be non-zero multiples of four; a misaligned base faults on the first
 * access, and a size that is not a whole number of words means the last word of
 * a clear runs past the end.
 *
 * Takes the bases as uintptr_t rather than as pointers so a caller can pass a
 * literal address without a cast at every call site, and so the host test can
 * pass the address of an array through the same door.
 */
bool fdcan_mmio_init(fdcan_mmio_t *m,
                     uintptr_t regs_base, uint32_t regs_bytes,
                     uintptr_t msgram_base, uint32_t msgram_bytes);

/* Fill in the io planrun.c walks a plan with. `retries` and `spin_limit` are
 * passed through to that structure; a spin limit of zero is refused there, and
 * this refuses it here too so the failure names the right file. */
bool fdcan_mmio_io(fdcan_mmio_t *m, fdcan_io_t *io,
                   uint32_t retries, uint32_t spin_limit);

/* Whether every access was in range and the message memory answered its probe.
 * ASK THIS AFTER A RUN. fdcan_run can return true while this is false, because
 * a clear is recorded as having worked whatever happened. */
bool fdcan_mmio_ok(const fdcan_mmio_t *m);

/* The first fault's reason as a short stable token, and its offset through
 * `offset` when that is not NULL. "ok" when there was none. */
const char *fdcan_mmio_fault(const fdcan_mmio_t *m, uint32_t *offset);

#endif /* JOINT_FDCAN_MMIO_H */
