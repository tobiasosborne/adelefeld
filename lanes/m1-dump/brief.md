# Lane m1-dump: the dump form, loaders, dumpers, inspectors (work package 1.4, second part)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/dump.h` completely, `modctx.h`, `scaled.h`, `fball.h`,
`adele.h`, `text.h`, `docs/api-m1.md`; `docs/conventions.md` sections 3, 4.2, 4.3, 4.6, 5 (the predicates of
every type, the raw predicate L of 5.3, CV-55), 8, 10 completely, 11.3 item 5, 12.8; `docs/SPEC.md` section
10; `docs/reviews/m0-gate/review.md` findings G3, G14 and `closure.md` C2, C3; `proto/text_grammar.py` (the
dump part: it is the reference); `tests/golden/dump.tsv`, `tests/golden/README.md`; `src/text.c` (how the
value form is parsed; match its style) and `lanes/m1-text/report.md` (the finding on setting a `mag`
exactly: `mag_set_ui_2exp_si` is exact, `mag_set_fmpz_2exp_fmpz` and `arf_get_mag` are not);
`src/modctx.c` (`adf_modctx_new_from_dump`, which handles the body `modctx` only) and
`lanes/m1-modctx-b/report.md` finding 2.

This code reads untrusted input and restores internal representations from it.

**You own:** `src/dump.c`, `tests/test_dump.c`, `tests/test_dump_golden.c`, `tests/test_dump_ctx.c`,
`tests/fuzz/fuzz_dump.c`, `tests/fuzz/corpus/dump/`, `tests/ref/vectors/m1-dump/`, `bench/bench_dump.c`,
and in `src/modctx.c` only the function `adf_modctx_new_from_dump` and static helpers you add for it (the
rest of that file is read-only).

Implement every function that `dump.h` declares (for `adf_rat`, `adf_fball`, `adf_scaled`, `adf_adele`,
`adf_cadele`: `load_str`, `load_str_binds`, `dump_str`, `dump_inspect`; and `adf_scaled_get_str`), and extend
`adf_modctx_new_from_dump` to every body that has context occurrences, as its header says.

Two lanes run at the same time as you: m1-scaled writes `src/scaled.c` and m1-local writes
`src/fball_local.c` and the local behaviour in `src/fball.c`. Their functions are not in your worktree. So:
- do not call functions of `scaled.h` (except the inline layout queries) or the five local conversions of
  `modctx.h`; read and write the fields of `adf_scaled_struct` and of a local `adf_fball_struct` directly,
  as the layouts and predicates of the headers and of conventions 5.3, 5.4 describe them; allocate the
  residue array as the header of `fball.h` says it is owned (read `adf_fball_clear` in `src/fball.c`:
  it frees `res` with `flint_free`);
- in tests, build local balls and scaled values field by field, with contexts from the constructors of
  `modctx.h` (they are on master), and clear them field by field where the library function does not exist;
  write one helper file of your own for that inside `tests/test_dump_ctx.c`;
- say in the report which assumptions about those two types you made, so that the orchestrator can check
  them when the other lanes land.

Rules of this lane:
- One validating pass over `(s, len)` first, by hand, no reliance on a NUL terminator, no `strlen`,
  `strtol`, `sscanf`, `atoi` on the input. Only after the whole text is valid is anything built. No FLINT
  load or string function ever sees raw input (M0-D9, CV-52): `arb_load_str`, `arb_set_str`,
  `fmpz_set_str` on raw input are forbidden; a number is copied into a buffer of its own after validation.
  Real balls are rebuilt exactly from their validated fields (midpoint mantissa and exponent, radius
  mantissa and exponent) with exact setters.
- The loader is strict: only canonical text; it never canonicalises; a field that violates a predicate is
  `ADF_DOMAIN`. Order of checks as in conventions 8.5; test it with texts that have two faults.
- Output in a temporary, swapped in on `ADF_OK` only; on every other status the output and, for the
  inspector, `*nctx` and the descriptors are as the header says.
- Context bindings: every rule of the header (count, NULL, mismatch of modulus or of block order, repeated
  occurrences sharing a pointer, the one-context convenience form).
- No recursion depending on the input, no allocation not bounded by the limits.

Tests: every row of `tests/golden/dump.tsv`; dump, load with the value's own contexts, `identical`, dump
again, same bytes, for random values of every type and both backends, 1, 2, 64 and 128 blocks, operands of
4096 bits; real balls with radius mantissa at 1, 2^29, 2^30 - 1 and with exponents of both signs; every
status; every rule on bindings; the inspector with `descs = NULL`, with too little capacity, with exact
capacity; hostile input as in lane m1-text (NUL inside, bytes 128 to 255, input ending at the end of a page
with a `PROT_NONE` page behind it, a heap block of exact size); non-canonical texts that denote a valid
value (upper-case hexadecimal, leading zeros, two spaces, a trailing space, a final newline) are refused.

Fuzzing: `tests/fuzz/fuzz_dump.c`, as `tests/fuzz/fuzz_text.c`: the input goes to every loader with a small
fixed set of contexts and to the inspectors; on `ADF_OK` the value is dumped and the bytes must equal the
input; predicates hold. Seed from the golden file. `make fuzz FUZZ_TARGET=dump FUZZ_SECONDS=170`; report
coverage of `src/dump.c` and the lines not reached.

Memory: `make check`, `make clean && make -j2 check SAN=1`, and your test programs under
`valgrind -q --error-exitcode=9 --leak-check=full --errors-for-leak-kinds=definite,indirect` (build without
sanitizers).

Benchmark rows of PLAN row 1.4 for the dump form, provisional. Mutation run:
`make mutate FILES=src/dump.c JOBS=2 LIMIT=300` (tool repaired today, `lanes/tools-mutate/report.md`); each
survivor killed by a new test or listed with its reason; append-only to `tools/mutate/equivalent.txt`.
