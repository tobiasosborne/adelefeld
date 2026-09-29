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
LIBS   ?= -lflint -lmpfr -lgmp
BIN     = bench_word
BIN_FBALL = bench_fball
BIN_MODCTX = bench_modctx
ASM_DIR = asm
CHAIN_SYMS = chain_add_run chain_mul_run chain_mul2_run

.PHONY: all run test asm clean

all: $(BIN) $(BIN_FBALL) $(BIN_MODCTX)

$(BIN): harness.c bench_word.c harness.h
	$(CC) $(FLAGS) -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_word.c $(LIBS)

# The fball rows link the finite-ball source itself, so `make -C bench` needs no built library.
$(BIN_FBALL): harness.c bench_fball.c ../src/fball.c harness.h
	$(CC) $(FLAGS) -I../include -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_fball.c ../src/fball.c $(LIBS)

# The modctx rows (lane m1-modctx) link the modulus-context source itself.
$(BIN_MODCTX): harness.c bench_modctx.c ../src/modctx.c harness.h
	$(CC) $(FLAGS) -I../include -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_modctx.c ../src/modctx.c $(LIBS)

asm: $(BIN)
	mkdir -p $(ASM_DIR)
	for s in $(CHAIN_SYMS); do \
		objdump -d -M intel -S --disassemble=$$s $(BIN) > $(ASM_DIR)/$$s.s; \
	done

run: all asm
	cd .. && ./bench/$(BIN) --run
	cd .. && ./bench/$(BIN_FBALL) --run
	cd .. && ./bench/$(BIN_MODCTX) --run

test: $(BIN)
	cd .. && ./bench/$(BIN) --selftest

clean:
	rm -f $(BIN) $(BIN_FBALL) $(BIN_MODCTX)
	rm -rf $(ASM_DIR)
