# Analysis proof lane report

The proof and numerical-check work is complete. Source closure and independent proof review remain outstanding.
No specification file was changed. No git command, tracker command, package installation, or sub-agent was used.
All computations were single-process; numerical runs set the OpenBLAS and OpenMP thread limits to 1.

## What was done

Wrote 15 numbered definitions, lemmas, and propositions in `docs/proofs/analysis.md`. Each has a Check entry,
a Used by entry, and a row in the final statement register. The proofs cover:

- The additive character, its triviality on Q, fractional-radius ball images, and the quotient measure.
- Local annihilators, self-dual measures, all three weighted finite-transform formulas, and reflection.
- Polynomial-Gaussian closure, the positive real transform, and its polynomial recurrence.
- Poisson summation over Q, its lattice reduction, idele theta series, and explicit Gaussian tail bounds.
- Local Tate integrals, inverse ramified test vectors, both real parities, and all local gamma factors.
- The conjugate idele character, global integrals, and the completion factor C^((s+e)/2).
- Splitting at norm 1, the two continued integrals, pole terms, and residues -1 at 0 and +1 at 1 for zeta.
- The primitive functional equation with W_chi=tau(chi)/(i^e sqrt(C)), using positive finite tau.
- Explicit series and integral truncation bounds and a midpoint quadrature certificate.

The prototype tests independent finite enumeration, exact rational phases, direct real integration, local
coset integration, independently truncated Poisson sums, and Hurwitz-zeta reference values. The continuation
path integrates exponential terms with incomplete Gamma integrals and does not call zeta or L evaluators.
Two cases also compare these integrated terms with ordinary quadrature of the split integrand.

Coverage includes D=2,M=3 and D=3,M=2, non-symmetric complex arrays, shifted complex Gaussians,
negative dilations, both parities, non-real primitive characters of conductors 5 and 7, and conductors 4 and 8.
The local functional equation is checked on non-symmetric local arrays, not only its special test vectors.

## Files written

Final deliverables:

- `docs/proofs/analysis.md`
- `proto/analysis_checks.py`
- `lanes/m0-proofs-analysis/report.md`

Reproducible supporting checks and results in this lane:

- `structure_checks.py`, `mutation_checks.py`
- `red.txt`, `checks-first.txt`, `checks-final.txt`, `structure.txt`
- `mutations-first.txt`, `mutations.txt`
- Eight `mutant_*.py` files and their eight `mutant_*.txt` results. These are deliberately incorrect copies.

A construction fragment, `checks_extra.py`, was written inside this lane and removed after incorporation.
No file owned by another lane was edited or removed.

## Checks run

Environment check:

    python3 -c 'import mpmath; import flint; print(mpmath.__version__); print(flint.__version__)'

Result: exit 0; mpmath 1.3.0; python-flint 0.8.0. No installation was needed.

The numerical command, run first with missing implementations and twice after implementation, was:

    OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 -B proto/analysis_checks.py

The shell redirected each run to the corresponding lane log.

| Run | Assertions | Failed groups | Seconds | Result log |
|---|---:|---:|---:|---|
| Initial red run | 0 | 2 | 0.145 | red.txt |
| First complete run | 2900 | 0 | 10.304 | checks-first.txt |
| Expanded final run | 3131 | 0 | 11.981 | checks-final.txt |

The red failures were the two expected NotImplementedError exceptions for finite and real transforms.
Both complete runs returned exit 0. The final run uses 55 decimal digits, with 90 guard digits for finite
local pole residues. Its comparison threshold is 1e-32 times max(1,abs(reference)). It does not weaken a
comparison after failure. The added pole checks use the represented displacement from the pole, so that
rounding in forming 1+epsilon is not mistaken for a residue error.

| Final check function | Assertions | Failures |
|---|---:|---:|
| check_characters | 2292 | 0 |
| check_local_fourier | 68 | 0 |
| check_finite_fourier | 58 | 0 |
| check_real_transform | 10 | 0 |
| check_real_closure | 17 | 0 |
| check_tail_bounds | 80 | 0 |
| check_poisson | 4 | 0 |
| check_gauss_sums | 334 | 0 |
| check_local_integrals | 16 | 0 |
| check_local_gamma | 14 | 0 |
| check_real_local | 7 | 0 |
| check_idele_character | 14 | 0 |
| check_global_integral | 8 | 0 |
| check_theta | 18 | 0 |
| check_functional_equation | 34 | 0 |
| check_continuation | 62 | 0 |
| check_quadrature_bound | 9 | 0 |
| check_continuation_bounds | 72 | 0 |
| check_poles | 14 | 0 |

The functional-equation comparisons cover 8 primitive characters and 4 complex s values each: 32 comparisons.
Their maximum relative error was 1.7518e-55. The two additional assertions reject missing conjugation and
reversing the Gauss-sum sign. The continued integral was compared at 20 character/point combinations,
including Re(s)<0 and complex points within 1.5e-8 of 0 and 1. Its maximum relative error was 4.2306e-48.
The 20 analytic truncation bounds were all less than 1e-33. Pole-residue checks had maximum relative error
9.769e-36. Bounds for deliberately small cutoffs and two midpoint meshes were also checked independently.

Mutation command, run twice:

    python3 -B lanes/m0-proofs-analysis/mutation_checks.py

First result: exit 1, 7 mutations killed, 1 mutation not constructed because its search string matched twice.
The runner was fixed to target only the finite-transform expression. No mathematical test was weakened.
Final result: exit 0, 8 mutations constructed, 8 killed, 0 survivors. Each mutant subprocess exited 1 with
an AssertionError. The eight mutations were:

- Reversed finite Fourier sign.
- Finite Haar weight 1/D in place of 1/M.
- Reversed real Fourier sign.
- Reversed ramified local Gauss-sum sign.
- Inverted the parity phase in the root number.
- Omitted conjugation in the functional equation.
- Omitted the conductor in the completion.
- Reversed the pole term at zero.

Structural inspection was first run through Python heredocs. It found 2 lines longer than 116 characters;
those were wrapped or shortened. A later inspection found 15 statements, 15 Check entries, 15 Used by
entries, 18 referenced check functions, 0 missing functions, and 0 overlength lines. After adding the
reference to check_continuation_bounds, the final reproducible command is:

    python3 -B lanes/m0-proofs-analysis/structure_checks.py

Final result: 15 statements, 15 Check entries, 15 Used by entries, 19 distinct referenced check functions,
0 missing functions, 0 overlength lines, and Python AST parsing succeeds for all three checked scripts.
The complete output is in `structure.txt`.

Source inventory used `rg --files refs` during initial and final inspections. No source files were available
under refs/ to this lane. No external formula has been attributed to an on-disk source that was not read.

## What is not done

- No production C, FLINT integration, or Arb interval implementation was written; those are later milestones.
- mpmath results are numerical diagnostics, not certified floating-point enclosures. The mathematical error
  bounds are proved, but this prototype does not certify mpmath rounding or its special-function algorithms.
- The midpoint certificate is an existence and correctness algorithm. No performance claim is made for it.
- There has been no independent reviewer audit of this new proof file.
- Source closure for the explicitly imported standard theorems is still pending.

## Sources pending

The mathematical derivations are written out here. Definition 1 records the exact standard theorems imported
without proof. These are the remaining source dependencies:

1. Tate thesis section 2.2: attribution of the paired additive-character and Fourier conventions.
2. Existence and uniqueness of Haar measure.
3. Compactness of products of compact Hausdorff spaces.
4. Classification of continuous characters of R.
5. Fubini, dominated convergence, and holomorphic parameter integration in the stated forms.
6. The holomorphic identity theorem and uniqueness of meromorphic continuation.
7. Uniqueness of Fourier series for a continuous periodic function.
8. Taylor's theorem with integral remainder.
9. The Chinese remainder theorem.
10. Unique prime factorization of integers and positive rationals.

These dependencies are marked with source-pending annotations in the proof. There is no imported adelic
Poisson theorem or Tate functional equation hidden behind a standard label: those are derived in the file.

## Findings against the specification

None found in the assigned sections. The formulas agree with the signs and measures fixed in SPEC 6 and 7.
The complex Gauss sums and odd-parity tests distinguish the conventions. No specification statement was
weakened or changed to obtain a passing result.
