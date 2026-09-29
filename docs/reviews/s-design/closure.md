# Closure review of milestone S design
Original statements: CLOSED 32, CLOSED WITH EDIT 1, OPEN 0. New items: VALID 13, MINOR 1, INVALID 0. NOT READY

The design reviewed here is `docs/proofs/solvers.md`, `proto/solvers_checks.py`, `docs/api-s.md`, and table 3 of
`docs/sources.md`. The verdict is for the design, not an implementation. The recommended S-D13 scope change
still needs an owner decision, and the SPEC, PLAN, and conventions edits in `api-s.md` remain proposals.
The three text edits and the real accuracy regression below are needed before the design is frozen. The further
findings R11-R14 are all MINOR.

## Findings of the first review: statements

The review's 33 rows are listed individually. A case count is finite evidence; the proof remains the basis for a
general statement. The reference checks were run again on the repaired file.

| Finding | Review | Closure | Evidence |
|---|---|---|---|
| D1.1 | VALID | CLOSED | Definition unchanged; reconstruction suite: 60,480 boxes, 0 failures. |
| L1.2 | VALID | CLOSED | Proof unchanged; reconstruction and certificates suites: 0 failures. |
| D1.3 | VALID | CLOSED | Certificate conditions unchanged; 1,033,943 arbitrary pairs, 0 false accepted. |
| L1.4 | VALID | CLOSED | EEA proof unchanged; same arbitrary-pair run. |
| P1.5 | VALID | CLOSED | Bijection proof unchanged; 177,120 boxes in author check, 0 failures. |
| P1.6 | VALID | CLOSED | Fast cases and enumeration unchanged; 366,208 complete problems, 0 failures. |
| P1.7 | VALID | CLOSED | Claim 3 now uses ell=max(limit,0); 329,875 limited calls, 0 failures. |
| P1.8 | VALID | CLOSED | Strict Thue bounds and shift unchanged; source lines 2184-2195 checked. |
| P1.9 | VALID | CLOSED | Documented and one-limb claims unchanged; multi-limb proof still excluded. |
| P1.10 | VALID | CLOSED | Forgetting proof unchanged; 4,628 ball rationals, 0 failures. |
| D2.1 | MINOR | CLOSED | Zero associate is 0; lines 481, 521, 565-567 and 1,830 residues checked. |
| L2.2 | VALID | CLOSED | Proof unchanged; 1,869 Howell row sets, 0 failures. |
| L2.3 | VALID | CLOSED | Proof unchanged; 1,869 Howell row sets, 0 failures. |
| P2.4 | VALID | CLOSED | Proof unchanged; 32,945 row pairs, 0 failures. |
| P2.5 | VALID | CLOSED | Pending-row invariant unchanged; author Howell checks: 0 failures. |
| P2.6 | VALID | CLOSED | K1-K7 unchanged; 8,142 false certificates, full checker accepted 0. |
| P2.7 | VALID | CLOSED | Separating-vector proof unchanged; 420 submodules, 0 failures. |
| P2.8 | VALID | CLOSED | Solver proof unchanged; 8,664 systems in author check, 0 failures. |
| P2.9 | VALID | CLOSED | Ball preimage proof unchanged; 393 systems, 0 failures. |
| P2.10 | VALID | CLOSED | Coordinate projection proof unchanged; same 393 systems. |
| P2.11 | MINOR | CLOSED | Padding now says nonempty; 28 empty FLINT matrices, 0 failures; doc line 293. |
| P2.12 | VALID | CLOSED | HNF proof unchanged; 2,016 probes, 0 differences. |
| L3.1 | VALID | CLOSED | Old parts unchanged; polynomial EEA source added at Shoup lines 20462, 20510-20521. |
| P3.2 | VALID | CLOSED | Old claims stand; 454 more certificates truncated at every allowed k', 0 failures. |
| P3.3 | VALID | CLOSED | Newton proof unchanged; 500 steps and requests, 0 failures. |
| P3.4 | VALID | CLOSED | Change-of-variable proof unchanged; 123 of 124 descent runs compared. |
| P3.5 | VALID | CLOSED | W and termination proof unchanged; 123 compared, one oracle overflow named. |
| R3.6 | MINOR | CLOSED | Now P3.6; four proof steps close the integer Bezout bound; 211 cases. |
| P3.7 | VALID | CLOSED | Polynomial EEA source added; 468 finite-field polynomials, 0 failures. |
| P3.8 | VALID | CLOSED | Correct-count hypothesis explicit; stored count is disclaimed. |
| P3.9 | INVALID | CLOSED | Source now says X^d returns d; FLINT gave 2, 3, 4 for X^2, X^3, X^4. |
| P3.10 | VALID | CLOSED | Final accuracy is tested on output balls; 60 FLINT calls, 0 failures. |
| 3.11 | INVALID | CLOSED WITH EDIT | Minimum removed; new valuation formula is false. R11. |

## New statements and decisions

| New item | Verdict | Evidence |
|---|---|---|
| P1.11 | VALID | Each accepted status follows P1.6 or P1.7; 93,408 results and 2,021,284 changes checked. |
| L3.1(3) | VALID | Scaling by a nonzero rational preserves roots; primitive positive normalisation is unique. |
| P3.2(2) | VALID | Taylor terms have valuations at least k'+s, k'+s, 2k'; 454 truncations checked. |
| P3.2(6) | VALID | X^2+3 at 2 has v(f(1))=2 and v(f'(1))=1, but no root modulo 8. |
| P3.6 | VALID | W gives v_p(f(a))>=2e and v_p(f'(a))>=e; Bezout gives e<=v_p(R). |
| P3.10 step 6 | VALID | Exact signs, disjointness, count, and final arb accuracy are tested. |
| P3.10 claim 4 | VALID | It follows from step 6; source for the measure is arb.rst:499-509. |
| P3.12 | VALID | Strong Hensel on g* gives the seed root; 5 neighbouring seed cases checked. |
| P3.13 | VALID | Prime matching uses P3.2(4); real completion uses a recomputed count. |
| P1.7 claim 3 | VALID | Four conditions follow the control flow; 51,291 cut searches found two roots. |
| S-D16 | VALID | SEED scope asserts one root and no coverage; f=0 is refused. |
| S-D17 | VALID | Entry and complete checks have separate claims; empty false-complete list is refused. |
| S-D18 | MINOR | The stated 2K bound misses a seed temporary with k0 much larger than K. R13. |
| S-D19 | VALID | Final accuracy is an output condition; NOT_DETERMINED leaves output untouched. |

P3.6 uses polynomial EEA over Q. Shoup's theorem and algorithm are at
`refs/src/shoup-ntb/ntb-v2.txt:20462-20470,20510-20521`. P3.12 uses the strong Hensel statement at
`refs/src/conrad-hensel/hensel.txt:315-319`. The FLINT accuracy definition is at
`refs/src/flint-3.0.1/arb.rst:499-509`. These are the source statements used above; the deductions are in
the cited proofs.

## Reference failures and interface contradictions

| Review item | Closure | Recheck |
|---|---|---|
| 1. Float overflow | CLOSED | Six 400-digit cases with other signs and coefficient positions pass. |
| 2. Seed on f, certificate on g | CLOSED | Content at 2 and 3 and a negative lead give g* certificates. |
| 3. Seed with no partition | CLOSED | Scope SEED says no coverage; complete verifier refuses it. |
| 4. Empty prime list accepted | CLOSED | Entries accepts; complete refuses X(X-1) and a dropped certificate. |
| 5. Real count trusted | CLOSED | Entries accepts [0,4]; complete recounts three roots and refuses it. |
| 6. Real accuracy after widening | CLOSED | Final balls are tested; the widening witness returns NOT_DETERMINED. |
| 7. Lowering B | CLOSED | P1.11 uses two pairs or full enumeration; m=2,c=1,A=B=1 passes. |
| 8. Count, prototypes, copy cost | CLOSED | Count 0/1/2 is defined; getters return int; get_poly says O(deg g). |

The repaired reference bound uses only integer multiplication and comparison. It handled coefficients of 400
digits as constants, leading coefficients, negative coefficients, and coefficients in a quadratic. The prime
seed checks include 8X at 2, 27X at 3, -8(X-1) at 2, and -9X at 3. These probes attack the adjacent inputs,
not just the review's X-10^400 and 27X cases.

## Table 3, decisions, and proposed edits

| Table-3 correction | Closure | Evidence in the repaired row or note |
|---|---|---|
| 1. Thue inequality | CLOSED | Modulus below strict bound product; inclusive bounds shifted by 1. |
| 2. Pair, not fraction | CLOSED | (-2,2) is nonreduced; A=B=2 has no solution in the example. |
| 3. fmpz_mod_mat | CLOSED | Howell entry admits composite moduli; solving entries retain prime condition. |
| 4. nmod Howell | CLOSED | Entry has no prime condition; source uses gcd against the modulus. |
| 5. Denominator above D | CLOSED | Note 3 includes P1.9 arguments and Shoup's bounded-point hypothesis. |
| 6. Generic real count | CLOSED | Formula branches and X^i stripping described; Sturm not claimed universally. |
| 7. Left kernel | CLOSED | Storjohann's left kernel is transposed for column unknowns. |
| 8. arb_calc | CLOSED | Flag rule retained; function explicitly outside RR. |

The two other source findings close: note 2 treats 1/1 and -1/1 as different equality-boundary ratios,
and the Shoup 4.9 row states the bounded-point hypothesis. The quoted passages were checked at
`refs/src/shoup-ntb/ntb-v2.txt:2184-2195,4198-4204`,
`refs/src/flint-3.0.1/fmpz_mod_mat.rst:278-293`,
`refs/src/flint-3.0.1/nmod_mat.rst:705-723`,
`refs/src/flint-src-3.0.1/fmpz_poly/num_real_roots.c:107-129,155-159`, and
`refs/src/storjohann-thesis/diss2up.txt:2094-2120`. The quote checker found 57 rows and 0 failures.

| Original decision | Closure | Reason |
|---|---|---|
| S-D1 | CLOSED | Separate residue type and alternatives are stated. |
| S-D2 | CLOSED | Own EEA and checked FLINT fast path are stated. |
| S-D3 | CLOSED | Four limit conditions and the fast cases are stated. |
| S-D4 | CLOSED | Count 0/1/2 has a meaning on every status. |
| S-D5 | CLOSED | Empty bounds and DOMAIN alternative are stated. |
| S-D6 | CLOSED | OK-with-kind and separate-report alternatives are now stated fairly. |
| S-D7 | CLOSED | K1-K5 are checked before a FLINT certificate is used. |
| S-D8 | CLOSED | 4096 is described as policy, not a proved resource bound. |
| S-D9 | CLOSED | CRT alternative does not deny possible savings. |
| S-D10 | CLOSED | Prime limit is described as unbenchmarked policy. |
| S-D11 | CLOSED | Count trust and own-Sturm alternative are explicit. |
| S-D12 | CLOSED | Four types, including the input type adf_resid, are named. |
| S-D13 | CLOSED WITH EDIT | The `reduced` flag contradicts itself for f=2X. R12. |
| S-D14 | CLOSED | The cylinder meaning and certificate-only alternative are explicit. |
| S-D15 | CLOSED | Partial lists and strict alternative have separate statuses. |

| Proposed edit | Closure | Evidence or condition |
|---|---|---|
| E-S1 | CLOSED | Unknowns and ball preimage are in the proposed SPEC text. |
| E-S2 | CLOSED | Prime certificates refer to g*; conditional on S-D13. |
| E-S3 | CLOSED | Exact roots, opposite signs, and disjointness are in the proposed text. |
| E-S4 | OPEN | Owner must decide if SPEC's "multiple roots later" means multiplicities later. |
| E-S5 | CLOSED | A>=0 and B>=1 precede the A>=m claim. |
| E-S6 | CLOSED | The accepted limited-search wording is retained. |
| E-P1 | CLOSED | S.3 admits a limited unresolved search. |
| E-P2 | CLOSED | Alternate valid certificates and p^s residues are distinguished. |
| E-C1 | CLOSED | DOMAIN and UNSUPPORTED are in the proposed status row. |
| E-C2 | CLOSED | Explicit report exception is conditional on S-D6. |
| E-C3 | CLOSED | All four types are named; conditional on S-D12. |
| E-C4 | CLOSED | Fast cases and fewer-than-two-found condition are present. |
| E-C5 | OPEN | Convention subsections and C prototypes remain deferred. |

These are verdicts on the proposals in `docs/api-s.md` section 6. The edits have not been applied to SPEC,
PLAN, or conventions, which were read-only in this lane. E-S4 is an owner scope decision: the SPEC still says
multiple roots are later, while S-D13 offers their distinct roots after squarefree preprocessing.

## Findings requiring edits

### R11 (MINOR): 3.11(5), derivative valuation in the new isolation formula

The new sentence says s_i = sum of v_p(r_i-r_j) for every polynomial whose roots in Z_p are the distinct
integers r_i. Take f = X(X^2+2X+2) at p=2. The quadratic is 2 or 1 modulo 4 at every integer, so it has
no root in Z_2. The only root is 0, while v_2(f'(0))=v_2(2)=1. The sum over other integer roots is 0.
The author's planted-root check only uses monic split products and cannot test this sentence.

In `docs/proofs/solvers.md` 3.11(5), replace the sentence beginning "In general, for a polynomial with
distinct integer roots" by:

> If the roots of f in Z_p are exactly the distinct integers r_i, the least isolating precision of r_i is 0
> for a single root and max_(j != i) v_p(r_i-r_j)+1 otherwise. It is at most s_i+1, where
> s_i = v_p(g*'(r_i)). If g* = product_i (X-r_i), then s_i = sum_(j != i) v_p(r_i-r_j).

The first formula counts when two integer roots enter the same ball. For the bound, g*(r_i)=0 and its
derivative has valuation s_i, so strong Hensel isolates r_i in r_i+p^(s_i+1) Z_p. The last identity follows
by differentiating the stated product. No leading-coefficient or nonsplit factor is omitted.

### R12 (MINOR): `reduced` has two meanings

In `docs/api-s.md` section 4, the field says `reduced=1` if g is a proper divisor of f up to a constant,
"that is" if f had a multiple factor or content. For f=2X, g*=X. It is equal to f up to a constant,
but f has content 2. S-D13 then says `reduced=1` means multiplicities were dropped, though f has no
multiple root. This is an interface contradiction, not a counterexample to L3.1.

Replace the `reduced` field's Meaning cell by:

> 1 iff f has a repeated irreducible factor over Q, equivalently deg gcd(f,f') > 0 for nonconstant f.
> Multiplication by a nonzero constant and removal of content alone do not set this flag. Root
> multiplicities are never returned.

In S-D13, replace "`reduced = 1` says that multiplicities were dropped" by
"`reduced = 1` says that the input had a repeated factor; multiplicities are never returned".

### R13 (MINOR): S-D18 misses a seed exponent

For g=X, p=2, a=2^5000, prec_p=1, Proposition 3.12 forms k0=v_p(g(a))-s=5000 and K=1.
It then says to reduce a modulo p^k0 before reducing to K. S-D18 checks the size of p^(2K)=4 but not
this temporary p^5000. The reference uses Python integers; a C implementation following the prose can
allocate beyond `ADF_ROOTS_BITS_MAX` despite the stated guard.

After the first sentence of the Recommended cell of S-D18 in `docs/api-s.md`, insert:

> The seed path also checks k0 = v_p(g(a))-s before materialising p^k0. If k0 >= K, it constructs
> (a mod p^K, K, s) directly, without constructing p^k0. Every other p-power is checked against
> ADF_ROOTS_BITS_MAX before allocation.

The direct certificate follows from the same Taylor argument as P3.12(2): v_p(g(a)) >= K+s and K>s.

### R14 (MINOR): accuracy test gap

| First review's check limitation | Closure | Evidence |
|---|---|---|
| Limit check tests only necessary conditions | CLOSED | Four conditions and recounted pairs kill the old mutant. |
| Complete check omits A values | CLOSED | A=0..2m is run for all m<=36: 366,208 problems. |
| Edge count counts attempts | CLOSED | 3,000 verified cases are printed. |
| Descent skips silently | CLOSED | One skip among 124 runs is named. |
| Real completeness check is weak | CLOSED WITH EDIT | Old mutant killed; finisher gap below. |
| FLINT probes are bounded | CLOSED | Bounds are stated; 60 real calls include 400-digit coefficients. |
| Changed certificate may be valid | CLOSED | 8,142 false certificates; each K1-K7 omission accepts false ones. |

One of three independent mutants survived the current new checks. Removing `real_accuracy_ok(out, prec)`
from `rr_finish` survives all 60 FLINT calls because none widens a ball. On f=X-1 at prec=8, feed the
finisher [1, 1+6/1024]. The endpoint test fails, widening gives a sign change but 7 bits of accuracy.
The correct function returns NOT_DETERMINED; the mutant returns OK. Add this exact regression to
`probe_s2_flint_real` or a separate `check_s2_rr_finish`:

```python
incoming = [(F(1), F(1) + F(6, 1024))]
assert rr_finish([-1, 1], incoming, 8)[0] == NOT_DETERMINED
```

This test is for a legal engine enclosure, not a claim that FLINT produced that enclosure in 60 probes.

## Checks run

All commands used `OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1` where Python did numerical work.

| Command | Result |
|---|---|
| `timeout 170s python3 proto/solvers_checks.py` | Exit 0; 34 PASS, 0 FAIL; 36.8 seconds. |
| `timeout 170s python3 docs/reviews/s-design/closure-checks/closure_probe.py` | Exit 0; 2 killed, 1 survived. |
| `python3 lanes/s-design-repair/run_reviewer_mutants.py` | Exit 0; 2 of 2 reviewer mutants killed. |
| `python3 lanes/s-design-repair/red_checks.py proto/solvers_checks.py` | Exit 0; 0 of 9 red probes failing. |
| `python3 lanes/s-sources/check_quotes.py` | Exit 0; 57 rows, 9,259 quoted characters, 0 failures. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py recon` | Exit 0; 60,480 boxes, 0 failures. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py certificates` | Exit 0; 29,000 triples. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py extras` | Exit 0; 32,946 matrices, 0 failures. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py roots` | Exit 0; 2 stale FINDING lines, 0 failures. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py real` | Exit 1; stale overflow check. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py matrix` | Exit 142; SIGALRM at 165 s. |

The `real` suite's second FINDING is stale as a verdict: `real_cert_ok` still takes a trusted count by design,
and `real_verify_complete` recomputes it. The `roots` suite prints its two FINDING lines unconditionally
after the seed and isolation repairs. The `matrix` suite has `signal.alarm(165)` at line 564 and exited 142,
as the repair report predicted. Its comparison workload was unchanged; the report's 215.5-second scratch run
exceeds the three-minute lane limit.
The first two development runs of `closure_probe.py` exited 1 before any mutant ran: one bad source path,
then an overbroad textual match in the mutation harness. The final command above uses the corrected harness.

## What was not examined

No C implementation of the proposed API exists here; ABI, allocation failure, aliasing, and threading were
not tested. The reviewer's matrix suite was limited by its alarm. The internal multi-limb reconstruction
path and FLINT complex-root arithmetic were not proved. The finite probes do not cover all coefficients,
primes, precisions, or high-dimensional matrices. The optional resultant bound and large-prime nmod_poly
path remain outside the proved design.

Sources pending: a local IVT proof for the real-completeness argument; Sturm's theorem if an own Sturm
counter replaces the documented FLINT count; an integral resultant Bezout identity if that optional claim
is restored; and FLINT 3.0.1 nmod polynomial documentation for S-D10's larger-prime alternative.

Findings against the specification: none. The unresolved S-D13 scope question concerns whether the design's
squarefree preprocessing is adopted as the interpretation of SPEC 9.1; it is not a proof that SPEC 9.1 is false.
