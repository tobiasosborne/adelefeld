# Lane m1-scaled: the scaled-residue policy (work package 1.7, second part)

Lane directory `lanes/m1-scaled/`.  Date 2026-09-28.  All functions of `include/adelefeld/scaled.h`
from `adf_scaled_init` to `adf_scaled_add_rat` (19 functions) are implemented in `src/scaled.c`.
The five cap functions at the end of the header are `src/cap.c` of lane m1-cap and were not touched.

## What was done

`src/scaled.c` implements the scaled-residue policy of `docs/SPEC.md` 4.4 against
`docs/proofs/policies.md` section 2 and `docs/proofs/precision.md` Propositions 5 and 6:

- life cycle (`init`, `clear`, `set`, `swap`), the predicate of `docs/conventions.md` 5.4
  (`is_canonical`), `identical`, `context`, `is_exact`;
- conversions: `set_rat` (the exact case, policies Definition 4, line 107), `set_fball`
  (the best scaled enclosure, policies Proposition 7, line 139; loss exactly when `c K/R` is not
  an integer, Lemma 6, line 124; the exact case of closure C5), `set_context` (policies
  Proposition 11, line 218; closure C5 verbatim), `get_fball` (the global canonical set);
- arithmetic at one context: `add` (tight, precision Proposition 5(1), line 99; one exact operand
  by policies Proposition 8, line 159), `sub` (as `x + (-y)`), `mul` (the default product of
  SPEC 4.4, losing the factor `h = gcd(u, v, K)`, policies Proposition 10.2, line 199),
  `mul_tight` (policies Proposition 10.3, line 200), `neg`, `mul_rat` (policies Proposition 9,
  line 184), `add_rat` (policies Proposition 8, with `*lost`).

The shared-context rule (`docs/conventions.md` 4.6, closure edit E1; SPEC 4.4 "Context
compatibility for scaled values") is the first statement of `add`, `sub`, `mul` and `mul_tight`:
different context pointers give `ADF_DOMAIN` before any write, exact operands included; the
result borrows the shared input context; the old context of the output is never compared.
Rational gcds use `fmpq_gcd`/`fmpq_gcd_cofactors`, which compute the generator of the subgroup
of Q exactly as `docs/proofs/precision.md:9-11` defines it
(`refs/src/flint-3.0.1/fmpq.rst:510-528`).  Only the public functions of `modctx.h` are used on a
context (`adf_modctx_get_modulus`); no block access, no layout knowledge.

Tests (`tests/test_scaled.c`, 23 tests; `tests/test_scaled_vectors.c`, 11 tests) cover, besides
the vector rows: the context rule in all its cases (different pointers, equal moduli at different
pointers, exact operands, output aliasing an input, output in a third context); the loss cases of
the default product with `adf_scaled_mul_tight` strictly finer and both containing the tight
`adf_fball_mul` of the converted operands (`adf_scaled_get_fball`, `adf_fball_contains`), and
`mul_tight` equal to that tight product; the exact tag kept through every operation; `set_fball`
and `set_context` reporting `lost` exactly when the set changes (plus best-enclosure checks over
all candidates of small scale and residue); the lossless meeting in `lcm(K, K')` with the
ordinary operations afterwards (policies Corollary 12, line 241); three random expressions of 30
operations in the tight and in the scaled policy with containment after every step (Theorem 3,
line 75); canonical data equal for equal sets from different inputs (Lemma 5, line 113); moduli
of one word and of 4096 bits (context without blocks); aliasing of every permitted combination
(`(x,x,y)`, `(y,x,y)`, `(x,x,x)`, unary `y = x`); exact zero and zero residues; and the literal
rows of SPEC 4.3 (`(3 mod 12) + (5 mod 18) = 2 mod 6`, `(3 mod 12) * (5 mod 18) = 3 mod 6`,
`12 * (5 mod 18) = 60 mod 216`) and the worked example of policies Proposition 10 (line 212),
and every exact result storing `u = 0` into an output that held a scaled value before.

The benchmark `bench/bench_scaled.c` holds the rows of `docs/PLAN.md` row 1.7 (add, mul), at a
one-word modulus and at 4096 bits, provisional (`docs/PERF.md` section 4 has no floor for these
operations).

## Files written

| File | Content |
|---|---|
| `src/scaled.c` | 19 functions of scaled.h (the cap block is not this lane) |
| `tests/test_scaled.c` | 23 tests: context rule, loss cases, tags, lcm, aliasing, big moduli, expressions |
| `tests/test_scaled_vectors.c` | 11 tests: the scaled rows of policies.jsonl and all rows of the lane vectors |
| `tests/ref/vectors/m1-scaled/scaled_ops.jsonl` | 1762 rows (neg 129, to_fball 129, scale 205, add_rat 245, sub 263, mul_exact 223, mul_tight 283, set_context 285) |
| `lanes/m1-scaled/gen_scaled_vectors.py` | the deterministic generator of the lane vectors |
| `lanes/m1-scaled/tdd-log.md` | the red-green log |
| `lanes/m1-scaled/mutate_scaled.log` | output of mutation run 1 |
| `lanes/m1-scaled/mutate_scaled2.log` | output of mutation run 2 (verification) |
| `bench/bench_scaled.c` | the four rows (add, mul at a one-word modulus and at 4096 bits) |
| `bench/results/2026-09-28T105923Z_scaled.txt` | the benchmark run (run artifact) |

Edited outside the lane but allowed by COMMON-C rule 5: `tools/mutate/equivalent.txt` (28
excused mutants of `src/scaled.c`, each with its reason).  No other file outside the paths of
the brief was changed.  `bench/Makefile` was not edited (COMMON-C rule 8: it finds
`bench/bench_scaled.c` by itself).

## Checks that were run

1. `python3 lanes/m1-scaled/gen_scaled_vectors.py` -> the 1762 rows above; every cross-check
   assertion of the generator passed (the P10.3 rows against `adfref.fball.mul`, the P11 rows
   against containment/loss/exactness, the P8 rows against `adfref.policies.scaled_add`).  Two
   runs are byte-identical: `md5sum` = `77bff3f766ad9994ff5aefc053e6c56e` both times.
2. `./build/test_scaled_vectors` -> 11 tests, 62707 checks, 0 failed checks, 0 failed tests.
   It runs `tests/ref/vectors/policies.jsonl` completely in its scaled rows (the tests assert the
   row counts: 360 `convert_from_tight`, 300 `scaled_add`, 300 `scaled_mul`; the 300
   `absolute_cap` rows are the file of lane m1-cap) and `tests/ref/vectors/m1-scaled/
   scaled_ops.jsonl` completely (per-op counts asserted).
3. `./build/test_scaled` -> 23 tests, 88372 checks, 0 failed checks, 0 failed tests.
4. `make -j2 check` -> `check passed: all 25 test programs`.
5. `make clean && make -j2 check SAN=1` -> `check passed: all 25 test programs`; the whole log
   has 0 lines matching `Sanitizer` or `runtime error` (no AddressSanitizer and no leak report).
6. `make clean && make -j2 check CC=clang CFLAGS='... -Wno-newline-eof'` -> `check passed: all 25
   test programs`.  The flag works around a finding against another lane's file (below); without
   it clang stops in `src/modctx.c` before any test is built.
7. `cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -c src/scaled.c` -> no output,
   `compiled clean` (the same flags the build uses).
8. `./bench/bench_scaled --run --trials 15 --cpu 2`, result
   `bench/results/2026-09-28T105923Z_scaled.txt` (`quiet_machine: no`; other lanes share the
   laptop, so these are harness probes, not machine figures):
   - `scaled_add_word`, call rate: min 157.7949, median 162.1051, max 167.0786 ns/op.
   - `scaled_mul_word`, call rate: min 133.3734, median 135.5950, max 137.6372 ns/op.
   - `scaled_add_4096`, call rate: min 1332.5459, median 1416.9654, max 1453.0318 ns/op.
   - `scaled_mul_4096`, call rate: min 7222.1819, median 7596.8804, max 12131.6516 ns/op.
   - checksum 576460755101312524.
   The rows are provisional: `docs/PERF.md` section 4 has no floor for scaled arithmetic, so no
   ratio is claimed (PERF section 7).
9. `make mutate FILES=src/scaled.c JOBS=2 LIMIT=300` (seed 20260928) -> 300 mutants in
   1597.3 s: 251 killed, 31 survived, 18 not compiled, 0 timed out, 0 excused;
   `mutate: FAILED` (the target fails on survivors).  The 31 survivors were handled as follows:
   - 3 were killed by a new test (`exact_results_store_a_zero_residue` in `tests/test_scaled.c`):
     `drop_call` of `fmpz_zero(y->u)` at `src/scaled.c:161`, `:200`, `:610`, the exact paths of
     `set_rat`, `set_fball` (R = 0) and `mul_rat`.  Each mutant was applied by hand (one line
     commented out) and the new test failed by assertion against it (2 failed checks each);
     the restored original passes (23 tests, 88372 checks, 0 failed).
   - 28 are equivalent for every input and are excused with their reasons in
     `tools/mutate/equivalent.txt` (28 `src/scaled.c` lines): 19 `swap_args` of commutative FLINT
     calls (product and sum do not depend on the operand order; the aliasing is the same), 6
     `cmp`/`zero_one` of `if (fmpq_sgn(q) < 0)` (the branch is reached only for `q != 0`, where
     the sign is -1 or 1 and `< 0`, `<= 0`, `< 1` agree), and 3 `drop_call` of a zero store into
     data that `fmpq_init`/`fmpz_init` already set to zero (0 = 0/1 for `fmpq`).
10. `make mutate FILES=src/scaled.c JOBS=2 LIMIT=300` a second time (same seed, after the new
   test and the excuses) -> 300 mutants in 1581.5 s: 254 killed, 0 survived, 18 not compiled,
   0 timed out, 28 excused; `mutate: passed`.  The three mutants killed by the new test are
   among the 254 (251 + 3 = 254).
11. `make -j2 check` once more after the mutation phase -> `check passed: all 25 test programs`.

The red-green sequence is in `lanes/m1-scaled/tdd-log.md`: the first red of each test file was
the one allowed link error; the first full run of `test_scaled` was red by assertion (53 failed
checks) and found one real bug (`adf_scaled_mul_rat` with `y = x` destroyed `x->s` before
multiplying; 49 of the failures) and two wrong test expectations.  The mutation phase then
added one test (red by hand-applied mutant, three times).

## What is not done

- The five cap functions (`src/cap.c`) and the text/dump functions of `scaled.h`
  (`adf_scaled_get_str`, `adf_scaled_dump_str`, the loaders) are other lanes' work.
- The local-backend case of `adf_scaled_set_fball` is refused (see the finding below) and is
  pinned by one test (`set_fball_refuses_a_local_value`); the lossless local-to-scaled path can
  be implemented and tested when work package 1.8 (lane m1-local) lands.
- The benchmark has no matched floor and no ratio (PERF section 4 names no scaled row); the
  numbers are provisional as the PLAN row asks.
- No fuzz target: nothing in this file parses text (`tests/ref/README.md` points the parsers to
  work package 1.4).

## Sources pending

None.  Everything cited is on disk: `docs/proofs/policies.md`, `docs/proofs/precision.md`,
`docs/conventions.md`, `docs/SPEC.md`, `docs/reviews/m0-gate/closure.md`, and the FLINT manual
under `refs/src/flint-3.0.1/` (`fmpq.rst:510-528` for `fmpq_gcd`/`fmpq_gcd_cofactors`,
`fmpz.rst:824-868` for `fmpz_fdiv_r`, `fmpz_divexact`, `fmpz_divisible`, `fmpz.rst:1040-1051`
for `fmpz_gcd`, `fmpz_gcd3`, `fmpz_lcm`).  The formulas of policies Proposition 10.3 and
Proposition 11 are the proof's own statements (my proofs need no source); the generator
cross-checks them numerically against the reference.

## Findings against the specification

1. **HEADER-FINDING, `adf_scaled_set_fball` and local inputs.**  scaled.h says the conversion
   returns "ADF_OK always" (conventions 3.2 row "Conversion from tight to scaled"), and fball.h
   admits an input of either backend.  For an `ADF_LOCAL` input the set is `(A0 + K Zhat)/d` and
   the formula `s* = gcd(c, R/K)` needs the CRT lift `A0` of the residue array
   (`docs/conventions.md` 5.3); `src/fball.c` of lane m1-fball does not read local values yet
   (its own HEADER-FINDING) and its `adf_fball_get_fmpz3` silently writes nothing for them, so
   the formula would run on stale data and return a wrong set.  Following the choice of
   `src/cap.c` (same work package, same gap), the function returns `ADF_UNSUPPORTED` with the
   output untouched for a local input instead of guessing (lanes/COMMON-C.md rule 3).  This
   deviates from the header's "ADF_OK always"; the alternative reading would silently produce a
   wrong result.  When 1.8 lands, the check can be dropped in favour of `adf_fball_get_fmpz3`.
2. **`make check CC=clang` fails in `src/modctx.c` (lane m1-modctx-b), which is not mine.**  The
   file has no newline at its end of file and clang's `-Wnewline-eof` (in `-Wpedantic`) rejects
   it with `-Werror`: `src/modctx.c:817:2: error: no newline at end of file`.  One newline at the
   end of that file fixes it; I did not edit the file (it is read-only for this lane).  The clang
   run above passes with `-Wno-newline-eof`.
3. **Observation, not an error of the specification** (policies.md records the same at line 209):
   the default product of SPEC 4.4 loses exactly the factor `h = gcd(u, v, K)` and is not tight;
   for `u = v = 0` it loses the full factor `K`.  The tight variant `adf_scaled_mul_tight`
   recovers it.  This is DECISION CV-48 and is implemented as specified.

## Avoidable costs (COMMON-C.md rule 7)

- `adf_scaled_sub` builds a temporary for `(-y)` and then runs `adf_scaled_add`, which builds
  another temporary; the direct formula (one gcd, one reduction) would save one temporary chain
  and one `swap`.  Kept for the definition "z = x + (-y), with adf_scaled_neg".
- Every status-returning binary call allocates one temporary `adf_scaled_struct` and five to
  seven `fmpz`/`fmpq` temporaries, even when no aliasing is present; a batch kernel with
  caller-per-thread scratch would amortise this (PERF section 5, hypothesis 2, is exactly this
  question).
- `adf_scaled_set_context` recomputes `K/K'` and one rational gcd per call.  `gcd(K, K')` is a
  property of the two contexts, which are immutable and shared; caching it in the context pair
  (or a derived-context table) would move the cost out of the per-value conversion.
- `fmpq_gcd_cofactors` computes both cofactors; `adf_scaled_set_fball` uses only one (the other
  is `(R/K)/s*`).  `fmpq_gcd` plus one division would do the same work; noted, not optimised.
- The 4096-bit `scaled_mul` row (7.6 us median) is dominated by the two `fmpq` multiplications
  of 2000-bit numerators and their gcd reductions (canonical output); the default product of
  SPEC 4.4 itself needs no integer gcd at all.
