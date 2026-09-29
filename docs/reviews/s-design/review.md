# Adversarial review of milestone S
VALID 28 / MINOR 3 / INVALID 2. NOT READY

Reviewed on 2026-09-29. The counts include Definitions 1.1, 1.3 and 2.1, and Remark 3.6.
The 30 rows of the author's final table omit those three definitions.
No design, specification, plan, convention, or author check was changed.

The reconstruction and modular linear algebra survived the independent finite checks below.
The p-adic search survived the checks at 2 and 3. Its central proofs close.
Two numbered statements are false as written. The root interface also has conflicting contracts.
The real reference algorithm raises an exception on an admitted integer polynomial.
These findings must be repaired before the design is implemented literally.

## Statement verdicts

| Statement | Verdict | Finding or evidence |
|---|---|---|
| D1.1 | VALID | Finite reduced pairs, including m = 1 and A = 0; 60,480 boxes. |
| L1.2 | VALID | A common divisor of d and m divides n; the valuation converse closes. |
| D1.3 | VALID | The four conditions define a lattice basis with the required signs. |
| L1.4 | VALID | EEA identities apply also at c = 0; 1,334 independently found certificate pairs. |
| P1.5 | VALID | Both directions of phi and injectivity hold; arbitrary pairs were tested. |
| P1.6 | VALID | All four cases hold; 302,400 bounded and unbounded-search calls. |
| P1.7 | VALID | The status conditions and loop bound hold; the author's limit test is weaker. |
| P1.8 | VALID | The Thue application correctly uses A + 1 and B + 1. |
| P1.9 | VALID | The documented and one-limb claims hold; multi-limb proof is expressly excluded. |
| P1.10 | VALID | Forgetting gives a proper inclusion, including H = 1. |
| D2.1 | MINOR | Correct form; repair the associate-of-zero wording and the source location. |
| L2.2 | VALID | First differing coefficient proves the product lower bound. |
| L2.3 | VALID | Annihilator criterion, greedy membership, count, and suffix property close. |
| P2.4 | VALID | Ideals determine pivots; reduced differences determine all remaining entries. |
| P2.5 | VALID | Pending-row invariants close; 32,946 arbitrary two-row inputs were checked. |
| P2.6 | VALID | The two lower bounds and their product force the complete image and kernel. |
| P2.7 | VALID | The separating character exists over every Z/N, including composite N. |
| P2.8 | VALID | 1,875,272 systems; zero dimensions and rectangular systems included. |
| P2.9 | VALID | Integer multiplication is injective on Zhat; 250 ball systems were checked. |
| P2.10 | VALID | Coordinate ideals give exactly the projections; 184 projections checked. |
| P2.11 | MINOR | Qualify the out-of-bounds claim by nonempty input; fix one documentation line. |
| P2.12 | VALID | Triangular integer elimination proves all three claims; 2,560 HNF probes. |
| L3.1 | VALID | The multiplicity argument closes; its missing EEA source is on disk. |
| P3.2 | VALID | Strong Hensel proves existence and uniqueness; 29,000 candidate certificates. |
| P3.3 | VALID | Taylor expansion gives 2k - s; 2,477 steps match exhaustive extensions. |
| P3.4 | VALID | The change of variable and both content increments hold. |
| P3.5 | VALID | The opened-class invariant, partition, depth claim, and termination proof close. |
| R3.6 | MINOR | The conditional bound closes; give that proof and separate the resultant claim. |
| P3.7 | VALID | Root counting by gcd degree holds; 791 finite-field polynomials checked. |
| P3.8 | VALID | Conditional on the stated IVT and a correct count; neither is a certificate field. |
| P3.9 | INVALID | X^2, X^3 and X^4 return 2, 3 and 4; the stated outside-contract behavior is false. |
| P3.10 | VALID | Its completeness implication holds; the reference and API need separate repairs. |
| 3.11 | INVALID | s + 1 is sufficient, not the smallest possible isolating precision. |

## Proof audit and counterexamples

### D2.1: the zero associate and the citation

The representative of the associate of zero in Z/N is zero, not the integer gcd(0, N) = N.
The nonzero pivots are unaffected. The inheritance formula is at
`refs/src/storjohann-thesis/diss2up.txt:565-567`, with the residue formula at line 521.
The cited lines 515-523 do not contain the associate formula. Repair R1 gives the replacement.

### P2.5 and P2.6: the two main linear algebra risks

No gap remains in I3. At a column j, a change involving the current vector and T[j] preserves their joint
span. It therefore preserves every earlier column's available later span. For a column after j, those
vectors were excluded from its I3 expression; newly pending vectors can only enlarge that available span.
The new annihilator vector establishes I3 for j. Final row reductions preserve every suffix span.
A pivot replacement strictly decreases a positive divisor, so the pending-vector bound also closes.

For P2.6, put P = product(N/p_i) and Q = product(N/g_i).
K1 gives |S(E)| >= P and |S(G)| >= Q. K2 and K3 give S(E) inside Im A and S(G) inside ker A.
The first isomorphism theorem gives |Im A| |ker A| = N^c = P Q.
Both inclusions and both inequalities must be equalities. A false incomplete kernel cannot satisfy K1-K4.
K5 then establishes the canonical representation, not the counting argument itself.
The independent scalar-certificate search tried 105,106 invented certificates and accepted 237; none was false.

### P2.11: empty input and a documentation line

The out-of-bounds conclusion applies to a nonempty matrix with fewer rows than columns.
An empty matrix returns before that loop:
`refs/src/flint-src-3.0.1/fmpz_mat/strong_echelon_form_mod.c:167-168` and
`refs/src/flint-src-3.0.1/fmpz_mat/howell_form_mod.c:20-21`.
The row-padding precondition for fmpz_mod_mat_howell_form is at
`refs/src/flint-3.0.1/fmpz_mod_mat.rst:293`, not line 291.
This is a prose repair. Padding nonempty inputs remains necessary.

The probes compared fmpz Howell, nmod Howell, and integer HNF with independently enumerated spans.
There were 2,560 matrices, including N = 1 and empty dimensions, and zero differences.
This is finite evidence, not a proof of all FLINT paths.

### P3.2-P3.5: the author's first two uncertainties

The strict inequalities are sufficient at both 2 and 3. Equality in strong Hensel is insufficient:
f = X^2 + 3, p = 2, a = 1 has v(f(a)) = 2 and v(f'(a)) = 1, but no root modulo 8.
The stated checker rejects the candidate k = s = 1.

For P3.2(3), the truncated centre a' differs from a by a multiple of p^k'.
Thus f'(a') = f'(a) modulo p^k', preserving valuation s since k' > s.
Taylor gives v(f(a')) >= k' + s. This supplies the detail behind the abbreviated reference to part 2.

For P3.5, W is needed only for an opened class whose transformed polynomial has a root modulo p.
Such a child increases content by at least 2. Induction gives w >= 2e.
At a simple digit, s = w - e >= e, hence K >= s + 1 >= e + 1.
Reducing to precision K therefore keeps the returned ball inside the child where its root was found.
For a simple root with derivative valuation s, expansion about the root gives w = e + s when e > s.
The digit is then simple. This proves the stated sufficient depth bound.
An infinite unresolved chain has f(a_e) tending to zero and f'(a_e) tending to zero, so its limit is multiple.
The finite-branching argument therefore proves eventual completeness when all roots are simple.

The tests used 1,878 calls at each prime. They checked 2,477 returned certificates and Newton steps.
The separate malformed-certificate search tried 29,000 triples and accepted 352, with zero false acceptances.
Finite congruence tests do not by themselves prove that approximate roots lift indefinitely.

### R3.6: close the conditional claim; do not conflate two bounds

The displayed bound follows directly from W and the displayed integer Bezout identity.
An opened class at depth e with a child gives v(f(a)) >= 2e and v(f'(a)) >= e.
Evaluating u f + v f' = R there gives v_p(R) >= e.
Consequently no class at depth v_p(R) + 1 has a child, and that depth suffices.

For nonconstant squarefree f, polynomial EEA over Q supplies a rational Bezout identity.
Clearing its finitely many denominators supplies a nonzero integer R and integer polynomials u, v.
This already proves a usable bound, without the resultant assertion. Repair R3 states it precisely.
A bound involving the resultant must not silently become a bound involving just the discriminant when
p divides the leading coefficient. No implementation relies on this optional remark.

### P3.9: false descriptions of FLINT outside its contract

The code first removes the full power of X dividing the polynomial, then adds that exponent to the answer:
`refs/src/flint-src-3.0.1/fmpz_poly/num_real_roots.c:107-117`.
The installed FLINT 3.0.1 returns 2 for X^2, 3 for X^3, and 4 for X^4. None throws.
Each polynomial has exactly one distinct real root. Thus both blanket claims in P3.9(1) fail:
non-squarefree quadratics do not always return zero, and non-squarefree cubics/quartics do not always throw.

For (X - 1)^2 the claimed return value zero is correct. The discriminant exception at lines 128-129 applies
when the polynomial remaining after the removal of X factors has degree 3 or 4 and discriminant zero.
The squarefree-input promise at `refs/src/flint-3.0.1/fmpz_poly.rst:3264-3268` remains applicable.
This finding does not invalidate RR after its squarefree preprocessing.

### 3.11: a sufficient precision is called minimal

Take f = X^3 + 2X over Z_2. Its derivative at 0 is 2, so s = 1.
A nonzero root would satisfy X^2 = -2. Squares modulo 4 are 0 or 1, while -2 is 2 modulo 4.
There is no such root. Thus 0 is the only root in all of Z_2.
The ball Z_2, of precision 0, already isolates it. Even the ball 2 Z_2, of precision 1, does so.
Both precisions are below s + 1 = 2. The smallest precision allowed by R1 is 2; that is a different claim.

### Reference algorithm failure and interface contradictions

1. `real_roots_ref([-10**400, 1], 6)` raises OverflowError. Its initial bound uses Python `/`, converting
   the coefficient ratio to a float. The exact answer is the single integer 10^400.
   The failure is at `proto/solvers_checks.py:1747`. Repair R6 removes floating-point division.
2. The seed function states its condition and s using f, but the stored certificates refer to normalized g.
   For f = 27X, p = 3, a = 0, the stated seed succeeds with s = 3.
   The stored g = X has derivative valuation 0, so that certificate fails the proposed verifier.
3. The seed function returns complete = 0 without providing a partition of the remaining roots.
   The type requires nu = 0 exactly when complete = 1. No rule constructs its unresolved list.
   Give seed results a separate scope, or specify a covering unresolved partition.
4. The proposed root-list verifier expressly does not check completeness. With f = X(X - 1), p = 3,
   g = f, n = nu = 0 and complete = 1, all its listed prime-place checks pass vacuously.
   There are two roots. This is not a counterexample to P3.5; it shows that this verifier cannot certify
   the result type's completeness claim. Its name and a separate completeness-checking entry point must say so.
5. At the real place, the proposed entry checks also accept [0, 4] for (X - 1)(X - 2)(X - 3), with
   n = 1 and the correct count = 3. This interval contains three roots. The entry itself only promises
   at least one real root, whereas the type description promises one per ball.
   `real_cert_ok` additionally accepts that interval if supplied the false count 1.
   This is outside P3.8's correct-count hypothesis; a supplied count is not a trusted count.
6. The real API adds an accuracy promise absent from P3.10. RR may double a radius without checking the
   final accuracy. FLINT promises its own relative-accuracy measure, not a bound stated relative to an
   unknown exact root. Use the documented measure and check it after widening; see R8.
7. Reconstruction note 4 proposes finding a second solution by lowering B below the first denominator.
   For m = 2, c = 1, A = B = 1, both solutions have denominator 1. Lowering B to 0 loses both.
   Enumeration works; that proposed shortcut does not certify every NOT_UNIQUE result.
8. `_reconstruct_first` says count is written on every status but assigns it no meaning on NO_SOLUTION.
   The actual prototypes of `_get_particular` and `_get_dual` must return int, as their descriptions require.
   Copying the polynomial in `_get_poly` is not constant cost.

## Sources and the eight corrections of table 3

All source references here are files on disk. No external formula was supplied from memory.

| Correction in author report 5.1 | Verdict |
|---|---|
| 1. Thue inequality reversed | RIGHT. The strict inequality is n < r* t*. |
| 2. Thue gives a pair, not a reduced fraction | RIGHT conclusion; WRONG parameter claim for A = B = 2. |
| 3. fmpz_mod_mat is not limited to prime fields | RIGHT. Its Howell entry and wrapper admit composite moduli. |
| 4. nmod Howell need not have prime modulus | RIGHT. Its function entry and gcd-based code support this. |
| 5. A row denominator exceeding D proves absence | RIGHT under P1.9's stated row/argument hypotheses. |
| 6. Generic real count is not always Sturm | RIGHT distinction; WRONG blanket non-squarefree behavior. |
| 7. Storjohann uses a left kernel | RIGHT. Transposition is necessary for this design's column unknowns. |
| 8. arb_calc flags are not used here | RIGHT as a scope statement, not a correction of the quoted flag theorem. |

Correction 1 is supported by `refs/src/shoup-ntb/ntb-v2.txt:2184-2195`.
For correction 2, m = 8 and inclusive reconstruction bounds A = B = 2 require Thue parameters r* = t* = 3:
0 < 3 <= 8 < 9. The pair (-2, 2) meets those strict bounds and is not reduced.
The author's sentence that A = B = 2 itself satisfies Thue's hypothesis is false: 8 < 4 is false.
The already corrected table row also needs this distinction; under strict bounds 2 there is no such pair.
P1.8 applies the shift correctly.

Corrections 3 and 4 are supported by `refs/src/flint-3.0.1/fmpz_mod_mat.rst:278-293`,
`refs/src/flint-src-3.0.1/fmpz_mod_mat/howell_form.c:14-16`,
`refs/src/flint-3.0.1/nmod_mat.rst:705-723`, and the gcd operations at
`refs/src/flint-src-3.0.1/nmod_mat/strong_echelon_form.c:72-91`.
Correction 5 follows from P1.5 and Shoup's conditional denominator bound at
`refs/src/shoup-ntb/ntb-v2.txt:4189-4204`. It is not a blanket promise about every unsupported FLINT input.
Correction 6 is addressed above. Correction 7 is supported at
`refs/src/storjohann-thesis/diss2up.txt:2094-2120`.
For correction 8, `refs/src/flint-3.0.1/arb_calc.rst:120-138` permits undecided output intervals.
The old note's instruction to exclude endpoint and multiple roots from the input is unnecessary for a
completeness-status interface; such inputs may instead remain unresolved. The function is not used by RR.

Other source findings:

- Shoup's EEA identities, cost, uniqueness, group theorems, and root theorems match their uses:
  `refs/src/shoup-ntb/ntb-v2.txt:3641-3724`, `4159-4240`, `6360-6367`, `6626-6647`,
  `7283-7295`, and `8019-8064`. The denominator bound at 4202 assumes a bounded lattice point exists.
  Table 3's wording "in every case" must retain that hypothesis.
- Table 3 note 2 incorrectly treats 1/1 and -1/1 as an example of a common nonzero multiple of a pair.
  Their determinant is 2 and they are different ratios. They illustrate the equality boundary instead.
- Conrad's H1, H2, Taylor identity, and examples agree:
  `refs/src/conrad-hensel/hensel.txt:31-110`, `315-360`, `433-441`.
  Baker and Thorne have the stated different hypotheses at
  `refs/src/baker-padic/padicnotes.txt:572-582`, `662-672`, and
  `refs/src/thorne-padic/jackthornenotes.txt:567-580`.
- The missing polynomial Bezout source is already present:
  `refs/src/shoup-ntb/ntb-v2.txt:20510-20521` gives EEA over an arbitrary field.
  It covers both Q and F_p. L3.1 and P3.7 need no new fetched source.
- FLINT's real isolation, exact endpoints, and exact rational evaluation match P3.9(2)-(4):
  `refs/src/flint-3.0.1/arb_fmpz_poly.rst:66-105`, `arb.rst:461-466`,
  `fmpz_poly.rst:2277-2280`. Relative accuracy is defined at `arb.rst:499-509`.
- IVT and Sturm remain explicitly conditional dependencies. Their source-pending labels are honest.
  The optional resultant identity and the larger-prime nmod polynomial API are also still pending.

## Decisions S-D1 to S-D15

These are recommendations about a proposal. This review does not approve specification changes.

| Decision | Judgment | Recommendation and alternative |
|---|---|---|
| S-D1 | SOUND | Separate residue type; raw arguments and a factored partial-ball type are fair alternatives. |
| S-D2 | SOUND | Own EEA supplies both rows; the checked FLINT fast path is a fair alternative. |
| S-D3 | REPAIR | NOT_DETERMINED is defensible, but the limit-zero equivalence is false. |
| S-D4 | REPAIR | Candidate-returning function is useful; define count on every status. Strict-only is fair. |
| S-D5 | SOUND | Empty bounds give an empty set. DOMAIN is a fair alternative convention. |
| S-D6 | CONDITIONAL | The explicit report exception works. The OK-with-kind alternative is described unfairly. |
| S-D7 | SOUND | Own H and checked FLINT/HNF are valid choices; validate before consuming a fallback certificate. |
| S-D8 | CONDITIONAL | 4096 is an explicit size policy, not a measured safe memory or time budget. |
| S-D9 | SOUND | One modulus avoids factorisation. CRT may still save work despite a final canonicalisation. |
| S-D10 | CONDITIONAL | Exhaustive residues are sound. Threshold unbenchmarked; the alternative is fair. |
| S-D11 | SOUND | Trusting the documented squarefree count is explicit; own Sturm is a fair alternative. |
| S-D12 | CONDITIONAL | Deferring formats needs the stated exception; adf_resid is also an input type. |
| S-D13 | CONDITIONAL | Squarefree preprocessing changes the scope and requires consistent g certificates. |
| S-D14 | REPAIR | The cylinder accessor has its stated meaning; certificate-only is another immediate option. |
| S-D15 | SOUND | Partial partitions are useful and the strict-only alternative is fair. Distinguish seed results. |

S-D3: m = 2, c = 1, A = 2, B = 1, limit = 0 returns NOT_UNIQUE immediately.
Thus "with limit = 0 it is exactly 2AB >= m and no search" is not the algorithm's contract.
Use the full four conditions of P1.7(3). The proposed LIMIT alternative is also consistent if explicitly chosen.

S-D6: a solver returning OK with kind EMPTY is not inherently incorrect. OK means that the requested result
set was computed. The root API itself returns OK for an empty complete list. A separate non-solvability report
argument would also preserve CV-06 without treating the whole result as a report. State these tradeoffs neutrally.

S-D7: check dimensions and K1-K5 before greedy reduction consumes a certificate from the optional FLINT engine.
A check only after constructing the answer is too late to protect operations that assume valid pivots.
No such extra guard is mathematically needed for Algorithm H itself, whose invariants were proved.

S-D8 and S-D10 are policy choices. The review did not benchmark 4096-dimensional systems or primes near 2^20.
The S-D8 alternative is fair about FLINT allocation failure. The proposed dimension cap does not bound the
bit length of N, the coefficient storage, or the size of pending vectors.

S-D13: L3.1 proves the same distinct roots, including roots multiple in f. Calling them simple roots of g is
correct. It does not by itself authorize replacing SPEC's "multiple roots ... later" with "multiplicities ...
later". E-S4 records that required decision. Real roots can also be handled by squarefree factorisation while
retaining multiplicity information; UNSUPPORTED is a chosen restricted alternative, not a mathematical necessity.

Missing decisions/contracts:

- Seed scope and polynomial normalization: either add a seed result kind or specify its unresolved partition.
- Meaning of verification: distinguish checking listed certificates from verifying a complete result.
- Root resource limits and checked slong arithmetic for e + j, k + s and 2k - s.
  A valid slong precision can otherwise overflow an intermediate exponent in a literal C implementation.
- Final real accuracy after widening, and the exact status if that accuracy cannot be met.

## Proposed edits

| Edit | Verdict | Required change |
|---|---|---|
| E-S1 | REPAIR | State unknowns in (Z/N)^c or Zhat^c; ball outputs are preimages of modular cosets. |
| E-S2 | REPAIR | Certificates refer to normalized g under S-D13, not to f with multiple roots. |
| E-S3 | REPAIR | Test the squarefree g; include exact-point roots and pairwise disjointness. |
| E-S4 | CONDITIONAL | A scope change for the owner to approve; mathematically supported by L3.1. |
| E-S5 | REPAIR | State A >= 0 and B >= 1 before claiming A >= m always gives several. |
| E-S6 | ACCEPT | The limited search description is sound when read with P1.7. |
| E-P1 | REPAIR | The S.3 row must admit a limited unresolved search; not every call gives a complete answer. |
| E-P2 | REPAIR | Valid changed certificates may pass; finite residues are not individual p-adic roots. |
| E-C1 | ACCEPT | DOMAIN and UNSUPPORTED are needed for the stated functions. |
| E-C2 | CONDITIONAL | Correct if S-D6 is chosen; it is an explicit exception, not an existing permission. |
| E-C3 | CONDITIONAL | Correct if S-D12 is chosen; include all four listed types. |
| E-C4 | REPAIR | Include the fast cases and fewer-than-two-found condition, as in P1.7(3). |
| E-C5 | DEFER | Freeze repaired root-list invariants and complete prototypes before copying them. |

None of these edits was applied. No counterexample to the existing SPEC 9.1 or 9.2 was found.
The findings above concern the design, its reference implementation, its source descriptions, and proposed edits.

## Repairs

The following texts are ready to paste into the named locations. They are review proposals, not applied changes.

### R1. D2.1, replace the sentence about prescribed associates and its inheritance citation

For a nonzero representative a in [0, N), the prescribed associate is gcd(a, N).
For the zero residue the prescribed representative is 0. The inheritance formula is
`refs/src/storjohann-thesis/diss2up.txt:565-567`; the residue formula is at line 521.
The choices over Z are stated at line 481. These choices give E2 and E3 for nonzero pivots.

### R2. P2.11(2), replace the last two sentences

The row-count precondition is stated at `refs/src/flint-3.0.1/fmpz_mod_mat.rst:293` for that wrapper.
For a nonempty matrix the strong-echelon routine accesses (col, col) for each column, so nonempty inputs
with fewer rows than columns must be padded. Empty matrices return before this loop:
`refs/src/flint-src-3.0.1/fmpz_mat/strong_echelon_form_mod.c:167-168`.

### R3. Replace Remark 3.6 by this conditional proposition

Proposition 3.6 (an optional integer Bezout depth bound).
Let f be the polynomial after removal of its p-content. Suppose u and v are in Z[X], R is a nonzero integer,
and u f + v f' = R. Then Algorithm P is complete whenever D >= v_p(R) + 1.

Proof.
1. An opened class (a, e) with a child has w >= 2e by W.
2. Its constant coefficient gives v_p(f(a)) >= 2e. Differentiating the change of variable gives
   v_p(f'(a)) >= w - e >= e.
3. Since u(a) and v(a) are integers, evaluating the identity gives v_p(R) >= e.
4. No opened class at depth v_p(R) + 1 has a child. No unresolved class is therefore produced at that limit.

For nonconstant squarefree f, EEA in Q[X] gives u_0 f + v_0 f' = 1.
Multiplying by a common positive denominator gives u, v, R as above.
Source for polynomial EEA: `refs/src/shoup-ntb/ntb-v2.txt:20510-20521`.
A nonzero constant has no roots and needs no depth bound.
A specific bound using the resultant or the discriminant is not asserted here.

### R4. P3.9(1), replace the outside-hypothesis description

Outside the squarefree-input hypothesis there is no promised count of distinct roots.
The zero polynomial throws. The code strips X^i first and adds i to its answer, so X^d returns d.
If the remaining polynomial is a non-squarefree quadratic, its quadratic contribution is zero.
If the remaining polynomial has degree 3 or 4 and zero discriminant, the routine throws.
Read `refs/src/flint-src-3.0.1/fmpz_poly/num_real_roots.c:107-129` and `:155-159`.
Only a nonzero squarefree polynomial is passed to this routine by Algorithm RR.

### R5. Replace 3.11(5)

A Newton step from a certificate with derivative valuation s gives certified precision 2k - s.
The certificate definition requires k >= s + 1, which is sufficient for isolation.
A particular root may already be isolated in a ball of smaller precision.

### R6. Replace the real reference's floating-point bound construction

```python
B = 1
lead = abs(g[-1])
height = max(abs(c) for c in g[:-1])
while B * lead <= lead + height:
    B *= 2
```

This keeps the original exact bound and also handles coefficients larger than a machine float.
Add the regression f = X - 10^400 and require the single exact root 10^400.

### R7. Root-list normalization, seed scope, and verification

Under S-D13, define g to be the primitive integer squarefree part of f with positive leading coefficient.
Every seed condition, derivative valuation s, lifting step, and stored certificate refers to this g.
The seed function rejects f = 0 before constructing g.

Add a scope field with values PARTITION and SEED.
For PARTITION at a prime, the listed balls and unresolved classes cover every root; complete = 1 exactly
when nu = 0. For SEED, n = 1, nu = 0 and complete = 0. No coverage of the other roots is asserted.
All balls returned by either scope satisfy R1-R3 for g.

Name the existing local checker `adf_rootlist_verify_entries`.
It verifies only the listed prime certificates, or existence in the listed real intervals, and disjointness.
It does not certify a coverage flag or a count stored in the object.

Add `adf_rootlist_verify_complete(L, f, depth)`.
It returns 1 only if complete = 1 and completeness has been independently verified.
At a prime, rerun Algorithm P on g through depth, require no unresolved classes, and match every certified
root with exactly one listed ball by the intersection criterion of P3.2(4).
At the real place, recompute the squarefree root count, require n to equal it, and test P3.8 including
pairwise disjointness. Return 0 if verification fails or the supplied depth does not suffice.
SEED results do not pass this completeness check.

### R8. Real precision contract

The returned real balls have `arb_rel_accuracy_bits(ball) >= max(prec, 2)`, with exact roots allowed.
After any widening and after constructing the actual output balls, test this condition, the sign or
exact-point condition, pairwise disjointness, and equality with the independently computed count.
Return NOT_DETERMINED with the output untouched if any required condition fails.
Source for the accuracy measure: `refs/src/flint-3.0.1/arb.rst:499-509`.

For the reference test, measure the actual returned width instead of assuming that bits was used.
For the C test, measure the final stored ball, including any widening.

### R9. Reconstruction reports and limited-search wording

A NOT_UNIQUE result is certified by two distinct reduced pairs satisfying the original bounds and congruence,
or by repeating the complete enumeration. Lowering B below the first denominator is not a general method.
For `_reconstruct_first`, set count = 0 on NO_SOLUTION and NOT_DETERMINED; q is untouched on these statuses.
On OK, count is 1 for proved uniqueness, 2 for two found solutions, and 0 for a cut search with a candidate.

For a nonempty valid box, put ell = max(limit, 0).
NOT_DETERMINED occurs exactly when A < m <= 2AB, |T| <= B, floor(B/|T|) > ell, and the rounds with
x <= ell found fewer than two reduced pairs. The fast cases retain their proved statuses even when ell = 0.
Use this text in S-D3 and E-C4.

### R10. Replacement descriptions for proposed edits E-S1, E-S2, E-S3, E-S5, E-P1 and E-P2

E-S1, replacement second cell:

Exact integer matrices. For integer right-hand sides, unknowns are in (Z/N)^c with arbitrary N >= 1.
Return a particular solution and the Howell generators of the complete kernel, with the P2.6 certificate,
or a separating vector proving non-solvability. For positive-radius finite-ball right-hand sides, unknowns
are in Zhat^c and the output describes the inverse image of the modular solution coset of P2.9.

E-S2, replacement second cell, conditional on S-D13:

Distinct roots in Z_p of a nonzero integer polynomial. Certificates are for its normalized squarefree part g:
v_p(g(a)) >= k + s and v_p(g'(a)) = s < k. Each listed ball contains exactly one root of g and therefore of f.
A list is complete only when every search class has been resolved. A root modulo p need not lift;
X^2 + 1 at 2 is an example. Multiplicities are not reported.

E-S3, replacement second cell:

Isolating balls for the distinct real roots of a nonzero integer polynomial f.
Use its squarefree part g. Each ball is either an exact point where g vanishes or has opposite nonzero
signs of g at its exact endpoints. The balls are pairwise disjoint. The list is complete when its length
is the certified count of the real roots of g. Report the final accuracy as specified by the root API.

E-S5, insert before the statements about the bounds:

The following claims assume A >= 0 and B >= 1. If A < 0 or B < 1, the solution set is empty.

E-P1, replacement S.3 content:

Partial rational reconstruction from a residue, with certified one/none/several statuses and an explicit
limited-search NOT_DETERMINED result; complete classification when the search is allowed to finish.
Certificate pair and enumeration as in `proofs/solvers.md` section 1.

E-P2, replacement test description:

Compare every small modular solution set and kernel with direct enumeration. Check K1-K7 as applicable.
Changed certificates must be rejected when their asserted result or required canonical representation is false;
valid alternate certificates may pass. Check R1-R3 for each prime root certificate and independently enumerate
compatible roots modulo p^M: a simple root of derivative valuation s has p^s representatives at sufficient M.
Test root coverage, unresolved classes, real endpoint cases, real counts, and actual output accuracy separately.

## Checks

The independent Python checks import no author functions. They run the author's unchanged definitions in a
separate process and exchange JSON. Expected rational pairs, finite modules, and roots modulo powers are
computed independently. The C probe uses repeated addition to enumerate row spans.
All target suites reported this author-file SHA-256:

```text
09588ed7a576dc85971458fe5a53069c09b917347f3ba9bd6696507b75fd6b36
```

Each command below used `OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1`.
Raw outputs are in `lanes/s-review/`; each Python suite prints the hash above before the output shown.
A zero failure count means the review reproduced its expected outcomes, including the explicitly named findings.
It does not mean that the reviewed design has no defects.

Command: `timeout 170s python3 docs/reviews/s-design/review_checks.py recon`.
Output, exit 0:

```text
reconstruction: boxes=60480, calls=302400, boundary_boxes=2160, solutions=157476, empty_boxes=8400, failures=0
arbitrary_reconstruction_certificates: tried=1031943, accepted=1334, false_accepted=0
total: failures=0, findings=0, seconds=4.38
```

This enumerates every m in 1..18, c in -m..2m, A in 0..m+1 and B in 1..8.
Limits are -1, 0, 1, 3 and 100. The 2,160 boundary boxes have |2AB - m| <= 1.
Arbitrary certificate entries were enumerated for m <= 9, with B = 12.

Command: `timeout 170s python3 docs/reviews/s-design/review_checks.py matrix`.
Output, exit 0:

```text
matrix: matrices=17330, systems=1875272, vectors=6036862, certificates=1875272, failures=0
invented_scalar_certificates: tried=105106, accepted=237, false_accepted=0
total: failures=0, findings=0, seconds=164.23
```

For N in 1, 4, 6, 8, 9, 12, 16, 36, this includes every matrix of shapes 0x0, 0x1, 0x2,
1x0, 2x0, 1x1, 1x2 and 2x1, with every right-hand side.
It also includes every 2x2 matrix for N <= 9 from that list, with selected right-hand sides, and 30 samples
per modulus and shape 2x2, 2x3, 3x2, 3x1, 1x3. Kernel and coset comparisons use explicit finite sets.

Command: `timeout 170s python3 docs/reviews/s-design/review_checks.py extras`.
Output, exit 0:

```text
arbitrary_howell: matrices=32946, failures=0
adelic: systems=250, integer_points=3020, projections=184, failures=0
mod_p_count: polynomials=791, failures=0
total: failures=0, findings=0, seconds=6.66
```

The Howell cases exhaust two rows and two columns for N in 1, 4, 6, 8, 9, 12.
The finite-field cases exhaust nonzero leading coefficients and degree 0..5 over F_2 and F_3.
Ball-system cases include empty dimensions, noncanonical triples, and denominators sharing factors with radii.

Command: `timeout 170s python3 docs/reviews/s-design/review_checks.py roots`.
Output, exit 0:

```text
padic_p2: calls=1878, certificates=1212, residues=9422
padic_p3: calls=1878, certificates=1265, residues=155461
newton: calls=2477, failures=0
singular_children: nodes=14, failures=0
FINDING 3.11-minimum: f=X^3+2X, p=2: only root 0, s=1; Z_2 already isolates it (k=0 < 2)
FINDING seed-normalization: f=27X, p=3, a=0: seed s=3; stored g=X has derivative valuation 0
total: failures=0, findings=2, seconds=0.92
```

This exhausts nonzero polynomials of degree 0..3 with coefficients in -2..2, at depths 0, 1 and 2,
separately at 2 and 3. Extra cases include zero, content, divisible leading coefficient and higher s.
Newton extensions are checked by enumerating the possible extension digits.

Command: `timeout 170s python3 docs/reviews/s-design/review_checks.py certificates`.
Output, exit 0:

```text
invented_root_certificates: tried=29000, accepted=352, false_accepted=0
strong_hensel_boundary: equality_counterexamples=1, failures=0
bezout_depth: polynomials=48, failures=0
local_and_forget: reduced_pairs=24111, ball_rationals=5383, proper_witnesses=769, failures=0
total: failures=0, findings=0, seconds=0.39
```

This includes negative and excessive centres and s, equality k = s, and the explicit identity
-4(X^2 - c) + 2X(2X) = 4c for the optional depth bound. It also checks L1.2 and P1.10 directly.

Command: `timeout 170s python3 docs/reviews/s-design/review_checks.py real`.
Output, exit 0:

```text
real_planted: polynomials=59, exact_intervals=106, failures=0
FINDING reference-overflow: real_roots_ref([-10**400,1],6) raises OverflowError; exact answer is {10**400}
FINDING untrusted-real-count: real_cert_ok((X-1)(X-2)(X-3),1,[[0,4]]) accepts a 3-root interval
real_endpoints: positive_width_endpoint_rejected=1, exact_root_point_accepted=1
total: failures=0, findings=2, seconds=0.8
```

The planted rational roots include repeated roots and roots separated by 2^-40 at requested precision 6.
Their positions are known exactly, without using a Sturm count as the oracle.

Commands:

```sh
cc -O2 -Wall -Wextra docs/reviews/s-design/checks/flint_probe.c \
  -o lanes/s-review/flint_probe -lflint -lgmp -lmpfr
timeout 170s lanes/s-review/flint_probe
```

Output, both commands exit 0:

```text
FLINT=3.0.1
flint_howell_hnf: matrices=2560, engines=3, differences=0
flint_reconstruct_large: planted=712, bit_sizes=8, differences=0
FINDING flint_nonsquarefree: f=X^2, count=2, throws=0
FINDING flint_nonsquarefree: f=X^3, count=3, throws=0
FINDING flint_nonsquarefree: f=X^4, count=4, throws=0
flint_real_planted: polynomials=60, exact_balls=10, differences=0
total: failures=0
```

The reconstruction moduli have bit parameters 63, 64, 65, 127, 128, 129, 255 and 1024.
These are planted solutions inside the documented uniqueness range, not an exhaustive multi-limb proof.
The real inputs include repeated factors before preprocessing, positive quadratic factors without real roots,
and roots spaced by 1/1024. The probes use the installed FLINT 3.0.1, not python-flint's bundled library.
The first compile failed because the review probe used a nonexistent fmpz_poly_divexact symbol.
It was replaced by the documented fmpz_poly_divides; no design code was involved.

Command: `timeout 130s python3 docs/reviews/s-design/checks/mutation_checks.py`.
Output, exit 0:

```text
limit_discards_two_found_solutions: exit=0
PASS check_s3_limit: 239540 calls with limits 0, 1, 2, 5; NOT_DETERMINED 148555 times; 0 failures
changed_statuses: 51291
witness: ('NOT_DETERMINED', [], (2, 0, 1, 1))
real_ignores_requested_precision: exit=0
PASS check_s2_real_completeness: 18 polynomials, 42 changed lists; 0 failures
precision_violations_x2_minus_2: 2
total: surviving_mutants=2, reproduction_failures=0
```

Command: `timeout 35s python3 docs/reviews/s-design/checks/repair_checks.py`.
Output, exit 0:

```text
integer_bound_repair: huge_linear_polynomials=4, failures=0
RR_widening_contract: prec=8, input_radius=3/1024, output_radius=3/512, promised_max=1/256
total: repair_cases=4, widening_counterexamples=1, failures=0
```

The widening example is a permissible incoming enclosure for X - 1 with root at its left endpoint.
Its midpoint is 1 + 3/1024; its radius has top-bit position -9, so its incoming relative accuracy is 8 bits.
RR doubles the radius to 3/512, keeps a sign change and a complete one-root list, but exceeds 1/256.
This is a contract counterexample to deriving final accuracy from the incoming enclosure promise.
It is not an observed FLINT output for X - 1.

### Judgment of the author's checks

Command: `timeout 170s python3 proto/solvers_checks.py`, with the same one-thread environment.
Exit 0; 25 checks; reported total 19.1 seconds; failures 0.
The complete output is `lanes/s-review/author_checks_output.txt`.

The weakest contract check is `check_s3_limit`. It tests only necessary conditions for NOT_DETERMINED.
It never independently counts the reduced pairs already visited in the permitted rounds.
The surviving mutant above changes 51,291 statuses and still passes.
For m = 2, c = 1, A = 1, B = 3, limit = 1, the first round finds 1/1 and -1/1.
The required status is NOT_UNIQUE; the mutant's NOT_DETERMINED is a direct violation of P1.7(3).

Other limitations:

- `check_s3_complete` does not exhaust all A through 2m, despite the report's wording.
  It uses 0..min(m, 9)-1 plus m-1, m, m+1 and 2m.
- `check_s3_edge` increments its large-random count before skipping noninvertible denominators.
  The printed 3,000 is a count of attempts, not 3,000 verified reconstructions.
- `check_s2_descent` catches an oversized finite-root oracle and skips that comparison without printing a count.
- `check_s2_real_completeness` uses the same Sturm routine for construction and several checks.
  Its optional mpmath cross-check catches every exception and may disappear. Its claimed gap checks are absent.
  It does not test the requested width; the second surviving mutant demonstrates that omission.
- The FLINT probes are bounded comparisons, not proofs of the large internal paths.
  In particular the original real probe excludes large coefficients and cannot expose the float-bound failure.
- Mutation requirements must distinguish false result sets from different valid certificates.
  The author's own run accepted 1,831 changed certificates whose claims were true.

Command: `python3 docs/reviews/s-design/checks/audit_artifacts.py`.
Output, exit 0:

```text
artifacts: files=7, python_syntax=4, overlong_lines=0, reports=2
verdicts: VALID=28, MINOR=3, INVALID=2, total=33
author_check_hash: unchanged=1
total: failures=0
```

## Not examined and sources pending

All 33 statements and all 15 decisions were read. The primary source passages used by the proofs were checked.
The one-limb reconstruction path, Howell sources, real count sources, and real isolation source were read.
The roughly 600 lines of specialized multi-limb reconstruction internals were not proved line by line.
Neither were FLINT's internal complex-root iteration and pseudo-remainder arithmetic proved from first principles.

Exhaustion does not cover all 2x2 matrices at N = 16 or 36, all higher-dimensional matrices, all right-hand
sides for the sampled larger shapes, all coefficients, all primes, or all precisions.
The certificates and mathematical proofs, rather than these finite bounds, must supply the general result.
No C implementation of the proposed public API exists in this review. No ABI, allocation, aliasing,
threading, or high-dimension performance test of that future implementation was possible.
The two surviving mutants are targeted demonstrations, not a full mutation campaign.

Sources pending:

- [source pending: a local statement/proof of IVT for the real completeness argument]
- [source pending: Sturm's theorem if it is to replace or justify FLINT's trusted count internally]
- [source pending: the integral resultant Bezout identity if that optional sentence is retained]
- [source pending: FLINT 3.0.1 nmod polynomial documentation for the larger-prime alternative of S-D10]

The Q[X] and F_p[X] Bezout source is no longer pending; it is Shoup at the location given above.
No finding against the existing specification was established.
