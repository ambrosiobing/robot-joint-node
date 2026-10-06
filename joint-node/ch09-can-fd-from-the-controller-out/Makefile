# Chapter 9, stage one: the arithmetic, with no hardware and no board.
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
	@echo "chapter 9 stage one passed: the arithmetic only, and no wire was claimed"

# The vectors must match the reference. A hand edit fails here rather than
# surviving to be overwritten later by somebody who did not know it was there.
generated:
	@$(PYTHON) tools/gen_bt_vectors.py --check

pytest:
	@$(PYTHON) test/test_bittiming.py

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_bittiming: $(BUILD) test/test_bittiming.c src/bus/bittiming.c \
                         src/bus/bittiming.h test/vectors.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_bittiming.c src/bus/bittiming.c

ctest: $(BUILD)/test_bittiming
	@./$(BUILD)/test_bittiming

# The table in doc/bit-timing.md comes from here rather than from a person
# typing it, so it cannot drift away from the arithmetic it describes.
table:
	@$(PYTHON) tools/gen_bt_vectors.py --table

clean:
	@rm -rf $(BUILD)
