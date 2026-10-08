# Chapter 9: everything that can be checked without hardware.
#
# Stage one is the arithmetic: bit timing, the message memory layout and
# the frame length code. Stage two is the controller's configuration, as
# a plan and an executor, both of which run against a register model here
# rather than against silicon. Nothing in this Makefile needs a board,
# and nothing in it claims a wire.
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
INCLUDE  = -Isrc/bus -Itest
PYTHON  ?= python3
BUILD    = build

.PHONY: all generated pytest ctest table clean
all: generated pytest ctest
	@echo
	@echo "chapter 9 passed: the arithmetic and the configuration, on a host, with no wire claimed"

# The vectors must match the reference. A hand edit fails here rather than
# surviving to be overwritten later by somebody who did not know it was there.
generated:
	@$(PYTHON) tools/gen_bt_vectors.py --check
	@$(PYTHON) tools/gen_msgram.py --check
	@$(PYTHON) tools/gen_msgram.py --audit-chapter
	@$(PYTHON) tools/gen_frame_vectors.py --check
	@$(PYTHON) tools/gen_initplan.py --check
	@$(PYTHON) tools/gen_planrun.py --check

pytest:
	@$(PYTHON) test/test_bittiming.py
	@$(PYTHON) test/test_msgram.py
	@$(PYTHON) test/test_frame.py
	@$(PYTHON) test/test_initplan.py
	@$(PYTHON) test/test_planrun.py

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_bittiming: $(BUILD) test/test_bittiming.c src/bus/bittiming.c \
                         src/bus/bittiming.h test/vectors.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_bittiming.c src/bus/bittiming.c

$(BUILD)/test_msgram: $(BUILD) test/test_msgram.c src/bus/msgram.c \
                      src/bus/msgram.h test/vectors_msgram.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_msgram.c src/bus/msgram.c

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

ctest: $(BUILD)/test_bittiming $(BUILD)/test_msgram $(BUILD)/test_frame        $(BUILD)/test_initplan $(BUILD)/test_planrun
	@./$(BUILD)/test_bittiming
	@./$(BUILD)/test_msgram
	@./$(BUILD)/test_frame
	@./$(BUILD)/test_initplan
	@./$(BUILD)/test_planrun

# The table in doc/bit-timing.md comes from here rather than from a person
# typing it, so it cannot drift away from the arithmetic it describes.
table:
	@$(PYTHON) tools/gen_bt_vectors.py --table
	@$(PYTHON) tools/gen_initplan.py --table
	@$(PYTHON) tools/gen_planrun.py --table

clean:
	@rm -rf $(BUILD)
