# Result of lane s2-slice2: all roots at a prime (Algorithm P)

## What was done

The search for all roots of f in Z_p works end to end: header, library code, tests against an enumeration and
against the reference, a Julia call, and a differential script. Slice 1 is unchanged: no line of slice 1 in
`src/roots.c` was edited, and `tests/test_roots_seed.c` passes unchanged (16 tests, 5347209 checks, 0 failed).

**Declared in `include/adelefeld/roots.h`** (as `docs/api-s.md` section 4 states them):
- `adf_roots_padic_partial(L, f, p, prec_p, depth)` (S-D15) and `adf_roots_padic(L, f, p, prec_p, depth)`.
- `adf_rootlist_get_unresolved(a, e, L, i)`.
- `adf_rootlist_verify_complete(L, f, depth)`. At the real place it returns 0, marked TEMPORARY.
- `ADF_ROOTS_P_EVAL_MAX = 1048576 = 2^20` (S-D10). Above it both search functions return `ADF_UNSUPPORTED`,
  L untouched. The header says that this status is TEMPORARY and goes away with the slice that adds the route
  of P3.7(2). `verify_complete` returns 0 above the bound.
- The top comment of the header now names slice 2, and the paragraphs "The prime" and "Limits" say what the
  search adds (the bound on p; the limit also on K = max(prec_p, s + 1) of every root and on the exponent of
  every class).

**Implemented in `src/roots.c`** (a new section at the end of the file; cited by file and line):
- `padic_search` is Algorithm P (solvers.md:1162 to 1176) on g*. The polynomial of a child class is formed
  from that of its parent: g_(a1,e+1)(Y) = g_(a,e)(b + p Y) / p^v, w(a1, e+1) = w + v (solvers.md:1149). This
  uses `fmpz_poly_taylor_shift`, a scaling by p^i, and the content at p.
- The roots modulo p are found by `nmod_poly_evaluate_nmod` of h and h' at every residue 0, ..., p - 1
  (P3.7(1)). No routine of FLINT for roots or factorisation modulo p is called.
- A simple digit is certified by `adf_roots_certify` (P3.4(3), step 3 first case):
  - s = w - e, and j = the least j >= 1 with j > w - 2 e;
  - beta_j by Newton steps of P3.3 on h from (b, 1, 0);
  - the certificate ((a + p^e beta_j) mod p^(e+j), e + j, s), lifted by P3.3 on g to K = max(prec_p, s + 1)
    and reduced modulo p^K.
  The Newton step of slice 1 (`newton_step`) is reused.
- A multiple digit gives a child class. At e + 1 > depth the class is kept as unresolved.
- The lists are sorted by the centre (insertion sort).
- `adf_roots_padic_core` does the statuses, and after them the normalisation and the search. L is written only
  on OK, by exchanging the struct with a local list (so f may be L->g). The strict function returns
  NOT_DETERMINED when nu > 0, and L is untouched.
- `adf_rootlist_verify_complete` checks, in this order:
  - depth >= 0, f != 0, a prime;
  - `verify_entries`;
  - scope PARTITION, complete = 1, nu = 0;
  - p <= bound;
  - it reruns `padic_search` on L->g with prec 1 through depth; there must be no class;
  - every root ball of the rerun meets exactly one listed ball, and every listed ball exactly one root ball
    (P3.13(2), P3.2(4)).
- Statuses, in order:
  - DOMAIN: f = 0, the real place, prec_p < 1, depth < 0;
  - UNSUPPORTED: p > 2^20;
  - LIMIT: 2 prec_p bits(p) > ADF_ROOTS_BITS_MAX.
  All three are decided before any allocation (tested with FLINT's allocator counted: 0 allocations).
  During the search, LIMIT comes from any of these:
  - an overflow in s = w - e, 2 e, w - 2 e, j, e + j, s + 1, e + 1 or w + v (checked slong);
  - a root with 2 K bits(p) > limit;
  - a class exponent e with 2 e bits(p) > limit.
  Each is decided before that power of p is formed, with L untouched. No partial list is returned on LIMIT.
- Two hidden functions (visibility hidden, not exported; `tests/test_exports.sh`: 246 of 246 declared, 0
  exported and not declared):
  - `adf_roots_padic_core(..., strict, bits_max)`, the search with the limit as a parameter;
  - `adf_roots_certify(...)`, one digit.
  The test declares them itself, to reach the limits of S-D18 that no polynomial of a size a test can handle
  reaches.

**Tests (`tests/test_roots_padic.c`, 8 tests, 55131 checks, 2.2 s):**
- **Against the enumeration.**
  - Setup: p = 2, 3, 5, 7; for each prime 40 polynomials: 16 random polynomials of degree 1 to 4 with
    coefficients in [-3, 3], and 24 products of 2 to 4 linear factors. The products have planted roots in
    [-6, 6], repeated roots, and two roots congruent modulo p^t with t up to 6, 4, 3, 2; a leading coefficient
    1, 2, 3 or p; and a factor X^2 + 1 or X^2 + X + 1. Every run uses prec 1 to 6 and depth 0 to 8: 8640 runs.
  - The precision M: M = max(2 v_p(R) + 2, K + s over the balls, ue over the classes), from an integer Bezout
    identity u g + v g' = R of the test (fmpq_poly_xgcd, and the identity is checked in the test). The claim
    that this M is large enough is proved in a comment in the test, with (W) and P3.4(4).
  - A run fails if an approximate root modulo p^M lies in zero or in two balls or classes.
  - A run fails if a ball does not hold exactly p^s of the x modulo p^(K+s+3) with x = centre modulo p^(s+1),
    all = centre modulo p^K.
  - A run also fails on any of these:
    - K != max(prec, s + 1), or ue != depth + 1;
    - complete != (nu == 0);
    - is_canonical or verify_entries refuses;
    - the balls are not nested with prec - 1;
    - the strict function does not return NOT_DETERMINED with L untouched exactly when the partial one has
      complete = 0 (with an equal list otherwise);
    - verify_complete at the same depth is not equal to complete.
  - Counts: 7902 complete, 738 incomplete, 1572 empty, 2820 with s > 0; 8594 approximate roots; 14910 balls
    counted; 7200 nested pairs; 0 polynomials skipped. The test fails if one of the four counts is 0.
- **Vectors.** `tests/ref/vectors/s2-slice2/padic.jsonl`, 650 lines from `lanes/s2-slice2/gen_vectors.py`,
  which imports `proto/solvers_checks.py`:
  - the 31 PADIC_CASES × depth 0, 1, 3, 8 × prec 1, 3 (248 lines);
  - x^2 + 1 at 2 at the depths 0, 1, 2, 5;
  - the zero polynomial, constants and degree 1 at 2, 3, 5, 7, and the DOMAIN cases of prec and depth;
  - 300 random products at the primes up to 101;
  - planted roots at 65537 and 1048573.
  Every line is compared on the strict status, the partial list (g, reduced, complete, the certificates, the
  classes, in order), and strict = partial on OK. The number of roots of PADIC_CASES is compared at depth 8
  (54 lines). verify_complete accepts each complete list at its own depth and at d0, and refuses it at d0 - 1
  (199 lines). Result: 540 OK, 90 NOT_DETERMINED, 20 DOMAIN, 0 differ.
- **Fixed cases:**
  - x^2 + 1 at 2: strict NOT_DETERMINED with L untouched at depth 0; partial gives the class (1, 1);
    depth 1 to 4 give OK and the empty list;
  - x^2 - 9 at 2: the certificates (3, 4, 1) and (13, 4, 1);
  - x^2 - 3 at 2: empty;
  - X^3 + 2X at 2: (0, 2, 1);
  - the zero polynomial gives DOMAIN; constants give the empty complete list with g = 1;
  - 27 X at 3 gives (0, 5, 0); 4X - 6 at 2 is empty, and at 3 has 2a = 3 modulo 3^5;
  - (X - 1)^2 (X + 2) at 3: 2 roots, reduced = 1.
- **verify_complete.** Changed lists of the 160 polynomials, all refused:
  - a certificate dropped: 321 of 321, and the entries verifier still accepts each;
  - two balls merged into the least ball that holds both: 185 of 185;
  - a ball moved by p^(K-1): 321 of 321;
  - a ball doubled: 321 of 321;
  - a false n (n - 1 with the arrays of n): 136 of 136;
  - a class kept with complete = 1: 128 of 128;
  - a depth one below the least depth: 79 of 79.
  Also:
  - seed lists: 1794 of 1794 refused;
  - X (X - 1) at 3 with an empty list called complete: entries 1, complete 0; the true list is accepted at
    depth 0;
  - depth < 0, f = 0 and the real place give 0.
  A check fails if a changed list is accepted, and the test fails if a kind has none.
- **Limits:**
  - DOMAIN, UNSUPPORTED (p = 1048583, 2^64 - 59, also with prec = WORD_MAX) and LIMIT of prec (7, 2 and
    1048573) give their status with 0 FLINT allocations and L untouched, for both functions;
  - 1048573 <= 2^20 works;
  - LIMIT during the search: X (X - 2^10) at 2 has K = 11, so bits_max 44 gives OK and 43 gives LIMIT, with L
    untouched; a class of exponent 6 at depth 5 gives LIMIT for 23 and OK for 24;
  - `adf_roots_certify` at the exponents near WORD_MAX gives LIMIT with 0 allocations and the outputs
    untouched, in 8 cases: j overflows, 2e overflows, and K is beyond the bits.
- **Accessors and aliasing:**
  - get_unresolved (real place, out of range, a = L->ua + 0);
  - f = L->g for both functions, equal to a run on a copy.

**Julia** (`tests/julia/roots.jl`, 20 new tests, all passed):
- (X^2 - 2)(X - 3) in Z_7 through `adf_roots_padic` and the accessors: 3 roots, (R2) and (R3) checked in
  BigInt, s = 0, 1, 1, and the square roots of 2 checked modulo 7^10;
- at depth 0: strict NOT_DETERMINED, and partial gives the class (3, 1);
- DOMAIN and UNSUPPORTED.

## Files written

- `include/adelefeld/roots.h`: additions (declarations, the constant, comments).
- `src/roots.c`: additions (a new section; one include line `<flint/nmod_poly.h>`).
- `tests/test_roots_padic.c` (new).
- `tests/ref/vectors/s2-slice2/padic.jsonl` (new, 650 lines).
- `tests/julia/roots.jl`: additions.
- `tests/fuzz/diff_roots_padic.py` (new).
- `lanes/s2-slice2/`:
  - `gen_vectors.py`, `bite.py`;
  - `redgreen.log`, `fuzz180.out`, `julia.log`;
  - `check-all.log`, `check-san.log`, `check-clang.log`, `check-headers.log`;
  - `result.md`.

## Checks

Every test program ran under `timeout`. The final runs were made on the final tree. The logs are in
`lanes/s2-slice2/`.

- **Red.** `make -j2 build/test_roots_padic && timeout 300 ./build/test_roots_padic`, against stubs, with the
  declarations in the header: 8 tests, 23186 checks, 18026 failed checks, 8 failed tests (assertions; the
  details are in `redgreen.log`).
- **Green.** The same command: `8 tests, 55131 checks, 0 failed checks, 0 failed tests`. The first green run
  had 462 failures from a defect of the test: the nested balls were paired by index. That was repaired in the
  test.
- **`tests/test_roots_seed.c`, unchanged**, in every build: `16 tests, 5347209 checks, 0 failed checks, 0 failed
  tests`.
- **`make clean && make check-all`**: last line
  `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`. It includes the
  static checker over the tree: "src/*.c tests/*.c tools/adf/adf.c (70 files): no finding".
- **`make clean && make -j2 check SAN=1`**: last line `check passed: all 52 test programs`. No sanitizer report
  in the log (0 lines with "runtime error" or "AddressSanitizer").
- **`make clean && make -j2 check CC=clang`**: last line `check passed: all 52 test programs`.
- **`sh lanes/m1-headers/check_headers.sh`**: last line `check_headers: passed`. roots.h declares 19
  functions; none is variadic, and every name starts with adf_.
- **`python3 tools/memcheck/check_uninit.py src/roots.c tests/test_roots_padic.c`**: use-before-init 0,
  clear-before-init 0, init-without-clear 0.
- **`sh tests/test_exports.sh`**: `passed: 246 of 246 declared functions are exported ... no exported name is
  undeclared`. The two hidden functions are not exported.
- **Julia**: `LD_PRELOAD=<system libgmp> julia --startup-file=no tests/julia/roots.jl build/libadelefeld.so`
  gives 10/10 (slice 1) and 20/20 (slice 2) passed (`julia.log`). The system libgmp is preloaded because of
  the Julia and gmp quirk of `tests/test_julia.sh`. `make check-all` runs only `smoke.jl`.
- **Differential run (a smoke test by CLAUDE.md rule 2, not a fuzzing campaign)**:
  `timeout 240 python3 tests/fuzz/diff_roots_padic.py --seconds 180 --seed 1` made 355548 calls: 304300 OK,
  25464 NOT_DETERMINED, 25784 DOMAIN; 160140 at p <= 7; 25464 lists with classes, 111676 with s > 0;
  0 disagreements (`fuzz180.out`). A call fails on any of these:
  - a different status of either function;
  - a different g, reduced, complete, certificate list or class list (in order);
  - scope != PARTITION;
  - verify_entries or is_canonical != 1;
  - verify_complete != complete;
  - strict != partial on OK.
- **The tests bite (item 5)**: `python3 lanes/s2-slice2/bite.py` made six changes, one at a time, in a copy
  under `build/bite/`. All 6 were caught:
  - `g'(b) != 0` removed: abort in `newton_step` ("no inverse in a Newton step"), exit -6;
  - `j > w - 2e` to `>=`: abort ("Newton step from k = 4, s = 4"), exit -6;
  - `k > s` to `>=`: 4540 failed checks in 5 tests;
  - the loop to p - 1: 9441 failed checks in 6 tests;
  - a class dropped: 1544 failed checks in 6 tests;
  - complete = 1 with a class: 3123 failed checks in 5 tests.
  A first run of the script hung on the change `k > s`: the test computed v_p(0) for two equal centres of the
  defective list. The test now guards against that, and the second run is the one above.

## What is not done

- As the brief says: `adf_roots_real`, `get_arb`, the route of P3.7(2), `set`, `swap`, `identical`,
  benchmarks.
- The Julia additions were written after the C code. They were not seen red.
- `make mutate` was not run. Item 5 replaces it.

## Sources pending

- `refs/src/flint-3.0.1/fmpq_poly.rst` is not under refs/. The test oracle calls `fmpq_poly_xgcd`, and it
  checks the identity S g + T g' = 1 itself, so it does not rely on the description of that function (marked
  in the test).
- solvers.md P3.7 keeps its `[source pending]` for the routines of P3.7(2). They are not used in this slice.

## Header findings

None that block. I own `roots.h` in this lane, so no HEADER-FINDING marks were set. Notes:
- In Algorithm P every unresolved class has the exponent depth + 1 exactly. Classes are opened at the levels 0
  to depth only, and a class goes to U only from level e = depth. The header states this.
- The abort messages of `newton_step` (slice 1 code, now shared with the search) name
  `adf_root_padic_from_seed` even when the search calls them. I did not change them, to leave slice 1 alone
  while its review runs.

## Findings against the specification and docs/proofs/solvers.md

1. **Not everything can be decided before allocation.** `docs/api-s.md` section 4 (the row of
   `adf_roots_padic`) says "LIMIT: an exponent or size beyond the limits of S-D18, decided before allocation".
   S-D18 (`docs/SPEC.md` 15.3) says "gives LIMIT before any allocation". For the search this can hold only for
   the requested precision. The precision K = max(prec_p, s + 1) of a root, and the exponent of a class, are
   known only during the search, after g* and the polynomials of the open classes are allocated.
   - Example: X (X - 2^m) at 2 has two roots with s = m. The digit is simple only at the level m, so
     K = m + 1 is known only there. For m + 1 > 2^22 (2 K bits(2) > 2^24) no test before the search can know
     it.
   - The implementation decides this LIMIT before the power of p is formed, with L untouched, and the header
     says so. The seed function of slice 1 has the same order.
   - The specification could say: "decided before any power of p beyond the limit is formed; L untouched".
2. No statement of solvers.md section 3 was found wrong. Algorithm P, P3.4(3) (j > w - 2 e), P3.4(4)
   (w(a1, e+1) >= w + 1), P3.5(2), (3), (5) and P3.13(2) were implemented as written. P3.6(3) (the integer
   Bezout identity) is used by the test oracle, not by the library. Every result agreed with the enumeration
   and the reference in all runs above.
3. Note on item 5 of the brief: the changes "g'(b) != 0 removed" and "j > w - 2e to >=" are caught only by the
   defect guard of the library (the abort in `newton_step`), not by an assertion of the test. The first call
   that meets the change aborts before a list is returned. I did not measure whether the assertions of the
   test alone would catch these two changes without the guard.

## Avoidable costs (COMMON-C rule 7)

- At every class the evaluation at all p residues costs p deg h. For large p, multipoint evaluation, or the gcd
  with X^p - X of P3.7(2), is cheaper (the later slice).
- `adf_roots_certify` forms g' again for every root; it could be formed once per search.
- `val_p` allocates a temporary for each coefficient in `content_val_p`.
- `verify_complete` normalises f again (in `verify_entries`) and runs the whole search again (api-s note 3).
- The scaling after the Taylor shift multiplies coefficient by coefficient; the power p^i is built up as it
  goes.
