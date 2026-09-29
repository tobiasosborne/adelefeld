# Real roots by exact isolation (issue adf-8di, lane r-slice1)

Status: implemented in `src/roots_real.c` (first slice), called by `adf_roots_real` (`src/roots.c`), tested by
`tests/test_roots_real_isolate.c` and `tests/test_roots_real.c`; reference `proto/real_isolation.py`. Not
reviewed. This file holds only what the slice uses.

## 1. The problem and what changes

`adf_roots_real` (Algorithm RR, `docs/proofs/solvers.md` Propositions 3.8 to 3.10) took its candidates from
`arb_fmpz_poly_complex_roots`, whose working precision grows like `2^(8 + e/100)` bits for two roots of relative
distance `2^-e` (`lanes/d-realroots/progress.md`): the quadratic with the roots `2^1500` and `2^1500 + 1` took
111.5 s at `prec = 2` (`lanes/r-slice1/runs/bench_before.txt`). The candidates now come from an isolation of the
real roots in exact integer arithmetic (Algorithm D below) and a refinement of each isolating interval (Algorithm
F). The certificate of Algorithm RR does not change: `count` is FLINT's count (S-D11), every stored ball passes
the exact test of P3.8 at its exact end points, the balls are strictly ordered, their number is `count`, and
each has the accuracy of S-D19 (`real_finish`, `src/roots.c`). Nothing that Algorithms D and F compute is
trusted by that certificate; the propositions below say why it passes.

Decisions of the brief (orchestrator, 2026-09-29): (1) Descartes bisection, not a Sturm chain; (2) refinement
by plain bisection, quadratic interval refinement later "if the benchmark asks for it"; (3) the certificate does
not change; (4) a dyadic root met at a bisection point is an exact ball; (5) no change of the interface or of
the statuses. Decision (2) was not kept, by the benchmark it names: plain bisection is quadratic in `prec`
(`X^2 - 2`: 0.0174 s at `prec = 4096`, 0.434 s at 16384, 6.50 s at 65536; about 6600 s extrapolated to
`ADF_ROOTS_REAL_PREC_MAX = 2^21`, which `tests/test_roots_real.c` asks), so step Q (Eqir of [KS]) is part of
this slice. Two steps were added that the brief did not name, both measured: step G (galloping from an
anchor, section 5) and the static floor (section 5, the separation of touching cells).

## 2. Sources

- [SM] `refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex`. `:547` to `:553`: Descartes' rule of signs for an
  interval `I = (a, b)`: `P_I(x) = (x + 1)^n P((a x + b) / (x + 1))`, `v_I` the number of sign variations of its
  coefficients (zeros are not considered, footnote at `:547`); `v_I >= m_I` and `v_I = m_I` modulo 2, `m_I` the
  number of roots of `P` in `I`; so `v_I = m_I` when `v_I <= 1`. `:364`: the separation `sigma_P`, the least
  distance of two distinct complex roots. `:569`: "each interval `I` of width `w(I) < sigma_P / 2` yields
  `v_I = 0` or `v_I = 1`". `:571` to `:575` (Theorem subad): `var(P, I_1) + var(P, I_2) <= var(P, I)` for two
  disjoint subintervals `I_1`, `I_2` of `I`. `:302`: the known cost of the Descartes method, `O~(n^4 tau^2)`,
  tree size `O(n (tau + log n))` (a citation of Eigenwillig, Sharma and Yap there; that paper is not on disk).
- [KS] `refs/src/kerber-sagraloff/tex/arxiv.tex`. `:278` to `:298`: Algorithm Eqir (one step of the quadratic
  interval refinement): `m' = a + round(N f(a) / (f(a) - f(b))) w`, `w = (b - a) / N` (`:287` to `:288`); the
  signs at `m'` and at `m' + w` or `m' - w` (`:290` to `:294`); on success the subinterval and `N^2`, else `I`
  and `sqrt(N)` (`:295`). `:337` to `:342`: `N = 4` at the start, squared after a success, `sqrt(N)` after a
  failure, a bisection and `N = 4` when `N` drops to 2; with a bound on the width every step succeeds, "and,
  thus, quadratic convergence" (`:342`, a statement of [KS] about Kerber's analysis, not proved here).
- FLINT 3.0.1, each routine read in its documentation and source: see the comment at the top of
  `src/roots_real.c` (`_fmpz_poly_taylor_shift`, `_fmpz_poly_scale_2exp`, `_fmpz_poly_remove_content_2exp`,
  `_fmpz_poly_reverse`, `_fmpz_poly_div_root`, `_fmpz_poly_set_length`, `fmpz_poly_evaluate_fmpz`,
  `arf_set_fmpz_2exp`, `mag_set_ui_2exp_si`).
- (IVT), as in `solvers.md` line 64.

## 3. Notation

`g` is the normalised polynomial of the input (squarefree, primitive, positive leading coefficient), of degree
`d >= 1`. A cell `(c, k)`, `c` and `k` integers, is the open interval `(c 2^k, (c + 1) 2^k)`; its ball has the
midpoint `(2c + 1) 2^(k-1)` and the radius `2^(k-1)`, so its exact end points are the ends of the cell and
`arb_rel_accuracy_bits` of the ball is `acc(c) = bits(|2c + 1|) - 2` (flint-3.0.1 `arb.rst:499` to `509`: the
top bit of the radius is at `k`, that of the midpoint at `bits(|2c + 1|) + k - 1`). A point `(c, k)` is the number
`c 2^k`. `M = ADF_ROOTS_BITS_MAX`, `KMIN = 2 - M`: no cell with `k < KMIN` is formed (its radius would be at most
`2^-M`, not of admissible size, `roots.h` "Real balls"); where one would be needed, the result is `ADF_LIMIT`.

## 4. Algorithm D (isolation) and its propositions

**Lemma R1 (a root bound).** Let `h` have degree `n >= 1` and `h(0) != 0`. Let `B(K)` be the statement
`|h_n| 2^(K n) > sum_(i<n) |h_i| 2^(K i)`. (1) If `B(K)`, every complex root `z` of `h` has `|z| < 2^K`, and
`B(K')` for every `K' > K`. (2) `B(-bits(h_n))` fails and `B(bits(A) + 1)` holds, `A = max_(i<n) |h_i|`.
So the least `K` with `B(K)` is found by bisection between these two.

*Proof.* (1) Dividing by `2^(K n)`, `B(K)` says `|h_n| > sum_(i<n) |h_i| t^(i-n)` at `t = 2^K`; the right side
decreases strictly in `t > 0` (some `h_i`, `i < n`, is not 0: `h_0`), so `B` holds for every larger `K`. For
`|z| >= 2^K`: `|h(z)| >= |z|^n (|h_n| - sum_(i<n) |h_i| |z|^(i-n))`
`>= |z|^n (|h_n| - sum_(i<n) |h_i| 2^(K(i-n))) > 0`.
(2) At `K = -bits(h_n) <= -1`: `|h_n| 2^(K n) <= |h_n| 2^K < 1 <= |h_0|`. At `K = bits(A) + 1 >= 2`: `A < 2^(K-1)`
and `2^K - 1 >= 2^(K-1)`, so `sum_(i<n) |h_i| 2^(K i) <= A (2^(K n) - 1) / (2^K - 1) < 2^(K n) <= |h_n| 2^(K n)`.
Check: `proto/test_real_isolation.py` `test_root_bound`; C: every test that isolates.

**Algorithm D** (positive roots of `h`, `h` squarefree, `h(0) != 0`; `src/roots_real.c` `isolate_positive`).
`K = max(least K of R1, KMIN)`. The root node is `(q_0, 0, K)` with `q_0` a positive multiple of `h(2^K X)`
(`_fmpz_poly_scale_2exp`). For a node `(q, c, k)`: `v = var((x + 1)^n q(1 / (x + 1)))` (reverse, Taylor shift by
1). `v = 0`: dropped. `v = 1`: the cell `(c, k)` is output. `v >= 2`: if `k - 1 < KMIN`, `ADF_LIMIT`; else
`left` = a positive multiple of `2^n q(X / 2)`, `right = left(X + 1)`; if `right(0) = 0` the point `(2c + 1, k - 1)`
is output and `right := right / X`, `left := left / (X - 1)`; the children `(left, 2c, k - 1)` and
`(right, 2c + 1, k - 1)`, those of degree `>= 1`, are treated alike (only nodes with `v >= 2` are kept on the
stack). The negative roots: the positive roots of `h(-X)`, a cell `(c, k)` mirrored to `(-c - 1, k)`, a point `c`
to `-c`. The root 0: if `g(0) = 0`, the point 0, and `h = g / X` (`g` squarefree: `X` divides it once).

**Proposition R2 (isolation is correct).** Let `h` be squarefree with `h(0) != 0`, and let Algorithm D return
without `ADF_LIMIT`. Then every output point is a positive root of `h`, every output cell contains exactly one
root of `h` and it is simple, and every positive root of `h` is an output point or lies in exactly one output
cell; the cells and points are pairwise disjoint.

*Proof.* Invariant (I) of a node `(q, c, k)`: `q` is not zero, `q(0) != 0`, `q(1) != 0`, and `x -> (x - c 2^k)/2^k`
maps the roots of `h` in the cell `(c, k)` one to one onto the roots of `q` in `(0, 1)`, all simple.
The root node: `q_0(X)` is a positive multiple of `h(2^K X)`, `q_0(0) = h(0) != 0`, `q_0(1) = h(2^K) != 0` and every
positive root is below `2^K` (R1(1), and `K` only enlarged). A split: `left(X) = 2^n q(X/2)` has in `(0, 1)` the
images of the roots of `q` in `(0, 1/2)`, that is of `h` in the cell `(2c, k-1)`; `right(X) = left(X + 1)` those in
`(1/2, 1)`, the cell `(2c + 1, k - 1)`; `right(0) = 2^n q(1/2)`, so `right(0) = 0` exactly when the midpoint
`(2c + 1) 2^(k-1)` is a root of `h`; it is simple, so after the division by `X - 1` and by `X` neither child has
it, and the children keep the other roots. `left(0) = 2^n q(0) != 0` and `right(1) = 2^n q(1) != 0`;
`left(1) = right(0)`, which is not zero after the division (a simple root). Scaling by a positive number keeps
the roots and the signs. So (I) holds for every node. Descartes' rule ([SM]:547 to 553) for `I = (0, 1)`, that
is `a = 0`, `b = 1`, `P_I(x) = (x + 1)^n q(1/(x + 1))`: `v = 0` means no root in the cell, `v = 1` exactly one.
The open cells of one level and the points between them partition the cell of their parent level, so every
positive root is in exactly one of: a cell on the stack, an output cell, an output point, a dropped cell (no
root). When the stack is empty, the first class is empty. The points are distinct (different midpoints) and
lie outside every open cell of their level and below.

**Proposition R3 (isolation ends, and the size of its tree).** Let `sigma` be the least distance of two distinct
complex roots of `h` (`sigma = infinity` for `n = 1`). (1) A node `(q, c, k)` with `v >= 2` has `2^(k+1) >= sigma`.
(2) At each level at most `floor(n/2)` nodes have `v >= 2`. (3) So Algorithm D forms at most
`1 + 2 floor(n/2) (K - ceil(log2 sigma) + 2)` nodes, or returns `ADF_LIMIT` when it would split a node of
`k = KMIN`; it ends.

*Proof.* (1) The roots of `q` are the images of distinct roots of `h` under `x -> (x - c 2^k)/2^k` (by (I) and the
divisions, which only remove roots), so `sigma_q >= sigma / 2^k`. If `2^(k+1) < sigma` then the width 1 of
`(0, 1)` is below `sigma_q / 2`, and [SM]:569 gives `v <= 1`. (2) For the children of one node, with `P = q`,
`I = (0, 1)`, `I_1 = (0, 1/2)`, `I_2 = (1/2, 1)`: `var(q, I_1)` is the variation count of `left0 = 2^n q(X/2)` on
`(0, 1)` (the transform of `q` for `I_1` is a positive multiple of that of `left0` for `(0, 1)`), and dividing out
a root at the end point does not change the count: `(x + 1)^n left0(1/(x+1)) = -x (x + 1)^(n-1) left(1/(x+1))`
for `left0 = (X - 1) left`, and `(x + 1)^n right0(1/(x+1)) = (x + 1)^(n-1) right(1/(x+1))` for `right0 = X right`:
the coefficient sequences are the same up to a shift and a sign. So [SM]:571 to 575 gives
`v(left) + v(right) <= v(q)`, and by induction the sum of `v` over the nodes of a level is at most `v` of the
root, which is at most `n` (`n + 1` coefficients). Nodes with `v >= 2`: at most `floor(n/2)` per level.
(3) Levels are `k = K, K - 1, ...`; by (1) no node below `k = ceil(log2 sigma) - 1` is split, so there are at most
`K - ceil(log2 sigma) + 2` levels with splits, each creating at most `2 floor(n/2)` children. The limit `KMIN`
stops the descent in any case.

Check: `tests/test_roots_real_isolate.c` `isolate_on_real_cases` (68 runs, every list true by a Sturm chain of
the test), `proto/test_real_isolation.py` `test_var01_is_a_bound_with_parity`.

## 5. Algorithm F (refinement) and its propositions

Input: a cell `(c, k)` from Algorithm D with exactly one root `r` of `g`, `need = max(prec, 2)`, and a floor `f`:
the right end of the closed set of the item before it in increasing order (the right end of its cell as
isolated, or the point), none for the first item. A step, repeated (`refine_cell`):

- **G (anchor).** The cell has an anchor `p` if it touches 0 (`c = 0`: `p` its left end 0; `c = -1`: `p` its right
  end 0) or if an end is a root of `g` (not clean). With `sigma = +1` for a left anchor and `-1` for a right one,
  and `v0` the sign of `g` between `p` and `r` (`sign g(p)`, or `sigma sign g'(p)` when `g(p) = 0`), the predicate
  `P(t): sign g(p + sigma 2^t) = v0` holds exactly for `2^t < |r - p|`. Galloping `t = k - 1, k - 2, k - 4, ...`
  then bisection of the exponent gives `T = floor(log2 |r - p|)` (or meets `r`); the new cell is
  `(p + 2^T, p + 2^(T+1))` or `(p - 2^(T+1), p - 2^T)` of exponent `T`. `ADF_LIMIT` if `P(KMIN)` fails.
- **Stop.** Both ends clean, `c 2^k > f`, and `acc(c) >= need`: the cell is the result.
- **Q (Eqir, [KS]).** Both ends clean and `N = 2^j > 2` (`j = 2` at the start and after a bisection): one Eqir
  step on the grid of `N + 1` points of exponent `k - j` (`j` capped so that `k - j >= KMIN`): success gives the
  grid cell with a sign change and `j := 2 j`; a failure `j := j / 2`; a zero at a grid point is `r`.
- **B (bisection).** The sign at the midpoint (`ADF_LIMIT` if `k - 1 < KMIN`); zero: `r` is the midpoint;
  else the half with the sign change; `j := 2`.

**Proposition R4 (refinement).** Let the cell `(c, k)` contain exactly one root `r` of `g` and no root of `g`
other than possibly its ends, and let `f < r` if there is a floor. Then:

1. (One root.) Every cell visited contains `r` and no other root in its interior; a returned point is `r`; a
   returned cell has ends that are not roots, `g(lo) g(hi) < 0`, `lo > f`, and `acc(c) >= need`.
2. (It ends.) Algorithm F returns after finitely many steps: a cell, the point `r`, or `ADF_LIMIT`. It returns
   `ADF_LIMIT` only if `r` is closer than `2^KMIN` to 0 or to a root at an end, or if the stop needs a cell of
   exponent below `KMIN`. The stop holds for every clean cell (not touching 0) of exponent
   `k <= min(floor(log2 |r|) - need - 1, floor(log2 (r - f)) - 1)`.
3. (Dyadic roots.) If `r = m 2^t` with `m` odd and `bits(m) <= need + 1`, the result is the exact point `r`.
4. (Nested.) The sequence of cells visited depends only on `g` and the start cell; `need` and `f` decide only
   where it stops. So for `need' > need`, and for any floors, the result for `need'` lies in the result for
   `need` (the stop for `need` comes no later in the sequence, see the proof).

*Proof.* (1) By induction over the steps. G: `g` has a single simple root `r` in the open cell and none between
`p` and the far end except `r`; so on `(p, r)` it has one sign, `v0` (at `p` itself either `g(p) != 0`, or `p` is
a simple root and `g` has the sign of `g'(p)` on the side `sigma`), and `-v0` beyond `r`; `P(t)` for
`p + sigma 2^t` in the cell is therefore `2^t < |r - p|`, a zero is `r`, and the new cell contains `r`, with the
end next to `p` of sign `v0` and the far one of sign `-v0` (or the old far end when `T = k - 1`). `p` is a
multiple of `2^k`, so the new ends are multiples of `2^T` and the new cell is `(c', T)`. Q: [KS]:290 to 294 on
signs that are not zero; a success keeps a subcell with a sign change, which contains a root, and the only root
in the old cell is `r`; the new ends were evaluated and are not zero. B: likewise. The stop tests the rest.
(2) Each G makes `k` smaller (`T <= k - 1`). Each Q success and each B makes `k` smaller. A Q failure halves
`j`; with `phi = log2 j`, a success adds 1, a failure subtracts 1, a bisection sets it to 1, so the failures are
at most the successes plus the bisections plus 1. Hence every bounded run of steps makes `k` smaller, and
`k` reaches `KMIN` after finitely many steps unless the loop stops or meets `r` first; at `KMIN` G, Q and B
return `ADF_LIMIT` rather than form a smaller cell. The bound: after G the cell has no anchor, and Q and B keep
both ends clean. For `c >= 1`, `r < (c + 1) 2^k <= (2c + 1) 2^k`, so `2c + 1 > |r| 2^-k` and
`bits(2c + 1) >= floor(log2 |r|) - k + 1`, `acc(c) >= floor(log2 |r|) - k - 1`; for `c <= -2`,
`|2c + 1| = 2|c| - 1 >= |c| > |r| 2^-k`, the same. `lo > r - 2^k >= f` when `2^k < r - f`. `P(KMIN)` fails only
when `|r - p| < 2^KMIN`; then no cell of exponent at least `KMIN` separates `r` from `p`.
(3) A cell of exponent `k' <= t` has ends that are multiples of `2^k'`, and `r` is one, so no such cell has `r`
in its interior: before the exponent drops to `t` the sequence meets `r` (as a midpoint, a grid point, or a
galloping point; every step keeps `r` in the interior of a cell or returns it). A cell `(c', k')` with
`k' >= t + 1` that contains `r` has, for `r > 0`, `c' < m 2^(t-k') < c' + 1`, so `2c' + 1 < m + 1`,
`bits(2c' + 1) <= bits(m)`, `acc <= bits(m) - 2 < need` (for `r < 0` the same with `|.|`): it does not stop.
(4) The steps G, Q, B read only `g`, the cell, `j` and the signs; `need` and `f` enter only the stop. If the stop
for `need` is at step `s`, the cell there satisfies clean, `lo > f_1` and `acc >= need`; for `need' > need` the
stop is at some step `s' >= s`, or at an earlier step whose cell satisfies `acc >= need' > need` and `lo > f_2`.
The floor `f` of an item is the right end of the previous item's cell as isolated, which does not depend on
`need`, so `f_1 = f_2`, and an earlier stop for `need'` would also be a stop for `need`: `s' >= s`, and the cells
of the sequence are nested. (Were `f` the right end of the previous item's refined ball, it would move left as
`need` grows, and the lists would not always be nested: for `(3X - 11)(3X - 13)`, whose cells `(3, 0)` and
`(4, 0)` touch at 4 and have accuracies 1 and 2, the ball of `13/3` would be `[17/4, 9/2]` at `prec = 2` and
`[4, 9/2]` at `prec = 3` (found with a moving floor in a scratch copy of `proto/real_isolation.py`); that is
why the floor is static. The fault "moving floor" of `lanes/r-slice1/bite.py` is caught by the case `191/3`,
`193/3` of `touching_cells_and_nesting`.)

Check: `tests/test_roots_real_isolate.c` `dyadic_roots_and_zero_are_exact_balls` (R4(3)),
`touching_cells_and_nesting` (R4(4), 98 pairs of lists at 15 precisions), the slow families (R4(2), G), the
high precisions (Q); `tests/test_roots_real.c` `precision_accuracy_below_2_and_nesting` (108 nested pairs).

**Proposition R5 (the whole).** For `g` of degree `>= 1`, if Algorithms D and F return without `ADF_LIMIT`, the
balls (exact points and cells) in increasing order satisfy every test of Algorithm RR, steps 5 and 6 of
`solvers.md` (P3.8 at the exact end points, `hi_i < lo_(i+1)`, their number is the number of real roots of `g`,
accuracy at least `max(prec, 2)` or exact); no ball is widened. So `adf_roots_real` returns `ADF_OK` unless a ball
is not of admissible size (`ADF_LIMIT`) or FLINT's count differs from the number of real roots
(`ADF_NOT_DETERMINED`, which would be a defect of FLINT or of this file).

*Proof.* R2 and R4(1) for each item; the items are pairwise disjoint (R2) and sorted by their left end. Order:
if item `i + 1` is a cell, its ball's left end is above the floor, the right end of the closed set of item `i`,
which is at least the right end of the ball of item `i`. If it is a point `p` and item `i` a cell: the cell's
right end is at most `p`; if it were `p`, it would be a root, which a returned cell does not have. Two points
are distinct. Number: R2, every real root in exactly one item.

## 6. Cost, and the lower bound

What is proved (R3, R4(2)): the tree has `O(n (K - log2 sigma))` nodes; each costs two Taylor shifts of degree
at most `n` (`O(n^2)` additions of integers of at most about `tau + n (K + depth)` bits). The refinement of a root
takes `O(log(k_0 - T))` evaluations in G and at most `8 (k_0 - k_end) + 4` in Q and B (an attempt of Q costs at
most 4 evaluations, B one; the attempts are at most twice the successes plus the bisections plus 1, and each
success or bisection lowers `k`); each evaluation is an exact
Horner evaluation at a point of `bits(c) + |k|` bits. What is not proved here: that Q converges quadratically
([KS]:342 states it for Kerber's analysis, with a width bound; not on disk). Where the time goes (measured,
`lanes/r-slice1/runs/bench_after.txt`, a shared machine): a cluster of two roots far from 0 and from each other's
scale (`thirds`: the roots `2^e + 1/3`, `2^e + 2/3`) costs a descent of about `e` levels in the tree (0.22 s at
`e = 30000`), since Algorithm D has no step G of its own; the two roots `2^e` and `2^e + 1` cost 0.014 s at
`e = 100000` (the midpoint meets `2^e`, and G finds the other root).

Lower bound, in the sense of `docs/PERF.md` (the row in its section 4). PROVED: the input must be read and the
output written. MODEL, restricted to results certified as decision (3) prescribes: the certificate evaluates
`g` exactly at both end points of each ball that is not exact, so no implementation that keeps it is faster than
those `2 n` exact signs at the end points it outputs. The benchmark measures them on the balls of this
implementation (`eval2`); the end points have the length that the separation or `prec` forces (within 2 bits),
except after a last Eqir step, which may give up to about twice the accuracy asked for (`X^2 - 2` at
`prec = 2^21`: mantissas of `2^22` bits), where the floor for shortest end points is lower than `eval2`.

## 7. Not done, open

- Algorithm D descends one level at a time; a cluster far from its own scale (`thirds`) costs a number of
  levels linear in the bit size. A continued-fraction or quadratic step in the isolation is a later slice.
- The quadratic convergence of Q is cited, not proved.
- Sturm's theorem (behind FLINT's count, S-D11) is still `[source pending: Sturm's theorem]`.

## 8. Statements

| Number | Content | Status | Check |
|---|---|---|---|
| R1 | a root bound `2^K`, found by bisection | proved here | `test_root_bound`; C tests |
| R2 | Algorithm D is correct | proved modulo Descartes' rule ([SM]:547 to 553) | `isolate_on_real_cases` |
| R3 | Algorithm D ends; tree size | proved modulo [SM]:569 and :571 to 575 | slow families |
| R4 | refinement: one root, ends, dyadic roots exact, nested | proved modulo (IVT) | dyadic, nesting, timed tests |
| R5 | the list passes steps 5 and 6 of Algorithm RR | proved modulo R2, R4 | all C tests; both verifiers |
