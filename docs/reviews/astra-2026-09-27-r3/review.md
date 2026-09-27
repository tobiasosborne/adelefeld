# Third review of adelefeld — 2026-09-27

Evidence labels apply to the paragraph or table row they introduce: **[proved here]** for deductions, counterexamples and justified recommendations; **[checked by a run]** for inspected text/source or recorded executions; **[from memory]** for external facts not established here. A source inspected through a tool is labelled **[checked by a run]**, not claimed as a new proof. Ratification concerns the basis for milestone 0, not completion of its proof gates.

## 1. Closure table

**[checked by a run]** The brief lists twelve round-1 remainders, despite calling them eleven. All twelve and N1–N12 are covered below. RESOLVED means the specified repair is present; it does not certify future implementation.

| ID | Status | Evidence / reason |
|---|---|---|
| M6 | RESOLVED | **[checked by a run]** SPEC 6 now specifies multiple wraps, closed pieces with gluing, authoritative lifts and a piece limit. |
| M9 | RESOLVED | **[checked by a run]** SPEC 7 restricts dilation to nonzero rational scalars or certified ideles. |
| M10 | RESOLVED | **[checked by a run]** SPEC 8 exempts conductor 1 and specifies absolute convergence. |
| M12 | RESOLVED | **[checked by a run]** SPEC 9.2 has the zero-radius branch, reduced fractions and invertible denominators. |
| D2 | RESOLVED | **[checked by a run]** SPEC 4.1 / PLAN 4 specify whole-value fallback, unused local `A`, and new contexts after cancellation. |
| P1 | RESOLVED | **[checked by a run]** PLAN 1 / 0.1 permit only a provisional scaffold before the contract reviews. |
| P2 | RESOLVED | **[checked by a run]** PLAN 1F.7 / 7 require independent oracles and output precision, and identify wrapper tests. |
| F1 | RESOLVED | **[checked by a run]** PERF 1 labels allocator estimates MODEL and records the limited counter probe. |
| F2 | RESOLVED | **[checked by a run]** PERF 2 withdraws the mismatched 80/37 ratio and labels the entropy denominator. |
| F3 | RESOLVED | **[checked by a run]** PERF specifies compulsory I/O and implementation-restricted dependency bounds. |
| F4 | RESOLVED | **[checked by a run]** Current baseline source and commit `4b143d9` both hash to `690ca1b22ab95fa5895c9dd264a47ebd46b750233eac2adc34ea79df536908b1`. |
| F6 | RESOLVED | **[checked by a run]** PERF 4 now qualifies worst cases, layouts, padding and information counts. |
| N1 | RESOLVED | **[checked by a run]** SPEC 9.3.2 separates series domains, all-place existence and finite-data certification. |
| N2 | RESOLVED | **[checked by a run]** SPEC 9.3.1 / 9.3.5 require named projections and separate the additive character. |
| N3 | RESOLVED | **[proved here]** The radius, extended-Log and 2-adic exception formulas are correctly transcribed; see audit below. |
| N4 | RESOLVED | **[checked by a run]** SPEC 9.3.2 / PLAN 1F.4 make ball uncertainty and domain handling the wrapper's responsibility. |
| N5 | PARTLY | **[proved here]** Local existence, counts and guarded precision are correct; the all-place exact-rational result still needs the restriction in R4. |
| N6 | RESOLVED | **[checked by a run]** SPEC 9.3.4 separates integer, rational, principal-unit and quasi-character powers. |
| N7 | RESOLVED | **[checked by a run]** PLAN 1F.7 uses 5/13 for trigonometric exp comparisons and independent truncations at 2/3. |
| N8 | RESOLVED | **[proved here; checked by a run]** Zero cases, explicit CRT moduli and scaled conversion are valid; zero-predicate regressions pass. |
| N9 | RESOLVED | **[checked by a run]** SPEC 4.1 / PLAN 4 explicitly describe the archimedean enlargement and partial-place tag. |
| N10 | RESOLVED | **[checked by a run]** SPEC 9.3.3 / 9.3.6 separate real root parity, scalar outputs, fractional parts and complex branches. |
| N11 | RESOLVED | **[checked by a run]** PERF 4 uses materialised output contracts instead of equating requested precision with output size. |
| N12 | RESOLVED | **[proved here]** SPEC 5 / PLAN 5,7 remove exactly one redundant factor 2 when its valuation is 1. |

## 2. New findings table

**[proved here]** Severities reflect the contracts demonstrated below. MAJOR means a wrong or unfulfillable public mathematical contract; MINOR means a bounded clarification with a concrete repair.

| ID | Location | Severity | Finding |
|---|---|---|---|
| R1 | SPEC 9.3.7, symbols; PLAN 1F.9 | MAJOR | **[proved here]** Kronecker symbols are not determined by the numerator modulo the denominator. |
| R2 | SPEC 9.3.7, Hilbert; prototype oracle | MINOR | **[proved here]** Specify valuation parity plus relative unit precision, finite ambiguous places, and why the finite solvability test lifts. |
| R3 | SPEC 9.3.7, profinite power; PLAN 1F.9 | MINOR | **[proved here]** The criterion is correct modulo the supplied N; state that target and the coarsening contract explicitly. |
| R4 | SPEC 9.3.3; PLAN 1F.8 | MAJOR | **[proved here]** All-place roots of an exact rational cannot generally be returned as a finite list of branches. |
| R5 | SPEC 9.3.7, analytic rows and introductory cost claim | MINOR | **[proved here]** Fix local-constant normalisations, pole statuses and cyclotomic convention; remove the blanket claim of cheapness. |
| R6 | PLAN 1 versus milestone 0.5 | MINOR | **[proved here]** The ban on C beyond the scaffold needs an exception for milestone-0 benchmark/proof probes. |

## 3. Findings in full

### R1 — separate Kronecker precision from Jacobi precision

**Quote [checked by a run].** “Legendre, Jacobi, Kronecker symbol”; “residue modulo the lower entry”.

**Failure and evidence [proved here; checked by a run].** `(1/2)=+1`, `(3/2)=-1`, although `1=3 mod 2`; the installed FLINT routine confirms this in [checks.txt](checks/checks.txt). At denominator `-1`, the symbol records the sign of the numerator, which a profinite unit coset does not contain. At denominator 0 it is 1 precisely for numerator ±1, so it is not a finite-residue operation either. This is a wrong precision guarantee, not merely a loose enclosure.

**Replacement [proved here].** “Legendre takes an odd prime denominator; Jacobi takes a positive odd denominator, and both depend on the numerator modulo that denominator. For positive Kronecker denominator `b=2^t m`, `m` odd, use the Jacobi factor modulo `m` and, if `t>0`, the factor `(a/2)^t`, where `(a/2)=0` for even `a` and `(-1)^((a^2-1)/8)` for odd `a`. Thus `lcm(m,8)` is a sufficient modulus when `t>0`; `m` suffices when `t=0`. Negative or zero denominators are supported for exact integer inputs only, with the sign/±1 cases explicit. On insufficient coset data return the possible symbols or `NOT_DETERMINED`.” Add `(1/2)` versus `(3/2)` to 1F.9. The modulus above is deliberately sufficient, not minimal when `t` is even.

### R2 — the Hilbert formulas and oracle are sound; complete the finite-data contract

**Quote [checked by a run].** “`p = 2`: modulo 8”; “for ideles only finitely many places need data”; the prototype describes its comparison as “against the definition”.

**Formula audit [proved here].** Write `a=p^alpha u`, `b=p^beta w`, with unit parts `u,w`. The implemented formulas are

```
odd p: (-1)^(alpha*beta*(p-1)/2) (u/p)^beta (w/p)^alpha
p=2:  (-1)^(eps(u)*eps(w) + alpha*om(w) + beta*om(u))
eps(u)=(u-1)/2 mod 2; om(u)=(u^2-1)/8 mod 2.
```

**[proved here]** Negative valuations cause no difficulty: only parity enters. At odd `p`, remove squares to make `alpha,beta` either 0 or 1. When both are 0, the sets `{u x^2+w}` and `{z^2}` in `F_p`, each of size `(p+1)/2`, intersect; this gives a solution with `y=1` and a unit derivative. When only `alpha=1`, solvability is equivalent to `w` being a square modulo `p`: otherwise `y,z` are divisible by `p` and primitivity makes the equation impossible modulo `p^2`. When both are 1, solvability is equivalent to `-uw` being a square modulo `p`, by dividing the equation by `p` with `z` divisible by `p`. These are exactly the formula's four cases. At the real place the form is anisotropic precisely when both entries are negative.

**Oracle audit [proved here].** The code restricts the integer coefficients to valuations 0 or 1. A local nonzero solution can be rescaled to a primitive integral one, so reduction is necessary. Conversely, for odd `p`, a primitive solution modulo `p^2` either has a unit coordinate with unit coefficient (simple Hensel lifting), or both `a,b` have valuation 1, `x,y` are units and `z` is divisible by `p`. In the latter case solve `u x^2+w y^2=0` by simple lifting and set `z=0`. At 2, a primitive solution modulo 16 with an odd coordinate of odd coefficient has derivative valuation 1 and residual valuation at least 4. Otherwise both coefficients have valuation 1, `x,y` are odd and `z=2z'`; divide by 2 to get `u x^2+w y^2-2z'^2=0 mod 8`, again with derivative valuation 1. Strong Hensel applies because `v(F)>2v(F')`: Newton correction strictly increases this valuation gap and converges, preserving the unit coordinate. Thus the prototype's precisions `(16,9,25)` really test the definition on its stated inputs. This would not be valid for arbitrary `k` or unnormalised coefficient valuations.

**[proved here; checked by a run]** Each 2-adic square class has a representative in `{1,3,5,7,2,6,10,14}`: odd squares are 1 modulo 8, and every unit 1 modulo 8 is a square by the same Hensel criterion. The independent check covers all 64 pairs and agrees with the formula. Modulo 8 alone is insufficient for the code's unreduced two-even-coefficient form: `(2,6)_2=-1` but `(1,1,0)` is a primitive modular solution. See [checks.py](checks/checks.py).

**Product formula [checked by a run].** For nonzero *diagonal rationals*, the product is 1, including infinity and 2. The prototype's 2,000 cases pass. A source proving the general statement is [Sutherland, Lecture 10, Theorem 10.11](https://math.mit.edu/classes/18.782/2023sp/LectureNotes10.pdf): bilinearity reduces to pairs of `-1` and primes, then the supplementary laws and quadratic reciprocity finish the proof. This is source verification, not a proof by random tests.

**Precision and replacement [proved here].** “For local nonzero balls, certify valuation parity and unit square class: unit parts modulo `p` at odd `p`, modulo 8 at 2, and signs at infinity. In additive-ball notation `a+p^A Z_p`, sufficient relative precision is `A-v(a)>=1` at odd `p` and `>=3` at 2; these are worst-case sufficient requirements, not always necessary. For two ideles with scales `r,s`, every odd place outside the supports of `r,s` is identically +1. Compute the finite exceptional set `{infinity,2} union supp(r) union supp(s)`; at those places report a sign only if constant on all input square classes, otherwise `{±1}` or `NOT_DETERMINED`. The all-place family is determined exactly when every exceptional entry is determined.” Extracting the support may require factoring the scales, or supplied certified support.

**[proved here]** The smaller necessary candidate set at odd primes uses only *odd* valuations of either scale. If both valuations are even, the symbol is already +1. If only alpha is odd, only the second unit's Legendre symbol is needed; interchange the roles for beta odd. At 2, the displayed parity formula can likewise require less than all three relative digits in special cases. The real signs are already certified by the idele type.

**[proved here]** Finite support does **not** mean finite precision always determines the family: two positive ideles of scale 1 and unrestricted unit cosets allow `(1,1)_2=+1` and `(3,3)_2=-1`, with every odd symbol +1. Nor is their product forced to 1: choose both 2-components equal to 3, all other finite components 1 and positive real components. The rational product formula does not extend to arbitrary pairs of ideles. The catalogue's definition correctly says a nonzero *triple*, not that all three coordinates must be nonzero.

### R3 — profinite powers need a target modulus and a fallback

**Quote [checked by a run].** “determined exactly when `c^M = 1 mod N`”; “always for an exact integer exponent”.

**Well-definedness and criterion [proved here].** For a fixed profinite unit `a`, its image in each finite group `(Z/L)^x` has finite order `h_L`. Define `a^x mod L` using `x mod h_L`. These values are compatible under reduction, so define a unique profinite unit, continuously in both arguments. Negative integer exponents are included through modular inverses. For the independent enclosures `a in c U(N)`, `x in e+M Zhat`, the image modulo **N** is `c^e` times the subgroup generated by `c^M`. Hence the criterion is necessary (compare exponents `e,e+M`) and sufficient (integer exponents are dense). It does not assert that the whole profinite output is exact.

**Replacement [proved here].** “For `N,M>=1`, the output is determined **modulo N** iff `powmod(c,M,N)=1`. On failure a strict target-N interface returns `NOT_DETERMINED`; an enclosure interface returns `c^e U(D)`, where `D=gcd(N,powmod(c,M,N)-1)`, and reports the loss. This is the largest determined divisor of N, computed without factorisation. For an exact signed integer exponent, compute modulo N directly, using inversion when negative; exponent 0 returns exact 1. Canonicalise the output unit coset as in section 5.” Add coarsening, negative exponents and canonical-modulus cases to 1F.9, not just successful criteria.

**Maximality and canonical form [proved here].** For every divisor `L` of `N`, constancy is equivalent to `L | c^M-1`; their largest is D. If odd N is replaced by 2N with an odd unit lift of c, D is replaced by 2D, which is the same unit coset. Thus the rule respects `U(2N)=U(N)`; it must not identify the unit coset with the additive ball. The run verifies 9,288 cases and negative exponent choices.

**Unrestricted maximal modulus [proved here].** D is not necessarily the largest modulus among *all* integers: every profinite unit square is 1 modulo 24. For example `N=5,c=2,e=0,M=2` fails the target-5 criterion and gives D=1, but the output is still 1 modulo 24. For finite M a greatest unrestricted modulus exists. To specify it precisely, take canonical N, `g=gcd(e,M)>0`, and `v_p(0)=infinity`; its exponent at each prime is:

| Prime | Largest determined exponent |
|---|---|
| p dividing N | **[proved here]** `min(v_p(N)+v_p(g), v_p(c^M-1))` (canonical N ensures `v_2(N)>=2` if even). |
| Odd p not dividing N | **[proved here]** `1+v_p(g)` if p-1 divides g, otherwise 0. |
| `p=2` not dividing N | **[proved here]** 1 if g is odd; `2+v_2(g)` if g is even. |

**[proved here]** For `p|N`, variation in the base forces the principal-unit subgroup raised to both e and M, equivalently to g; log gives precisely `p^(v_p(N)+v_p(g))` as its congruence depth. Variation in the exponent additionally forces `c^M=1`. Outside N, the base ranges over all units, including 1: constancy means that the exponent of `(Z/p^k)^x` divides g. Its torsion/principal-unit decomposition gives the last two rows. Only primes dividing N or at most g+1 can occur, proving finiteness. These same formulas handle an exact nonzero integer exponent by setting M=0 and g=|e|; exact exponent 0 has no largest finite modulus, since the result is exactly 1.

**[proved here]** This unrestricted maximum is computable without factoring, but no cheap algorithm follows. For example it divides `B=8 N g (g+1)!` by the table. Enumerate the residues b modulo B satisfying `gcd(b,B)=1` and `b=c mod N`, choose one b0, and take the gcd of B and all `b^e-b0^e` and `b^M-1`, using modular powers. Constancy for exponents e and e+M is necessary and sufficient for all exponents in the coset, so this finite algorithm gives the maximum. It can be enormous. Promise only the cheap divisor-of-N fallback unless tighter precision is separately implemented and costed.

**[checked by a run]** The local maximum table agrees with the exhaustive gcd algorithm in 138 small input cases, including negative representatives of the exponent.

### R4 / N5 — restrict the all-place exact-rational root operation

**Quote [checked by a run].** “The function returns all roots with identifiers, or takes a seed”; “the all-places root ... is available for exact rationals”; PLAN 1F.8: “roots on exact rationals”.

**Failure [proved here].** For the exact rational 1, every independent choice of ±1 at each finite prime, together with a real choice ±1, is an adelic square root. There are continuum many. A finite list of local branch identifiers does not specify these, and a rational singleton cannot express an arbitrary such branch. An enclosing idele coset can contain all the roots, but also contains nonroots; it is not an enumeration of the roots. Round 2 correctly repaired local branches but left this global result contract incomplete.

**Replacement [proved here].** “The all-place exact-rational API returns a **diagonal rational** n-th root, with the nonnegative branch for even n and the unique real branch for odd n; optionally return both diagonal rational roots for even n. Test numerator and denominator for integer n-th powers, with the real sign condition. This API does not enumerate all adelic branches. Enumeration of all branches is supported only over a finite named set of places. Exact 0 has the single root 0. Degree 1 is the identity, including on idele enclosures; the all-place `NOT_DETERMINED` restriction is for nontrivial root degrees.” This can be implemented with integer root tests and does not require prime factorisation.

**[proved here]** This restriction loses no existence certificate for a nonzero rational: if a root exists in every `Q_p`, all valuations of the rational are divisible by n, so its absolute value is a rational n-th power. The real condition supplies the permissible sign. What it restricts is the set of branches returned, not the local-to-global existence test for this special equation.

### R5 — state the normalisations and limits of the analytic catalogue

**Quote [checked by a run].** “cheap once the types exist”; “Gauss sums, local constants ... a character at a place”; “complex ball”; cyclotomic direction “a convention to be fixed from a source”.

**Assessment and replacement [proved here].** “The displayed local zeta factors are for the trivial character. They are meromorphic: at finite p exclude `s=2 pi i k/log(p)` from finite-ball evaluation; at infinity exclude `s=0,-2,-4,...`. A ball containing a pole returns a pole/domain status or an explicitly unbounded enclosure, never a finite complex ball. Gauss sums specify the modulus/conductor, the multiplicative character extended by zero, and the additive character/sign. Local constants specify whether the result is epsilon or gamma, the quasi-character and s, SPEC 6's additive character and Fourier kernel, and the self-dual additive Haar measure. Fix their defining functional equation in 0.3/0.4 before implementing them.” For example defining gamma by `Z(hat f,chi^-1,1-s)=gamma(s,chi) Z(f,chi,s)` with the project's exact transform removes the otherwise ambiguous Fourier sign; epsilon additionally requires an explicit L-factor normalisation.

**Evidence [proved here].** The finite factor is the geometric series over nonnegative valuations for `Re(s)>0`; the real factor is `2 integral_0^infinity exp(-pi x^2) x^(s-1) dx`, evaluated by `t=pi x^2`. The displayed formulas follow and their poles prohibit a universally finite ball. A finite sum `sum_j chi(j) exp(2 pi i j/C)` can change when its additive character is replaced by its inverse; “a character” alone does not state that choice. These are missing conventions rather than reasons to reject these functions.

**Cyclotomic conventions [checked by a run].** With SPEC 5's `(t,u')`, arithmetic reciprocity (uniformisers map to arithmetic Frobenius) acts by `zeta_n -> zeta_n^(u'^-1 mod n)`; geometric reciprocity acts by `zeta_n -> zeta_n^(u' mod n)`. The first convention and the inverse of the unit projection are explicit in [Milne, Class Field Theory, V §5, pp. 180–182](https://www.jmilne.org/math/CourseNotes/CFT.pdf). The second is explicit in [Milne, Points on Shimura Varieties mod p, p. 8](https://www.jmilne.org/math/articles/1992aP.pdf). These sources were inspected; neither attribution is invented from memory. Keep the convention choice as a milestone-0 gate and record it in the API name/documentation and golden vector.

**[proved here]** A useful vector is the idele with p-component p and all other components 1: away from p its class unit is `p^-1`; arithmetic reciprocity must therefore send `zeta_n` to `zeta_n^p` for `p` coprime to n. The positive real coordinate acts trivially because a continuous map from a connected group to a finite group is constant. For a unit coset, the action on `mu_n` is determined exactly when its reduction there is fixed, equivalently `U(N) subset U(n)`; after the redundant factor-2 normalisation this is divisibility of canonical moduli.

**Tier placement [proved here; recommendation].** Keep every listed function in Tier A **with the stated Q/Dirichlet scope and staged delivery**: local constants at milestone 5 and theta at milestone 4 are already necessary for the Tate/Poisson promises. General local-field representations or arbitrary test-function oracles would belong in Tier B, but are not needed here. Replace “cheap” by “implemented as the prerequisite types and analytic algorithms arrive; costs depend on precision, conductor, support and any required factorisation”. All-place Hilbert support can require factoring scales; a directly summed Gauss sum has conductor-many terms; theta needs certified tails. The short useful omission is an explicit `haar_volume` for a finite ball: `1/N` for positive radius, 0 for a singleton, already justified by SPEC 7's measure. It uses only rational arithmetic and makes existing Fourier normalisation accessible.

### R6 — allow the validation code needed by milestone 0

**Quote [checked by a run].** PLAN 1: “Until then the only C is a provisional build scaffold”; 0.5: “Benchmark harness ... matched rows for the word kernels”.

**Issue and replacement [proved here].** A C/FLINT benchmark harness cannot remain an empty scaffold. Say “Until milestone 0 is reviewed, production C is limited to a provisional scaffold with no public types; disposable mathematical/API probes and benchmark harnesses are allowed.” The contract-first ordering and public-header freeze stay intact. This is a small scheduling contradiction, not an objection to performing the milestone-0 work.

### Verification of the remaining catalogue and proofs

**Binomial integrality and current enclosure [proved here].** For fixed k, the rational polynomial `binom(x,k)` is continuous on each `Q_p`; its integer values on the dense nonnegative integers imply that it maps `Z_p` into `Z_p`, hence `Zhat` into `Zhat`. The falling-factorial numerator changes by a multiple of N under `x -> x+Nt`, so division by `k!`, combined with integrality, gives exactly the claimed conservative modulus `N/gcd(N,k!)`. It is valid, including negative integer lifts, and is not tight in general. At k=0 return exact 1.

**Tighter and tight rules [proved here].** A stronger centre-independent enclosure is `N/gcd(N,lcm(1,...,k))` for k>=1: Vandermonde gives the difference as `sum_(j=1)^k binom(x,k-j) binom(Nt,j)`, and `j binom(Nt,j)=Nt binom(Nt-1,j-1)` loses at most `max_(j<=k) v_p(j)`, rather than `v_p(k!)`. The exact smallest enclosing additive ball for the specified centre has centre `binom(a,k)` and radius

```
R = gcd( binom(a+N*j,k)-binom(a,k) : j=1,...,k ).
```

**[proved here]** To see tightness, let `h(t)=binom(a+Nt,k)`. Its Newton expansion uses the integer coefficients `Delta^j h(0)` multiplying `binom(t,j)`. The gcd of the nonconstant coefficients equals the displayed gcd by triangular integer changes of basis. It divides every difference for all integral t, then all profinite t by continuity. Conversely the first k sampled differences must belong to any enclosing radius ideal. This proves both enclosure and minimality, without factoring. The gcd of an empty list is 0, handling k=0. This is an optional quality improvement, not a defect in the advertised conservative rule.

**[checked by a run]** The new check covers 9,020 binomial cases. For `(a,N,k)=(0,8,4)`, R=2 whereas the advertised rule gives 1. Precision can depend on the centre: `(0,8,3)` gives R=8, `(1,8,3)` gives R=4. These corroborate, rather than replace, the proof.

**Content [proved here].** The idele's finite valuations give the fractional ideal `product p^(v_p(x_p)) Z = r Z`. Since r is an exact field of the idele representation, returning its positive generator requires no new factorisation. It is distinct from the finite multiplicative absolute value `1/r` and the full norm `|x_inf|/r`.

**Theta identity and hypotheses [proved here].** For a Schwartz–Bruhat test function from SPEC 7 and an exact idele x, put `g(y)=f(xy)`. Change of Haar variable gives `hat g(z)=|x|^-1 hat f(z/x)`. Applying Poisson summation to g gives exactly `Theta_f(x)=|x|^-1 Theta_hat_f(1/x)` with the project's signs and self-dual measure; no additional minus sign or constant is needed. This deduction uses the Poisson theorem that milestone 0.3 explicitly requires proving for the implemented class; it does not assert that gate is already complete. Invertibility of x and the Schwartz–Bruhat hypothesis are essential. For example, at x=0 and f(0) nonzero, the sum diverges.

**[proved here]** For enclosure evaluation, require `Re(A)` bounded positively for the Gaussian parameters and `|x_inf|` bounded away from zero. If the finite support is `(1/D) Zhat` and the idele scale is r, contributing rationals lie in `(1/(Dr)) Z`; the Gaussian bound therefore gives summable, uniform tails. Finite-coordinate uncertainty must enclose every possible coefficient across jumps. SPEC 7 / PLAN 4 already assign these enclosure and tail obligations. The catalogue identity is correct under those hypotheses, not a pointwise evaluation rule for an unknown centre alone.

**SPEC 9.3.2–9.3.4 transcription audit [proved here].** The five series discs use `c=1` at odd p and 2 at 2; the log-series disc is `1+p Z_p` also at 2. The exp/sin/sinh linear terms strictly dominate their differences, while cos/cosh may contract; the stated centred hull exponent `2N-v_p(2)` is correct. For a nonzero ball, divide by a to obtain `1+p^(N-m) Z_p`: log/exp are inverse isometries there for `N-m>=c`, giving exactly `Log(a)+p^(N-m) Z_p`. At 2 with `N-m=1`, removing the sign maps the full odd-unit group onto `1+4 Z_2`; its log image is `4 Z_2`. This also proves the conservative all-place `4 Zhat` enclosure.

**[proved here]** Roots require valuation divisibility, an n-th root of the torsion factor, and `log(u)/n in p^c Z_p`, exactly the conditions printed. The principal-unit root is then unique; the cyclic torsion kernels have sizes `gcd(n,p-1)` and `gcd(n,2)`. On the printed guard `N-m>=c+v_p(n)`, the branch is `b exp(log(1+h/a)/n)`. Log, division by n and exp give a relative root radius `p^(N-m-v_p(n))`; multiplying by b gives `N-v_p(n)-(n-1)v_p(b)`, exactly the printed exponent. The guard ensures the inverse remains on its isometry disc. Integer powers retain valuation and torsion; principal-unit powers use exp/log, and odd 2-adic units additionally need the parity factor `w^(s mod 2)`. All these local formulas are correct. R4 concerns only the global branch contract.

**Proposition 3 zero cases [proved here].** Two singletons coincide/intersect/contain one another iff their rational points agree. If only the inner radius is zero, Lemma 1 gives membership by the integral quotient `(a-b)/M`. The symmetric case gives overlap in the other order. A positive-radius ball contains two distinct rationals, a and a+N, so cannot be contained in a singleton. These handle every zero/nonzero combination before division. The prototype now implements those branches.

**Proposition 4, step by step [proved here].** (1) The chosen e_p makes `d c_p` integral and `e_p+n_p>=0`. (2) Its residue modulo the *integer* power `p^(e_p+n_p)` has an integer representative; modulus 1 is harmless. (3) These powers are pairwise coprime, so CRT gives A. (4) Dividing by d yields `v_p(A/d-c_p)>=n_p`, and `v_p(N)=n_p` identifies the entire local ball. (5) Outside S, d and N are units and A/d is integral, giving exactly `Z_p`. Thus the product sets agree even for nonrational centres, negative exponents and empty S. The example's CRT moduli are 8, 3 and 25, and `a=101/90,N=20/3` satisfies them.

**Proposition 6, step by step [proved here].** (1) R/K is positive, so `s=gcd(a,R/K)>0` even when a=0. (2) `a/s` is integral, and reducing it modulo K changes the centre by a multiple of sK. (3) `R/(sK)` is integral, proving containment by Proposition 3; no coprimality assumption is missing. (4) For nonzero q, `q Zhat=|q| Zhat`, so multiplication gives exactly the claimed scale and signed residue; canonical residue reduction may follow. (5) Multiplication by zero is exact zero. The potential conversion loss is correctly stated. Exact scalar addition and mixed-context rules remain explicitly assigned proof work, not claimed consequences of this proposition.

**Runs and milestone-0 assessment [checked by a run; proved here].** [prototype.txt](checks/prototype.txt) records all supplied tests passing: 6,000 arithmetic, 40,000 scaled, 3,000 binomial, 1,815 power, 2,000 Hilbert-product and 2,029 modular-solvability checks. [checks.txt](checks/checks.txt) records the additional finite checks, including Proposition 6 and root-guard boundaries. No performance timings were rerun; the restored baseline hash was verified. PERF's remaining floors are conditional models, not impossible speed promises. Apart from R6, the milestone-0 exits can be evaluated as proof/source/convention/benchmark/seam deliverables; future proof obligations need not be complete to ratify their plan. The two major contract issues above must nevertheless be removed before ratifying the documents as written.

## Ratification

**[proved here] DO NOT RATIFY — R1 (wrong Kronecker precision) and R4/N5 (unspecified, potentially infinite all-place branch output) block ratification as written.**

1. **[proved here]** Apply R1's separate symbol domains/precision rules and its counterexample regression.
2. **[proved here]** Apply R4's diagonal-rational all-place root contract, local-only branch enumeration, and zero/degree-1 cases in SPEC and PLAN.
3. **[proved here]** Apply R2's finite exceptional-place/status contract and record the valid modular-oracle lifting argument.
4. **[proved here]** Apply R3's explicit modulo-N target, coarsening/status rule, signed powers and canonicalisation tests; do not claim D is the unrestricted tight modulus.
5. **[proved here]** Apply R5's normalisation/pole/cost wording and select the sourced reciprocity convention during milestone 0; keep the present Tier A staging.
6. **[proved here]** Apply R6's exception for validation and benchmark C. Retain the existing proof and review gates. The stronger binomial rule and explicit Haar-volume helper are optional improvements, not ratification conditions.
