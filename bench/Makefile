# bench/Makefile: build and run the adelefeld word-kernel benchmarks.
# Contract: docs/PERF.md section 7.
#
#   make -C bench        build the benchmark binary
#   make -C bench run    build, run, save the compiled chain loops and a result
#   make -C bench test   run the harness self-tests
#   make -C bench asm    save the compiled chain loops under bench/asm
#   make -C bench clean  remove the binary and the saved assembly
#
# Override the build with e.g.  make -C bench FLAGS='-O3 -march=native'

CC      = gcc
FLAGS  ?= -O2 -g
LIBS   ?= -lflint -lmpfr -lgmp -lm
BIN     = bench_word
# Every other bench_<name>.c is found by wildcard and links all of ../src/*.c, so a lane that adds
# bench/bench_<name>.c does not edit this file (and parallel lanes do not conflict here).
LIB_SRC  := $(sort $(wildcard ../src/*.c))
LIB_BENCH := $(filter-out $(BIN),$(patsubst %.c,%,$(sort $(wildcard bench_*.c))))
ASM_DIR = asm
CHAIN_SYMS = chain_add_run chain_mul_run chain_mul2_run

.PHONY: all run test asm clean

all: $(BIN) $(LIB_BENCH)

$(BIN): harness.c bench_word.c harness.h
	$(CC) $(FLAGS) -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_word.c $(LIBS)

# The rows of the library link its sources themselves, so `make -C bench` needs no built library.
$(LIB_BENCH): %: %.c harness.c harness.h $(LIB_SRC)
	$(CC) $(FLAGS) -I../include -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c $< $(LIB_SRC) $(LIBS)

asm: $(BIN)
	mkdir -p $(ASM_DIR)
	for s in $(CHAIN_SYMS); do \
		objdump -d -M intel -S --disassemble=$$s $(BIN) > $(ASM_DIR)/$$s.s; \
	done

run: all asm
	cd .. && ./bench/$(BIN) --run
	for b in $(LIB_BENCH); do (cd .. && ./bench/$$b --run) || exit 1; done

test: $(BIN)
	cd .. && ./bench/$(BIN) --selftest

clean:
	rm -f $(BIN) $(LIB_BENCH)
	rm -rf $(ASM_DIR)
