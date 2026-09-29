# Makefile of adelefeld.
#
#   make all            build build/libadelefeld.a from src/*.c (empty src is allowed)
#   make check          build and run every tests/test_*.c; exit non-zero if any test fails
#   make check-all      make check, then the driver, exports and julia scripts and the self-tests
#                       of tools/mutate and tools/memcheck, one after the other
#   make clean          remove build/
#   make fuzz           run the coverage-guided fuzzers of tests/fuzz/ under libFuzzer
#   make mutate         run the mutation testing of tools/mutate/mutate.py over src/
#   make bench          run the benchmark harness under the contract of docs/PERF.md 7
#
# A new file src/<x>.c or tests/test_<x>.c is picked up without editing this file: the lists
# below are computed by $(wildcard ...) when make reads the file. Every test is its own
# executable. `make -j` works. Everything built lands under build/.
#
# SAN=1 adds the address and undefined-behaviour sanitizers:
#   make clean && make check SAN=1
# CC=clang builds and runs the same tests with clang:
#   make clean && make check CC=clang
#
# The support objects (tests/support/jsonl.c, tests/support/golden.c) are linked into every
# test: a test that reads a vector file or a golden file needs them, and the reader is tested
# like the library. They are not part of the library; they are not installed and no public
# header includes them.

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
# fmpz needs GMP; -lm is for the later ball code. MPFR is not in the list, and does not have
# to be: libflint.so.18 has a DT_NEEDED on libmpfr.so.6, so a program that calls arb_* links
# with -lflint alone. Tested on 2026-09-28 with a program that calls arb_init, arb_set_si,
# arb_sqrt and arb_get_str, built with `-lflint -lgmp -lm`; `ldd` shows libmpfr.so.6. Only a
# static libflint.a would need -lmpfr, and FLINT 3.0.1 ships none here. A lane that needs it
# anyway sets `LDLIBS='-lflint -lgmp -lmpfr -lm'` on the make command line.
LDLIBS   ?= -lflint -lgmp -lm

# Dependency files, so that a change in a header rebuilds what includes it.
DEPFLAGS = -MMD -MP

SAN ?= 0
ifeq ($(SAN),1)
CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif

# INV=1 builds the library and the tests with -DADF_CHECK_INVARIANTS (docs/conventions.md 4.4
# and 4.6; lanes/m1-invariants/report.md): every public function checks the predicates of its
# inputs on entry, and a context counts the values that refer to it. The flag changes the
# objects, so `make clean` between a build with INV=1 and one without. -pthread is for the
# threaded test of the borrow count (tests/test_invariants_lifetime.c).
INV ?= 0
ifeq ($(INV),1)
CPPFLAGS += -DADF_CHECK_INVARIANTS
LDLIBS   += -pthread
endif

BUILD = build
LIB   = $(BUILD)/libadelefeld.a

SRC       := $(sort $(wildcard src/*.c))
OBJ       := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
SUPPORT_SRC := $(sort $(wildcard tests/support/*.c))
SUPPORT_OBJ := $(patsubst tests/support/%.c,$(BUILD)/support/%.o,$(SUPPORT_SRC))
TEST_SRC  := $(sort $(wildcard tests/test_*.c))
TEST_BIN  := $(patsubst tests/%.c,$(BUILD)/%,$(TEST_SRC))
DEPS      := $(OBJ:.o=.d) $(SUPPORT_OBJ:.o=.d) $(patsubst tests/%.c,$(BUILD)/%.d,$(TEST_SRC))

.PHONY: all check check-all clean fuzz fuzz-build mutate mutate-selftest bench help
.DELETE_ON_ERROR:
# The objects are built by a pattern rule and are wanted again by the next run, so they are
# kept and not deleted as intermediate files.
.PRECIOUS: $(OBJ) $(SUPPORT_OBJ)

all: $(LIB)

$(BUILD) $(BUILD)/support $(BUILD)/fuzz:
	mkdir -p $@

$(LIB): $(OBJ) | $(BUILD)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)/support/%.o: tests/support/%.c | $(BUILD)/support
	$(CC) $(CPPFLAGS) -Itests $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Each test is one executable: its own translation unit, the support objects, the library,
# FLINT. It runs from the repository root, so that the paths of tests/golden and
# tests/ref/vectors resolve.
$(BUILD)/%: tests/%.c $(LIB) $(SUPPORT_OBJ) | $(BUILD)
	$(CC) $(CPPFLAGS) -Itests $(CFLAGS) $(DEPFLAGS) $(LDFLAGS) $< $(SUPPORT_OBJ) $(LIB) $(LDLIBS) -o $@

check: $(TEST_BIN)
	@fail=0; \
	for t in $(TEST_BIN); do \
	    echo "== $$t"; \
	    ./$$t || fail=1; \
	done; \
	if [ -n "$(TEST_BIN)" ] && [ $$fail -eq 0 ]; then echo "check passed: all $(words $(TEST_BIN)) test programs"; \
	elif [ $$fail -ne 0 ]; then echo "check FAILED"; exit 1; \
	else echo "check: no tests/test_*.c found"; fi

# The whole acceptance run of one tree, in the order the other lanes run them: the C tests, the
# driver, the exported symbols, the Julia smoke test, and the self-tests of the two static tools
# (a self-test is run before the tool is trusted on the tree, not after). It stops at the first
# failure: the name of the step is printed before it runs, so a log says where it stopped.
#
#   make check-all                 the whole run
#   make check-all CC=clang        the same with clang; CC and SAN go to the scripts that read
#                                  them (tests/test_exports.sh, tests/test_driver.sh)
#   make check-all SAN=1 INV=1     the same with the sanitizers and with the invariant checks
#
# CC, SAN and INV are passed on to `make check` explicitly, not only through the environment.
check-all:
	@set -e; \
	echo "== make check CC=$(CC) SAN=$(SAN) INV=$(INV)"; \
	$(MAKE) check CC="$(CC)" SAN="$(SAN)" INV="$(INV)"; \
	echo "== sh tests/test_driver.sh"; \
	SAN="$(SAN)" sh tests/test_driver.sh; \
	echo "== sh tests/test_exports.sh"; \
	CC="$(CC)" sh tests/test_exports.sh; \
	echo "== sh tests/test_julia.sh"; \
	sh tests/test_julia.sh; \
	echo "== python3 tools/mutate/selftest.py"; \
	python3 tools/mutate/selftest.py; \
	echo "== python3 tools/memcheck/selftest.py"; \
	python3 tools/memcheck/selftest.py; \
	echo "check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest"

clean:
	rm -rf $(BUILD)

# Fuzzing. Each tests/fuzz/fuzz_<name>.c is built with clang's libFuzzer and the address and
# undefined-behaviour sanitizers, and run for FUZZ_SECONDS seconds on at most 2 cores.
#
#   make fuzz                        FUZZ_SECONDS = 30
#   make fuzz FUZZ_SECONDS=10        a short run
#   make fuzz FUZZ_TARGET=support    only the target "support"
#   make fuzz FUZZ_WORKERS=2         two libFuzzer workers (still 2 cores in all)
#   make fuzz COV=0                  do not measure coverage
#
# The committed corpus of a target is tests/fuzz/corpus/<name>/. It is copied into
# build/fuzz/corpus/<name>/ before the run, and libFuzzer writes the inputs it keeps to that
# copy, so a run never adds files to the working tree. A crash exits non-zero and leaves the
# reproducer under build/fuzz/artifacts/.

FUZZ_CC       ?= clang
FUZZ_SECONDS  ?= 30
FUZZ_WORKERS  ?= 1
FUZZ_TARGET   ?=
FUZZ_EXTRA    ?=
COV           ?= 1
FUZZ_SRC      := $(sort $(wildcard tests/fuzz/fuzz_*.c))
FUZZ_NAMES    := $(patsubst tests/fuzz/fuzz_%.c,%,$(FUZZ_SRC))
ifneq ($(FUZZ_TARGET),)
FUZZ_NAMES    := $(filter $(FUZZ_TARGET),$(FUZZ_NAMES))
endif
FUZZ_BIN      := $(patsubst %,$(BUILD)/fuzz/%,$(FUZZ_NAMES))
FUZZ_COVBIN   := $(patsubst %,$(BUILD)/fuzz/%_cov,$(FUZZ_NAMES))
# fuzzer for the engine, address and undefined for the memory errors; -fno-sanitize-recover
# makes an undefined behaviour abort the run instead of printing a line and carrying on.
FUZZ_SAN      := -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined
FUZZ_CFLAGS   := -std=c11 -O1 -g -fno-omit-frame-pointer -Wall -Wextra
FUZZ_INCLUDES := -Iinclude -Itests
# libFuzzer is written in C++, so the link needs libstdc++. Clang looks for the libstdc++ of
# the gcc it was built against, which need not be the gcc that is installed, so the directory
# of an installed libstdc++ is added when clang does not find one on its own.
FUZZ_STDCXX   := $(firstword $(wildcard /usr/lib/gcc/*/*/libstdc++.so) \
                         $(wildcard /usr/lib/*/libstdc++.so) \
                         $(wildcard /usr/lib/gcc/*/*/libstdc++.a))
FUZZ_LDFLAGS  := $(if $(FUZZ_STDCXX),-L$(dir $(FUZZ_STDCXX)),)

fuzz-build: $(FUZZ_BIN)

# The coverage pattern rule comes first: for build/fuzz/support_cov both patterns match, and
# make takes the first rule whose prerequisites exist.
$(BUILD)/fuzz/%_cov: tests/fuzz/fuzz_%.c $(LIB) $(SUPPORT_OBJ) | $(BUILD)/fuzz
	$(FUZZ_CC) $(CPPFLAGS) $(FUZZ_INCLUDES) $(FUZZ_CFLAGS) $(FUZZ_SAN) $(FUZZ_LDFLAGS) \
	    -fprofile-instr-generate -fcoverage-mapping $< $(SUPPORT_SRC) $(LIB) $(LDLIBS) -o $@

$(BUILD)/fuzz/%: tests/fuzz/fuzz_%.c $(LIB) $(SUPPORT_OBJ) | $(BUILD)/fuzz
	$(FUZZ_CC) $(CPPFLAGS) $(FUZZ_INCLUDES) $(FUZZ_CFLAGS) $(FUZZ_SAN) $(FUZZ_LDFLAGS) \
	    $< $(SUPPORT_OBJ) $(LIB) $(LDLIBS) -o $@

# With COV=1 and llvm-cov installed, every target also gets a coverage build.
ifeq ($(COV),1)
ifneq ($(shell command -v llvm-cov 2> /dev/null),)
fuzz: $(FUZZ_COVBIN)
endif
endif

fuzz: $(FUZZ_BIN)
	@if [ -z "$(FUZZ_BIN)" ]; then echo "fuzz: no tests/fuzz/fuzz_*.c found"; exit 1; fi
	@fail=0; \
	for n in $(FUZZ_NAMES); do \
	  rm -rf $(BUILD)/fuzz/corpus/$$n $(BUILD)/fuzz/prof; \
	  mkdir -p $(BUILD)/fuzz/corpus/$$n $(BUILD)/fuzz/artifacts; \
	  if [ -d tests/fuzz/corpus/$$n ]; then \
	      cp tests/fuzz/corpus/$$n/* $(BUILD)/fuzz/corpus/$$n/ 2>/dev/null || true; \
	  fi; \
	  echo "== fuzz $$n for $(FUZZ_SECONDS) s, $(FUZZ_WORKERS) worker(s)"; \
	  ( cd $(BUILD)/fuzz && ./$$n $(CURDIR)/$(BUILD)/fuzz/corpus/$$n \
	      $(CURDIR)/tests/fuzz/corpus/$$n \
	      -max_total_time=$(FUZZ_SECONDS) -workers=$(FUZZ_WORKERS) -rss_limit_mb=2048 \
	      -print_final_stats=1 -artifact_prefix=$(CURDIR)/$(BUILD)/fuzz/artifacts/ $(FUZZ_EXTRA) ) \
	      || fail=1; \
	  if [ "$(COV)" = "1" ] && [ -x $(BUILD)/fuzz/$$n\_cov ] && command -v llvm-cov > /dev/null 2>&1; then \
	      echo "-- coverage of $$n over its corpus: which parts of the reader are reached"; \
	      mkdir -p $(BUILD)/fuzz/prof; \
	      LLVM_PROFILE_FILE=$(CURDIR)/$(BUILD)/fuzz/prof/$$n.profraw ./$(BUILD)/fuzz/$$n\_cov \
	          $(CURDIR)/$(BUILD)/fuzz/corpus/$$n -runs=0 > /dev/null 2>&1 || true; \
	      llvm-profdata merge -sparse $(BUILD)/fuzz/prof/$$n.profraw -o $(BUILD)/fuzz/prof/$$n.profdata; \
	      llvm-cov report $(BUILD)/fuzz/$$n\_cov -instr-profile=$(BUILD)/fuzz/prof/$$n.profdata \
	          || echo "   (llvm-cov report failed)"; \
	  fi; \
	done; \
	if [ $$fail -ne 0 ]; then \
	    echo "fuzz FAILED: a crash was found; the reproducer is under $(BUILD)/fuzz/artifacts/"; \
	    exit 1; \
	else echo "fuzz passed: $(words $(FUZZ_NAMES)) target(s), no crash"; fi

# Mutation testing. FILES defaults to every source file; LIMIT caps the number of mutants, so
# that a run stays inside a laptop hour. The tool works in a scratch copy under build/mutate/
# and never writes into the working tree. A run of 200 mutants of one file of about 1000 lines
# takes about 3.5 minutes with two jobs.
#
#   make mutate                                every src/*.c, 200 mutants, 2 jobs
#   make mutate FILES=src/fball.c               one file
#   make mutate FILES='src/a.c src/b.c' LIMIT=20 SEED=7
#
# The target fails if a mutant survives, unless tools/mutate/equivalent.txt lists it with a
# reason.
FILES        ?= $(SRC)
# Both spellings work: make mutate LIMIT=20 and make mutate MUTATE_LIMIT=20.
MUTATE_LIMIT ?= $(LIMIT)
MUTATE_SEED  ?= $(SEED)
MUTATE_JOBS  ?= $(JOBS)
LIMIT ?= 200
SEED  ?= 20260928
JOBS  ?= 2

mutate:
	@files="$(FILES)"; \
	if [ -z "$$files" ]; then echo "mutate: no source file; src/ is empty"; exit 1; fi; \
	python3 tools/mutate/mutate.py --root . --scratch $(BUILD)/mutate --jobs $(MUTATE_JOBS) \
	    --seed $(MUTATE_SEED) --limit $(MUTATE_LIMIT) --files $$files

# The self-test of the mutation tool: a small example with a deliberately weak test, which
# must leave a surviving mutant, and the same example with a strong test, which must not.
mutate-selftest:
	python3 tools/mutate/selftest.py

bench:
	$(MAKE) -C bench run

help:
	@echo "targets: all check check-all clean fuzz mutate mutate-selftest bench"
	@echo "  check-all: make check, then the driver, exports and julia scripts and the two"
	@echo "             tool self-tests, one after the other, stopping at the first failure"
	@echo "variables: SAN=1 adds the sanitizers, CC=clang builds with clang,"
	@echo "            FUZZ_SECONDS=30 sets the length of a fuzz run,"
	@echo "            FILES=, LIMIT=, SEED=, JOBS= set the mutation run"

# Header dependencies written by -MMD -MP; without this a changed header does not rebuild the tests.
-include $(DEPS)
