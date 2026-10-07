# Lane f4-slice3: result (slice 4e of docs/api-4.md: transform, derivative, integral, norm2 of adf_rfun)

All work is in the worktree /home/tobias/Projects/adelefeld-wt/f4-slice3 (branch lane/f4-slice3, base 234de7e).
- No git command that changes state was run, and no bd.
- Builds used `make -j2`, and every program ran under `timeout`.
- Build trees (`lanes/f4-slice3/build`, `b-san`, `b-inv`, `b-clang`, the driver's `build/`) and all scratch copies
  are removed.
- The lane directory is 168 KB, with no file above 100 KB.

## Files

- New:
  - `tests/test_rfun_fourier.c`
  - `tests/ref/vectors/f4-slice3/transform.jsonl` (254958 bytes) and `shifted.jsonl` (547 bytes)
  - `tests/driver/rfun-fourier.cmd` and `.out`
  - `tests/julia/rfun_fourier.jl`
  - `lanes/f4-slice3/{gen_vectors.py, plant_faults.py, redgreen.md, result.md}` and logs
- Changed:
  - `src/rfun.c`: about 300 lines appended; nothing above them changed.
  - `include/adelefeld/rfun.h`: 4 declarations appended with their comment blocks. Two lines of the top comment
    said the derivative, transform, integrals and norms were not in this header; they now say slice 4e follows.
  - `tools/adf/adf.c`: 4 enum values, 4 table rows, 2 dispatch lines and the function `adf_drv_rfun4e`.
  - `tools/adf/README.md`: a section at the end.
  - `tests/test_julia.sh`: one block after the rfun block.
  - `docs/api-4b.md`: "Slice 4e" appended.

## A. Vectors

Command: `timeout 600 python3 -B lanes/f4-slice3/gen_vectors.py` (51 s). It is deterministic: a rerun gives
identical md5 sums.

- `transform.jsonl` holds 60 functions:
  - Named cases: the Gaussian, `x exp(-pi x^2)`, `A = 4`, a zero polynomial, the zero function, and
    `A = 1 +/- 2i` (both root quadrants).
  - Random cases, 1 to 3 terms each, with polynomials of degree -1 to 6. `A` is real (k/4), complex
    (6/5 + i/5, 1 +/- 2i, 1/100 + 3i, 1/2 - 7i/4), on the boundary (`Re A = 10^-30`), or a ball.
  - 52 records have exact inputs and 8 have ball inputs.
- Each record holds:
  - the oracle's `rterm_transform` for every member, as 60-digit balls from the oracle's `ball()` at 400 bits;
  - `rterm_derivative` (exact, pi form);
  - the reflection, to check `F^2` against;
  - the integral, from the oracle transform evaluated at 0;
  - norm2, as the sum over every ordered pair `(k, l)` of `rterm_product(t_k, conj t_l)` transformed at 0;
  - transform values at y = 0, 1/4, -2/5, 3/2;
  - for 18 records, `mp.quad` values of the integral, of `|phi|^2` and of the transform at y = 1/5 and -2/5.
- `shifted.jsonl`:
  - `exp(-pi (x - 1/3)^2)` in exact pi form, with `B = 2 pi/3` and `C = -pi/9`;
  - its transform;
  - its transform value at y = 1/4, which is `exp(-pi/16) E(1/12)`. The generator asserts that the imaginary
    part is positive and that the value agrees with the closed form.
- `timeout 120 python3 -B proto/functions4_checks.py`: TOTAL 2670 checks.

## B and C. Tests, then code

RED A: `tests/test_rfun_fourier.c` was written in full, with the declarations; the build was a link failure
with 74 undefined references (`red-a.log`).

The code for all four functions was then appended in one step. Per function, the red evidence is the fault
table of F, not a separate red run for each function. The first green attempts needed five corrections, all in
the test and none in the code (`redgreen.md`):
- tightness at `Re(A) = 10^-30` (conditioning, see the stated bounds below);
- the same bound must not tighten a ball record;
- an exact result part (`-16`) need not contain the oracle's 400-bit ball around it; it must lie in that ball;
- two sentinels were not reset.

What the test covers:
- **Vectors:** every record. Every member's transform must lie in the result, term by term, parameter by
  parameter and coefficient by coefficient, with the exact length.
- **`F(F(x))`** encloses the reflection of every member.
- **Derivative:** every member's exact derivative.
- **Integral:** against the oracle, and against our own transform evaluated at 0 (an independent path).
- **norm2:** against the oracle's ordered pairs, against `mul(x, conj x)` integrated, and nonnegative.
- **Values and quadrature:** transform values at four points; the quadrature second source within 1e-40.
- **Tightness, stated in the file header:**
  - exact inputs at 128 bits: radius at most `2^-96 (1 + |v|)`;
  - ball inputs: at most `2^-2 (1 + |v|)`;
  - `Re(A) = 10^-30`: integrals, norms and values at `2^-16 (1 + |v|)`, and `F^2` by containment only. The
    exponents and coefficients there have size `2^100` and lose their low bits at 128 bits.
- **Exact cases:**
  - `F(exp(-pi x^2))` is identical to its input.
  - `F(x exp(-pi x^2))` is exactly `[0, i]`, and `F^2` of it is exactly `-x exp(-pi x^2)`.
  - At `A = 4`: amplitude 1/2, `A' = 1/4`, integral 1/2.
  - Both root quadrants:
    - the root has `Re > 0` and the imaginary part of the right sign;
    - `(A^(-1/2))^2 A` contains 1;
    - `F^2` contains 1 and is disjoint from -1.
  - The single-Gaussian norm closed form `(2 Re A)^(-1/2) exp(2 Re C + (Re B)^2/(2 pi Re A))`, for a real and a
    complex A.
- **Sign test (PLAN 4.3):**
  - the transform of the shifted Gaussian contains the oracle's transform;
  - its value at 1/4 contains the oracle value and has a positive imaginary part;
  - that value overlaps `exp(-pi/16) E(1/12)` computed in C and is disjoint from its conjugate;
  - end to end (`translate_rat` by 1/3, then `fourier`): the result agrees with `exp(-pi y^2) E(y/3)` at 13
    points.
- **Covariance:** 16 exact functions at 3 points each.
  - `F(D_h phi)(y) = |h|^-1 F phi(y/h)` for h = 2/3, -5, 3, -1/2.
  - `F(T_q phi)(y) = E(+q y) F phi(y)` for q = 1/3, -7/2, 5/8; `E(-q y)` is asserted disjoint.
- **Derivative:**
  - against central differences at 300 bits with `h = 2^-50`;
  - the explicit formula at `A = 1 + i`.
- **Statuses:**
  - prec cap + 1 gives LIMIT with the outputs untouched; prec 2, 0, -5, 53 and `ADF_REAL_PREC_MAX` are run.
  - `A = 0.0001 + 100i` gives NOT_DETERMINED at prec 2, with y untouched (sentinel bytes and `identical`), and OK
    at prec 128.
  - `A = (1.25 +/- 1.2) + 0.3i` gives NOT_DETERMINED, also with a zero polynomial.
  - `C = 10^300`: integral and norm2 give NOT_DETERMINED with z untouched; the transform is OK.
  - The zero function: transform and derivative have length 0, integral and norm are exactly 0.
  - Zero-polynomial terms are kept.
- **Aliasing:** `fourier(y, y)` and `derivative(y, y)` match the unaliased results; a failing aliased call
  leaves x untouched.
- **Caps:**
  - transform work `2 L^2 - L + 1`: 724 passes and 725 does not; the exact sum `2^20` passes and `2^20 + 1` does
    not; 2 x 512 passes and 2 x 512 + 31 does not;
  - derivative coefficients: length `2^16 - 1` passes and `2^16` does not;
  - norm: 256 terms pass and 257 do not; length 1024 passes and 1025 does not;
  - `2^16 + 1` raw terms give LIMIT for all four functions; `2^16` terms pass;
  - the add boundary (mutation survivor): exactly `2^16` coefficients pass and `2^16 + 1` do not.
- **INV:** 4 forked children abort (SIGABRT) on `Re(A) < 0`. The prec cap is decided before the entry
  predicate.

Final runs:
- Build: `make -j2 [VAR] BUILD=lanes/f4-slice3/<dir> <dir>/test_rfun <dir>/test_rfun_fourier`.
- Run: `ASAN_OPTIONS=detect_leaks=1 timeout 600 <dir>/<test>`.

| Build | test_rfun | test_rfun_fourier | Exit |
|---|---|---|---|
| plain | 64923 checks | 54258 checks (4.6 s) | 0 |
| `SAN=1`, leak detection on | 64923 | 54258 | 0 |
| `INV=1` | 64939 | 54271 | 0 |
| `CC=clang` | 64923 | 54258 | 0 |

`sh tests/test_exports.sh`: 570 of 570 declared functions are exported.

## D. User calls

- Driver commands: `rfun_fourier R`, `rfun_derivative R`, `rfun_integral R` (complex ball, as psi prints it)
  and `rfun_norm2 R` (real ball).
- Fixture `tests/driver/rfun-fourier`, 17 expected lines derived by hand (mpmath digits, printer of 9.5). It
  covers:
  - the exact transforms;
  - `C' = -1/(4 pi)`;
  - the shifted Gaussian's transform
    `rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (2.0944 +/- 4.9e-6)*i, C=(0 +/- 1.1e-6) + (0)*i))`;
  - that transform at `y = 1/4`, `(0.712 +/- 0.00037) + (0.411 +/- 0.00014)*i`, with a positive imaginary part;
  - the derivative, integrals, norms, DOMAIN and PARSE.
- First run: one line differed only in my formatting of the radius (I wrote `3.7e-4`; the 9.5 printer writes
  `0.00037`, positional at exponent -4). That was a defect of the derivation, and the fixture was corrected.
- `sh tests/test_driver.sh`: 84 cases, 101507 expected lines, all equal (SAN=0), run twice.
- The command lines of the fixture with the 60-digit shifted Gaussian are 216 and 119 characters long. Commands
  cannot be wrapped; slice 4d's `rfun-eval` does the same.
- Julia: `tests/julia/rfun_fourier.jl` calls `ccall((:adf_rfun_fourier,lib),Cint,(P,P,Clong),out,r,128)` and
  passes 11 of 11 (`julia.log`).
  - The checks: `F(x e^{-pi x^2}) = i y e^{-pi y^2}`, `F^2` is the reflection with `out` aliased, LIMIT with
    `out` untouched, and the derivative.
  - The plain run fails with `__gmpn_modexact_1_odd`; the test passes with `LD_PRELOAD` of libgmp, the fallback
    that `test_julia.sh` uses. The block is registered in `tests/test_julia.sh`.
  - The full `test_julia.sh` was not run.

## E. Statements

`docs/api-4b.md` "Slice 4e" holds:
- the convention, quoted: kernel `exp(+2 pi i x y)`, so `F(T_q phi) = E(+q y) F phi`;
- the order of checks;
- four numbered statements with their "Check:" lines:
  - the derivative formula with its proof;
  - R1 in the y variable: the recurrence `G_(j+1) = G_j'/(2 pi i) + w B G_j + (i/A) y G_j`, derived from P5;
  - R2 by `acb_sqrt_analytic` and `acb_inv`, with `acb.rst:590-601` quoted and the branch shown to be R2 step 1;
  - the Gaussian moment formula `M_j = A^(-1/2) exp(C + B^2/(4 pi A)) h_j`, with the recurrence
    `h_(j+1) = (j h_(j-1) + B h_j)/(2 pi A)` proved by integration by parts and identified with `H_j(B)`
    through the generating function;
  - the norm via conj and mul, clipped with `arb_nonnegative_part` (`arb.rst:417-423`);
- decisions where the design is silent;
- cost.

## F. Faults (`plant_faults.py`, scratch copies, removed)

| Fault | Result |
|---|---|
| negative real kernel (B' = -iB/A, z = B - 2 pi i y) | caught |
| A for 1/A | caught |
| B' without i | caught |
| C' without B^2/(4 pi A) | caught |
| the other root branch | caught |
| A^(+1/2) for A^(-1/2) | caught |
| amplitude omitted (A = 4) | caught |
| reflection dropped from F^2 (polynomial with the other kernel) | caught |
| Re(1/A) > 0 of the result not checked | caught |
| y written before the certificate | caught |
| transform work cap one unit low | survived in the first run (test gap); caught after the exact-boundary test |
| derivative without -2 pi A x P | caught |
| derivative without B P | caught |
| derivative coefficient cap: L for L + 1 | caught |
| integral: j + 1 for j in the recurrence | caught |
| integral without C | caught |
| integral without B^2/(4 pi A) | caught |
| a cross term of the norm dropped (sum of the norms of the terms) | caught |
| norm of phi times phi (conjugate dropped) | caught (abort, exit 134) |
| norm written on failure | caught |

Result: 20 of 20 caught (`faults2.log`).

### Mutation run

Command:

    ASAN_OPTIONS=detect_leaks=0 timeout 1260 python3 -u tools/mutate/mutate.py --root . --scratch <scratchpad> \
      --files src/rfun.c --limit 60 --seed 4005 --jobs 1 --timeout 180 --san \
      --make "make -s -j2 check INV=1 TEST_SRC='tests/test_rfun.c tests/test_rfun_fourier.c'" \
      --copy Makefile include src tests

- 750 candidates over the whole file (the tool has no line filter). The baseline took 43.5 s.
- The run stopped at the 20-minute cap (exit 124). The tool prints no killed count when it is stopped.
- Listed outcomes: 7 survived and 3 did not compile.

Survivors, one line each:
- 262:40 `>` -> `>=` in `adf_rfun_add` (slice 4d code): a test gap. A test was added in `caps`, and the
  planted mutant fails it.
- 730:13 `acb_poly_add(Gn, Gn, T)` -> `(Gn, T, Gn)`: equivalent, since addition is commutative.
- 720:9 `acb_mul(wB, w, B)` -> `(wB, B, w)`: equivalent (commutative).
- 386:17 `acb_add` argument swap in the translation (slice 4d): equivalent (commutative).
- 522:13 `arb_mul(w, w, h)` -> `(w, h, w)` in the idele dilation (slice 4d): equivalent (commutative).
- 388:9 `_acb_poly_normalise` dropped after the Taylor shift (slice 4d): equivalent. The shift never changes the
  top coefficient `a_(L-1)`, and the input is normalized.
- 669:16 `return 0` -> `return 1` in `rf_amplitude` (the root certificate): not killed.
  - For a canonical A box, `acb_sqrt_analytic` gave a finite root with certified `Re > 0` in every case I tried.
  - A nonfinite root would make the coefficients nonfinite, and the result predicate would reject them.
  - I found no input that reaches the branch, so it is defensive. It is not entered in `equivalent.txt`.

## Findings against the design, the oracle, the brief and the goldens

1. The brief's tentative "F(T_q phi) equals E(-q y) times F(phi)" has the wrong sign for this project. Conventions
   6.1 fixes `F f(y) = integral f(x) conj(psi_inf(x y)) dx` with `psi_inf(x) = E(-x)`, so the real kernel is
   `E(+x y)` and `F(T_q phi)(y) = E(+q y) F phi(y)`. The test asserts `+` and that `E(-q y)` is disjoint. The
   brief's question about Proposition 3 has the same answer: `E(+x y)` at the real place.
2. R2 and `acb_inv`: the canonical function with `A = (1.25 +/- 1.2) + 0.3i` has a true `Re(1/A)` in about
   `[0.4, 1.7]`, all positive.
   - Its transform is NOT_DETERMINED at every precision (checked at 2 to 2048 bits): `acb_inv` returns
     `0.756 +/- 24`, from the dependency in `a^2 + b^2`.
   - The design allows this ("certificate failure is NOT_DETERMINED").
   - An interval formula with exact quotient bounds would certify it. It is not done here: midpoint-radius
     `arb_div` alone would still fail. This is a possible later improvement.
3. Conditioning at `Re(A) = 10^-30`, which the design asks to test "near the boundary":
   - at 128 bits `F^2` keeps no relative accuracy in its coefficients;
   - integrals of terms with `C + B^2/(4 pi A) ~ 10^29` have relative radius near `2^-23`.
   These are enclosures, not defects, and more precision reduces them. Production tests should not expect
   `2^-96` tightness there.
4. The oracle has no multi-term norm. The vectors compose `rterm_product` over every ordered pair (design
   section 6). Its single-term norm check (`check_real`) agrees with the closed form tested in C.
5. The oracle's `ball()` gives a nonzero radius (about `10^-63`) around exact values. Exact C results are
   therefore checked as "lies in the oracle ball", not "contains it". This is a test convention, not an oracle
   defect.
6. Design section 9 item 5 lists only `adf rfun_fourier R` and `adf rfun_integral R`. The brief adds
   `rfun_norm2` and `rfun_derivative`; all four are implemented.
7. Goldens: no row of `tests/golden/rfun.tsv` is a transform of another. Rows 2 and 3 (the Gaussian,
   `x exp(-pi x^2)`) are the inputs of the exact-case tests. Nothing in the fixed files was changed.
8. No counterexample to Proposition 5, R1, R2 or section 6 was found.
9. No HEADER-FINDING: the four declarations are implemented as written in the design.

## Sources pending

None new. The FLINT lines used are cited in `src/rfun.c` and `docs/api-4b.md`:
- `acb.rst:445-449, 509, 590-601, 658-661`;
- `acb_poly.rst:140-146, 250-289, 563-569`;
- `arb.rst:6-12, 417-423`.

The Gaussian moment recurrence and the y-variable recurrence are proved in `docs/api-4b.md`. The analytic
imports of analysis Definition 1 remain as listed in api-4.md section 10.

## Not done

- The mutation run reached the 20-minute cap before 60 judged mutants, and its killed count was not printed.
  About a third of the candidates lie in the new code.
- There is no separate red run per function: the four functions were written in one step after RED A, and the
  fault table stands in for the per-function red evidence.
- The full `test_julia.sh` was not run (only `rfun_fourier.jl`), and neither was check-all (as the brief says).
- Avoidable costs:
  - pi and `2 pi A` are recomputed per term in the derivative and the integral;
  - norm2 copies the conjugate and integrates both `(k, l)` and `(l, k)`, which are conjugates of each other.
