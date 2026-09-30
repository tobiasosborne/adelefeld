# Lane r-review2 result: bug hunt through the second slice of the real roots

Worktree at 751d2c1. Nothing outside `lanes/r-review2/` changed. Build: `make -j2 BUILD=lanes/r-review2/build`
(the build directory was deleted afterwards; the programs were linked to /tmp/x, none is left in the lane).
Oracle: `oracle.py`, an own Sturm chain over Q (python-flint `fmpq_poly` only for rational arithmetic; chain,
sign variations, root counts in (lo, hi] are written there). Driver `drv.c` calls the public `adf_roots_real`,
both verifiers, and prints exact end points.

## Defects

BLOCKER: none found. MAJOR: none found.

MINOR 1 (cost, not correctness). Degree at most 50 and at most 10^4 bits, more than 5 s. The time is FLINT's
`fmpz_poly_num_real_roots`, not the new isolation:
- Random dense degree 50, 10000-bit coefficients (`fmpz_poly_randtest`), prec 2: `adf_roots_real` 8.2 s;
  the same count alone, 8.4 s. 5000 bits: 5.4 s, count alone 6.4 s. Command:
  `cc ... lanes/r-review2/prof.c ...; /tmp/x/prof 50 10000 1` (source `prof.c`).
- 50 real roots (2^190 X ... : `prod_(i=1..50) (3*2^190 X - (2^190 + i))`, 9598 bits) at prec 2: `adf_roots_real`
  9.6 s; plain count 6.4 s; `adf_roots_real_isolate_counted` plus refinement alone 0.22 s, `finish` 0.02 s
  (`prof3.c`, `mkcl.py`). At prec 1000: 16.3 s in one run (count 11.8 s), isolation and refinement 0.36 s,
  certificate 0.17 s (timings on a loaded laptop, two processes).
  The scaled count of R9 does not help here: the mean of the roots is not a rational of 16 bits, and the
  scale is about -1.
- The scale trick is used for negative scale only. Roots i 2^-150 (i = 1..50): 0.04 s. Roots i 2^150: 1.13 s
  (plain count; `mkasym.py`, `prof2.c`). At k = 190: 0.04 s against 1.63 s.
- `adf_rootlist_verify_complete` recomputes the plain count from f. For the input `prod_(i=1..50) (2^190 X - i)`
  `adf_roots_real` takes 0.10 s and the whole `drv` line (with both verifiers) 7.66 s; the plain count is
  about 7.6 s. Not a wrong result; the verifier is independent of the scaled count, which is what it is for.

MINOR 2 (cost at high precision). Degree 50, small coefficients, prec 20000: `prod_(i=0..24) (X^2 - 2(i+1))`
23.9 s (isolation and refinement 10.9 s, certificate 7.0 s in a second run); Wilkinson 50 plus 1: 38.8 s.
`X^50 - 2` (2 real roots): prec 16384 0.63 s, 65536 3.5 s, 262144 23.1 s, 2^21 (ADF_ROOTS_REAL_PREC_MAX):
did not end in 100 s. The header's measured `X^2 - 2` at 2^21 is 0.37 s in the report of r-slice2; the header does
not state the growth in the degree. Commands: `mkhp.py`, `prof2.c`, `prof3.c`.

MINOR 3 (only a remark). `ADF_LIMIT` for the roots 2^(1-M) (M = 16777216) and 2^-M, `ADF_OK` for 2^(2-M),
and OK for a root 2^(M-1), LIMIT for 2^M, exactly as roots.h says (`limit.py`, degree 1 and 2 with 16.7-million
bit coefficients, 0.2 to 4 s). No defect; recorded because it is the only place where LIMIT was reached.

## Item 1: the argument about the certificate

Claim: a value computed with `arb` decides no sign, end point or order in the output list, and the number of
balls is compared with a count that does not come from the isolation. Steps, `src/roots.c` at 751d2c1:

1. `adf_roots_real` (roots.c:1754). g = `normalise(f)` at 1768 (exact integer polynomial). The count is
   `real_count(g)` at 1775 (FLINT's `fmpz_poly_num_real_roots`, on g or on an equivalent H, 1704 to 1749). It is
   computed before and independently of the isolation, and is only passed to it (1780) as a stopping value.
2. Candidates: `adf_roots_real_isolate_counted` (roots_real.c:918). Every candidate ball is built from exact
   integers: an exact ball `arf_set_fmpz_2exp(mid, c, e)`, radius 0 (roots_real.c:987 to 992), or midpoint
   `(2c+1) 2^(k-1)` and radius `mag_set_ui_2exp_si(1, k-1)` (993 to 1000). `arf` mantissas are unbounded, the radius
   is a power of 2: no rounding. So the ball equals the closed interval `[c 2^k, (c+1) 2^k]`. Every `arb` use in
   roots_real.c (`local_ball` 168, `secant_grid` 190, `sign_2exp` 211) only steers where the next dyadic point
   is; the result of `sign_2exp` is either the arb's certified sign or the exact integer sign (216 to 229), and
   `secant_grid` returns 0 unless a unique integer is found (205), else the exact integer secant (qir_step 680
   to 691). Even a wrong filter would change only which cell the loop ends in.
3. `real_finish` (roots.c:1622). Line 1629: admissibility of each input ball by `real_ball_admissible`
   (1440, `arb_is_finite`, `arb_bits`, `arf_cmpabs_2exp_si`, `mag_cmp_2exp_si`: comparisons of exact
   binary numbers). This decides only `ADF_LIMIT`, no arithmetic on the ball. Line 1632: `m != count` gives
   `ADF_NOT_DETERMINED`; count is the argument of step 1, not `it.n`.
4. Lines 1636 to 1642: for each ball `arb_get_interval_fmpz_2exp` gives the exact end points `a 2^e`, `b 2^e` (arb.rst
   461 to 477, exact interval of the ball). `real_entry_ok` (1501): a = b needs `real_sign_at(g, a, e) == 0`;
   a < b needs `real_sign_at(g, a, e) * real_sign_at(g, b, e) < 0`, both by `real_sign_at` (1460): exact
   integer Horner, `fmpz_poly_evaluate_fmpz` for e >= 0, homogeneous integer Horner for e < 0. No arb value.
   The translate of 1516 to 1538 (degree >= 5, `fmpz_bits(a) > 4096`, both shifted mantissas below half the
   bits): with s = e + bits - 1 and origin = sgn(a) 2^(bits-1), a 2^e = sgn(a) 2^s + x 2^e, so
   g(a 2^e) = g(2^s (sgn(a) + x 2^(1-bits))). h = `scale_2exp(g, s)` (positive multiple of g(2^s Y), scale_2exp
   divides by the 2-content, flint scale_2exp.c) shifted by sgn(a) (Taylor shift, exact) is h(Y) with
   h(Z) = c * g(2^s (sgn(a) + Z)), c > 0; evaluating at Z = x 2^(1-bits) is `real_sign_at(h, x, 1 - bits)`;
   the same for y = b - origin. `_fmpz_poly_remove_content_2exp` divides by a positive power of 2. So the signs are
   those of g at the two exact end points: g itself, as claimed (docs R8 step 5). `use == 0` falls to 1540 to 1541
   on g.
5. A failing ball is widened once (1641, `real_widen` 1608): the same midpoint and radius doubled, or 2^-prec for
   radius 0 (exact `mag` operations). Nothing is rounded.
6. `real_balls_ok(g, out, m)` (1647, defined at 1549): for every stored ball (after widening) admissibility, exact
   end points again, `real_entry_ok` (the same exact test, on the widened end points), and order
   `arf_cmp(hi_prev, lo) < 0` on exact `arf` numbers built with `arf_set_fmpz_2exp` (1573 to 1576).
7. Accuracy, line 1649: `real_accurate` uses `arb_rel_accuracy_bits(x) >= prec` (1600). This is the one place
   where an arb-computed number decides a status (OK against NOT_DETERMINED) in the output. It cannot make a ball
   wrong (enclosure, sign changes and order are already decided); it decides only whether the S-D19 accuracy is
   claimed. Its exactness cannot be checked from the sources on disk: the arb sources of FLINT 3.0.1 are not
   under `refs/src/flint-src-3.0.1` (`[source pending: arb/rel_error_bits.c, arb/rel_accuracy_bits]`). Empirically:
   for 1000 outputs (`SLACK=0`), `rad <= |mid| 2^-prec` held exactly in rational arithmetic for every ball.
8. Conclusion: with a true count N, `ADF_OK` means m = N balls, each with an exact sign change or an exact zero at
   exact end points (so at least one root, and exactly one since g is squarefree by `normalise`), strictly ordered
   and disjoint; N disjoint balls hold at least N roots, all N roots of g, so each real root is in exactly one
   ball. Dependence not covered by the certificate: (a) FLINT's count N (S-D11, trusted); (b) the scaled count
   of `real_count` equals the plain count only by the mathematical equivalence R9 plus FLINT correct on H
   (`verify_complete_real` at 1829 recomputes the plain count on g); (c) `count == 0` gives the empty list with
   no test at all (`m = 0`, isolation skipped at roots_real.c:928), so the empty answer rests only on FLINT's
   count; (d) `normalise` squarefreeness (older code). A wrong filter or a wrong early stop can produce
   `ADF_NOT_DETERMINED` or `ADF_LIMIT` (a lost answer), not a wrong `OK` list. That held in the mutation run below.

Attack on the certificate (hidden entry `adf_roots_real_finish`, tests only): from true lists of about 2 900 random
inputs I built mutants (shift or resize either end point, drop, duplicate, swap, merge two balls into one,
insert a ball around a non-root, reversed ends, zero radius at a non-root, widen) and called the certificate
with the TRUE count. 15 099 mutants (seeds 1 to 5, and 323 at prec 4200 to 7000 with roots near 2^j (1 + eps 2^-3000),
which enter the translate at 1516 to 1538) gave 4 863 `OK`, 10 236 `NOT_DETERMINED`. Every `OK` list was checked with
my Sturm chain: n equal to the true count, exactly one root per ball, ordered: 0 wrong. The mutants that are
accepted are the benign ones (a ball that still holds exactly its root). `mutate.py`, `fdrv.c`.

## Item 2: the scaled count (R9)

Own Sturm count against the list count and n, on inputs of `item2.py`: 3 900 inputs in the completed runs (seeds 1, 2, 3, 11:
100 + 700 + 700 + 300; seeds 12 to 18: 7 x 300 = 2 100; the seed-4 run was cut by my own timeout and is not counted),
0 disagreements, all statuses `OK`. Generators: roots near p/q with
spread 2^-k; huge roots; tiny roots (negative scale); translates by 2^100 to 2^3000; repeated factors, root 0,
content 10^60; negative leading coefficient; symmetric pairs about a small rational (center exactly 1/2, 1/3,
p/255 ...) plus a root at the center itself, plus complex factors `(q 2^k X - p 2^k)^2 + i`. By a replay of the
conditions on the squarefree part in Python, in seeds 12 to 18: 448 inputs took the translate branch, 167 the
scale-only branch, the rest the plain branch (the replay is approximate; the C code was not instrumented). A
negative scale, a translate that is a root (odd multiplicity factor Y itself), non-squarefree input, content: all
present. `item2.py`.

## Item 3: the early stop

Exact dyadic roots on cell end points (roots at 2^-j, m/2^j, +-1/2, +-1/4 ...), clusters b/a with consecutive b,
multiple roots (mult 2 and 3), root 0 with `X^k`, `p(-X)`, with complex factors, mixed: 3 500 inputs of seeds 1 to
4 (item34.py mode 3: 500 + 3 x 1000), prec 2, 3, 64. 0 wrong lists, 0 non-OK statuses. Plus 5 000 dense and
sparse random inputs of degree 1 to 60 (`item34.py 9`, seeds 2 to 14; 500 each), prec 2, 3, 10, 64, 200: 0 wrong.
The cell that counts as one root cannot hold none or two if Descartes' rule is right and the count is true;
I found no input for which it does (gcov of `src/roots_real.c` over these runs: every line executed except the
`ADF_LIMIT` returns, the `flint_abort` guards, and `adf_roots_real_isolate`, the compatibility entry).

## Item 4: the filters

Degree 8 to 60 (Wilkinson 10 to 40 plus perturbation, `(2^k X - 1)^d - c` for c in 1, 2, 3, 2^10 and k in 1, 5, 20, 100,
`prod (2^k X - (m 2^(k-3) +- 1))`, Wilkinson scaled by 16 with +-1, `X^d - 2^e` times (X^2 - 1),
Mignotte `X^d - 2 (2^k X - 1)^2`), prec in 2, 64, 1000: 200 + 200 + 400 x 4 = 2 000 inputs, seeds 1 to 6, 0 wrong.
Each list: n equal to Sturm's count, exactly one root per ball, order, `rad <= |mid| 2^(1 - prec)`, both verifiers `1`.
`item34.py 4`. Wilkinson 50 at prec 1000: 0.04 s.

## Item 5: nesting

850 pairs (prec1 in 2, 3, 5, 20, 64; prec2 = prec1 + 1, 2, 10, 60, 300) over the families of item 3 and 4: 250 + 500 + 100: the ball at prec2 is inside the ball at prec1 in exact rational arithmetic
in all of them. 0 failures. `item34.py 5`.

## Item 6: memory and time

valgrind (`~/.local/bin/valgrind --leak-check=full`), 20 inputs (`mkvg.py`: X^2 - 2 at prec 2 and 64, constants,
X, Wilkinson 11, clusters of 8 at 2^40 and 2^400 scale, degree 8 and 9 at prec 300 and 5000, Mignotte of degree
12, roots 3 2^-m, a cubic with repeated roots, `X^2 - 4^k - 1` pairs, no real roots, and a root at X^3 factor with a
5000-bit coefficient): 0 errors from 0 contexts, definitely lost 0, indirectly lost 0, possibly lost 0 after
`flint_cleanup_master()` (before it 4 065 "possibly lost" blocks, all from FLINT's mpz cache, in
`_fmpz_clear_mpz` under FLINT routines). All 20 lists checked with the oracle: 0 defects. `alias.c` under valgrind:
`f = L->g`, DOMAIN, LIMIT (prec 3 000 000; and the root 2^(1-M)) leave L untouched, constant f, prec 0 and -5:
0 fails, 0 errors. Time, see MINOR 1 and 2. The slowest inputs at degree at most 50 and at most 10^4 bits are
above. Mignotte, degree 50, e = 4000, prec 5000: 0.42 s in total.

## What I tried without result (counts and what would have failed it)

- Wrong list with OK: none in 15 099 certificate mutants and about 15 000 end-to-end runs (2 000 item 4, 3 900
  item 2, 3 500 + 5 000 item 3, 850 nesting pairs, 360 at high prec (item8.py) (roots near 2^j (1 + eps 2^-3000), prec
  4200 to 12000: the translate branch of `real_entry_ok` was entered 64 times and used 32 times, by gcov, no failure)). A failure
  would be a status 0 with n different from Sturm's count, a ball with 0 or 2 roots, disorder, or verifier 0.
- Non-OK status on a valid input: none (histograms all `{0: N}`). `ADF_NOT_DETERMINED` never appeared in an
  end-to-end run.
- Crash or fault: none (valgrind, plus the runs above without a nonzero exit).
- The filters cannot be made uncertain and wrong at the same time: `arb_set_round_fmpz_2exp` and Horner give an
  enclosure, `arb_get_unique_fmpz` returns only a unique integer; I read this; the near-dyadic families
  (k up to 190) ran without a wrong ball. I did not count how often a filter was uncertain.
  A wrong filter could not have produced a wrong OK list in any case (item 1).

## Not done

- Not instrumented: which of R9's three branches the C code took (replayed in Python only).
- Not run: `make check`, the repository tests, mutation testing of the source, libFuzzer.
- Not reached: ADF_LIMIT from a gallop or a refinement below KMIN in a polynomial of degree above 2; the
  only LIMIT cases are the degree 1 and 2 roots near 2^-M. `roots_real.c` lines 424, 597, 805 stay unexecuted by me.
- No source for `arb_rel_accuracy_bits` (arb sources are not on disk under refs/src/flint-src-3.0.1); see step 7.

## Files (all under lanes/r-review2/)

oracle.py, drv.c, fdrv.c, item2.py, item34.py, item6.py, item8.py, mutate.py, limit.py, alias.c, prof.c, prof2.c,
prof3.c, mkcl.py, mk6.py, mkasym.py, mkhp.py, mkvg.py, chkvg.py, slow.py. Reproduce, for example:
`make -j2 BUILD=lanes/r-review2/build; cc -O2 -Iinclude lanes/r-review2/drv.c -o /tmp/x/drv
lanes/r-review2/build/libadelefeld.a -lflint -lgmp -lm; python3 lanes/r-review2/item2.py 12 300`
(`ADF_DRV` overrides the driver path of `oracle.py`; the programs expect /tmp/x).
