# Lane d-tate report

Completed the design and oracle. docs/api-5.md has 409 lines, maximum 109 columns.
The final oracle run returned 0: 466 checks in 6.73 seconds, maximum RSS 76524 KiB.
462 checks belong to this lane; 4 are inherited assertions from analysis_checks.py.

## Interface in twenty lines

1. No new owning integral-result or pole type; use acb, input S, mode and status.
2. local_tate_at(z,where,s,v,alpha,eta0,prec) evaluates the prescribed local vector.
3. local_gamma_at has the same arguments and the fixed local functional equation normalization.
4. local_epsilon_at simplifies before evaluation, including at gamma poles.
5. At p, eta0 has primitive conductor 1 or p^a; alpha is supplied separately.
6. At infinity, eta0 selects parity and alpha is ignored.
7. The trivial integral delegates to local_zeta_factor_at; odd infinity shifts s by 1.
8. tate_vector(phi,f,chi,prec) constructs the exact prescribed function, enclosed by existing types.
9. tate_integral(z,chi,s,bits,prec) returns uncompleted I only on certified Re(S)>1.
10. tate_continue has the same arguments and returns meromorphic I on a pole-free S.
11. tate_completed has the same arguments and returns Lambda=C^((s+e)/2) I.
12. These finite-character calls explicitly ignore chi->s, like adf_char_chi.
13. Internally omega is conj(chi), with its norm exponent reset to zero.
14. Functional-equation callers evaluate completed twice and multiply one side by root_number.
15. A successful global call certifies both coordinate diameters <=2^-bits.
16. Exact zeta poles 0 and 1 give DOMAIN; mixed pole rectangles give NOT_DETERMINED.
17. Near a pole, evaluate the explicit reciprocal term and check the final width.
18. Every failure preserves value/count outputs; local failures write where=v if supplied.
19. Same-type raw aliases z=s or z=alpha are allowed; character-member aliases are forbidden.
20. Local declarations extend localfactor.h; vector/global declarations go in a future tate.h.

## Decisions

- D1: acb/status results with distinct half-plane and continuation calls; no pole serialization.
- D2: inherited character/factor limits and a proposed 2^20 total Tate work cap, including retries.
  Direct transforms retain C^2<=2^20, so the global path can refuse below the character cap 65536.
- D3: theta Mellin splitting for the primary value, reused by continuation; Euler products are checks.
- D4: composite Taylor quadrature with an explicit geometric remainder. Midpoint is a comparison rule.
  Incomplete Gamma is a numerical reference only. Its missing enclosure source is not a premise.

These are proposals for the implementing slices. No approval or change to SPEC is claimed.

## What was done and written

- docs/api-5.md: seven proposed C calls, their domains/statuses/costs, five additional stepwise proofs,
  exact completion conversions, acceptance conditions, six faults per package and five thin-slice stages.
- proto/tate_checks.py: seven requested oracle functions, exact phase/cyclotomic expressions, numerical
  references and certified continuation using elementary ball quadrature, without an L-function evaluator.
- lanes/d-tate/progress.md: incremental notes, including oracle-development failures.
- lanes/d-tate/verify_design.py: reproducible line-budget, column, syntax and entry-point checks.
- lanes/d-tate/oracle.log and oracle-time.log: final numerical output and resource measurements.
- lanes/d-tate/design-check.log: final artifact-check output.
- lanes/d-tate/report.md: this report, written once after the work and checks.

No src/include file, specification, convention, proof source or golden was changed. No git or bd command ran.
No subagent or installation was used. FLINT used one thread; no run exceeded 120 seconds.

## Checks and failure conditions

| Groups | Count | What would fail |
|---|---:|---|
| local_exact, euler_exact | 63 | Wrong cyclotomic phase, inverse or finite transform identity |
| local_numeric, local_poles, real | 58 | Wrong closed form, measure, parity, pole or residue |
| global_numeric, global_flint, euler | 53 | Wrong completion/conjugation or an error above the Euler bound |
| bounds, midpoint, taylor | 60 | Omitted-region or quadrature error above its stated bound |
| faults_51 through faults_54 | 24 | A planted wrong formula not separated from the reference |
| width_witness, pole_qualification | 3 | Claimed positive width floor or odd pole examples absent |
| halfplane_width, continuation, input_radii | 74 | Reference disjoint, target width missed, or radius lost |
| poles, functional_equation | 30 | Wrong pole status/residue, infinite width, or disjoint equation sides |
| goldens, composite | 59 | A golden root interval missed or a CRT uniformizer factor omitted |
| poisson_dilation, inherited_analysis | 37 | A dilation/Poisson enclosure misses its independent reference |
| independence | 5 | A forbidden reference call, missing second call or copied functional-equation side |
| Total | 466 | |

The 24 planted-formula checks are oracle witnesses, not mutation coverage of future C code.
The inherited group calls analysis.check_poisson_right_and_idele and reruns its four assertions.
No long fuzz run was performed. Numerical mpmath comparisons use 60 digits and margin 1e-45 times
max(1,abs(reference)); they are not ball certificates. The Taylor continuation bounds are ball certificates.

At 100 bits, the five continuation cases use N=4,16,16,16,32 and R=32,128,128,128,512.
Their maximum Taylor degrees are 184,189,190,190,192. Coefficient work counts are
6208,18848,31668,31239,55692. Each case also ran at 32 and 64 bits with decreasing output widths.
The reference backend is python-flint 0.8.0, bundled FLINT 3.3.1, not the project's C FLINT 3.0.1.

Commands run:

- `timeout 20 python3 -B -` with an inline import/docstring probe: exit 0. mpmath 1.3.0,
  sympy 1.14.0 and python-flint 0.8.0 imported; its FLINT version was 3.3.1.
- `timeout 120 python3 -B proto/tate_checks.py`, five development runs: exits 1,1,1,1,0.
  Run 1 attempted finite evaluation of odd gamma at s=2; it now explicitly checks that pole.
  Run 2 failed faults_53 check 6: the selected displacement did not meet the asserted separation.
  The replacement uses two zeta values whose separation is 3.91106832887771501839772.
  Run 3 called contains on the golden parser's list; exact endpoint comparisons replaced that adapter.
  Run 4 compared a truncated Poisson midpoint at 1e-45 despite a larger valid tail; the check now uses
  the certified enclosure, both independent sides, and a width bound. Run 5 passed 434 checks.
- Five `timeout 10 python3 -B -` inline source/AST checks: all exited 0. The initial 390-line draft
  reported 4 overlong table lines; later drafts had 402,403,406,409 lines and 0 overlong lines.
  The final inline check parsed the oracle and found 7/7 requested entry points.
- Three timed runs of `timeout 120 python3 proto/tate_checks.py`: exits 0,0,0; counts 457,462,466.
  The final command was:

```sh
/usr/bin/time -f 'wall_seconds=%e user_seconds=%U system_seconds=%S max_rss_kib=%M exit=%x' \
  timeout 120 python3 proto/tate_checks.py \
  > lanes/d-tate/oracle.log 2> lanes/d-tate/oracle-time.log
```

  Final measurements: wall 6.73 s, user 6.70 s, system 0.02 s, maximum RSS 76524 KiB, exit 0.
- `timeout 10 python3 -B lanes/d-tate/verify_design.py > lanes/d-tate/design-check.log`: exit 0.
  Design 409 lines, oracle 884 lines, progress 40 lines; maximum columns 109,110,110; 0 overlong.
  Oracle AST parsed; 7/7 required entry points found; design below the 500-line cap.

## Findings against the specification

No counterexample found to the fixed SPEC, PLAN, convention or golden formulas.
The existing N-D23 qualification is necessary. Completed zeta at 1.125 and 1.25 differs by
3.91106832887771501839772, so their spectral hull cannot have arbitrarily small output width.
This confirms a recorded qualification; it does not request a specification change.

Brief qualifications: the odd real integral has a pole at -1 and odd real gamma at 2; neither uses
the trivial integral's pole list. Both are computed as poles by the oracle. The raw real golden (8,7)
lowers to primitive (4,3), with tau=2i and W=1; treating it as primitive conductor 8 would be wrong.

## Not done

No C implementation, driver implementation or Julia execution. No independent adversarial design review.
No C mutation campaign or performance claim beyond the measured oracle run.
No generic tensor Tate API, derivative in s, all-places product object, pole serialization or fast
large-conductor transform: the prescribed SPEC 8 vectors do not require them. The general P12 formula
and dilation/cutoff proof are recorded so a later generic interface has a defined mathematical starting point.

## Sources pending

- Inherited analysis prerequisites: Haar existence/uniqueness, product compactness, real characters,
  Fubini/dominated convergence, holomorphic parameter integration, identity theorem, Fourier uniqueness,
  Taylor's theorem, CRT and unique factorization. Their statements remain in analysis Definition 1.
- Universal FLINT Conrey phase/label identification, inherited from api-3c/api-3d.
- Signed primitive quadratic Gauss evaluation for universal W=1. Eight real goldens are examples only.
- Hurwitz constant term -digamma(a), used only by the mpmath L(1,chi) reference.
- Upper incomplete Gamma enclosure documentation and a vertical-strip Stirling bound from P15.
  Neither is used by the certified continuation or promised as a relative-accuracy estimate.

The added Euler, normalization, Taylor, near-pole and dilation arguments are written step by step in
api-5.md. On-disk FLINT and Tate references are cited there with file and line numbers.
