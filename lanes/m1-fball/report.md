# Lane m1-fball: finite balls, tight arithmetic, global backend (work package 1.2)

Lane directory: `lanes/m1-fball`. Date: 2026-09-28.

## What was done

`src/fball.c` implements every function declared in `include/adelefeld/fball.h` for the global
backend (predicate G of `docs/conventions.md` 5.2). That is: life cycle (`init`, `clear`, `set`,
`swap`, `is_canonical`, `identical`), constructors (`zero`, `one`, `set_si`, `set_fmpz`,
`set_rat`, `set_fmpz3`, `set_center_radius`, `canonicalise`), accessors (`is_exact`,
`get_fmpz3`, `get_center`, `get_radius`, `get_den`, `prec_at`, `haar_volume`), tight arithmetic
(`add`, `sub`, `neg`, `mul`, `mul_rat`, `div_rat`) and the set predicates and comparison
(`equal_set`, `overlaps`, `contains`, `contains_rat`, `compare`). Each function carries the
citation of the proof or convention it implements in a comment.

The tight rules are `docs/proofs/precision.md` Proposition 1 (sum, line 27) and Proposition 2
(product, line 34); the predicates are Proposition 3 (line 59); membership of a rational is
Lemma 1 (line 13); tightness is `docs/proofs/policies.md` Lemma 1 (line 43). The canonical
triple is computed as in `docs/proofs/policies.md` Summary 26, proof of the canonical column
(line 564), and `docs/conventions.md` Lemma 5.2.

Tests: `tests/test_fball.c` (20 tests, 399 checks) and `tests/test_fball_vectors.c` (9 tests,
34229 checks). The vector test runs the files `canonical`, `add`, `sub`, `neg`, `mul`, `scale`,
`predicates`, `compare`, `membership` of `tests/ref/vectors/` completely. The unit test covers
the literal rows of `docs/SPEC.md` 4.2 and 4.3, enclosure by enumeration, the tightness
witnesses, the valuation formula at each prime, canonical uniqueness field by field, aliasing,
operands of 4096 bits, the accessors and the domain cases.

Benchmark: `bench/bench_fball.c` adds the eight rows of `docs/PLAN.md` row 1.2 (tight add and
mul, latency chain and independent batch, word-sized and 4096-bit operands). Its name was added
to `bench/Makefile`.

## Files written

- `src/fball.c` (811 lines).
- `tests/test_fball.c` (1162 lines).
- `tests/test_fball_vectors.c` (404 lines).
- `bench/bench_fball.c` (409 lines).
- `bench/Makefile`: added the `bench_fball` program and its `run` and `clean` lines.
- `lanes/m1-fball/mutate_fball5.txt`: the final mutation run.
- `lanes/m1-fball/report.md`: this file.
- Run artifact `bench/results/2026-09-28T022748Z_fball.txt` (the one short run, see below).

No file outside the paths of the brief was changed, except the two lines of `bench/Makefile`
that the brief allows.

## Checks that were run

1. Build and tests, default compiler:

       make clean && make -j2 check

   Result: `check passed: all 6 test programs`.

       build/test_abi             2 tests,     13 checks, 0 failed
       build/test_fball          20 tests,    399 checks, 0 failed
       build/test_fball_vectors   9 tests,  34229 checks, 0 failed
       build/test_headers         8 tests,     58 checks, 0 failed
       build/test_scaffold        3 tests,     10 checks, 0 failed
       build/test_support        16 tests, 295152 checks, 0 failed

2. Address and undefined-behaviour sanitizers:

       make clean && make -j2 check SAN=1

   Result: `check passed: all 6 test programs`, the same six counts, no sanitizer report and no
   leak report.

3. Clang:

       make clean && make -j2 check CC=clang

   Result: `check passed: all 6 test programs`, the same six counts, no warning with
   `-Wall -Wextra -Wpedantic -Werror`.

4. Mutation testing of the source:

       make mutate FILES=src/fball.c LIMIT=200 SEED=20260928 JOBS=2

   Result (`lanes/m1-fball/mutate_fball5.txt`):

       the baseline passes (3.0 s)
       184 mutants in 319.1 s: 146 killed, 3 survived, 35 not compiled, 0 timed out, 0 excused
       mutate: FAILED: 3 mutant(s) survived

   The 35 "not compiled" are the tool's own broken tokens (for example `*` to `+` inside a
   declaration, `fmpz_gcd` to an undeclared `lcm`, or a dropped assignment that makes a
   variable used uninitialised under `-Werror`); they are not surviving claims. The three
   survivors are listed under "What is not done".

5. Benchmark, one short run (provisional, `quiet_machine: no`):

       make -C bench bench_fball
       ./bench/bench_fball --run --trials 5

   Result file `bench/results/2026-09-28T022748Z_fball.txt`. Machine 13th Gen Intel Core
   i7-1365U, gcc 13.3.0, FLINT 3.0.1, GMP 6.3.0, flags `-O2 -g`, pinned to cpu 2, 5 trials,
   batch length 256, checksum 23056. Per-operation times in ns (min / median / max):

       fball_add_chain_word    latency_chain      330.30 /    332.96 /    337.89
       fball_mul_chain_word    latency_chain     2537.42 /   4427.82 /   6027.41
       fball_add_chain_4096    latency_chain     1650.16 /   1678.64 /   1683.92
       fball_mul_chain_4096    latency_chain     2638.55 /   2809.26 /   3063.55
       fball_add_batch_word    independent_batch  347.26 /    352.48 /    356.25
       fball_mul_batch_word    independent_batch 1086.38 /   1088.75 /   1124.82
       fball_add_batch_4096    independent_batch 54177.58 / 54224.28 /  56040.69
       fball_mul_batch_4096    independent_batch 106910.10 / 146371.17 / 147382.02

   The chain rows start at A,H,d of 59 to 60 bits (word) or 4093 to 4096 bits (4096) and the
   second operand is the exact 3; the chain feeds its own result back, so the operand grows
   during the run. The batch rows use independent random pairs of the stated size. The batch
   `add` checksum is 0 because the sum of two random balls with coprime radii is `Zhat`
   ((A, H, d) = (0, 1, 1)); the timed work is the real add. These numbers are provisional: the
   machine is shared.

## Red-green log

The implementation was written before the two test files in this resumed session; the first
runs of the tests were the red runs. They found two real defects in the tests, both fixed:

- `tests/test_fball_vectors.c` first reported 212 failing checks on `canonical.jsonl`: the
  canonical vector stores the result as a JSON array `[A, H, d]`, and the comparison helper
  expected an object. Fixed with `ball_equals_array`.
- `tests/test_fball.c` first reported 1 failing check in `canonical_predicate`: the test
  assumed that two different representations of one set could both be canonical and still not
  identical, which contradicts Lemma 5.2. The assertion was replaced by the correct claim.

After that both files pass. Mutation testing (above) is the stronger evidence that the tests
pin the code.

## What is not done

**Local backend (ADF_LOCAL).** `docs/PLAN.md` row 1.2 scopes this lane to the global backend;
the local backend is work package 1.8. `include/adelefeld/fball.h` nevertheless writes local
semantics in several comments. This lane does not read `mctx` (its type is incomplete in the
public header and its accessors are implemented by lane m1-modctx). A HEADER-FINDING lists the
functions below.

**The three surviving mutants.** They are provably equivalent and need an entry in
`tools/mutate/equivalent.txt`, which is not in this lane's ownership. With the final line
numbers of `src/fball.c`:

    src/fball.c:556:5  swap_args | fmpq_gcd(g, g, t3) -> fmpq_gcd(g, t3, g): gcd is commutative
    src/fball.c:584:5  swap_args | fmpq_mul(r, r, N) -> fmpq_mul(r, N, r): multiplication is commutative
    src/fball.c:612:5  swap_args | fmpq_mul(r, r, N) -> fmpq_mul(r, N, r): multiplication is commutative

Two earlier equivalents were removed by rewriting `fmpz_sgn(d) < 0` as `fmpz_sgn(d) == -1` and
by testing `d = 0` and `H = 0` in `canonicalise`.

## Findings against the specification

### HEADER-FINDING 1: the local cases of `fball.h`

`docs/PLAN.md` row 1.2 and the brief scope this lane to `ADF_GLOBAL`, while `fball.h` documents
local results for `add`, `sub`, `neg`, `mul`, `mul_rat`, `div_rat`, `set`, `is_canonical`,
`identical`, `get_fmpz3` and `get_center`. This lane cannot read the block list of an
`adf_modctx_struct` without the accessors of lane m1-modctx. The most defensible reading is
therefore: implement predicate G fully, and do not write an output that would be wrong. The
resulting behaviour, all covered by `tests/test_fball.c::arithmetic_local_input_untouched`,
`local_shaped_is_canonical`, `identical_local_guard` and `predicates_local_and_radius_edges`:

| Function | Local input |
|---|---|
| `is_exact`, `get_radius`, `get_den`, `prec_at`, `haar_volume` | correct (they read only `H` and `d`, which the local predicate fixes) |
| `clear`, `swap`, `canonicalise` | correct (no context block is read) |
| `add`, `sub`, `neg`, `mul`, `mul_rat` | output untouched (the function is `void`; the header gives no status) |
| `div_rat` | `ADF_UNSUPPORTED`, output untouched |
| `set` | output untouched (residue array cannot be copied) |
| `identical` | fields and context pointer compared, residue arrays not compared |
| `is_canonical` | structural part checked (`mctx != NULL`, `res != NULL`, `A = 0`, `d >= 1`, `H >= 1`), `H = K` and the residue ranges not |
| `get_fmpz3`, `get_center` | output untouched (they need CRT recombination) |
| `equal_set`, `overlaps`, `contains`, `contains_rat`, `compare` | return 0 (or `ADF_CMP_DIFFERENT`), the fail-closed answer |

Work package 1.8 must replace the second half of this table.

### HEADER-FINDING 2: `prec_at` and the place functions

`adf_fball_prec_at` needs `adf_place_is_archimedean` and `adf_place_prime_get` of lane m1-rat.
Those are not in this worktree. `src/fball.c` declares the two as weak references and returns
`ADF_UNSUPPORTED` when they are absent; `tests/test_fball.c` supplies weak stand-ins (the
natural encoding: archimedean opaque = 0, prime p at opaque = p) so that the precision logic is
tested here. After the lanes are merged the strong definitions bind, and `adf_fball_prec_at`
uses the real places. No formula of the header is changed; the stand-ins are test-only.

## Optimisation notes (COMMON-C rule 7)

- The arithmetic computes the centre and radius as `fmpq` and then calls
  `adf_fball_set_center_radius`, so it pays a common-denominator `lcm`, a canonicalisation
  `gcd`, and the `fmpq` gcds. The direct `fmpz` formulation of `docs/proofs/precision.md`
  Propositions 1 and 2 would avoid part of this. The simple version was written on purpose.
- `adf_fball_mul` forms three `fmpq` products and takes two `fmpq_gcd`s. The tight radius is
  the gcd of three rationals, which is inherently two gcds.
- `canonicalise` and `set_fmpz3` both copy the raw triple into temporaries before `fb_store`;
  this is what makes aliasing safe, and it is one extra copy of two integers.

## Sources pending

None. All cited statements are on disk: `docs/proofs/precision.md`, `docs/proofs/policies.md`,
`docs/conventions.md`, `docs/SPEC.md`, and the reference `tests/ref/adfref/fball.py`,
`membership.py`. No external source is quoted.
