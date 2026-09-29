# Result of lane s2-slice4: roots modulo p for every prime of a place (issue adf-e0n)

## What was done

`adf_roots_padic` and `adf_roots_padic_partial` accept every prime of a place (2 <= p < 2^64, decision S-D10);
the TEMPORARY `ADF_UNSUPPORTED` above 2^20 is gone, and so is the refusal of `adf_rootlist_verify_complete`
above that bound. The roots modulo p of each class polynomial h of Algorithm P are found:

- for p <= `ADF_ROOTS_P_EVAL_MAX` = 128 by evaluation at every residue (solvers P3.7(1), unchanged);
- above it by the degree (P3.7(2)): t = X^p mod h (`nmod_poly_powmod_ui_binexp`), d = gcd(h, t - X)
  (`nmod_poly_gcd`), candidates from `nmod_poly_roots(d, 0)`, each written as -q0/q1 of a linear factor.
  The list is accepted only if every candidate is in [0, p), a root of h by evaluation, the candidates are
  distinct, and their number is deg d. Otherwise the function aborts (S-D20). Justification in
  `src/roots.c` at `modp_roots_gcd`: a refused list is a defect of FLINT or of the code, not a property of
  the input; NOT_DETERMINED would say that a larger depth may succeed, which is false; evaluation at every
  residue is no fallback above the bound.
- h = 0 modulo p cannot occur (L3.1(1), P3.4: the content at p is removed); `adf_roots_modp` aborts on it.

Hidden functions (declared in the test as the other hidden functions): `adf_roots_modp(roots, h, route)`
(route 0 auto, 1 evaluation, 2 degree) and `adf_roots_modp_check(h, cand, m)`.

The bound was measured (item 3) and set to 2^7 = 128 (see the table below). The header sentences about the
temporary status are rewritten (the prime, the macro, the route, the statuses, the cost, verify_complete).

## Files written

- `src/roots.c`: the section "the roots modulo p" (modp_deg_d, modp_check, adf_roots_modp_check,
  modp_roots_eval, modp_roots_gcd, adf_roots_modp); `padic_search` takes the roots of each class from
  `adf_roots_modp`; UNSUPPORTED removed from `adf_roots_padic_core`; the bound removed from
  `adf_rootlist_verify_complete`; comments of the slice 2 section updated.
- `include/adelefeld/roots.h`: the sentences about the temporary status; `ADF_ROOTS_P_EVAL_MAX` 1048576 -> 128
  with the measurement; the route of P3.7(2), its trust base and its abort; the cost; verify_complete.
- `tests/test_roots_bigp.c` (new, 5 tests).
- `tests/test_roots_padic.c`: only the three checks that expected UNSUPPORTED and the pinned value of the bound.
  `tests/test_roots_forged.c`: no check there expects UNSUPPORTED; unchanged.
- `tests/fuzz/diff_roots_padic.py`: primes 131 to 1031 against the reference, and primes of 21 to 64 bits against
  a planted oracle.
- `bench/bench_roots_modp.c` (new).
- `refs/fetch_sources.sh` (list FLINTSRC_MODP), `refs/manifest.sha256` (7 lines added, nothing else changed:
  the script rewrote the manifests with files other lanes had fetched into the shared `refs/src`; I reset both
  manifests and added only my lines; `sha256sum -c manifest.sha256`: 0 failures), `docs/sources.md` (one row in
  table 1, nine rows in table 3 S.2, item 5 under "Sources pending for milestone S").
- `lanes/s2-slice4/`: brief.md (given), progress.md, redgreen.log, mutants.py, mutants.log,
  bench_roots_modp_2026-09-29T144527Z.txt (the bench result file, moved here from bench/results/),
  checkall.log, check_san.log, check_clang.log, result.md.

## Sources fetched (refs/fetch_sources.sh, into the shared refs/src)

`flint-src-3.0.1/nmod_poly_factor/roots.c`, `nmod_poly/find_distinct_nonzero_roots.c`,
`nmod_poly/powmod_ui_binexp.c`, `nmod_poly/powmod_ui_binexp_preinv.c`, `nmod_poly/gcd.c`,
`nmod_poly/evaluate_nmod.c`, `flint.h.in` (tag v3.0.1). What they say (rows of docs/sources.md, every quote
checked line by line against the file by a script, 0 mismatches):
- `nmod_poly_roots(r, f, 0)` requires nothing of f but f != 0: it makes f monic itself (roots.c:194 to 197), does
  not need f squarefree or split (it takes the gcds with x^((p-1)/2) -+ 1 first, 96 to 102). The prime modulus is
  assumed and checked only by `FLINT_ASSERT` (35, 155), i.e. only in a debug build of FLINT.
- f = 0: `flint_throw` (169 to 173). deg f = 0: no factor (157, 159). deg f = 1: f made monic, exponent 1.
- Randomised: Rabin's Las Vegas method (17 to 19); `_nmod_poly_split_rabin` repeats a random split until it is
  proper (find_distinct_nonzero_roots.c:29 to 43). It never reports a failure; its running time is an expected
  time. The generator is `flint_randinit` in every call (roots.c:177), which sets two fixed words
  (flint.h.in:245 to 250): the result is deterministic.
- p < 10: evaluation at every residue (roots.c:38 to 55).
- `nmod_poly_powmod_ui_binexp`: modulus of length 0 aborts, length 1 gives 0, a base not shorter than the modulus
  is reduced first (powmod_ui_binexp.c:67 to 88). `nmod_poly_gcd(A, 0)` is A made monic (gcd.c:44 to 47).

Pending: the `fmpz_mod_poly` sources of item 4 of "Sources pending for milestone S" stay pending; the library
does not call those routines (a prime of a place is one word). `docs/proofs/solvers.md:1308` still carries
`[source pending: ... nmod_poly.rst (for nmod_poly_powmod, nmod_poly_gcd and the root finding)]`; for the one-word
modulus it is now settled by the C sources above (not my file; see findings).

## Checks

Red and green (lanes/s2-slice4/redgreen.log):
- Red 1: `make -j2 -s build/test_roots_bigp`: link error, undefined `adf_roots_modp`, `adf_roots_modp_check`.
- Red 2: routes implemented, UNSUPPORTED kept: `timeout 300 ./build/test_roots_bigp`: 5 tests, 905 checks,
  98 failed checks, 2 failed tests (planted_roots_above_the_bound, statuses_above_the_bound: status 8).
- Green 1: UNSUPPORTED removed, the search uses the route: 5 tests, 825 checks, 0 failed checks, 0.9 s.
- Red 3: test_roots_padic unchanged against green 1: FAIL at lines 1309 to 1311 (want 8, got 0, 10, 0).
- Green 2: the checks changed: `timeout 300 ./build/test_roots_padic`: 8 tests, 55135 checks, 0 failed.

What `tests/test_roots_bigp.c` checks (the runner counts an ADF_CHECK in a helper only when it fails, so the
count of checks does not count the cases):
1. both_routes_give_the_same_roots_below_the_bound: at p = 2, 3, 5, 7, 11, 13, 101, 257 and the three largest
   primes <= 128 (127, 113, 109): per prime 40 random polynomials of degree 0 to 12, 40 planted products (the
   expected list is the set of planted residues), 10 without root (products of X^2 - c, c a non-residue by
   Euler's criterion), 10 of degree 0 and 10 of degree 1 (expected root -c), 3 random of degree 101 to 150,
   3 with 120 planted roots and 3 of those times a cubic, 2 of the form c (X^p - X) + p H (expected: every
   residue). A case fails if the two routes give different lists, a list not strictly increasing, a list
   different from the expected one, or a list that `adf_roots_modp_check` refuses. 121 cases per prime.
2. planted_roots_above_the_bound_give_exactly_the_planted_list: p = 2^20 + 7 (21 bits), 2^32 - 5, the largest
   primes below 2^48 and 2^63 (63 bits), 2^64 - 59; per prime 12 products of 1 to 8 linear factors with roots of
   up to 100 bits of distinct residues (some with a repeated root, a leading coefficient 3, a quadratic
   without root), one with 40 roots and a quadratic, two with a pair r1 = r0 + p^t u (t = 1, 2), one with the
   roots 0 (twice) and -1 - p. A case fails unless both functions return OK, complete, the oracle g, exactly the
   oracle list (r mod p^K, K, s) in order, every planted root in exactly one ball, and is_canonical,
   verify_entries and verify_complete return 1. 16 cases per prime, 80 in all.
3. statuses_above_the_bound: DOMAIN, LIMIT, OK and verify_complete at 2^20 + 7, 2^32 - 5, 2^64 - 59; the limit of
   the precision at 64 bits (131072 OK, 131073 LIMIT).
4. a_wrong_candidate_list_is_refused: at p = 3, 101, 65521, 2^20 + 7, 2^32 - 5, 2^64 - 59, 20 planted
   polynomials each: the true list and its reverse accepted; refused: a non-root added, a root removed, a root
   replaced by a non-root, a root repeated in place of another, a root plus p; degree 0 with the empty list
   accepted and with one entry refused; X + 1 with the empty list refused.
5. the_zero_polynomial_modulo_p_aborts: in a child process, `adf_roots_modp` on 0 at p = 7 and 2^32 - 5, routes
   0, 1, 2: SIGABRT each time (6 cases).

Item 5, the tests bite (`timeout 900 python3 lanes/s2-slice4/mutants.py`, scratch copy build/mut-s2-slice4,
test programs under `timeout 120` and `ulimit -v 4000000`; log lanes/s2-slice4/mutants.log): 4 of 4 killed.
- test by evaluation removed: test_roots_bigp 101 failed checks (a_wrong_candidate_list_is_refused);
  test_roots_padic passes (it does not reach the degree route with a wrong list).
- comparison with deg d removed: test_roots_bigp 107 failed checks (the same test).
- X^p replaced by X^(p-1): test_roots_bigp aborts (exit 134, "the roots modulo 2 of FLINT are refused
  (1 candidates, deg gcd(h, X^p - X) = 11)"); test_roots_padic 6 failed checks in 2 tests.
- reduction modulo h dropped (`nmod_poly_pow(t, x, p)`): both programs abort, "Unable to allocate memory
  (34359738336)" and "(18446744073709551152)". This mutant is killed by a failed allocation at primes of 32 bits
  and more, not by a wrong answer: below that size X^p is formed in full and the result is right.

Fuzz (item 6): `sh tests/test_exports.sh` then `timeout 200 python3 tests/fuzz/diff_roots_padic.py --seconds 180
--seed 1`: 166075 calls in 180 s; statuses OK 142776, NOT_DETERMINED 11999, DOMAIN 11300; at p <= 7: 37652; at
131 <= p <= 1031 (the degree route against the reference): 33207; at primes of 21 to 64 bits (planted oracle):
49913; lists with classes 11999, with s > 0 44840; 0 disagreements. A smoke test of 180 s, not a long fuzz run.
A first run of 20 s found a defect of my own script (the content step multiplied every coefficient by its own
random factor, so the planted roots were not roots); repaired before the run above.

Benchmark (item 3): `make -C bench -s bench_roots_modp; timeout 560 ./bench/bench_roots_modp --run --trials 5`,
run 2026-09-29T144527Z, i7-1365U, pinned to cpu 2, -O2 -g, FLINT 3.0.1. The machine was shared with other lanes
(load average 3.6 to 4.4 at the start). Time of one call of `adf_roots_modp` in microseconds, median of 5 trials
of a batch of 16 polynomials (min and max in the result file); "split": deg distinct roots; ratio = route 1 /
route 2 (above 1: the degree route is faster); gm = geometric mean of the ratio over the 6 cases.

| p | deg 2 random | deg 2 split | deg 8 random | deg 8 split | deg 32 random | deg 32 split | gm |
|---|---|---|---|---|---|---|---|
| 61 | 0.81 / 1.17 | 0.88 / 2.33 | 3.47 / 2.18 | 3.39 / 11.33 | 16.08 / 8.83 | 16.17 / 83.63 | 0.593 |
| 127 | 1.56 / 1.33 | 1.65 / 3.01 | 11.25 / 5.31 | 7.72 / 15.38 | 47.43 / 19.48 | 48.46 / 117.40 | 0.939 |
| 251 | 3.72 / 1.64 | 3.64 / 2.57 | 16.70 / 5.12 | 18.37 / 16.72 | 73.02 / 16.80 | 95.27 / 132.31 | 1.817 |
| 509 | 6.39 / 1.62 | 7.47 / 3.13 | 29.15 / 4.56 | 29.31 / 17.36 | 124.84 / 15.24 | 129.55 / 108.63 | 3.160 |
| 1021 | 11.12 / 2.35 | 13.37 / 4.33 | 89.52 / 5.31 | 68.13 / 25.30 | 349.39 / 22.27 | 350.19 / 141.64 | 5.433 |
| 4093 | 57.75 / 2.07 | 53.06 / 4.10 | 252.49 / 6.62 | 272.72 / 30.36 | 1535.16 / 34.72 | 1537.98 / 202.52 | 18.606 |
| 65521 | 859.31 / 2.95 | 963.40 / 5.19 | 3751.98 / 7.31 | 3425.16 / 31.48 | 17487.11 / 51.41 | 22310.44 / 229.73 | 215.452 |
| 1048573 | 14597.78 / 5.46 | 11487.44 / 5.44 | 64241.81 / 9.65 | 55583.97 / 35.06 | 284849.28 / 46.95 | 247899.60 / 282.35 | 2611.924 |

(each cell: route 1 / route 2; the rows 13, 16381, 262139 are in the file.) The crossover of the geometric mean
lies between 127 (0.94) and 251 (1.82): the constant is 2^7 = 128. For the split family of degree 32 alone the
crossover is later (251: 0.72, 509: 1.19); for random polynomials of degree 8 and 32 earlier (61: 1.59, 1.82).
An earlier run (2026-09-29T144136Z, a coarser grid, deleted) gave the same picture.

Full checks:
- `make clean && make check-all`: `make check`: "check passed: all 57 test programs"; driver "27 cases, 100376
  expected lines, all equal (SAN=0)"; exports "passed: 260 of 260 declared functions are exported"; then
  `sh tests/test_julia.sh` FAILED: "Some tests did not pass: 19 passed, 1 failed": tests/julia/roots.jl:188
  expects `roots_padic(c, 1048583, 3, 2)[1] == UNSUPPORTED` and gets 0 (OK). That file is not mine (see
  findings). check-all stops there (last line "make: *** [Makefile:123: check-all] Error 1"). The rest was run by
  hand: the Julia test from a scratch copy build/julia-s2s4 with only line 188 changed to `== OK`: exit 0, 9
  test sets, all pass; `python3 tools/mutate/selftest.py`: "selftest: passed: the weak test leaves a survivor,
  the strong test leaves none, and no process of a mutant is left"; `python3 tools/memcheck/selftest.py`:
  "selftest: passed".
SAN_LINE
CLANG_LINE
HEADERS_LINE

## Not done

- tests/julia/roots.jl:188 still expects UNSUPPORTED (not my file); `make check-all` fails there until the
  orchestrator changes it (for example to `== OK`, which passes in the scratch copy).
- The search itself is never driven with a forged FLINT list (there is no injection point); the refusal is
  tested on `adf_roots_modp_check`, and the abort of the search on a refused list is seen through the mutant
  X^(p-1) (exit 134 with the message of `modp_roots_gcd`).
- No long fuzz run (180 s only, as the brief asks).

## Header findings

- `adf_rootlist_verify_complete` is documented as "A predicate; never aborts on a list that satisfies the pointer
  rule". Its rerun of the search can abort by S-D20 on a defect: the Newton step already could (slice 2), and now
  the refused list of FLINT can. I added one sentence at verify_complete for the new case; the general sentence
  "never aborts" is imprecise for both cases.
- The trust base above the bound: deg d comes from FLINT's `nmod_poly_powmod_ui_binexp` and `nmod_poly_gcd`,
  which are trusted (deterministic, documented); only the root finder is tested. A defect in the powering or the
  gcd that gave a smaller d and a root finder that returned exactly the roots of that d would go unnoticed. This
  is now written in the header (as FLINT's count is trusted at the real place, S-D11).
- The name of the test `domain_unsupported_and_limit_before_any_allocation_L_untouched` in
  tests/test_roots_padic.c still says "unsupported"; I changed only its checks, not its name.

## Findings against the specification or docs/proofs/solvers.md

- solvers.md:1304 to 1308 (decision S-D10 and its source-pending line): the "later slice" is done; the bound is
  2^7 by the benchmark, and the source pending for `nmod_poly_powmod`, `nmod_poly_gcd` and the root finding is
  settled for a one-word modulus by the C sources (docs/sources.md, item 5). solvers.md should be updated by its
  owner.
- solvers.md P3.7(2) is used on the class polynomials h = g_(a,e) modulo p, not only on g; its hypothesis (h not 0
  over F_p) holds by P3.4. Nothing wrong; the text of Algorithm P step 3 says "for every b in [0, p)", which is the
  evaluation route only; with the degree route the loop runs over the roots found. A sentence in Algorithm P
  would make the proof cover both routes.
- The cost statement P3.5(7) ("p evaluations of g and g' modulo p") holds only below the bound now.

## Performance notes (COMMON-C rule 7)

- The degree route powers twice: X^p mod h here, and x^((p-1)/2) mod d inside `nmod_poly_roots`. Computing
  x^((p-1)/2) mod h once would give both d and the first split.
- `modp_check` sorts a copy of the candidates to test distinctness; the candidates are sorted again afterwards.
- `padic_search` computes h' modulo p for every class, also when h has no root modulo p.
