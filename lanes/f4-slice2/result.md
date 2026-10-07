# Lane f4-slice2: result (slice 4d of docs/api-4.md, adf_rfun)

All work is in the worktree /home/tobias/Projects/adelefeld-wt/f4-slice2 (branch lane/f4-slice2, base a03fc50).
No git command that changes state was run, and no bd. Builds used `make -j2`, and every program ran under
`timeout`. Build trees, scratch copies and caches were removed at the end. The lane directory is 212 KB, and no
log in it is above 100 KB.

## Files

- New: `include/adelefeld/rfun.h`, `src/rfun.c`, `tests/test_rfun.c`, `tests/ref/vectors/f4-slice2/`
  (closure.jsonl, values.jsonl, texts.jsonl, 385276 bytes), `tests/driver/rfun-{text,algebra,eval}.{cmd,out}`,
  `tests/julia/rfun.jl`, `docs/api-4b.md`, `lanes/f4-slice2/{gen_vectors.py,plant_faults.py,redgreen.md,result.md}`
  and logs.
- Changed:
  - `src/text.c`: about 290 lines appended at the end, after the qclass and lball/sball sections. They hold
    the rfun reader, the rfun printer and their static helpers. Nothing above them changed.
  - `include/adelefeld.h`: one line.
  - `docs/conventions.md`: one clause added to the row "Integrals, Poisson summation" (line 225).
  - `tools/adf/adf.c`: 4 op enum values, 4 table rows, one dispatch line and the function `adf_drv_rfun`.
  - `tools/adf/README.md`: a new section at the end.
  - `tests/test_julia.sh`: one block after the psi block.

## A. Vectors

Command: `timeout 300 python3 -B lanes/f4-slice2/gen_vectors.py`. It is deterministic: a rerun gives identical
md5 sums.

- `closure.jsonl`, 135 records: 39 translate (q in 0, 1/3, -7/2 and a 2000/1000-bit rational), 50 dilate
  (h in 1, -1, 2/3, -5 and the 2000-bit rational), 11 reflect, 11 conj, 12 add, 12 mul.
  - Inputs: terms with dyadic, non-dyadic (6/5, 7/10, -1/3) and boundary (`Re(A) = 10^-30`) parameters.
  - Polynomials have degree 0 to 6 and may hold ball coefficients and an inexact zero top coefficient.
  - Exact results come from the oracle's `rterm_translate`, `rterm_dilate` and `rterm_product` (SymPy), in the
    form `(a + b pi) + i (c + d pi)`. The midpoint and both corners of every ball input are listed.
  - Sum, reflection and conjugation have no oracle function; they are written in the script from
    api-4.md section 5.
- `values.jsonl`: 12 functions at 40 points each (36 exact rationals and 4 balls with 3 sampled values each),
  plus the shifted Gaussian at 5 points. The values come from the oracle's `rvalue_ball` (python-flint at 400
  bits), printed as arb balls with 40 digits.
- `texts.jsonl`: 160 random rfun texts over dyadic decimals and 4 `max_items = 3` boundary texts. The expected
  canonical text or status comes from `proto/text_grammar.py` `canonical` (70 OK, 63 DOMAIN, 22 PARSE,
  9 LIMIT).
- The caps are checked in C, not by vectors:
  - 2^16 terms are accepted by `set_terms` with a real array.
  - 2^16 + 1 terms are refused both with a one-element array (LIMIT is decided before any read) and with a
    full array.
- The oracle runs: `timeout 120 python3 -B proto/functions4_checks.py` gives TOTAL 2670 checks.

## B and C. Tests, then code

- `tests/test_rfun.c` was written in full first. RED was a link failure with 251 undefined references
  (`red-a.log`).
- Going GREEN needed four corrections to the test, all test defects and none in the code (`redgreen.md`):
  1. The round trip of reprinted texts now checks containment, not identity (conventions 11.3 item 3: a
     printed radius 0.063 is not dyadic).
  2. Exactness is asserted only where no rounding enters: no pi part, dyadic q, inputs stored exactly.
  3. The tightness check is restricted to exact inputs.
  4. A gcc false `-Wstringop-overflow` warning was silenced by making two test helpers noinline.
- Final runs, `make -j2 [VAR] BUILD=lanes/f4-slice2/<dir> lanes/f4-slice2/<dir>/test_rfun`, then the program:

  - plain: `rfun: 64923 checks`, exit 0 (0.5 s);
  - `SAN=1` with `ASAN_OPTIONS=detect_leaks=1`: 64923 checks, exit 0; LeakSanitizer is active here (a 77-byte
    leak probe was reported);
  - `INV=1`: 64939 checks, exit 0, including 8 forked children that must abort on a non-canonical argument;
  - `CC=clang`: 64923 checks, exit 0.

- What the test covers:
  - every golden row: all 19 of `tests/golden/rfun.tsv`; status rows also check, by sentinel bytes, that x is
    untouched;
  - every vector record;
  - the shifted Gaussian: B contains 2 pi/3 and C contains -pi/9. With B negated, `phi(1/3) != 1` and
    `phi(-1/3) = 1`;
  - dilation by 0 gives DOMAIN, also on the zero function;
  - idele dilation by interval squaring, for `1 +/- 0.875`, `-2 +/- 0.5`, an exact -2 (identical to
    `dilate_rat`) and `2^-100 +/- 2^-101`;
  - `Re(A)` lost at prec 2 gives NOT_DETERMINED with y untouched;
  - sum and product order; `x + y` against `y + x` (not identical, but the values overlap);
  - aliasing of every permitted combination, with nonzero radii;
  - prec 2, 0, -5, 53, `ADF_REAL_PREC_MAX`, and the maximum + 1 (LIMIT is decided first);
  - evaluation: DOMAIN for nonfinite x, NOT_DETERMINED for `C = 1e300`, a wide x ball;
  - caps and their exact boundaries;
  - INV aborts.
- Text tests: all eight `tests/test_text_*` programs pass in a lane build: adele 84390 checks, classify 20776,
  fball 21545, idele 73465, limits 105, local 68868, r3r9 148, rat 18122, 0 failed.
- `sh tests/test_exports.sh`: 544 of 544 declared functions are exported; none are undeclared.

## D. User calls

- Driver commands: `rfun R`, `rfun_translate R with Q`, `rfun_mul R with S`, `rfun_eval R with X`.
  - `X` is a real ball, read by the adele reader as `(X ; 0)`.
  - A wrong operand kind gives DOMAIN.
- Fixtures: `rfun-text` (7 commands), `rfun-algebra` (7) and `rfun-eval` (6), with expected lines derived by
  hand before the first run (digits from mpmath at 50 digits, printer of 9.5). Examples:
  - B = `2.0943951023931954923 +/- 8.5e-21`
  - C = `-0.34906585039886591538 +/- 4.8e-21`
  - `exp(-pi/4)` at digits 5 = `0.45594 +/- 1.9e-6`
  - the sign test at x = -1/2 gives `0.043214 +/- 8.2e-8`
- All three agreed on the first run. No red run of the driver was made: the commands were coded first.
- `sh tests/test_driver.sh`: 82 cases, 101473 expected lines, all equal (SAN=0).
- Julia: `tests/julia/rfun.jl` calls `ccall((:adf_rfun_translate_rat,lib),Cint,(P,P,P,Clong),out,r,q,128)` on
  storage from the layout queries, inside `GC.@preserve`, and frees every string. It is registered in
  `tests/test_julia.sh`. Run directly against `build/libadelefeld.so`: 9 of 9 passed. The full `test_julia.sh`
  was not run.

## E. Statements

`docs/api-4b.md` has 14 numbered statements with their "Check:" references. It also covers:

- the order of checks, the caps and the work units;
- the Horner Taylor-shift proof;
- the interval-squaring argument;
- the decisions where the design is silent, each with its alternative;
- cost.

## F. Faults (scratch copies; `plant_faults.py`, removed after the run)

| Fault | Result |
|---|---|
| B sign under translation | caught |
| C without -pi A q^2 | caught |
| C with +B q | caught |
| dilation of A by h, not h^2 | caught |
| dilation loses the sign of h in B | caught |
| product adds only A | caught |
| product multiplies A | caught |
| product order j before i | caught |
| sum order y before x | caught |
| reflection forgets B | caught |
| conjugation forgets A | caught |
| Re(A) > 0 not re-checked after rounding | caught |
| y written before the last check (product NOT_DETERMINED path) | caught |
| evaluation with +pi A x^2 | caught |
| evaluation of P at the midpoint of x only | caught (second run; the first spelling did not compile) |
| translation shift with +q | caught |
| dilation by 0 accepted | caught |
| idele dilation by h h (two signed copies) | caught |
| inexact trailing coefficient removed (reader) | caught |
| reader: Re(A) <= 0 not DOMAIN | caught (exit 134, an abort) |
| printer: Re(A) unconstrained | caught |
| terms cap off by one | survived in the first run (test gap); caught after a test with 2^16 + 1 real terms |

Result: 22 of 22 caught (`faults.log`).

### Mutation run

Command:

    ASAN_OPTIONS=detect_leaks=0 timeout 1260 python3 -u tools/mutate/mutate.py --root . --scratch <scratchpad> \
      --files src/rfun.c --limit 60 --seed 404 --jobs 1 --timeout 120 --san \
      --make "make -s -j2 check INV=1 TEST_SRC='tests/test_rfun.c'" --copy Makefile include src tests

- 505 candidates; the baseline took 23.0 s.
- The run stopped at the 20-minute cap (exit 124). The tool prints no summary when it is stopped, so the number
  of killed mutants is not known.
- Listed outcomes: 13 survived, 7 not compiled, 1 timed out. The timeout is `while(0)` -> `while(!(0))` in the
  INV macro, an endless loop.
- 11 survivors were test gaps. Tests were added, and all 11 mutants were planted again and caught
  (`plant_faults.py --survivors`, INV build, 11 of 11).

| Survivor | Test added |
|---|---|
| 238:14 | `set_terms` validated from term 1: first term invalid |
| 289:16 | mul input cap: inputs above the caps read from text |
| 290:30 | terms boundary: 256 * 256 accepted |
| 294:14 and 298:25 | coefficients boundary: 256 constants times length 256 or 257 |
| 299:16 | the same coefficients boundary |
| 299:73 | work boundary: 1024 * 1024 accepted |
| 330:18 and 344:43 | translation work exactly 2^20, including a zero polynomial |
| 105:13 | `P = [0]` |
| 259:5 | INV entry check of x alone |

The 2 remaining survivors are equivalent (one line each):

- 381:23 `k + 1 < L` -> `k - 1 < L`: the extra passes k = L-1 and L have empty inner loops (`j` starts at
  `L - 2 < k`).
- 381:27 `k + 1 < L` -> `k + 1 <= L`: the same; the extra pass k = L-1 has an empty inner loop.

## Findings against the design, the oracle and the goldens

1. The brief's example `adf rfun 'rfun(term(P=[1], A=1, B=0, C=0))'` is not a sentence of `rfun_v`: a
   coefficient and A, B, C are complex numbers `(r) + (r)*i`. The driver answers PARSE (fixture `rfun-text`).
   Design section 9 item 4 writes `adf rfun_translate R 1/3` without the driver's separator `with`.
2. Design section 1 charges "one unit per dense polynomial coefficient multiply-add", and section 5 states the
   cost `O(sum(degree+1)^2)`. I charged translation the n(n-1)/2 multiply-adds that it performs. The other
   reading refuses lengths 1025 to 1448 that this one accepts.
3. Design section 2 does not say whether the reader's stage 7 may use the idele-class kernel B before
   NOT_DETERMINED. I used it. With the enclosure alone, `1 +/- (1 - 10^-59)` would be NOT_DETERMINED at every
   prec, because a `mag` radius has 30 bits.
4. Design section 5's sentence "For an idele use h=a.inf ... and r,u in F3" mentions r, u. For the real
   function only `a.inf` enters; r and u concern ffun (F3).
5. The oracle `proto/functions4_checks.py` has no function for sum, reflection or conjugation. These vectors
   come from the design text, not from an independent oracle.
6. Goldens: all 19 rows pass. The row with radius 0.999999 is not exact at prec 128. It was checked by
   containment per conventions 11.3 (reread text contains the value; the input interval lies in the ball); its
   printed text was not compared with the golden's second column.
7. HEADER-FINDING (placement only): design section 2 declares `adf_rfun_set_str` and `adf_rfun_get_str`
   without naming a header. `text.h` is not owned by this lane, so they are declared in `rfun.h` and defined in
   `src/text.c`.
8. No counterexample to Proposition 5 or the design formulas was found.

## Sources pending

None new. The FLINT lines relied on are cited in `src/rfun.c` and in the header comment of the text.c section
from `refs/src/flint-3.0.1/{acb_poly,acb,arb,arf}.rst`. The Taylor-shift identity is proved in `docs/api-4b.md`
statement 9.

## Not done

- The full `sh tests/test_julia.sh` was not run (only `rfun.jl`), and neither was check-all (as the brief says).
- The mutation run did not reach 60 judged mutants within 20 minutes, and its killed count was not printed.
- The faults were run before the survivor tests were added. Those tests only add checks.
- Avoidable cost noted: translation recomputes `pi A` for each term.
