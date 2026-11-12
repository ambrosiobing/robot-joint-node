# Chapter 4: the board support package, and the image floor under it.
#
#     make            everything that needs no toolchain
#     make layers     chapter 4's rule over the whole joint-node tree
#     make pytest     the map, the startup, the divisor and the console
#     make ctest      needs a host C compiler: the divisor against its vectors
#     make linkcheck  needs arm-none-eabi-gcc: links a real image and reads
#                     its first two words back
#     make firstlight needs arm-none-eabi-gcc: THE IMAGE THAT PRINTS, as an
#                     .elf and the .bin that gets copied to the board
#     make table      the tables in doc/memory-map.md, doc/console.md and
#                     doc/interrupts.md
#
# On the win11 aquamarine authoring laptop there is no C compiler and the house
# rule keeps it that way, so `make` stops short of linkcheck. Everything else
# runs there. The cross toolchain is on PATH in WSL on win11 skyhorizon.

PYTHON  ?= python3
BUILD    = build

CROSS      ?= arm-none-eabi-gcc
OBJCOPY    ?= arm-none-eabi-objcopy
CROSSARCH  ?= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
CROSSFLAGS ?= -std=c11 -Wall -Wextra -Werror -O1 -g -ffreestanding -fno-common

.PHONY: all generated layers pytest ctest linkcheck firstlight stamp table clean

all: generated layers pytest
	@echo
	@echo "chapter 4 passed: the map, the script, the startup, the vector table and the console arithmetic, checked without a toolchain"

# The MEMORY block and the header come from the map. A hand edit fails here
# rather than surviving to be overwritten by somebody who did not know it was
# generated.
generated:
	@$(PYTHON) tools/gen_memmap.py --check
	@$(PYTHON) tools/gen_baud.py --check
	@$(PYTHON) tools/gen_vectors.py --check

# Chapter 4's own rule, over every chapter's C. It lives at the chapter root
# rather than under tools/, which is where the workflow calls it from.
layers:
	@$(PYTHON) check_layers.py

pytest:
	@$(PYTHON) test/test_memmap.py
	@$(PYTHON) test/test_startup.py
	@$(PYTHON) test/test_vectors.py
	@$(PYTHON) test/test_firstlight.py
	@$(PYTHON) test/test_baud.py
	@$(PYTHON) test/test_console.py

$(BUILD):
	@mkdir -p $(BUILD)

# baud.c is pure arithmetic with no register in it, so it runs on a host and is
# compared against the same vectors the generator wrote. console.c cannot be:
# it is nothing but registers, and test_console.py checks it by construct.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g

$(BUILD)/test_baud: $(BUILD) test/test_baud.c src/bsp/baud.c src/bsp/baud.h test/vectors_baud.h
	@$(CC) $(CFLAGS) -Isrc/bsp -Itest -o $@ test/test_baud.c src/bsp/baud.c

ctest: $(BUILD)/test_baud
	@./$(BUILD)/test_baud

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
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -ffunction-sections -fdata-sections -c src/bsp/vectors.c -o $(BUILD)/vectors.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -ffunction-sections -fdata-sections -c src/bsp/baud.c -o $(BUILD)/baud.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -ffunction-sections -fdata-sections -c src/bsp/console.c -o $(BUILD)/console.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -ffunction-sections -fdata-sections -c test/link_probe.c -o $(BUILD)/link_probe.o
	@$(CROSS) $(CROSSARCH) -nostdlib -Wl,--gc-sections -Wl,-L,src/bsp -Wl,-Map=$(BUILD)/image.map -T src/bsp/stm32h7a3.ld $(BUILD)/startup.o $(BUILD)/vectors.o $(BUILD)/baud.o $(BUILD)/console.o $(BUILD)/link_probe.o -o $(BUILD)/image.elf
	@arm-none-eabi-size $(BUILD)/image.elf
	@$(PYTHON) tools/check_image.py $(BUILD)/image.elf

# THE IMAGE THAT PRINTS. Everything under it has been checked without a part
# for weeks; this is the first thing in the volume whose correctness can only be
# seen on the board, because readable text at 115200 is the first evidence that
# the assumed clock, the computed divisor, the pin configuration, the startup's
# copy and zero, and the vector table's placement are all right at once.
#
# `stamp` is a prerequisite and is PHONY, so the stamp is rewritten on every
# build and that one translation unit always recompiles. That is deliberate and
# it is why this target never builds as a no-op: a stamp that does not move
# cannot tell a new image from the one the previous session flashed, and the
# board is shared.
#
# link_probe.c is NOT linked here. It carries its own `main` for linkcheck, and
# two definitions of main is a link error rather than a silent choice.
stamp:
	@$(PYTHON) tools/gen_stamp.py $(BUILD)/stamp.c

firstlight: $(BUILD) generated pytest stamp
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/startup.c -o $(BUILD)/fl_startup.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/vectors.c -o $(BUILD)/fl_vectors.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/baud.c -o $(BUILD)/fl_baud.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/console.c -o $(BUILD)/fl_console.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/app/first_light.c -o $(BUILD)/fl_first_light.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c $(BUILD)/stamp.c -o $(BUILD)/fl_stamp.o
	@$(CROSS) $(CROSSARCH) -nostdlib -Wl,--gc-sections -Wl,-L,src/bsp -Wl,-Map=$(BUILD)/first_light.map -T src/bsp/stm32h7a3.ld $(BUILD)/fl_startup.o $(BUILD)/fl_vectors.o $(BUILD)/fl_baud.o $(BUILD)/fl_console.o $(BUILD)/fl_first_light.o $(BUILD)/fl_stamp.o -o $(BUILD)/first_light.elf
	@$(OBJCOPY) -O binary $(BUILD)/first_light.elf $(BUILD)/first_light.bin
	@arm-none-eabi-size $(BUILD)/first_light.elf
	@$(PYTHON) tools/check_image.py $(BUILD)/first_light.elf
	@$(PYTHON) tools/gen_stamp.py --check $(BUILD)/stamp.c
	@echo
	@echo "first_light.bin is ready to copy to the board. The stamp above is what"
	@echo "the banner will print, so read it back off the console before reading"
	@echo "any number: the part is shared and a skipped flash prints the old image."

table:
	@$(PYTHON) tools/gen_memmap.py --table
	@$(PYTHON) tools/gen_baud.py --table
	@$(PYTHON) tools/gen_vectors.py --table

clean:
	@rm -rf $(BUILD)
