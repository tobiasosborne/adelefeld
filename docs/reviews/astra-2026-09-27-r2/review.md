# Second review of adelefeld, 2026-09-27

Evidence labels apply to the paragraph, table row, or replacement they introduce. **[proved here]** means a proof, counterexample, or recommendation justified by the stated requirements; **[checked by a run]** means an inspected document/header or recorded execution; **[from memory]** means an unverified external claim. A design recommendation is not a claim of a uniquely optimal design. Closure judgements below are **[checked by a run]** for the revised wording and **[proved here]** for the remaining objections, explained once in section 3. A future proof gate resolves an ordering problem, but does not count as an already completed proof.

## 1. Closure table

| ID | Status | Reason |
|---|---|---|
| M1 | RESOLVED | The gcd rules now have proofs, including rational and zero radii; one proof-order detail is noted in N8. |
| M2 | RESOLVED | Basic neighbourhoods, rational singletons, and local/partial places are distinguished. |
| M3 | RESOLVED | Exact rationals have their own type; equality, overlap, containment, and point comparison are separated. |
| M4 | RESOLVED | The exceptional primes and the distinction between coordinatewise nonzero and invertible are corrected. |
| M5 | RESOLVED | Mixed unit precision, real rounding, class sign, conductors, quasi-characters, and additive hulls are specified. |
| M6 | PARTLY | Splitting and lifts are accepted; half-open endpoints and more than one real wrap still need a contract. |
| M7 | RESOLVED | The paired signs and the full finite phase image for fractional radii are fixed. |
| M8 | RESOLVED | Haar weights, array indexing, reflection, and weighted Plancherel are explicit. |
| M9 | PARTLY | The Gaussian family is repaired, but dilation still needs an explicit nonzero/invertible argument restriction. |
| M10 | PARTLY | Test vectors and the continuation gate are repaired; the zero assertion needs the nontrivial-character exception. |
| M11 | RESOLVED | Module certificates, simple local roots, and later completeness work have distinct contracts. |
| M12 | PARTLY | Full and partial reconstruction are separated; the modular input conditions and exact-radius branch need stating. |
| M13 | RESOLVED | The continuum count and positive-integer modulus restriction are correct. |
| D1 | RESOLVED | Three distinct sound policies replace the invalid fixed-radius rule; missing conversion proofs are explicitly gated. |
| D2 | PARTLY | Shared-denominator storage is fixed; the proposed local payload still cannot store its promised large blocks. |
| D3 | RESOLVED | Complex function values are separated from `C x A_f`; public result domains and lifecycle work are inventoried. |
| D4 | RESOLVED | The seams sketch precedes public-interface freeze, and function-field infinity is retained. |
| P1 | PARTLY | Milestones agree, but WP 0.1 still puts a header/library before the contract work that is meant to precede it. |
| P2 | PARTLY | Most old vacuous tests are replaced; the new elementary-function tests repeat this problem (N7). |
| P3 | RESOLVED | Analytic and solver deliverables have exits; estimates are qualified and text edits need no artificial tests. |
| F1 | PARTLY | Instruction, clock, and object sizes check out; allocator totals need a model label and counter access needs a recorded probe. |
| F2 | PARTLY | Most state counts are repaired; the 80/37 comparison still compares different features. |
| F3 | PARTLY | Invalid historical ratios are withdrawn; compulsory-I/O assumptions and the implementation-specific multiply bound need explicit limits. |
| F4 | PARTLY | Workload labels and a replacement-harness gate are present; changed source is still paired with the old measurement. |
| F5 | RESOLVED | Performance conclusions are now hypotheses about complete kernels; arb is retained. |
| F6 | PARTLY | Conversion/DFT/SIMD rows are useful conditional models; general read-all claims and the new elementary rows overstate their floors. |
| A1 | RESOLVED | Prior art is described modestly; the unread thesis and incomplete survey are disclosed and assigned work. |
| A2 | RESOLVED | The GRH-only claim is withdrawn and the needed general number-field backend is distinguished from existing FLINT facilities. |

## 2. New findings table

The severities are recommendations **[proved here]** from the failures below. BLOCKER means a blocker to freezing/implementing that contract, not to beginning corrective contract work.

| ID | Location | Severity | Finding |
|---|---|---|---|
| N1 | SPEC 9.3, convergence and full-adeles paragraph | MAJOR | The five convergence discs are right, but a nontrivial all-place domain exists; representability is a separate restriction. |
| N2 | SPEC 9.3, generic interface and character row | MAJOR | Named-place evaluation is sound; a default projection called `sin(adele)` obscures a change of domain and result type. |
| N3 | SPEC 9.3, radius rule | MAJOR | Keeping the radius is sound on the stated discs, but is not always tight; extended log needs a different rule. |
| N4 | SPEC 9.3; PLAN 1F.2 | BLOCKER | FLINT's centre arithmetic is not a ball wrapper, and its logarithm does not implement the proposed whole domain. |
| N5 | SPEC 9.3 roots; PLAN 1F.5 | BLOCKER | Root branches, divisibility conditions, precision loss, and certification over infinitely many places are unspecified. |
| N6 | SPEC 9.3 powers; PLAN 1F.5 | MAJOR | Integer powers, principal-unit powers, roots, and complex-valued quasi-characters are different operations. |
| N7 | PLAN 1F.1–1F.3, acceptance table | MAJOR | The proposed exp comparison works in Q_p only for odd p congruent to 1 mod 4 and needs other-prime oracles and width targets. |
| N8 | precision.md, all propositions; prototype | MINOR | Propositions 4 and 5 are valid; the missing work is explicit local moduli, zero-case order, conversions, and independent edge checks. |
| N9 | SPEC 4.1; PLAN 1F.1/1F.4 | MINOR | `C x A_f` is a legitimate archimedean enlargement, but is not complexification; partial complex places need a tag. |
| N10 | SPEC 9.3, remaining domain/type rows | MAJOR | Odd real roots, p-adic sign/floor, complex branches, and scalar-valued valuations need separate contracts. |
| N11 | PERF 4, elementary-function rows | MAJOR | Requested precision alone does not force that many output bits, and neither elementary row fixes a workload or output representation. |
| N12 | SPEC 5; PLAN 5/7, canonical value text | MINOR | Unit moduli are not unique: `U(2N)=U(N)` for odd N, so canonical set text needs a normalisation rule. |

## 3. Findings in full

### M6, M9, M12 — remaining small mathematical contracts

**Quoted text [checked by a run].** SPEC 6: “crosses an integer ... gives two pieces”; SPEC 7: “closed under ... dilation”; SPEC 9.2: “bounds `|n| <= A`, `0 < d <= B`; `2 A B < m` is sufficient for uniqueness.”

**What remains and evidence [proved here].** Two pieces suffice for one crossing; an interval spanning several integers can produce more, subject to merging. A closed arb interval cannot describe `[0,1)` exactly. Closed pieces in `[0,1]` are a valid alternative if their endpoints are interpreted with the finite-coordinate gluing: `(1,0 mod 2)` is `(0,-1 mod 2)`, not `(0,0 mod 2)`. Specify that alternative, endpoint flags, or an authoritative lift. Dilation by zero takes a nonzero Gaussian to a constant, which is not Schwartz. Modular reconstruction needs reduced fractions and denominators invertible modulo the modulus; otherwise “residue of a rational” is not defined by modular inversion. The full-ball algorithm also needs the `N=0` case before division by `N`.

**Replacement [proved here].** “A quotient value retains a lift, or pieces with explicit endpoint and gluing semantics; splitting enumerates every crossed integer and obeys a resource limit. Dilation is by an exact nonzero real scalar and an invertible finite scalar, or by a certified idele enclosure under the stated uncertainty semantics. Partial reconstruction takes `m>0`, reduced `n/d`, `gcd(d,m)=1`, and `n == c*d (mod m)` with the stated bounds. For full radius zero, test the sole candidate `a` against the real interval.”

### M10 — exempt the primitive trivial character

**Quoted text [checked by a run].** SPEC 8: “For a primitive character ... the same test function gives zero.”

**Counterexample [proved here].** The primitive trivial character has conductor 1; its spherical integral is the nonzero zeta integral already displayed. There is no ramified prime at which to average it to zero.

**Replacement [proved here].** “The spherical vector gives zero for a nontrivial primitive twist; conductor 1 is the zeta case.” Keep the continuation and Gauss-sum proof gates. Specify *absolute Haar integrability* when describing the `Re(s)>1` boundary: a conditionally ordered Dirichlet series outside it is not the defining adelic integral.

### D2, P1 — finish the representation before freezing it

**Quoted text [checked by a run].** SPEC 4.1: “A block too large for a machine word uses an integer fallback”; PLAN 4: `ulong *res`; PLAN 0.1: “Makefile, header, empty library”; PLAN 1: contracts “before the first C file.”

**Evidence and replacement [proved here].** A residue modulo `2^100` does not fit in `ulong`; the proposal needs a tagged word/fmpz array or a rule that *the whole value* falls back to global storage. Also say whether `A` is live in local mode or is a cache, and how canonical cancellation changes `H,d` and invalidates the context. Replace the local payload comment accordingly. Make 0.1 an opaque, disposable build scaffold, and freeze the public header only after 0.3, 0.4, and 0.6. This permits useful work now without claiming that the current structs are approved.

### N1 — convergence, extension, and representation are different questions

**Quoted text [checked by a run].** SPEC 9.3: “the power series converges exactly ... `p Z_p` (`4 Z_2` ...)”; “the only point of our types in the domain is 0”; “This is a fact about the adeles, not a gap in the software.”

**Convergence proof [proved here].** Write `v=v_p`, `v(p)=1`, and `v(0)=infinity`. Counting multiples of prime powers in `k!` gives `v(k!)=sum_j floor(k/p^j)=(k-s_p(k))/(p-1)`. Thus the exponential terms have valuations `k v(x)-v(k!)`. They tend to infinity for `v(x)>1/(p-1)`. The sine, cosine, sinh and cosh terms are odd/even subsequences with the same denominators, so the same sufficient condition applies. In Q_p the condition is `v(x)>=1` for odd p and `v(x)>=2` at 2. At odd p, `v(x)<=0` makes both parity subsequences fail the necessary term-to-zero test. At 2 with `v(x)=1`, terms of degree `2^j` have valuation 1 and terms of degree `2^j+1` have valuation 2. This proves divergence for all five series at the missing boundary. The claimed Q_p discs are **correct**.

**Logarithm proof [proved here].** The series `log(1+z)=sum_{k>=1}(-1)^(k+1) z^k/k` converges exactly for `v(z)>0`: valuations are `k v(z)-v(k)`, and `v(k)<=log_p(k)`; for `v(z)<=0`, the terms with `k=p^j` do not tend to zero. Therefore the series domain is `1+p Z_p` for **every** p, including `1+2 Z_2`. The smaller `1+4 Z_2` is the domain on which log is inverse to exp. Do not replace the mathematical series domain by a library's smaller accepted domain.

**Extension [proved here].** For odd p, write each nonzero x uniquely as `p^m omega u`, with `omega^(p-1)=1` and `u in 1+p Z_p`. Lift the nonzero residue root of `T^(p-1)-1` uniquely, since its derivative is a unit; this constructs omega. For p=2 write `x=2^m epsilon u`, `epsilon in {1,-1}`, `u in 1+4 Z_2`, choosing epsilon modulo 4. Define `Log_p(x)=log(u)`. The convergent-series product identity makes this a continuous homomorphism on all `Q_p^x`, killing the torsion factor and p. This is the Iwasawa convention meant here. Other homomorphic extensions can assign a value to p; fixing it to zero matters. At 2 the sign factor is **not** the Teichmüller lift: the latter of every odd unit is 1.

**Global counterexample [proved here].** The common finite domain of the five series is

```
D = 4 Z_2 × product_(p odd) p Z_p  ⊂ A_f.
```

It contains, for example, the nonzero adele with coordinate 4 at 2 and p at every odd p. Componentwise exp, cos and cosh have integral unit coordinates there; sin and sinh have integral coordinates; hence all five produce adeles. The functions are partial functions on A, with domain `R × D`, not everywhere-defined functions on A. D has empty interior: any basic finite ball leaves some odd coordinate unrestricted in `Z_p`, containing 1. Among *diagonal rational finite singletons*, only zero belongs to D, since a nonzero rational has only finitely many nonzero valuations. But `adf_adele` with finite part exactly zero permits any real coordinate, and positive-radius balls can contain nonzero points of D without being wholly contained in it. These are different assertions.

**Replacement [proved here].** “These series are not total operations on A. Their simultaneous finite domain is D above. No positive-radius `adf_fball` certifies membership in D; the only supported rational finite singleton that does is zero. Named-place evaluation is the general finite-data interface. A value with finite part exactly zero admits the simultaneous operation, with finite output 0 or 1 as appropriate.” The limitation on finite-data certification is partly a representation limitation, not a nonexistence theorem.

### N2 — offer projection explicitly, and keep characters distinct

**Quoted text [checked by a run].** SPEC 9.3: “by default the real place only”; “nothing is silently dropped”; “`exp(2 pi i x)`, hence `cos(2 pi x)`, `sin(2 pi x)` ... the character `psi`.”

**Assessment [proved here].** `f_S(x)=(f_v(x_v))_(v in S)` is a sound map to a product over a *finite named* S, with ball images computed place by place. It is useful. The surprise is the default: `sin` on a number-like adele changes both its places and its type. A result tag reveals the change after the fact; an explicit projection expresses it before evaluation. Different user expectations are a design judgement **[from memory]**; the type change itself is objective.

**Replacement [proved here; recommendation].** “Expose `sin(project(x, S))`, or `sin_at(x, S)` with mandatory S. `sin_at(x, infinity)` is an archimedean scalar/one-place result. Unqualified simultaneous `sin(x)` requires a certified domain and preserves the place set; otherwise return a domain/precision status. Do not retain untouched finite coordinates and call that componentwise sine. Keep the core atomic on failure; an optional per-place result/status map can retain successful evaluations.”

**Character evidence [proved here].** With SPEC 6's signs, `psi((r;0))=exp(-2*pi*i*r)`, while `psi(q)=1` for every diagonal rational q. In particular its real/imaginary parts on diagonal `q=1/4` are 1 and 0, not the ordinary cosine and sine of `2*pi*q`. A local `psi_p` is complex-valued, not a Q_p-valued p-adic exponential involving a p-adic pi or i. Replace the row by “additive character `psi:A/Q -> C^x`, and explicitly named real/imaginary parts.” Its analogy with the circle is useful; it is not an extension of diagonal rational sine.

### N3 — what the radius assertion really proves

**Quoted text [checked by a run].** SPEC 9.3: “`f(x + h) - f(x)` in `h Z_p` ... the output radius equals the input radius.”

**Disc estimate [proved here].** Put `c=1` for odd p and `c=2` for p=2. For `x,y in p^c Z_p`, factor `x^k-y^k` to get

```
v((x^k-y^k)/k!) >= v(x-y)+(k-1)c-v(k!).
```

For every `k>=2` the last two terms have positive difference (use the factorial formula above). Hence exp, sin and sinh are isometries: their linear term determines `v(f(x)-f(y))=v(x-y)`. Cos and cosh have no linear term and are strictly contracting on this disc. All five are 1-Lipschitz. Therefore keeping exponent N for a ball `a+p^N Z_p` wholly in the disc **is a sound conservative enclosure policy**, provided the computed centre is accurate modulo `p^N`. It is not a theorem that every smallest output ball has that radius.

**Tightness counterexamples [proved here].** On `p^N Z_p`, cos and cosh have smallest enclosing ball `1+p^(2N-v_p(2)) Z_p`. The quadratic term has this valuation at `x=p^N`, and every higher even term has strictly greater valuation. Thus at an odd prime `cos(p Z_p)` has hull radius `p^2`; at 2, `cos(4 Z_2)` has hull radius `2^3`, not `2^2`. A smallest *enclosing ball* need not be the exact image set.

**Log on its series domain [proved here].** For units x,y in one principal-unit disc, `log(y)-log(x)=log(y/x)`. If `v(y/x-1)>=c`, the first term strictly dominates the others, giving `v(log(y)-log(x))=v(y-x)`. This proves isometry for `1+p Z_p` at odd p and `1+4 Z_2`. On the larger 2-adic series domain, log kills -1 because `2 log(-1)=log(1)=0`; the product identity is valid by convergence of the series. Each odd unit is a sign times a member of `1+4 Z_2`. Consequently `log(1+2 Z_2)=4 Z_2`, since exp and log are inverse on `4 Z_2`/`1+4 Z_2` (their formal inverse identities are justified by the same convergent tails). For odd units differing by valuation 1 the logs differ by valuation at least 2; for differences of valuation at least 2, isometry holds. Thus the draft's 1-Lipschitz statement even on `1+2 Z_2` is correct, but keeping exponent 1 is loose and injectivity fails (`log(1)=log(-1)`).

**Extended log [proved here].** On all units, Iwasawa log is still 1-Lipschitz: different residue classes have unit difference at odd p and logs in `p Z_p`; matching classes use the preceding argument. On `B=a+p^N Z_p` excluding zero, put `m=v(a)`. For odd p, `N>m` gives the exact image ball

```
Log_p(B) = Log_p(a) + p^(N-m) Z_p.
```

At 2 this holds for `N-m>=2`; when `N-m=1`, B is a whole valuation shell and its image is `4 Z_2`. These follow by writing `x=a(1+h/a)` and applying the principal-unit isometries and their inverses. Absolute precision loses m digits when m is positive. Explicitly, `x=3,y=12` at p=3 have difference valuation 2 but log difference `log(4)` of valuation 1; `x=2,y=10` at p=2 have difference valuation 3 but log difference `log(5)` of valuation 2. The original radius cannot enclose both in either example.

**Replacement [proved here].** “Retaining input radius is the default safe policy on the stated discs, not a tightness claim. Extended log uses the valuation-adjusted formula above. Centre approximation, truncation error, and input uncertainty are combined by the coarsest resulting precision. Exact input does not imply an exactly representable transcendental output; a requested output precision remains necessary.” These formulas also specify tests at both primes, rather than leaving the 2-adic exception implicit.

**Runs [checked by a run].** [math_checks.txt](checks/math_checks.txt) records exact-rational truncations: `v_2(cos(4)-1)=3`, `v_3(cos(3)-1)=2`, `log_2(-1)=0 mod 2^16`, `v_2(log(3))=2`, and both extended-log losses above. Its factorial-tail bound is `k v(x)-v(k!) >= (k((p-1)v(x)-1)+1)/(p-1)`; its log cutoff uses `k-v_p(k)>=k/2` for `k>=4`. Thus these outputs are controlled finite checks, not a floating-point convergence experiment.

### N4 — FLINT provides the kernels, not these ball contracts

**Quoted text [checked by a run].** SPEC 9.3: “`exp`, `log`, square root and Teichmüller lift ... FLINT's `padic` module, which has them”; PLAN 1F.2: “through FLINT's `padic`.”

**Installed API [checked by a run].** [padic-header.txt](checks/padic-header.txt) records FLINT 3.0.1 declarations for `padic_exp` (generic/rectangular/balanced), `padic_log` (also Satoh), `padic_sqrt`, `padic_teichmuller`, and signed **integer** `padic_pow_si`. There are no padic sin/cos/sinh/cosh or general n-th-root/general-exponent functions in this header. The availability assertion is correct, but does not establish domains, branch choice, or propagation of input uncertainty.

**Probe [checked by a run].** Compiling and running [flint_probe.c](checks/flint_probe.c) against the installed library gives:

```
p=2: exp(2) status 0; exp(4) status 1
p=2: log(3) status 0; log(-1) status 0; log(5) status 1
p=3: log(2) status 0; log(4) status 1
p=2: teich(-1) = 1
exp: input precision 2, output precision 8;
     v_3(exp(3)-exp(12)) = 2
log: input precision 2, output precision 8;
     v_3(log(1)-log(10)) = 2
```

**Implication [proved here].** The last two pairs of arguments belong to the same input balls. Returning the centre computation at precision 8 as an image ball excludes a possible true output. Merely matching the input/output precision is also insufficient for roots or nonunit log. Domain checks must inspect the *whole input ball*: a stored zero with low precision is not exact zero. FLINT's return status is not an `ADF_DOMAIN`/`ADF_NOT_DETERMINED` distinction for that ball.

**Domain/source qualification [checked by a run].** The current [padic documentation](https://flintlib.org/doc/padic.html) specifies reduction to the output variable's precision. The installed 3.0.1 runs establish its restricted log branch, rather than an Iwasawa-log implementation; N1 distinguishes this from the maximal mathematical series domain. The official [3.0.1 manual](https://flintlib.org/download/flint-3.0.1.pdf), §12.1.11, additionally documents Teichmüller output zero for integral multiples of p and an abort for negative valuation; the wrapper must guard that domain. Web search retrieved manual excerpts, but full pinned-source downloads failed DNS; this review does not claim a 3.0.1 source-code audit. The reproducible evidence for installed behaviour is the header and probe. The header's `padic_equal` compares only unit and valuation, ignoring N; it must not serve directly as equality of ball sets.

**Replacement [proved here].** “Our local wrapper owns prime validation, exact/unknown zero, input precision, domain certification, output precision and root branches. FLINT evaluates centres only after those checks. For odd-unit `log_2`, remove the sign modulo 4 before calling FLINT. For general `Log_p`, remove `p^m` and the torsion factor, with enough internal precision for division/cancellation. Unsupported or ambiguous inputs are reported without exposing an undefined output.” Keep integer powers separate from the new power API.

### N5 — roots require branches and relative precision

**Quoted text [checked by a run].** SPEC 9.3: “where Hensel's lemma gives a root; the choice of root is stated”; “on ideles where every local root exists”; PLAN 1F.5: “`log`, roots and powers on ideles.”

**Domain and proof [proved here].** Fix an integer degree `n>=1`, and let `a!=0`. Use N1's decomposition `a=p^m omega u` at odd p or `a=2^m epsilon u` at 2. A root exists precisely under the following conditions:

| Place | Necessary and sufficient conditions |
|---|---|
| Odd p | `n` divides m; `omega` is an n-th power in `mu_(p-1)`; `v_p(log u)>=1+v_p(n)`. |
| p=2 | `n` divides m; `epsilon` is an n-th power in `{1,-1}`; `v_2(log u)>=2+v_2(n)`. |

Indeed the valuation and torsion factors must be n-th powers, while the principal factor is solved uniquely by `exp(log(u)/n)` on the principal-unit group. Necessity follows by taking log. Sufficiency follows by multiplying that root by a selected torsion root and `p^(m/n)`. The exp/log inverse used here follows from the convergent formal identities in N3. The torsion equations are finite and must be checked, not silently discarded. When solvable the number of roots is `gcd(n,p-1)` at odd p and `gcd(n,2)` at 2. To justify the only group fact needed: a finite subgroup of a field's multiplicative group is cyclic, since combining elements of maximal prime-power orders gives an element of the group's exponent E, and `X^E-1` has at most E roots. The group therefore has order E.

**Square specialisation [proved here].** At odd p, m must be even and the unit part a nonzero square modulo p. At 2, m must be even and the unit part congruent to 1 modulo 8. The latter follows also from squaring an odd integer. For example, 3 is not a square in Q_2 despite being a nonzero unit; 9 is a square, although the derivative `2x` is never a unit at an odd root in Z_2. Merely invoking *simple-root* Hensel lifting would miss it. Exact zero has the single root zero and needs its own uncertainty treatment.

**Precision proof [proved here].** Suppose `b^n=a!=0`, put `m=v_p(a)=n v_p(b)`, and let `c=1` at odd p and 2 at p=2. On an input ball `a+p^N Z_p` satisfying

```
N-m >= c+v_p(n),
```

choose the branch near b, namely `b*exp(log(1+h/a)/n)`. Then its image is exactly

```
b + p^(N-v_p(n)-(n-1)*v_p(b)) Z_p.
```

Log and exp are isometries on the required discs; division by n subtracts `v_p(n)`, and multiplication by b adds `v_p(b)`. Their bijections also give equality of these branch balls. This proves both the guard condition and the loss `v_p(n*b^(n-1))`; the formula is not justified outside the guard. Unit square roots lose no digits at odd primes, but at 2 a certified branch on `1+2^N Z_2`, `N>=3`, has radius `2^(N-1)` and is selected by its residue modulo 4. Unit p-th roots at odd p lose one digit, with input at least modulo `p^2`. A ball containing zero or crossing different power classes requires splitting, an enclosure of a specified root relation, or `NOT_DETERMINED`, not the same derivative formula.

**Branches [proved here].** There is no positive p-adic root. Return all branches with disjoint identifiers/certificates, or require a root seed/torsion choice. “Whichever FLINT returns” is not a stable mathematical branch: the probe returns `sqrt(9)=-3 mod 2^12`. For input uncertainty, the contract must specify one branch for every represented argument or the union of all admissible roots. A set containing some nonsquares is not an argument on which a total square-root function has been certified.

**All-place obstruction [proved here].** A finite unit coset `c U(N)` leaves all units possible at every prime outside N. Choose an odd prime also outside the valuation scale's support and choose a nonsquare residue there. This produces a represented idele with no square root. Thus ordinary finite-precision `adf_idele` values cannot certify the total all-place square-root domain, regardless of successful tests at finitely many named primes. An exact rational known globally, or a future symbolic power-image type, carries extra information which a unit coset does not. This is separate from returning the preimage relation on the subset of squares.

**Log at all places [proved here].** Iwasawa log has an easier but different tail: it sends *every* `Q_p^x` into `p Z_p` at odd p and `4 Z_2` at 2. Hence its full finite image is adelic, but generally is not a single rational coset. A conservative representable output is `0+4 Zhat`, refined at finitely many requested places using N3. The real coordinate still needs positivity for real log, or a separately named `log_abs`. State this enclosure policy instead of suggesting infinitely many local evaluations can be materialised exactly.

**Replacement [proved here].** “Milestone 1F supplies local roots with the above domain certificates, explicit branches and precision losses, including nonsimple 2-adic square roots and p dividing n. Named-place roots extend to partial balls. Global roots require additional certified structure; generic idele cosets return `NOT_DETERMINED` for universal square-root eligibility. A root-preimage solver, if offered, is a separate operation. All-place log returns an explicitly described finite-precision adelic enclosure.” The degree, branch, target precision, completeness, and resource-limit arguments belong in 0.4.

### N6 — separate four meanings of powers

**Quoted text [checked by a run].** SPEC 9.3: “`x^s` ... through `log` and `exp` where both converge ... on idele classes: the quasi-character `t^s chi`.”

**Evidence [proved here].** `exp(s Log_p(x))` kills valuation and torsion. For example `exp(Log_p(p))=1`, not p; at 2, `exp(Log_2(-1))=1`, not -1. Thus even at integer s=1 this formula is not a general power operation. For odd p and a nontrivial Teichmüller unit omega, integers `p^j` tend to 0 in Z_p while `omega^(p^j)=omega`, so no continuous Z_p-exponent operation on that base can agree with all integer powers. A nonzero valuation obstructs continuity similarly: `p^(m*p^j)` tends to zero or becomes unbounded, not to 1.

**Replacement [proved here].** Offer these separately:

1. Integer powers by ring/idele arithmetic; negative degrees require invertibility. This needs no log convergence.
2. Rational exponents via the explicit root branches of N5.
3. Principal-unit powers: `exp(s log u)` when `v_p(s log u)>1/(p-1)`. For `s in Z_p`, this always works on `1+p Z_p` at odd p and `1+4 Z_2`. On all odd 2-adic units, integer-compatible Z_2 powers instead use `epsilon^(s mod 2)*exp(s log u)`. The binomial series gives the same extension on `1+2 Z_2`: its coefficients are integral for Z_2 exponents by continuity from nonnegative integers, and its terms tend to zero. For odd-p general units, offer a separate torsion exponent modulo `p-1` alongside the principal-unit exponent, if needed.
4. `t^s chi(u')` is a **complex-valued quasi-character** for complex s and positive real t. It returns acb, not an idele or a p-adic power.

For ball-valued base and exponent, propagate uncertainty through multiplication of `s` and `log u`; the exponent's accuracy is not free. All-place noninteger powers inherit N5's unresolved tail/branch issues.

### N7 / P2 — repair the elementary acceptance tests

**Quoted text [checked by a run].** PLAN 1F.3: “over an extension where `sqrt(-1)` exists in `Q_p` (`p = 1 mod 4`)”; 1F.1: “agreement with `arb`”; 1F.2: “enclosure on random balls against exact rational points.”

**What works [proved here].** For odd p, `i in Q_p` exists exactly when `p == 1 (mod 4)`: use the cyclic residue group above and lift a root with derivative `2i` a unit. At 2, -1 is 7 modulo 8, so there is no root. When i is already in Q_p, no field extension is required and

```
sin x = (exp(i*x)-exp(-i*x))/(2*i),
cos x = (exp(i*x)+exp(-i*x))/2
```

hold on the convergence disc by comparing the absolutely convergent series. An approximate i needs a certified root enclosure and enough precision. For p congruent to 3 modulo 4 an extension would be additional infrastructure; it is unnecessary for a first oracle. Hyperbolic comparisons use `exp(x)` and `exp(-x)` at **every** prime, with an extra absolute digit before division by 2 when p=2.

**What does not test the claim [proved here].** A fake `sin=0, cos=1` passes `sin^2+cos^2=1`. Whole-disc output balls can pass every enclosure/identity check. A wrapper calling arb compared with the same arb call tests routing, not independent enclosure mathematics. Rational input points do not make their transcendental outputs rational; one needs a certified reference computation, not an assumed “exact rational” output.

**Replacement [proved here].** “Compare custom series with exp formulas at p=5 and 13; use independent exact-rational truncations with valuation tail bounds at p=2 and 3 as well. Include `cos(4)=9 mod 16` at 2 and `sin(3)=3 mod 9` at 3, domain boundaries, negative valuation, zero balls, N3's precision cases, and N5's root losses. Require a stated output precision/width, not just identity overlap. Arb/FLINT wrapper tests check projection, guard digits, error/status translation, and input/output uncertainty; they are explicitly wrapper tests.” The truncation bounds and runnable oracle are in [math_checks.py](checks/math_checks.py); no extension-field implementation is needed for these tests.

### N8 — audit of the precision proofs, step by step

**Quoted text [checked by a run].** Proposition 4: “modulo the power of `p` in question”; Proposition 5: “an integer multiple of `s t K`; apply Proposition 3(3)”; the closing list: “Still to be proved here.”

**Audit [proved here].** Lemma 1 is correct with a *reduced* rational denominator; divisibility at every prime eliminates it. Lemma 2 correctly uses integer Bezout coefficients after rational scaling and covers either/both zero radii. Proposition 1 is an exact Minkowski sum. Proposition 2's expansion proves enclosure; its four witnesses force any enclosing translation subgroup to contain all three coefficients, hence their gcd. This is stronger than merely excluding numerically smaller radii, since it proves containment in *every* enclosing ball. Move its `G=0` paragraph before any implicit division by G; no formula changes. Proposition 3 correctly deduces overlap from subgroup sum, containment from membership and subgroup inclusion, and equality from two positive integer ratios.

**Proposition 4 [proved here].** It is valid even for negative `n_p` and nonrational `c_p`. Spell out `d=product p^e_p` with `e_p>=max(0,-v_p(c_p),-n_p)`, omitting the valuation bound when `c_p=0`. The actual integer CRT modulus at p is `p^(e_p+n_p)`, not the generally composite rational expression `d*p^n_p`; its unit factor has no effect on the local ideal. An exponent zero gives modulus 1 and imposes no condition. Density of Z in Z_p supplies each `A_p`. The moduli are pairwise coprime. Finally `v_p(N)=n_p` on S and zero elsewhere proves equality of the local **sets**, not just that a lies in them. This supplies the last implicit step. The recorded example gives `a=101/90`, `N=20/3` from local centres `1/2,2/9,7/5` and exponents `2,-1,1` at 2,3,5.

**Proposition 5 [proved here].** It is also valid. Positive s,t give positive g and coprime integers A,B; `gcd(AK,BK)=K` proves exact addition. In the product, all three coefficients factor as `stK` times `u,v,K`; their gcd is `stK*gcd(u,v,K)`. K is positive, so this radius is positive even when u=v=0. The quotient by the proposed radius is an integer, and both balls have centre stuv, precisely the two conditions of Proposition 3(3). Reducing the residue modulo K changes the centre by an integer multiple of its radius. There is no counterexample or hidden coprimality condition on u,v.

**Missing proof obligations and replacement [proved here].** Keep the closing list, and add elementary-function domain/precision/truncation proofs and zero-radius predicates. For example, singleton overlap is rational equality if both radii vanish, and rational membership if only one vanishes; a positive-radius ball cannot lie inside a singleton. For scaled conversion of `a+R Zhat`, `R>0`, to modulus K, `s=gcd(a,R/K)`, `u=a/s mod K` gives an enclosure because `R/(sK)` is integral. Exact nonzero scalar multiplication uses scale `|q|s` and residue `sign(q)u`; scalar zero stays an exact zero. Exact scalar addition needs its own alignment/widening rule. Mixed contexts, absolute-cap behaviour on exact tags, canonical storage conversion, and error states are still obligations, not consequences of Proposition 5 alone.

**Prototype evidence [checked by a run].** [prototype.txt](checks/prototype.txt) reports 6,000 arithmetic and 40,000 scaled checks passing; the DFT errors are `7.9e-16` and `4.4e-16`. The new exact-point probe makes `overlaps(1,0,1,0)` and `contains(1,0,1,0)` raise `ZeroDivisionError`; their tested positive-radius paths do not implement the promised singleton cases. The fractional-splitting test samples 41 integer translates; it does not prove universal coverage. Keep it as a regression and add exhaustive finite quotient coverage. These limitations do not invalidate the mathematical proofs above.

### N9 — the complex type is coherent, but specify its purpose

**Quoted text [checked by a run].** SPEC 4.1: “the ring `C x A_f` ... useful (complex shifts of the real coordinate)”; PLAN 1F.1: “on `adf_adele` and `adf_cadele`, returning partial balls.”

**Assessment [proved here].** This is a legitimate ring with a natural inclusion `A -> C x A_f`, and is a reasonable optional domain for complexifying only archimedean arguments. It is not a C-algebra: a unital complex-scalar action would supply a square root of -1 in its Q_3 factor, which N7 rules out. Thus `(i;0)` squares to `(-1;0)`, not to the ring's `-1=(-1;-1)`. This distinction matters for anything called “a complex number type.” Complex-valued test functions already have acb outputs and complex Gaussian parameters; they do not require this ring.

**Alternatives [proved here; recommendations].** Keep `C x A_f` if archimedean shifts are the intended use, preferably with an explicit name such as “complex archimedean adele.” If the intention is adjoining a global i, use `A[T]/(T^2+1)` instead: its finite factors are `Q_p[T]/(T^2+1)`, not Q_p. At p=5 that factor splits into two copies of Q_5; at p=3 it is a quadratic field. This is the finite-place behaviour required by the adeles of Q(i), and belongs with the later number-field work. If only pairs of adelic signals are wanted, `A x A` is yet another construction, with no implicit choice of complex multiplication.

**Replacement [proved here].** “`adf_cadele` is an optional archimedean enlargement, with no unital complex-scalar embedding into its finite factor. Its infinity projection has type C, unlike the R infinity projection of `adf_adele`. Partial balls carry the archimedean field tag; real-only operations reject complex input unless their complex branch contract is defined.” This makes the present meaning usable without overselling it as the uniquely useful interpretation.

### N10 — repair the remaining function-table domains and codomains

**Quoted text [checked by a run].** SPEC 9.3: “`sqrt`, n-th roots | non-negative reals”; “absolute value, valuation, sign, floor and fractional part | everywhere | everywhere”; PLAN 1F.1 promises the same list through arb/acb.

**Evidence [proved here].** Odd real n-th roots exist for negative inputs (`(-2)^3=-8`); even roots need a sign or nonnegative-branch convention. A real ball crossing zero needs different treatment for square root, reciprocal, and log. Valuation at zero is infinity, and sign/absolute value/valuation do not return the same scalar field as their input. There is no real-style ordered-field sign/floor on all Q_p: Q_5 contains a square root of -1, so no compatible order exists there. A p-primary fractional-part section is meaningful, but its complementary “integral part” is in Z_p, not necessarily an ordinary integer. On C, logarithm and noninteger powers require branch cuts and behaviour on balls meeting zero/cuts; “through acb” does not specify the project's semantics.

**Replacement [proved here].** Split these into rows with explicit domains and result types: real/complex/local absolute value; local valuation in `Z union {infinity}`; real sign and integer floor with ambiguity on crossing balls; a separately defined local fractional-part section; even/odd real root branches; and complex log/root/power branches. The already-correct rational-function requirement should also be phrased as certification of a nonzero local denominator versus a global idele denominator. Do not use one blanket “everywhere” row.

### F1, F2 — remaining hardware evidence and space comparisons

**Quoted text [checked by a run].** PERF 1 calls the glibc chunk formula “measured”; PERF 2 compares “finite ball as two `fmpq`, all small” with “16-byte two-integer finite part + 21-byte real part: x2.16.”

**What checks out [checked by a run].** The installed probe reproduces all five FLINT sizes and also `sizeof(__mpz_struct)=16`, so the mpz size used in the space calculation is correct. Guest CPU/cache information agrees with the table. Re-reading the primary measurements confirms Zen 2 [MUL's 3/4-cycle outputs and one/cycle throughput](https://www.uops.info/html-instr/MUL_R64.html#ZEN2), and [VPMULUDQ's four products, three-cycle latency, one/cycle throughput](https://uops.info/html-instr/VPMULUDQ_YMM_YMM_YMM.html#ZEN2). The round-1 ADD/ADC parameters remain conditional model inputs; this round did not remeasure instructions or clock frequency. A fresh `perf_event_open` probe for user-space hardware cycles fails with `errno=2`; this supports an unavailable event in this invocation, not a claim that every PMU facility is universally unavailable.

**Allocator qualification [proved here, observations checked by a run].** `malloc(512)` reports 520 *usable* bytes. That does not measure the full chunk footprint, allocator metadata, arena slack, or pooled FLINT objects. `max(32,round16(n+8))` remains a particular glibc small-chunk **MODEL**, consistent with the recorded samples. Under that model, `528/512=1.03125` and `(528+16+8)/512=1.078125` are correct. They are payload-plus-selected-overhead estimates, not measured resident bytes for a general fmpz.

**Space mismatch and replacement [proved here].** The x2.16 arithmetic is `80/37`, but two rational fields permit independent denominators and a general arb has different exponents/radii from the restricted 21-byte real model. A two-integer finite part does not have those states. Delete this as an overhead/floor ratio, or label it only a comparison of unlike restricted layouts, as for 56/29. The `(A,H,d)` proposal is a third layout and needs its own measurement. Clarify that the `Nd` count means an *integer radius N* and centres on the fixed grid `(1/d)Z`; it is not the count for an arbitrary fractional-radius family. The 20! ratios are arithmetically correct against the **unrounded entropy** 61.077... bits; label that denominator rather than the rounded 62-bit storage floor.

### F3, F4, F6 — keep the surviving ratios conditional

**Quoted text [checked by a run].** PERF 3: “one high-half multiply ... 4 cycles”; “at least 14 cycles”; “write 520: `max(16,17)=17`”; PERF 4: “all `n` bits are read”; PERF 7: the baseline “is kept as a dated reference.”

**Ratios [proved here from the supplied times].** Multiplying ns by the explicitly assumed 3.7–4.5 GHz gives the correct ranges: add `2.738–3.330` times one cycle; mul `4.616–5.614` times four cycles; mul `1.319–1.604` times fourteen cycles. These are conditional ratios, not calibrated performance bounds. The mandatory ADD/high-half MUL assumptions describe the selected implementation, not every possible modular algorithm. Keep the labels at that level; no speedup follows from a large ratio to a weak floor.

**Compiled path [checked by a run; model derivation proved here].** [baseline_current_O2.s](checks/baseline_current_O2.s) reproduces the round-1 multiply path. With three-cycle immediate IMUL and one-cycle ADD/ADC/SUB as explicit model assumptions, its partial dependency length is `4+max(4,3+1)+1+1+3+1=14`. The immediate-IMUL parameter remains **[from memory]** in this review; it was not independently timed. The code-specific bound is sound conditional on those assumptions, and does not include all corrections. The current source changed b's random range, so it is not the exact program that produced the preserved output. Preserve the old source/hash with that output, or attach new timings to the new hash; do not silently relabel the old measurement. No historical performance timing was rerun here.

**I/O conditions and replacement [proved here].** Require compulsory fresh dense inputs, newly materialised outputs, an explicit layout and cache state. Without these, cached results, exact zeros, early exits, and small tagged values can avoid the claimed traffic. Under pure 64-byte-read/32-byte-write capacity, the sum's payload bound is `max(1024/64,520/32)=16.25` cycles per result. Rounding it to 17 requires whole per-result store issues/no packing across results; it is valid in that stronger model, not from bandwidth alone. The 32-, 24-, and 16-cycle vector rungs for multiply/reduction/gcd, their scalar alternatives, the CRT formulas, dense DFT output bound and packed-add bounds are otherwise correct **under compulsory-I/O contracts**. The selected AVX2 product kernel needs `ceil(k/4)` multiply issues for an isolated k-vector; `k/4` is its weaker asymptotic resource rung, not a complete reduction cost.

**Read-all claims [proved here].** “PROVED: all n bits” should be an explicit-interface requirement or a worst-case bit-access theorem with an input family, not a claim about every call. For example, a zero-tag CRT input or a reconstruction problem with degenerate bounds can have a constant-size result. Parsing can reject an invalid prefix without reading the whole string; validating a full valid string has a different worst-case requirement. Replace these rows with “worst-case explicit dense input/output” and list which bytes are compulsory. A call-rate benchmark *can* be compared with a throughput floor when these contracts match; “call rate” alone is not the reason the present fixed-operand numerator is unsuitable. Continue withholding the historical large-number/arb optimality ratios.

### N11 — output precision is not automatically output length

**Quoted text [checked by a run].** PERF 4: real elementary function “PROVED: `p` output bits written”; p-adic exp/log “PROVED: `n log2 p` output bits written.”

**Counterexamples [proved here].** `exp(0)=1`, `sin(0)=0`, and `log(1)=0` have short exact representations at every requested precision. An inexact *input ball* may already prevent the requested accuracy. A normalised padic value may have far fewer than n significant digits. Even the output-state count on the principal discs is `p^(n-c)` for exp and log at precision `p^n`, where `c=1` for odd p and 2 at 2 and `n>=c`: exp fixes the first c digits, and log's image is divisible by `p^c`. This follows from the isometries above. It is an information count, not a per-call store count.

**Replacement [proved here].** “For dense materialised b-bit real mantissas, add the I/O floor for `ceil(b/8)` output payload bytes, with exponent/radius metadata specified. For an explicitly padded n-digit p-adic residue, use its required output bytes; for compact representations use actual significant precision and distinguish worst-case output size from every-call cost. Record prime size, input valuation, absolute versus relative accuracy, branch, and reusable context data. No arithmetic-optimality ratio is established.” Keep rectangular/balanced splitting as FLINT algorithm references. The blanket quasi-linear real-function reference remains **[from memory]** until tied to a particular function, argument range, error target and source; large-argument reduction and root degree cannot be omitted from its parameters.

### N12 — canonical unit-coset text needs one extra reduction

**Quoted text [checked by a run].** PLAN 5: “Value form: canonical”; PLAN 7: “the same set from different inputs prints identically.”

**Proof [proved here].** Every profinite unit is already 1 modulo 2. Consequently `U(2)=U(1)` and, for odd N, `U(2N)=U(N)`. For example, the unit cosets represented by `5 mod 6` and `2 mod 3` are the same set, although their additive balls are different. A least residue alone cannot canonicalise the unit-coset value.

**Replacement [proved here].** “In canonical unit-coset value text, remove a factor 2 when the modulus has 2-adic valuation exactly 1, then reduce c modulo the resulting modulus. Here `c U(N)` means any profinite **unit lift** of the residue c, not the diagonal integer c, which need not be a unit. A physical dump may preserve the supplied presentation modulus. Unit-coset equality compares semantic constraints, not just the two stored integers.” This agrees with the separate value/dump design.

## 4. What is right and should be kept

**[proved here]** Keep the finite-ball gcd arithmetic, shared denominator, exact rational type, distinction between local and full information, scaled-residue policy, quotient lifts/splitting, idele topology, Fourier signs and Haar weights. Propositions 4 and 5 need clarification, not replacement. Elementary functions belong naturally in the basic package once local domains and precision are explicit; named-place evaluation is useful.

**[checked by a run]** The extended prototype passes, the claimed FLINT kernels exist, the five series discs in SPEC are confirmed by the proofs above, and the principal hardware figures and arithmetic of the surviving conditional ratios check out. **[proved here]** Keep independent analytic paths, proof gates, certified tails, the restrained performance hypotheses, and the decision to retain arb.

## What this changes in the plan

The order below is a recommendation **[proved here]** from the demonstrated dependencies.

1. Begin milestone 0 with a provisional build scaffold; freeze no public representation until 0.3, 0.4 and 0.6 are reviewed.
2. Correct 9.3's domain table and projection names. Specify the complex infinity tag and separate characters, valuations, integer powers and analytic powers.
3. Add N1–N6's local domain, branch, uncertainty and tail proofs to 0.3; make the FLINT precision counterexample a mandatory wrapper regression.
4. Split 1F into archimedean wrappers, local exp/log, local branches/roots/powers, custom series, then partial-place composition. Keep general n-th roots in the basic-package plan, with explicit work for p dividing n.
5. Replace generic all-place roots on idele cosets with the named-place interface or a separately specified certified domain. Define the conservative all-place log output.
6. Finish zero predicates, conversions, large-block fallback, quotient endpoints, and canonical unit-coset text; use the repaired independent acceptance tests and explicit precision targets.
7. Correct PERF's model labels, compulsory-I/O contracts and elementary rows; bind each new measurement to its source hash. Complete source retrieval and the existing analysis/seams proof gates before milestone 0 exits.

**Overall verdict [proved here]: Yes, milestone 0 may begin as the planned contract/proof phase; its exit gate and the elementary-function interface are not yet approved.**
