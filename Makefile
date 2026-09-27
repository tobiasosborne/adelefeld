# Makefile of adelefeld, work package 0.1 (provisional build scaffold).
#
#   make all      build build/libadelefeld.a from src/*.c (empty src is allowed)
#   make check    build and run every tests/test_*.c; exit non-zero if any test fails
#   make clean    remove build/
#   make fuzz     placeholder
#   make mutate   placeholder
#   make bench    placeholder
#
# A new file src/<x>.c or tests/test_<x>.c is picked up without editing this file: the lists
# below are computed by $(wildcard ...) when make reads the file. Every test is its own
# executable. `make -j` works. Everything built lands under build/.
#
# SAN=1 adds the address and undefined-behaviour sanitizers:
#   make clean && make check SAN=1

CC       ?= cc
AR       ?= ar
# GNU make predefines ARFLAGS as rv, and a build must have a symbol index, so rcs unless set.
ifeq ($(origin ARFLAGS),default)
ARFLAGS := rcs
endif
CFLAGS   ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -Iinclude
LDFLAGS  ?=
# FLINT 3.0.1, headers in /usr/include/flint, so <flint/...> needs no extra -I.
# fmpz needs GMP; -lm is for the later ball code. MPFR is not linked: nothing here needs it.
LDLIBS   ?= -lflint -lgmp -lm

# Dependency files, so that a change in a header rebuilds what includes it.
DEPFLAGS = -MMD -MP

SAN ?= 0
ifeq ($(SAN),1)
CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif

BUILD = build
LIB   = $(BUILD)/libadelefeld.a

SRC       := $(sort $(wildcard src/*.c))
OBJ       := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
TEST_SRC  := $(sort $(wildcard tests/test_*.c))
TEST_BIN  := $(patsubst tests/%.c,$(BUILD)/%,$(TEST_SRC))
DEPS      := $(OBJ:.o=.d) $(patsubst tests/%.c,$(BUILD)/%.d,$(TEST_SRC))

.PHONY: all check clean fuzz mutate bench help
.DELETE_ON_ERROR:

all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(LIB): $(OBJ) | $(BUILD)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Each test is one executable: its own translation unit, the library, FLINT.
$(BUILD)/%: tests/%.c $(LIB) | $(BUILD)
	$(CC) $(CPPFLAGS) -Itests $(CFLAGS) $(DEPFLAGS) $(LDFLAGS) $< $(LIB) $(LDLIBS) -o $@

check: $(TEST_BIN)
	@fail=0; \
	for t in $(TEST_BIN); do \
	    echo "== $$t"; \
	    ./$$t || fail=1; \
	done; \
	if [ -n "$(TEST_BIN)" ] && [ $$fail -eq 0 ]; then echo "check passed: all $(words $(TEST_BIN)) test programs"; \
	elif [ $$fail -ne 0 ]; then echo "check FAILED"; exit 1; \
	else echo "check: no tests/test_*.c found"; fi

clean:
	rm -rf $(BUILD)

# Placeholders. Each says what it will do; none exists yet.

fuzz:
	@echo "fuzz: not built. It will run coverage-guided fuzzing on the parsers of work"
	@echo "package 1.4 and on anything that reads untrusted input, under docs/PLAN.md 7."

mutate:
	@echo "mutate: not built. It will mutate the arithmetic and precision rules of"
	@echo "src/ and report surviving mutants, under docs/PLAN.md 7."

bench:
	@echo "bench: not built. It will run the benchmark harness of work package 0.5 under"
	@echo "the contract of docs/PERF.md section 7, one dated output file per run."

help:
	@echo "targets: all check clean fuzz mutate bench; variable SAN=1 adds the sanitizers"
