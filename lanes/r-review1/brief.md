# Lane r-review1: adversarial review of the real roots by exact isolation (issue adf-8di)

Your task is to REFUTE, not to confirm. The code under review was written by Claude Opus in lane
r-slice1: `src/roots_real.c` (new: isolation by Descartes bisection, Algorithm D; refinement by
galloping, quadratic interval refinement and bisection, Algorithm F), the call site in `adf_roots_real`
of `src/roots.c`, the sentences on the real roots in `include/adelefeld/roots.h`,
`tests/test_roots_real_isolate.c`, the additions to `tests/test_roots_real.c`,
`tests/fuzz/diff_roots_real.py`, `proto/real_isolation.py`, and the design `docs/design/real-roots.md`
(Lemma R1, Propositions R2 to R5 with proofs, cost, lower bound). Contract: `roots.h`;
`docs/proofs/solvers.md` Propositions 3.8 to 3.10; decisions S-D11, S-D13, S-D19 and N-D2 of
`docs/SPEC.md` 15. Report of the author: `lanes/r-slice1/result.md`. Earlier review of the old route:
`docs/reviews/s2/review-real.md`.

The certificate (`real_finish`: FLINT's count, exact signs at the end points, order, accuracy) was not
changed and stands between the new code and a wrong answer. So a wrong list with `OK` needs a defect
of the certificate too; but a defect of the isolation shows as a false `NOT_DETERMINED`, a false
`LIMIT`, an abort, a loop without end, or a cost without bound, and these count.

**You own:** `lanes/r-review1/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`, none longer than 3 minutes. Build with
`make -j2 BUILD=lanes/r-review1/build` and link your programs against that archive.

## What counts as a finding

- A list with `OK` that misses a real root, holds a ball without a root or with two roots, or balls
  that overlap or are out of order; a ball below the accuracy of S-D19. Your own oracle: polynomials with
  roots you planted (rational, dyadic, quadratic irrational by products of `a X^2 + b X + c` with known
  discriminant), counts by your own Sturm chain in exact rational arithmetic.
- `NOT_DETERMINED`, `LIMIT` or an abort on a squarefree or non-squarefree input for which the header
  promises an answer. Try: roots at dyadic points at every level of the bisection; roots at 0; roots
  that are end points of the first interval (the root bound of Lemma R1 attained); clusters far from
  0 (`2^e + 1/3`, `2^e + 2/3`); many tiny roots near the smallest admitted scale; roots of both signs
  with equal absolute value; Mignotte polynomials; Wilkinson 20 and 40; Chebyshev; degree 1; constants;
  content and negative leading coefficient; coefficients of 10^5 bits; degree 500.
- A false statement among R1 to R5 of the design, or a proof step that does not follow. Read them as a
  referee. In particular: the termination of Algorithm D (which statement of the source is used, is
  it quoted rightly from the file on disk); the claim that the refinement keeps exactly one root in the
  cell at every step of galloping and of the quadratic step; the static floor and the nesting of the
  balls when `prec` grows (find `prec1 < prec2` with a ball at `prec2` not inside the ball at `prec1`).
- Cost: an input of modest size (degree at most 50, coefficients at most 10^4 bits, `prec` at most
  4096) that takes more than 10 s. The author names one weak case (a cluster far from its own scale,
  one level of descent for each bit); find how bad it gets and whether another exists. Memory that
  grows beyond what the output needs.
- An overflow of an exponent (`slong`), in particular in the galloping and in the scale of the first
  interval; an allocation before `LIMIT` is decided.
- A leak or undefined behaviour (sanitizer build, valgrind at `~/.local/bin/valgrind`).
- A sentence of the header that the code does not keep; a test that cannot fail; the differential
  script accepting a planted fault (plant one in a scratch copy).

## Report

`lanes/r-review1/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong list
with `OK`, a memory fault, a loop without end; MAJOR; MINOR), the input, what the code returns, what is
true and why, and the command that reproduces it with a program in your lane directory. Then: what you
attacked without result, with the number of cases and what would have made a case fail. Then, in five
lines at most, your judgment of the regression the author reports (well-separated roots at high
precision are about 2 times slower than FLINT's route): is a hybrid worth it. No praise, no summary
of the code.
