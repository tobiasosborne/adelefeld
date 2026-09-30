# Lane f-review4: referee of F8 and F9, review of the fast series at a prime

Date: 2026-09-30. Worktree at commit 9bf6938 (master). Reviewed: `docs/api-1f4.md` F8, F9 (lines 198-289);
`src/lfunc.c` (`log_sum` 373-416, `log_split` 422-441, `log_short` 444-458, `log_balanced` 463-496, the
route selection 599-604); `tests/test_lfunc.c` `stored_before_optimisation` and the fixture
`tests/ref/vectors/f-slice5/stored.jsonl`. The old route is `src/lfunc.c` at commit 1cf0e42 (`git show`,
built into a scratch directory under renamed symbols).

## Findings

No BLOCKER. F8 and F9 are true as written, and the code implements them with the digit counts of the
proofs (section 1 and 2). No input was found on which the new library differs from the old one, from the
two new routes forced at every precision, from my own exact oracle, or from FLINT (section 3, 21188 + 136
inputs). The findings are about the test that guards the claim and about the cost.

### MAJOR 1: the stored comparison guards F9 with 9 of its 2000 rows, none at the word prime

The claim of the lane is that `stored_before_optimisation` (2000 old-code results) detects a wrong F9. The
rows of the fixture that reach `log_balanced` (a ball result with `K > 64` and a working modulus above the
tagged word) are 9: `p = 2` (1 row, `K = 2049`), `3` (2 rows), `5` (2), `7` (1), `11` (3); `K` between
1794 and 2656; 0 rows at `p = 2^64 - 59` (its rows have `K <= 38`). The F8 route (`log_sum`) is reached by
384 rows, all with `K <= 38`: no row with `K` in `39 .. 64`. Counted from the fixture with the route rule
of `Log_centre` (`python3 -c` over `stored.jsonl` with `p^W < 2^62`, `W = K + e(2K)`; a row at the
boundary may be classed one route off by this estimate, which cannot move the counts by more than a few
rows and cannot create a word-prime row with `K > 64`, since the largest `K` there is 38).

Consequence, shown with planted faults (section 5): a fault of F9 that only acts at a prime above `2^32`
(the residual inverse taken modulo `p^(K-m-1)` instead of `p^(K-m)`, i.e. one digit short, exactly the
statement of F9 step 2 that the brief asks about) passes the stored comparison with 0 failed checks; it is
caught only by `precision_2000` (2 failed checks), a test that predates the lane. A fault of F8 that only
acts for `K > 40` passes the stored comparison with 0 failed checks; it is caught by `flint_second_opinion`
(40) and `cases_from_the_reference` (1). The lane's report ("each is rejected by the stored comparison")
holds for its three coarse faults, which act at every `K`; the fixture does not cover the range where F8
and F9 actually run at the prime the lane optimised for. The fixture's `N` distribution ("bounded by 3000;
most requests use `N = -2 .. 36`") was chosen for file size, and that choice removed the coverage.

What would close it: 20 to 50 stored rows per prime with `K` in `39 .. 64` (F8) and `K` in `65 .. 3000`
(F9), including the word prime, and a few with several doubling steps of F9 (`v(z) = 1`, `K >= 200`).
My probe suite (`lanes/f-review4/oracle.py`, modes `small`, `exhaustive`, `random`) does this and catches
every planted fault (section 5); it is 16 s to 6 min of Python and is not a unit test.

### MINOR 2: a small-`N` regression above 10 percent at the word prime

Input: `adf_lball_log` of the exact `1 + p`, `p = 2^64 - 59`, `N = 2` (result `p Z_p + p^2 Z_p`, `K = 2`,
one term). Old route (the code of 1cf0e42, which for this modulus of 128 bits is the same loop the new
code keeps only for word-size moduli): median 0.834, 0.822, 0.823 us; new (`log_sum`, F8): 0.939, 0.929,
0.928 us; ratios 1.126, 1.130, 1.128 in three runs of 20000 alternating calls each
(`lanes/f-review4/rate-recheck.log`). The plausible cause (not profiled) is the setup of `log_sum`
(nine `fmpz_init`/`clear` pairs, four `fmpz_ui_pow_ui`, three exact divisions) for a sum of one term.
At `N = 3` the ratio is 1.058, at `N = 4` 1.017, and from `N = 8` on the new route is faster (the lane's
measurement starts at `N = 8`). Over 1920 input/precision pairs (8 primes, `N = 1 .. 80`, three inputs, 1500 alternating calls each,
`lanes/f-review4/rate.log`) this is the only pair above 1.10 that reproduces; the other (`p = 2`, `N = 12`,
1.107, where both routes run the same code) is 1.017 on rechecking. 1399 of the 1920 pairs are below 0.90.
Command: `sh lanes/f-review4/bench.sh <scratch> <input>`; the two inputs are in `rate-recheck.log`.

### MINOR 3: the cost above the word range is not near-linear in `N`

Doubling `N` multiplies the median time by 2.3 to 2.9 (`lanes/f-review4/scale.log`, new route only,
median of 3):

| input | 1000 | 2000 | 4000 | 8000 | 16000 | 32000 | s |
|---|---:|---:|---:|---:|---:|---:|---|
| `log(1 + p)`, `p = 2^64-59` | 0.00115 | 0.00322 | 0.00892 | 0.0229 | 0.0581 | 0.143 | ratios 2.8, 2.8, 2.6, 2.5, 2.5 |
| `Log(13/17)`, same `p` | 0.0316 | 0.0868 | 0.235 | 0.593 | 1.470 | 3.410 | ratios 2.7, 2.7, 2.5, 2.5, 2.3 |
| `log` of a random unit of `N` digits, same `p` | 0.0156 | 0.0449 | 0.127 | 0.344 | 0.898 | - | ratios 2.9, 2.8, 2.7, 2.6 |
| `log(1 + p)`, `p = 3` | 0.000093 | 0.000243 | 0.000649 | 0.00184 | 0.00530 | 0.0147 | ratios 2.6, 2.7, 2.8, 2.9, 2.8 |

That is `N^1.3` to `N^1.5` in this range: the cost of `O(log^2 N)` multiplications of `N log2(p)`-bit
integers with GMP's Toom multiplication, as the tree of F9 predicts; it is quasi-linear in the number of
big-integer operations, not linear in the time. The lane's own table shows the same (`p = 2`, `log`:
0.000096, 0.000808, 0.0305 s at 2000, 10000, 100000: `N^1.32` then `N^1.58`). `docs/PERF.md` does not
claim linearity; the header does not either. This is a finding against the brief's word, not the code.
At `N = 100000`, `p = 2^64 - 59`: `log(1 + p)` 0.62 s, `Log(13/17)` 14.1 s, `log` of a random unit of
100000 digits 9.7 s (`lanes/f-review4/mem.log`). The two-second target of `docs/PERF.md` is stated for
the three measured families at `K = 10000` only, so this is not a violation of it; it says that a general
unit at `10^5` digits costs 5 to 7 seconds more than the measured families.

Memory of the splitting tree at `N = 100000` (peak resident set of one process that makes one call, minus
the 4.7 MiB of an empty process): `p = 2`: 0.9 MiB; `p = 3`: 1.3 to 2.2 MiB; `p = 2^64 - 59`,
`log(1 + p)`: 20 MiB; `Log(13/17)`: 37 MiB; `log` of a random unit: 43 MiB, for an input of 0.8 MiB and
an output of 0.8 MiB. `lanes/f-review4/mem.log`. Not a finding: the tree holds `O(log T)` integers of the
size of the result.

### MINOR 4: two imprecisions of the text of F8 and F9 (no consequence for the code)

(a) F8, statement: "The input z need only be known modulo p^K." True (step 3), but the code does not use
it: `zr` is the residue modulo `p^W`, `W >= K` (`Log_centre`, line 563), so `b = zr / p^vz` carries
`W - vz` digits. Harmless. (b) F9, statement: "Repeat with v replaced by m, until m = K": the factor at
`m = K` is `l = u` itself and the quotient `u / l = 1`; the text says "At the last step the remaining
quotient is 1 modulo p^K" in step 2, which is right; the statement could say that the step `m = K` has no
residual. Harmless. (c) F9 relies on Proposition 7b's count `T >= 1` for every factor (otherwise
`log_split(A, B, R, q, 1, 1)` recurses without end, `b - a = 0` is neither the leaf nor a split with
`a < d < b`): this holds because `0 < z = l - 1 < p^m <= p^K` gives `w = v_p(z) < K`, and then `J >= 2`
(`k = 1` gives `w < K`), `T = J - 1 >= 1`; neither F9 nor the comment of `log_short` says it. The planted
fault `f9_valuation` (section 5) crashes with SIGSEGV for this reason, which is how the invariant surfaced.

## 1. Referee's reading of F8 and F9 (`docs/api-1f4.md` 198-289)

Notation as there: `p` prime, `v = v_p`, `c = 1` for odd `p` and `2` at `p = 2`; `e(k) = floor(log_p k)`
(Proposition 7b); `T` the count of Proposition 7b for `n = K` and lower bound `v`: `T = J - 1`, `J` the least
`k >= 1` with `k v - e(k) >= K`.

### F8

Hypotheses: `z` in `p Z_p`, `v(z) >= v >= 1`, `K > v`, `z = p^v b`; so `b` is in `Z_p` (a unit if `v(z) = v`,
divisible by `p` otherwise). `e_k = v_p(k)`, `d_k = k / p^(e_k)` (a unit), `h_k = k v - e_k`, `H_k = K - h_k`.

Step 1. `e_k <= (k - 1)/(p - 1) < k <= k v` (Lemma 5, line 122), so `h_k >= 1 > 0`. Right. If `h_k >= K`
then `z^k / k = p^(h_k) b^k / d_k` is in `p^K Z_p`, since `b^k / d_k` is in `Z_p`. Right.

Step 2. Let `y = b^k / d_k` and `y'` an integer with `y' = y` modulo `p^(H_k)`. Then
`p^(h_k) y' - z^k / k = p^(h_k) (y' - y)` is in `p^(h_k + H_k) Z_p = p^K Z_p`. Right. `y'` is what the code
forms: `b^k` modulo `p^(H_k)` (the power of a residue equals the residue of the power, ring operations),
then division by the unit `d_k` in `Z / p^(H_k)`, which is multiplication by the inverse, which does not
change the class modulo `p^(H_k)` of the quotient. Right.

Step 3 (the input needs `K` digits). `z' = z + p^K t`, `t` in `Z_p`, gives `b' = b + delta`,
`delta = p^(K - v) t`. `b'^k - b^k = sum_{i=1}^k binom(k, i) b^(k-i) delta^i`. With
`i binom(k, i) = k binom(k-1, i-1)`, `v(binom(k, i)) >= v_p(k) - v_p(i)`, so the `i`-th term has valuation
`>= i (K - v) + e_k - v_p(i)`. As `v_p(i) <= i - 1` (`p^(v_p(i)) <= i` and `2^a >= a + 1`) and `K - v >= 1`,
`i (K - v) - v_p(i) >= i (K - v) - (i - 1) = (i - 1)(K - v - 1) + (K - v) >= K - v`. So every term is in
`p^(K - v + e_k) Z_p`, and `p^(h_k) (b'^k - b^k) / d_k` is in `p^(K - v + e_k + k v - e_k) = p^(K + (k-1) v)`,
inside `p^K Z_p`. Right. (Shorter: `z'^k - z^k` is `(z' - z) sum z'^i z^(k-1-i)`, in
`p^(K + (k-1) v) Z_p`, and dividing by `k` costs `e_k <= (k - 1) v`.)

Step 4 (the nonincreasing modulus). `G_k = K - k v + e(k)`; `G_(k+1) - G_k = -v + (e(k+1) - e(k)) <= -v + 1
<= 0` because `e` grows by at most 1 per step (`p^e <= k < p^(e+1)` gives `k + 1 <= p^(e+1)`, so
`e(k+1) <= e + 1`). `G_k >= H_k` because `e(k) >= v_p(k)` (Proposition 7b, step 1). So `b^(k-1)` modulo
`p^(G_(k-1))`, times `b`, reduced modulo `p^(G_k)`, is `b^k` modulo `p^(G_k)`, which determines `b^k` modulo
`p^(H_k)`. Right. `G_k > 0` for `k <= T`: `k <= T < J` gives `k v - e(k) < K`. Right. (When `H_k <= 0` and
`G_k > 0` the code skips the term but still updates the power and the modulus: lines 387-394.)

Step 5 (division by a word). `Q = p^(H_k)`, `0 <= r < Q`, `gcd(Q, d_k) = 1`. `j = (-r mod d_k) (Q mod d_k)^(-1)
mod d_k` gives `r + j Q = r - r = 0` modulo `d_k`; `0 <= r + j Q < Q + (d_k - 1) Q = d_k Q`, so
`0 <= (r + j Q) / d_k < Q`; and `d_k ((r + j Q) / d_k) = r + j Q = r` modulo `Q`, so the quotient is the class
`r / d_k` in `Z / Q`. Right. `d_k <= k <= T <= 2K <= 2^26` fits a word; `(Q mod d_k)` is invertible modulo
`d_k` and nonzero for `d_k > 1`. Right.

Step 6. `S = sum (-1)^(k+1) term_k` as an integer, then modulo `p^K`: each `term_k` is `z^k / k` modulo `p^K`
by steps 1-2, so `S` is the partial sum modulo `p^K`, and the tail is in `p^K Z_p` by Proposition 7b. Right;
and reducing the sum modulo `p^(G_k)` would indeed lose digits (`G_k <= G_1 = K - v < K` for `k >= 1`).

Boundaries asked by the brief. `k` a power of `p`: `e(k)` steps up by one, `G_k` drops by `v - 1 >= 0`,
`H_k = G_k` (then `v_p(k) = e(k)`), nothing special. `v = 1` at `p = 2`: F8 uses only Lemma 5 and
Proposition 7b, both stated for `v >= 1` at every `p` including 2; F8 holds there (the code never calls
`log_sum` with `vz = 1` at `p = 2`: `Log_centre` makes `z = s a - 1` with `s a = 1` modulo 4, `vz >= 2`).
`K` just above a power of `p`: `K` enters F8 only through `H_k`, `G_k` and `T`; no step depends on the
digits of `K`. The digit counts of the code are those of the proof: `Q = p^(G_k)` (line 387-388: `Q` is
divided by `p^(vz - (e(k) - e(k-1)))`, from `Q = p^K` at `k = 0`, so `Q = p^(K - k vz + e(k))`);
`Qt = Q / p^(e(k) - e_k) = p^(H_k)` (396-397); `H = K - k vz + e` (393), the skip test `H <= 0`;
`scale / p^e = p^(k vz - e) = p^(h_k)` (410). Not one less anywhere. The word division (402-409) is
step 5 with `r = term mod kp`, `qi = (Qt mod kp)^(-1) mod kp`, `j = (kp - r) qi mod kp` (`j = 0` for `r = 0`),
all in `[0, kp)`; `r + j Qt` is formed in `fmpz` (`fmpz_addmul_ui`), so there is no word overflow to speak
of; `n_mulmod2` has no restriction on its arguments (`ulong_extras.rst:375-380`).

Verdict on F8: true as written, every step checked.

### F9

Hypotheses: `u` an integer, `u = 1` modulo `p^v`, `v >= c`, `u` taken modulo `p^K` (`K > c`, from `Log_core`);
`m = min(2v, K)`; `l = u mod p^m` (least nonnegative residue).

Step 1. `l = u` modulo `p^m` and `m >= v`, so `l = 1` modulo `p^v`: a unit, and `l >= 1`. `u/l - 1 = (u - l)/l`
with `u - l` in `p^m Z` and `l` a unit: in `p^m Z_p`. Both `l` and `u/l` are in `1 + p^v Z_p`, inside
`1 + p^c Z_p`, inside `1 + p Z_p`, where Lemma 9 gives `log(u) = log(l) + log(u/l)`. Right.

Step 2. If `u' = u` modulo `p^K` then `u'/u = 1 + p^K t/u`, in `1 + p^K Z_p`, and Lemma 9 item 5 (valid for
`K >= c`) gives `v(log(u'/u)) >= K`, so `log(u') = log(u)` modulo `p^K`. Right: so the factorisation may
be carried out on residues. Residual: `u - l = p^m d`, `u/l = 1 + p^m (d/l)`; if `i = l^(-1)` modulo `p^(K-m)`
then `p^m d i = p^m d / l` modulo `p^K`. Right: `K - m` digits of the inverse suffice, and the new `u` is
`1 + p^m ((d i) mod p^(K-m))`, a residue in `[1, p^K)`. The code does exactly this (lines 486-489: `term =
(u - low) / Q`, `z = p^(K-m)`, `inv = low^(-1) mod z`, `u = 1 + Q ((term inv) mod z)`). At `m = K` the residue
`l = u` and `u/l = 1`: no residual, the loop breaks (482). Right.

Step 3. `v -> m = 2v` while `2v <= K`, else `m = K`: the code's `m = v <= K/2 ? 2v : K` is `min(2v, K)` for
every parity of `K` (`v <= floor(K/2)` iff `2v <= K`). Finitely many steps, `ceil(log2(K/v)) + 1`. Right.
Each factor's `z = l - 1` satisfies `0 <= z < p^m`, and if `z != 0` then `w = v_p(z)` satisfies `v <= w < m
<= K`. So `T = count(K, w) >= 1` (see MINOR 4 (c)).

Step 4 (the tree). Leaf `(a, a+1)`: `A/B = 1/a`, `R = q`: `sum_{k=a}^{a} q^(k-a)/k = 1/a`. Right. Split at
`d`: `sum_{k=a}^{b-1} q^(k-a)/k = sum_{k=a}^{d-1} q^(k-a)/k + q^(d-a) sum_{k=d}^{b-1} q^(k-d)/k
= A_L/B_L + R_L A_R/B_R = (A_L B_R + R_L A_R B_L) / (B_L B_R)`, and `R_L R_R = q^(d-a) q^(b-d) = q^(b-a)`.
The code (436-438) forms `tmp = R C B` (`R = R_L`, `C = A_R`, `B = B_L`), `A = A D + tmp` (`D = B_R`),
`B = B D`, `R = R U`. Right, exact integers, no reduction: no digit can be lost. Right.

Step 5. `z A_(1,T+1)/B_(1,T+1) = sum_{k=1}^T z q^(k-1)/k = sum (-1)^(k-1) z^k/k` with `q = -z`. Right: the
partial sum of Proposition 7b. `B_(1,T+1) = T!`, `v_p(T!) = D`. Each `z^k/k` has valuation `k w - v_p(k) >= k -
v_p(k) > 0`, so the sum is in `Z_p`; hence `(A z) / (p^D B')` in `Z_p` with `B'` a unit means `A z / p^D`
is in `Z_p` and is a rational with denominator a power of `p`: an integer. So `p^D | A z` exactly and the
result is `(A z / p^D) B'^(-1)` modulo `p^K`. Right; the code (452-456) does this with `fmpz_remove`,
`fmpz_divexact`, `fmpz_invmod`.

Step 6. Each factor's partial sum is `log(l_i)` modulo `p^K` by Proposition 7b for its own `w_i >= v_i`;
the sum of the factor logs is `log(u)` modulo `p^K` by step 1 and 2 (each residual is congruent to the true
quotient modulo `p^K`, and `log` respects that by step 2). Right. F6 gives the ball from the residue, as
before. Right.

Where F9 rests on F8: nowhere; F9 is independent of F8 (the route selection chooses one or the other). Where
it rests on Proposition 8: nowhere either, since the tree needs no working precision; only the LIMIT test of
F7 (`pow_ok(p, W)` with `W` of F5) is kept before the route is chosen, unchanged. F9 rests on Lemma 9 (product
identity on `1 + p Z_p`, item 5 for `r >= c`) and Proposition 7b (the count for each factor).

Boundaries. `p = 2`, `v = 1`: excluded by the hypothesis `v >= c = 2`, and the code's `vz >= 2` at `p = 2`
(above); F9 does not claim that case and the code never reaches it. `3 + 4 Z_2`: the code passes
`u = -a` (residue `1` modulo 4), fine. `K` just above a power of `p`: irrelevant to F9. `v > K/2` at entry:
`m = K` at once, `l = u`, `T = 1` (as `2w - e(2) >= K + 1 - 1`), `log(u) = u - 1` modulo `p^K`: the same as
the old route with `T = 1`.

Why both routes must give the SAME ball: both `log_sum_word` (old) and `log_sum` (F8) return the residue
modulo `p^K` of the same rational number, the partial sum `S_T(z_r)` with the same `T` and the same `z_r`
(the old one because each of its terms is `z_r^k/k` modulo `p^(W - e_k)`, `W - e_k >= K`; the new one by
F8 steps 1-2); and `log_balanced` returns the residue modulo `p^K` of `log(1 + z_r)`, which equals
`S_T(z_r)` modulo `p^K` by Proposition 7b. `set_ball_residue` is a function of that residue and `K`. So the
identity of the results is a theorem given F8, F9 and Proposition 7b, not a coincidence of the fixture;
the numerical comparison below tests the implementation, not the mathematics.

Verdict on F9: true as written, every step checked; the code implements it with the digit counts of the
proof (`K - m` for the residual inverse, no reduction in the tree, `T` from the exact valuation of each
factor).

## 2. The code against F8 and F9 (`src/lfunc.c`)

Route (lines 599-604): `P = p^W` with `W = K + e(T)` from F5, formed before the choice; `fmpz_bits(P) <= 62`
(P below `2^62`, a tagged word) takes the old loop `log_sum_word`; otherwise `K <= 64` takes `log_sum`
(F8), `K > 64` takes `log_balanced` (F9). Where the boundary lies: `p = 2`, old loop for `W <= 61`
(`K` up to about 57); `p = 3`, `W <= 39` (`K` up to about 36); `p = 65537`, `W <= 3` (`K <= 2`);
`p = 2^64 - 59`, never (`W >= 1` gives 64 bits): every result with `K >= 1` at the word prime goes through
F8 (`K <= 64`) or F9. The exact-result and no-sum cases (`K <= c`, unit part 1, `vz >= K`) precede the
choice and are unchanged. Digit counts: section 1. The word division: section 1, F8 step 5; `r + j Q` is an
`fmpz`, not a word.

Checked by forcing: the three routes were compiled into one program (`lanes/f-review4/build.sh`: the
worktree's `lfunc.c` with the selection replaced by `log_sum` alone, and by `log_balanced` alone, under
renamed symbols) and run on every input of section 3 beside the library and the old code: 0 differences
in 21188 inputs, of which 7440 have `N <= 80` (both new routes at word-size moduli, where the library takes
the old loop) and 9548 have `N` in `60 .. 400` at `p = 2, 3` (every `N`).

## 3. Old route, new route, my oracle, FLINT: the same inputs

Oracle (`lanes/f-review4/oracle.py`, own proof in its header): `log(1 + z)` for a rational `z`, `w = v_p(z)
>= 1`, as the EXACT rational partial sum of degrees `1 .. T0 - 1`, `T0 = ceil(2(n+4)/(2w-1))`, formed as a
pair of integers by a divide-and-conquer that never reduces (independent of the library's tree: it carries
the powers of `z` inside the recursion and a denominator `z_den^(b-a) prod k`), then reduced modulo `p^n`
(the tail is in `p^(n+4) Z_p` since `k w - v_p(k) >= k (w - 1/2)`); cross-checked on 857 inputs against the
naive `Fraction` sum term by term (0 mismatches). `Log(x)`: `x = p^m a`; at 2, `log(a)` by the series on
`1 + 2 Z_2` (`log(-1) = 0`); at odd `p`, `w = +-1` exactly when `a = +-1` modulo `p`, else the root of unity
by Newton on `T^(p-1) - 1` to `p^(n+3)` (own implementation) and the series at the residue `a/w`. Where the
exact sum would exceed `3e7` bits (random centres of `1000` or more digits), a Proposition-8 sum with ONE
common denominator `lcm(1..T0-1)` modulo `p^(n + e)` is used and marked `direct` (own code, not an
independent method); above `n = 6000` such a case is left to FLINT. FLINT: `padic_log`, and
`padic_teichmuller` + `padic_div` for `Log` at odd `p`, at the centre, precision `K` (probe.c). Ball inputs
also check that `f(a + p^M)` and `f(a - p^M)` lie in the returned ball ("point").

Every comparison is field by field (status, exact flag, `v`, `N`, numerator, denominator) against the new
library; aliasing (`y = x`) is checked on every call. Commands (from the worktree root; `<b>` is the
scratch directory of `build.sh`):

    timeout 300 make -j2 BUILD=lanes/f-review4/build            # exit 0
    timeout 120 sh lanes/f-review4/build.sh <b>                  # exit 0
    timeout 120 python3 -B lanes/f-review4/oracle.py <b>/probe selftest   # 524 checks, 0 mismatches
    timeout 900 python3 -B lanes/f-review4/oracle.py <b>/probe small
    timeout 900 python3 -B lanes/f-review4/oracle.py <b>/probe exhaustive
    timeout 900 python3 -B lanes/f-review4/oracle.py <b>/probe random
    timeout 1500 python3 -B lanes/f-review4/oracle.py <b>/probe large
    timeout 1500 python3 -B lanes/f-review4/oracle.py <b>/probe largeball

| mode | inputs | old = new | F8 forced = new | F9 forced = new | oracle checks | FLINT checks | point checks | time |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `small`: 7 primes, `N = 1 .. 80` every `N`, 6 exact + 4 ball inputs per `N` (log, Log; `v < 0` and `v > 0` balls; `N > M`) | 7440 | 7440, 0 diff | 7440, 0 diff | 7440, 0 diff | 7040 (7019 exact, 21 zero), 0 fail; 330 naive, 0 | - | 4480, 0 fail | 24 s |
| `exhaustive`: `p = 2, 3`, `N = 60 .. 400` every `N`, same families | 9548 | 9548, 0 | 9548, 0 | 9548, 0 | 8184 exact, 0 fail; 400 naive, 0 | - | 5456, 0 | 30 s |
| `random`: 7 primes, 600 inputs each, `N = 1 .. 400` (300 at the word prime), random centres | 4200 | 4200, 0 | 4200, 0 | 4200, 0 | 4130 exact, 0 fail; 127 naive, 0 | 4132, 0 fail, 0 refused | 5146, 0 | 381 s |
| `large`: 7 primes, `N = 1000, 5000` (all) and `20000` (`p <= 11`), exact `log(1+p)`, `log(1-p)`, `log((1+p)/(1-p))`, `Log(2)`, `Log(5/7)`, `Log(2/p^3)`, `Log(1+p)` (+`log 3`, `log -5` at 2) | 136 | 129, 0 diff (old not run at `p = 2^64-59`, `N = 5000`) | 129, 0 diff | 136, 0 diff | 121 (100 exact, 21 direct), 0 fail; 9 skipped (`n > 6000`, full-size residue) | 130, 0 fail, 0 refused | - | 128 + 398 + 216 s, three runs (`large.log`) |
| `largeball`: random balls at `N = 1000`, `5000` | not run (stopped at the orchestrator's request; the mode exists in `oracle.py`) | | | | | | | |

Exact results (`Log(+-p^m)`, `log(1)`) and non-OK statuses are compared field by field and excluded from
the oracle counts. The `random` mode ran 381 s in one process and the `large` parts up to 398 s, above
the 3-minute rule of `lanes/COMMON.md` (the Python oracle at `N = 20000` costs about 8 s per input; the
old route at `p = 11`, `N = 20000` costs 17 s per call); they should be split by prime next time. A first `large` run over all
primes was stopped by its 1500 s timeout with no output (the old and the F8 route at `N = 20000` for
`p = 5 .. 11`, and the oracle's direct sum at the word prime, `N = 5000`, 309 s, add up); it was rerun
in parts. No input on which any two of the five (old, new, F8 forced, F9
forced, oracle/FLINT) differ was found.

## 4. Cost

Section "Findings", MINOR 2 (small `N`), MINOR 3 (scaling and memory). Files `rate.log` (1920 rows),
`rate-recheck.log`, `scale.log`, `mem.log`; programs `bench.c`, `bench.sh`. Commands:
`sh lanes/f-review4/bench.sh <b> <input>` with the input lines of `rate.log` (8 primes, `N = 1 .. 80`,
`log(1+p)`, `Log(13/17)`, `Log(13/17 p^-2)`, 1500 alternating calls each, 268 s in one process), of
`scale.log` (3 calls each, 31 s) and of `mem.log` (one call per process). Nothing was pinned to a core;
another lane (codex) was running on the machine at the time.

## 5. Planted faults

`lanes/f-review4/faults.py <scratch> [names]`: each fault is a text replacement in a scratch copy of
`src/lfunc.c`, compiled with the project's flags into a copy of the archive, linked with the unmodified
`tests/test_lfunc.c` (`timeout 600`), and the probe of section 3 is built against the faulty archive and
run in mode `small` (7440 inputs).

| fault | what | `stored_before_optimisation` failed checks | other tests of `test_lfunc` that fail | probe `small`: inputs that differ from old |
|---|---|---:|---|---:|
| `f8_digit` (brief a) | unit power of F8 modulo `p^(H_k - 1)` | 650 | cases_from_the_reference 30, identities_as_containment 429, flint_second_opinion 209 | 3272 |
| `f9_valuation` (brief b) | factor count `T` for valuation `w + 1` | 18, then SIGSEGV (exit -11: `T = 0` at the last factor, endless `log_split`) | cases_from_the_reference 2, flint_second_opinion (before the crash) | probe crashes (exit 1) |
| `f9_tree` (brief c) | combine `A = A_L B_R + R_L A_R` (no `B_L`) | 18 | cases_from_the_reference 2, flint_second_opinion 336, precision_2000 8 | 1408 |
| `f9_inverse` (own) | residual inverse modulo `p^(K-m-1)` | 10 | flint_second_opinion 266, precision_2000 2 | 824 |
| `f9_inverse_bigp` (own) | the same, only for `p > 2^32` | 0 | precision_2000 2 | 160 |
| `f8_digit_K40` (own) | `f8_digit` only for `K > 40` | 0 | cases_from_the_reference 1, flint_second_opinion 40 | 1621 |
| `control_lower_bound` | factor count for the lower bound `v` instead of `w` (not a fault) | 0 | none (0 failed checks of 454429) | 0 |

The three faults of the brief are caught by the stored comparison (650, 18, 18 failed checks of its 4000).
18 = 9 rows x 2 checks (value and alias): exactly the 9 rows that reach F9 (MAJOR 1). The two own faults
that act only where the fixture has no row survive the stored comparison and are caught only by other
tests. The control confirms that the harness reports 0 for a correct variant.

Unmodified suite: `timeout 600 lanes/f-review4/build/test_lfunc`: 12 tests, 454429 checks, 0 failed, 4.6 s.

## What was attacked without result

- F8, every step, including the four boundaries named by the brief (section 1): no false step.
- F9, every step, the valuations of the factors, the residual inverse's digit count, the exactness of the
  tree, the `p`-part of `T!`, the dependence on Lemma 9 and Proposition 7b (section 1): no false step.
- The code's digit counts against the proof, line by line (section 1 and 2): none one too small.
- Route boundary: three routes forced on 21188 inputs (7440 of them at `N <= 80`, where the library would
  never use the new routes): 0 differences.
- Old versus new on 21188 + 129 inputs: 0 differences; my oracle on 19475 results (19433 exact rational
  sums, 21 exact zeros, 21 by the direct sum; 100 of them at `N >= 1000`): 0 failures; FLINT on 4262
  centres: 0 differences, 0 refusals; 15082 ball points `a +- p^M`: all inside the returned ball; every
  call with `y = x`: same result.
- `Log` with negative valuation (`v` from -6 to -1, exact `2/p^3`, `-7/p`, balls with `v < 0`): included in
  every mode above, 0 differences.
- A small-`N` regression: found one (MINOR 2), 13 percent at one term; none at `N >= 8`.
- Memory of the tree at `N = 100000`: at most 43 MiB above the empty process, for 0.8 MiB of input.

## Files written (all under `lanes/f-review4/`)

`probe.c`, `build.sh`, `oracle.py`, `faults.py`, `bench.c`, `bench.sh`; logs `rate.log`, `rate-recheck.log`,
`scale.log`, `mem.log`, `large.log`; `result.md`. The build directory `lanes/f-review4/build`
was removed at the end; scratch copies and binaries were under the session scratchpad (outside the tree).
Nothing outside `lanes/f-review4/` was changed; no git command that changes state, no `bd`.

## Not done

- `N = 20000` at `p = 65537` and `2^64 - 59` (mode `large`, part 4): stopped after 10 minutes at the
  orchestrator's request before the first prime completed; the F9 route at these sizes was checked against
  FLINT only at `N = 5000` (7 + 7 inputs, 0 differences) and by the lane's own benchmark, not by me.
- Mode `largeball` (random centres of 1000 and 5000 digits, all primes): not run. Random centres above 400
  digits were therefore not compared old/new by me; the stored fixture has 9 such rows (MAJOR 1).
- No exact rational oracle for random centres of 1000 digits or more (the sum has `10^9` bits); `Log` at odd
  `p` with a residue not `+-1` modulo `p` is checked at `N >= 5000` by FLINT only (9 inputs skipped).
- No measurement pinned to one core; the ratios of MINOR 2 are from alternating calls in one process, three
  runs, which is what the lane did.
- No review of `exp` (unchanged by the lane) or of the benchmark `bench/bench_lfunc.c`.

## Sources pending

None. FLINT's `padic_log` and `padic_teichmuller` are cited from `refs/src/flint-3.0.1/padic.rst:490-497,
551-559`; `n_mulmod2` and `n_invmod` from `ulong_extras.rst:375-380, 477-483`.

## Findings against the specification

None. `docs/SPEC.md` was not contradicted by any result.
