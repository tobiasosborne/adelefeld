# Lane m1-dump: report (Claude opus; summary by the orchestrator of the final message of the lane)

Work package 1.4, second part: the dump form. Base `ec4a2fc` (before the scaled policy and the local backend
landed).

## What was done

- `src/dump.c` (1825 lines): every function of `dump.h` for rat, fball, scaled, adele, cadele (`load_str`,
  `load_str_binds`, `dump_str`, `dump_inspect`) and `adf_scaled_get_str`. One validator by hand over
  `(s, len)`; stages of conventions 8.5 in order (syntax; limits; word size; predicates of section 5;
  occurrences); nothing is built before the whole text has passed; output in a temporary, swapped in on OK.
  No FLINT load or string function sees raw input. Real balls are rebuilt exactly (`arf_set_fmpz_2exp`; the
  radius written into the fields of the `mag`). The grammar covers the 15 bodies of conventions 10.1; stage
  6 is implemented for rat, fball, scaled, adele, cadele, modctx, qclass.
- `src/modctx.c`: `adf_modctx_new_from_dump` handles every body (through the hidden function
  `adf_dump_ctx_occurrence` of `src/dump.c`); its old static helpers are removed. `adf2x ...` now gives PARSE
  as the reference does (finding R4 of reviewer `contexts`).
- Tests: `test_dump.c` 15 tests / 12165 checks; `test_dump_ctx.c` 14 / 28587; `test_dump_golden.c` 1 / 735.
  Fuzz target `tests/fuzz/fuzz_dump.c`, 448 seeds. Vectors `tests/ref/vectors/m1-dump/dump_ref.jsonl`, 2180
  records from `proto/text_grammar.py`. `bench/bench_dump.c`.

## Decision of the orchestrator at the merge (finding F1 of the lane)

Record 136 of `tests/ref/vectors/m1-modctx/modctx.jsonl` expected PARSE for `adf1 Q fball g 1 6 1`, written
when the function knew only the body `modctx`. The text is a valid dump without a context occurrence;
`modctx.h` says an occurrence index that is not below the number of occurrences is `ADF_DOMAIN`. The record
is set to DOMAIN.

## Process deviation (reported by the lane)

`test_dump_ctx.c` and `test_dump_golden.c` were written after the code. The lane showed afterwards that they
fail against a stub (thousands of failed checks; 270 for the golden test).

## Checks by the lane

- Golden file: 165 rows pass. Reference vectors: 2180 records pass.
- `make check` and `make check SAN=1`: all pass except one check of `test_modctx` (F1, now repaired).
- Valgrind on the three test programs: exit 0.
- Fuzz, 170 s: 1816714 executions, no crash; coverage of `src/dump.c` 99.50% of lines; not reached: three
  unreachable `flint_abort` guards (161, 1417, 1724), and 175, 748, 913, which unit tests reach.
- Mutation (tool called directly with the three test programs of the lane, because the baseline of
  `make mutate` failed on F1; limit 300 of 1007 possible mutants; run 3 with the sanitizers): 234 killed,
  0 survived, 56 not compiled, 10 excused. The lines for `equivalent.txt` are in
  `lanes/m1-dump/equivalent-added.txt`.
- Benchmark (provisional): dumping about 1 ns per byte; loading a 4096-bit value 20 to 26 ns per byte.

## Assumptions about the two types of other lanes

Scaled: the struct of `scaled.h`; fields swapped in directly; no function of `scaled.h` called. Local fball:
backend `ADF_LOCAL`, `A = 0`, `H = K`, `d`, `res` of `k` words from `flint_malloc`, borrowed `mctx`; relies on
`adf_fball_clear` freeing `res` and on `adf_fball_swap` exchanging backend, `mctx`, `res`. The tests use
their own `local_identical` and `local_is_L`, because the functions of the library did not handle local
values in the worktree of the lane.

## Header findings

H1: a typed loader given a dump of another body: PARSE chosen. H2: inspector with `nctx == NULL`: DOMAIN
chosen. H3: qclass pieces with an arb exponent above 2^20 give LIMIT inside stage 6, as the reference does;
this contradicts the order of 8.5.

## Findings

F2: the reference refuses char moduli above 10^5 with UNSUPPORTED; the C returns DOMAIN from
`new_from_dump`. F3: the default `max_items` of 1048576 does not bound work: a dump below 1 MiB with 83354
blocks takes 1.8 s to validate (quadratic coprimality check) and 61.6 s to build the context. (Decision
M1-D5 of the same day limits a context to 65536 blocks; the loader has to apply it.) F4: the dump text of a
real ball equals `arb_dump_str` on 700 random balls.

## Not done

Stage 6 of ucoset, idele, idclass, lball, sball, ffun, rfun, char (bodies of later milestones). 707 of
1007 mutants not run. No floor for the benchmark.

## Sources pending

A FLINT document stating that a `mag` is `MAG_MAN * 2^(MAG_EXP - MAG_BITS)` with 30 mantissa bits; the
code relies on `mag.h:113-140`.
