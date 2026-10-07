# Lane d-functions4

Completed the design and executable oracle. The final required command exited 0 with 2670 checks:

```sh
timeout 120 python3 proto/functions4_checks.py
```

The design has 457 lines, below the 500-line limit. Its lines and the oracle's lines are at most 116 characters.
Python-flint is 0.8.0, using FLINT 3.3.1. The source contracts were read under refs/src/flint-3.0.1.
The ball checks do not claim to execute the future C implementation or FLINT 3.0.1.

## Interface in twenty lines

1. ffun keeps the fixed D,M,acb-array layout and DM<2^62 predicate; init is (1,1,[0]).
2. rfun keeps ordered polynomial-Gaussian terms; init is len=0; Re(A)>0 must be certified.
3. Lifecycle, canonical predicates, representation identity and exported layout queries are specified.
4. Raw setters copy arrays/terms; numerical failures preserve every output.
5. Typed readers/printers and dump/load/inspect declarations use the already fixed grammars.
6. Refinement uses lcm in both dimensions, zero extension and repetition, with charged work.
7. Addition/product operate on the common refinement; duplicated balls can lose correlations.
8. Translation means f(x-q); after denominator refinement it reads index k-Dq.
9. Rational dilation means f(qx), with support holes and negative signs handled exactly.
10. Idele dilation requires a singleton unit image modulo DM; uncertain real dilation retains balls.
11. Finite Fourier uses E(-jk/DM)/M, swaps D,M and squares to reflection.
12. Its acb error statement includes coefficient radii, phase errors and every rounding step.
13. Real closure retains A,B,C as parameters; products, derivatives and conjugates are included.
14. Real Fourier gives explicit new parameters and polynomial, with the root positive on positive A.
15. Finite evaluation enumerates every coset met and includes zero when the ball leaves support.
16. Partial-place evaluation is proposed on all adelic completions; missing primes have no default.
17. Haar integrals are closed formulas; L2 squared norms include every real cross term.
18. Poisson computes its two lattice sums independently and reports their separate cutoffs.
19. Tail prefixes, precision retries and final width certificates are bounded; input radii can obstruct width.
20. Tensor pairs and lists suffice for milestone 5's test vectors; seven callable implementation slices are mapped.

## Decisions

- D1: use the integrals/Poisson status row explicitly for test-function algebra; accept the stated work caps.
  Proposed array cap 2^20, term/coefficient caps 2^16, work cap 2^20; direct Fourier also requires L^2<=2^20.
  Exact integer work is bounded at 2^20 bits, and numerical precision at the existing 2^21 bits.
- D2: paired factor calls and caller-owned lists; no owning adf_afun value type in this milestone.
- D3: the new sball evaluator encloses all adelic completions, with a whole-real-axis bound for NONE.

The root branch and both dump bodies are already fixed. They are not questions for TJO.

## Findings against the specification

No counterexample was found to SPEC 7's formulas, PLAN's exact-input tests, or the 37 fixed text goldens.

Conventions 3.2 does not explicitly classify test-function algebra. D1 proposes its row assignment.
The valid layout (D,M)=(1025,1) already exceeds the proposed direct-transform work bound, requiring LIMIT;
the generic ring-arithmetic row would supply no status. The oracle checks 1024^2<=2^20<1025^2.

Analysis P15:726-728 needs a qualification if interpreted as promising arbitrary final output width on
fixed parameter balls. For phi=c exp(-pi x^2), c in [1,2], and f_fin=1_Zhat, the Poisson value width is
sum_n exp(-pi n^2)=1.08643481121330801..., independent of working precision. The additive integral width is 1.
The oracle computes the theta witness. This is not a counterexample to P15's truncation formulas:
truncation and discretization can shrink while input uncertainty remains. The design returns
NOT_DETERMINED if the requested final width cannot be certified within the precision cap.

The brief's rational nonzero-support description needs containment rather than equality. A delta at
index 1 for (D,M)=(2,3) occupies one class, not all of (1/2) Z. No support minimality is assumed.

## Checks and rejection criteria

All invocations, including the dependency and formatting probes, are recorded in
[check-commands.md](check-commands.md). Final output is [oracle-final.log](oracle-final.log).

| Group | Count | A check fails if |
|---|---:|---|
| finite_algebra | 1044 | A refined, translated, dilated, added or multiplied cell differs from rational evaluation |
| finite_transform | 80 | A transform differs by more than the numerical margin, or reflection/Parseval fails |
| dilation_covariance | 440 | Transform/dilation layouts or values disagree, or finite integral scaling is wrong |
| idele_indices | 325 | Unit images differ from exact lifts or nontrivial unit dilation selects a wrong cell |
| evaluation | 605 | An exact coset index is missed/added or the outside-support flag is wrong |
| real_closure | 11 | Expanded polynomial or exponent identities differ exactly |
| real_transform | 23 | Quadrature, reflection, odd sign or the square-root reciprocal identity fails |
| integral_norm | 5 | Closed integrals or the conjugate-product norm differ from independent quadrature |
| goldens | 39 | Any of 37 expected texts/statuses differs, or either row count differs |
| golden_transforms | 24 | Golden-derived coefficient corners disagree with the ball transform |
| poisson | 12 | Independent sums fail their tail allowances or any searched cutoff misses its target |
| tails | 14 | A sampled omitted absolute sum exceeds its bound or the slow-prefix limit is not recognized |
| findings | 1 | The fixed-input theta width witness is not greater than 1 |
| precision | 8 | Finite transform sign/radius bounds fail or radii fail to decrease |
| certified_poisson | 8 | The two ball sums fail overlap/width tests or their widths fail to decrease |
| faults_41 through faults_45 | 30 | Any of the six distinguishing witnesses per work package is lost |
| resource_examples | 1 | The direct-transform cap boundary inequality fails |
| Total | 2670 | Any assertion fails |

Numerical quadrature uses 60 decimal digits and margin 1e-45 times max(1, absolute reference value).
It is not a certificate. Finite transforms and the separate certified_poisson group use acb/arb enclosures.
Poisson numerical cutoff pairs for targets 2^-30,2^-80,2^-120 were (1,8),(2,16),(2,16).
Certified Poisson ran at 80,144,212 bits with width targets 2^-30,2^-70,2^-110, respectively.
Thirty mathematical fault witnesses ran. Zero C mutations, zero C ABI/status tests and zero fuzz runs ran.

Earlier runs are retained, not hidden:

| Command, before the recorded edits | Exit | Result/log |
|---|---:|---|
| timeout 120 python3 -B proto/functions4_checks.py | 1 | first: assertion 604 in evaluation failed |
| timeout 120 python3 -B proto/functions4_checks.py | 124 | second: timeout after 120 seconds |
| timeout 120 python3 -B -u proto/functions4_checks.py | 0 | third: 2067 checks |
| timeout 120 python3 -B -u proto/functions4_checks.py | 0 | fourth: 2099 checks |
| timeout 120 python3 -u proto/functions4_checks.py | 0 | fifth: 2100 checks |
| timeout 120 python3 -u proto/functions4_checks.py | 0 | sixth: 2230 checks |
| timeout 120 python3 proto/functions4_checks.py | 0 | final: 2670 checks |

The first failure was my expected partial-place set: it omitted index 5 from {2,5}. The corrected set follows
j/2=1 mod 3. The timeout exposed the near-zero-A ratio-prefix cost. The oracle now preflights its 4096-step
prefix budget, and proves cutoff 4096 insufficient by checking the first omitted term at n=4097.
The design charges the prefix and returns LIMIT. No mathematical tail or tolerance was weakened.

The dependency probe exited 0 with three installed versions. Four earlier layout/syntax probes exited 0;
the first reported five overlength design lines, later wrapped. The last syntax probe parsed one AST,
reported 457 design lines and zero overlength lines. A final four-file line check also found zero violations.
The exact -c bodies, output values, and all redirects are in check-commands.md.

## Files written

- docs/api-4.md
- proto/functions4_checks.py
- lanes/d-functions4/progress.md
- lanes/d-functions4/check-commands.md
- lanes/d-functions4/oracle-first.log, oracle-second.log, oracle-third.log, oracle-fourth.log
- lanes/d-functions4/oracle-fifth.log, oracle-sixth.log, oracle-final.log
- lanes/d-functions4/report.md, written once at completion

No src/, include/, fixed specification, convention, proof or golden file was changed.
No git command, tracker command, package installation or subagent was used.

## Not done

No C implementation, header changes, executable driver commands or Julia wrapper were built: this is the design lane.
The commands and ccall signatures are acceptance targets for the seven implementation slices.
C parser/dump failures, memory ownership, alias transactions and the production resource preflights need C tests.
The full tail kernel's worst-case cap has not been benchmarked. Its proposed constants remain a TJO decision.
No FFT, minimal representation, owning tensor-sum type, or general function-equality solver is designed.
Ball dilation is accepted through a certified idele and a fixed index operation; arbitrary uncertain scaling is not.
Milestone 5's Mellin integration, continuation, pole behavior and incomplete-Gamma evaluation are only consumers here.
No independent adversarial agent review was run; the design remains to be reviewed before the first C slice.

## Sources pending

Inherited analytic imports still need local line references, as recorded in analysis Definition 1:30-66:
Haar existence/uniqueness, product compactness, Fubini, dominated convergence, holomorphic parameter
integration, the identity theorem, Fourier uniqueness, CRT and unique factorization.
[source pending: those local theorem references]

The ball-enclosure, complex-root and character-convention sources were read on disk and cited by path and line.
Milestone 5 retains P15's pending incomplete-Gamma enclosure and vertical-strip Stirling sources.
No claim from memory was substituted for those sources. The additional finite-index and branch proofs are written out.
