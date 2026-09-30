# Lane i-repair1: result

Worktree at 09767d5. Run 03:10 to 03:36 (2026-09-30). No state-changing git command, no `bd`. Only files under
"You own" were written (see the list at the end). No compiled binary is in the lane directory
(`lanes/i-repair1/` holds `brief.md`, `progress.md`, `redgreen.log`, this file).

## Part A: the review findings

All five findings were seen to reproduce BEFORE any change (`lanes/i-repair1/redgreen.log`, top): the reviewer's
`limit_alloc` on the INV=1 archive printed `allocator_calls=14 requested_bytes=135160` for all four functions
(0 calls on the default archive); `findings` printed the F2 to F5 lines (class T = 1 +/- 1/2 for exact 5 at prec 2;
exact 3, k = -1, prec 2 gives 1/4 +/- 1/8; the "product of 1 copy" of the exact rational 2 has real part 2 and
content 2; simple radius 3, smallest radius 6).

**F1 (MAJOR), repaired in code.** The test of `prec` against `ADF_IDELE_PREC_MAX` is now the first statement of
every function that takes a `prec`, before `ID_INV`, `IC_INV`, `ADF_INV_RAT`, `ADF_INV_ADELE`, `MP_INV_IDELE`:
`src/idele.c` (`set_rat`, `mul`, `inv`, `mul_rat`, `norm`), `src/idclass.c` (`set_idele`, `mul`, `inv`),
`src/idpow.c` (public `adf_idele_pow`, `adf_idele_pow_tight`, `adf_idclass_pow`, `adf_idclass_pow_tight`),
`src/idmap.c` (`adf_adele_div_idele`). In `idpow.c` the content limit (`|k| (bits(n) + bits(d)) >
ADF_IDELE_POW_BITS_MAX`, the second sentence of the header "both decided before any allocation") also moved in front
of the entry check, since it too would otherwise allocate under INV=1; `pow_bits_exceeded` now returns 0 for
`b == 0` (a forged 0/0 content), so that the debug build aborts in its own entry check instead of dividing by zero.
`include/adelefeld/idele.h` says that the test of `prec` is the first thing a function does, before the entry check,
and that the rule holds for `idclass.h`, `idpow.h`, `idmap.h`. Tests, in the INV=1 build red first:
`LIMIT_before_any_allocation_of_the_idele_functions` (norm, mul, inv, mul_rat, set_rat) in `tests/test_idele_maps.c`;
`..._of_the_class_functions` in `tests/test_idclass.c`; `..._of_the_power_functions` in `tests/test_idpow.c`;
`..._of_the_division` in `tests/test_idmap.c`. They count FLINT allocator calls with the reviewer's hooks
(`memory.rst:16-21`) on the reviewer's input (content (2^4096+3)/(2^2048+7), unit [1 mod 2^2048+1]), at
`ADF_IDELE_PREC_MAX + 1` and `WORD_MAX`. Red (INV=1, 03:13:44): test_idele_maps 4 failed checks, test_idclass 1,
test_idpow 4, test_idmap 2, each "14 allocator calls" (set_rat and the class mul/inv did not allocate before: their
entry checks are cheap for the input used; they are reordered all the same). Green: 0 failed checks in the four.
Reviewer's `limit_alloc` after: `allocator_calls=0 requested_bytes=0` for all four functions on BOTH archives.
Not done: only functions that take a `prec` in these four headers were reordered; other headers were not touched.

**F2 (MINOR), text.** `idclass.h`: the class of a rational is the exact `<1 ; [1]>` when the real ball of q is
exact AND q has at most `p = max(prec, 2)` bits, `prec` of the class call (the condition of G.5). `idele.h`
(norm): I chose the SECOND option, the sentence says what the code does. Why: the first option (round once, on the
exact end points `|m| - rho`, `|m| + rho`) is what row i2-2 of `docs/api-2.md` (line 341) rejects: the difference of
two numbers of distant exponents can need unbounded memory (`arf.rst:103-106`), and the ends of E1 are the input
of Statement F and of kernel B (E5, B1 decided on the rounded ends, i-review1 checked that against 543000 cases);
a change to one rounding would change the status (B1) of inputs and the ends that
`tests/ref/vectors/i-slice2/` fix (I did not try it: the design contradicts the kernel, so I did not implement it).
The sentence now says: the ends of the real ball are rounded to p bits (E1), then once more after the exact scale;
t is the exact 1 for a rational q when the real ball is exact and q has at most p bits, p of this call. Tests (they
pass on the old code, they pin true statements): `norm_of_an_exact_rational_is_exact_only_when_it_fits_the_norm_
precision` (exact 5: p = 2 gives a ball that contains 1 and is not exact; p = 3 and 64 give the exact 1) in
`tests/test_idele_maps.c`; `class_of_an_exact_rational_is_exact_only_when_it_fits_the_class_precision` in
`tests/test_idclass.c` (also the unit [1]).

**F3 (MINOR), text.** `idpow.h` now says: for k > 0, X exact and |m|^k of at most p bits, Z is exact (K.4); for
k < 0 the promise is not made (1/|m|^|k| is binary only when |m| is a power of 2; exact 3, k = -1, p = 2 gives
1/4 +/- 1/8, a ball around 1/3). Test `exact_power_only_for_positive_exponents` in `tests/test_idpow.c` (passes on
old code, pins the statement; also k = 1 exact for the default and tight power).

**F4 (MINOR), text.** `docs/api-2.md` Statement L.5 (section 3, lines 503 to 513): the copies are of `x` for
k > 0 and of the inverse idele for k < 0 (content `1/r`, unit `c'^(-1) U(N')`), giving content `r^k` and unit
`c'^k U(N')`; the counterexample of the reviewer (exact 2, k = -1) is in the text; for k = 0 the empty product is the
exact idele 1, and the formula `c'^0 U(N')` is stated NOT to hold (result unit is [1]). The tag
"[Correction of lane i-repair1, finding F4]" stands in the text.

**F5 (MINOR), text.** `docs/api-2.md` Statement M.2: equal when N'' is even; when N'' is odd the smallest hull has
modulus 2 N'', so the simple hull has HALF the radius and is the coarser set; the example 2 + 3 Zhat against
5 + 6 Zhat is in the text. `docs/conventions.md` 5.7 (idele to adele) has the same direction. `idmap.h` already had
it right ("twice as coarse") and was not edited.

## Part B: the pass over the documents

1. `docs/conventions.md` 3.2: the row "Unit coset, idele, idele class arithmetic" (line 211) gains `LIMIT` (prec
   above `ADF_IDELE_PREC_MAX`, first; the content power limit of `adf_idele_pow`), `DOMAIN` (`set_parts`, valuation
   at the archimedean place), and names `NOT_UNIT` for `set_rat`, `mul_rat` with q = 0. New rows, each read from its
   header: `adf_lball` constructors and accessors (`lball.h`); `adf_lball` arithmetic (`neg`, `add`, `sub`, `mul`,
   `inv`, `div`); `adf_lball` valuation, absolute value, decomposition; `adf_lball_pow_si`; `adf_sball` constructors,
   projection and accessors; `adf_sball` operations; series at a prime (`lfunc.h`); real functions on `arb`;
   functions of `adf_sball` at one place. Table rows are single lines and are longer than 116 characters, as the
   existing rows of the table are; all my other lines are at most 116.
2. `docs/conventions.md` 5.7: the accessors are `adf_idclass_get_t`, `adf_idclass_get_unit` (and
   `adf_idele_get_unit`); 5.6 "Power": the exponent `k` is an `slong`; 5.7 "Idele to adele": the simple ball is
   `r c'' + r N'' Zhat`, formed from the normal form, and the radius relation (see F5).
3. `docs/conventions.md` 2.2: the example uses `adf_sball_project` and `adf_sball_add` (real signatures), and says a
   set of places is an array of `adf_place_t` and a length `n` (`slong`), no `adf_places_t` type. 7: the last bullet
   of the section is replaced in the same sense. Change log entry at the head (form of the other entries).
4. `docs/PLAN.md`: row 2.1 "`k = 1, -1` give `M_k = Nbar`, the normal modulus (`proofs/ideles.md` P13.4)"; row 1F.4
   "the library's own series on `fmpz` modulo a working power of `p` (N-D9, `SPEC.md` 15.4; FLINT's `padic_exp`,
   `padic_log` are a second opinion in the tests)"; a paragraph "Change log of the edit of 2026-09-30" before that
   of version 1.3. `docs/SPEC.md` 9.3.2, paragraph "Implementation" (lines 602 to 616): the sentences "our wrapper
   owns ... FLINT is called only for the centre" are replaced by N-D9; a row "1.3, edit" at the head of the change
   log table. No formula changed.
5. `docs/proofs/functions.md`: after the table of "Statement index and proof status", a new subsection "Statements
   proved in the lane documents" with four tables: L0 to L8 (with L4a), S1 to S7, L9 to L13 with S8 and S9, and F1 to
   F7 of `docs/api-1f4.md` (the file has F1 to F7, no F8); each row has the line where proved (for example
   `api-1f.md:75`) and its status from `docs/reviews/f1/` (f-review1, f-review2, f-review3), or "not reviewed"
   (L9 to L13, S8, S9: no review covers them). The one open item, `[source pending: a proof that log and Log take no
   nonzero rational value ...]`, is carried over from `api-1f4.md`. `docs/proofs/ideles.md`: `ideles.md` has no
   heading "Statement index and proof status", its table is "Table of statements"; the new subsection "Statements
   proved in `docs/api-2.md`" sits after that table and has Statements A to O with the line, and the status from
   `lanes/i-review1/result.md` and `docs/reviews/m2/review-slices-2-3.md`. Only index material was added to either
   file.
6. `docs/PLAN.md` section 6: "Status of milestone 1F", "Status of milestone 2" and "Status of milestone S on
   2026-09-30", in the form of the table for milestone 0 (WP, State, Record), from `git log` and the result files.
   1F: 1F.1 to 1F.4 done (1F.2 only its first slice: no `sinh`, `cosh`, complex, special functions), 1F.5, 1F.7, 1F.8,
   1F.9 not started, 1F.6 partly (`adf_lball_pow_si`); the cost finding of f-review3 is open (`f-slice5` has a brief
   and no result). 2: 2.1 to 2.4 done, reviewed and repaired; left: text forms (in work in `t-slice1`) and dump forms.
   S: S.3, S.1, S.2 and the driver commands done; left: a long differential run of the route above the evaluation
   bound (from HANDOFF 2026-09-29 21:10), and the cost of `adf_roots_real` for a dense polynomial. I did not read the
   tracker (`bd` is forbidden), so no bead is named as closed.

## Checks (each in the foreground, under `timeout 900`; the last line)

- `make clean && make -j2 check-all`: `check-all passed: make check, driver, exports, julia, mutate-selftest,
  memcheck-selftest` (03:27).
- `make clean && make -j2 check SAN=1`: `check passed: all 70 test programs` (03:31).
- `make clean && make -j2 check CC=clang`: `check passed: all 70 test programs` (03:32).
- `make clean && make -j2 check INV=1`: `check passed: all 70 test programs` (03:35).
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`.
- The reviewer's programs again (`lanes/i-repair1/redgreen.log`, bottom): `limit_alloc` on the INV=1 archive and on
  the default archive: `status=LIMIT allocator_calls=0 requested_bytes=0` for all four functions. `findings` prints
  the same F2 to F5 lines as before, as it must: it reports the behaviour of the code, which is unchanged (the code
  was right, the sentences were wrong).
- Line width: every added line of prose is at most 116 characters (checked over the diff); table rows are not.

## What is not done

- No mutation run (the brief: COMMON-C rule 5 does not hold); the new tests were seen red only for F1 (the INV=1
  allocation counts). The F2, F3 tests pass on the old code by design.
- The F2 option "round once" was not implemented or tried; the reason is above.
- Julia and driver were not changed. `docs/api-2.md` section 4 (another lane) was not touched.
- The row for adele to idele in conventions 3.2 still lists `NOT_DETERMINED`, which `adf_idele_set_adele` never
  returns (a superset is allowed by 3.2); the brief did not name it and I left it.
- The statuses in the milestone tables come from the log and the result files, not from a run of the tracker.
- `tools/mutate` was not run; `bench` was not run.

## Files written

`src/idele.c`, `src/idclass.c`, `src/idpow.c`, `src/idmap.c`; `include/adelefeld/idele.h`, `idclass.h`,
`idpow.h` (sentences and order of the checks only); `tests/test_idele_maps.c`, `tests/test_idclass.c`,
`tests/test_idpow.c`, `tests/test_idmap.c` (additions at the end); `docs/api-2.md` (L.5 and M.2, section 3),
`docs/conventions.md`, `docs/PLAN.md`, `docs/SPEC.md` (9.3.2 and its change log), `docs/proofs/functions.md` and
`docs/proofs/ideles.md` (the index tables), `lanes/i-repair1/progress.md`, `redgreen.log`, `result.md`.
