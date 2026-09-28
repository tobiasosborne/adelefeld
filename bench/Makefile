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
BIN_RECON = bench_recon
ASM_DIR = asm
CHAIN_SYMS = chain_add_run chain_mul_run chain_mul2_run

.PHONY: all run test asm clean

all: $(BIN) $(BIN_FBALL) $(BIN_RECON)

$(BIN): harness.c bench_word.c harness.h
	$(CC) $(FLAGS) -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_word.c $(LIBS)

# The fball rows link the finite-ball source itself, so `make -C bench` needs no built library.
$(BIN_FBALL): harness.c bench_fball.c ../src/fball.c harness.h
	$(CC) $(FLAGS) -I../include -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_fball.c ../src/fball.c $(LIBS)

# The recon rows link the reconstruction and the finite ball themselves (lane m1-recon).
$(BIN_RECON): harness.c bench_recon.c ../src/recon.c ../src/fball.c ../src/rat.c harness.h
	$(CC) $(FLAGS) -I../include -DBENCH_CFLAGS='"$(FLAGS)"' -o $@ harness.c bench_recon.c ../src/recon.c ../src/fball.c ../src/rat.c $(LIBS)

asm: $(BIN)
	mkdir -p $(ASM_DIR)
	for s in $(CHAIN_SYMS); do \
		objdump -d -M intel -S --disassemble=$$s $(BIN) > $(ASM_DIR)/$$s.s; \
	done

run: all asm
	cd .. && ./bench/$(BIN) --run
	cd .. && ./bench/$(BIN_FBALL) --run
	cd .. && ./bench/$(BIN_RECON) --run

test: $(BIN)
	cd .. && ./bench/$(BIN) --selftest

clean:
	rm -f $(BIN) $(BIN_FBALL) $(BIN_RECON)
	rm -rf $(ASM_DIR)
