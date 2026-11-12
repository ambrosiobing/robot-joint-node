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
#     make fdcanprobe-a   the register map probe, writing ONLY offset 0x0EC
#     make fdcanprobe-b   the same probe, writing ONLY offset 0x14C
#     make addrmap        reads first, writes once, names every address before
#                     touching it
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

.PHONY: all generated layers pytest ctest linkcheck firstlight fdcanprobe-a fdcanprobe-b addrmap stamp table clean

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
	@$(PYTHON) test/test_fdcanprobe.py
	@$(PYTHON) test/test_addrmap.py
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

# TWO IMAGES, ONE SOURCE FILE, ONE -D EACH.
#
# The read-only baseline has to be identical in both, and the way to guarantee
# that is to compile the same translation unit twice rather than to maintain two
# files that could drift. The only difference is which single RCC offset is
# written, and a build with neither -D is a compile error in the source.
#
# Joseph's decision, Thursday 12 November 2026: the competing candidate is not
# written, because its meaning for this silicon is exactly what the experiment
# is to determine, and writing to it would move the subject.
FP_OBJS = $(BUILD)/fp_startup.o $(BUILD)/fp_vectors.o $(BUILD)/fp_baud.o \
          $(BUILD)/fp_console.o $(BUILD)/fp_probe.o $(BUILD)/fp_stamp.o

fdcanprobe-a: FP_DEF  = -DFP_VARIANT_A
fdcanprobe-a: FP_NAME = fdcan_probe_a
fdcanprobe-b: FP_DEF  = -DFP_VARIANT_B
fdcanprobe-b: FP_NAME = fdcan_probe_b

fdcanprobe-a fdcanprobe-b: $(BUILD) generated pytest stamp
	@echo "checking that a build with NEITHER variant refuses, which only a compiler can say:"
	@if $(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -fsyntax-only src/app/fdcan_probe.c >/dev/null 2>&1; then \
		echo "  FAIL fdcan_probe.c compiles with neither FP_VARIANT_A nor FP_VARIANT_B,"; \
		echo "       so an image that cannot name the candidate it wrote can be built,"; \
		echo "       and the board is shared so such a capture is unattributable."; \
		exit 1; \
	else \
		echo "  ok   it refuses, so every image names its one write"; \
	fi
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/startup.c -o $(BUILD)/fp_startup.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/vectors.c -o $(BUILD)/fp_vectors.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/baud.c -o $(BUILD)/fp_baud.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/console.c -o $(BUILD)/fp_console.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app $(FP_DEF) -ffunction-sections -fdata-sections -c src/app/fdcan_probe.c -o $(BUILD)/fp_probe.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c $(BUILD)/stamp.c -o $(BUILD)/fp_stamp.o
	@$(CROSS) $(CROSSARCH) -nostdlib -Wl,--gc-sections -Wl,-L,src/bsp -Wl,-Map=$(BUILD)/$(FP_NAME).map -T src/bsp/stm32h7a3.ld $(FP_OBJS) -o $(BUILD)/$(FP_NAME).elf
	@$(OBJCOPY) -O binary $(BUILD)/$(FP_NAME).elf $(BUILD)/$(FP_NAME).bin
	@arm-none-eabi-size $(BUILD)/$(FP_NAME).elf
	@$(PYTHON) tools/check_image.py $(BUILD)/$(FP_NAME).elf
	@echo
	@echo "$(FP_NAME).bin is ready. Read the stamp off the console before any"
	@echo "number, and read the tag: an a image and a b image differ in one"
	@echo "write and nothing else, so the tag is how a capture is attributed."

# READS FIRST, WRITES ONCE. It closes a gap in fdcanprobe, which read FDCAN
# only AFTER the enable and so could not say whether the enable was needed, and
# it tests whether the 0x60 mirroring that capture found is periodic or confined
# to three pairs. Every read is announced and flushed before the access, so a
# truncated capture names the read that stopped the part.
AM_OBJS = $(BUILD)/am_startup.o $(BUILD)/am_vectors.o $(BUILD)/am_baud.o \
          $(BUILD)/am_console.o $(BUILD)/am_addrmap.o $(BUILD)/am_stamp.o

addrmap: $(BUILD) generated pytest stamp
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/startup.c -o $(BUILD)/am_startup.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/vectors.c -o $(BUILD)/am_vectors.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/baud.c -o $(BUILD)/am_baud.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/bsp/console.c -o $(BUILD)/am_console.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c src/app/addrmap.c -o $(BUILD)/am_addrmap.o
	@$(CROSS) $(CROSSFLAGS) $(CROSSARCH) -Isrc/bsp -Isrc/app -ffunction-sections -fdata-sections -c $(BUILD)/stamp.c -o $(BUILD)/am_stamp.o
	@$(CROSS) $(CROSSARCH) -nostdlib -Wl,--gc-sections -Wl,-L,src/bsp -Wl,-Map=$(BUILD)/addrmap.map -T src/bsp/stm32h7a3.ld $(AM_OBJS) -o $(BUILD)/addrmap.elf
	@$(OBJCOPY) -O binary $(BUILD)/addrmap.elf $(BUILD)/addrmap.bin
	@arm-none-eabi-size $(BUILD)/addrmap.elf
	@$(PYTHON) tools/check_image.py $(BUILD)/addrmap.elf
	@echo
	@echo "addrmap.bin is ready. If the capture stops partway, the LAST offset"
	@echo "printed is the read that stopped it, and the fault handler should say"
	@echo "so itself. A capture that ends mid-list is a result, not a mishap."

table:
	@$(PYTHON) tools/gen_memmap.py --table
	@$(PYTHON) tools/gen_baud.py --table
	@$(PYTHON) tools/gen_vectors.py --table

clean:
	@rm -rf $(BUILD)
