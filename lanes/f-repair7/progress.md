# f-repair7: running notes

Lane: close F1 to F4 of docs/reviews/f1/review-gfunc.md (lane f-slice10, `src/gfunc.c`, `tests/test_gfunc.c`).
Owned: `src/gfunc.c`, `tests/test_gfunc.c`, `docs/api-1f8.md` (inside G1, G4, G6), `lanes/f-repair7/`.

## Order of work

1. F1 (inexact idele units at higher degrees) and F2 (degree 1 is an exact copy): tests first, red against the
   reviewer's faults F13 and F11, green against the library. `redgreen.log`.
2. F3 (entry checks in the debug build), F4 (the sentences of `docs/api-1f8.md`).
3. The fault table over all thirteen faults, then the final checks.

## What was done

- F1: `tests/test_gfunc.c`, new `ADF_TEST(idele_root_inexact_units)` and the helper `idele_mk_mod_bits`.
  Five inexact cosets ([1 mod 8], [5 mod 6], [2 mod 3], [1 mod 5], [1 mod (2^200 + 1)]), a positive and a negative
  real ball, the degrees 2, 3, 4, 5, 6, 7, 8, 12, 64, 2^32, WORD_MAX, UWORD_MAX, both signs: 480 calls.
- F2: `tests/test_gfunc.c`, new `ADF_TEST(degree_one_is_an_exact_copy)`: the idele (1, 1, [5 mod 6]) at n = 1 for
  both signs and prec 2 and 64, field by field; the adele ((2^300 + 12345) 2^-300 +- 2^-600 ; 2 + 4 Zhat) at n = 1,
  prec 2, both signs.
- F3: `src/gfunc.c` entry checks: `ADF_INV_RAT(a)` in `adf_rat_root`, `ADF_INV_ADELE(x)` in `adf_adele_root`,
  `ADF_INV_IDELE(x)` in `adf_idele_root` (the macro of `src/invariants.h` does not exist for `adf_idele`, so it is
  defined in `gfunc.c` in the pattern of `src/lball.c:33-46`), `ADF_INV_ADELE_LIM(x, prec)` in the five series
  functions, so that ADF_LIMIT from prec alone stays first (N-D8, as `src/idele.c:258`).
  `tests/test_gfunc.c`, new `ADF_TEST(entry_check_of_the_public_functions)` under `#ifdef ADF_CHECK_INVARIANTS`.
- F4: `docs/api-1f8.md` G1 proof step 2 (`|r| >= 2`, not `r >= 2`), G1 proof step 3 (what `refs/` says about the
  two ends of `2 <= n < bits(f) <= WORD_MAX`, and the source that is missing), G6 (d) and proof step 4 (the prime
  of the search fits a `ulong` below `3.65e17` bits of numerator, the limit, and what the code does beyond it).

## Red and green

`redgreen.log`. New test against the library: 12 tests, 1091373 checks, 0 failed.
New test against F13: 100 failed checks, 1 failed test. Against F11: 12 failed checks, 1 failed test.

## Notes

- The release object code is unchanged: the `.text` of `build/gfunc.o` is byte for byte the `.text` of the object
  built from `git show HEAD:src/gfunc.c`, and the disassembly and the code relocations are identical. Only the
  DWARF sections differ, because the line numbers of `src/gfunc.c` moved (`object-identity.log`).
- The build with `INV=1` needs every file of `src/`, and `src/text.c` (not owned here, lane f-slice11 at the time)
  does not compile under `-DADF_CHECK_INVARIANTS`: two printer checks use `__func__, #x` outside a macro
  (`src/text.c:2622-2623` and `:2942-2943`), so no `INV=1` build of the tree works. `inv-build.sh` builds the
  same archive by hand (all files with the flag except `text.o`, taken from the release build) and its
  `test_gfunc` passes: `13 tests, 1091382 checks, 0 failed`.
- The thirteen planted faults of the review all fail the new test: F11 (12 failed checks) and F13 (100) were
  survivors before, and F06 is now caught by two tests instead of one.
- `make -j2 check-all` in `build/`: last line
  `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`.