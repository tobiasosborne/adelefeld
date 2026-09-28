# Reviewer `dump`: the dump form (loaders, dumpers, inspectors) and `adf_modctx_new_from_dump`

Read `lanes/m1-review/COMMON.md`; it binds you. Author: Claude opus. You are of another model family.
Files under review: `src/dump.c`; the function `adf_modctx_new_from_dump` of `src/modctx.c`;
`tests/test_dump.c`, `tests/test_dump_ctx.c`, `tests/test_dump_golden.c`, `tests/fuzz/fuzz_dump.c`.
Header `include/adelefeld/dump.h`, `modctx.h`. `docs/conventions.md` sections 8 and 10, 5 (the predicates;
5.3 the raw predicate L, CV-55), 4.3. Reference `proto/text_grammar.py` (dump part). Golden
`tests/golden/dump.tsv`. The lane's report `lanes/m1-dump/report.md`: it says two of its three test files
were written after the code, and it lists assumptions about the scaled type and the local backend that
were made before those two landed; both are on master now.

This code reads untrusted input and restores internal representations from it. Look in particular at:
- **An accepted text that builds an invalid value.** A loaded value must satisfy the predicate of its type
  (`is_canonical`), also for local balls (residues below their blocks, `H = K`, `A = 0`, `d >= 1`), scaled
  values (`0 <= u < K`, `s > 0`, exact tag), real balls (finite, mantissas as the conventions say). Find a
  text that is accepted and gives a value on which a later operation is wrong or reads out of bounds.
  Build your own generator of dump texts from the grammar of conventions 10.1 and mutate valid texts.
- **Identity.** Dump, load with the value's own contexts, `identical`, dump again: same bytes, for both
  backends, now with the real functions of `scaled.h` and of the local backend (`adf_fball_set_local`,
  arithmetic) producing the values, not values built field by field.
- **Context bindings:** count, NULL, mismatch of modulus, of block order, repeated occurrences, the
  one-context form; a binding whose context is freed by the caller while the value lives is the caller's
  fault, but a loader that keeps a pointer into the input text or into a descriptor is not.
- **Memory:** input not NUL-terminated at the end of a page; every failure path frees what it allocated
  (valgrind, and a counting allocator through `__flint_set_memory_functions`); the inspector with
  capacities 0, exact, one less; output untouched on every status other than OK.
- **Cost:** the lane reports a dump below 1 MiB that takes 60 s to load (83354 blocks); decision M1-D5 of
  `docs/SPEC.md` section 15 now limits a context to 65536 blocks: does the loader apply the limit before
  the quadratic coprimality check and before it builds anything? Find the most expensive text within the
  default limits and measure it.
- **The differential test** against the reference on at least 10^5 texts, all bodies.
- **The order of checks** of conventions 8.5 with two faults; header finding H3 of the lane (a LIMIT raised
  inside stage 6).
