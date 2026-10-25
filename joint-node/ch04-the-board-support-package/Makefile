# Chapter 4: the board support package, and the image floor under it.
#
#     make            everything that needs no toolchain
#     make layers     chapter 4's rule over the whole joint-node tree
#     make pytest     the memory map and the startup, checked by construct
#     make linkcheck  needs arm-none-eabi-gcc: links a real image and reads
#                     its first two words back
#     make table      the tables in doc/memory-map.md and doc/console.md
#
# On the win11 aquamarine authoring laptop there is no C compiler and the house
# rule keeps it that way, so `make` stops short of linkcheck. Everything else
# runs there. The cross toolchain is on PATH in WSL on win11 skyhorizon.

PYTHON  ?= python3
BUILD    = build

CROSS      ?= arm-none-eabi-gcc
CROSSARCH  ?= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
CROSSFLAGS ?= -std=c11 -Wall -Wextra -Werror -O1 -g -ffreestanding -fno-common

.PHONY: all generated layers pytest linkcheck table clean

all: generated layers pytest
	@echo
	@echo "chapter 4 passed: the map, the script, the startup and the console arithmetic, checked without a toolchain"

# The MEMORY block and the header come from the map. A hand edit fails here
# rather than surviving to be overwritten by somebody who did not know it was
# generated.
generated:
	@$(PYTHON) tools/gen_memmap.py --check
	@$(PYTHON) tools/gen_baud.py --check

# Chapter 4's own rule, over every chapter's C. It lives at the chapter root
# rather than under tools/, which is where the workflow calls it from.
layers:
	@$(PYTHON) check_layers.py

pytest:
	@$(PYTHON) test/test_memmap.py
	@$(PYTHON) test/test_startup.py
	@$(PYTHON) test/test_baud.py

$(BUILD):
	@mkdir -p $(BUILD)

# A REAL IMAGE, because a linker script nothing links against is unverified.
#
# -Lsrc/bsp is not optional: the script says INCLUDE memory.ld, and the linker
# looks for that in the search path rather than beside the script, so without it
# the link fails with a missing file and the cause is three steps removed from
# the message.
#
# --gc-sections is on deliberately. It is what discards a vector table that
# nothing references, and the script's KEEP is what stops it. Linking without
# it would leave that pair untested.
linkcheck: $(BUILD) generated pytest
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -ffunction-sections -fdata-sections -c src/bsp/startup.c -o $(BUILD)/startup.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -ffunction-sections -fdata-sections -c test/link_probe.c -o $(BUILD)/link_probe.o
	@$(CROSS) $(CROSSARCH) -nostdlib -Wl,--gc-sections -Wl,-L,src/bsp -Wl,-Map=$(BUILD)/image.map -T src/bsp/stm32h7a3.ld $(BUILD)/startup.o $(BUILD)/link_probe.o -o $(BUILD)/image.elf
	@arm-none-eabi-size $(BUILD)/image.elf
	@$(PYTHON) tools/check_image.py $(BUILD)/image.elf

table:
	@$(PYTHON) tools/gen_memmap.py --table
	@$(PYTHON) tools/gen_baud.py --table

clean:
	@rm -rf $(BUILD)
