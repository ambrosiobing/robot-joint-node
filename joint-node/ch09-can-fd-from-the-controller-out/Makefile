# Chapter 9: everything that can be checked without hardware.
#
# Stage one is the arithmetic: bit timing, the message memory layout and
# the frame length code. Stage two is the controller's configuration, as
# a plan and an executor, both of which run against a register model here
# rather than against silicon, and the element layout, which is the shape a
# frame takes in the controller's own memory. Nothing in this Makefile needs a
# board, and nothing in it claims a wire.
#
#     make            the whole stage
#     make pytest     the half that runs on the authoring laptop
#     make ctest      the half that needs a C compiler
#
# On the win11 aquamarine authoring laptop there is no C compiler, and the house
# rule is that one is never run there even though Qt ships one. Run the python
# half directly:
#
#     python tools/gen_bt_vectors.py --check
#     python test/test_bittiming.py
#
# The C half runs on the Pi 4, or on the win11 skyhorizon WSL side.
#
# The layer rule is not checked from here. It is chapter 4's check, it reads the
# whole joint-node tree at once, and running it from this chapter would make
# this chapter depend on that one.

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
INCLUDE  = -Isrc/bus -Isrc/bsp -Itest
PYTHON  ?= python3
BUILD    = build

# The cross compiler, for `make cross` only. The same warning set as the
# host build plus the architecture, so that a difference between the two
# is the architecture and nothing else. Found on PATH in WSL on win11
# skyhorizon as the Debian arm-none-eabi-gcc; the CubeIDE toolchain on the
# PowerShell side needs its script dot-sourced first and is not this.
CROSS      ?= arm-none-eabi-gcc
CROSSARCH  ?= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
CROSSFLAGS ?= -std=c11 -Wall -Wextra -Werror -O1 -g -ffreestanding

.PHONY: all generated pytest ctest cross table clean
all: generated pytest ctest
	@echo
	@echo "chapter 9 passed: the arithmetic and the configuration, on a host, with no wire claimed"

# The vectors must match the reference. A hand edit fails here rather than
# surviving to be overwritten later by somebody who did not know it was there.
generated:
	@$(PYTHON) tools/gen_bt_vectors.py --check
	@$(PYTHON) tools/gen_msgram.py --check
	@$(PYTHON) tools/gen_msgram.py --audit-chapter
	@$(PYTHON) tools/gen_msgaddr.py --check
	@$(PYTHON) tools/gen_frame_vectors.py --check
	@$(PYTHON) tools/gen_initplan.py --check
	@$(PYTHON) tools/gen_planrun.py --check
	@$(PYTHON) tools/gen_element.py --check
	@$(PYTHON) tools/gen_fifo.py --check

pytest:
	@$(PYTHON) test/test_bittiming.py
	@$(PYTHON) test/test_msgram.py
	@$(PYTHON) test/test_msgaddr.py
	@$(PYTHON) test/test_frame.py
	@$(PYTHON) test/test_initplan.py
	@$(PYTHON) test/test_planrun.py
	@$(PYTHON) test/test_element.py
	@$(PYTHON) test/test_fifo.py

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_bittiming: $(BUILD) test/test_bittiming.c src/bus/bittiming.c \
                         src/bus/bittiming.h test/vectors.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_bittiming.c src/bus/bittiming.c

$(BUILD)/test_msgram: $(BUILD) test/test_msgram.c src/bus/msgram.c \
                      src/bus/msgram.h test/vectors_msgram.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_msgram.c src/bus/msgram.c

# From an element index to the place that element lives. It links msgram
# because it reads the map msgram computes, and the one refusal that matters
# is reachable only from a real layout: this node configures no dedicated
# receive buffers, so that section's offset is the first transmit event.
$(BUILD)/test_msgaddr: $(BUILD) test/test_msgaddr.c src/bus/msgaddr.c src/bus/msgaddr.h src/bus/msgram.c test/vectors_msgaddr.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_msgaddr.c src/bus/msgaddr.c src/bus/msgram.c

$(BUILD)/test_frame: $(BUILD) test/test_frame.c src/bus/frame.c \
                     src/bus/frame.h test/vectors_frame.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_frame.c src/bus/frame.c

# initplan is the first target that links more than one module: the plan needs
# the bit timing to produce NBTP and the layout to produce the offsets, which is
# the point of it.
$(BUILD)/test_initplan: $(BUILD) test/test_initplan.c src/bus/initplan.c                         src/bus/initplan.h src/bus/bittiming.c                         src/bus/msgram.c test/vectors_initplan.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_initplan.c 	    src/bus/initplan.c src/bus/bittiming.c src/bus/msgram.c

# The executor, against a register model written twice: once here in C and once
# in the generator. Its own failure paths cannot be exercised on a board, because
# a register that declines a write does not announce it, which is the reason the
# executor reads everything back in the first place.
$(BUILD)/test_planrun: $(BUILD) test/test_planrun.c src/bus/planrun.c                        src/bus/planrun.h src/bus/initplan.c                        src/bus/bittiming.c src/bus/msgram.c                        test/vectors_planrun.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_planrun.c 	    src/bus/planrun.c src/bus/initplan.c src/bus/bittiming.c 	    src/bus/msgram.c

# The element layout, against the same vectors the Python half uses. It links
# msgram too, for one assertion only: that the two files agree about how many
# bytes a frame occupies in the message memory. They are declared in different
# headers and nothing but this test makes them meet.
$(BUILD)/test_element: $(BUILD) test/test_element.c src/bus/element.c src/bus/element.h src/bus/frame.c src/bus/msgram.c test/vectors_element.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_element.c src/bus/element.c src/bus/frame.c src/bus/msgram.c

# The queue protocol. It links msgaddr and msgram as well, for one rule:
# every index the protocol hands out must be an index the message memory
# can address. A queue that wraps at the width of the status field instead
# of the size of the queue leaves both modules correct on their own terms,
# and only a rule that spans the two notices.
$(BUILD)/test_fifo: $(BUILD) test/test_fifo.c src/bus/fifo.c src/bus/fifo.h src/bus/msgaddr.c src/bus/msgram.c test/vectors_fifo.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_fifo.c src/bus/fifo.c src/bus/msgaddr.c src/bus/msgram.c

# The board accessors, pointed at arrays. This is the first target that
# compiles something from src/bsp/, which is the one directory allowed to
# name an address. It runs the real plan through the real executor through
# the real accessors, and its control arm is a block of zeroes: a wrong base
# address accepts every write and reads back nothing, so the plan has to stop
# at the one register whose value the part supplies rather than the software.
$(BUILD)/test_fdcan_mmio: $(BUILD) test/test_fdcan_mmio.c src/bsp/fdcan_mmio.c src/bsp/fdcan_mmio.h src/bus/planrun.c src/bus/initplan.c src/bus/bittiming.c src/bus/msgram.c
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_fdcan_mmio.c src/bsp/fdcan_mmio.c src/bus/planrun.c src/bus/initplan.c src/bus/bittiming.c src/bus/msgram.c

ctest: $(BUILD)/test_bittiming $(BUILD)/test_msgram $(BUILD)/test_msgaddr $(BUILD)/test_frame        $(BUILD)/test_initplan $(BUILD)/test_planrun $(BUILD)/test_element $(BUILD)/test_fifo $(BUILD)/test_fdcan_mmio
	@./$(BUILD)/test_bittiming
	@./$(BUILD)/test_msgram
	@./$(BUILD)/test_msgaddr
	@./$(BUILD)/test_frame
	@./$(BUILD)/test_initplan
	@./$(BUILD)/test_planrun
	@./$(BUILD)/test_element
	@./$(BUILD)/test_fifo
	@./$(BUILD)/test_fdcan_mmio

# Compile every shipping translation unit FOR THE PART, which no other
# target does. The host build proves the arithmetic; this proves the code is
# buildable for the processor it is meant for, and the two differ in ways
# that are silent on a host: a plain enum is unsigned on ARM EABI, so a test
# for a negative enumerator is dead code there and a -Werror failure with it,
# and both the plan and the addressing range check enumerators.
#
# Objects only. There is no startup, no linker script and no console in this
# volume yet, so there is no image to link: that floor is chapter 4's and it
# has no code tree at all. Compiling is what can honestly be claimed today.
cross:
	@rm -rf $(BUILD)/cross && mkdir -p $(BUILD)/cross
	@n=$$(ls -1 src/bus/*.c src/bsp/*.c 2>/dev/null | wc -l); test "$$n" -gt 0 || { echo "cross: the globs matched no sources, so nothing was compiled and this target would otherwise pass"; exit 1; }
	@for f in src/bus/*.c src/bsp/*.c; do $(CROSS) $(CROSSFLAGS) $(CROSSARCH) $(INCLUDE) -c $$f -o $(BUILD)/cross/$$(basename $$f .c).o || exit 1; done
	@want=$$(ls -1 src/bus/*.c src/bsp/*.c | wc -l); got=$$(ls -1 $(BUILD)/cross/*.o | wc -l); test "$$want" -eq "$$got" || { echo "cross: $$want sources and $$got objects, so one was skipped"; exit 1; }; echo "cross: $$got translation units compiled for cortex-m7 hard float, no image linked"

# The table in doc/bit-timing.md comes from here rather than from a person
# typing it, so it cannot drift away from the arithmetic it describes.
table:
	@$(PYTHON) tools/gen_bt_vectors.py --table
	@$(PYTHON) tools/gen_initplan.py --table
	@$(PYTHON) tools/gen_planrun.py --table
	@$(PYTHON) tools/gen_element.py --table
	@$(PYTHON) tools/gen_msgaddr.py --table
	@$(PYTHON) tools/gen_fifo.py --table

clean:
	@rm -rf $(BUILD)
