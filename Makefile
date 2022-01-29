#
#  Makefile
#
#  Copyright (c) 2021 by Daniel Kelley
#

.PHONY: all prog test test-prog help

RUBY ?= ruby

all: prog

prog: src/cgr101-scpi

src/cgr101-scpi: src/*.[ch]
	$(MAKE) -C src

check: prog test-prog # test-net

test-prog:
	$(RUBY) -I test test/test_prog.rb $(TEST_ARG)

test-net:
	$(RUBY) -I test test/test_net.rb $(TEST_ARG)

vcheck: prog
	CGR101_PROG="valgrind -q src/cgr101-scpi" \
	$(RUBY) -I test test/test_prog.rb $(TEST_ARG)

scheck:
	$(MAKE) -C src check

clean:
	$(MAKE) -C src $@

help:
	@echo Targets: help test prog
