# Lane f-slice4: `exp`, `log` and `Log` at a prime (milestone 1F, WP 1F.4)

The power series at a prime on local balls: `adf_lball_exp`, `adf_lball_log`, `adf_lball_Log` (the
Iwasawa logarithm with `Log(p) = 0`), with the domains, the radii and the precision losses that
`docs/SPEC.md` 9.3.2 and `docs/proofs/functions.md` Propositions 6 to 8, 10, 11 state. This is intricate
numerical work: the result must be an enclosure of the image of the WHOLE input ball, with the radius
the propositions give, not the value at the centre with the input's precision.

What is known (unreviewed notes of an earlier lane,
`/home/tobias/Projects/adelefeld/.claude/worktrees/agent-adaf36d4414085b09/lanes/d-functions/progress.md`
and `probe/probe.out`): `padic_exp(3)` = 958 and `padic_exp(12)` = 5125 modulo `3^8` (they differ at
digit 2, so `exp` of the ball `3 + 9 Z_3` is not known to 8 digits: the regression of review N4);
`padic_log` refuses 3 and -1 at `p = 2`; on refusal FLINT's output is untouched. The C source of
FLINT's `padic` module is NOT on disk.

Decisions of the orchestrator:
- The series are evaluated by the library's own code on `fmpz` and `fmpq` with the truncation counts of
  Proposition 7 and 7b and the working precision of Proposition 8 (proved on disk). FLINT's
  `padic_exp` and `padic_log` are used in the TESTS as a second opinion at the centre (their
  documentation `refs/src/flint-3.0.1/padic.rst` is on disk: cite file and line), not in the library.
  If you find that you need FLINT's routines in the library for speed, fetch their C source with
  `refs/fetch_sources.sh` in the way it fetches the others, read it, cite it, and say what is trusted.
- Simple correct evaluation first (a rectangular or plain Horner sum with exact rationals reduced
  modulo the working power of `p`); no binary splitting in this slice. A benchmark row is welcome, an
  optimisation is not.
- Decisions where the specification is silent are yours: take the one a careful numerical analyst
  would take, implement it, list it with the alternatives.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold:
you write the header; rule 5 is replaced by item 4 below), `docs/SPEC.md` 9.3.2 (every precision case
there is a test), 9.3.1, 15, `docs/proofs/functions.md` Definition 1, Lemma 5, Propositions 6, 7, 7b, 8,
Lemma 9, Propositions 10, 11, 12 (cite file and line in the code), `docs/PLAN.md` row 1F.4,
`include/adelefeld/lball.h`, `docs/api-1f.md`, `docs/reviews/f1/review-lball.md`.

**You own:** `include/adelefeld/lfunc.h` (new), `src/lfunc.c` (new), `tests/test_lfunc.c` (new),
`tests/julia/lfunc.jl` (new), `proto/lfunc_checks.py` (new; the reference and the oracles of this slice,
exact arithmetic), `tests/ref/vectors/f-slice4/` (new, below 1 MB), `docs/api-1f4.md` (new: the section
of this slice, with the statements you add and their proofs), `lanes/f-slice4/`. In
`include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files need. Everything else is
read-only: `lball.h` and `src/lball.c` are changed by another lane at this moment (additions only, no
existing declaration changes): use the public functions of `lball.h` as they are on master.

1. Header first, with the comment block of every declaration (set statement, proposition with file and
   line, aliasing, statuses: outside the domain of convergence `DOMAIN`; a ball that meets the domain
   and its complement `NOT_DETERMINED`; uncertain zero under `log`; limits).
2. Tests first, red then green (`lanes/f-slice4/redgreen.log`). Oracles:
   - enumeration: for small `p` (2, 3, 5, 7) and every ball of the domain with small precision, the
     value of the series at EVERY point of the ball modulo a higher power (computed by your reference
     with exact rationals and a proved tail bound) lies in the result ball, and the result is the
     smallest ball that the proposition promises (say which: smallest, or the radius of the
     proposition);
   - the precision cases of SPEC 9.3.2, including the losses at `x = 3, 12` (`p = 3`) and
     `x = 2, 10` (`p = 2`); `log(-1) = 0` at 2; balls at the edge of the domain; `exp(log(x)) = x` and
     `log(exp(x)) = x` on the domains where Lemma 9 gives them, as containment; `Log(p) = 0`,
     `Log(x y) = Log x + Log y` as containment;
   - FLINT's `padic_exp`, `padic_log` at the centre for 1000 random inputs up to precision 200, as a
     second opinion;
   - the prime `2^64 - 59`; precision 2000; every status; every aliasing combination.
3. The code. `tests/julia/lfunc.jl`: a user computes `exp(5)` and `log(6)` at `p = 5` to 20 digits,
   `log(-1)` at 2, and sees the `DOMAIN` of `exp(1)` at 5.
4. Show that the tests bite: five faults of your choice in a scratch copy under `build/` (among them:
   the result radius taken from the input precision without the loss; one term too few in the
   truncation). No mutation run, no fuzz target.
5. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`; give the last line of each.

Result: `lanes/f-slice4/result.md` and the same text as your final message: functions built, decisions,
numbers of the tests and what would have made a case fail, what is proved and what is not, findings,
the next slice you propose.
