# Result of lane s2-slice1: roots at a prime, slice 1 (the type and the seed function)

## What was done

The first slice of S.2 works end to end: header, library code, tests against enumeration and against the
reference, a Julia call, and a differential script.

**The type `adf_rootlist`** (`include/adelefeld/roots.h`) has every field of api-s section 4: place, scope,
reduced, complete, g, n, a, K, s, nu, ua, ue, ball, count.
- Layout: 120 bytes, alignment 8. Offsets: place 0, scope 8, reduced 12, complete 16, g 24, n 48, a 56, K 64, s 72,
  nu 80, ua 88, ue 96, ball 104, count 112. They are pinned in `tests/test_roots_seed.c`.
- In this slice the classes (ua, ue) and the real balls stay empty.
- `init`, `clear` and `is_canonical` are implemented. The predicate is in the header. At the real place it is marked
  temporary.

**`adf_root_padic_from_seed`** implements solvers P3.12.
- It forms g* by `fmpz_poly_gcd`, an exact division and the primitive part.
- It tests the strong form with exact values of g(a) and g'(a).
- If k0 >= K, it reduces directly and never forms p^k0 (S-D18). Otherwise it makes Newton steps (P3.3) and then
  reduces modulo p^K.
- Statuses, in this order:
  - DOMAIN: f = 0, prec_p < 1, or the real place.
  - LIMIT: 2 prec_p bits(p) > ADF_ROOTS_BITS_MAX = 2^24. This is decided before any allocation.
  - NOT_DETERMINED.
  - LIMIT again, for K = s + 1 above the limit, decided before any power of p is formed.
  - OK: scope SEED, n = 1, nu = 0, complete = 0.
- On every status other than OK, L is untouched.
- f and a may be fields of L.
- If a Newton step does not raise the precision, the function calls `flint_abort`. That can only happen through a
  defect of the library.

**The rest of the interface:**
- `adf_rootlist_verify_entries` works at a prime. It checks:
  - that g is the normalised polynomial of f, and the reduced flag;
  - the scope rules and the shapes;
  - (R1), (R2) and (R3) for every certificate;
  - the classes, and that the balls are pairwise disjoint.
  At the real place it returns 0. The header says this is temporary.
- Accessors: `length`, `is_complete`, `unresolved_length`, `place`, `scope`, `get_poly`, `get_cert`, `get_fball`.
  `get_fball` returns the canonical triple (a, p^K, 1).
- Two functions with hidden visibility in `src/roots.c`, `adf_roots_seed_core` and `adf_roots_cert_check`, take the
  prime as an fmpz. The test declares them itself; they are not in any public header. Through them the test reaches
  primes that an `adf_place_t` cannot hold.

**Item 2: what an `adf_place_t` guarantees.**
- A finite place is made only by `adf_place_prime` (`include/adelefeld/place.h:30` to `36`). User code creates places
  only through the functions of place.h (`docs/conventions.md:1014` to `1016`). The integer inside is private.
- `adf_place_prime` returns DOMAIN unless `n_is_prime(p)` (`src/place.c:56` to `63`). This test is a certified
  decision for every word (`src/place.c:23` to `28`, citing `flint-3.0.1/ulong_extras.rst:833` to `836`).
- The argument is a `ulong`, so 2 <= p < 2^64 (`place.h:34`; `conventions.md:1021` to `1022`, and 5.8 at lines 659
  to 660).
- So p is a proved prime of one word, and it was tested by the constructor when the place was made.
- The seed function relies on exactly this, and the header says so ("The prime"). It adds no primality test. The
  predicate and the verifier only refuse p < 2: a forged p = 1 would make `fmpz_remove` abort.

## Files written

- `include/adelefeld/roots.h` (new).
- `include/adelefeld.h`: one include line.
- `src/roots.c` (new).
- `tests/test_roots_seed.c` (new): 16 tests, including the layout pins.
- `tests/ref/vectors/s2-slice1/seed.jsonl`: 7262 lines, 1.17 MB, written by `lanes/s2-slice1/gen_vectors.py`,
  which imports `proto/solvers_checks.py`.
- `tests/julia/roots.jl` (new).
- `tests/fuzz/diff_roots_seed.py` (new).
- `lanes/s2-slice1/`:
  - `gen_vectors.py`, `mutants.py`, `redgreen.log`;
  - `check-plain.log`, `check-san.log`, `check-clang.log`, `exports.log`, `julia.log`, `fuzz180.out`;
  - `result.md`.

## Checks

**Red, then green** (`redgreen.log`):
- Red 1: link error.
- Red 2, with a stub: 13 of 15 tests failed by assertions (1927229 failed checks). The two tests that passed test the
  layout and the init value, which the stub implements.
- Green: the first run had 1 failed check, and the test was wrong. For 27 X at 3 it expected NOT_DETERMINED for the
  seed 9, but g = X and v(9) = 2 > 0 = 2s, so the strong form holds. The test was repaired.
- The test of allocations (below) was written after the code. Its red run is the mutant `limit_after_normalise`.

**`./build/test_roots_seed`: 16 tests, 5347209 checks, 0 failed.**
- **Enumeration.** p = 2, 3, 5, 7, 11; 18 polynomials each (10 random of degree 1 to 4, 8 products with planted roots,
  repeated roots included); every seed in [0, p^4); prec_p 1 to 6. That is 1918512 calls:
  - OK 341322, of which 2694 have s > 0; NOT_DETERMINED 1577190;
  - 833 balls enumerated; the strong form of f and of g differ at 10781 seeds; 0 failures.

  A case fails when any of these is violated:
  - the status is OK exactly when the oracle's strong form holds for g. The oracle forms g over Q with `fmpq_poly_gcd`,
    independently of the library;
  - g, reduced, scope, n, nu and complete are right;
  - s = v(g'(a)) and K = max(prec_p, s + 1);
  - 0 <= a' < p^K and a' = a modulo p^(s+1);
  - `is_canonical` and `verify_entries` accept the list;
  - among the x modulo p^(K+s+3) with x = a modulo p^(s+1), exactly p^s have g(x) = 0 modulo p^(K+s+3), and all of
    them are a' modulo p^K;
  - a' is the same for every seed of the same class and K.
- **Equality refused.** 26304 seeds with v(g(a)) = 2 v(g'(a)) exactly (X^2 - d and X^3 - d, p = 2, 3, 5). All give
  NOT_DETERMINED with L untouched (a deep snapshot of the struct and of its arrays). X^2 + 3 at 2 and X^3 - 10 at 3
  with the seed 1 are fixed cases, and so is g'(a) = 0.
- **Conrad's examples** (hensel.txt:331 to 361), with the digits of the source:
  - 4.2: seed 2 gives 2036 mod 3^7, seed 5 gives 1697.
  - 4.3: X^3 - 10 with seed 1 is refused; seed 4 gives 3658 mod 3^8. X^3 - 5 with seed 2 is refused.
  - 4.4: seed 0 gives 1492 mod 2^11, seed 2 gives 982 mod 2^10, seed 1 gives 599 mod 2^10 with s = 0.
- **The normalised polynomial and the prime 2:**
  - 27 X at 3 gives (0, 5, 0) with g = X and reduced = 0.
  - (X - 1)^2 (X + 2) gives g = X^2 + X - 2 and reduced = 1, with s = 1 at 3 and s = 0 at 5. A false reduced flag is
    refused.
  - -6 X^2 + 18 gives g = X^2 - 3.
  - X^2 - 9 at 2 gives (13, 4, 1) and (3, 4, 1). X^2 + 1 at 2 with seed 1 is refused. X^2 - 3 at 2 is refused for 72
    seeds. X^2 - 17 at 2 is solved to 40 digits. X^3 + 2 X at 2 gives (0, 2, 1).
- **Precision.** Six certificates, prec_p 1 to 60: K = max(prec_p, s + 1), and the balls are nested (354
  comparisons). Seeds moved by +-(3^200 + t) p^(64 + 7t) give the same ball.
- **Large primes:**
  - p = 2^64 - 59 through the place: s = 3 with prec_p up to 2000, and a square root to 2000 digits.
  - 2^64 + 13, 2^89 - 1, 2^299 + 443 and 2^521 - 1 through the hidden core: s = 2, prec_p 1 to 2000; the certificate
    is checked independently in the test; a seed off the root is refused; kmax + 1 and WORD_MAX give LIMIT.
  - Primality: `fmpz_is_prime` = 1 for the 65-bit and 89-bit primes. For 2^299 + 443 and 2^521 - 1 the test uses only
    `fmpz_is_probabprime` = 1 (BPSW), not a proof.
- **Limits:**
  - prec_p = WORD_MAX at 7 and at 2, and WORD_MAX/2 + 1 at 2^64 - 59, give LIMIT.
  - At 7, kmax + 1 gives LIMIT, even for a seed that would be NOT_DETERMINED.
  - At 2^64 - 59, K = 131072 is computed with Newton steps, and 131073 gives LIMIT.
  - LIMIT from s: X (X - 2^m) at 2 gives OK for m = 2^22 - 1 and LIMIT for m = 2^22.
  - L is untouched in every case.
- **Allocations.** With FLINT's allocator counted (`__flint_set_memory_functions`), an OK call makes 10 allocations
  and three LIMIT calls make 0. GMP's own limb allocations and libc are not counted.
- **DOMAIN.** f = 0, prec_p 0, -5 and WORD_MIN, and the real place: L untouched, both on the init value and on a
  list with content.
- **Aliasing.** f = L->g together with a = L->a gives the same root at a higher precision. `get_cert` into L->a works.
  Indices out of range return 0.
- **`get_fball`** gives (a', 7^20, 1) and (0, 81, 1). It returns 0 out of range, and x is untouched.
- **The verifier.** 2200 seed lists, all accepted. Every kind of change was refused at least once, and no false list
  was accepted:

  | change | tried | refused | accepted and true |
  |---|---|---|---|
  | centre moved by t p^(K-1) | 10984 | 10496 | 488 |
  | K + 1, centre not lifted | 2200 | 1383 | 817 |
  | s + 1 | 2200 | 2200 | 0 |
  | s - 1 | 75 | 75 | 0 |
  | g replaced by f | 1545 | 1545 | 0 |
  | two balls of one root (PARTITION, n = 2) | 2200 | 2200 | 0 |
  | centre + p^K | 2200 | 2200 | 0 |
  | SEED with complete = 1 | 2200 | 2200 | 0 |

  "Accepted and true" was checked by enumeration.
- **Hand-made lists:**
  - (1, 1, 1) for X^2 + 3 at 2 is refused by (R1) (P3.2(6)).
  - The empty PARTITION list with complete = 1 for X (X - 1) at 3 passes (P3.13 example (a)).
  - The verifier or the predicate refuses the same ball twice, SEED with n = 2, a g that is not normalised, a negative
    centre, and s < 0.
  - K = WORD_MAX with centre 0 is canonical, and the verifier returns 0 without forming p^K.
- **Vectors.** All 7262 lines: OK 2062, NOT_DETERMINED 5196, DOMAIN 4; 144 lines have p above 32 bits. 0 differ in
  status, g, reduced, certificate, scope, n, complete, the verifier or the predicate. The 6552 seeds of check_s2_seed
  give 1802 OK and 4750 NOT_DETERMINED, which are the numbers of solvers.md:1519 to 1520.

**Do the tests bite (item 5)?** `python3 lanes/s2-slice1/mutants.py`, one change at a time in a scratch copy under
`build/mutants/`. 15 mutants: 13 killed, 2 survived.
- **Strong form `>` changed to `>=`:**
  - As it stands, the library's guard in `newton_step` aborts.
  - With the guard removed, the mutant loops (timeout).
  - With no lifting from k <= s (`strong_ge_nolift`), 5437230 checks fail in 7 tests.
- **s from f instead of g.** Two variants were run: the whole core run on f, and only s taken from f. They fail
  80506 and 223890 checks.
- **One Newton step dropped:** 297041 failed checks.
- **Final reduction of the centre dropped:** 273624 failed checks.
- **Removed from the verifier:** (R1) 3 failed checks, (R2) 24, (R3) 4.
- **The limit test as a plain unsigned product:** killed, by prec_p = WORD_MAX / 2 + 1 = 2^62 at 2^64 - 59, where the
  product 2 * 2^62 * 64 wraps to 0.
- **The first limit test removed:** killed by the allocation count.
- **Survived, and equivalent:** the checked s + 1 and the checked k + k, each replaced by a plain addition.
  - s = v_p(g'(a)) is at most the bit length of an integer held in memory, so s + 1 cannot overflow.
  - k < K <= 2^22 after the limit test, so k + k cannot overflow.
  - So none of the checked additions can overflow. The only operation that can is the product of the limit test.

**Build and run:**
- `make clean && make -j2 check`: `check passed: all 50 test programs`.
- The same with `SAN=1`: `check passed: all 50 test programs`, with 0 sanitizer reports.
- The same with `CC=clang`: `check passed: all 50 test programs`. The first clang run failed: a variable in the test
  was set but not used. It was removed, and all three runs were repeated.
- After those clean runs, two comments were reflowed (the header, and one test line). A plain `make -j2 check` after
  that also passed: 50 programs; roots 16 tests, 5347209 checks, 0 failed.
- `sh tests/test_exports.sh`: `test_exports: passed: 233 of 233 declared functions are exported, 0 are not
  implemented yet, no exported name is undeclared, no variadic function`. The two hidden functions are not in
  `nm -D`.
- `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia --startup-file=no tests/julia/roots.jl
  build/libadelefeld.so`: `adf_root_padic_from_seed through ccall | 10 10`.
  - It covers: the square root of 2 in Z_7 to 20 digits from the seed 3, squared in Julia modulo 7^20; the seed 4
    giving -a; 27 X; X^2 + 1 at 2; DOMAIN.
  - Without the LD_PRELOAD it fails with the known quirk (`undefined symbol: __gmpn_modexact_1_odd`).
  - `sh tests/test_julia.sh`: `test_julia: passed (with LD_PRELOAD=...)`.
- The header alone compiles in C11 (gcc) and C++17 (g++).
- `lanes/m1-headers/check_headers.sh`: step 1 passed (45 single-header compilations, roots.h among them, 0
  failures). Step "clang test_abi" failed with `undefined reference to adf_resid_reconstruct`. That is not code of
  this lane, and I did not check whether it fails on the base commit too.

**Fuzzing.** `python3 tests/fuzz/diff_roots_seed.py --seconds 180 --seed 1`:
- 453919 calls, 0 disagreements. About 113000 each of small, medium, word and p = 2 primes. OK 200971 (48523 with
  s > 0), NOT_DETERMINED 239311, DOMAIN 13637.
- It asserts that the status, the certificate, g and reduced are equal to the reference, that the shape is SEED, and
  that `verify_entries`, `is_canonical` and `root_cert_ok` accept.
- **This run was a smoke test, not a long fuzzing run.**

## What is not done

- Not in this slice, and not declared: `adf_roots_padic`, `adf_roots_padic_partial`, `adf_roots_real`,
  `verify_complete`, `get_unresolved`, `get_arb`, `set`, `swap`, `identical`, benchmarks.
- Because `verify_complete` does not exist yet, the acceptance test "no seed list passes `verify_complete`" is not
  done.
- `INV=1` was not run.
- No long fuzzing run.
- The allocation test counts FLINT's allocator only.
- Primes above one word are tested only through the hidden functions. Two of them are only probable primes in the
  test (see Large primes above).

## Header findings

- **HEADER-FINDING (S-D10 against CV-18).** S-D10 says every prime, and the brief asks for primes of 65 and 300 bits.
  But `adf_place_t` holds only primes below 2^64 (conventions 7, lines 1021 to 1022). The seed function needs no bound
  on p. Its core takes any fmpz prime and is tested at 65, 89, 300 and 521 bits, but the interface cannot reach it.
- **The limit.** S-D18 says "p^(2K) larger than ADF_ROOTS_BITS_MAX", which I read as the bit length.
  - The test used is 2 K bits(p) > 2^24. It is an upper bound: bits(p) is less than 2 log2(p), and equal to it for
    p = 2.
  - It is decided by a division, so nothing overflows.
- **UNSUPPORTED.** api-s lists `UNSUPPORTED` for the seed function. It is never returned: the function needs no roots
  modulo p.
- **Order of the statuses.** NOT_DETERMINED comes before the LIMIT that comes from s; api-s does not order these two.
- **Additions not in api-s:**
  - `verify_entries` also checks the reduced flag.
  - `get_fball` returns 0 if K bits(p) > ADF_ROOTS_BITS_MAX.
  - The predicate at the real place is a temporary strict reading: PARTITION, complete = 1, n = count.
  - A Newton step that does not raise the precision aborts, as linsolve does.

## Findings against the specification and `docs/proofs/solvers.md`

- No statement of section 3 was found false.
  - P3.12 and its counts are reproduced exactly: 1802 of 6552 seeds OK and 4750 NOT_DETERMINED.
  - P3.2(6) holds: (1, 1, 1) for X^2 + 3 at 2 is refused by (R1) alone.
  - The enumeration of P3.2(1) held for 833 distinct balls. They are the balls of all 341322 OK calls: the calls
    with the same class and the same K were checked to give the same a'.
- The conflict between S-D10 and conventions 7 (above) is a finding against the specification. A later slice for
  primes above one word needs a place type that holds them.

## Avoidable costs (not optimised)

- g(a) and g'(a) are evaluated exactly at the unreduced seed. For a huge seed, their size grows with deg times the
  bits of a.
- Each call recomputes gcd(f, f') and uses `fmpz_poly_divides`, which FLINT's documentation calls unoptimised. The
  verifier and `is_canonical` compute a gcd again.
- `newton_step` recomputes p^k, p^s and p^(2k - s) with `fmpz_pow_ui` at every step, and `eval_mod` reduces the point
  on every call.
