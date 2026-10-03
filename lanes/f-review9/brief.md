# Lane f-review9: adversarial review of the first all-places forms (lane f-slice10, WP 1F.8): the root at all places, the five series on a finite part exactly 0

Your task is to REFUTE, not to confirm. Lane f-slice10 (Claude Opus; you are of another model family on purpose)
landed today, not reviewed: `include/adelefeld/gfunc.h`, `src/gfunc.c` (357 lines): `adf_rat_root`,
`adf_adele_root`, `adf_idele_root`, `adf_adele_exp`, `_sin`, `_sinh`, `_cos`, `_cosh`; the driver commands
`root X with N [with SIGN]`, `exp X`, `sin X`, `sinh X`, `cos X`, `cosh X` (`tools/adf/adf.c`,
`tools/adf/README.md`); the statements G1 to G6 with proofs (`docs/api-1f8.md`); decision N-D16 (`docs/SPEC.md`
15.4, last row); the oracle `proto/gfunc_checks.py` with fixtures `tests/ref/vectors/f-slice10/`; the tests
`tests/test_gfunc.c`, `tests/julia/gfunc.jl`, `tests/driver/gfunc-*`. Its report is `lanes/f-slice10/result.md`
(read its "Decisions", "Findings" and "Avoidable costs"); its brief `lanes/f-slice10/brief.md`.

Contract: `docs/SPEC.md` 9.3.1 (lines 557-575), 9.3.2 ("On all primes at once"), 9.3.3 ("At all places" and the
rational root), 15.4 (N-D8, N-D12, N-D16); `docs/proofs/functions.md` Propositions 12, 14, 16, 22;
`docs/conventions.md` 2.1, 2.2, 3.1, 3.2, 3.3 (rule 4 is new today); the comment blocks of `gfunc.h`;
`include/adelefeld/rfunc.h` (the real functions it calls), `adele.h`, `fball.h`, `idele.h`, `ucoset.h`.
`refs/src/` is on disk (FLINT 3.0.1 under `refs/src/flint-3.0.1/`).

**You own:** `lanes/f-review9/` only. Everything else is read-only. No git command that changes state, no `bd`.
At most 2 cores (other lanes and another session share this laptop). Build the archive ONCE with
`timeout 600 make -j2 BUILD=lanes/f-review9/build lanes/f-review9/build/libadelefeld.a` and link your programs
against it (`-Iinclude -lflint -lgmp -lm`; `-std=gnu11` if you use `clock_gettime`); build the driver for your
attacks into your own directory (read `tools/adf/Makefile`; do not write to `build/`). Do not run the test suites
of the repository as a whole. Every program under `timeout`; none over 180 s. Compile reproducers with
`-fsanitize=address,undefined` where memory is in question.

## What counts as a finding

- **A wrong value or enclosure** (your OWN oracle, exact integers for the finite parts; for the real part an
  interval computed at higher precision, stated): a root that is not a root; a rational with a rational root
  reported `DOMAIN`, or the reverse; the two coordinates of an adele root on different branches; an idele result
  that is not canonical or not an idele (real ball containing 0, content not positive, unit not exact); a series
  result whose real ball misses a point of the image, or whose finite constant is wrong. Inputs: numerators and
  denominators of thousands of bits, perfect powers and their neighbours, `n` at and around the bit lengths,
  `WORD_MAX`, `WORD_MAX + 1`, `UWORD_MAX`; negative rationals with even and odd `n`; `1`, `-1`, `0`; real balls
  positive, negative, crossing 0, exact, huge, tiny, with infinite or NaN parts if a caller can build them;
  `prec` 2, 1, 0, negative, `ADF_REAL_PREC_MAX` and one above.
- **A wrong status or place.** `DOMAIN` must be PROVED: referee G2 (a root at every FINITE place implies a
  rational root: the step for `-r^n` with even `n`; is the valuation argument complete for a rational that is
  not plus or minus an n-th power? the prime 2?) and G6 (the first prime outside the local domain and the bound
  of the search); find an input where `DOMAIN` is returned and a point of the input has the value, or where
  `NOT_DETERMINED` is returned and the header promised a decision, or `OK` where the input meets the complement
  of the domain. `where`: written on `OK`? wrong place? the maximum rule of conventions 3.3 with the real place;
  `where = NULL`. The order of the checks stated in the header (`LIMIT`, `n = 0`, `n = 1`, the sign, the values)
  against the code, case by case. Outputs changed on a status other than `OK`; aliasing `y = x`.
- **The exact 0, the selector and degree 1**: the adele whose real ball is the exact 0 and whose finite part is 0
  with either sign; an adele `(I ; 0)` with `I` not exact (what is the root for `sign = -1`, even and odd `n`?);
  the idele forms; `n = 1` on inexact inputs (identity, no rounding); the claim of decision 2 that degree 1
  ignores `sign`.
- **The series**: `(I ; 0)` with `I` at the limits of the real functions (the lane says `exp` of `2^1000` is
  `NOT_DETERMINED` at the real place: is that the status `rfunc.h` documents, and is the combination with a
  finite `DOMAIN` right?); an exact `q != 0` whose numerator is divisible by many small primes (the place against
  your own search; the cost: the lane names `first_failing_prime` as an avoidable cost, measure it once on a
  numerator of 10^5 bits divisible by the first few thousand primes and say whether a caller can be made to wait
  minutes); `q` with a large denominator; a finite ball `0 + 4 Zhat`, `4 + 8 Zhat`, a local-backend value.
- **A false step in G1 to G6**: referee each as a proof (the claim in your words; each step; a counterexample in
  exact integers where a step fails). G1: `n >= bits(q)` excludes a root (the cases 0, 1, -1, and `q = 2`,
  `n = 1`); the cast of a `ulong` degree to FLINT's `slong`. G4: the exact idele (is the result's unit `[sign]`
  right when the rational root is negative? content `|rho|`?).
- **The driver and Julia**: every operand type the commands accept or refuse; a sign operand of `0`, `2`, `-1`
  with odd `n`, a non-integer, a huge integer; a degree of `2^64`, negative, non-integer; a fourth operand; the
  printed values against the library's; hostile input lines. Does a command that existed before behave
  differently now (the table has a new arity code: check three old commands of arity 1, 2, 3, 4 with too few and
  too many operands against the golden files' expectations)?
- **A test of the lane that cannot fail**: plant at least 10 faults of your own in a scratch copy of
  `src/gfunc.c` (different from the lane's ten; among them: the sign applied to the real part only; `where`
  written on `OK`; the idele's unit `[1]` always; the real root taken at `prec - 1`; the first failing prime
  search starting at 3; `n = 1` rounding the real ball; the `LIMIT` check after the `n = 0` check), build
  `test_gfunc` against each (`make -j2 BUILD=<your dir> <your dir>/test_gfunc` on the scratch copy), and report
  the faults that pass. The lane itself says its idele vectors all have exact units (fault A5 is caught by two
  checks only): look there.
- **A sentence of `gfunc.h`, N-D16, the README or `docs/api-1f8.md` that the code does not keep.**

KNOWN, not to be reported again (the lane's own findings): conventions 3.3 cannot name a prime for the finite
part of the root (rule 4 was added); SPEC 9.3.3 on exact ideles and SPEC 9.3.1 line 573 (both amended today);
`adf_real_root` at `prec` 2 gives a ball containing 0 for a positive input (not this lane's code; say only if it
makes a `gfunc.h` sentence false); the selector of the exact 0 differs from `lroot.h`; the four avoidable costs
of its report (measure the third, as said above).

## Report

`lanes/f-review9/report.md`, written once, at the end; running notes in `lanes/f-review9/progress.md` as you go
(if the session is cut off they are the report). For each finding: severity (BLOCKER: a wrong value or enclosure,
a `DOMAIN` that is not proved, a memory fault, undefined behaviour; MAJOR; MINOR), the input, what the code
returns, what is true and why, and the command that reproduces it with a program in your lane directory. Then
what you attacked without result, with counts and what would have made a case fail; the fault table; the proof
steps checked. No praise, no summary.
