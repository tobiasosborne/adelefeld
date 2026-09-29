# Lane d-realroots: design of a real root isolation with a bounded cost (issue adf-8di)

A design, with proofs and measurements. No change of `src/` or `include/`. Review of a design before code
is what found defects in this project (`docs/workflow.md`); write for a reviewer who will try to refute you.

The problem. `adf_roots_real` (`src/roots.c`, `include/adelefeld/roots.h`, `docs/proofs/solvers.md`
Propositions 3.8 to 3.10, Algorithm RR) is correct and its cost has no bound: the candidates come from
`arb_fmpz_poly_complex_roots`, and for the squarefree quadratic with the roots `2^e` and `2^e + 1` at
`prec = 2` it takes 0.03 s (`e = 600`), 9.4 s (`e = 1200`), 97 s (`e = 1500`), no answer in 175 s
(`e = 1800`); the end points of the balls have `2^20` bits (`docs/reviews/s2/review-real.md`,
`lanes/s2-slice3/result.md` finding 1).

Read first: `CLAUDE.md`, `docs/workflow.md`, `docs/PERF.md` (how speed is judged: a lower bound first),
`lanes/PROOFSTYLE.md`, the sources named above, `docs/sources.md` table 3, and the FLINT 3.0.1
documentation AND C source of every routine you discuss (`refs/src/flint-3.0.1/`,
`refs/src/flint-src-3.0.1/`; fetch what is missing with `refs/fetch_sources.sh` in the way it is written,
and say what you fetched). Rule 4 of CLAUDE.md: a statement about FLINT or about a theorem of other people
is quoted from a file on disk with file and line, or marked `[source pending: ...]`.

**You own:** `docs/design/real-roots.md` (new), `proto/real_isolation.py` and `proto/test_real_isolation.py`
(new), `bench/bench_roots_real.c` (new), `refs/fetch_sources.sh` and `refs/manifest.sha256` (additions),
`docs/sources.md` (new rows), `lanes/d-realroots/`. Everything else is read-only.

What the design must contain.
1. Where the time goes: measure it (a benchmark program, a profile with `perf` if it is installed), for
   the family above and for two others that you choose to separate the causes (large roots far apart; small
   roots close together; degree). State the cause with the lines of FLINT's source.
2. A lower bound in the sense of `docs/PERF.md`: what any algorithm must do for this input (bit size of
   the output, separation of the roots, the cost of one exact evaluation), with the theorem and its source.
3. At least two candidate algorithms that isolate only the real roots in exact arithmetic (for example
   bisection with Descartes' rule of signs on the squarefree part; a Sturm chain with bisection; what
   FLINT 3.0.1 offers for real roots of `fmpz_poly`, read in its source), each with: the statement it
   proves, its cost in the sizes of the input, what it needs from FLINT, and how the result is certified
   by the exact test of Proposition 3.8 and by the count. Then the refinement of an isolating interval to
   `prec` bits, with its cost. A reference implementation of the recommended one in
   `proto/real_isolation.py` (exact rational arithmetic, `python-flint` or plain integers), tested against
   the 18 `REAL_CASES` of `proto/solvers_checks.py` and against the slow family up to `e = 3000`, with
   times.
4. The statements to be added to `docs/proofs/solvers.md`, written out as propositions with proofs in the
   style of that file (as text in your design file; do not edit `solvers.md`).
5. What changes in the interface: nothing, or what (statuses, a `LIMIT`, the accuracy promise of S-D19,
   exact balls for rational roots). Decisions for TJO as a table: question, recommendation, alternatives
   stated fairly.
6. The slices in which it would be built, each end to end, and the test that must fail for a wrong
   implementation.

Report (`lanes/d-realroots/result.md` and your final message): what was measured, with commands and
numbers; what is proved and what is not; sources fetched and pending; the decisions.
