# Chapter 10, the motion master. All C, and all of it runs before any board.
#
#     make           build everything and run the tests
#     make test      the tests alone
#     make tools     jn-listen and jn-setpoint alone
#     make clean
#
# Two of the three tests run anywhere there is a compiler. The third opens a
# socket, so it runs on Linux and reports a skip with its reason everywhere
# else, including on a Linux host where the virtual interface is not up:
#
#     sudo modprobe vcan
#     sudo ip link add dev vcan0 type vcan
#     sudo ip link set up vcan0
#
# There is no compiler on the win11 aquamarine authoring laptop, so nothing in
# this directory has ever been built there. It is built by the Pi 4 and by the
# workflow in .github/workflows/checks.yml, and the workflow is where the first
# information about a change to it comes from.

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
INCLUDE  = -Iinclude
BUILD    = build

HDRS = include/jn_frame.h include/jn_log.h include/jn_bus.h
OBJS = $(BUILD)/jn_frame.o $(BUILD)/jn_log.o $(BUILD)/jn_bus.o

TESTS = $(BUILD)/test_frame $(BUILD)/test_log $(BUILD)/test_bus
TOOLS = $(BUILD)/jn-listen $(BUILD)/jn-setpoint

.PHONY: all test tools clean
all: test tools
	@echo
	@echo "chapter 10 built: the frame layout and the log format are proved,"
	@echo "and the socket is proved wherever the interface was up"

# An order-only prerequisite, after the bar. Without it the directory's own
# timestamp changes every time a file is written into it, and everything that
# depends on the directory is rebuilt on the next run for no reason.
$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c $(HDRS) | $(BUILD)
	@$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

# Each test links only what it needs, which is itself a check: if test_frame
# ever needs the socket object to link, something has reached across the line
# this chapter is built around.
$(BUILD)/test_frame: test/test_frame.c $(BUILD)/jn_frame.o $(HDRS) | $(BUILD)
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_frame.c $(BUILD)/jn_frame.o

$(BUILD)/test_log: test/test_log.c $(BUILD)/jn_frame.o $(BUILD)/jn_log.o $(HDRS) | $(BUILD)
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_log.c \
		$(BUILD)/jn_frame.o $(BUILD)/jn_log.o

$(BUILD)/test_bus: test/test_bus.c $(OBJS) $(HDRS) | $(BUILD)
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ test/test_bus.c $(OBJS)

$(BUILD)/jn-listen: app/jn_listen.c $(OBJS) $(HDRS) | $(BUILD)
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ app/jn_listen.c $(OBJS)

$(BUILD)/jn-setpoint: app/jn_setpoint.c $(OBJS) $(HDRS) | $(BUILD)
	@$(CC) $(CFLAGS) $(INCLUDE) -o $@ app/jn_setpoint.c $(OBJS)

test: $(TESTS)
	@./$(BUILD)/test_frame
	@./$(BUILD)/test_log $(BUILD)/jn_test.log
	@./$(BUILD)/test_bus

tools: $(TOOLS)

clean:
	@rm -rf $(BUILD)
