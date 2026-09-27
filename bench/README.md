# bench: the adelefeld benchmark harness

This directory holds the benchmark harness of work package 0.5 and the word-kernel rows. The
measurement contract is `docs/PERF.md` section 7. `bench/baseline.c` and
`bench/baseline_2026-09-27.txt` are the dated reference of the design review and are not changed.

## Build, run, test

    make -C bench          # build bench/bench_word
    make -C bench run      # build, save the chain assembly, write one result file
    make -C bench test     # harness self-tests
    make -C bench asm      # save the chain assembly only
    make -C bench clean    # remove the binary and bench/asm

`make -C bench` uses `gcc -O2 -g` by default and links `-lflint -lmpfr -lgmp`. Override with
`make -C bench FLAGS='-O3 -march=native'`; the flags in force are written into the result file.
`run` pins to cpu 2 through `sched_setaffinity`; change it with
`./bench/bench_word --run --cpu N`.

## Result file

Every run writes one dated text file `bench/results/<utc>_word.txt`. It has `key: value` lines
and then one tab-separated table. The keys state the machine, the pinned core, the compiler and
flags, the FLINT and GMP versions, the clock source, whether the TSC is present and invariant,
the measured cycles per nanosecond and its spread over the calibration trials, the seed and the
modulus. The result file also says `quiet_machine: no`: other agents share this laptop, so the
numbers test the harness rather than the machine.

The table has one row per benchmark. The columns are

    name kind operand_family bit_lengths seed warm allocating calls ops
    checksum ns_per_op_min ns_per_op_median ns_per_op_max

`kind` is `latency_chain`, `independent_batch` or `call_rate`. A chain feeds its own result back
as the next left operand; a batch works on independent pairs. `calls` is the number of kernel
repetitions in one timed sample and `ops` is the number of operations in that sample. The
statistics are the minimum, the upper median and the maximum of the per-operation time over
`trials` samples.

Every kernel leaves its result in its context. The driver reads the context only after the timed
region, prints the checksums and leaves the last checksum in the result file, so the optimiser
cannot delete the work.

## Reading the dependency path from the assembly

`make -C bench run` writes one file per chain kernel under `bench/asm`:

    bench/asm/chain_add_run.s          nmod_add
    bench/asm/chain_mul_run.s          nmod_mul
    bench/asm/chain_mul2_run.s         n_mulmod2_preinv

Each file is `objdump -d -M intel -S --disassemble=<kernel>` of the built binary, so the C line
and the machine instruction appear together. To read the chain, find the innermost loop (the
backward `jne`) and follow the register that carries the value:

1. `chain_add_run`: the loop body holds `add rax,r11` and `sub r9,rax`. `rax` is read at the top
   of the loop and written by `add rax,r11`, so the next iteration starts from the result of the
   previous one. The path is `add` then `sub`/`cmp`/`cmovb`.
2. `chain_mul_run`: `rsi` is read by `mov rax,rsi` at the loop top and written by `shr rsi,cl` at
   the bottom. In between, `mul r9`, `mul rdx`, `adc rdx,rcx`, `add rdx,0x1`, `imul rdx,rdi`,
   `sub rsi,rdx`, `cmovb`, `sub rax,rdi`, `cmovae` form the reduction. This is the 14-cycle path
   discussed in `docs/PERF.md` section 3.
3. `chain_mul2_run`: `mul rbx` reads and writes `rax`, then a `call n_ll_mod_preinv` reduces the
   128-bit product. The call is in the loop, so this kernel is a chain through a library call.

The self-test `make -C bench test` checks a weaker property automatically: for each chain kernel
it disassembles the function, requires a loop back edge, and requires a register that is read
before it is first written in the loop and is not the loop-control register. That register is the
loop-carried value. The test prints the instruction it found, so the check can be compared with
the saved file. The test does not prove that the arithmetic result is on that path; a reader has
to confirm the path in the assembly, as above.

## Self-tests

`make -C bench test` runs, and fails on:

- `bench_stats_compute` on known arrays: n=5 (`1, 3, 5`), n=4 with the upper median (`2, 8, 10`),
  n=1 and an array with equal values.
- the chain kernels over 1000 steps against a second FLINT function for the same operation
  (`n_addmod` against `nmod_add`, `n_mulmod2_preinv` against `nmod_mul`, and the converse), so a
  kernel that skips steps or uses the wrong operand fails.
- the assembly loop-carried check of the three chain kernels.

## Rows and limits

The word rows are add and multiply modulo the 62-bit prime `p = 2^62 - 57`, as a latency chain
and as an independent batch, plus the same for `n_mulmod2_preinv`. The chain rows are comparable
with the `nmod_add` and `nmod_mul` rows of `baseline_2026-09-27.txt`. The batch rows have no row
in the baseline; they are compared with the throughput figures of `docs/PERF.md` section 1 only
as REFERENCE, and no ratio is claimed here.

The batch arrays hold 1024 residues (8 KiB each, 24 KiB for the three arrays) and stay in L1.
The timed batch repeats passes over the same arrays, which is a throughput measurement of warm
data, not a compulsory-I/O measurement.
