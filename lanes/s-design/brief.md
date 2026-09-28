# Lane s-design: the design of milestone S (solvers), with proofs

Read `CLAUDE.md`, `lanes/COMMON.md`, `lanes/PROOFSTYLE.md` (the form of every proof file and of its checks).
Then `docs/PLAN.md` section 6 "Milestone S" and sections 7, 8; `docs/SPEC.md` sections 3, 4, 9.1, 9.2 and
section 15 (the decisions, above all M1-D3, M1-D8, M1-D10, M1-D11); `docs/proofs/quotient.md` Propositions 11
to 13; `docs/conventions.md` sections 3 (statuses), 4 (aliasing, output states, lifetime), 5 (predicates),
12 (foreign functions); `docs/api-m1.md` (the pattern of an interface document); the headers
`include/adelefeld/recon.h`, `fball.h`, `modctx.h`, `rat.h`; and `docs/sources.md` "Table 3: milestone S" with
its notes, its list "not on disk", and `lanes/s-sources/report.md` "Findings against the specification".

The sources are under `/home/tobias/Projects/adelefeld/refs/src/` (read only): `shoup-ntb`,
`storjohann-thesis`, `conrad-hensel`, `baker-padic`, `thorne-padic`, `flint-3.0.1` (documentation),
`flint-src-3.0.1` (C sources). CLAUDE.md rules 3 and 4 bind you: every theorem, algorithm and promise of
FLINT that you use is read in its source and cited by key, file and line; nothing is cited from memory; what
is not on disk is marked `[source pending: ...]`. Table 3 is an index made by a weaker model: read the source
at each place yourself before you lean on a row, and report every row that says more or less than the source.

**You own:** `docs/proofs/solvers.md` (new), `proto/solvers_checks.py` (new), `docs/api-s.md` (new),
`lanes/s-design/`. You change no other file: not `docs/SPEC.md`, not `docs/PLAN.md`, no header, no code.
What those files must say differently is written as a list of exact edits in `docs/api-s.md`.

## What is to be designed

The three work packages, in the order S.3, S.1, S.2. For each: the mathematical statements with proofs; the
algorithm, with the proof that it returns what the statement promises; the certificate, and the proof that a
cheap check of the certificate implies the result; the public interface; the acceptance tests.

**S.3 Partial rational reconstruction.** Given `m > 0`, a residue `c`, bounds `A >= 0`, `B > 0`: the reduced
fractions `n/d` with `gcd(d, m) = 1`, `n = c d mod m`, `|n| <= A`, `0 < d <= B`.
- The complete answer for EVERY admitted `(m, c, A, B)`, not only for `2 A B < m`: exactly one; none;
  several; and what "uniqueness not certified" means and when it is returned (SPEC 9.2). Say what the
  function does for `m <= 2 A B`: decide between proving the set of solutions (it is finite; give the
  algorithm and its cost) and a status. Thue's lemma gives existence under its own hypothesis: state the
  ranges in which the answer can be "none".
- The condition `gcd(d, m) = 1` and the reduced form: the row of the Euclidean algorithm need not be
  reduced and need not have a denominator prime to `m`. Prove what the algorithm returns in these cases.
- The sign convention (Shoup bounds `|t|`, SPEC and FLINT ask `d > 0`), `c` outside `[0, m)`, `A = 0`,
  `m = 1`, `m = 2` (FLINT's documentation asks `m > 2`).
- Whether the implementation calls `fmpq_reconstruct_fmpz_2` or its own loop: read FLINT's code and say
  exactly what it promises outside `2 N D < m` and for non-reduced candidates. A wrapper is allowed only
  where FLINT's promise is proved from its source or verified after the call by the certificate.
- The relation to the adelic ball: `1/5` is `5 mod 6` and is not in `5 + 6 Zhat` (SPEC 9.2). Which type
  carries the input, so that a user cannot confuse the two problems.

**S.1 Linear systems modulo `N`.** `A x = b mod N`, `A` an integer matrix `r x c`, `N >= 1` arbitrary
(composite, with square factors), and the adelic form: `A x = b` with `b` a vector of finite balls.
- The set of solutions: empty, or a coset `x0 + K` with `K` the kernel, a submodule of `(Z/N)^c`. The output:
  `x0`, generators of `K`, and a certificate; or a certificate that there is no solution.
- The canonical form of the generators (Howell form, Storjohann), so that equal kernels print identically
  (PLAN 7, "Canonical form"), and the proof that it is canonical over `Z/N`.
- The certificate: which matrices, which identities a checker verifies, and the proof that the identities
  imply (a) `x0` solves, (b) the generators lie in the kernel, (c) they generate ALL of it (completeness is
  the hard part: PLAN 7 asks "the kernel is complete on small cases"; give the statement that a checker can
  verify for every case, and its cost), (d) in the case "none", that no solution exists.
- FLINT: the solving functions of `nmod_mat` and `fmpz_mod_mat` are documented for prime moduli; the entry
  for general `N` is the Howell form. Decide what is called and what is verified afterwards. Decide between
  one computation modulo `N` and computations modulo the prime powers of a context with recombination;
  the factorisation of `N` is not assumed known unless the context gives it.
- Sizes and statuses: `r`, `c` up to what, the zero matrix, `N = 1`, `r = 0` or `c = 0`.

**S.2 Roots.** (a) Simple roots of an integer polynomial at a given prime by Hensel lifting, to a requested
precision, with certificate; the strong form `|f(a)| < |f'(a)|^2`; uniqueness of the lift and in which ball;
`p = 2` separately; a root modulo `p` that does not lift (`x^2 + 1` at 2); the output as finite balls of the
library. What "complete" means for the list of roots in `Z_p` and when it can be certified (multiple roots
are excluded from version 1: say what is returned when one is met). (b) Real roots: isolating balls with a
completeness status: what FLINT offers (`fmpz_poly_num_real_roots`, `arb_fmpz_poly`), what it certifies,
and the proof that count plus isolation gives completeness.

## The three documents

1. `docs/proofs/solvers.md`, in the form of `lanes/PROOFSTYLE.md`: definitions, propositions with complete
   hypotheses, stepwise proofs, "Check:" and "Used by:" after each statement, the table of all statements
   with status at the end. An open statement honestly marked is worth more than a gap hidden in a proof.
2. `proto/solvers_checks.py`: plain Python 3 (standard library, `python-flint`, `mpmath`), one check per
   statement, by independent computation (enumeration of all `n/d` within the bounds; enumeration of all of
   `(Z/N)^c` for small `N`, `c`; exact rational arithmetic), including the adversarial cases named above.
   One line per check with counts; exit status non-zero on any failure; under 3 minutes. Run it and give
   its output in the report. It is also the reference oracle of the later implementation: write the
   algorithms in it as you specify them, next to the brute-force enumeration, and compare the two.
3. `docs/api-s.md`, on the pattern of `docs/api-m1.md`: the types and functions proposed, each with its set
   statement, the statement of `solvers.md` it implements, aliasing, statuses with the state of each output,
   cost class; the rules of `docs/conventions.md` hold (statuses of section 3; no new status without a
   decision). Then: the list of decisions that TJO must take, each as a question with your recommendation
   and the alternative (numbered S-D1, S-D2, ...); the exact edits proposed for `docs/SPEC.md` and
   `docs/PLAN.md`; the work packages for the implementation lanes (files, order, which can run in parallel,
   which statement each function's test must check).

## Report

Your final message is the complete report (write it also to `lanes/s-design/report.md` if you can): what
was written; the output of `proto/solvers_checks.py`; the table of statements with status; every place where
a source says something different from `docs/SPEC.md`, `docs/proofs/quotient.md` or table 3 of
`docs/sources.md`; sources pending; what you are least sure of, in order, so that the reviewer (codex
`gpt-6-astra`, refute mode) starts there.
