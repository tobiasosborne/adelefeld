# Lane m0-proofs-functions: work package 0.3a, `docs/proofs/functions.md`

Read `docs/SPEC.md` section 9.3 (all of 9.3.1 to 9.3.6), `docs/PLAN.md` row 0.3 and rows 1F.3 to 1F.8,
`docs/reviews/astra-2026-09-27-r2/review.md` (findings N1 to N7, N10 with their proofs) and
`docs/reviews/astra-2026-09-27-r3/review.md` (R4), and their `checks/`. Style model: `docs/proofs/precision.md`.

**You own:** `docs/proofs/functions.md`, `proto/functions_checks.py`.

Rewrite, stepwise and complete, the proofs of everything section 9.3.1 to 9.3.6 marks **[proved]**:

1. The decomposition `x = p^m w u` (N1), including `p = 2` and why `w` there is not the Teichmueller
   representative.
2. Domains of convergence of `exp`, `sin`, `sinh`, `cos`, `cosh`, `log` at every prime, with exact statements
   of what happens on the boundary (`p Z_p` at `p = 2`), and the valuations of the terms (Legendre's formula).
3. The radius rules of the table in 9.3.2: isometry of `exp`, `sin`, `sinh` on the domain; the safe and the
   smallest ball for `cos`, `cosh`; `log` on `1 + p Z_p` and on `1 + p^c Z_p`, non-injectivity at 2; the image of
   a ball under the Iwasawa `Log`, with the case `N - m = 1` at 2.
4. **Truncation bounds** for each series: for input valuation `>= v` and wanted absolute precision `p^n`, an
   explicit number of terms `T(p, v, n)` such that the tail has valuation `>= n`, proved, for all `p` including
   2 and 3 (where the factorials bite). State also the working precision needed for the partial sum with its
   denominators. These bounds will be implemented literally in C, so they must be explicit and checked by
   enumeration for `p = 2, 3, 5, 13`, several `v`, `n` up to 40.
5. The common domain `D` at all primes, that it contains no finite ball of positive radius and only the rational
   0; `Log` of an idele lies in `4 Zhat` (N5).
6. Roots (9.3.3): the existence criterion at odd `p`, at 2, at the real place; the number of roots; the precision
   formula `b + p^(N - v(n) - (n-1) v(b)) Z_p` under the guard `N - m >= c + v(n)`, with proof that the image is
   exactly that ball, and an example showing what fails outside the guard; the all-places statements and R4.
7. Powers (9.3.4): the four operations; `exp(s Log x)` is not a power; principal-unit powers with both
   uncertainties, an explicit radius rule for `u^s` when `u` is known modulo `p^A` and `s` modulo `p^B`.
8. The statements of 9.3.5 and the typed claims of 9.3.6 that need proof (no order on `Q_5`; the fractional
   part).

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

