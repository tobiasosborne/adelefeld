# Lane i-slice2: idele classes, valuations, absolute values, norm (milestone 2, WP 2.1 in part, 2.2, 2.3 in part)

The second vertical slice of milestone 2, on top of lane i-slice1 (on master: `ucoset.h`, `idele.h`,
`docs/api-2.md` section 1, `lanes/i-slice1/result.md`). A user takes a rational, makes its idele, reads
its valuations, absolute values and norm, and maps it to its class.

Functions of the slice (the final list is yours):
- `adf_idclass`: life cycle, `is_canonical`, `identical`, set from parts, accessors, `mul`, `inv`,
  layout queries; the map idele to class with the sign on the unit (`docs/proofs/ideles.md`
  Proposition 15; PLAN 2.3).
- `adf_idele_mul_rat` (an idele times an exact nonzero rational).
- PLAN 2.2: the valuation of an idele at a named prime (by divisibility, no factorisation), the
  absolute value at a named place, the norm; "norm exact before rounding" (PLAN 2.2 tests).
- Not in this slice: powers (`M_k`), hulls and the map to adeles, division, text and dump forms.

The real kernel of i-slice1 (kernel B, end points, `NOT_DETERMINED` by the exponent gap) is reused, not
copied: if it is static in `src/idele.c`, move its declaration to a private header `src/idele_internal.h`.
The unfinished design lane d-ideles left an UNREVIEWED reference for these functions in the worktree
`/home/tobias/Projects/adelefeld/.claude/worktrees/agent-acc17965b8910c1f2` (`proto/ideles_checks.py`
part 2: class map, norms); read it, take what you can prove, and say what you took.

Decisions are yours where the specification is silent (TJO: decide and go on): take the one a careful
numerical analyst would take, implement it, and list it in the result file with the alternatives, in
the form of the table of `docs/api-2.md` 1.4. Finding 1 of i-slice1 stands (PLAN 2.1 says `M_k = N` for
`k = 1, -1`; `ideles.md` P13.4 says `Nbar`): the proof file holds.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold:
you write the headers; rule 5 is replaced by item 5 below), `docs/SPEC.md` 5 and 15, `docs/PLAN.md`
milestone 2, `docs/proofs/ideles.md` (cite file and line in the code), `docs/conventions.md` 2 to 5.7, 6,
7 and 12, the headers and the result file of i-slice1.

**You own:** `include/adelefeld/idclass.h` (new), `include/adelefeld/idele.h` (additions only; no change
of an existing declaration), `src/idclass.c` (new), `src/idele.c`, `src/idele_internal.h` (new),
`tests/test_idclass.c`, `tests/test_idele_maps.c` (new), `tests/julia/idclass.jl` (new),
`proto/ideles_checks.py`, `tests/ref/vectors/i-slice2/` (new, below 1 MB), `docs/api-2.md` (a new
section 2; section 1 only where it is wrong, and say so), `lanes/i-slice2/`. In `include/adelefeld.h`
and `tests/test_julia.sh` you may add the lines your files need and nothing else. Everything else is
read-only. Other lanes work on roots, local balls and partial balls; one lane reviews `src/ucoset.c`
and `src/idele.c` as they are on master.

1. Headers first, with the comment block of every declaration (set statement, proposition with file and
   line, aliasing rule, statuses and the state of the outputs on each). Missing statements with proofs
   go to `docs/api-2.md`, "Statements to add".
2. Tests first, red then green (`lanes/i-slice2/redgreen.log`). Oracles: enumeration in `Z/M`; exact
   rational arithmetic for valuations, absolute values and the norm (the product formula on rationals:
   the norm of the idele of a rational is 1, exactly); negative rationals; the exact units; the class
   of `q` times an idele equals the class of the idele; every aliasing combination; every status.
3. The code. 4. `tests/julia/idclass.jl`: a user makes the idele of `-6/35`, reads the valuations at 2,
   3, 5, 7, 11, the absolute values, the norm, and its class; run by `tests/test_julia.sh`.
5. Show that the tests bite: four faults of your choice in a scratch copy under `build/`. No mutation
   run, no fuzz target.
6. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass; give the last
   line of each. These runs take up to 5 minutes each: run them under `timeout 900`.

Result: `lanes/i-slice2/result.md` and the same text as your final message.
