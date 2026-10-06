# Chapter 17. There is nothing to compile, because nothing was built.
#
#     make        check the decision document
#
# The chapter's deliverable is doc/fieldbus-decision.md and the only thing a
# program can check about it is its shape: that the four headings are present
# and that every claim says where it came from. Whether the claims are still
# true needs a person and a browser, and the document says which of them are
# most likely to have moved.

PYTHON ?= python3

.PHONY: all check clean
all: check

check:
	@$(PYTHON) tools/check_decision.py

clean:
	@:
