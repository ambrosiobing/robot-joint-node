#!/usr/bin/env python3
"""Chapter 4's rule: nothing above the board layer includes a vendor header.

    python tools/check_layers.py

The moment a control loop includes a peripheral header, the control loop is
tied to one silicon vendor and cannot be built on a host machine. Chapter 20
has to build most of this node on a host with no hardware at all, and that is
only possible if the rule was kept from chapter 4 onward rather than
retrofitted at the end. Retrofitting it means untangling every include in the
tree at the point where the deadline is closest, which is why this check exists
before there is much to check.

The rule is not that vendor headers are bad. They are how the silicon is
reached, and `src/bsp/` exists to reach it. The rule is that they stop there.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Where the silicon is allowed to be named, as a path segment pair. Every
# chapter keeps its own tree, so this matches chNN/src/bsp/ wherever it appears
# rather than one fixed path from the root.
BOARD_LAYER = ("src/bsp",)

# Vendor and toolchain headers. A prefix match is enough: the point is to catch
# the family, not to keep a list of every header in it up to date.
VENDOR_PREFIXES = (
    "stm32", "cmsis", "core_cm", "system_stm32",
    "hal_", "stm32h7xx", "ll_",
    "nrfx", "nrf_", "esp_", "driver/",
)

INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)


def is_board_layer(path):
    rel = path.relative_to(ROOT).as_posix()
    return any(("/" + d + "/") in ("/" + rel) for d in BOARD_LAYER)


def vendor_header(name):
    low = name.lower()
    return any(low.startswith(p) or ("/" + p) in low for p in VENDOR_PREFIXES)


def main():
    sources = sorted(p for p in ROOT.rglob("*.[ch]") if "build" not in p.parts)
    if not sources:
        print("no C sources yet, so nothing to check. The rule still applies.")
        return 0

    violations = []
    for path in sources:
        if is_board_layer(path):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for name in INCLUDE.findall(text):
            if vendor_header(name):
                rel = path.relative_to(ROOT).as_posix()
                violations.append(f"{rel} includes <{name}>")

    if violations:
        print(f"FAIL  {len(violations)} includes break chapter 4's rule:")
        for v in violations:
            print("  " + v)
        print("\n  Vendor headers belong under " + ", ".join(BOARD_LAYER) + ".")
        print("  Everything above that layer has to build on a host, which is what")
        print("  lets chapter 20 run its first stage with no hardware at all.")
        return 1

    above = sum(1 for p in sources if not is_board_layer(p))
    print(f"ok  {len(sources)} C files, {above} of them above the board layer, "
          f"no vendor header among them")
    return 0


if __name__ == "__main__":
    sys.exit(main())
