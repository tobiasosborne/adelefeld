# Lane i-review2: adversarial review of milestone 2 (ideles), slices 2 and 3

Your task is to REFUTE, not to confirm. Milestone 2 is implemented except for the text and dump forms.
Slice 1 (unit cosets, ideles, `mul`, `inv`, `set_rat`) has had a bug hunt (`lanes/i-review1/result.md`: no
defect; do not repeat it). Under review now, all written by Claude Opus:
- slice 2 (`lanes/i-slice2/result.md`): `include/adelefeld/idclass.h`, `src/idclass.c`; in `idele.h` and
  `src/idele.c`: `adf_idele_mul_rat`, `valuation_at`, `abs_at`, `abs_inf`, `norm`, the limit
  `ADF_IDELE_PREC_MAX`; `src/idele_internal.h`; `docs/api-2.md` section 2 (statements F to I);
- slice 3 (`lanes/i-slice3/result.md`): `include/adelefeld/idpow.h`, `idmap.h`, `src/idpow.c`,
  `src/idmap.c`; `docs/api-2.md` section 3 (statements J to O);
- the tests `tests/test_idclass.c`, `test_idele_maps.c`, `test_idpow.c`, `test_idmap.c`, and parts 3 and 4
  of `proto/ideles_checks.py`.
Contract: the headers; `docs/proofs/ideles.md` (Propositions 13 to 19 in particular);
`docs/conventions.md` 5.6, 5.7, 3; `docs/SPEC.md` 4.5, 5, 15 (15.4: N-D6, N-D8).

**You own:** `lanes/i-review2/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`, none longer than 3 minutes. Build with
`make -j2 BUILD=lanes/i-review2/build` and link your programs against that archive.

## What counts as a finding

- Powers: a point of `X^k` (a point of the coset to the power `k`, a point of the real ball to the
  power `k`) outside the result; for the tight power a result that is not the smallest coset (your own
  enumeration in `(Z/M)^*`, with `M` up to 10^5, all `N <= 40`, `k` from -12 to 12 and `k` = 30, 60, 64,
  210, and primes `k`); `k = 0`, `WORD_MIN`, `WORD_MAX`; exact units; `N = 1, 2, 4, 8`; the modulus `M_k`
  for `k` with many divisors; the sign of the real part for negative balls and even and odd `k`;
  `LIMIT` decided after the power is formed (time, memory), or refused for a small result.
- Hulls: a point of the idele outside `adf_adele_set_idele` or outside the simple hull; the small
  hull not the smallest ball that contains the idele's finite part; the exact unit not giving the
  exact rational.
- Adele to idele: `OK` for an adele that contains a non-unit at some prime or 0 at the real place;
  `NOT_UNIT` where a unit exists in the ball; the statuses against SPEC 4.5.
- Division: a quotient `a/x` of points outside the result; the radius not the one of Proposition 19
  (against enumeration), with `a = 0`, `M = 0`, odd `N`, `r` with factors shared with the radius;
  division by an exact unit; the real part across 0 in the divisor.
- Slice 2: the class map (the sign goes to the unit: negative real parts); the class of `q X` equal
  to the class of `X` for rational `q` (as sets, by your enumeration); the valuation and absolute
  value at primes that divide numerator, denominator, neither; the norm "exact before rounding": find
  an input where the returned ball does not contain the true norm, or where rounding happened
  twice; the norm of the idele of a rational contains 1.
- A false statement among F to O, or a proof step that does not follow. Statement J (the two powers,
  including what the design lane called E3) first: referee it.
- Statuses, `where` there is one, outputs on a status other than `OK`, aliasing of every combination,
  an abort from valid input, a leak or undefined behaviour (sanitizer build; valgrind at
  `~/.local/bin/valgrind`), `prec` at and above `ADF_IDELE_PREC_MAX`.
- A sentence of a header that the code does not keep; a test that cannot fail.

## Report

`lanes/i-review2/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
enclosure, a wrong status that claims a unit, a memory fault; MAJOR; MINOR), the input, what the code
returns, what is true and why, and the command that reproduces it with a program in your lane
directory. Then what you attacked without result, with counts and what would have made a case fail.
No praise, no summary of the code.
