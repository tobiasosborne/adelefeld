# Lane f-slice10: the first all-places forms (milestone 1F, WP 1F.8): the root of an exact rational at all places, then the five series on a finite part exactly 0

No function at all places exists yet (`docs/conventions.md` 2.1, line 122: the all-places form has no `_at`
suffix). You build the first two, as thin working slices that a user can call (library, driver, Julia), ONE AFTER
THE OTHER: slice A complete end to end (header, tests red then green, code, driver, Julia) before slice B starts.
The facts were gathered with file and line in `lanes/f-slice10/brief-draft.md` (sections 1 and 2; written by a
research agent: every citation you rely on you open yourself). `Log` on ideles (the third part of PLAN row 1F.8)
is NOT yours: it is the next lane.

## Slice A: the root at all places

SPEC 9.3.3 (lines 636-646): for an exact rational `a` and a degree `n >= 1` the all-places root returns the
RATIONAL root (odd `n`: the one real root; even `n`: the non-negative one, optionally both), found by testing
numerator and denominator for n-th powers; no factorisation. It does not list all adelic roots (1 has continuum
many adelic square roots). An idele of finite precision and `n >= 2`: `NOT_DETERMINED`; degree 1 is the identity.
Exact 0 has the single root 0. Proposition 16 (`docs/proofs/functions.md:538`) proves the contract; Proposition 14
(`:446`) the real branches; Proposition 22 (`:725`) what an unqualified all-places map requires.

1. `adf_rat_root` on `adf_rat`: the exact rational n-th root by integer root tests on numerator and denominator
   (`fmpz_root`, `refs/src/flint-3.0.1/fmpz.rst:983-988`: "returns 1 if the root was exact"; FLINT's `n` is a
   `slong` and an even `n` needs a non-negative argument; yours is a `ulong`: decide and test `n` above the bit
   length (no root except for 0, 1 and, for odd `n`, -1: compare the static `integer_root`, `src/lroot.c:99-106`,
   which you cannot call), `n` above `WORD_MAX`, negative `a` with even `n`).
2. `adf_adele_root`: `x = (I ; F)` with `F` exactly a rational `q` (`adf_fball_is_exact`, `fball.h:154`);
   `y = (real root of I ; rational root of q)`, the real part by `adf_real_root` (`rfunc.h:79`; `prec` above
   `ADF_REAL_PREC_MAX` is `LIMIT` first, N-D8), the two coordinates on the SAME branch.
3. `adf_idele_root`.
4. One driver command `root X with N` (and the way to ask for the other sign), `tools/adf/README.md`, a Julia
   example.

Decisions of the orchestrator for slice A (proposed N-D16; you implement them, and you say in the report where one
cannot be proved or contradicts a text, with the counterexample; alternatives are in the draft, section 2):
- **Types.** The three functions above. An adele with exact finite part and ANY real ball is admitted (the
  coordinates are independent, `adele.h:12`).
- **The sign.** A parameter `sign` in `{+1, -1}`: `+1` is the non-negative root for even `n` and the only root for
  odd `n`; `-1` is the non-positive root for even `n` (for the adele: the negative of both coordinates). `-1` with
  odd `n` names no branch: treat it as `lroot.h` treats a seed that names no branch (read it; say what you took).
  Exact 0: the root 0 for either sign.
- **No rational root** (2 with `n = 2`; -4 with `n = 2`): `DOMAIN` (Proposition 16: no adelic root exists, so
  every point of the input is outside the domain). `where` (conventions 3.1 line 183, 3.3 lines 234-247): the real
  place when the real coordinate is the cause (even `n`, real ball certified negative); when only the finite part
  fails, no prime can be named without factorisation: `where` is left untouched and the header says so. If the
  conventions name a better value for "the finite part as a whole", use it and say so.
- **A finite part that is not exactly a rational** (positive radius, or the local backend, `fball.h:152-153`),
  `n >= 2`: `NOT_DETERMINED`, without examining the primes of the modulus (that would need a factorisation; the
  header says so, with the example `2 + 4 Zhat`, `n = 2`, where a look at 2 would prove `DOMAIN`). `n = 1` is the
  identity for every input. `n = 0`: as `adf_real_root` and `lroot.h` treat it.
- **Ideles.** A unit coset of positive modulus and `n >= 2`: `NOT_DETERMINED` (SPEC 9.3.3 line 637). An idele whose
  unit is EXACT (`adf_ucoset_is_exact`, `ucoset.h:102`; the idele of an exact rational, `idele.h:135`) is an exact
  rational at the finite places, and Proposition 16 speaks of finite precision only: it follows the rational
  contract (the root idele: content `r^(1/n)`, unit `[sign]`-consistent, real ball by `adf_real_root`; `DOMAIN`
  when no rational root). Write the statement that covers it (the proof is Proposition 16 step 3 and 4 applied to
  the rational `r * (+-1)`); if the representation of an idele makes this false or ugly, return `NOT_DETERMINED`
  for every idele with `n >= 2` and say why.
- **Combined status** over the real and the finite coordinate: conventions 3.3 (the maximum; the first place in
  canonical order). `(I ; q)` with `I` negative, even `n`, `q` a perfect power: `DOMAIN` at the real place.

## Slice B: `exp`, `sin`, `sinh`, `cos`, `cosh` at all places

SPEC 9.3.2 lines 588-596 and Proposition 12 (`functions.md:375-408`): the common finite domain is
`D = 4 Z_2 x product over odd p of p Z_p`; it contains no finite ball of positive radius and its only rational is
0, so the all-places form applies only to an adele whose finite part is EXACTLY 0, with any real part; at the
finite argument 0 the outputs are 1, 0, 0, 1, 1 (step 1, line 394).

`adf_adele_exp`, `adf_adele_sin`, `adf_adele_sinh`, `adf_adele_cos`, `adf_adele_cosh`: the real part by the real
function at `prec` bits (there is no `adf_real_sinh`, `cosh`: see how the `_at` forms at the real place do it,
`rfunc.h:143-150`, and call what exists; `src/rfunc.c` is read-only), the finite part the exact rational 1 or 0.
Driver commands `exp X`, `sin X`, `sinh X`, `cos X`, `cosh X` (none exists; the names are free), README, Julia.

Decisions for slice B:
- **The output's finite part is the exact constant** (as N-D12 at a prime: "Only the exact 0 gives an exact
  constant"); SPEC 9.3.1 line 573 ("an exact input does not give an exactly representable output") states the
  general case, to which the argument 0 is the exception already made at a prime.
- **An exact non-zero finite part `q`**: `DOMAIN`, with `where` the FIRST prime in canonical order at which `q`
  lies outside the local domain (`v_2(q) < 2` at 2, `v_p(q) < 1` at odd `p`): it exists and is found by trying
  2, 3, 5, ... (prove the bound on the search: the primes dividing the numerator are few).
- **A finite part of positive radius or in the local backend**: `NOT_DETERMINED`, without examining the primes of
  the modulus, as in slice A; the header says so and gives one example where a look at a prime would prove
  `DOMAIN` and one (`0 + 4 Zhat`) where no look can decide.

## Rules

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
new header, which is yours; rule 5 as written), `lanes/f-slice10/brief-draft.md` sections 1 and 2, `docs/SPEC.md`
9.3.1 (lines 557-573), 9.3.2 (lines 577-600), 9.3.3 (lines 613-646), 15.4 (N-D7, N-D8, N-D10, N-D12, N-D13),
`docs/proofs/functions.md` Propositions 12, 14, 16, 22 and the Python checks `proto/functions_checks.py:433`
(`check_global`) and `:585` (`check_global_roots`), `docs/conventions.md` 2.1, 3.1 (lines 169-192), 3.2 (line
221), 3.3 (lines 234-247), `docs/PLAN.md` row 1F.8 (line 278), `include/adelefeld/rfunc.h`, `rat.h`, `adele.h`,
`fball.h`, `idele.h`, `ucoset.h`, `place.h`, `sball.h`, `lroot.h`, `src/lroot.c` lines 99-106, `src/rfunc.c`
(`adf_real_root`), `docs/api-1f6.md` (the style of statements and their "Check:" lines), `lanes/f-slice9/brief.md`
and `result.md` (the pattern of a slice of this family and of its report), `tests/test_lpow.c`,
`proto/lpow_checks.py:70` (`iroot`: an integer root independent of FLINT), `tools/adf/adf.c` (the table
`adf_drv_ops[]` near lines 187-229, `adf_drv_places` near line 1890, the dispatch near line 2495, the idele
commands after it), `tools/adf/README.md`, `tests/julia/lpow.jl`, `tests/test_julia.sh` (lines 183-185).
`refs/src/` is on disk: FLINT is `refs/src/flint-3.0.1/` (the `.rst` files are in its top directory).

**You own:** `include/adelefeld/gfunc.h` (new) and `src/gfunc.c` (new), `tests/test_gfunc.c` (new),
`tests/julia/gfunc.jl` (new), `proto/gfunc_checks.py` (new; the oracle of this lane, exact integers for the finite
parts), `tests/ref/vectors/f-slice10/` (new, below 1 MB), `docs/api-1f8.md` (new: statements G1 ... with proofs,
in the style of `docs/api-1f6.md`), `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/gfunc-*.cmd` and
`.out` (new), `lanes/f-slice10/` (except `brief.md` and `brief-draft.md`). In `include/adelefeld.h` and
`tests/test_julia.sh` you may add the lines your files need. Everything else is read-only: you call it. If the
names `gfunc.*` are taken, choose others and say so. No git command that changes state, no `bd`. At most 2 cores
(another session on this machine runs emulators); every program under `timeout`; build into
`BUILD=lanes/f-slice10/build` while you work and into `build/` only for the final checks. Julia is on the PATH.

Order of work, for slice A and then again for slice B:
1. Header first: the comment block of every declaration (set statement; proposition with file and line; the
   branch; every status and the input that gives it; what is untouched on a status other than `OK`; aliasing
   (`y` may be `x`); the limits).
2. Tests first, red then green (`lanes/f-slice10/redgreen.log`; a link error counts only for the first test of the
   file). Oracles with a STATED output precision (finite parts: exact; real parts: the ball contains the true
   value, checked with `mpmath` at a higher precision or by `root^n` containing `I`, and its radius is bounded as
   the real function's header says):
   - A: `n` from 1 to 12 and large (above the bit length of numerator and of denominator; above `WORD_MAX`);
     `a = num/den` over a grid with signs and big values (thousands of bits; perfect powers and perfect powers
     plus or minus 1); the root by `iroot` (independent of FLINT) and `root^n = a`; the criterion "every
     valuation divisible by `n` and the sign" of `check_global_roots` against the integer test; the named cases
     of the PLAN row (`1`: `1` and `-1`; `8`: the cube root `2`; `2`: no square root) and `-4` with `n = 2`, `-8`
     with `n = 3`, `0`, `n = 1`, `n = 0`; the adele: real balls positive, negative, crossing 0, with even and odd
     `n`, the combined status and `where`, `prec` above the limit, a finite part of positive radius, a
     local-backend finite part; the idele: `n = 0, 1, 2, 3`, exact and inexact units; both signs everywhere;
   - B: the five functions on `(I ; 0)` for real balls `I` positive, negative, large, crossing 0; finite parts
     `0` (OK), `1`, `4`, `12`, `1/3`, `-9/2` (DOMAIN and the place: check the place against a search in exact
     integers), `0 + 4 Zhat`, `3 + 9 Zhat`, a local-backend value (NOT_DETERMINED);
   - every status with the state of the outputs the header promises; every aliasing combination; the driver
     golden files; the Julia example (A: 8 has the cube root 2, 2 has no square root, 1 has the square roots 1
     and -1, an inexact idele of degree 2 is `NOT_DETERMINED`; B: `exp` of `(1/2 ; 0)`, `cos` of `(0 ; 0)`, `exp`
     of the rational 1 is `DOMAIN` with its place).
3. The code; the driver commands; the Julia example.
4. Planted faults in a scratch copy under your build directory, five for each slice (among them, A: the sign of
   the even root taken for odd `n`; the denominator not tested; `n` above the bit length passed to `fmpz_root`;
   the real part computed from the finite part; the inexact idele of degree 2 returned as `OK`; B: `sin` and
   `cos` constants exchanged; the exact non-zero finite part accepted; the place of `DOMAIN` off by one prime).
   Mutation testing of `src/gfunc.c`, `lanes/COMMON-C.md` rule 5 (at most 60 mutants, `--seed 1`, 2 jobs,
   `--san`, `timeout 1300`). NOTE the tool's defect: `--san` is defeated when the `--make` string contains "SAN"
   (as in `ASAN_OPTIONS`), and mutants that did not compile are counted as killed: read the log and say how many
   compiled.
5. Final checks, once, at the end: `timeout 900 make -j2 check-all` in `build/` (this one run may take 10
   minutes); one `ASAN_OPTIONS=detect_leaks=1` sanitizer build of `test_gfunc` in `BUILD=build/san` and its run.
   Give the last line of each.

Report: `lanes/f-slice10/result.md`, written ONCE, AT THE END (the harness refuses the name `report.md` for a
Claude subagent); running notes in `lanes/f-slice10/progress.md` as you go, so that slice A is recorded as
finished before slice B starts. In it: functions built; the decisions above as you implemented them, and your own
where the brief is silent, each with its alternatives; what is proved (the new statements) and what is not; the
numbers of the tests and what would have made a case fail; the fault table; mutation survivors one line each;
findings against the specification or this brief; what the next lane (`Log` on ideles, questions 8 and 9 of the
draft) should know. Give the same text as your final message.
