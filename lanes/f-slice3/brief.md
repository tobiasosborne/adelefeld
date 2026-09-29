# Lane f-slice3: local balls completed (milestone 1F, the rest of WP 1F.3)

The rest of work package 1F.3, on top of `adf_lball` as it is on master after lane f-repair1.

Functions of the slice (the final list is yours):
- The full decomposition of `docs/proofs/functions.md` Proposition 4: `x = p^m w u` with `w` the
  Teichmueller factor (odd `p`) or the sign (`p = 2`) and `u` a principal unit; determined when the
  relative precision suffices (say exactly when, from the proposition), else `NOT_DETERMINED`. The
  Teichmueller factor is returned as a ball at the precision asked, computed by the lifting the proof
  file names (cite it), and also as its index: the residue modulo `p` (modulo 4 at `p = 2`).
- The `p`-primary fractional part `{x}_p` of Proposition 19, as an exact rational in `[0, 1)` with
  denominator a power of `p`; `NOT_DETERMINED` when the ball does not determine it (precision below 0).
- Unit-part accessors that a caller of the Hilbert symbols will need: the unit modulo `p^k` as `fmpz`.
- Integer powers `adf_lball_pow_si` (smallest ball; the exponent 0 gives the exact 1; negative exponents
  through the inverse; statement with proof: the radius of `x^k` for a ball, with the factor `v_p(k)`).
- The repair of finding 1 of lane f-slice2 (`lanes/f-slice2/result.md`): `adf_lball_set_fball` needs 13 s
  to return `LIMIT` for a radius of 87 million bits, because the valuation is formed before the limit is
  decided. Decide `LIMIT` from bit lengths first where that is sound; say what bound you prove.

Decisions where the specification is silent are yours (TJO: decide and go on): take the one a careful
numerical analyst would take, implement it, list it in the result file with the alternatives.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold
for `lball.h`; rule 5 is replaced by item 4 below), `docs/proofs/functions.md` Lemma 3, Proposition 4,
Proposition 19 (cite file and line in the code), `docs/SPEC.md` 9.3.5, 9.3.6 and 15,
`docs/conventions.md` 5.8, `docs/api-1f.md`, `include/adelefeld/lball.h`, `src/lball.c`,
`docs/reviews/f1/review-lball.md` and `lanes/f-repair1/result.md` (the rule on `LIMIT`).

**You own:** `include/adelefeld/lball.h` (additions; an existing declaration is not changed),
`src/lball.c`, `src/lball_decomp.c` (new), `tests/test_lball_decomp.c` (new), `tests/test_lball.c`
(additions for `set_fball`), `tests/julia/lball2.jl` (new), `proto/functions_checks.py` (a new section
"f-slice3" at its end), `tests/ref/vectors/f-slice3/` (new, below 1 MB), `docs/api-1f.md` (a new section
"Slice 1F.3-b" at the end; nothing else), `lanes/f-slice3/`. In `tests/test_julia.sh` you may add the
lines your file needs. Everything else is read-only. Another lane writes the local `exp` and `log` in
new files and reads `lball.h`; another reviews `src/lball.c` as it is on master.

1. Header first, with the comment block of every declaration. Missing statements with stepwise proofs
   go to your section of `docs/api-1f.md`.
2. Tests first, red then green (`lanes/f-slice3/redgreen.log`). Oracle: enumeration modulo small prime
   powers (`p` = 2, 3, 5, 7, 11): for every ball and every point of it (several digits), the
   decomposition of the point by brute force (the Teichmueller representative as the limit of
   `a^(p^n)`, computed modulo `p^N` by your own powering) lies in the returned factors, and the factors
   are the smallest balls; `w^(p-1) = 1` to the precision; the fractional part against exact rational
   arithmetic; powers against repeated multiplication and against enumeration (tightness); the prime
   `2^64 - 59`; every status; every aliasing combination.
3. The code. `tests/julia/lball2.jl`: a user decomposes `50/3` at `p = 5` and `-12` at `p = 2`, reads
   `m`, `w`, `u`, the fractional part of `7/25` at 5, and `(1/3)^-7` at 5.
4. Show that the tests bite: four faults of your choice in a scratch copy under `build/`. No mutation
   run, no fuzz target.
5. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`; give the last line of each.

Result: `lanes/f-slice3/result.md` and the same text as your final message.
