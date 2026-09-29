# S.2 slice 1 adversarial review

## What was done

Read `CLAUDE.md`, the S.2 rows of `docs/PLAN.md`, `docs/SPEC.md` 9.1 and 15.3, `docs/api-s.md` 4,
`docs/proofs/solvers.md` 3.1 to 3.3 and 3.11 to 3.13, the header, source, tests, and the author's report.
Built the library. Wrote an oracle from planted distinct factors and direct enumeration modulo powers of `p`.
Wrote separate probes for hand-built lists, aliasing, limits, and the width of the public prime type.

## Findings

### 1. BLOCKER under the brief's hand-built-list rule: composite place accepted

Input: `f = g = X^2 + 3`; `L.place.opaque = 4`; scope SEED; `n = 1`, `nu = 0`, `complete = 0`,
`reduced = 0`; certificate `(a, K, s) = (1, 1, 0)`; all pointers and lengths consistent.
`adf_place_prime(4)` returns `ADF_DOMAIN = 7`, but `adf_rootlist_is_canonical(L)` and
`adf_rootlist_verify_entries(L, f)` both return 1. The code tests only `p >= 2` in
`src/roots.c:273-276`; it then uses 4 as if it were prime. The claimed place does not exist.
Even if the ball is read as `1 + 4 Z_2`, it contains no root: squares modulo 8 are 0, 1, or 4,
so `x^2 + 3` is never 0 modulo 8. The verifier's R2 and R3 checks pass because `g'(1) = 2`
is nonzero modulo 4 and `g(1) = 4` is zero modulo 4.

Reproduce: `timeout 10 lanes/s2-review/forged_place`.
Output: `prime_constructor=7 canonical=1 verified=1`; exit 0.

Scope: `include/adelefeld/roots.h:60-66` excludes places not made by `place.h`. The finding
applies to the review brief's explicit rule that a hand-built value accepted by
`adf_rootlist_is_canonical` is a fair input. It does not arise from a valid public place constructor.

### 2. MAJOR: allocation occurs before a derived-precision LIMIT

Input: `f = X(X - 2^4194304)`, `p = 2`, seed `a = 0`, `prec_p = 1`, and an initialised `L`.
Here `g = f`, `g(0) = 0`, and `g'(0) = -2^4194304`. Thus the strong form holds,
`s = 4194304`, and `K = s + 1 = 4194305`. The bound is
`2 K bits(2) = 16777220 > ADF_ROOTS_BITS_MAX = 16777216`, so `ADF_LIMIT` is due.
The function returns `ADF_LIMIT = 10` and leaves `L` unchanged, but makes 16 calls to
FLINT's allocation hooks before returning. `src/roots.c:591-595` allocates and normalises
before `src/roots.c:533-537` checks the derived `K`. This violates the “before any allocation”
rule of `docs/SPEC.md:903` (S-D18). The header's narrower promise about checking before
forming powers of `p` is met.

Reproduce: `timeout 120 lanes/s2-review/limit_alloc`.
Output: `m=4194304 status=10 FLINT_allocations_before_return=16 L_unchanged=1`; exit 0.

## Findings against the specification

### 3. MAJOR, already noted by the author: S-D10 cannot be exposed by this place type

`docs/SPEC.md:895` says the library accepts every prime. The public constructor accepts an
`ulong`, and `include/adelefeld/place.h:34` explicitly says primes at or above `2^64` cannot
be passed. The program tests `2^64 + 13`: `fmpz_is_prime` returns 1, while conversion to
`ulong` yields 13 and the constructor makes the place of 13. FLINT documents that return 1
from `fmpz_is_prime` means proven prime at `refs/src/flint-3.0.1/fmpz.rst:1529-1532`.
The hidden core is not a public replacement for the missing place value. This is the
`HEADER-FINDING` already stated in `lanes/s2-slice1/result.md`; it is not a new discovery.

Reproduce: `timeout 30 lanes/s2-review/prime_width`.
Output: `prime_bits=65 flint_is_prime=1 ulong_bits=64 converted_word=13 constructor_status=0`;
exit 0.

No counterexample was found to the stated mathematical propositions in `solvers.md` section 3.

## Attacks without a further result

- `seed_oracle.c` built `g` from distinct planted factors, independently of the library's
  squarefree routine. It covered repeated linear and quadratic factors, content, a leading
  coefficient divisible by `p`, roots divisible by a high power of `p`, and `p = 2, 3, 5`.
  For seeds `-24` to `24` and precisions 1 to 5: 1,715 calls, 605 `OK`, 1,110
  `NOT_DETERMINED`, 175 `OK` calls with `s > 0`, and 257 direct-path cases with `k0 >= K`.
  All 605 `OK` balls were enumerated modulo `p^(K+s+2)`; zero root-count, seed-class,
  status, polynomial, flag, or certificate mismatches. A case failed if the strong-form
  status differed, metadata differed, or the seed class did not have exactly `p^s`
  approximate roots, all in the returned ball. No enumeration was skipped.
- `edge_probe.c`: 24 checks, zero failures. It covered `p = 2` with `s = 1`, an exact
  negative root, a seed `3 + 2^4096`, aliasing both `f = L->g` and `a = L->a`, reuse of
  `L` at another prime, a prime near `2^64`, one precision above its limit, a constant,
  and zero `f`. A failed status, centre, `K`, `s`, verifier result, or unwanted write to
  `L` would have failed a check.
- `verifier_probe.c`: ten hand-built valid-prime list cases, zero failures. It changed
  the centre, `K`, `s`, polynomial content, `reduced`, `complete`, an exponent above the
  verifier limit, and an overlapping unresolved class. A false acceptance or rejection
  relative to the entry rules would have failed.
- No valid-prime false certificate, reachable `flint_abort`, word overflow, or failed
  aliasing call was found in these probes. This is not a proof that none exists.

## Checks run

All executable probes were run under `timeout`. Each compile used
`cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror FILE build/libadelefeld.a`
`-lflint -lgmp -lm -o OUTPUT`, with `FILE` and `OUTPUT` in this lane.

| Command | Result |
|---|---|
| `timeout 180 make -j2` | Exit 0; `build/libadelefeld.a` built. |
| Compile `forged_place.c`; `timeout 10 lanes/s2-review/forged_place` | Exit 0; `7, 1, 1`. |
| First compile of `seed_oracle.c` | Exit 1: used nonexistent `fmpz_divisible_ui`. Replaced it. |
| Compile and `timeout 120 lanes/s2-review/seed_oracle` | Exit 0; 1,715 calls, zero failures. |
| Recompile and rerun `seed_oracle` after adding `reduced` and counters | Exit 0; 605 enumerations, zero failures. |
| Compile `limit_alloc.c`; `timeout 120 lanes/s2-review/limit_alloc` | Exit 0; 16 allocations before LIMIT. |
| Compile `edge_probe.c`; `timeout 30 lanes/s2-review/edge_probe` | Exit 0; 20 checks, zero failures. |
| Recompile and rerun `edge_probe` after constant and zero cases | Exit 0; 24 checks, zero failures. |
| First compile and `timeout 30 lanes/s2-review/prime_width` | Compile exit 0; run exit 1. |
| Correct expected status to `ADF_OK`; recompile and rerun `prime_width` | Exit 0; 65-bit prime maps to 13. |
| Compile `verifier_probe.c`; `timeout 30 lanes/s2-review/verifier_probe` | Exit 0; ten cases, zero failures. |

The first `prime_width` run failed only because the probe expected `ADF_DOMAIN` for 13;
13 is prime, so the constructor returned `ADF_OK`. That result made the narrowing example
stronger and the expected value was corrected. The first `seed_oracle` compile did not run.

## Files written

`lanes/s2-review/forged_place.c`, `seed_oracle.c`, `limit_alloc.c`, `edge_probe.c`,
`prime_width.c`, `verifier_probe.c`, their same-named compiled executables, and this report.
The required `make -j2` also generated `build/libadelefeld.a` and its objects.

## Not done

No sanitizer or leak run, no full random differential fuzz, and no exhaustive large-prime or
large-coefficient search. The probes stayed under the lane's three-minute command bound.

## Sources pending

None. The modular nonexistence argument and the limit calculation above are direct proofs.
