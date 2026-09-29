# Lane i-slice1: ideles, first vertical slice (milestone 2, work package 2.1 in part, and 2.3 in part)

The first piece of milestone 2 that a user can call and that computes something true, end to end: unit
cosets and ideles with multiplication and inverse, and the map from a rational to an idele.

Functions of the slice (names after `docs/conventions.md` 2 and 5.6, 5.7; the final list is yours):
- `adf_ucoset`: init, clear, set (from `c`, `N`), the exact units `[1]` and `[-1]` (modulus 0, M0-D1),
  `is_canonical`, `equal_set`, `contains`, `mul`, `inv`, accessors, `adf_sizeof_ucoset`,
  `adf_alignof_ucoset`.
- `adf_idele`: init, clear, set from parts, `adf_idele_set_rat` (exact unit `sign(q)`, M0-D1), `mul`,
  `inv`, `is_canonical`, accessors, layout queries.
- Not in this slice: `adf_idclass`, powers, norms, hulls, division, text and dump forms.

An unfinished design lane left notes and a reference, UNREVIEWED: notes `lanes/d-ideles/progress.md` of
the branch, and part 2 of `proto/ideles_checks.py` (about 1000 lines), both in the worktree
`/home/tobias/Projects/adelefeld/.claude/worktrees/agent-acc17965b8910c1f2` (read there, copy what is
right into your tree). Its finding 3 binds you: the ball product of `arb_mul` followed by a sign test
cannot be the kernel of the sign preservation of SPEC 5 (`x = y = 1 +/- (1 - 2^-30)`: the ball product
contains 0, the product set does not); the real kernel works on the end points.

Decisions taken by the orchestrator (TJO: decide and go on; the orchestrator records them in
`docs/SPEC.md` 15): D2-1 results of operations are stored in normal form, constructors keep the modulus
as supplied (CV-17); D2-2 the real kernel works by end points, and `NOT_DETERMINED` is returned when the
result cannot be certified free of 0 at the precision. If the slice needs another decision, take the one
a careful numerical analyst would take, implement it, and list it in the result file with the
alternatives.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold:
you write the headers; rule 5 is replaced by item 5 below), `docs/SPEC.md` 5 and 15, `docs/PLAN.md`
milestone 2, `docs/proofs/ideles.md` (the propositions on cosets, products, inverses; cite file and line
in the code), `docs/conventions.md` 2 to 5.7 and 12, `include/adelefeld/adele.h` and `resid.h` for the
style of a header, `tests/ref/vectors/` and `tests/golden/` for existing vectors of `ucoset` and `idele`.

**You own:** `include/adelefeld/ucoset.h`, `include/adelefeld/idele.h` (new), `src/ucoset.c`,
`src/idele.c` (new), `tests/test_ucoset.c`, `tests/test_idele.c` (new), `tests/julia/ideles.jl` (new),
`proto/ideles_checks.py`, `tests/ref/vectors/i-slice1/` (new), `docs/api-2.md` (new; ONLY the section of
this slice: types, set statements, functions, statuses), `lanes/i-slice1/`. In `include/adelefeld.h` and
`tests/test_julia.sh` you may add the lines your files need and nothing else. Everything else is
read-only. Other lanes work on `src/roots*.c` and on local balls (`lball`).

1. Headers first, with the comment block of every declaration (set statement, proposition with file and
   line, aliasing rule, statuses and the state of the outputs on each). Where `ideles.md` has no statement
   for what you need, write the statement and its proof in `docs/api-2.md`, section "Statements to add".
2. Tests first, red then green (`lanes/i-slice1/redgreen.log`). Oracles: enumeration in `Z/M` for cosets
   (products and inverses of cosets at equal and mixed moduli, the `gcd` rule, an exact factor with
   `gcd(0, N) = N`, the point 1 in `x * x^-1`); vectors written by a script from `proto/ideles_checks.py`;
   for the real part exact rational end points: containment of the product set, sign preserved, the
   example above; negative rationals; huge operands; every aliasing combination; every status.
3. The code. 4. `tests/julia/ideles.jl`: a user multiplies two ideles made from rationals and reads the
   result; run by `tests/test_julia.sh`.
5. Show that the tests bite: three faults of your choice in a scratch copy under `build/`. No mutation
   run, no fuzz target in this slice.
6. `make clean && make check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass; give the last
   line of each.

Result: `lanes/i-slice1/result.md` and the same text as your final message: functions built, decisions
taken, the numbers of the tests and what would have made a case fail, findings, the next slice you
propose.
