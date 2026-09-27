# Adversarial review of docs/proofs/functions.md

Object: `docs/proofs/functions.md` (725 lines, 22 numbered statements) and `proto/functions_checks.py`.
Scope: SPEC 9.3.1 to 9.3.6. Reviewer's checks: `docs/reviews/m0-proofs/functions_review_checks.py`, written
without importing the author's functions. All p-adic values there come from exact integer powers with exact
division by the p-part of each denominator; the torsion factor is computed as the limit x^(p^K), not by the
author's digit lifting. The author's file is used only in section M, as source text, to run the author's checks
against mutated formulas.

Result: 20 VALID, 2 MINOR, 0 INVALID. No counterexample was found to any formula that milestone 1F will
implement: truncation counts, working precision, radius rules, Log images, root criterion, root precision under
the guard, and the principal-unit power radius. Each of these was tested by exhaustive enumeration modulo p^k,
including the edge cases named in the brief.

## Summary

| # | Statement | Verdict | One line |
|---|---|---|---|
| 1 | Definition 1 (places, precision, series) | VALID | Definitions only; consistent with SPEC 9.3 notation |
| 2 | Lemma 2 (real facts assumed) | VALID | Declared as assumed, `[source pending]`; nothing claimed beyond that |
| 3 | Lemma 3 (convergence, lifting, cyclic groups) | VALID | Standard; proofs complete |
| 4 | Prop 4 (p^m w u) | VALID | Exhaustive against brute force mod 2^8, 3^5, 5^4, 7^3 |
| 5 | Lemma 5 (Legendre and v_p(k) bounds) | VALID | 15000 cases, including the bound used by my own oracle |
| 6 | Prop 6 (exact domains) | VALID | p=2, v=1 shell: terms at 2^j and 2^j+1 have valuation 1 and 2 |
| 7 | Prop 7 (truncation counts) | VALID | 2,352,855 omitted terms, n to 150, none below n; log count 2x too long |
| 8 | Prop 8 (working precision W) | MINOR | True; exact divisibility of y^k mod p^W by p^e used, not proved |
| 9 | Lemma 9 (identities, exp/log bijections) | VALID | Denominator bound (d-1)/(p-1) rechecked by hand |
| 10 | Prop 10 (series radii, cos hull) | VALID | Ball images enumerated; centred cos hull 2N - v_p(2) exact |
| 11 | Prop 11 (log radii, Iwasawa Log) | VALID | 360 Log balls, m in -3..2, r = 1..3, including r = 1 at p = 2 |
| 12 | Prop 12 (domain D, Log enclosure) | MINOR | True; step 4 cites Prop 11 for a fact Prop 11 does not state |
| 13 | Prop 13 (root criterion, counts) | VALID | 5216 unit classes, n up to 25, both directions witnessed |
| 14 | Prop 14 (real roots) | VALID | Standard; modulo the intermediate value theorem |
| 15 | Prop 15 (guarded root precision) | VALID | 660 exact images at guard, guard+1; guard-1 fails if p divides n |
| 16 | Prop 16 (all places, rational roots) | VALID | Prime construction and the rational criterion rechecked |
| 17 | Prop 17 (four powers) | VALID | Integer compatibility incl. negative s; 2-adic sign formula pointwise |
| 18 | Prop 18 (base, exponent uncertainty) | VALID | 1680 exact images; each term strict minimum in 134-592 cases |
| 19 | Prop 19 (p-primary fractional part) | VALID | Proof complete |
| 20 | Prop 20 (psi is not cos) | VALID | Sign convention `[source pending]`; the claim holds for either sign |
| 21 | Prop 21 (no order on Q_p) | VALID | Proof complete, all p |
| 22 | Prop 22 (typed values, invertibility) | VALID | Proof complete |

## Details per statement

### 1. Definition 1 (VALID)

Read only. The meaning of "precision p^n" (error in p^n Z_p, n any integer) matches SPEC 9.3.2 and is used
consistently later.

### 2. Lemma 2 (VALID)

It lists real and complex facts used without proof and marks them `[source pending]`. That is honest under
rule 4 of `lanes/COMMON.md`. Only Props 12, 14, 16, 17 and 20 depend on it, and only in their real or complex
clauses.

### 3. Lemma 3 (VALID)

Step 2 (simple Hensel lifting): the Taylor remainder has integral coefficients because f is in Z_p[T], so
f(r + t p^k) = f(r) + t p^k f'(r) mod p^(k+1) holds. Step 3 (cyclic finite groups) and step 4 (gcd count) are
complete. Wording in step 3, "take an element whose order has that exponent", means "whose order is divisible
by that full prime power"; the argument is correct.

### 4. Prop 4 (VALID)

Tried: every unit modulo 2^8, 3^5, 5^4 and 7^3, times p^(-3), against a brute-force search for pairs (w, u) with
w in mu_(p-1) (odd p) or {1, -1} (p = 2) and u = 1 mod p^c. The pair was always unique and equal to the one the
statement prescribes (check D2, 1084 units). At 2, w(-1) = -1. The Teichmueller convention "t^(p-1) = 1, hence
1 at p = 2" is the author's definition and is marked `[source pending]`. It agrees with SPEC 9.3. Some texts use
the other convention, with value +/-1 at 2; the pending source should settle which one the API name follows.

### 5. Lemma 5 (VALID)

Check L: for p in 2, 3, 5, 7, 13 and k up to 3000: v_p(k!), counted factor by factor, equals (k - s_p(k))/(p-1)
and is at most (k-1)/(p-1); v_p(k) <= (k-1)/(p-1) and v_p(k) <= k/2. No failures.

### 6. Prop 6 (VALID)

Step 1: k d - v_p(k!) >= k(d - 1/(p-1)) + 1/(p-1); for integral d this is unbounded exactly when d >= c. Step 2:
d <= 0 gives terms of valuation <= 0. Step 3 at p = 2, d = 1: term valuation k - v_2(k!) = s_2(k), so degrees
2^j give 1 and degrees 2^j + 1 give 2 (check D1, for x = 2, 6, -2, 10, 24690 and j = 1..9). Step 4: log fails at
v(z) <= 0 along k = p^j. The statement is exactly the SPEC table column "converges exactly on".

### 7. Prop 7 (VALID)

Proof: for k >= 1, k v - v_p(k!) >= (k d + 1)/(p-1) with d = (p-1)v - 1 >= 1 because v >= c. The bound is
increasing in k, so every degree k >= K is safe. This settles the brief's concern that the valuation of
x^k/k! is not monotone. The first omitted degree of sin and sinh is 2 floor(K/2) + 1 >= K; for cos and cosh it
is 2 ceil(K/2) >= K. For log, k v - v_p(k) >= k(2v - 1)/2.

Tried (check T1): the literal formulas, coded from lines 170 to 181, for p in 2, 3, 5, 7, 13, v from c (from 1
for log) to that value plus 5, and n from -6 to 150. For each case I tested every omitted degree from the first
omitted one up to 4T + 40, with the worst-case valuation v(x) = v. In all 28,260 cases none of the 2,352,855
omitted terms had valuation below n. Beyond 4T + 40 the monotone bound of the proof applies. For n <= 0: K = 1,
so only the constant term is kept, and the sin and log sums are empty (check T4).

Slack (check T2 and T3). The factorial counts are close to minimal: at most 7 surplus terms (exp at p = 3), and
the count is exactly minimal in 2310 to 3071 cases per function. The log count uses v_p(k) <= k/2 and is about
twice what is needed: 149 surplus terms at n = 150 for every p. Nothing is wrong, but Iwasawa Log and the
series log run about 2x longer than they need to (PERF). Optional repair O-1 below gives a tight integer rule,
checked safe in 3925 cases (check T5).

### 8. Prop 8 (MINOR)

The statement is true. Check W1 implemented the algorithm literally: x modulo p^W, powers modulo p^W, exact
division by p^e, the unit inverse modulo p^(W-e), then reduction modulo p^n. It was run for p in 2, 3, 5, 7,
all six series, v in {c, c+1, c+3}, n up to 34, random x with v(x) = v to v + 2, and three different lifts of
x modulo p^W. The result matched an exact oracle in all 6912 evaluations, and no division was inexact.

Necessity (check W2): W - 1 gives a wrong digit in 2007 of the 6912 evaluations. Dropping D (W = max(v, n)) gives
a wrong digit in 1914. So D is needed and the formula is sharp in many cases.

Gap. The algorithm says "divide the numerator by p^e". Step 2 of the proof says only "Dividing by d_k loses
exactly e absolute digits". It never shows that the reduced integer y^k mod p^W is divisible by p^e, and without
that the step is not an exact integer operation. In C this is the precondition of `fmpz_divexact`. The fact
holds for two reasons. First, v(y) >= v because W >= v, so v(y^k) >= k v > v_p(k!) >= e by Lemma 5. Second,
W >= e: a nonconstant term is kept only when n > v (factorial series: K >= 2 forces (p-1)n - 1 > d) or n >= v
(log: J >= 2 forces 2n > 2v - 1), so n >= 1 and W >= n + D > e. Repair R-1 adds this.

A side effect of the same argument is that the "max with v" in W never binds when a nonconstant term is kept.
That explains the one mutant that survives the author's check (section "Checks", M): it is an equivalent
mutant, not a weak check.

### 9. Lemma 9 (VALID)

I rechecked step 4 by hand. A term of E(L) with outer degree j and inner degrees k_1, ..., k_j summing to d has
denominator valuation v_p(j!) + sum v_p(k_i) <= (j-1)/(p-1) + sum (k_i - 1)/(p-1) = (d-1)/(p-1). With inputs of
valuation >= c the term valuation is >= d c - (d-1)/(p-1), which tends to infinity. For each d there are
finitely many terms (j <= d). The same bound holds for L(E - 1). Step 5 gives v(exp(x) - 1) = v(x) and
v(log(1+z)) = v(z) on p^r Z_p for r >= c, so the discs map into each other and the formal inverses give the
bijections. At p = 2 the restriction to r >= 2 is necessary (exp(log(-1)) = 1), and the statement makes it.

### 10. Prop 10 (VALID)

Tried (check R1): p in 2, 3, 5; N = c, c+1, c+2; up to 9 centres a in p^c Z_p per N. All centres modulo p^N
were used where there are at most 8, and a random sample otherwise. For each ball I enumerated all points
modulo p^M, M = 2N - v_p(2) + 1.
- exp, sin, sinh: the image is exactly f(a) + p^N modulo p^M. This is equality of sets, so it covers both the
  hull and surjectivity.
- cos, cosh: the image lies in f(a) + p^(N+1), which is stronger than "N is safe", as proof step 2 says.
- Centred balls: the hull exponent is exactly 2N - v_p(2).

210 balls, no failure. Golden values: cos 4 = 9 mod 16 at 2, v_3(cos 3 - 1) = 2, sin 3 = 3 mod 9 (check R3).

Info for 1F, not a defect (check R2). For a non-centred cos or cosh ball, the rule min(N + v(a), 2N - v_p(2))
was never below the true hull exponent in the balls tried. It is not the hull at p = 2 when v(a) = N - 1,
because the linear and quadratic terms cancel there (example: p = 2, N = 3, a = 4 gives hull exponent 6 against
5). The rule is not proved in the file. Do not use it as a tight rule without a proof.

### 11. Prop 11 (VALID)

Tried (checks G1, G2, G3).
- Series log: balls a + p^N in 1 + p Z_p, N = 1..4, p in 2, 3, 5. The image is exactly log(a) + p^N for N >= c,
  and exactly 4 Z_2 for p = 2, N = 1. 64 balls.
- Iwasawa Log: centres a = p^m * unit with m in {-3, -1, 0, 1, 2}, 8 units each, r in {1, 2, 3}. The ball is
  written with its literal N = m + r, and r is recomputed as N - v(a). The image set equals Log(a) + p^r for
  r >= c, and 4 Z_2 at p = 2, r = 1 (the brief's N - m = 1 case). 360 balls, no failure.
- Golden values: log(-1) = 0 at 2; v_2(log 3) = 2; the losses at 3, 12 (p = 3) and 2, 10 (p = 2); Log(p) = 0.

The proof shows both directions: step 4 uses the exact bijections of Lemma 9, not only a Lipschitz bound.

### 12. Prop 12 (MINOR)

The mathematics is right. Step 4 says "Proposition 11 puts every local Log in p^c Z_p". Prop 11 does not state
this. It follows from Prop 4 (u in 1 + p^c Z_p) and Lemma 9 (log maps 1 + p^c Z_p onto p^c Z_p). See repair R-2.
Steps 2 and 3 (no rational ball in D; 0 is its only rational point) were read and are complete.

### 13. Prop 13 (VALID)

Tried (check Q1): all unit classes modulo p^K, K = max(2 v(n) + 2, c + v(n) + 1), for p in 2, 3, 5, 7 and
n in 1..10, 12, 16, 25, as far as p^K <= 3000. This includes n = 4, 8, 16 at 2 and n = 9 at 3. When the
criterion holds, the root t exp(log(u)/n) was built and checked, z^n = a modulo p^(K+v(n)+2). When it fails, a
finite witness was checked: a is not an n-th power modulo p^K. 5216 classes, no failure.
- Counts (check Q2): the number of residues z modulo p^(c+v(n)+1) with z^n = a modulo p^(c+2v(n)+1) equals
  gcd(n, p-1) at odd p and gcd(n, 2) at 2.
- Square tests (check Q3): 1 mod 8 at 2; quadratic residue at odd p; 3 fails, 9 passes.

### 14. Prop 14 (VALID)

Standard, modulo the intermediate value theorem (Lemma 2).

### 15. Prop 15 (VALID)

Tried (check P1): unit level, beta = zeta (1 + p^c t) with zeta running over up to three torsion units (so the
non-principal branches are included). p in 2, 3, 5, 7; n in 1..6, 8, 9, 10, 12, 25, so degrees p and p^2 are
included; r at the guard c + v(n) and one above. Both directions were tested:
- the n-th powers of the output ball beta + p^(r-v(n)) are exactly the input ball beta^n + p^r;
- every z = beta mod p^c whose n-th power lies in the input ball lies in the output ball.

660 cases, no failure. Check P2 tested the literal exponent N - v(n) - (n-1)j with Fractions, for j in -2..2
(negative valuations) and N at the minimum the guard allows and one above. Where a branch point just outside
exists (Eout - 1 - j >= c), it maps outside the input ball, so the exponent cannot be lowered. 150 cases.

One step outside the guard (check P3): when p divides n, the ball 1 + p^(c+v(n)-1) contains a non-n-th power for
(p, n) = (2,2), (2,4), (2,6), (3,3), (3,9), (5,5), (3,6). So the guard is necessary there.

Observation, not a defect (check P4). At p = 2 with odd n the guard r >= 2 is stronger than needed. For r = 1
every point has exactly one n-th root, and the image of the ball is b + 2^(j+1) Z_2, which is the formula with
r = 1. The "ratio in 1 + 4 Z_2" branch of the statement does not exist for half of those points, so the
statement as written correctly excludes r = 1. SPEC's NOT_DETERMINED there is safe but unnecessary. See the
optional O-2. At odd p with p not dividing n the guard r >= 1 is minimal, since r = 0 means the ball contains 0.

### 16. Prop 16 (VALID)

Step 1 was checked by hand: q divides H = 1 + A + ... + A^(ell-1), H = 1 modulo every prime of S, A^ell = 1
modulo q, and A = 1 modulo q would force q = ell, which is in S. So A has order ell modulo q, and ell divides
q - 1. The criterion for rationals uses all places, including 2 and the real place. A Grunwald-Wang type
exception (16 is an 8th power at every odd place) cannot occur because p = 2 is included: v_2(16) = 4 is not
divisible by 8.

### 17. Prop 17 (VALID)

Check X2: exp(s log u) = u^s modulo p^k for u in {1 + p^c, 1 - p^c, 1 + 7 p^(c+1)} and s in {-7, -2, -1, 0, 1,
3, 10}; exp(Log p) = 1. Check X3, pointwise at 2: x^s = w^(s mod 2) exp(s log u) for odd x modulo 2^8. The
continuity claim in step 2 also needs the continuity of exp (Prop 10); the proof cites only Prop 11. This is
trivial and needs no repair.

### 18. Prop 18 (VALID)

Tried (check X1): exhaustive images of {u^s : u in u0 + p^A, s in s0 + p^B} modulo 2^8, 3^5 and 5^4, with
exponents taken modulo p^(k-c) (the period of u^s modulo p^k).
- Bases: u0 in {1, 1 + p^c, 1 + p^(c+1), 1 + 3p^c, 1 + p^(c+2)}, so alpha takes the values c, c+1, c+2 and
  infinity. The author's check has only c and infinity.
- A from c to c+3; s0 in {0, 1, -1, 2, p, p^2, 3p+1}; B in 0..3.

1680 images, all equal to u0^s0 + p^min(A+beta, B+alpha, A+B). The test separates the three terms: A + beta is
the strict minimum below k in 592 cases, B + alpha in 421, A + B in 134.

At 2 with sign (check X3): for A >= 2 and B >= 1 the ball rule holds after multiplying by the known sign. For
A = 1 or B = 0, 64 of the cases have images that meet both classes modulo 4, so a single principal-unit
formula would be wrong there. This is what step 5 says. Step 5 gives no explicit formula for these cases; 1F.6
has to split or return a status. SPEC 9.3.4 does not claim more.

### 19 to 22 (VALID)

- Prop 19: uniqueness (r - r' is an integer in (-1, 1)) and constancy exactly for N >= 0 are complete.
- Prop 20: the convention is quoted from SPEC 6 and its source is marked pending. psi(1/4) = 1 holds with
  either sign convention.
- Prop 21: the sum-of-two-squares argument at odd p and -7 = 1 mod 8 at 2 are complete.
- Prop 22: the invertibility argument (x and x^(-1) both integral almost everywhere) and the counterexample
  x_p = p are complete.

## Judgement of the author's checks

Mutation run (section M of my checks): I applied 22 textual mutants to the formulas inside
`proto/functions_checks.py` and ran the corresponding author check. 21 were killed. The one survivor,
W = max(1, n + D) instead of max(v, n + D), is an equivalent mutant (see statement 8). The author's structural
checks are therefore strong on the formulas that 1F will implement.

Weak checks, by name:
- `check_regression_mutations` is not mutation testing. It asserts 13 fixed scalar facts, some of them
  tautological: `vp(10**3-1, 3) == 3`, `5 not in {x*x % 8}`, and `{b for b in range(1,16,4) if b*b % 8 == 1}`.
  Nothing in it changes a formula and reruns a check. The claim "13 rejected wrong rules" in the commit message
  overstates what it does. CLAUDE.md rule 2 asks for mutation testing of precision rules; section M of my file
  supplies it.
- `check_typed_and_projection`, `else` branch: `vp(-a, p) >= n` and `vp(0, p) != vp(p**n, p)` hold by
  construction and test nothing.
- `check_power_precision`: alpha is only c or infinity, never a finite value above c. At p = 13, k = 2 almost
  every radius is capped at k, so that prime tests nothing.
- `check_root_precision`: b is only in {1, 2} (or {1, 3} at 2). There is no n = 9 at 3 and no n = 25 at 5. The
  "scaled" part tests only one direction (the output ball maps onto the input ball) and uses only p^2 sample
  points.
- `check_working_precision`: one input x = -p^v/(p+1) per case and two lifts. It still kills W - 1 and
  W without D.
- `check_real_and_character` tests real roots only on integer points; this is declared.

## Repairs

R-1 (Prop 8, proof step 2). Replace step 2 by:

    2. Since W >= v, the representative y satisfies v(y) >= v, so v(y^k) >= k v > v_p(k!) >= e by the
       estimate in Proposition 7 step 1 (for log, k v > v_p(k) >= e). A nonconstant term is retained only
       when n >= 1: for the factorial series K >= 2 forces (p-1)n - 1 > (p-1)v - 1, i.e. n > v, and for log
       J >= 2 forces n >= v. Hence W >= n + D > e, and the integer residue y^k mod p^W is divisible by p^e;
       the division is exact. Dividing by d_k then loses exactly e absolute digits. The remaining error lies
       in p^(W-e) Z_p, contained in p^n Z_p because e <= D. Multiplication by a unit inverse loses no
       precision. The same argument shows that max(v, .) in W never binds when a nonconstant term is kept.
       This explains why rounding each numerator at n before dividing is not a valid substitute.

R-2 (Prop 12, proof step 4, first sentence). Replace it by:

    4. Proposition 4 writes each nonzero coordinate as p^m w u with u in 1+p^c Z_p, and Lemma 9 maps
       1+p^c Z_p onto p^c Z_p. Hence every local Log(x_p) = log(u) lies in p^c Z_p, independently of the
       valuation or torsion of x_p.

Optional O-1 (Prop 7, log count, for PERF; the present count is correct). Replace the log part of the
statement by:

    For log(1+z) assume v(z) >= v, with an integer v >= 1, even at 2. Let e(k) be the largest e with
    p^e <= k, and let J be the least integer k >= 1 with k v - e(k) >= n. Put T_log = J-1.

and proof step 2 by:

    2. v_p(k) <= e(k), so the degree k term has valuation >= k v - e(k). This quantity is nondecreasing in
       k (it grows by v >= 1 per step, and e(k) grows by at most 1 per step). So every omitted degree
       k >= J has valuation >= n.

Check T5: safe in 3925 cases (p in 2, 3, 5, 7, 13; v = 1..5; n from -6 to 150). It saves up to 149 terms at
n = 150. J is found by a loop over integers; there is no floating-point logarithm.

Optional O-2 (Prop 15 and SPEC 9.3.3, only if 1F wants fewer NOT_DETERMINED results). Add:

    At p = 2 with n odd, the guard may be relaxed to r >= 1: every point of a+2^N Z_2 has exactly one n-th
    root, and the image is b + 2^(N-(n-1)j) Z_2. (For r >= 2 this is the formula above with v_2(n) = 0.)

Proof sketch: at r = 1 the ball is a times all units; x -> x^n is a bijection of Z_2^x for odd n (on +/-1 and
on 1 + 4 Z_2 by Lemma 9, since v_2(n) = 0); so the root image is b times all units. Check P4: 25 cases. Before
adoption this must be written out stepwise in the proof file.

## Checks

Command (standard library only, one core, 18 s on the laptop):

    python3 -B docs/reviews/m0-proofs/functions_review_checks.py

Output (long lines wrapped by hand; the per-mutant timings, all at most 0.3 s, omitted):

    [ok] L  Lemma 5 (Legendre, v_p(k) bounds, oracle cutoff bound): 15000 cases, 0 bad
    [ok] D1 Prop 6 boundary shells (p=2 v=1 diverges; v=c converges): 0 bad
    [ok] D2 Prop 4 unique p^m w u (odd p: mu_(p-1); p=2: w by x/2^m mod 4): 1084 units, 0 bad
    [ok] T1 Prop 7 every omitted term (to 4T+40) has valuation >= n: 28260 (p,fn,v,n) cases,
         p in 2,3,5,7,13, v in c..c+5, n in -6..150, 2352855 omitted terms, bad=0
    [ok] T2 Prop 7 slack: max surplus terms over the least safe count (info): 2/cos:2, 2/cosh:2, 2/exp:5,
         2/log:149, 2/sin:3, 2/sinh:3, 3/cos:4, 3/cosh:4, 3/exp:7, 3/log:149, 3/sin:4, 3/sinh:4, 5/cos:2,
         5/cosh:2, 5/exp:4, 5/log:149, 5/sin:2, 5/sinh:2, 7/cos:2, 7/cosh:2, 7/exp:3, 7/log:150, 7/sin:2,
         7/sinh:2, 13/cos:1, 13/cosh:1, 13/exp:2, 13/log:150, 13/sin:1, 13/sinh:1
    [ok] T3 Prop 7 cases where the count is exactly minimal (info): exp=2310, sin=3006, sinh=3006,
         cos=3071, cosh=3071, log=325
    [ok] T5 optional log count J* (least k with k v - floor(log_p k) >= n) is safe: 3925 cases, bad=0,
         max terms saved against Prop 7's T_log: 149
    [ok] T4 Prop 7 n <= 0 gives K=1 (constant only) and empty log/sin sums
    [ok] W1 Prop 8 W=max(v,n+D): modular partial sum + tail = f(x) mod p^n: 6912 evaluations
         (p 2,3,5,7; 6 functions; v in c,c+1,c+3; n up to 34; random x and lifts), bad=0,
         undefined divisions=0
    [ok] W2 Prop 8 necessity probes (info): W-1 wrong in: 2007 of 6912; W=max(v,n) (no D) wrong in
         1914 of 6912
    [ok] R1 Prop 10: exp/sin/sinh image = f(a)+p^N exactly; cos/cosh within N+1; centred hull 2N-v(2):
         210 balls (p 2,3,5; N=c..c+2; all centres mod p^N), bad=0
    [ok] R2 cos/cosh non-centred hull vs min(N+v(a), 2N-v(2)) (info, not claimed by the proof):
         6 deviations (hull exponent E != prediction), 0 with E < prediction; (p, fn, N, a, E, pred)
         e.g. [(2, 'cos', 3, 4, 6, 5), (2, 'cos', 3, 4, 6, 5), (2, 'cos', 4, 8, 8, 7)]
    [ok] R3 cos 4 = 9 mod 16 at 2; v_3(cos 3 - 1) = 2; sin 3 = 3 mod 9
    [ok] G1 Prop 11 series log: image of a+p^N (N>=1) is log(a)+p^N, at 2 with N=1 it is 4Z_2: 64 balls,
         bad=0
    [ok] G2 Prop 11 Iwasawa Log(a+p^N) = Log(a)+p^(N-m) (r>=c); 4Z_2 at p=2, r=1; m in -3..2:
         360 balls, bad=0
    [ok] G3 log(-1)=0, v_2(log 3)=2, Log losses at 3,12 (p=3) and 2,10 (p=2), Log(p)=0
    [ok] Q1 Prop 13 criterion: true => explicit root verified; false => not an n-th power mod p^K:
         5216 unit classes, p 2,3,5,7, n up to 25 incl. 4,8,16 at 2 and 9 at 3, bad=0
    [ok] Q2 Prop 13 count gcd(n,p-1) / gcd(n,2) (residues mod p^(c+e+1) with z^n=a mod p^(c+2e+1)): bad=0
    [ok] Q3 square tests: 1 mod 8 at 2; quadratic residue at odd p; 3 not a 2-adic square, 9 is
    [ok] P1 Prop 15 at the guard and one above: branch image = beta + p^(r-v(n)), both directions:
         660 (p,n,beta,r) cases, torsion and non-torsion beta, bad=0
    [ok] P2 Prop 15 literal exponent N - v(n) - (n-1)j with j in -2..2 (Fractions, minimal N): 150 cases,
         bad=0
    [ok] P3 guard - 1 with p | n: input ball 1 + p^(c+v(n)-1) contains a non-n-th power: 7 (p,n) pairs,
         bad=0
    [ok] P4 (info) p=2, n odd, r=1: unique root image is b + 2^(j+1) (guard is conservative there):
         25 cases, bad=0
    [ok] X1 Prop 18 image of (u0+p^A)^(s0+p^B) = u0^s0 + p^min(A+beta, B+alpha, A+B): 1680 exhaustive
         images (2^8, 3^5, 5^4), bad=0; cases where each term is the strict minimum below k:
         {'A+beta': 592, 'B+alpha': 421, 'A+B': 134}
    [ok] X2 Prop 17 u^s = exp(s log u) agrees with integer powers (s in -7..10); exp(Log p) = 1
    [ok] X3 Prop 17/18 at 2 for odd units: pointwise formula; ball rule for A>=2,B>=1: 180 cases, bad=0;
         A=1 or B=0 cases whose image has both classes mod 4: 64
         mutant 'K-1 for factorial series' -> check_truncation: killed
         mutant 'd=(p-1)v instead of (p-1)v-1' -> check_truncation: killed
         mutant 'T_sin = floor((K-1)/2)' -> check_truncation: killed
         mutant 'T_cos = floor(K/2)' -> check_truncation: killed
         mutant 'T_log = J-2' -> check_truncation: killed
         mutant 'T_log with 2v instead of 2v-1' -> check_truncation: killed
         mutant 'W = max(v, n+D-1)' -> check_working_precision: killed
         mutant 'W = max(v, n) (no D)' -> check_working_precision: killed
         mutant 'W = n+D (drop v)' -> check_working_precision: SURVIVED
         mutant 'cos hull 2N (drop v_p(2))' -> check_series_radii: killed
         mutant 'cos hull 2N-1 everywhere' -> check_series_radii: killed
         mutant 'Log r=1 at 2 gives Log(a)+2Z_2' -> check_log_radii: killed
         mutant 'Log image r+1' -> check_log_radii: killed
         mutant 'root log condition c+v(n) -> 1+v(n)' -> check_root_criteria: killed
         mutant 'root log condition drops v(n)' -> check_root_criteria: killed
         mutant 'root out exponent -n j' -> check_root_precision: killed
         mutant 'root out exponent without v(n)' -> check_root_precision: killed
         mutant 'root guard c+v(n)-1' -> check_root_precision: killed
         mutant 'power R drops A+B' -> check_power_precision: killed
         mutant 'power R drops B+alpha' -> check_power_precision: killed
         mutant 'power R = A+B+1 term' -> check_power_precision: killed
         mutant 'power R uses alpha+1' -> check_power_precision: killed
    [ok] M  author's checks against wrong formulas (info: survivors are weak checks): 21 of 22 killed;
         survivors: ['W = n+D (drop v)']
    total 18.4s; failures: []

The author's own file was also run unchanged: `python3 -B proto/functions_checks.py` passes all 20 checks
in 3.0 s. Its outputs are as printed in the file (for example `check_truncation: scenarios=4224
omitted_terms=11500509`).

## Findings against the specification

None that make SPEC 9.3.1 to 9.3.6 false. Two remarks, neither a defect:
- SPEC 9.3.3: the guard N - m >= c + v(n) is stronger than necessary at p = 2 for odd n (see O-2).
- SPEC 9.3.2: the log truncation that 1F will copy from Prop 7 is correct but about 2x longer than needed
  (see O-1).
