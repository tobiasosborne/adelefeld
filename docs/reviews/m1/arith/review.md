# Review of milestone 1, reviewer `arith`: rat.c, fball.c (global), adele.c, recon.c

BLOCKER FOUND: 1 BLOCKER, 2 MAJOR, 3 MINOR.

Worktree: `/home/tobias/Projects/adelefeld/.claude/worktrees/agent-a6ed61fcce34217ac`, at commit a19cd71.
Reproducers: `docs/reviews/m1/arith/checks/`. Build a C reproducer from the root of the worktree after `make -j2`
with `sh docs/reviews/m1/arith/checks/build.sh <name>`; `build_san.sh <name>` builds it together with `src/*.c`
under ASan and UBSan. Every `.out` file next to a program is the output of the run quoted below.
Baseline: `make -j2 check` in this worktree passes, 30 test programs.

## Findings

### R1 (BLOCKER) adf_adele_reconstruct answers ADF_OK with a value outside the real ball

- File and line: `src/recon.c:95` (`fmpz_get_ui(exp)`) and `src/recon.c:106` (`-fmpz_get_si(exp)`), in
  `fmpq_set_dyadic`, called from `adf_adele_reconstruct` at `src/recon.c:217-218`.
- Input: the adele `x = (I ; 6)`, `I` the exact real ball `3 * 2^(2^64 + 1)` (built with `arb_set_si` and
  `arb_mul_2exp_fmpz`), finite part the exact 6. `arb_is_finite(I) = 1`, `adf_adele_is_canonical(x) = 1`: the input
  is admitted (`adele.h`, predicate of conventions 5.5; `recon.h` has no other precondition).
- Requirement: `recon.h:40-42` and conventions 6.8 (CV-51): the candidates are the rationals in
  `[mid - rad, mid + rad]` and in the finite ball; one candidate gives `ADF_OK`, none `ADF_NO_SOLUTION`. Here the
  real interval is the single point `3 * 2^(2^64+1)`, which is not 6: the answer is `ADF_NO_SOLUTION`.
- Behaviour: `arb_get_interval_fmpz_2exp` returns `a = b = 3`, `exp = 2^64 + 1` correctly; `fmpz_get_ui(exp)`
  returns the low limb, 1; the interval becomes `[6, 6]`; the function returns `ADF_OK` with `q = 6`. The same for
  `2^-(2^64+3)` with finite part `1/8` (`fmpz_get_si` gives -3; `ADF_OK`, `q = 1/8`) and for `2^(2^64)` with finite
  part 1 (shift 0; `ADF_OK`, `q = 1`). For `exp = -2^63` the negation at line 106 is signed overflow: the
  sanitizer build stops with `src/recon.c:106:42: runtime error: negation of -9223372036854775808 cannot be
  represented in type 'long int'`.
- The comment at `src/recon.c:84-86` says such an exponent "is not reachable in practice"; it is one call of
  `arb_mul_2exp_fmpz` away, and the header admits it. The lane report (`lanes/m1-recon/report.md`, "Nothing is
  enumerated ... never overflows") does not cover this.
- Reproducer: `checks/recon_huge_exp.c`, output `checks/recon_huge_exp.out`:

      A (3 * 2^(2^64+1), fin = 6): arb_is_finite=1 arb_is_exact=1 adf_adele_is_canonical=1
        arb_get_interval_fmpz_2exp: a = 3, b = 3, exp = 18446744073709551617; fmpz_get_ui(exp) = 1, ...
        status = OK (0), q = 6
      B (2^-(2^64+3), fin = 1/8): ...
        status = OK (0), q = 1/8
      B' (2^(2^64), fin = 1): ...
        status = OK (0), q = 1

  and `checks/recon_huge_exp.san.out` (the UBSan report above). Exponents in `[2^62, 2^64)` and `[-2^63, -2^62]`
  gave the right status in the five cases run (`A2`, `C2`, `C3`, `B2`, `D` of the same output), although
  `checks/probe_mul_2exp.out` shows that `fmpz_mul_2exp(t, 1, 2^62 + 1)` returns 2 on this machine; I do not claim a
  wrong result there.
- A repair that needs no big integer: compare the end points with the candidates through `arf`/`fmpz` exponent
  arithmetic, or reject (with `ADF_LIMIT`, which conventions 3.2 allows for this class) an exponent whose shift does
  not fit.

### R2 (MAJOR) adf_adele_reconstruct aborts the process on an admitted ball instead of returning a status

- File and line: `src/recon.c:216` (`arb_get_interval_fmpz_2exp`) and `src/recon.c:95` (`fmpz_mul_2exp`).
- Input: (1) `I = 2^(2^36)` exact, finite part the exact 1; (2) `I = 1 +/- 2^(-2^36)`, finite part the exact 1. Both
  finite, both admitted. Answers: (1) `ADF_NO_SOLUTION`; (2) `ADF_OK`, `q = 1`. Each is decidable from a few words.
- Requirement: conventions 3.2, row "Reconstruction and solvers": `OK`, `NO_SOLUTION`, `NOT_UNIQUE`,
  `NOT_DETERMINED`, `LIMIT`; conventions 4.4: only a non-canonical input is undefined. `recon.h:20` and `:42` give
  the cost as "a constant number of divisions ... with the end points converted exactly to rationals", with no
  bound.
- Behaviour: under `ulimit -v 2000000` both calls abort in GMP (`Cannot reallocate memory (old_size=16
  new_size=8589934600)`, exit 134); without a limit the call tries to allocate 8 GiB, or 2^62 bits for an exponent
  of `2^62`. Ground truth: the code cites `arb.rst:468-470` only for the abort on an infinite or NaN ball; the same
  passage (`refs/src/flint-3.0.1/arb.rst:468-474`) says that the function aborts, or allocates "a huge amount of
  memory", when the exponent difference is large. The code does not guard against it.
- Reproducer: `checks/recon_big_alloc.c`, output `checks/recon_big_alloc.out`:

      case 1: arb_is_finite=1 adf_adele_is_canonical=1; calling adf_adele_reconstruct
      GNU MP: Cannot reallocate memory (old_size=16 new_size=8589934600)
      exit=134
      case 2: arb_is_finite=1 adf_adele_is_canonical=1; calling adf_adele_reconstruct
      GNU MP: Cannot reallocate memory (old_size=16 new_size=8589934600)
      exit=134

  Case 3 (exponent `2^62`) returned the right status, `NO_SOLUTION`; see the last item of "Not examined".

### R3 (MAJOR) adf_adele_div_rat and adf_cadele_div_rat at prec = 1 return ADF_OK with a non-finite real part

- File and line: `src/adele.c:318-319` (`arb_set_fmpq(t, tq, prec)`, `arb_div`), `src/adele.c:543-544` (acb).
- Input: `x = (1 ; 1)` (`adf_adele_set_si(x, 1)`), `q = 1/3`, `prec = 1`. No document restricts `prec`: conventions
  2.2 says "the real working precision in bits, as in arb"; neither `adele.h` nor `arb.rst` states a minimum.
- Requirement: `adele.h:27-28` "A result must satisfy conventions 5.5, whose real part must be finite"; conventions
  5.5 predicate `arb_is_finite(inf)`; conventions 4.4 (CV-08): a non-finite ball produced from finite inputs is
  "never stored; the function returns `ADF_NOT_DETERMINED`". `adele.h:28-30` claims that arb operations on finite
  balls give finite balls.
- Behaviour: `arb_set_fmpq(1/3, 1)` is `0.25 +/- 0.25`, which contains 0; `arb_div` gives `nan +/- inf`; the
  function returns `ADF_OK` and `adf_adele_is_canonical(z) = 0`. The same for the cadele. (The enclosure itself
  holds: the whole line contains `3`.) At `prec >= 2` the converted ball excludes 0 and the result is finite.
- Reproducer: `checks/adele_prec1.c`, output `checks/adele_prec1.out`:

      prec 1: arb_set_fmpq(1/3) = 0.2500000000 +/- 0.25000; adele div_rat status OK, inf = nan +/- +inf,
              adf_adele_is_canonical = 0; cadele status OK, is_canonical = 0
      prec 2: ... adele div_rat status OK, inf = 4.000000000 +/- 4.6667, adf_adele_is_canonical = 1; ...

- Either a documented precondition `prec >= 2` (conventions 2.2 and `adele.h`) or the status of CV-08.

### R4 (MINOR) four excused mutants of adele.c are not equivalent: arb_mul and acb_mul are not symmetric

- File and line: `tools/mutate/equivalent.txt:54` (`src/adele.c:223`, `arb_mul`), `:58` (`adele.c:285`), `:61`
  (`adele.c:464`, `acb_mul`), `:65` (`adele.c:514`); the block comment at `equivalent.txt:43-51`.
- Requirement: `equivalent.txt:8-10`: the reason "has to say why the mutant computes the same thing as the original
  for every input". The reasons say "arb_mul sets z to that product rounded to prec bits ..., so the same value is
  written" and the same for `acb_mul`.
- Behaviour: in 200000 random pairs (`arb_randtest`, `acb_randtest`, prec 2 to 201), `arb_mul(x, y)` and
  `arb_mul(y, x)` differ bitwise (same midpoint, different radius) in 38216 pairs, `acb_mul` in 55563. `arb_add`,
  `acb_add`, `adf_fball_add` and `adf_fball_mul` never differ (0 of 200000). Both orders are valid enclosures, so no
  result is wrong; the excuse is false as written, and the mutant is killable in principle (by `adf_adele_identical`
  against a pinned ball).
- Reproducer: `checks/swap_equiv.c`, output `checks/swap_equiv.out`:

      arb_mul differs at prec 35:
        x = 0.54873595200479030609 +/- 1.1642e-10
        y = 0.52426910400390625000 +/- 2.2702e-16
        x*y rad = 7.5584996703410844976e-11
        y*x rad = 7.5584996594990627727e-11
        midpoints equal: 1
      cases 200000: arb_add differs 0, arb_mul differs 38216, acb_add differs 0, acb_mul differs 55563,
      adf_fball_add differs 0, adf_fball_mul differs 0

### R5 (MINOR) all 18 excuses of src/fball.c in equivalent.txt name lines that have moved

- File and line: `tools/mutate/equivalent.txt:14-16`, `:30-34`, `:71-80` (entries `src/fball.c:54`, `141`, `142`,
  `214`, `468`, `469`, `497`, `551` to `556`, `582`, `584`, `610`, `612`, `684`).
- Requirement: `mutate.py` matches an excuse by `(file, line, kind)` alone (`tools/mutate/mutate.py:638-657`,
  `:849`); an entry is valid only while the named line holds the named call.
- Behaviour: after the merge of the local backend none of the 18 lines holds the call its reason names (for example
  `fball.c:469` is now `}` and `fball.c:610` is `adf_fball_add(...)`'s signature). Two consequences: the real
  commutative swaps (now at, e.g., `fball.c:74`, `637`, `638`, `742` to `747`) are no longer excused, so a run of
  `make mutate FILES=src/fball.c` reports them as survivors; and `fball.c:142:drop_call` now excuses the removal of
  `fmpq_set_fmpz_frac(N, x->H, x->d)` in `fb_radius`, a non-equivalent mutant (it is very likely killed by the
  tests, so the excuse would not fire; I did not run it). The entries of `rat.c`, `adele.c` and `recon.c` (21) are
  on their lines.
- Reproducer: `checks/stale_excuses.py`, output `checks/stale_excuses.out`, last line:
  `entries: 39, stale (the named call is not on the named line): 18`.

### R6 (MINOR) citations and comments that do not say what the code claims

- `src/rat.c:15` and `src/rat.c:225` cite `fmpq.h:209` for `fmpq_sub`; line 209 declares `fmpq_add_fmpz`, `fmpq_sub`
  is at `/usr/include/flint/fmpq.h:212`.
- `src/adele.c:30-32` cites `arb.rst:11-14` for "The result of an (approximate) operation ... contains the result";
  the sentence is at `arb.rst:9-11` (line 14 begins the paragraph on precision). `src/adele.c:110-112` cites
  `arb.rst:23-24` for "Arithmetic operations done on exact input ..."; the sentence is at `arb.rst:25-26`.
- `src/recon.c:40-43` cites `arb.rst:468-470` as the abort condition "on an infinite or NaN ball" and omits the rest
  of the same sentence and the warning at `arb.rst:472-474` (see R2).
- Stale comments: `src/recon.c:50-61` says the local backend "is not built yet" and that `adf_fball_get_radius`
  reads 0 for a local value; `src/fball.c:546-549` now returns `K/d`. `src/fball.c:49-53` says the place functions
  "are not in this worktree" and keeps `#pragma weak` on them; `src/place.c` is merged, so the weak references are
  dead code (a program that obtains a place must call `adf_place_prime` or `adf_place_inf`, which pulls `place.o`).
- Reproducer: `checks/citations.py`, output `checks/citations.out` (the cited lines as they are on disk).

## Examined and found correct

Oracles: exact arithmetic with `fractions.Fraction` in Python, written for this review; canonical triples computed
by Lemma 5.2 of conventions (`d0 = lcm(den a, den N)`, `A0 = a d0 mod H0`), not by the gcd route of `fball.c`; radii
of results computed as the generator of the subgroup of Q spanned by enumerated differences, not by the formula.
The oracle was checked against injected errors (a doubled product radius and a flipped `overlaps`: 200 of 200 lines
flagged).

- `src/fball.c`, global backend, every public function: `set_fmpz3` (raw triples, negative `d`, `H = 0`, `A = 0`),
  `add`, `sub`, `neg`, `mul`, `x * x`, `mul_rat`, `div_rat` (also `q = 0`: `NOT_UNIT`, output untouched),
  `equal_set`, `overlaps`, `contains` both ways, `contains_rat`, `compare`, `get_center` (in `[0, N)`),
  `get_radius`, `get_den`, `haar_volume`, `prec_at` at 2 and 3 (exact input: `DOMAIN`, `*e` untouched), `is_exact`,
  canonical outputs, and aliasing `(x, x, y)`, `(y, x, y)`, `(x, x, x)` of add, sub, mul, and `(x, x)` of neg,
  mul_rat, div_rat against the unaliased result with `adf_fball_identical`. Enclosure: every enumerated product/sum
  of lattice points lies in the output; tightness: the output radius equals the enumerated generator. Predicates by
  enumeration of lattice points over one full period (inside: `a` and `a + N` in the other ball). 55000 cases
  (`fball_fuzz` with operand sizes 3, 6, 80 and 200 bits; zero, negative and exact centres, different denominators,
  one third exact inputs), 0 failures, 0 aliasing differences (`checks/fball_fuzz.out`).
- `src/rat.c`, every function: `set_fmpq` on non-canonical input (common factors, negative denominator), `set_fmpz2`
  (`den = 0`: `DOMAIN`, untouched), `add`, `sub`, `mul`, `neg`, `div` and `inv` (zero: `NOT_UNIT`, untouched),
  `sgn`, `equal`, `is_zero`, canonical outputs, aliasing including `div(a, a, a)`. 70000 cases at 8 and 200 bits, 0
  failures (`checks/rat_fuzz.out`).
- `src/adele.c`, real coordinate of `add_rat`, `mul_rat`, `div_rat` and `set_rat`, with and without aliasing of the
  output with the input, at `prec` 2, 3, 4, 7, 10, 53, 128: the output interval contains the exact image of the
  input interval (end points read exactly), the finite part is the exact set image, the output is canonical; the
  claim of `adele.h:102-103` (a dyadic `q` with an odd mantissa of at most `prec` bits gives an exact ball) holds.
  40000 lines, 0 failures (`checks/adele_prec.out`). `div_rat` at `prec = 1`: R3.
- `src/recon.c`: `adf_fball_reconstruct` with intervals whose end points are lattice points, of width exactly `N`
  and `2N`, `lo = hi`, `lo > hi`, exact balls, and the output aliased with `lo` and with `hi`;
  `adf_adele_reconstruct` with random finite arbs (exponents up to 2^6 bits) and arbs centred on a candidate, radius
  zero or not. The oracle walks the lattice from below `lo`; the real interval is read with `arf_get_fmpq` of
  midpoint and radius, not with `arb_get_interval_fmpz_2exp`. Output untouched (marker `-99/7`) on every failure
  status. 120000 calls at 3, 6 and 60 bits, 0 failures (`checks/recon_fuzz.out`). Huge exponents: R1, R2.
- Surviving mutant `src/recon.c:111` (drop `fmpq_canonicalise(q)`, a survivor in
  `lanes/m1-testgaps/mutate-recon*.log`, not in `equivalent.txt`): the mutated file (`checks/recon_mut111.c`) gives
  byte-identical output to the original on 60000 fuzz lines (`checks/recon_mut111.out`), so it is observationally
  equivalent here. It is not equivalent by the rule of `equivalent.txt:8-10`: without the call, non-canonical `fmpq`
  reach `fmpq_cmp`, `fmpq_sub`, `fmpq_div`, which `fmpq.rst:21-26` forbids. The call should stay, and the survivor
  should be excused with that reason, not "same result for every input".
- Excuses `recon.c:90` (both), `recon.c:122`, `recon.c:176-177`, `rat.c:222`, `rat.c:238`, and the `arb_add`,
  `acb_add`, `adf_fball_add`, `adf_fball_mul` swaps of `adele.c`: the lines match and the claims hold (for the arb
  and fball swaps: 0 differences in 200000 pairs, `checks/swap_equiv.out`; for `recon.c:122`, `fmpq_cmp` returns
  only -1, 0, 1 and the rest of the function answers `NO_SOLUTION` for `lo > hi`, which the fuzz exercises).
- Sanitizers: `fball_fuzz`, `adele_prec`, `recon_fuzz`, `rat_fuzz` built with `src/*.c` under ASan and UBSan
  (`-fno-sanitize-recover=undefined`): 5000, 3000, 5000 and 5000 cases, no report, oracle failures 0
  (`checks/san_runs.out`). `valgrind --leak-check=full` on `fball_fuzz 300 9 40`: 0 errors, no leak
  (`checks/valgrind_fball.out`).
- Proofs: `precision.md` Propositions 1 to 3 and Lemma 1, 2 read against `fb_*` functions (the product radius
  `gcd(aM, bN, NM)` with `fmpq_gcd`, which is non-negative by `fmpq.rst:512-513`); `quotient.md` Proposition 11
  against `adf_fball_reconstruct`; conventions 4.1, 4.3, 4.4, 5.1, 5.2, 5.5, 6.8.

A remark that is not a finding: `adf_fball_contains(x, y)` means "x inside y" (SPEC 4.2, first inside second), while
`adf_fball_contains_rat(x, q)` means "q in x" and FLINT's `arb_contains(x, y)` means "y inside x". Header, SPEC and
code agree; the two argument orders are a trap for users.

## Not examined

- The local backend of `src/fball.c` (`fb_local_*`, the local branches of `add`, `sub`, `neg`, `mul`, `mul_rat`,
  `is_canonical`, `identical`, `set`); another reviewer's. `src/cap.c`.
- The complex coordinate of `adf_cadele` beyond the `prec = 1` probe of R3: no enclosure fuzz of `acb` add_rat,
  mul_rat, div_rat.
- `prec <= 0`.
- A mutation run of any file (it exceeds the three-minute limit); R5 is from the line contents, not from a run.
- Whether the project's tests kill the mutant now excused by `fball.c:142:drop_call`.
- The benchmark rows and the Julia layer.
- Why `fmpz_mul_2exp` wraps at shifts of `2^62` and more (`checks/probe_mul_2exp.out`) while the reconstructions of
  R1 with such exponents came out right.
