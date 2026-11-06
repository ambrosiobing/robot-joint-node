/* console.h: the first thing in this volume that can say anything.
 *
 * Chapter 4's image floor, fourth piece. The map says where things go, the
 * startup gets the part to main, baud.h computes the divisor, and this writes
 * the registers that turn a character into a bit stream on a wire.
 *
 * `[vendor]` USART3, PD8 transmit, PD9 receive, 115200, AF7, from the board's
 * own device tree and from the pinctrl file for this exact package,
 * stm32h7a3z(g-i)txq. The same file gives chapter 9's bus pins as PD0 and PD1
 * at AF9, which independently confirms what that chapter settled by reasoning
 * when st.com was unreachable.
 *
 * FIVE THINGS HERE GO WRONG QUIETLY.
 *
 *   - A CONSOLE CANNOT CHECK ITS OWN BAUD. It writes into a transmitter and
 *     never hears back, so there is no loopback and no echo. Chapter 10's
 *     lesson is harder here than there: a loopback at least proves wiring, and
 *     this proves nothing at all by itself. The instrument is the TERMINAL, a
 *     PC at a known fixed rate, and the board is what is measured. So
 *     console_report prints the clock it was given, the divisor it computed and
 *     the rate that follows: legible output then confirms the assumption, and
 *     illegible output says which assumption to doubt. Without those numbers
 *     printed, output confirms nothing.
 *
 *   - PORT D CARRIES THIS AND CHAPTER 9'S BUS. PD8 and PD9 here, PD0 and PD1
 *     there. MODER, OSPEEDR and PUPDR are one register for all sixteen pins, so
 *     every write to them is read, modify, write and nothing else, and init
 *     SNAPSHOTS the other pins' bits and checks they did not move. AFRL and
 *     AFRH split at pin 8, so the alternate function writes cannot reach the bus
 *     pins; that is luck rather than care, and it is written down because the
 *     next peripheral on this port may not be so lucky.
 *
 *   - A STALE TRANSMISSION-COMPLETE FLAG ANSWERS THE NEXT BYTE. TC is left set
 *     by the previous character, so a flush that waits for it returns at once,
 *     before the byte it was supposed to wait for has gone anywhere. Every
 *     put therefore CLEARS the flags it will later read, before writing the
 *     byte rather than after.
 *
 *   - AN ENABLE BIT THAT DID NOT TAKE LEAVES EVERY LATER WRITE SILENT. A
 *     peripheral with no clock accepts nothing and reads back zeroes, which is
 *     the same picture as a console that works and a terminal that is not
 *     listening. Both enable bits are read back, and the register offsets they
 *     live at are `[vendor]` reasoning from a sibling-named header rather than
 *     a reading of this part's manual, which is exactly why.
 *
 *   - AN UNBOUNDED WAIT ON A FLAG IS A BLANK CONSOLE. If the clock assumption
 *     is wrong enough, or the peripheral is not enabled, TXE never arrives.
 *     `while (!(ISR & TXE));` then produces nothing and no clue, which is
 *     indistinguishable from a part that never started. Every wait here is
 *     counted and bounded, the same rule chapter 9's executor follows.
 *
 * EVERY FUNCTION RETURNS A BOOL, so a refusal is expressible rather than
 * swallowed. A void putchar cannot report that the transmitter never became
 * ready, and the caller would print a whole banner into nothing.
 *
 * This file names no vendor header. It is allowed one, being under src/bsp, and
 * does not need one: a base address, an offset and a volatile access is the
 * whole of it.
 */
#ifndef JOINT_CONSOLE_H
#define JOINT_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#include "baud.h"

/* `[vendor]` Zephyr dts/arm/st/h7/stm32h7.dtsi for the three bases, and NuttX
 * arch/arm/src/stm32h7/hardware/ for the register offsets within them. */
#define CONSOLE_RCC_BASE    0x58024400u
#define CONSOLE_GPIOD_BASE  0x58020C00u
#define CONSOLE_USART3_BASE 0x40004800u

/* The pins, and the alternate function they take. */
#define CONSOLE_TX_PIN 8u
#define CONSOLE_RX_PIN 9u
#define CONSOLE_PIN_AF 7u

/* Chapter 9's pins on the same port, so init can check they did not move.
 * `[vendor]` the same pinctrl file, at AF9. */
#define CONSOLE_BUS_RX_PIN 0u
#define CONSOLE_BUS_TX_PIN 1u

typedef enum {
    CONSOLE_OK = 0,
    CONSOLE_BAD_ARGUMENT,       /* a null, a misaligned base, a zero bound */
    CONSOLE_RATE_REFUSED,       /* baud_solve said no; see `rate.verdict` */
    CONSOLE_CLOCK_NOT_ENABLED,  /* an enable bit did not read back */
    CONSOLE_REGISTER_NOT_TAKEN, /* a configuration write did not read back */
    CONSOLE_PINS_DISTURBED,     /* another pin's bits moved, bus pins included */
    CONSOLE_TX_TIMEOUT,         /* the transmitter never became ready */
} console_verdict_t;

typedef struct {
    volatile uint32_t *rcc;
    volatile uint32_t *gpio;
    volatile uint32_t *usart;

    uint32_t kernel_hz;         /* what the CALLER believes, not a measurement */
    baud_t   rate;              /* what that belief implies */

    uint32_t spin_limit;        /* reads a wait may take before it is a failure */
    uint32_t worst_spins;       /* the most any wait actually took, so a report
                                 * can say how close the bound came */
    uint32_t bytes;             /* characters accepted by the transmitter */

    console_verdict_t verdict;
} console_t;

/* Bring the console up, or refuse and say why.
 *
 * `kernel_hz` is what the caller believes the USART's kernel clock to be. It is
 * NOT measured here and cannot be: that is the open question this chapter has
 * not answered, and printing it is the whole point of console_report.
 *
 * Nothing is written to any register until the rate has been solved, so an
 * impossible rate leaves the part exactly as it was.
 */
bool console_init(console_t *c, uintptr_t rcc_base, uintptr_t gpio_base,
                  uintptr_t usart_base, uint32_t kernel_hz, uint32_t baud,
                  uint32_t spin_limit);

/* One character. False when the transmitter never became ready within the
 * bound, which is a report rather than a hang. */
bool console_put(console_t *c, char ch);

/* A null terminated string, stopping at the first character that could not be
 * queued. False if any did not go. */
bool console_write(console_t *c, const char *s);

/* Eight hex digits, upper case, no prefix. */
bool console_hex32(console_t *c, uint32_t v);

/* An unsigned decimal, no padding. */
bool console_dec(console_t *c, uint32_t v);

/* Wait until the last character has actually left the shift register, rather
 * than merely having been handed to the transmitter. A caller that resets or
 * stops without this loses whatever was still in flight. */
bool console_flush(console_t *c);

/* THE LINE THAT MAKES THE REST MEAN ANYTHING: the assumed clock, the prescaler,
 * the divisor, the register word, the rate that follows and the error in parts
 * per thousand. Print this first in any image that prints at all. */
bool console_report(console_t *c);

/* The refusal as a short stable token. "ok" when there was none. */
const char *console_verdict_name(console_verdict_t v);

#endif /* JOINT_CONSOLE_H */
