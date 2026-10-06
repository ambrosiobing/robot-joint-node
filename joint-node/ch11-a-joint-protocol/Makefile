# Stage one: everything provable with no hardware.
#
# Chapter 20 builds this stage first, because it needs no board and it is the
# one that will run thousands of times. Nothing here opens a serial port, flashes
# anything or waits on a bus.
#
#     make            the whole stage, the bus load audit included
#     make audit      that audit on its own
#
# On the win11 aquamarine authoring laptop there is no make and no C compiler,
# so run the python half directly:
#
#     python tools/gen_msgs.py proto/messages.yaml --check
#     python test/test_msgs.py
#     python test/test_gen.py
#
# The layer rule is not checked from here. It is chapter 4's check and it reads
# the whole joint-node tree at once, so running it from this chapter would make
# this chapter depend on that one, which is the arrangement the volume's README
# rules out. It runs from chapter 4 and covers these files from there.

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
INCLUDE  = -Isrc/bus -Itest
PYTHON  ?= python3
BUILD    = build

.PHONY: all generated pytest ctest audit clean
all: generated audit pytest ctest
	@echo
	@echo "stage one passed: no hardware was involved, and none was claimed"

# The generated files must match the description. A hand edit fails here rather
# than surviving to be overwritten later by somebody who did not know.
generated:
	@$(PYTHON) tools/gen_msgs.py proto/messages.yaml --check

pytest:
	@$(PYTHON) test/test_msgs.py
	@$(PYTHON) test/test_gen.py

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_msgs: $(BUILD) test/test_msgs.c src/bus/joint_msgs.c src/bus/joint_msgs.h test/vectors.h
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_msgs.c src/bus/joint_msgs.c

$(BUILD)/test_expiry: $(BUILD) test/test_expiry.c src/bus/command.c src/bus/joint_msgs.c
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_expiry.c src/bus/command.c src/bus/joint_msgs.c

ctest: $(BUILD)/test_msgs $(BUILD)/test_expiry
	@./$(BUILD)/test_msgs
	@./$(BUILD)/test_expiry

# Part of the stage since Tuesday 6 October 2026. It compares this tool's
# arithmetic with the figures printed in chapter 11, which the chapter presents
# as this tool's own output. Three of them disagreed and the chapter was wrong;
# now that they match, the check is what keeps them matching, so it is gating
# rather than excused.
audit:
	@$(PYTHON) tools/busload.py --audit-chapter

clean:
	@rm -rf $(BUILD)
