# Lane m0-proofs-analysis: work package 0.3c, `docs/proofs/analysis.md`

Read `docs/SPEC.md` sections 5 (characters), 6, 7, 8 and the rows Gauss sums, local constants, local zeta factor,
theta series of 9.3.7; `docs/PLAN.md` rows 0.3, 3.2 to 3.4, 4.1 to 4.5, 5.1 to 5.4;
`docs/reviews/astra-2026-09-27/review.md` (findings M6 to M10 with their proofs). Style model:
`docs/proofs/precision.md`.

**You own:** `docs/proofs/analysis.md`, `proto/analysis_checks.py`.

Fix the conventions exactly as `docs/SPEC.md` section 6 states them (additive character
`psi(x) = exp(2 pi i (-x_inf + sum_p {x_p}_p))`, transform with `conj(psi(x y))`, Haar measure with `Z_p` of
volume 1 and Lebesgue measure at infinity) and prove, with every sign and constant explicit:

1. `psi` is a character of `A`, trivial on `Q`; values on balls, including fractional radius (M7).
2. Self-duality of the measures for this `psi`; the weighted finite Fourier transform of section 7 with all three
   displayed formulas, the support and periodicity of the transform, the second transform.
3. The real family `P(x) exp(-pi A x^2 + B x + C)`, `Re(A) > 0`: closure under product, translation, dilation
   and the transform, with the explicit formula of the transform of each term under the chosen sign (an algorithm:
   the polynomial part by a stated recurrence).
4. Poisson summation for the implemented class `f = f_inf x f_fin`: statement over `Q` in `A`, reduction to a
   sum over a lattice in `R`, and **proved tail bounds**: explicit `B(T)` with
   `|sum over |n| > T of term| <= B(T)` for the polynomial-Gaussian family, usable in code.
5. The local Tate integrals: unramified; ramified with the inverse character (why the naive test function gives
   zero); the real place for both parities. The global integral for `Re(s) > 1`: zeta and primitive Dirichlet
   characters, the completed L-function with the factor `C^((s+e)/2)`, why the idele character is
   `conj(chi(u'))`.
6. **Continuation and functional equation**: the splitting of the integral at `|x| = 1`, the use of Poisson
   summation, the pole terms with their residues, and the functional equation with Gauss sum, conductor, parity
   and conjugate character, in the normalisation that follows from the conventions above. State the root number
   explicitly as a formula in the Gauss sum `tau(chi)` with the definition of `tau` spelled out (which additive
   character, which sign). Derive the computable form of the continued integral: two rapidly convergent
   integrals plus pole terms, and explicit tail bounds for the sums and integrals in it, so that milestone 5 can
   implement it with certified error.
7. Gauss sums and the local constant `gamma(s, chi)` defined by the local functional equation, with the explicit
   value at unramified, ramified and real places under these conventions.

The checks must be able to tell the sign conventions apart: use shifted (non-even) Gaussians, non-symmetric finite
functions with `D` different from `M`, a non-real character (conductor 5 or 7) and both parities. Compare the
functional equation numerically at several complex `s` to 30 digits with `mpmath` (Hurwitz zeta for the
L-values), and compare the continued integral computed by your formula with `pi^(-s/2) Gamma(s/2) zeta(s)` at
points left of `Re(s) = 1` and near the poles `s = 0, 1`.

**Form of every proof file.** Numbered definitions, lemmas, propositions; each statement names its hypotheses
completely (which `p`, which ranges, zero and negative cases, `p = 2` separately wherever it differs). Proofs are
stepwise: each step follows from named earlier steps or from a named standard theorem. A standard theorem that is
used and not proved is stated exactly and marked `[source pending: ...]` (the sources lane is fetching texts to
`refs/` in parallel; do not wait for it). After each statement: "Check:" with the name of the function in your
proto file that tests it numerically, and "Used by:" with the section of `docs/SPEC.md`. End the file with a table
of all statements: number, one-line content, status (proved here / proved modulo the named standard theorem /
open), check. A statement you cannot prove is marked open with the reason; an open statement honestly marked is
worth more than a gap hidden in a proof.

**Checks.** The proto file is plain Python 3 (standard library; `python-flint` and `mpmath` are importable). It
must test the statement itself, by an independent computation (enumeration in small quotients, exact rational
arithmetic, truncated series with proved tail bound), not an identity that a wrong formula also satisfies. It
prints one line per check with counts and exits non-zero on any failure. Total running time under 3 minutes.

