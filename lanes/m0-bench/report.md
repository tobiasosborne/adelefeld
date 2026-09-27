# Lane m0-bench report: work package 0.5, benchmark harness

## What was done

A benchmark harness for the word kernels, with the contract of `docs/PERF.md` section 7:

- `bench/harness.h`, `bench/harness.c`: statistics (min, upper median, max), wall clock on
  `clock_gettime(CLOCK_MONOTONIC_RAW)`, a TSC calibration, `sched_setaffinity` pinning, the timed
  measurement loop, and a dated result-file opener.
- `bench/bench_word.c`: six matched rows over the modulus `p = 2^62 - 57`, the same modulus as
  `bench/baseline.c`: `nmod_add` chain and batch, `nmod_mul` chain and batch,
  `n_mulmod2_preinv` chain and batch. It also holds the self-tests.
- `bench/Makefile`: `make -C bench`, `make -C bench run`, `make -C bench test`,
  `make -C bench asm`, `make -C bench clean`.
- `bench/README.md`: how to build, what the result file means, and how to read the dependency
  path from the saved assembly.
- `bench/asm/`: `chain_add_run.s`, `chain_mul_run.s`, `chain_mul2_run.s`. Each is
  `objdump -d -M intel -S --disassemble=<kernel>` of the built binary.
- `bench/results/2026-09-27T202944Z_word.txt`: the one result file of the final run. An earlier
  run wrote a second file; the fixed chain operand was only 31 bits in it, so the generator was
  corrected to full width and the result was regenerated. The earlier file was removed.

The chain kernels feed their own result back as the next left operand. The driver reads each
context only after the timed region, prints a checksum, and writes the checksum into the result
file. The globals (machine, core, compiler, flags, FLINT, GMP, clock, TSC calibration, seed,
modulus) are `key: value` lines; the rows are one tab-separated table.

## Files written

    bench/harness.h
    bench/harness.c
    bench/bench_word.c
    bench/Makefile
    bench/README.md
    bench/asm/chain_add_run.s
    bench/asm/chain_mul_run.s
    bench/asm/chain_mul2_run.s
    bench/results/2026-09-27T202944Z_word.txt

## Checks that were run

1. `make -C bench` builds `bench/bench_word` with `gcc -O2 -g` and `-lflint -lmpfr -lgmp`.
   Result: exit 0, no warnings.
2. `gcc -O2 -g -Wall -Wextra -DBENCH_CFLAGS='"-O2 -g"' -o /tmp/bench_word_warn harness.c
   bench_word.c -lflint -lmpfr -lgmp` (extra warning pass). Result: exit 0, no diagnostics.
3. `make -C bench test`. Result: 10 PASS, 0 FAIL:
   - stats n=5 -> min 1 median 3 max 5
   - stats n=4 -> min 2 median 8 (upper) max 10
   - stats n=1 -> 7 7 7
   - stats n=6 all equal
   - chain_add 1000 steps matches an `n_addmod` loop
   - chain_mul 1000 steps matches an `n_mulmod2_preinv` loop
   - chain_mul2 1000 steps matches an `nmod_mul` loop
   - asm chain_add_run: `add rax,r11` (loop-carried rax) in loop [26d0,26eb)
   - asm chain_mul_run: `sub rsi,rdx` (loop-carried rsi) in loop [2730,2777)
   - asm chain_mul2_run: `mul rbx` (loop-carried rax, implicit) in loop [2980,299b)
   - `# self-test PASS (0 failures)`
4. `make -C bench run`. Result: exit 0; whole run 1.9 s wall. Result file
   `bench/results/2026-09-27T202944Z_word.txt`. Pinned to cpu 2 (requested 2, actual 2).
5. Checksum consistency: the XOR of the six `checksum` columns equals the final `checksum` line.
   Recompute: `2729610574065456106`. Result: match.
6. Assembly read by hand. `bench/asm/chain_add_run.s` loop holds `add rax,r11`; `rax` is loaded
   from `x0`, written by the add, and the next iteration starts from it. `bench/asm/chain_mul_run.s`
   loop holds `mul r9`, `mul rdx`, `adc rdx,rcx`, `imul rdx,rdi`, `sub rsi,rdx`, and `shr rsi,cl`;
   `rsi` is the loop-carried value. `bench/asm/chain_mul2_run.s` holds `mul rbx` and a call to
   `n_ll_mod_preinv`; `rax` carries the chain.

## Final numbers

Machine: 13th Gen Intel(R) Core(TM) i7-1365U, 12 cores online. This is not the machine of
`docs/PERF.md` (AMD Threadripper 3970X, Zen 2). Pinned to cpu 2. gcc 13.3.0, `-O2 -g`, FLINT
3.0.1, GMP 6.3.0. Clock `CLOCK_MONOTONIC_RAW`. TSC invariant: yes. Cycles per nanosecond over 9
trials of 20 ms: min 2.68798, median 2.68800, max 2.68800, spread 0.00001. Seed 20260928.
`quiet_machine: no`. 15 trials per case; min / median / max in ns per operation:

    nmod_add_chain           0.8107 / 0.8115 / 0.8130
    nmod_mul_chain           4.9603 / 4.9691 / 4.9733
    n_mulmod2_preinv_chain   5.9530 / 5.9571 / 5.9744
    nmod_add_batch           0.5343 / 0.5345 / 0.6940
    nmod_mul_batch           1.4520 / 1.4720 / 1.4820
    n_mulmod2_preinv_batch   2.9141 / 2.9170 / 2.9274

The `nmod_add_batch` max of 0.6940 ns is an outlier on a shared machine; the min and median are
0.5343 and 0.5345 ns.

## Comparison with `bench/baseline_2026-09-27.txt`

The two comparable rows are the chains. Baseline (AMD 3970X, gcc `-O2`, cpu 2):

    nmod_add chain   min 0.74   median 0.74   max 0.77
    nmod_mul chain   min 4.99   median 5.07   max 5.19

Difference of the medians: add `(0.8115 - 0.74)/0.74 = +9.7 %`, multiply
`(4.9691 - 5.07)/5.07 = -2.0 %`. Both are below 20 %, so no larger difference needs an
explanation. The two machines differ: the TSC here counts 2.688 ticks per ns, so 0.8115 ns is
about 2.18 TSC ticks for the add and 4.969 ns about 13.4 TSC ticks for the multiply. The baseline
machine ran at 3.7 to 4.5 GHz, so its cycle counts are larger while its nanoseconds are similar.
TSC ticks are not core cycles, so these tick counts are not a floor comparison.

The batch rows and the `n_mulmod2_preinv` rows have no baseline row. They are not compared with
the floors here.

## What is not done

- No `call_rate` row is produced. The harness has the `bench_kind` value and the reporting path,
  but the three FLINT word functions are inlined, so a fixed-operand call rate would be hoisted
  unless wrapped in a non-inlined call. The row is left out rather than measured misleadingly.
- Only `-O2 -g` was measured. `make -C bench FLAGS='-O3 -march=native'` is supported and records
  the flags, but no such run is in `bench/results/`.
- The batch rows cover warm L1-resident arrays only. No compulsory-I/O or cold-cache row.
- The numbers are for testing the harness: other lanes run on this laptop (`quiet_machine: no`).

## Sources

- `docs/PERF.md` section 7, the benchmark contract; sections 1 to 3, the rows and hardware model.
- `bench/baseline.c` and `bench/baseline_2026-09-27.txt`, the dated reference rows (read only).
- FLINT 3.0.1 headers on this machine: `/usr/include/flint/nmod.h` lines 131 to 173 (`nmod_add`,
  `nmod_mul`, `NMOD_MUL_PRENORM`); `/usr/include/flint/ulong_extras.h` lines 172 to 181
  (`n_mulmod2_preinv`) and 223 to 241 (`n_addmod`).

Sources pending: none.

## Findings against the specification

None. The harness meets the clauses of `docs/PERF.md` section 7. The 14-cycle dependency path of
`docs/PERF.md` section 3 is a MODEL on the hardware of section 1 (AMD Zen 2); this machine is an
Intel i7-1365U with different latencies, so no comparison with that number is made.
