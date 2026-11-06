#!/usr/bin/env python3
"""The console, checked by construct for the five mistakes with no symptom.

    python test/test_console.py

None of this runs the code; a console cannot be run without a board and cannot
check its own baud even then. What it can do is read console.c for the
constructs that have to be right, and two of these are about ORDER, which no
amount of reading the registers afterwards would reveal.

Every rule matches the construct and not the prose beside it: the comments are
stripped first, so a rule cannot pass on a file whose code was deleted and whose
explanation was left behind.
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
CONSOLE = HERE / "src" / "bsp" / "console.c"
HEADER = HERE / "src" / "bsp" / "console.h"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"  FAIL {msg}")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def body_of(code, name):
    m = re.search(r"\b" + re.escape(name) + r"\s*\([^)]*\)\s*\{", code)
    if not m:
        return None
    i = m.end() - 1
    depth = 0
    for j in range(i, len(code)):
        if code[j] == "{":
            depth += 1
        elif code[j] == "}":
            depth -= 1
            if depth == 0:
                return code[i + 1:j]
    return None


def main():
    code = strip_comments(CONSOLE.read_text(encoding="utf-8"))
    header = strip_comments(HEADER.read_text(encoding="utf-8"))

    init = body_of(code, "console_init")
    put = body_of(code, "console_put")
    flush = body_of(code, "console_flush")
    report = body_of(code, "console_report")

    for name, b in (("console_init", init), ("console_put", put),
                    ("console_flush", flush), ("console_report", report)):
        if b is None:
            fail(f"{name} was not found, so every rule about it is vacuous")
    if init is None or put is None or flush is None:
        print(f"test_console: {len(failures)} failure(s)")
        return 1

    # 1. THE SHARED REGISTERS ARE NEVER ASSIGNED, ONLY READ, MODIFIED, WRITTEN.
    # MODER, OSPEEDR and PUPDR are one word for all sixteen pins of port D, and
    # two of those pins are chapter 9's bus. A plain assignment to any of them
    # puts the other fourteen wherever the written value says, and a console
    # that works while having quietly returned PD0 and PD1 to inputs is the
    # worst outcome available: chapter 9 would be debugging a bus this file
    # broke.
    for reg in ("GPIO_MODER", "GPIO_OSPEEDR", "GPIO_PUPDR", "GPIO_AFRH"):
        for m in re.finditer(r"gpio\s*\[\s*" + reg + r"\s*\]\s*=\s*([^;]+);",
                             init):
            rhs = m.group(1)
            # The right hand side has to derive from the register's own prior
            # value, whether through a snapshot or a direct read.
            if not re.search(r"_before\b", rhs) \
                    and not re.search(r"gpio\s*\[\s*" + reg, rhs) \
                    and not re.search(r"\b(moder|ospeedr|pupdr|afrh)\b", rhs):
                fail(f"{reg} is assigned from {rhs.strip()!r}, which does not "
                     f"derive from its own prior value, so the other pins on "
                     f"port D take whatever that expression says")

    # 2. The non-owned bits are snapshotted and compared. Rule 1 stops the
    # obvious form; this is the one that catches a mask that is wrong.
    if not re.search(r"moder_before", init):
        fail("console_init does not snapshot MODER, so nothing checks that the "
             "other pins on port D kept their settings")
    if not re.search(r"CONSOLE_PINS_DISTURBED", init):
        fail("console_init never refuses with PINS_DISTURBED, so the snapshot "
             "is taken and never used")

    # 3. BOTH ENABLE BITS ARE READ BACK. The offsets they live at are vendor
    # reasoning from a sibling-named header rather than a reading of this
    # part's manual, and a peripheral with no clock accepts every write and
    # reads back zeroes, which looks exactly like a working console nobody is
    # listening to.
    for bit in ("RCC_GPIOD_EN", "RCC_USART3_EN"):
        writes = len(re.findall(r"\|=\s*" + bit, init))
        reads = len(re.findall(r"&\s*" + bit, init))
        if writes == 0:
            fail(f"{bit} is never set")
        if reads == 0:
            fail(f"{bit} is set and never read back, so a clock that did not "
                 f"come on would leave every later write silent")

    # 4. THE ORDER IN console_put: THE FLAGS ARE CLEARED BEFORE THE BYTE. A
    # transmission-complete flag left set by the previous character makes a
    # later flush return at once, before this byte has gone anywhere. Clearing
    # after the write would be too late and would read as correct.
    icr = put.find("USART_ICR")
    tdr = put.find("USART_TDR")
    if icr < 0:
        fail("console_put never clears the status flags, so a flag left by the "
             "previous character answers this one's flush")
    elif tdr < 0:
        fail("console_put never writes TDR, so nothing is transmitted")
    elif icr > tdr:
        fail("console_put clears the status flags AFTER writing the byte, so a "
             "flush waiting for transmission complete reads the previous "
             "character's flag and returns before this byte has gone")

    # 5. THE ORDER IN console_init: CR1 IS CLEARED BEFORE BRR AND PRESC. The
    # divisor registers have no guarantee while the peripheral is enabled, and
    # a divisor that half takes is a rate nobody computed.
    cr1_zero = re.search(r"usart\s*\[\s*USART_CR1\s*\]\s*=\s*0u?\s*;", init)
    brr_write = re.search(r"usart\s*\[\s*USART_BRR\s*\]\s*=", init)
    if not cr1_zero:
        fail("console_init never disables the peripheral before configuring "
             "it, so BRR and PRESC are written to a running USART")
    elif brr_write and cr1_zero.start() > brr_write.start():
        fail("console_init writes BRR before clearing CR1, so the divisor is "
             "written to a running USART")

    # 6. PRESC IS WRITTEN EXPLICITLY. Its reset value happens to be divide by
    # one, which is what makes leaving it alone work by luck rather than by
    # decision, and chapter 9 learned that lesson on the element sizes.
    if not re.search(r"usart\s*\[\s*USART_PRESC\s*\]\s*=", init):
        fail("console_init never writes PRESC, so the prescaler is whatever "
             "the last thing to run left there")

    # 7. EVERY WAIT IS BOUNDED. An unbounded spin on a flag that never arrives
    # is a blank console and no clue, which is indistinguishable from a part
    # that never started.
    for name, b in (("console_put", put), ("console_flush", flush)):
        for m in re.finditer(r"while\s*\((.*?)\)\s*\{(.*?)\n    \}", b, re.S):
            if "spin_limit" not in m.group(2):
                fail(f"{name} has a while loop whose body does not test "
                     f"spin_limit, so it can wait for a flag that never "
                     f"arrives")
    if "spin_limit" not in put or "spin_limit" not in flush:
        fail("a wait exists that does not mention spin_limit at all")

    # 8. NO LIBRARY CALL AND NO ALLOCATION ANYWHERE. The image links with
    # -nostdlib and defines no `end` symbol, so a malloc would fail to link and
    # a memcpy would pull in a library this has no business needing.
    for bad in re.findall(r"\b(mem\w+|str\w+|printf|sprintf|malloc|calloc|"
                          r"free)\s*\(", code):
        fail(f"console.c calls {bad}, and this image links with -nostdlib and "
             f"no heap")

    # 9. EVERY ENTRY POINT RETURNS A BOOL, so a refusal is expressible. A void
    # putchar cannot report that the transmitter never became ready, and the
    # caller would print a whole banner into nothing.
    for fn in ("console_init", "console_put", "console_write", "console_hex32",
               "console_dec", "console_flush", "console_report"):
        if not re.search(r"\bbool\s+" + fn + r"\s*\(", header):
            fail(f"{fn} does not return bool in the header, so a refusal "
                 f"cannot be reported to its caller")

    # 10. Every verdict is named, and none shares a token with another: a
    # console line that cannot distinguish two causes is a console line that
    # sends a reader to the wrong place.
    names = re.findall(r"case\s+(CONSOLE_\w+)\s*:\s*return\s+\"([^\"]+)\"",
                       code)
    seen = {}
    for enumerator, token in names:
        if token in seen:
            fail(f"{enumerator} and {seen[token]} both report as {token!r}")
        seen[token] = enumerator
    declared = set(re.findall(r"\b(CONSOLE_[A-Z_]+)\b(?=\s*[,=])", header))
    for enumerator in sorted(declared):
        if enumerator not in {e for e, _t in names}:
            fail(f"{enumerator} is declared and has no name, so it would "
                 f"report as unknown")

    # 11. THE REPORT PRINTS WHAT MAKES THE REST MEAN ANYTHING. A console cannot
    # check its own baud: it writes into a transmitter and never hears back. The
    # terminal is the instrument, so the assumed clock and the computed divisor
    # have to be printed by the thing whose correctness depends on them.
    if report is None:
        fail("there is no console_report, so legible output would confirm "
             "nothing about the clock it assumed")
    else:
        for needle, what in (("kernel_hz", "the assumed kernel clock"),
                             ("usartdiv", "the divisor"),
                             ("brr", "the register word"),
                             ("actual_baud", "the rate that follows"),
                             ("error_permille", "the error")):
            if needle not in report:
                fail(f"console_report does not print {what}, so a reader "
                     f"cannot tell whether the assumption behind it was right")

    # 12. The rate is solved BEFORE any register is touched, so an impossible
    # rate leaves the part as it was rather than half configured.
    solve = init.find("baud_solve")
    first_write = min((i for i in (init.find("rcc["), init.find("gpio["),
                                   init.find("usart["))
                       if i >= 0), default=-1)
    if solve < 0:
        fail("console_init never solves the rate")
    elif first_write >= 0 and solve > first_write:
        fail("console_init writes a register before solving the rate, so an "
             "impossible rate leaves the part half configured")

    # 13. NO STRING LITERAL IN THIS CHAPTER'S C MAY BE BROKEN ACROSS A LINE.
    # A compiler would catch it, but the compiler is on another machine: the
    # authoring laptop has none by house rule, so without this the class is
    # only ever found in WSL or in CI, two pushes from where it was written.
    # It happened five times in one day, every time from a quoted heredoc
    # losing a backslash level and turning an escape into a real newline.
    for f in sorted((HERE / "src").rglob("*.[ch]"))             + sorted((HERE / "test").rglob("*.[ch]")):
        for n, line in enumerate(f.read_text(encoding="utf-8").splitlines(), 1):
            if re.sub(r"\.", "", line).count('"') % 2:
                fail(f"{f.name}:{n} has an odd number of quotes, so a string "
                     f"literal runs off the end of the line: "
                     f"{line.strip()[:50]!r}")

    if failures:
        print(f"test_console: {len(failures)} failure(s)")
        return 1
    print("ok  the shared port registers are read, modified and written and "
          "checked for disturbance, both clocks read back, the flags cleared "
          "before the byte, every wait bounded, no library call, and the "
          "report prints the clock it assumed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
