# Lane f-slice1: local balls, first vertical slice (milestone 1F, work package 1F.3)

The first piece of milestone 1F that a user can call: the type `adf_lball` (a ball in `Q_p` at one prime)
with exact ball arithmetic and its decomposition.

Functions of the slice (names after `docs/conventions.md` 2 and 5.8; the struct is fixed there):
- init, clear, set (from a rational centre, a prime place and a precision), set from an `adf_fball`
  (the projection to the prime `p`), `is_canonical`, `equal_set`, `contains`, `overlaps`, accessors,
  `adf_sizeof_lball`, `adf_alignof_lball`;
- `add`, `sub`, `neg`, `mul`, `inv` (status when the ball contains 0 or the unit part is not determined);
- `valuation` (status when the ball contains 0), `abs` as the exact rational `p^-v`, the decomposition
  `p^m u` of a ball that does not contain 0.
- Not in this slice: `exp`, `log`, roots, `adf_sball`, text and dump forms.

An unfinished design lane left notes, UNREVIEWED, in the worktree
`/home/tobias/Projects/adelefeld/.claude/worktrees/agent-adaf36d4414085b09`: `lanes/d-functions/progress.md`
(facts found, a probe of FLINT's `padic`), and an appended part 2 of `proto/functions_checks.py` that does
NOT run yet (it ends with an OverflowError). Read the notes. Do not copy part 2: write the reference of
this slice yourself, small, as a new section at the end of `proto/functions_checks.py`, and keep the
existing checks of that file untouched and passing.

Decisions taken by the orchestrator (TJO: decide and go on): the arithmetic is the library's own on
`fmpz` (centre, valuation, precision), not FLINT's `padic` type, whose C source is not on disk; every
result is the SMALLEST ball that contains the set of all results (sum, product, inverse of the points of
the input balls), and the test proves it by enumeration. If the slice needs another decision, take the
one a careful numerical analyst would take, implement it, and list it in the result file with the
alternatives.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold:
you write the header; rule 5 is replaced by item 5 below), `docs/SPEC.md` 4 and 9 and 15,
`docs/PLAN.md` milestone 1F, `docs/proofs/functions.md` (Proposition 4 and what it cites; cite file and
line in the code), `docs/proofs/precision.md`, `docs/conventions.md` 2 to 5.9, 7 and 12,
`include/adelefeld/fball.h`, `place.h` and `resid.h` for the style of a header.

**You own:** `include/adelefeld/lball.h` (new), `src/lball.c` (new), `tests/test_lball.c` (new),
`tests/julia/lball.jl` (new), `proto/functions_checks.py` (a new section at its end),
`tests/ref/vectors/f-slice1/` (new), `docs/api-1f.md` (new; ONLY the section of this slice),
`lanes/f-slice1/`. In `include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files
need and nothing else. Everything else is read-only. Other lanes work on roots and on ideles.

1. The header first, with the comment block of every declaration (set statement, proposition with file
   and line, aliasing rule, statuses and the state of the outputs on each). `functions.md` has no
   statement for the sum, product and inverse of BALLS, for the projection of a finite ball to a prime,
   and for the decomposition of a ball: write each statement with its proof, stepwise, in
   `docs/api-1f.md`, section "Statements to add to functions.md".
2. Tests first, red then green (`lanes/f-slice1/redgreen.log`). Oracle: enumeration modulo small prime
   powers (`p` = 2, 3, 5, 7; all balls with valuation between -3 and 3 and relative precision up to 4):
   the result contains every sum, product, inverse of points, and no smaller ball does. Also: valuation
   of balls around 0; negative valuations; the prime `2^64 - 59`; centres of thousands of bits; every
   aliasing combination; every status with the state of the outputs.
3. The code. 4. `tests/julia/lball.jl`: a user makes `1/3` at `p = 5` to precision 10, multiplies and
   inverts, reads valuation and unit; run by `tests/test_julia.sh`.
5. Show that the tests bite: three faults of your choice in a scratch copy under `build/` (for example
   the precision of a product taken as the minimum of the two precisions without the valuations). No
   mutation run, no fuzz target in this slice.
6. `make clean && make check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass; give the last
   line of each.

Result: `lanes/f-slice1/result.md` and the same text as your final message: functions built, decisions
taken, the numbers of the tests and what would have made a case fail, findings, the next slice you
propose.
