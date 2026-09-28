# lanes/m1-modctx-b: red-green log

Per COMMON-C rule 1, in order.  The first test of the file was linked before
`src/modctx.c` existed, so the first red run was a link error; after that every
red run was an assertion failure.

1. `tests/ref/adfref/modctx_ref.py` written from `docs/conventions.md` 5.14 and
   `docs/proofs/policies.md` Definition 16 to Lemma 18.
2. `lanes/m1-modctx-b/gen_modctx_vectors.py` writes
   `tests/ref/vectors/m1-modctx/modctx.jsonl` (140 records).  One bug found and
   fixed in the reference generator: it did not reject trailing block tokens
   (`adf1 Q modctx 6 2 2 3 5`), so it disagreed with the golden file; after the
   fix the vector expects PARSE.
3. `tests/test_modctx.c` first version, linked against the not-yet-written
   `src/modctx.c`: link error, as expected for the first test of a file.
4. First implementation of `src/modctx.c`, then
   `make -j2 build/test_modctx && ./build/test_modctx`:
   6 tests, 8159 checks, **71 failed checks**, 2 failed tests.  Causes:
   `new_prime_powers` branch length 17 instead of 16, the `roundtrip`/`dump`
   branches fell through to the generic result check, the `dump` vector with
   `K = 10**30` has no block and is not built by `new_blocks`, and the
   `max_items` test used `len = 22` for a 21-byte text.
5. After those fixes: 6 tests, 8132 checks, 0 failed checks, 0 failed tests.
6. `make clean && make -j2 SAN=1 build/test_modctx && ./build/test_modctx`: the
   thread test leaked the thread-local FLINT caches; `flint_cleanup()` in the
   worker fixed it; then 0 failed checks and no LeakSanitizer report.
7. Added `tests/ref/adfref/modctx_ref.py` checks, golden dump bodies, the
   `matches_desc` block-zero case, the 2000-block loader round trip and the page
   test.  The final run is 11 tests, 10197 checks, 0 failed checks.
8. Mutation, first run: 71 killed, 32 survived, 47 not compiled.
   Mutation, second run (added control-byte, overflow, page, many-block tests):
   82 killed, 20 survived, 48 not compiled.
   Mutation, third run (restructured `adf_dump_h`, `_fmpz_vec_init/clear`,
   `dump_str` length check): 82 killed, 13 survived, 53 not compiled.
   Mutation, fourth run: 82 killed, 11 survived, 48 not compiled.
   Mutation, fifth run (unconditional program init/clear, page cases):
   79 killed, 7 survived, 58 not compiled, 1 timed out.
   Mutation, final run with `tools/mutate/equivalent.txt`:
   81 killed, 0 survived, 58 not compiled, 1 timed out, 10 excused.