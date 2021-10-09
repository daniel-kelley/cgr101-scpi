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

test: prog test-prog # test-net

test-prog:
	$(RUBY) -I test test/test_prog.rb $(TEST_ARG)

test-net:
	$(RUBY) -I test test/test_net.rb $(TEST_ARG)

clean:
	$(MAKE) -C src $@

help:
	@echo Targets: help test prog
