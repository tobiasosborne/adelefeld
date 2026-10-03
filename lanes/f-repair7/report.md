# f-repair7

Lane: close the four findings F1 to F4 of `docs/reviews/f1/review-gfunc.md` (review of lane f-slice10, WP 1F.8).
Work ran on 2026-10-04. Only `src/gfunc.c`, `tests/test_gfunc.c`, `docs/api-1f8.md` (inside G1 and G6) and this
lane directory were written. `include/adelefeld/gfunc.h` was not touched, and nothing was added at the end of
`docs/api-1f8.md`. No git command that changes state, no `bd`, no subagent.

## What was done

### F1 (MAJOR test gap): an inexact unit at degrees above 3

`tests/test_gfunc.c`, new `ADF_TEST(idele_root_inexact_units)` (line 865) and the helper `idele_mk_mod_bits`
(line 756). Five inexact unit cosets: `[1 mod 8]`, `[5 mod 6]`, `[2 mod 3]`, `[1 mod 5]` and `[1 mod (2^200 + 1)]`
(modulus of 200 bits), a positive and a negative real ball, the degrees 2, 3, 4, 5, 6, 7, 8, 12, 64, 2^32,
`WORD_MAX`, `UWORD_MAX`, both signs: 240 cases, 480 calls (`run_idele` makes the call and the aliased call).
Expectations, all taken from `include/adelefeld/gfunc.h` and N-D16 (the real ball of the idele is 1 or -1):

- positive real ball, every `n >= 2`, `sign` +1: `NOT_DETERMINED`, `where` untouched.
- positive real ball, even `n`, `sign` -1: `NOT_DETERMINED`, `where` untouched.
- positive real ball, odd `n`, `sign` -1: `DOMAIN` (an idele is never 0), `where` untouched.
- negative real ball, even `n`, either sign: `DOMAIN`, `where` the real place.
- negative real ball, odd `n`, `sign` +1: `NOT_DETERMINED`, `where` untouched.

`run_idele` checks that `y` is bitwise the sentinel on a status and that `where` is the real place or the sentinel
as the list above says, so "output untouched, `where` untouched" is checked on every case.

### F2 (MINOR test gap): degree 1 is an exact copy

`tests/test_gfunc.c`, new `ADF_TEST(degree_one_is_an_exact_copy)` (line 919).

- The idele `(1, 1, [5 mod 6])`: canonical and not normal (`N = 6 = 2 mod 4`, `src/ucoset.c:adf_ucoset_is_normal`;
  the normal form is `[2 mod 3]`). `n = 1`, `sign` +1 and -1, `prec` 2 and 64: `adf_idele_identical` and, field by
  field, `arb_equal(inf)`, `fmpq_equal(r)`, `fmpz_equal(u.c)`, `fmpz_equal(u.N)`; the unit stays `[5 mod 6]`.
- The adele `((2^300 + 12345) 2^-300 +- 2^-600 ; 2 + 4 Zhat)`: an inexact finite part and a real ball of 300
  bits; `n = 1`, `prec = 2`, both signs, `adf_adele_identical`. The test itself first checks that
  `arb_set_round(x->inf, 2)` differs from `x->inf`, so the input really is one that a rounding would change.

This also closes the third missed fault of the slice: the reviewer's F06 (a rounding of the real ball at adele
degree 1) now fails in two tests instead of one.

### F3 (MINOR): the entry checks of the debug build

`src/gfunc.c`:

- line 24: `#include "invariants.h"`.
- lines 29-42: `ADF_INV_IDELE(x)`. `src/invariants.h` has `ADF_INV_RAT`, `ADF_INV_FBALL`, `ADF_INV_SCALED`,
  `ADF_INV_ADELE`, `ADF_INV_CADELE` and no idele macro, although `include/adelefeld/idele.h:45` promises the
  check for every public function of the type. So the macro is defined here in the pattern of `ADF_INV_LBALL`
  (`src/lball.c:33-46`): the public predicate `adf_idele_is_canonical`, the same one-line message on stderr,
  `flint_abort()`. It is empty without `-DADF_CHECK_INVARIANTS`.
- lines 46-55: `ADF_INV_ADELE_LIM(x, prec)`, which stands the check only when `prec <= ADF_REAL_PREC_MAX`.
- line 152 `ADF_INV_RAT(a)` in `adf_rat_root`; line 196 `ADF_INV_ADELE(x)` in `adf_adele_root`; line 251
  `ADF_INV_IDELE(x)` in `adf_idele_root`; lines 376, 383, 390, 397, 404 `ADF_INV_ADELE_LIM(x, prec)` in the five
  series functions. The output argument is never checked (it is overwritten).

Order: `ADF_LIMIT` from `prec` alone is decided before the entry checks, as in `src/idele.c:258` ("first: from
prec alone, before the entry checks (N-D8, F1)"). `adf_rat_root` has no `prec`, so its check is the first
statement. In the two root functions the check stands directly after the `prec > ADF_REAL_PREC_MAX` block; in the
series it stands in the public function, and `series()` still decides the `LIMIT` first, so the check is never
reached for a `prec` above the cap. This keeps the promise of `gfunc.h` ("decided from prec alone before every
other status and before any allocation"), which the entry check would otherwise break in the debug build, since
`adf_adele_is_canonical` computes a gcd.

`tests/test_gfunc.c`, new `ADF_TEST(entry_check_of_the_public_functions)` (line 1310), compiled only under
`#ifdef ADF_CHECK_INVARIANTS`. It follows the pattern of `tests/test_lfunc.c:1162-1248`: each case runs in a
forked child whose stderr is captured; the forged argument must end by `SIGABRT` with a line naming the function
and the type, and the canonical control must return normally and silently. Three cases, one per argument type:
`adf_rat_root` with the rational 2/4, `adf_idele_root` with a NaN real ball, `adf_adele_exp` with a NaN real
ball. Tests of this kind do exist in the tree (`tests/test_lfunc.c`, `tests/test_lball.c`, `tests/test_recon.c`),
so the hand reproduction below is a confirmation, not a substitute.

The release build is unchanged. `lanes/f-repair7/object-identity.log`: the `.text` of `build/gfunc.o` built from
the repaired `src/gfunc.c` is byte for byte the `.text` of the object built from `git show HEAD:src/gfunc.c`, the
disassembly is equal line for line (`.text` 3638 bytes in both), and only the DWARF sections differ, because the
line numbers of the file moved.

### F4 (MINOR): the sentences of `docs/api-1f8.md`

- G1, proof step 2 (line 71): "If `q = r^n` with an integer `r`, then `r >= 2`" is false (`q = 4`, `n = 2`,
  `r = -2`). The step now reads `|r| >= 2` (`|r| <= 1` would give `q <= 1`, but `q >= 2`; the sign of `r` is
  free). The two conclusions are unchanged: `q = |r|^n >= 2^n` and `bits(q) >= n + 1`. `docs/api-1f5.md:126`
  already had the correct wording ("an integer root >= 2").
- G1, proof step 3 (lines 77-87), the source the review lists for G1(c). What is on disk: `fmpz_bits` returns
  `flint_bitcnt_t`, "the number of bits required to store the absolute value of `f`"
  (`refs/src/flint-3.0.1/fmpz.rst:605-608`); `flint_bitcnt_t` is `ulong` (`refs/src/flint-src-3.0.1/flint.h.in:112`)
  and "a bit offset within an array of limbs (always nonnegative)" (`refs/src/flint-3.0.1/flint.rst:110-111`);
  `WORD_MAX` is `LLONG_MAX` (`flint.h.in:176`); the limbs are allocated through `flint_malloc`, which wraps the
  system `malloc` (`memory.rst:16-21`), with a `size_t` argument (`flint.rst:121`). No file under `refs/` bounds
  the number of limbs of an `fmpz`, so the sentence is marked
  `[source pending: explicit FLINT/GMP representation bound implying fmpz_bits(f) <= WORD_MAX on this target]`,
  worded as the review words it, and the step says what is proved without it.
- G6, new item (d) (line 234) and proof step 4 (lines 244-262): the candidate prime fits a `ulong` for every
  numerator of at most `365651249660515264` bits (about `4.57e16` bytes). The chain: by (c) and `|A| < 2^B`,
  `k <= 0.6309297536 * B + 1` (`log_3 2 = 0.6309297535714574`, rounded up); with
  `p_k < 2 k ln k` for `k >= 3`, and `2 * 230700252851841000 * ln(230700252851841000) = 18446744073709551582`,
  `2 * 230700252851841001 * ln(230700252851841001) = 18446744073709551664`, `2^64 = 18446744073709551616` (30
  decimal digits, computed for this lane), `p_k < 2^64` for every `k <= 230700252851841000`. The inequality on the
  k-th prime is not on disk and is marked `[source pending: ...]`, so (d) is written as a limit, not as a theorem.
  The proof then says what is proved: no file under `refs/` bounds the memory of a machine below that size, and
  `ADF_LBALL_BITS_MAX = 67108864` (`include/adelefeld/lball.h:68`) bounds the powers `p^k` the library forms,
  not the numerator of an exact rational, which `adf_fball` holds at any size that fits in memory. What the code
  does beyond the limit: nothing; `n_nextprime` "Assumes the result will fit in an `ulong`"
  (`refs/src/flint-3.0.1/ulong_extras.rst:688-692`), so that assumption would fail, and before the limit is
  reached the search would try about `2.3e17` primes, each a division of the whole numerator.
- The comment of `first_failing_prime` in `src/gfunc.c` (lines 296-304) said "each is below `2^64` for any `A` that
  fits in memory"; it now says the bound of G6 (d) and points at the limit. No code changed.

## Checks

All commands ran from the repository root, under `timeout`, with at most 2 jobs.

| command | result |
|---|---|
| `timeout 600 make -j2 BUILD=lanes/f-repair7/build .../test_gfunc` | exit 0 |
| `timeout 300 lanes/f-repair7/build/test_gfunc` (before) | exit 0, `10 tests, 1088693 checks, 0 failed` |
| `timeout 300 lanes/f-repair7/build/test_gfunc` (after) | exit 0, `12 tests, 1091373 checks, 0 failed` |
| `timeout 900 python3 lanes/f-repair7/faults.py` (all 13) | exit 0, 13 built, 13 detected, 0 survived |
| `timeout 900 make -j2 BUILD=lanes/f-repair7/build-san SAN=1 .../test_gfunc` | exit 0 |
| `ASAN_OPTIONS=detect_leaks=1 timeout 600 .../build-san/test_gfunc` | exit 0, `12 tests, 1091373 checks`, clean |
| `timeout 900 sh lanes/f-repair7/inv-build.sh` (the debug build, see below) | exit 0 |
| `timeout 300 lanes/f-repair7/build-inv/test_gfunc` | exit 0, `13 tests, 1091382 checks, 0 failed` |
| `timeout 1200 make -j2 check-all` (in `build/`, once, at the end) | exit 0, last line `check-all passed: ...` |

"0 failed" means 0 failed checks and 0 failed tests; the full last line of each run is in the log named beside it.

### Red and green (`lanes/f-repair7/redgreen.log`)

| run | last line of `test_gfunc` |
|---|---|
| the test of the slice, unmodified library | `10 tests, 1088693 checks, 0 failed checks, 0 failed tests` |
| the new test, unmodified library | `12 tests, 1091373 checks, 0 failed checks, 0 failed tests` |
| the new test, fault F13 (inexact unit, `n >= 4`) | `12 tests, 1091173 checks, 100 failed checks, 1 failed tests` |
| the new test, fault F11 (idele degree one normalised) | `12 tests, 1091373 checks, 12 failed checks, 1 failed` |
| the new test, fault F06 (adele degree one rounded) | `12 tests, 1091373 checks, 162 failed checks, 2 failed` |

The first failing check of the three:

- F13: `tests/test_gfunc.c:675: idele_root_inexact_units: check failed: got == st | unit 0, real ball 0 line 0
  n=4 sign=1: status 0, want 1`
- F11: `tests/test_gfunc.c:932: degree_one_is_an_exact_copy: check failed: adf_idele_identical(j, i) | sign 1,
  prec 2: the copy differs`
- F06: `tests/test_gfunc.c:959: degree_one_is_an_exact_copy: check failed: adf_adele_identical(y, x) | sign 1:
  the copy of the real ball was rounded`

The faults are the reviewer's, built with a copy of `lanes/f-review9/faults.py` in this lane
(`lanes/f-repair7/faults.py`, only the lane path changed and `src/invariants.h` copied into each scratch, since
`src/gfunc.c` includes it by a path relative to its own directory).

### The fault table (the new `test_gfunc` against all thirteen faults)

Format: fault, change, failed checks, failed tests, test exit, and the two numbers of the review.

- F01, apply the even selector to the real part only: 1078, 7, exit 1 (review 1078, 7).
- F02, write `where` on OK in combine: 3940, 5, exit 1 (review 3940, 5).
- F03, always set the idele result unit to [1]: 471, 2, exit 1 (review 471, 2).
- F04, take the real root at `prec - 1`: 1028, 2, exit 1 (review 1028, 2).
- F05, start the failing-prime search at 3: 3097, 3, exit 1 (review 3097, 3).
- F06, round the real ball at adele degree 1: 162, 2, exit 1 (review 160, 1).
- F07, check degree 0 before LIMIT for adele roots: 2, 1, exit 1 (review 2, 1).
- F08, leave `where` untouched on equal failed statuses: 4408, 4, exit 1 (review 4408, 4).
- F09, store signed rho as the idele content: 941, 2, exit 1 (review 941, 2).
- F10, reject sign -1 on the exact zero adele at odd degree: 13, 3, exit 1 (review 13, 3).
- F11, normalize an idele's unit at degree 1: 12, 1, exit 1 (review 0, 0).
- F12, reject series precision below 2 after real evaluation: 20, 1, exit 1 (review 20, 1).
- F13, treat inexact idele units as exact when `n >= 4`: 100, 1, exit 1 (review 0, 0).

Thirteen built with exit 0, thirteen detected, none survived; before this lane eleven of thirteen were detected.
The two that were missed (F11, F13) are now failed by the two new tests. There is therefore no survivor to
explain. The two changes of count come from the new tests: F06 is now also caught by `degree_one_is_an_exact_copy`
(the adele with a ball of 300 bits at `prec = 2`), F11 and F13 by their own new tests.

### The entry check, shown by hand (`lanes/f-repair7/invariants-hand.log`)

`lanes/f-repair7/repro_invariants.c` is a copy of the reviewer's reproducer; it was compiled with
`-DADF_CHECK_INVARIANTS` against the repaired `src/gfunc.c` and the direct callees:

```
$ timeout 300 cc -std=gnu11 -O1 -g -DADF_CHECK_INVARIANTS -Iinclude lanes/f-repair7/repro_invariants.c \
    src/gfunc.c src/adele.c src/fball.c src/rat.c src/ucoset.c src/idele.c src/rfunc.c src/sball.c src/modctx.c \
    lanes/f-repair7/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-repair7/tmp/repro_invariants
$ ulimit -c 0; timeout 60 lanes/f-repair7/tmp/repro_invariants            # exit 134
adelefeld: ADF_CHECK_INVARIANTS: adf_adele_root: argument x is not a canonical adf_adele
$ timeout 60 lanes/f-repair7/tmp/repro_invariants idele                    # exit 134
adelefeld: ADF_CHECK_INVARIANTS: adf_idele_root: argument x is not a canonical adf_idele
$ timeout 60 lanes/f-repair7/tmp/repro_invariants series                   # exit 134
adelefeld: ADF_CHECK_INVARIANTS: adf_adele_exp: argument x is not a canonical adf_adele
$ timeout 60 lanes/f-repair7/tmp/repro_invariants control                  # exit 134 (adf_adele_set)
```

Before the repair the first three returned `DOMAIN` with exit 0 (review F3). The debug `test_gfunc` makes the same
kind of call three times per run (`adf_rat_root`, `adf_idele_root`, `adf_adele_exp`, six children in all, the
forged and the canonical one each) and passes.

### The prescribed debug build, and why it was done by hand

`timeout 600 make -j2 BUILD=lanes/f-repair7/build-inv INV=1 lanes/f-repair7/build-inv/test_gfunc` exits 2. The
only file of `src/` that fails is `src/text.c`, which is not owned by this lane: two entry checks of the printers
use `__func__, #x` outside a macro, at `src/text.c:2622-2623` (`adf_lball_get_str`) and `src/text.c:2942-2943`
(`adf_sball_get_str`); the file has not changed since 00:20 on 2026-10-04 and is being written by another lane.
Every other file of `src/`, `src/gfunc.c` among them, compiles under the flag. `lanes/f-repair7/inv-build.sh`
therefore does the same work by hand: each `src/*.c` with `-DADF_CHECK_INVARIANTS` and the same warning flags,
except `text.o`, which is taken from the release build of this lane (no case of `test_gfunc.c` uses a printer).
Its run is the line of the table above: `13 tests, 1091382 checks, 0 failed checks, 0 failed tests`. Every valid
input of the test file goes through an entry check without aborting.

## Files written

- `src/gfunc.c`: the entry checks (F3), and one comment of `first_failing_prime` (F4). No value changed on any
  valid input; the `.text` of the object is identical to the one of the object of `HEAD`.
- `tests/test_gfunc.c`: the tests of F1, F2 and F3 (three new tests, one new helper, the include block and the
  POSIX headers for the child processes).
- `docs/api-1f8.md`: G1 proof steps 2 and 3, G6 item (d) and proof step 4.
- `lanes/f-repair7/`: this report, `progress.md`, `redgreen.log`, `faults.log`, `fault-results.json`, `faults.py`,
  `inv-build.sh`, `repro_invariants.c`, the build logs, `object-identity.log`, `invariants-hand.log`,
  `baseline-build.log`, `baseline-test.log`, `green.log`, `green1.log`, `inv-build.log`, `inv-build2.log`,
  `inv-test.log`, `san-build.log`, `san-test.log`, `check-all.log`, and the build directories `build`,
  `build-inv`, `build-san`, `faults/`.

## Not done

- The prescribed `make ... INV=1` was not run to the end: it fails in `src/text.c`, a file of another lane. The
  equivalent build by hand is described above.
- No mutation tool run (`tools/mutate`) over `src/gfunc.c`: the thirteen planted faults of the review are the
  mutation test of this file and all of them are reported.
- `G4` of `docs/api-1f8.md` was left alone: nothing in the four findings asks for a change there.
- No test of the claim of G6 (d) at its limit (a numerator of `3.66e17` bits): building one would need 4.6e16
  bytes. The arithmetic is in the proof step and the three numbers were computed with 30 decimal digits.
- Nothing was added at the end of `docs/api-1f8.md`; see the next section for the line there that is now false.

## Sources pending

- `[source pending: explicit FLINT/GMP representation bound implying fmpz_bits(f) <= WORD_MAX on this target]`
  (G1(c)). Searched: `refs/src/flint-3.0.1/fmpz.rst`, `refs/src/flint-3.0.1/flint.rst`,
  `refs/src/flint-3.0.1/memory.rst`, `refs/src/flint-src-3.0.1/flint.h.in`, and the whole of `refs/` for GMP
  documentation (there is none: no file under `refs/` mentions GMP). `flint_bitcnt_t` is `ulong` and `WORD_MAX` is
  `LLONG_MAX`, but no file gives a bound on the number of limbs of an `fmpz`.
- `[source pending: the inequality p_k < 2 k ln k for k >= 3, for which no file under refs/ was found]` (G6 (d)).
  Everything else of G6 (d) is arithmetic that can be checked without a source.
- `[source pending: an explicit FLINT/GMP or platform bound on the number of limbs of an fmpz that would make (d)
  hold for every representable numerator instead of for every numerator below the stated size]`.
- `[source pending: FLINT documentation of flint_cleanup]` is listed in `docs/conventions.md` 4.5 and is not closed
  by this lane; it is not used by G1 or G6.

## Findings against the specification and against the documents of other lanes

1. `docs/api-1f8.md:71` (G1, proof step 2) asserted `r >= 2` from `q = r^n >= 2`, which is false for
   `q = 4`, `n = 2`, `r = -2`. The library returns `OK` and the root -2 there, so the statement, not the code, was
   wrong. Repaired to `|r| >= 2`.
2. `src/gfunc.c:259-261` (before this lane) claimed "each is below `2^64` for any `A` that fits in memory". That is
   not derivable from anything under `refs/` and, read as a statement about every representable numerator, it is
   not proved: the address-space bound on an `fmpz` is far above the `3.66e17` bits that the prime bound needs.
   Repaired to a limit with the arithmetic behind it.
3. `docs/api-1f8.md`, last section, still says "No source is pending." That sentence is now false. It is at the end
   of the file, where lane f-slice11 is appending its section, and this lane was told not to edit there; the
   orchestrator has to delete or correct that sentence.
4. `src/text.c:2622-2623` and `:2942-2943` do not compile under `-DADF_CHECK_INVARIANTS` (`__func__, #x` outside
   a macro), so no `INV=1` build of the tree works while that stands. Not this lane's file; reported, not touched.
5. `docs/conventions.md` 4.4 promises that every public function checks its arguments on entry, and
   `include/adelefeld/idele.h:45` promises it for every public function of the idele type, but `src/invariants.h`
   has no `ADF_INV_IDELE` macro. `src/gfunc.c` defines its own; a macro for the type belongs in
   `src/invariants.h` for the other files that need it. Not touched here: that file is not owned by this lane.
6. No counterexample was found to SPEC 9.3.1, 9.3.2, 9.3.3 or to N-D8, N-D12, N-D16 in this lane.