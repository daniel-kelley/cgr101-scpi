#
#  Makefile
#
#  Copyright (c) 2020 by Daniel Kelley
#
# Dependencies:
#   flex/bison
#   libusb

NAME := cgr101-scpi

DEBUG := -g
# address thread undefined etc.
ifneq ($(SANITIZE),)
DEBUG += -fsanitize=$(SANITIZE)
endif


INC :=
CPPFLAGS := $(INC) -MMD

BEAR ?= bear
CODECHECKER ?= CodeChecker
CODECHECKER_OUT = ./reports
CODECHECKER_ARGS := --ctu
CODECHECKER_ARGS += --enable alpha.deadcode.UnreachableCode
CODECHECKER_ARGS += --enable bugprone-argument-comment
CODECHECKER_ARGS += --enable bugprone-bad-signal-to-kill-thread
CODECHECKER_ARGS += --enable bugprone-branch-clone
CODECHECKER_ARGS += --enable bugprone-exception-escape
CODECHECKER_ARGS += --enable bugprone-macro-parentheses
CODECHECKER_ARGS += --enable bugprone-multiple-statement-macro
CODECHECKER_ARGS += --enable bugprone-narrowing-conversions
CODECHECKER_ARGS += --enable bugprone-no-escape
CODECHECKER_ARGS += --enable bugprone-parent-virtual-call
CODECHECKER_ARGS += --enable bugprone-posix-return
CODECHECKER_ARGS += --enable bugprone-reserved-identifier
CODECHECKER_ARGS += --enable bugprone-spuriously-wake-up-functions
CODECHECKER_ARGS += --enable bugprone-string-integer-assignment
CODECHECKER_ARGS += --enable bugprone-suspicious-include
CODECHECKER_ARGS += --enable bugprone-suspicious-string-compare
CODECHECKER_ARGS += --enable bugprone-unhandled-self-assignment
CODECHECKER_ARGS += --output $(CODECHECKER_OUT)

WARN := -Wall
WARN += -Wextra
WARN += -Werror
WARN += -Wconversion
WARN += -Wdeclaration-after-statement
WARN += -Wmissing-prototypes
WARN += -Wshadow
WARN += -Wstrict-prototypes
WARN += -Wundef
WARN += -Wunused

CFLAGS := $(WARN) $(DEBUG)

LDFLAGS := $(DEBUG)
LDLIBS := -lpthread

SRC := main.c
SRC += server.c
SRC += cgr101.c
SRC += scpi.tab.c
SRC += scpi_core.c
SRC += scpi_error.c
SRC += scpi_output.c
SRC += scanner.c
SRC += parser.c

OBJ := $(SRC:%.c=%.o)
DEP := $(SRC:%.c=%.d)

.PHONY: all check clean

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

scpi.tab.c: scpi.y
	bison -d $<

scanner.c: scanner.l
	flex $<

check: compile_commands.json
	-$(CODECHECKER) analyze $< $(CODECHECKER_ARGS)
	$(CODECHECKER) parse $< $(CODECHECKER_OUT)

compile_commands.json:
	$(BEAR) $(MAKE)

clean:
	-rm -rf $(OBJ) $(DEP) $(NAME) \
		scanner.c scanner.h \
		scpi.tab.c scpi.tab.h \
		compile_commands.json $(CODECHECKER_OUT)

-include $(DEP)
