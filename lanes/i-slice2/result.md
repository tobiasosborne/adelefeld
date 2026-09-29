# Lane i-slice2: result (idele classes, the class map, valuations, absolute values, norm)

Worktree at adf87c1; `refs/src` linked to the main checkout. Work 2026-09-29 23:42 to 2026-09-30 00:35 (clock).

## What was built

A user makes the idele of a rational, reads its valuations, absolute values and norm, multiplies it by a
rational and maps it to its class, in C and from Julia.

- `include/adelefeld/idclass.h` (new), `src/idclass.c` (new): `adf_idclass` `(t, u)`, 64 bytes (`t` at 0, `u` at
  48, alignment 8). `init` (`<1 ; [1]>`), `clear`, `set`, `swap`, `is_canonical` (`t` finite and positive, `u`
  canonical), `identical`, `set_parts` (`OK`, `DOMAIN`), `set_idele` (the class map `(abs(X)/r, sign(X) u)`,
  ideles.md P15; `OK`, `NOT_DETERMINED`, `LIMIT`), `get_t`, `get_unit`, `norm` (a copy of `t`), `mul`, `inv`
  (kernel B with the sign +1; `OK`, `NOT_DETERMINED`, `LIMIT`), `adf_sizeof_idclass`, `adf_alignof_idclass`.
- `include/adelefeld/idele.h` (additions only), `src/idele.c`: `adf_idele_mul_rat` (`OK`, `NOT_DETERMINED`,
  `NOT_UNIT` for `q = 0`, `LIMIT`), `adf_idele_valuation_at` (`slong`, by `fmpz_remove` on numerator and
  denominator, no factorisation; `DOMAIN` at the real place), `adf_idele_abs_at` (exact `adf_rat` `p^(-v)`;
  `DOMAIN` at the real place), `adf_idele_abs_inf` (the exact ball `[abs(m) +- rho]`, void), `adf_idele_norm`
  (`abs(X)/r` with `1/r` entering exactly through the integers of `r`, one rounding per end; `OK`,
  `NOT_DETERMINED`, `LIMIT`).
- The real kernel of slice 1 is reused, not copied: `ends_of_abs`, `ball_from_ends`, `kernel_defect` of
  `src/idele.c` are now `adf_idele_*` with hidden visibility, declared in `src/idele_internal.h` (new), with the
  new `adf_idele_ends_scale` (Statement F: `lo = RD_p(l a / b)`, `hi = RU_p(h a / b)`, `l a` exact, one
  division rounded). `adf_idclass_set_idele` calls `adf_idele_norm` for `t`.
- **The precision limit (orchestrator's addition to the brief): done.** `ADF_IDELE_PREC_MAX 2097152` in
  `idele.h` (the value of `ADF_ROOTS_REAL_PREC_MAX`); every function of `idele.h` and `idclass.h` with a `prec`
  (`set_rat`, `mul`, `inv` of slice 1; `mul_rat`, `norm`, `idclass_set_idele`, `idclass_mul`, `idclass_inv`)
  returns `ADF_LIMIT` for a larger `prec`, decided from `prec` alone before any allocation and before every
  other status (also before `NOT_UNIT` of `set_rat` and `mul_rat` for `q = 0`: the maximum of conventions 3.3),
  outputs untouched. The rule is a new sentence in the common rules of `idele.h` and in the status line of each
  function. Seen red first: `lanes/i-slice2/limit_repro.c`, `adf_idele_set_rat` of 1/3 at `WORD_MAX`, crashed
  with a segmentation fault (exit 139, under `ulimit -v 4000000`); green after: status 10. A `prec` below 2 is
  still taken as 2 (M1-D4, as slice 1).
- `docs/api-2.md`: new section 2 (types, functions and statuses, "Statements to add" F, G, H, I with stepwise
  proofs, decisions i2-1 to i2-9, the precision limit). Section 1 changed only in the three rows of `set_rat`,
  `mul`, `inv`, which gained `LIMIT` (they were right for slice 1; the change is said in section 2).
- `proto/ideles_checks.py`: part 3 (the reference of slice 2 and `check_api_classes`); `__main__` runs all
  three parts, `part3` alone.
- `lanes/i-slice2/gen_vectors.py` -> `tests/ref/vectors/i-slice2/idele_maps.jsonl` (1097 lines, 477131 bytes)
  and `idclass.jsonl` (922 lines, 404855 bytes): 864 KB in all, below the 1 MB of the brief; regenerated
  identically (sha256 checked).
- Tests `tests/test_idclass.c` (14 tests), `tests/test_idele_maps.c` (10 tests); Julia `tests/julia/idclass.jl`,
  run by a new block of `tests/test_julia.sh` (2c, the same LD_PRELOAD retry as 2b); two lines in
  `include/adelefeld.h` (the include and its line in the list of headers).

## Checks run (commands and results)

| Command | Result |
|---|---|
| `timeout 300 python3 proto/ideles_checks.py` | 16 checks pass (parts 1, 2, 3). Part 3 alone (`part3`, 8.5 s): 1500 ideles, 4600 exact points of the input sets inside the class, 350 `NOT_DETERMINED`, 2037 class products and inverses checked pointwise, valuations and absolute values against trial division, 600 rationals whose norm contains 1 (exact 1 in 207). Two faults injected into the reference (class without the sign; scaling by `(a+1)/b`) each made the check FAIL |
| `./build/test_idclass` | 14 tests, 9738 checks, 0 failed. Vectors: 922 lines (304 class, 309 mul, 309 inv), 301 `NOT_DETERMINED` where the reference says so, 22 exact results; units against enumeration in `Z/M` for every coset with `N <= 20` and both exact units, both signs (class map), and for every coset with `N <= 12` (and the exact units) times one of modulus 0, 3, 6, 9, 12 (product, inverse, all aliasings) |
| `./build/test_idele_maps` | 10 tests, 10466 checks, 0 failed. Vectors: 1097 lines (265 mul_rat, 304 norm, 264 valuation lines with 2154 places, 264 abs_inf), 161 `NOT_DETERMINED`, 28 exact results; "norm of the idele of a rational: 78 exact 1, 6 NOT_DETERMINED (p < 4)" |
| `./build/test_idele` (slice 1, unchanged) | 12 tests, 8613 checks, 0 failed |
| `make BUILD=build/inv INV=1 build/inv/test_idclass build/inv/test_idele_maps build/inv/test_idele` and run | 9738, 10466, 8613 checks, 0 failed; a probe passing a class with `t = -2` to `adf_idclass_mul` aborts with `ADF_CHECK_INVARIANTS: adf_idclass_mul: argument x is not a canonical adf_idclass` |
| `sh tests/test_julia.sh` | testset "idele classes, valuations and the norm through ccall (milestone 2, slice 2)": 43 of 43 pass (log `lanes/i-slice2/julia.log`); `test_julia: passed (with LD_PRELOAD=...)` |
| `make clean && make -j2 check-all` (00:22 to 00:26) | `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`; `check passed: all 62 test programs`; `test_exports: passed: 345 of 345 declared functions are exported, ... no exported name is undeclared` (the kernel symbols are hidden) |
| `make clean && make -j2 check SAN=1` (00:26 to 00:31) | `check passed: all 62 test programs`; 0 lines with `runtime error`, `AddressSanitizer` or `LeakSanitizer` |
| `make clean && make -j2 check CC=clang` (00:31 to 00:33) | `check passed: all 62 test programs`; 0 warnings |
| `sh lanes/m1-headers/check_headers.sh` | `check_headers: passed` (57 single-header compilations, 36 inline functions exported, 345 declarations, 15 in `idclass.h`, 20 in `idele.h`, 0 variadic) |
| `sh lanes/i-slice2/run_faults.sh` (item 5) | four faults, each caught; below |

The four check commands ran after the last edit of any file except this result file and `progress.md`.

What would have made a case fail: a class unit other than `sign(X)` times the unit at any enumerated level; a
real result that misses the exact rational end points (`abs(X)/r`, `abs(X) abs(q)`, `T T'`, `1/T`) or the rounded
ends of the reference, contains 0, has the wrong sign, or is not exact when the reference ends coincide; a
status other than the reference's (the status is a function of correctly rounded end points, Statement F.4);
a valuation other than the count of one division at a time; an absolute value other than `p^(-v)`; a product of
the `|r|_p` other than `1/r`; a norm of the idele of a rational that misses 1, or that is not exactly 1 when the
real ball is exact; a class of `q x` whose unit set differs from that of `x` or whose `t` does not meet it; a
touched output on `DOMAIN`, `NOT_UNIT`, `NOT_DETERMINED`, `LIMIT` (also aliased); an aliased call that differs
from the plain one.

Red and green (`lanes/i-slice2/redgreen.log`): red 0, the crash of the precision limit (above); red 1, link
errors of both test files; red 2, stubs with plausible wrong outputs: test_idele_maps 3868 failed checks in all
10 tests, test_idclass 3952 in 12 of 14 (the layout test, fixed by the header, and the init test, whose stub
set the right value, passed). A test that passed on a first set of do-nothing stubs
(`class_of_q_times_x_is_the_class_of_x`: two zero balls overlap) was given checks of canonical form, enclosure
and the unit by enumeration. The first green run had 3 failed checks, both defects of my tests, repaired
without weakening the contract: (1) the norm of the idele of 5/8 at prec 2 is `NOT_DETERMINED` (the mag radius
of `set_rat` is `(2^29 + 1) 2^-31`, a little above 1/4, so the norm ends are 1/4 and 2, three binades apart);
the contract promises `OK` only when `hi < 2^(p-1) lo`, which holds for the idele of any rational from prec 4
(bound written in the test), so the test requires `OK` from 4 and allows `NOT_DETERMINED` below with the output
untouched; (2) a test rational `-(3^2000 + 1)/2^5000` was not in lowest terms (a non-canonical input); now
`3^2000 + 2`, with a check.

Faults (scratch copies under `build/faults2/`, log `lanes/i-slice2/faults.log`):

| Fault | Caught by |
|---|---|
| G1 the class map without the sign on the unit (`Psi` of P15.3) | test_idclass: 323 failed checks in 6 tests |
| G2 the norm multiplies by `r` instead of dividing | test_idclass 1002 in 7 tests; test_idele_maps 642 in 4 |
| G3 the valuation ignores the denominator of `r` | test_idele_maps: 737 in 4 tests |
| G4 Statement F: the lower end rounded up | test_idclass 190 in 2 tests; test_idele_maps 317 in 3 |

G2 was first caught by test_idele_maps only: `set_idele` had its own copy of the norm. It now calls
`adf_idele_norm` (one code path), and the rerun catches G2 in both programs. No mutation run and no fuzz target
(brief item 5).

## Decisions taken (the table of `docs/api-2.md` 2.4 has the alternatives)

| Id | Taken | Main alternative |
|---|---|---|
| i2-1 | accessors `adf_idclass_get_t`, `adf_idclass_get_unit` (verb first, as `adf_idele_get_unit`) | `adf_idclass_t_get`, `adf_idclass_unit_get` of conventions 5.7 |
| i2-2 | real part of norm, class and `mul_rat`: E1 ends, one correct rounding of `l a / b`, kernel B | `arb_mul_fmpz`/`arb_div_fmpz` and a sign test; exact ends without E1 (unbounded memory) |
| i2-3 | the norm has the class kernel: a certified positive ball or `NOT_DETERMINED` | always `OK`, a ball that may contain 0 |
| i2-4 | valuation as `slong *`; `DOMAIN` at the real place | `fmpz` output; 0 or a flag at the real place |
| i2-5 | `abs_at`: exact `adf_rat` at a prime, `DOMAIN` at the real place; `abs_inf`: exact ball | one `arb` output for all places (rounds `p^(-v)`) |
| i2-6 | `adf_idclass_norm(t, x)`: a copy of `t`, void | no such function |
| i2-7 | class unit `adf_ucoset_mul(u, [sign X])`, normal form | `u` with `c` negated, modulus as stored |
| i2-8 | `mul_rat` by 0: `NOT_UNIT` | `DOMAIN` |
| i2-9 | kernel shared through `src/idele_internal.h` (hidden symbols) | a copy in `src/idclass.c` |
| (orchestrator) | `ADF_IDELE_PREC_MAX = 2^21`, `LIMIT` first, before any allocation | none offered |

From the unreviewed d-ideles reference (`agent-acc17965b8910c1f2`, `proto/ideles_checks.py`) I took: the formulas
of the class map (`t = abs(X)/r`, unit `sign(X) u`, its lines 1045-1050), of `mul_rat` (content `r abs(q)`, unit
`u [sign q]`, real part by scaling, 1022-1029, 879-888), the valuation by counting and `p^(-v)` (1053-1061), and
the shape of its checks (pointwise `Phi`, unit set equal to `sign * u`, invariance under `Q^x`, valuations
against factorisation, 1131-1237). I proved each of these again in Statements F to I before use. Not taken:
its `DOMAIN` for a composite `p` (a place cannot hold one), its scaling by a Fraction (here integers `a/b`, M1-D4),
powers, hulls and division.

## Findings

Against the specification (none is a mathematical error):
1. conventions 5.7 names the class accessors `adf_idclass_t_get`, `adf_idclass_unit_get`; slice 1 already used
   `adf_idele_get_real`, `adf_idele_get_unit`. I followed slice 1 (i2-1). One of the two should be changed.
2. conventions 3.2, row "Unit coset, idele, idele class arithmetic", lists `OK`, `NOT_DETERMINED`, `NOT_UNIT`;
   the orchestrator's precision limit adds `LIMIT` to every function with a `prec` in this row. The row should
   say so.
3. Finding 1 of i-slice1 (PLAN 2.1 `M_k = N` against P13.4 `Nbar`) stands; not touched (powers are not in this
   slice).

Notes (not findings against the specification):
4. The Python reference models a `mag` radius as `ru(x, 30)`; FLINT's radius may be a few ulps larger
   (`mag.rst:15`). Single operations have the same status in C and reference (the vectors check every one), but a
   chain of two (a C result fed to a second function) can differ at small `prec`: the norm of the C idele of 5/8
   at prec 2 is `NOT_DETERMINED`, that of the reference's idele is `OK`. Vectors therefore feed reference
   inputs, never chained results.
5. `arb_get_str` prints the norm of the idele of -6/35 at 64 bits as `[1.00000000000000 +/- 3.4e-19]`, fine here;
   the text form of classes (not in this slice) needs the constrained printing of conventions 9.5 for wide balls
   (i-slice1, note 3).

Avoidable costs (COMMON-C rule 7): `valuation_at` removes `p` from both numerator and denominator, although a
non-zero count in one implies 0 in the other (`gcd = 1`), and it keeps a quotient it discards; `abs_at` recomputes
the valuation; the class map and `mul_rat` multiply by an exact unit through `adf_ucoset_mul` (a gcd with 0 and
a normal form) where a negation modulo `N` would do; Statement F forms `l a` for `a = 1` too; `set_idele` checks
the limit and the invariant once more inside `adf_idele_norm`.

## Not done

- Powers (`pow`, `pow_tight`, `M_k`), hulls and the map to adeles, division, text and dump forms of the three
  types, set predicates of classes (`equal_set`, `contains`, `overlaps`), characters of the class group.
- `docs/api-2.md` section 2 and the statements F to I are not reviewed.
- No mutation run, no fuzz target (brief item 5).

## Files

New: `include/adelefeld/idclass.h`, `src/idclass.c`, `src/idele_internal.h`, `tests/test_idclass.c`,
`tests/test_idele_maps.c`, `tests/julia/idclass.jl`, `tests/ref/vectors/i-slice2/idele_maps.jsonl`,
`tests/ref/vectors/i-slice2/idclass.jsonl`, `lanes/i-slice2/{gen_vectors.py, run_faults.sh, limit_repro.c,
redgreen.log, progress.md, result.md}` and the logs `check-all.log`, `check-san.log`, `check-clang.log`,
`check-headers.log`, `faults.log`, `julia.log`, `proto.log`, `gen_vectors.log`.
Changed: `include/adelefeld/idele.h` (additions, the limit), `src/idele.c`, `docs/api-2.md`,
`proto/ideles_checks.py`, `include/adelefeld.h` (two lines), `tests/test_julia.sh` (block 2c).
