# Lane f-slice2: partial balls `adf_sball` and the archimedean wrappers (milestone 1F, WP 1F.1 and 1F.2)

The second vertical slice of milestone 1F. A user takes an adele, projects it to a named set of places,
and applies a function at the real place; the result names its places.

Functions of the slice (names after `docs/conventions.md` 2, 5.9 and 7; the struct of `adf_sball` is fixed
in 5.9; the final list is yours):
- `adf_sball`: init, clear, set, swap, `is_canonical`, `identical`, `equal_set`, `contains`, `overlaps`,
  accessors (the number of places, the place `i`, the component at a place as `adf_lball` or as the real
  ball), `adf_sizeof_sball`, `adf_alignof_sball`.
- The projection of an `adf_adele` to a list of places (primes and the real place), with `adf_lball`
  components made by `adf_lball_set_fball` (lane f-slice1, on master).
- Componentwise `add`, `sub`, `neg`, `mul` of partial balls over the SAME set of places (another set is
  `DOMAIN`).
- The real wrappers (WP 1F.2), on the real component of an `adf_sball` with the real tag, thin wrappers
  around `arb`: `exp`, `log`, `log_abs`, `sin`, `cos`, `sqrt`, `root` of odd and even degree. A failure at
  a place is reported with that place and no value is written (PLAN 1F.1). Read SPEC 9 for the statuses:
  a ball that crosses 0 under `log`, a negative ball under `sqrt`, a ball that contains 0 under an even
  root. FLINT facts already probed (unreviewed notes of lane d-functions, worktree
  `/home/tobias/Projects/adelefeld/.claude/worktrees/agent-adaf36d4414085b09/lanes/d-functions/`):
  `arb_root_ui(-8, 3)` and `arb_root_ui(0, 3)` are NaN, `arb_sqrt(0) = 0`. The wrapper never returns a
  NaN or an infinite ball with `OK`.
- Not in this slice: complex components, functions at primes (`exp`, `log` in `Q_p`), special functions,
  text and dump forms.

Decisions taken by the orchestrator (TJO: decide and go on): the limits of lane f-slice1
(`ADF_LBALL_EXP_MAX`, `ADF_LBALL_BITS_MAX`) and its statuses are accepted; the odd root of a negative
real ball is computed as minus the root of the negated ball; the list of places of an `adf_sball` is
stored sorted with the real place last, without repetition (unless conventions 5.9 or 7 say otherwise:
then they hold). If the slice needs another decision, take the one a careful numerical analyst would
take, implement it, and list it in the result file with the alternatives.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold:
you write the header; rule 5 is replaced by item 5 below), `docs/SPEC.md` 4, 9, 15, `docs/PLAN.md`
milestone 1F, `docs/proofs/functions.md` (the statements on the real functions and guard digits; cite
file and line), `docs/conventions.md` 2 to 5.9, 7, 12, `include/adelefeld/lball.h`, `adele.h`, `place.h`,
`docs/api-1f.md`, `lanes/f-slice1/result.md`, FLINT's `arb.rst` under `refs/src/flint-3.0.1/` for every
`arb` function you call (file and line in the code).

**You own:** `include/adelefeld/sball.h`, `include/adelefeld/rfunc.h` (new), `src/sball.c`, `src/rfunc.c`
(new), `tests/test_sball.c`, `tests/test_rfunc.c` (new), `tests/julia/sball.jl` (new),
`proto/functions_checks.py` (a new section at its end), `tests/ref/vectors/f-slice2/` (new, below 1 MB),
`docs/api-1f.md` (a new section for this slice; do not change the section of slice 1),
`lanes/f-slice2/`. In `include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files
need and nothing else. Everything else is read-only; `lball.h` and `src/lball.c` are under review by
another lane: report what you find wrong in them, do not change them.

1. Headers first, with the comment block of every declaration (set statement, proposition with file and
   line, aliasing rule, statuses and the state of the outputs on each).
2. Tests first, red then green (`lanes/f-slice2/redgreen.log`). Oracles: for the projection, enumeration
   (every point of the adele's finite ball modulo a small modulus lies in the component at each prime,
   and the component is the smallest such ball); for the real functions, exact rational end points and
   `mpmath` interval arithmetic at a higher precision: the result contains the image of both end points
   and of the midpoint, the radius is within a stated factor of the true width, monotone functions are
   checked at the end points; balls crossing 0 and the branch points; every status; every aliasing
   combination; a failure at one place names the place.
3. The code. 4. `tests/julia/sball.jl`: a user makes the adele of `2/3`, projects it to the places
   `{2, 5, real}`, takes `log` at the real place and reads the three components; run by
   `tests/test_julia.sh`.
5. Show that the tests bite: five faults of your choice in a scratch copy under `build/`. No mutation
   run, no fuzz target in this slice.
6. `make clean && make check-all` (single job is allowed), `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass; give the last
   line of each. These suite runs may take 5 minutes each: run them under `timeout 900`.

Result: `lanes/f-slice2/result.md` and the same text as your final message: functions built, decisions
taken, the numbers of the tests and what would have made a case fail, findings, the next slice you
propose.
