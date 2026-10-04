# Lane f-slice13: the profinite power, binomial coefficients, Haar volume and the cyclotomic action (milestone 1F, WP 1F.9, second group of the catalogue)

PLAN row 1F.9 (`docs/PLAN.md` line 279) and SPEC 9.3.7 (`docs/SPEC.md` lines 685-720: the rows "Haar volume of
a finite ball", "profinite power", "binomial coefficient", "content of an idele", "cyclotomic action") list
them; `docs/proofs/catalogue.md` proves them (Proposition 10: content and Haar volume; Definition 11 and
Propositions 12, 13: the profinite power, its target precision, the coarsening to `D`, the canonical modulus
and the finest modulus; Proposition 14: the binomial enclosure and the smallest ball; Proposition 15: the two
conventions of the cyclotomic action; conventions 6.6, CV-53, and decision M0-D10 for its names). The Python
checks are in `proto/catalogue_checks.py`. The symbols of the catalogue landed today in `symbol.h` (lane
f-slice12): its header, its `docs/api-1f9.md` and its report `lanes/f-slice12/report.md` are the pattern of this
lane. The content of an idele exists already (`adf_idele_content`, `idele.h:217`): check it against Proposition
10 with one test and do not write it again.

You build four thin working slices that a user can call (library, driver, Julia), ONE AFTER THE OTHER, each
complete end to end before the next starts; if time runs short, stop after a complete slice:

- **Slice A: binomial coefficients** `binom(x, k)` for a profinite integer `x = a + N Zhat` (`adf_fball` with an
  integral centre and radius; say what a non-integral ball gives) and a non-negative integer `k`: the
  conservative ball (modulo `N / gcd(N, k!)`) and the SMALLEST ball (Proposition 14: centre `binom(a, k)`,
  radius the gcd of `binom(a + N j, k) - binom(a, k)` for `j = 1..k`), as two named functions or one with a
  policy, as you justify. PLAN test: `(a, N, k) = (0, 8, 4)` gives radius 2; `k = 0` gives the exact 1.
- **Slice B: the profinite power** `a^x`, `a = c U(N)` a unit coset, `x = e mod M` a profinite integer
  (`adf_fball`): determined modulo `N` exactly when `c^M = 1 mod N`; otherwise the coarser coset `c^e U(D)`,
  `D = gcd(N, c^M - 1)` (no factorisation), or `NOT_DETERMINED`, as the SPEC row and Proposition 12 say: read
  them and implement what they say (which of the two is the default and which is the separately named
  operation is stated there or is your decision, with the alternatives); negative exponents through inverses;
  exponent 0 gives the exact unit; the canonical moduli. Then the **finest-modulus variant** as a separately
  named operation (Proposition 13: `canon(F)` with its CRT centre). PLAN test: `N = 5, c = 2, e = 2, M = 4`:
  `F = 120`, centre 49, not `c^e`. Look at `include/adelefeld/idpow.h` first: integer powers of unit cosets
  exist there; say how your functions relate to them and reuse what fits.
- **Slice C: the Haar volume of a finite ball** (`1/N` for radius `N > 0`, 0 for a point; exact): one function,
  its test against Proposition 10, a driver command. Small on purpose.
- **Slice D: the cyclotomic action** of an idele class on a root of unity of order `n`: the two functions named
  by their exponent (`..._cyclo_exp_uinv`: `z -> z^(u'^(-1) mod n)`; `..._cyclo_exp_u`: `z -> z^(u' mod n)`),
  returning the exponent modulo `n` when the class determines it (the unit coset must be fine enough: which
  modulus must divide which: Proposition 15), else `NOT_DETERMINED`; the test vector of the specification
  (find it in SPEC 9.3.7 or `catalogue.md`; quote it with its line).

Local zeta factors are NOT yours (they need complex balls and pole handling: the next lane).

Decisions (proposed N-D19; propose them in the header FIRST, with the alternatives, then implement): the names
and the file (propose `include/adelefeld/catalogue.h`, `src/catalogue.c`, or separate small headers: justify);
input and output types (existing types only); statuses and `where`; limits (`k` and `e` as `ulong`/`slong` or
`fmpz`; a bound on `k` for the smallest-ball computation, which costs `k` binomials: say what is refused).
Where SPEC 9.3.7, `catalogue.md`, conventions 6.6 and the PLAN row disagree or are silent, that is a finding
with both quotations; take the reading a careful number theorist would, and say so.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
new header; rule 5 as written), `docs/SPEC.md` 9.3.7, 5 (unit cosets, profinite integers), 15.4 (N-D16 to N-D18
for the style of a decision), `docs/proofs/catalogue.md` lines 182-345, `proto/catalogue_checks.py`,
`docs/conventions.md` 2.1, 2.2, 3.1 to 3.3, 4.4, 5.6, 5.7, 6.6, `include/adelefeld/symbol.h` and `src/symbol.c`
(the newest pattern: value through a pointer, status returned, debug invariant entry checks),
`include/adelefeld/fball.h`, `ucoset.h`, `idpow.h`, `idclass.h`, `idele.h`, `docs/api-1f9.md`,
`tests/test_symbol.c`, `tools/adf/adf.c` and `tools/adf/README.md` (the commands `pow`, `powtight`, `legendre`
as patterns), `tests/julia/symbol.jl`, `tests/test_julia.sh`, `docs/reviews/f1/review-gfunc.md` (what reviewers
of this code base find). `refs/src/` is on disk (FLINT 3.0.1 under `refs/src/flint-3.0.1/`: `fmpz_bin_uiui`,
`fmpz_powm`, `fmpz_gcd`, `fmpz_CRT` in `fmpz.rst`: read their domains, cite file and line).

**You own:** the new header and source you propose (and no existing header), `tests/test_catalogue.c` (new),
`tests/julia/catalogue.jl` (new), `proto/catalogue2_checks.py` (new: YOUR oracle in exact integers, independent
of `proto/catalogue_checks.py`: binomials by the polynomial and enumeration of `a + N j` over a period you
prove sufficient; profinite powers by enumerating unit lifts modulo a multiple of the moduli and integer
exponents `e + M t`; the cyclotomic exponents by direct modular arithmetic), `tests/ref/vectors/f-slice13/`
(new, below 1 MB), `docs/api-1f9.md` (a NEW section at the end: the interface and decisions, statements Y9 ...
with proofs or the exact citation of `catalogue.md`), `tools/adf/adf.c`, `tools/adf/README.md`,
`tests/driver/catalogue-*.cmd` and `.out` (new; expected lines written BY HAND before the run),
`lanes/f-slice13/`. In `include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files need.
Everything else is read-only. No git command that changes state, no `bd`. At most 2 cores (another session
shares this laptop); every program under `timeout`; build into `BUILD=lanes/f-slice13/build` while you work and
into `build/` only for the final checks. Julia is on the PATH.

Order of work for each slice: header first; tests first, red then green (`lanes/f-slice13/redgreen.log`), with
the oracle rows and the PLAN test cases, every status with `where` and untouched outputs, aliasing, inputs of
thousands of bits, the boundary one step too coarse (`NOT_DETERMINED`) with two points of the input that give
different results; the code with debug invariant entry checks; the driver command; the Julia example; four
planted faults per slice in a scratch copy under your build directory, each with the test that fails.
Mutation testing of the new source once at the end (at most 60 mutants, `--seed 1`, 2 jobs, `--san`,
`timeout 1300`; a `--make` that builds and runs only your test; say how many compiled).

Final checks, once, at the end: `timeout 900 make -j2 check-all` in `build/` (if it fails, repair and say
plainly that the full run was not repeated, as lane f-slice12 did, or repeat it once if time allows); your
test built and run with `SAN=1` (`ASAN_OPTIONS=detect_leaks=0`: this sandbox cannot run LeakSanitizer; say so),
with `INV=1` (`BUILD=lanes/f-slice13/build-inv`) and with `CC=clang` (`BUILD=lanes/f-slice13/build-clang`).
Every source file ends with a newline; `tools/memcheck/selftest.py` must pass (it checks init and clear pairs
in tests: f-slice12 failed it on its debug child tests).

Report: `lanes/f-slice13/report.md`, written ONCE, AT THE END; running notes in `lanes/f-slice13/progress.md`
as you go (each slice recorded as finished before the next starts). In it: functions built; the decisions with
their alternatives; what is proved and what is cited; the numbers of the tests and what would have made a case
fail; the fault table; mutation survivors one line each; findings against the specification, the proofs or
this brief; what is not done.
