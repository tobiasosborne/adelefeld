# Lane i-slice3: powers, hulls, division (milestone 2: the rest of WP 2.1, 2.3, 2.4)

The third vertical slice of milestone 2, on top of lanes i-slice1 and i-slice2 (on master:
`ucoset.h`, `idele.h`, `idclass.h`, `src/idele_internal.h`, `docs/api-2.md` sections 1 and 2). With it
the milestone is complete except for the text and dump forms.

Functions of the slice (the final list is yours):
- Powers (PLAN 2.1, M0-D6): of a unit coset, an idele and a class. The default enclosure `c^k U(N)`, and
  the smallest coset `chat^k U(M_k)` of `docs/proofs/ideles.md` Proposition 13 as a separately named
  operation; the exponent 0 gives the exact unit; negative exponents. `ideles.md` P13.4 holds against
  PLAN 2.1 (`M_k = Nbar` for `k = 1, -1`; finding 1 of i-slice1). A limit on the bits of `r^k`
  (`LIMIT`, decided before the power is formed).
- PLAN 2.3: the map idele to adele with both hulls (the simple hull and the smallest hull; the
  unsuffixed name is the smallest hull); an exact unit gives the exact rational `r c`.
  `adf_idele_set_adele` (an adele to an idele, with the status when the adele does not certify a unit:
  `UNIT_NOT_CERTIFIED`, `NOT_UNIT`; read SPEC 4.5 and 5).
- PLAN 2.4: the division of an adele by an idele (the smallest ball of `ideles.md` Proposition 19,
  M0-D7) and by an exact rational; the status when the divisor is only an adele. No `adf_adele_div` of
  two adeles.
- Set predicates of ideles and classes (`equal_set`, `contains`, `overlaps`) if the conventions ask
  them of every value type (2.3 there): then they belong here.

The unfinished design lane d-ideles left an UNREVIEWED reference for these functions in the worktree
`/home/tobias/Projects/adelefeld/.claude/worktrees/agent-acc17965b8910c1f2` (`proto/ideles_checks.py`
part 2: `pow_tight`, hulls, division; its progress notes name a Statement E3 on powers that "needs
review before code"): read it, take what you can PROVE, write the proofs in `docs/api-2.md`, and say
what you took. Decisions where the specification is silent are yours (TJO: decide and go on): the one
a careful numerical analyst would take, listed with the alternatives in the form of `docs/api-2.md` 2.4.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold:
you write the headers; rule 5 is replaced by item 5 below), `docs/SPEC.md` 4.5, 5, 15, `docs/PLAN.md`
milestone 2, `docs/proofs/ideles.md` Propositions 13 to 19 (cite file and line in the code),
`docs/conventions.md` 2 to 5.7, 6, 12, `docs/api-2.md`, the result files of i-slice1 and i-slice2,
`include/adelefeld/adele.h`, `fball.h`.

**You own:** `include/adelefeld/idpow.h`, `include/adelefeld/idmap.h` (new), `src/idpow.c`, `src/idmap.c`
(new), `include/adelefeld/ucoset.h`, `idele.h`, `idclass.h` (additions only: predicates; no existing
declaration changes), `src/ucoset.c`, `src/idele.c`, `src/idclass.c` (additions), `src/idele_internal.h`,
`tests/test_idpow.c`, `tests/test_idmap.c` (new), `tests/julia/idmap.jl` (new), `proto/ideles_checks.py`
(a new part 4), `tests/ref/vectors/i-slice3/` (new, below 1 MB), `docs/api-2.md` (a new section 3),
`lanes/i-slice3/`. In `include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files
need. Everything else is read-only (`src/adele.c`, `src/fball.c` in particular: use their public
functions).

1. Headers first, with the comment block of every declaration. Every function with a `prec` keeps the
   limit `ADF_IDELE_PREC_MAX` as i-slice2 did.
2. Tests first, red then green (`lanes/i-slice3/redgreen.log`). Oracles: powers against enumeration
   modulo small `M_k` (squares: `M_2 = 24` for `N = 1`), against repeated multiplication (containment)
   and for the smallest coset tightness by enumeration; hulls: containment of every point of the idele
   (enumerated modulo a small modulus, real end points exact), tightness of the small hull; the
   division against multiplication by the inverse, the radius `gcd(|a| lcm(N, 2), M)/r` against
   enumeration, with `a = 0`, `M = 0`, odd `N`; negative rationals; exact units; every aliasing
   combination; every status.
3. The code. 4. `tests/julia/idmap.jl`: a user makes the idele of `-6/35`, squares it, maps it to an
   adele, divides the adele of `1/2` by it, and reads the results; run by `tests/test_julia.sh`.
5. Show that the tests bite: five faults of your choice in a scratch copy under `build/`. No mutation
   run, no fuzz target.
6. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`, in the FOREGROUND; give the
   last line of each.

Result: `lanes/i-slice3/result.md` and the same text as your final message. Times in your notes are
read from `date`. Leave no compiled binary in your lane directory.
