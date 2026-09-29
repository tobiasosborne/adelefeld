# Lane i-slice1: result (ideles, first vertical slice of milestone 2)

Worktree at f0a23fc; `refs/src` linked to the main checkout. Work 22:46 to 23:35 (clock).

## What was built

A user can make unit cosets and ideles, multiply and invert them, and read the result, in C and from Julia.

- `include/adelefeld/ucoset.h`, `src/ucoset.c`: `adf_ucoset` `(c, N)` with the exact units `N = 0`
  (M0-D1). `init` (= `[1]`), `clear`, `set`, `swap`, `is_canonical`, `is_normal`, `identical`,
  `set_fmpz2` (c reduced into `1..N`, N kept as supplied; `DOMAIN` otherwise), `one`, `minus_one`,
  `get_fmpz2`, `is_exact`, `normalise`, `equal_set`, `contains`, `overlaps`, `mul`, `inv`,
  `adf_sizeof_ucoset`, `adf_alignof_ucoset`. Results of `mul`, `inv`, `normalise` are in normal form (D2-1).
- `include/adelefeld/idele.h`, `src/idele.c`: `adf_idele` `(inf, r, u)`. `init` (the exact idele 1),
  `clear`, `set`, `swap`, `is_canonical`, `identical`, `set_parts` (`OK`, `DOMAIN`), `set_rat` (`OK`,
  `NOT_UNIT`), `mul`, `inv` (`OK`, `NOT_DETERMINED`), `get_real`, `content`, `get_unit`,
  `adf_sizeof_idele`, `adf_alignof_idele`. Layouts: ucoset 16 bytes, idele 80 bytes (inf 0, r 48, u 64),
  alignment 8.
- The real kernel of `mul`, `inv` and `set_rat` (D2-2): directed-rounded end points of the absolute value
  (`arf` correct rounding), then a ball built from them (kernel B: B1 `NOT_DETERMINED` when
  `e(hi) - e(lo) > p`; B2 exact; B3 midpoint `RN_p((lo+hi)/2)`; B4 midpoint `lo + rho'` with lower end
  exactly `lo`). `arb_mul` is not used. A result that fails its own sign test aborts (never returned).
- `docs/api-2.md` section 1: types, set statements, functions, statuses; "Statements to add" A (exact units),
  B (normal form), C (product and inverse as computed), D (the set of an idele value; product, inverse,
  idele of a rational), E (the real kernel: end points, ball from ends, size of the midpoint `<= 2 p + 30`
  bits, what the status says), each with a stepwise proof; table of the decisions of the slice.
- `include/adelefeld.h`: two includes and two comment lines. `tests/test_julia.sh`: a block that runs
  `tests/julia/ideles.jl` (with the same LD_PRELOAD retry); no existing line changed.
- `proto/ideles_checks.py`: part 2 appended (unit cosets, kernel B, idele mul/inv/set_rat), adapted from
  the unreviewed d-ideles part 2 (its coset functions and kernel; the kernel B3 here uses
  `max(hi - m, m - lo)`; `ref_uc_gcd` makes `gcd(0, N') = N'` explicit). `__main__` runs part 1 and part 2.
- `lanes/i-slice1/gen_vectors.py` -> `tests/ref/vectors/i-slice1/ucoset.jsonl` (1803 lines),
  `idele.jsonl` (1758 lines).
- Tests `tests/test_ucoset.c`, `tests/test_idele.c`; Julia `tests/julia/ideles.jl`.

## Checks run (commands and results)

| Command | Result |
|---|---|
| `timeout 180 python3 proto/ideles_checks.py` | 15 checks pass, 69 s. Part 2: 4356 ordered coset pairs (66 cosets, 2 exact) against enumeration at 3 levels, 4356 products and 66 inverses at 2 levels; 12000 kernel results (end points, enclosure, sign, size), 2823 `NOT_DETERMINED`, promised-OK zone 8834, promised-ND zone 2653; 3000 idele mul/inv, 9560 exact points inside |
| `./build/test_ucoset` | 11 tests, 43178 checks, 0 failed. Enumeration: 4356 ordered pairs at 3 levels, 8712 product sets, 132 inverse sets; vectors: 373 set (152 `DOMAIN`), 1386 mul with 3 predicates, 44 inv |
| `./build/test_idele` | 12 tests, 8613 checks, 0 failed. Vectors: 260 set_rat, 749 mul, 749 inv; 489 `NOT_DETERMINED` exactly where the reference says; 82 exact results; largest midpoint 3 bits below `2 p + 30` |
| `make clean && make -j2 check-all` | `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest` (59 programs; `test_exports: passed: 297 of 297 declared functions are exported`); 4 min 9 s |
| `make clean && make -j2 check SAN=1` | `check passed: all 59 test programs`; no sanitizer report in the log; 5 min 3 s |
| `make clean && make -j2 check CC=clang` | `check passed: all 59 test programs`; 2 min 38 s |
| `sh lanes/m1-headers/check_headers.sh` | `check_headers: passed` (51 single-header compilations, 32 inline functions exported, 297 declarations, 0 variadic) |
| `make BUILD=build/inv INV=1 build/inv/test_ucoset build/inv/test_idele` and run | both pass (43178 and 8613 checks); a probe passing `(4, 6)` to `adf_ucoset_mul` aborts with `ADF_CHECK_INVARIANTS: adf_ucoset_mul: argument x is not a canonical adf_ucoset` |
| `sh tests/test_julia.sh` | testset "ideles through ccall (milestone 2, slice 1)": 23 of 23 pass; `test_julia: passed (with LD_PRELOAD=...)` |
| `sh lanes/i-slice1/run_faults.sh` (item 5) | four faults, each caught; see below |

The four check commands of item 6 ran before three last edits: comment text in the two headers (the
wording of the invariant rule), the wrapping of one line in `tests/test_julia.sh` and two in `ideles.jl`.
After them `check_headers.sh`, the two tests, `test_julia.sh` and the fault script were run again and pass.

Red and green: `lanes/i-slice1/redgreen.log`. Each test file first failed to link, then ran against a stub
(every function a no-op): test_ucoset 26550 failed checks in 10 of 11 tests, test_idele 7660 in 11 of 12;
only the layout tests passed (the layout is the header's). Two defects of my own tests were found on the way
(a Julia prefix check of a printed decimal string; an exact-end oracle called on an exponent of 10^21, where
`arf_get_fmpq` aborts, `arf.rst:310-314`); both oracles were replaced, not weakened (the second by a
multiplication back that must contain 1).

What would have made a case fail: a product or inverse coset that differs at any level from the enumerated
product set; an exact result where a coset is due or the converse (levels with an odd prime give every coset
at least 2 points); a stored pair other than the reference's normal form; any predicate differing from
inclusion/intersection/equality of the images; a status other than the reference's (the status is a function
of correctly rounded end points, so the reference computes it exactly); a result ball that misses the exact
rational end points `sign L`, `sign H` or the rounded `lo`, `hi`, contains 0, has the wrong sign, or is not
exact when `lo = hi`; a touched output on `DOMAIN`, `NOT_UNIT`, `NOT_DETERMINED` (also aliased); an aliased
call that differs from the plain one.

Faults (scratch copies under `build/faults/`, log `lanes/i-slice1/faults.log`):

| Fault | Caught by |
|---|---|
| F1 lower end `abs(m) - rho` rounded up | test_idele: 678 failed checks (vectors only) |
| F2 B1 at `> p + 1` instead of `> p` | test_idele: 73 (spec5 test at p = 60, vectors) |
| F3 no halving of `N = 2 mod 4` in the normal form | test_ucoset 1352 in 5 tests, test_idele 176 |
| F4 the rejected design: `arb_mul` then `arb_is_nonzero` | test_idele: 421 (spec5 164, huge exponents, vectors) |

No mutation run and no fuzz target (brief item 5).

## Decisions taken (alternatives in `docs/api-2.md` 1.4)

- D2-1 and D2-2 of the orchestrator applied as given.
- i1-1: `NOT_DETERMINED` exactly when `e(hi) - e(lo) > p` on the rounded end points, tested first. The status
  is then a function of the inputs that the reference reproduces bit for bit; `OK` whenever
  `hi < 2^(p-1) lo`, `NOT_DETERMINED` whenever `hi >= 2^(p+1) lo`. Alternative: try B3 first and refuse only
  where B4 would be needed (valid balls in a few more cases, but a status depending on `mag` rounding).
  SPEC 5 example: `NOT_DETERMINED` for `p <= 60`, `OK` from 61 (C and reference agree).
- i1-2 `adf_idele_set_parts` (not `set_arb_fmpq_ucoset`); i1-3 the content as `fmpq_t` (not `adf_rat_t`);
  i1-4 `set_parts` canonicalises any `fmpq` with a non-zero denominator; i1-5 `set_rat` uses kernel B on
  `RD_p`, `RU_p` of `abs(q)` (never `NOT_DETERMINED`, E4), not `arb_set_fmpq`; i1-6 a kernel defect aborts
  (as S-D20).
- Beyond the brief's list: `adf_ucoset_overlaps` (P9.3, cheap), `adf_ucoset_normalise`,
  `adf_ucoset_is_normal` (named in conventions 5.6), `adf_ucoset_is_exact`, `adf_ucoset_one`,
  `adf_ucoset_minus_one`. Under `-DADF_CHECK_INVARIANTS` every function that reads a value checks it, except
  `is_canonical` and `is_normal` (local macros in the two source files, using `adf_inv_fail`).
- `gcd(0, N') = N'` is coded explicitly, not taken from `fmpz_gcd`, whose documentation says the result "is
  always positive" (`fmpz.rst:1042-1044`), which cannot hold for `gcd(0, 0)`.

## Findings

Against the specification and plan:
1. `PLAN.md` 2.1 (tests column) says "`k = 1, -1` give `M_k = N`"; `ideles.md` P13.4 (line 284) says
   `M_k = Nbar`. For `N = 6`, `M_1 = 3`. Reported first by the design lane d-ideles (its F1); I read it off the
   table of P13 (`a_p = v_p(Nbar)`) and did not run it, since powers are not in this slice.
2. SPEC 5, "Sign preservation": confirmed in C: `arb_mul` of `x = 1 +- (1 - 2^-30)` with itself contains 0 at
   precisions 64, 128, 1024, 100000 (`spec5_sign_preservation`, and from Julia at 128).

Notes for later slices (not findings against the specification):
3. `arb_get_str(z, 15, 0)` prints the certified positive product of the SPEC 5 example at prec 128 as
   `[+/- 4.01]`, which hides the sign. The text form of ideles needs the constrained printing that
   conventions 9.5 and `tests/golden/idele.tsv` ask for; `arb_get_str` alone does not give it.
4. `arf_get_fmpq` aborts for exponents near `10^21` (`arf.rst:310-314`): an oracle of exact end points cannot
   be used at such exponents; the library itself never calls it.

Avoidable costs (COMMON-C rule 7): `equal_set` and `contains` copy and normalise both cosets (two
temporaries); `mul` takes a gcd even for equal moduli; B3 forms two exact differences where one bound would
do; `set_parts` always canonicalises `r` (a gcd) even when it is canonical.

## Not done

- `adf_idclass`, powers (`pow`, `pow_tight`), norms and valuations (2.2), hulls (2.3), division (2.4),
  `adf_idele_mul_rat`, text and dump forms.
- `docs/api-2.md` section 1 is not reviewed; the orchestrator records D2-1, D2-2 in `docs/SPEC.md` 15.
- The rest of the d-ideles part 2 (pow_tight by its Statement E3, class map, norms, hulls, division, its
  findings) was not copied: it is for later slices and stays unreviewed in that worktree.

## Next slice proposed

Slice 2: the class map and exact scalings, which reuse kernel B with an exact rational: `adf_idclass`
(init, set, predicates, `mul`, `inv`) and `adf_idele_class` (`t = abs(x_inf)/r`, `u' = sign(x_inf) u`,
P15), `adf_idele_mul_rat`, and the valuation, absolute value and norm of 2.2 (`abs(x_inf)/r` exact before
rounding). Oracle: exact points and the kernel end points as here. Then slice 3: powers with `M_k` (P13; the
design lane's Statement E3 needs a review before code), and slice 4: hulls and division (P16, P19).
