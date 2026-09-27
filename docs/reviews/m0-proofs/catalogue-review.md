# Adversarial review of `docs/proofs/catalogue.md`

Object: `docs/proofs/catalogue.md` (15 numbered statements) and its checks `proto/catalogue_checks.py`, against
`docs/SPEC.md` 9.3.7 (Tier A catalogue) and section 5. Reviewer's own checks:
`docs/reviews/m0-proofs/catalogue_review_checks.py` (independent code; the author's module is imported only to run
mutants against the author's checks). No file under `refs/` exists; external theorems the author marks
`[source pending]` (Hensel's lemma, quadratic reciprocity, Gamma continuation, Haar scaling, the unit-group
exponents, the p-adic logarithm, the reciprocity naming conventions) were not checked against a source here either.
The verdicts below are about the deductions and the formulas, given those theorems.

## Summary

| No. | Statement | Verdict | One line |
|---:|---|---|---|
| 1 | Symbol definitions | VALID | Agrees with `sympy.kronecker_symbol` on all 14641 pairs in [-60,60]^2. |
| 2 | Symbol precision | VALID | K is a period for b = 1..120; larger than least period for 36 b (allowed). |
| 3 | Symbol sets | VALID | 5220 inputs exact; symbol of a profinite argument used, never defined (note). |
| 4 | Hilbert formulas | VALID | Standard formulas; square-invariant and symmetric on 3000 random pairs. |
| 5 | Odd p = solvability | VALID | Steps (2)-(4) checked; 1200 pairs at p = 3..13 confirmed. |
| 6 | 2-adic table | VALID | Re-derived: -1 by no solution mod 16 (and 32), +1 by Hensel witnesses. |
| 7 | Hilbert precision | MINOR | Local unit part of an idele must include the cofactor of `r`; unstated. |
| 8 | Product formula | VALID | Generator cases complete; 3000 random pairs (numerators to 1e5) give 1. |
| 9 | Local zeta factors | MINOR | True; allows an "unbounded enclosure" where SPEC prescribes a status. |
| 10 | Content, volume | VALID | Correct; the author's check is definitional. |
| 11 | Profinite power | VALID | Compatibility argument complete. |
| 12 | Power and `D` | VALID | 52920 brute-force cases incl. negative `c`, `e`, `N = 1, 2, 2 odd`. |
| 13 | Finest modulus `F` | MINOR | Table correct (19133 cases); output centre not given, and it is not `c^e`. |
| 14 | Binomial | VALID | 9000 cases incl. negative `a`, `N = 1`, `k = 0`; proof complete. |
| 15 | Cyclotomic action | MINOR | True (34176 triples); "containment implies divisibility" not proved. |

Counts: VALID 11, MINOR 4, INVALID 0.

## Details per statement

### 1. Definition 1 (residue symbols) - VALID

Tried: coded Definition 1 from the prose only (Legendre by the set of squares, not Euler; `(a/2)` by `a mod 8`
in {1,7}; `b < 0` and `b = 0` rules), compared with `sympy.kronecker_symbol` on a, b in [-60, 60]: 14641 pairs, 0
differences. The boundary cases `(0/-1) = 1`, `(a/0)` for a in {-1, 0, 1, 2}, `(a/-2)` with negative even `a`
are inside that range. The Euler-criterion argument in the proof is correct (the value of `a^((p-1)/2)` is a root
of `X^2 - 1` in a field, hence +-1; the squares exhaust the `(p-1)/2` roots of `X^((p-1)/2) - 1`).

Author's check: the Legendre part is compared with an independent squares set (good). Jacobi and Kronecker are never
compared with anything independent: the loops test only that the author's function is periodic and that two
enumerations of the same function agree. Three wrong formulas pass `check_symbols` unchanged (run in section M of
my checks): `(a/2) = (-1)^eps(a)` instead of `(-1)^om(a)`; Jacobi without prime multiplicities (`(2/9) = -1`);
`(0/-1) = -1`. The first one also has period dividing 8 and gets `(1/2) = 1`, `(3/2) = -1` right, which are the only
two explicit values the check asserts. This check is weak.

### 2. Proposition 2 (symbol precision) - VALID

Tried: for b = 1..120, `K = m` (t = 0) or `lcm(m, 8)` (t > 0) is a period of `a -> (a/b)` over 16K consecutive
`a`, 0 failures. The least period is smaller than K for 36 values of b (for example t even: modulus `lcm(m, 2)`
suffices), which the statement allows ("sufficient, not minimal"). Examples `(1/2) = 1`, `(3/2) = -1` confirmed.
Proof steps are correct; step (2) is the identity `((a+8k)^2 - 1)/8 = (a^2 - 1)/8 + 2ak + 8k^2`.

### 3. Proposition 3 (symbols on finite input sets) - VALID, with a note

Tried: N = 1..24, b in {1,2,3,4,5,6,8,9,12,15,16,21,24,40,45}, every unit residue c mod N (so N = 2 odd and N = 1
are included) and additive balls with a in [-3, 3]. The claimed set (residues modulo `lcm(N,K)`) was compared with
the set over integers `r = c mod N` coprime to `2 N b` (which represent profinite units of the coset) and over
`a + N j` for 12 L/N and 12 L values respectively: 5220 inputs, 0 differences. Symbol 0 on a unit coset: 0 times.
`K | N` implies a single value: 388 cases, 0 failures.

Note (not a defect of the deduction): the statement speaks of "the symbol" of a profinite unit or integer, but
Definition 1 defines it only for integer `a`. The intended definition, which Proposition 2 makes well-defined, is
"`(x/b)` for `x` in `Zhat` is `(r/b)` for any integer `r = x mod K`". Adding that sentence costs nothing (see
Repairs).

Author's check: `vals == sampled` and `finite == samples` enumerate the same function over two ranges of
residues that both cover every class; this is a tautology and would pass any formula. Only 4 coset inputs.

### 4. Definition 4 (Hilbert symbol and formulas) - VALID

The odd-prime and 2-adic formulas are the usual ones (Serre, A Course in Arithmetic, III.1.2, Thm. 1 **[from
memory, source pending]**). Tried: my own implementation of the formulas on 3000 random pairs of rationals (sign,
numerator to 3000, denominator to 300), at places inf, 2, 3, 5, 7, 11: invariance under multiplying each argument
by one of the squares 1, 4, 9, 25, 49, 1/4, 9/100, 289, 81/16, and symmetry, 0 failures. `17` (1 mod 8) is in the
square class of 1 at 2 for all 64 pairs. The real-place argument is correct.

### 5. Proposition 5 (odd-prime formula equals solvability) - VALID

Read step by step. (2): two subsets of size `(p+1)/2` of `F_p` intersect; with `y = 1` not both of `x, z` vanish
because `w` is a unit; simple Hensel in that coordinate. (3): the `(w/p) = -1` case forces `p | y, z`, then
`p^2 | p u x^2`, `p | x`: correct, and uses only congruences modulo `p^2`. (4): primitivity plus `p | z` excludes
`p | x, y`; `u x^2 + w y^2 = 0 mod p` with both nonzero iff `(-uw/p) = 1`; the lifted solution with `z = 0` works.
(5) the case table matches `(-1)^(alpha beta (p-1)/2)(u/p)^beta(w/p)^alpha`. The claim "primitive solution modulo
`p^2` is equivalent to local solvability for valuations 0/1" follows because each -1 case was refuted modulo `p^2`.

Tried, independently of the author's modular test: for p in {3, 5, 7, 11, 13}, every pair of unit residues
(u, w) and alpha, beta in {0, 1} (1200 pairs): where the formula gives -1, no primitive solution modulo `p^2`
(this alone proves -1, with no theorem needed); where it gives +1, an explicit integer point with one unit
coordinate satisfying `v_p(F) > 2 v_p(dF)` (strong Hensel). 0 failures.

Author's check: genuine (independent enumeration), but only 16 pairs per prime and only p = 3, 5. The sign factor
`(-1)^(alpha beta (p-1)/2)` is exercised only at p = 3 (at p = 5 it is 1).

### 6. Proposition 6 (2-adic formula, 64 entries) - VALID

The table in the file was parsed and compared with (i) my own formula, (ii) a decision independent of the author's
"primitive solution mod 16 iff solvable" argument: -1 entries by non-existence of a primitive solution modulo 16
(which proves -1 without Hensel), +1 entries by an explicit point with `v_2(F) > 2 v_2(dF)` in a unit coordinate.
64 of 64 agree. The 28 minus entries also have no primitive solution modulo 32. The remark "(2,6)_2 = -1 but
(1,1,0) solves modulo 8" is confirmed, and the pair has no primitive solution modulo 16.

Proof step (2) read: the case split (odd coordinate with odd coefficient; both coefficients of valuation 1 forcing
`x, y` odd and `z` even; the remaining case contradicting the equation mod 4) is exhaustive and each Hensel
inequality holds (`v >= 4 > 2`, resp. `v >= 3 > 2`). One unstated step: in the valuation-1 case "a primitive
solution has odd `x, y`" needs the equation modulo 16 divided by 2 (`u x^2 + w y^2 = 2 z'^2 mod 8`), which rules out
exactly one of `x, y` odd. It is immediate; no repair needed.

Author's check: genuine for the 64 entries (the mutant with `alpha om(u) + beta om(w)` swapped is killed).

### 7. Proposition 7 (Hilbert precision, exceptional places) - MINOR

What holds: local balls at the stated bounds give a constant symbol (2000 random pairs of balls with rational
centres at p in {2,3,5,7}, 10 points each, 0 failures). The bound 3 at p = 2 cannot be lowered to 2 in general:
`1 + 4 Z_2` against `2` gives both signs (`(1,2)_2 = 1`, `(5,2)_2 = -1`). At odd p the hypothesis
`A - v_p(a) >= 1` is the same as "the ball excludes zero", so it is redundant there, not wrong. The candidate
places (inf, 2, odd primes of odd valuation in `r` or `s`) are correct: at other odd p all exponents are even.
The counterexample in step (5) to the product formula for ideles is correct.

Gap (for a literal C implementation): the statement says "evaluate all permitted square classes" of the idele
components but never says what they are. The component at p of an idele `(x_inf, r, c U(N))` is `r u_p`; its
unit part is `r' u_p` with `r' = r p^(-v_p(r))`, and `u_p` is known modulo `p^(v_p(N))`. An implementation that
feeds the unit coset residue `c mod p^n` alone as the unit part is wrong: for `r = s = 3` and unit exactly 1,
the true symbol at 2 is `(3,3)_2 = -1`, while `(1,1)_2 = +1` (checked). Nothing in the text excludes that
reading. Repair: see Repairs, R7.

Author's check: the precision probe `hilbert_formula(centre + p^A j, centre, p)` with centre 3 at p = 2 is already
constant at `A = 2` (the second argument has valuation 0, so only `eps` matters, which is fixed mod 4); it cannot
tell bound 2 from bound 3. Three probes in total. Nothing tests the idele part (candidate places, scale cofactor,
set of signs). Weak.

### 8. Proposition 8 (rational product formula) - VALID

Read: bimultiplicativity holds for each displayed formula (the sign factor is bilinear in `(alpha, beta)`,
`eps` and `om` are additive on odd units modulo 2); the generator list `-1, 2, odd primes` with symmetry covers all
pairs; each case uses exactly reciprocity or a supplement. Tried: 3000 random pairs with numerators up to 1e5 and
denominators up to 1e4 (so primes well beyond 7), product over inf, 2 and all prime factors: 1 in every case; a
prime outside the support gave +1 in every case.

Author's check: genuine but small (numerators up to 9, denominators up to 5, primes up to 7).

### 9. Proposition 9 (local zeta factors) - MINOR

What holds: `2 int_0^inf exp(-pi x^2) x^(s-1) dx = pi^(-s/2) Gamma(s/2)` at s = 0.7, 2, 3.5+2i, 1.5-3i, max
relative error 6.8e-30 (mpmath, 30 digits). `1 - p^(-s)` vanishes at `s = 2 pi i k / log p` (p in {2,3,5,7},
k in {-2,-1,1,2,3}), with derivative `log p`, so the poles are simple; the real factor is finite at -1, -3, -5 and
of size 2e12 at distance 1e-12 from 0, -2, -4.

Deviation from the specification: SPEC 9.3.7 says "a ball containing a pole returns a status, never a finite ball".
The statement says "returns a pole/domain status or an explicitly unbounded enclosure". That widens the contract
(an `acb` with infinite radius is an allowed return in the proof file, not in the SPEC). Either the statement is
narrowed to the SPEC or the SPEC is changed; the proof file cannot decide it. Repair: R9.

Author's check: nothing tests the pole set, the simplicity of the poles, or the factor `pi^(-s/2)`. The series
tail identity is an algebraic identity in exact rationals; the Gamma test checks `Gamma(1), Gamma(2), Gamma(3)`
only and never evaluates the stated real factor or the Gaussian integral. A wrong pole formula
(`pi i k / log p`, `2 pi i k / p`) or a wrong factor (`pi^(-s) Gamma(s/2)`) passes. Blind for the statement's main
content.

### 10. Proposition 10 (content and Haar volume) - VALID

Correct. `vol(a + (n/d) Zhat) = d/n` re-derived by scaling to `d a + n Zhat` and counting (n < 30, d < 12);
`prod_p |r|_p = 1/r` for 500 random rationals. Author's check: both loops evaluate the definition against itself
(`hits/8N = 1/N`; rebuilding `r` from its factorisation). It could not fail for any formula; it is harmless
because the statement is elementary.

### 11. Definition 11 (continuous profinite power) - VALID

Compatibility (`ord_{L'}(u) | ord_L(u)` for `L' | L`) and continuity in `x` are argued correctly.

### 12. Proposition 12 (target precision, `D`) - VALID

Tried by brute force (all t over two full periods of the exponent group modulo N): N = 1..40, c in [-N, N) coprime
to N (negative representatives, N = 1, N = 2, N = 2 odd), M = 1..6, e in [-4, 4]: determination modulo N iff
`c^M = 1 mod N`; for every divisor L of N determination iff `L | D`; every output lies in `c^e U(D)` (with a modular
inverse for e < 0). 52920 cases, 0 failures. `D` is independent of the representative `c` (`(c+N)^M = c^M mod N`).
For `N = 2m`, `D` may be even (N = 6, c = 5, M = 1 gives D = 2); `U(2) = U(1)`, so this is harmless.

### 13. Proposition 13 (canonical and finest modulus) - MINOR

What holds: the prime-exponent table agrees with an independent local brute force in 19007 cases (canonical
N <= 30, every unit c mod N, e in [-5, 6], M in 0..6 where M = 0 is the exact-exponent row) and 126 random cases
(canonical N <= 400 with primes <= 37, c <= 1e4, M <= 16, |e| <= 30); 0 mismatches. My brute force does not use
the table to choose its precision: at each prime it raises the level `p^K` until the computed depth is below K.
The example `N = 5, c = 2, e = 0, M = 2`: `D = 1`, `F = 24`, confirmed. Applying the table to a non-canonical
`N = 2 m` gives a coarser, never a finer modulus (1176 equal, 392 coarser, 0 finer), so misuse is safe.

Gaps:

1. The statement gives the modulus `F` but not the coset. The output lies in `r U(F)` with `r = c^e` modulo
   `p^(v_p(F))` for `p | N` but `r = 1` modulo `p^(v_p(F))` for `p` not dividing `N` (the base 1 is allowed there).
   `c^e` is not the centre in general, even modulo `canon(F)`: 3033 of 19007 cases. Example `N = 5, c = 2, e = 2,
   M = 4`: `F = 120`, true centre 49, while `c^e = 4` is not a unit modulo 120. A C implementation that returns
   `c^e U(F)` returns a wrong enclosure. The CRT rule above was checked in all 19007 cases, 0 failures.
2. `F` is often twice an odd number (11808 of 19007 cases; for example 2 not dividing N with g odd gives
   `v_2(F) = 1`, and `N = 4, c = 3, M = 1` gives `F = 2`). The statement insists on canonical form for inputs but
   not for its output. It should return `canon(F)`.
3. Proof step (2) proves necessity of the two conditions (`c^M = 1`, `w^g = 1` modulo `p^k`) and asserts that
   "combining the two bounds gives the first row"; the converse (both conditions imply constancy) is one line and
   missing: `(c w)^(e+Mt) = c^e (c^M)^t w^e (w^M)^t`.
4. Step (6) calls the finite algorithm "independent", but its level `B = 8 N g (g+1)!` is justified "by the table".
   If the table underestimated an exponent below `v_p(B)`, the algorithm would still disagree, so the cross-check
   is meaningful, but it is not independent. The author's check also restricts to e >= 0, M <= 3, N <= 18, and
   does not test the exact-exponent row or the output centre.

Repair: R13.

### 14. Proposition 14 (binomial) - VALID

Tried: N = 1..40, a in [-12, 12], k = 0..8 (9000 cases): the gcd over `j = 1..k` equals the gcd over all integer
`t` in [-60, 60]; the conservative radius `N / gcd(N, k!)` divides it; `R = 0` iff `k = 0` (a degree-k polynomial
in t with nonzero leading coefficient `N^k/k!` cannot take one value at `k + 1` points). Example (0, 8, 4): R = 2,
conservative radius 1. The Newton-basis argument (unimodular triangular change between `Delta^j h(0)` and
`h(j) - h(0)`) is complete, and minimality over rational radii holds because any enclosing group `R' Zhat` contains
the integer combination `R` of the differences. Author's check: genuine.

### 15. Proposition 15 (cyclotomic action) - MINOR

What holds: for N, n < 49 and every unit c mod N (34176 triples), the image of `c U(N)` in `(Z/n)^x` is a
singleton iff `canon(n) | canon(N)`. The test idele (p at p, 1 elsewhere including the real place) has class unit
`CRT(1 mod p^j, p^-1 mod m)` modulo `n = p^j m`; its arithmetic action is `z -> z^p` for n prime to p and trivial
on p-power roots, for p in {2,3,5,7} and n < 60. The two conventions agree with SPEC 9.3.7.

Gap: step (3) states "containment is equivalent to divisibility after canonicalising both moduli". Divisibility
implies containment is clear; containment implies divisibility is not argued, and it is exactly where the factor
2 matters (for non-canonical moduli it is false: `U(3)` is inside `U(6)` but 6 does not divide 3). The statement
index claims no implication is left open. Repair: R15 gives the missing argument.

Author's check: the containment loop is a genuine enumeration (only c = 1, which suffices by translation). The
"uniformiser" part asserts `pow(pow(p, -1, n), -1, n) == p`, a tautology; it never constructs the idele class.

## Repairs

**R3 (optional, Proposition 3).** Append to the statement: "For `x` in `Zhat` and `b > 0`, `(x/b)` means `(r/b)`
for any integer `r = x mod K`; this is well defined by Proposition 2. For a unit coset, `x` ranges over
`c U(N)`."

**R7 (Proposition 7).** Insert after "...can have a nontrivial symbol.":

    The component at a prime p of an idele with scale r and unit coset c U(N) is r u_p, where u_p is a unit
    with u_p = c mod p^n, n = v_p(canon(N)), and is otherwise arbitrary. Its valuation is v_p(r). Its unit part
    is r' u_p with r' = r p^(-v_p(r)); the cofactor r' is part of the square class. At odd p the unit residue
    modulo p is r' c mod p if n >= 1, and ranges over all of (Z/p)^x if n = 0. At p = 2 the unit residue
    modulo 8 ranges over the odd residues congruent to r' c modulo 2^min(n,3). The permitted square classes are
    these (valuation parity, unit residue) pairs. Example: r = s = 3 with unit exactly 1 gives (3,3)_2 = -1;
    the unit parts without r' would give (1,1)_2 = +1.

and replace the precision hypothesis by: "If `A - v_p(a) >= 1` (for odd p this is the same as the ball excluding
zero) ... or both are at least 3 for p = 2 (2 is not enough: `1 + 4 Z_2` against 2 gives both signs)".

**R9 (Proposition 9).** Replace the last sentence of the statement by: "A complex input ball containing any pole
returns a pole status and never a finite complex ball (`docs/SPEC.md` 9.3.7)." If an unbounded enclosure is wanted
as an alternative return, that is a change to SPEC 9.3.7 and goes to "Findings against the specification".

**R13 (Proposition 13).** Add to the statement after the table:

    All outputs lie in the coset r U(F), where r is the solution, by the Chinese remainder theorem, of
    r = c^e mod p^(v_p(F)) for p | N and r = 1 mod p^(v_p(F)) for p not dividing N (c^e by a modular inverse
    when e < 0). In general r is not c^e: for N = 5, c = 2, e = 2, M = 4, F = 120 and r = 49. The returned
    modulus is canon(F): v_2(F) = 1 occurs, for example when 2 does not divide N and g is odd.

Add to proof step (2), after "equivalently every member raised to g is 1": "Conversely, if `c^M = 1` and `w^g = 1`
modulo `p^k` for all `w` in `U(p^n)`, then `(c w)^(e+Mt) = c^e (c^M)^t w^e (w^M)^t = c^e` modulo `p^k` for all
`t`." Add to proof step (3): "At `p` not dividing `N` every output is 1 modulo `p^(v_p(F))`, since the base 1 is
allowed." In step (6) replace "independent" by "finite cross-check (its level B is bounded using the table)".

**R15 (Proposition 15).** Add to proof step (3):

    Conversely let a = canon(N), b = canon(n), and suppose b does not divide a. Take a prime q with
    k = v_q(b) > j = v_q(a). Let u be the unit of Zhat equal to 1 at every prime other than q and to
    u_q = 1 + q^j at q if j >= 1 or q odd, and u_q = 3 if q = 2 and j = 0. Then u is in U(a). Since b is
    canonical, q = 2 implies k >= 2, and u_q - 1 has valuation j < k (or u_q = 3 is not 1 mod 4); so u is not
    in U(b), and U(a) is not inside U(b). Without canonical form the claim fails: U(3) is inside U(6).

## Checks

Command (46 s on one core):

    python3 docs/reviews/m0-proofs/catalogue_review_checks.py

Output:

    == sec_symbols
    [ok] Def 1 vs sympy.kronecker_symbol, a,b in [-60,60]: 14641 pairs, 0 differ
    [ok] Prop 2: K is a period of a -> (a/b), b in 1..120: 120 values of b, 0 failures; K larger than least
         period for 36 b (allowed: 'sufficient, not minimal')
    [ok] Prop 2 examples (1/2)=1, (3/2)=-1
    [ok] Prop 3: claimed finite sets = sampled truth (cosets and balls): 5220 inputs, 0 differ; zero on a unit
         coset 0 times; 'K | N => determined' 388 cases, 0 failures
    == sec_hilbert
    [ok] Prop 6: table parsed from catalogue.md: 8 rows
    [ok] Prop 6: 64 pairs; -1 by no primitive sol. mod 16, +1 by Hensel witness; = formula = table: 0 mismatches
    [ok] Prop 6: the 28 minus entries also have no primitive solution mod 32: 0 exceptions
    [ok] Prop 6 remark: (2,6)_2 = -1 but (1,1,0) solves mod 8
    [ok] Prop 5: odd p in {3,5,7,11,13}, all unit residues, valuations 0/1: 1200 pairs, 0 failures (-1 by
         enumeration mod p^2, +1 by Hensel witness)
    [ok] Def 4: invariant under squares and symmetric, 3000 random pairs, 6 places: 0 failures
    [ok] Def 4 at 2: 17 is a square class 1 (residue 1 mod 8)
    [ok] Prop 8: product formula, 3000 random rational pairs (num <= 1e5, den <= 1e4): 0 failures
    [ok] Prop 7: at p=2, relative precision 2 is not enough in general: ball 1 + 4 Z_2 against 2 gives both signs
    [ok] Prop 7: 2000 random ball pairs at the stated precision bound, 10 points each: 0 failures
    [ok] Prop 7 check weakness: author's probe (3+2^A j, 3) is constant already at A=2: so the probe cannot
         tell bound 2 from bound 3
    [ok] Prop 7 idele pitfall: scales r=s=3, unit 1: true (3,3)_2 vs unit-only evaluation: true -1, unit part
         without the cofactor of r gives 1
    [ok] Prop 7: the idele (3,3) is the rational 3, product over places: signs at inf,2,3 = [1, -1, -1]
    == sec_zeta
    [ok] Prop 9: 2 int exp(-pi x^2) x^(s-1) dx = pi^(-s/2) Gamma(s/2), 4 values of s: max rel. error 6.82e-30
    [ok] Prop 9: 1 - p^-s vanishes at s = 2 pi i k/log p with derivative log p: max |1-p^-s| there 1.59e-30
    [ok] Prop 9: real factor finite at -1,-3,-5 and ~1e12 near 0,-2,-4: min |value| at 1e-12 from pole: 2.0e+12
    == sec_volume
    [ok] Prop 10: vol(a + (n/d) Zhat) = d/n by counting (n<30, d<12): 0 failures
    [ok] Prop 10: prod_p |r|_p = 1/r, 500 random r
    == sec_power_target
    [ok] Prop 12: determination mod N, largest divisor D, output in c^e U(D): 52920 cases (N<=40, c in [-N,N),
         M<=6, e in [-4,4]), 0 failures
    == sec_power_finest
    [ok] Prop 13: table F = independent local brute force (canonical N<=30, e in [-5,6], M in 0..6): 19007
         cases, 0 mismatches, 0 skipped (K limit)
    [ok] Prop 13: table output F is twice an odd number (non-canonical): 11808 of 19007 cases (e.g. 2 not
         dividing N with g odd, or N=4, c=3, M=1)
    [ok] Prop 13 repair: centre = CRT(c^e at p|N, 1 at p not dividing N) is the true centre: 0 failures
    [ok] Prop 13 gap: 'c^e mod canon(F)' is NOT the centre of the output coset: 3033 of 19007 cases; first
         (N,c,e,M,F,true centre) = (3, 2, -4, 0, 240, 1)
    [ok] Prop 13 example N=5,c=2,e=0,M=2: D=1, F=24: F=24
    [ok] Prop 13 example N=5,c=2,e=2,M=4: F=120, true centre 49, c^e = 4 is not even a unit mod 120
    [ok] Prop 13: table misapplied to N = 2*odd is never finer than the truth: equal 1176, coarser 392, finer
         (unsafe) 0
    [ok] Prop 13: random canonical N<=400 (primes<=37), c<=1e4, M<=16, |e|<=30: 126 cases, 0 mismatches
    == sec_binomial
    [ok] Prop 14: R = gcd over j=1..k equals gcd over t in [-60,60]: 9000 cases (N<=40, a in [-12,12], k<=8),
         0 failures
    [ok] Prop 14: conservative radius N/gcd(N,k!) divides R; R=0 iff k=0: 0, 0 failures
    [ok] Prop 14 example (0,8,4): R=2, conservative radius 1
    == sec_cyclo
    [ok] Prop 15: action of c U(N) on n-th roots determined iff canon(n) | canon(N): 34176 (N, n, c) triples,
         N,n < 49, 0 failures
    [ok] Prop 15: test idele acts by z -> z^p (n prime to p), trivially on p-power roots: 0 failures
    == sec_mutants
       mutant '(a/2) = (-1)^eps(a) instead of (-1)^om(a)' vs check_symbols: SURVIVES (check blind)
       mutant 'Jacobi ignores prime multiplicity' vs check_symbols: SURVIVES (check blind)
       mutant '(0/-1) = -1 instead of 1' vs check_symbols: SURVIVES (check blind)
       mutant '2-adic: alpha om(u) + beta om(w) (swapped)' vs check_hilbert: killed
       mutant 'finest modulus without v_p(g) at p|N' vs check_power: killed
    [ok] own check kills (a/2) mutant: (3/2) vs (5/2)
    [ok] own check kills (0/-1) mutant
    [ok] own check kills Jacobi-multiplicity mutant: (2/9) = 1, (2/3) = -1

    0 failed: []

(Long output lines are wrapped here at 116 characters; the script prints them on one line.)

Author's checks, re-run unchanged: `python3 proto/catalogue_checks.py` passes (symbols 2279, Hilbert 96 class pairs,
product 690 pairs, zeta 30, power 2050 + 738, binomial 980, content/volume 827, cyclotomic 1604 cases).

Weak author checks, summarised: `check_symbols` (Jacobi/Kronecker only self-consistent; 3 of 3 mutants survive),
`check_zeta` (pole set, simplicity and `pi^(-s/2)` untested), `check_content_volume` (definitional),
`check_hilbert` precision probe (cannot distinguish bound 2 from 3; idele part untested), `check_cyclotomic` test
vector (tautology), `check_power` finest modulus (level chosen from the table; e >= 0, M <= 3 only; centre and
exact-exponent row untested). Genuine: the 64-entry and odd-prime Hilbert enumerations, the target-modulus power
enumeration, the binomial comparison, the cyclotomic containment enumeration.
