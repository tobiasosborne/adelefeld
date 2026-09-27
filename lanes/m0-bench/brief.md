# Lane m0-bench: work package 0.5 (benchmark harness)

Read `docs/PERF.md` completely (section 7 is your contract; sections 1 to 3 give the rows), `bench/baseline.c` and
`bench/baseline_2026-09-27.txt` (both are dated references: read only, do not change them).

**You own:** `bench/harness.h`, `bench/harness.c`, `bench/bench_word.c`, `bench/Makefile`, `bench/README.md`,
`bench/asm/`, `bench/results/`.

Task: a harness that satisfies every clause of `PERF.md` section 7, and matched rows for the word kernels
(word add modulo a 62-bit modulus as a latency chain and as an independent batch; word multiply modulo the same, by
FLINT's `nmod_mul` / `n_mulmod2_preinv` as used in the baseline, chain and batch).

- Each benchmark declares: kind (latency chain, independent batch, call rate), operand family and actual bit
  lengths, seed, warm or allocating, affinity (pin with `sched_setaffinity`; record the core), compiler, flags,
  FLINT and GMP versions, clock source (`clock_gettime(CLOCK_MONOTONIC_RAW)`; also record whether `rdtsc` is
  invariant and the measured cycles per nanosecond, stated as a measurement with its spread).
- Results are consumed outside the timed region (checksum printed), so the compiler cannot delete the work.
- Minimum, median and maximum over repetitions are reported. Output: one dated text file per run under
  `bench/results/`, machine-readable (key: value lines plus one table).
- The compiled hot loop of each chain benchmark is saved as assembly under `bench/asm/` (`objdump -d` of the
  function, or `-S` output), next to the result, and the README says how to read the dependency path from it.
- `bench/Makefile` with `make -C bench` and `make -C bench run`. Do not touch the top-level `Makefile`.

Other agents are running on this laptop now, so your numbers are for testing the harness only. Say so in the result
file (field `quiet_machine: no`). Keep runs short (each benchmark under 5 seconds). Compare your numbers with the
baseline file and explain any difference larger than 20 percent, or say that it is unexplained.

Write tests of the harness itself where it can be wrong: the statistics (min, median, max on a known array), and
that a chain benchmark really is a chain (each input depends on the previous output; show it in the assembly).
