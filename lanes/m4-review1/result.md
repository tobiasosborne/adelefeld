# Lane m4-review1: adversarial review of milestone 4 (functions on the adeles)

I found no true value outside an enclosure, no wrong kernel sign and no memory error or leak. Nothing was ever written
on a failure status, and every planted fault was caught. There are four findings: one MAJOR, which is against the
F4 statement rather than the code, and three MINOR. There are also two notes on statements and headers.

Signs, derived before I read the lanes' texts. conventions.md:842-847 gives psi_inf(x) = E(-x) and
psi_p(x) = E(fp_p(x)), the standard character of tate-poonen notes.txt:693-700. The transform is
F_c f(y) = int f(x) conj(psi(xy)) dx (conventions.md:852-854, CV-54). Its kernels:
- Real kernel: conj(E(-xy)) = E(+xy).
- Finite kernel: a cell j/D + M Zhat has volume 1/M. At y = k/M, conj(psi_f(jk/(DM))) = E(-jk/L), so
  g_k = (1/M) sum_j f_j E(-jk/L).

Both agree with api-4.md:166 and :202. My own Poisson model checks the pair: left and right sums agree to 1e-71 on
600 tensors. With only one sign flipped they would differ.

Reproduction set-up. Every command below runs from the worktree root, after the builds:
`timeout 600 make -j2 BUILD=lanes/m4-review1/build lanes/m4-review1/build/libadelefeld.a`, then
`cc -O1 -Iinclude -o lanes/m4-review1/gen/h lanes/m4-review1/gen/h.c lanes/m4-review1/build/libadelefeld.a -lflint -lgmp -lm`
(and `gen/t` from `gen/t.c` the same way, plus `-ldl`). The input format of `gen/h` is in its header comment.

## Findings

### 1. MAJOR (against the statement F4, not the code): the transform radius exceeds the F4 bound
- **Input:** `adf_ffun_fourier` at prec 200, (D, M) = (1, 3), with f_j = 1 +/- 2^-1 (real) for j = 0, 1, 2.
- **Returned:** g_0 has a real radius of 0.5000000055879354 = 0.5 (1 + 2^-26.4).
- **What the statement gives:** F4 step 3 (api-4.md:178-180) bounds it by
  R_k <= (sum_j [r_j + (|m_j| + r_j) eta_j + delta_j])/M + delta_div. Here r_j = 1/2, eta_j <= 6 * 2^-200, and the
  midpoint operations are exact (phase E(0) = 1, sum 3, division 3/3), so R_0 <= 1/2 + 2^-196.
- **Cause:** the excess of 5.6e-9 is the upward rounding of the 30-bit mag radius in acb_mul, acb_add and
  acb_div_ui. F4 counts rounding of midpoints only ("delta_j bounds rounding of the midpoint"). In general the
  excess is about L 2^-30 sum_j r_j / M. Hunt 1 measured excesses up to 1e48 times the eta/delta part at p = 200
  with radii near 2^-3.
- **Verdict:** the enclosure is sound. The statement is false as written. F4 needs a term for the relative
  rounding of the radii.
- **Reproduce:**
  `echo "fourier 200 1 3 1 1 1 -1 0 1 0 0 1 1 1 -1 0 1 0 0 1 1 1 -1 0 1 0 0" | lanes/m4-review1/gen/h`.
  Field 4 of the first cell is the radius mantissa and field 5 its exponent.

### 2. MINOR: the shared dump validator calls fmpz_set_str before the grammar stage ends
- **Input:** `adf_ffun_load_str` on the refused text `adf1 Q ffun 1 fffffffffffffffff -43 0 0 0 -1b 0 0 0`.
- **Returned:** PARSE, output untouched, but fmpz_set_str was called twice before the refusal (once by load and once
  by inspect).
- **Cause:** `dp_w_other` (src/dump.c:1129-1134) is the grammar stage of the ffun body. It converts the D and M
  tokens with `dp_fmpz`, and a token longer than 15 hex digits goes to fmpz_set_str (src/dump.c:181-186). That
  happens before the remaining tokens are read.
- **Statements it breaks:**
  - api-4.md:93: "Strict lossless loaders; validate all tokens before FLINT."
  - dump.h:292-293: "Validate length, alphabet/header, full grammar, ... before constructing any value."
  - src/dump.c:27-28: "nothing is built before the whole text has passed stage 6."
- **Not a BLOCKER:** the bytes passed `dp_h` and were copied into a private buffer, and the scratch D·M is used
  only to check the token count.
- **Counts:** 59 of 20000 mutated dumps on the plain build and 63 of 20000 on SAN. All were ffun dumps with a
  D or M token longer than 15 digits. No rfun dump was affected, and arb_load_str and arb_set_str were never called.
- **Reproduce:**
  `echo "fload adf1 Q ffun 1 fffffffffffffffff -43 0 0 0 -1b 0 0 0" | lanes/m4-review1/gen/t`. Column 6 is the
  number of fmpz_set_str calls; it reads 2.

### 3. MINOR: no bound on running time for bits near the 2^21 cap
- **Input:** `adf_tensor_poisson` with phi = exp(-pi x^2), f = 1 on Zhat (D = M = 1), prec 64.
- **Returned:** the time grows about 20-fold each time bits grows 4-fold: 0.014 s, 0.047 s, 0.66 s and 13.0 s for
  bits 2000, 8000, 32000 and 128000. At bits = 2^21, which is in range (api-4.md:325), the call had not returned
  after 160 s; I extrapolate hours.
- **Why:** the cutoff is about 1024, so the D1 work units are far from 2^20. The units count evaluations, not their
  precision. The width 2^-2^21 cannot be reached below the precision cap, so the call can only end NOT_DETERMINED.
  The doubling retries reach the cap first.
- **What is wrong:** api-4.md:326-328 states the cost in evaluations and retries without the precision. No LIMIT is
  promised for this input, so this is not MAJOR.
- **Reproduce:** the line below (or the same line with 128000 for bits, 13 s):
  `echo "poisson 2097152 64 1 1 1 1 0 0 0 1 0 0 1 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 1 1 1 1 0 0 0 1 0 0" | timeout 160 lanes/m4-review1/gen/h`

### 4. MINOR: two suite tests compile code from a lane directory
- tests/test_ffun_dump.c and tests/test_rfun_dump.c are three-line wrappers around
  `#include "../lanes/f4-slice7/dump_test.h"`.
- The test code that the Check lines of api-4a.md:328-330 and api-4b.md:281-283 refer to therefore lives under
  lanes/, which is a work area. It holds the 2000-iteration round-trip loop at line 201 and the arb_load_str
  interposer at line 73. No other tests/*.c includes code from lanes/.
- Removing or cleaning that lane directory breaks `make check`.

### Notes on statements and headers (hunt 8)
- **HEADER-FINDINGs of f4-slice7:**
  - Cost: dump.h:297 now says "cost input size plus clearing old x". That is right.
  - Identity: dump.h:294 states the D1 caps with "Above caps: LIMIT". dump.h:298 says the design's wording "needs
    these qualifications". The header makes no identity claim at all, so it is not wrong. The design text
    api-4.md:105-106 still states identity without conditions, and the lane may not edit it.
- **Check lines of api-4a.md, 4b.md and 4c.md:** every named check exists. I verified:
  - in test_ffun.c: lifecycle, aliases_and_precision, invariants, text, sum, fourier, caps, allocation_preflight,
    and the 1000 init/clear count at :476-479;
  - in test_ffun_algebra.c: product, numeric_product, unary, idele with CRT lifts modulo lcm at :259, covariance,
    guarded_fourier, caps;
  - in test_rfun.c and test_rfun_fourier.c: the named functions;
  - in test_tensor.c: eval_vectors, local_vectors, exact_cases, sball_vectors, none_bounds, tensor_vectors,
    integral_vectors;
  - in test_poisson.c: vectors, direct, theta, witness, planted_tails, tail_checks;
  - in dump_test.h: 2000 round trips and the interposer.
- **"SAN with leak detection"** (api-4b.md:30), which the codex lanes could not run, now passes (hunt 6).
- I did not re-derive every count in the Check lines (for example "1651 comparisons" and "164 reference texts").

## What I attacked without a result

Every reference is my own: Fractions, exact integer cyclotomic coefficient vectors evaluated once at 2^-400, or
mpmath at 70-80 digits. Neither proto/functions4_checks.py nor any lane vector was used.

| Hunt | Count | Result | What would have made a case fail |
|---|---|---|---|
| 1 finite transform and algebra | 5005 ffun with (D, M) up to (12, 12); radii on 30% of entries; prec 2-200; 13.2M cell checks | 0 failures; 164 undecided | Exact transform of the centre and of a random member outside a cell; F(F f) not containing f(-x) with layout (D, M); Parseval not containing the exact (1/M) sum of abs(f_j)^2; a model value at cell representatives or off-grid rationals outside the result of add, mul, refine, translate (denominators 1, D, 2D, random), reflect, conj, dilate_rat (q with numerator and denominator up to 4 in size, either sign), dilate_idele (unit image by own lifts modulo lcm(N, L); OK only for a singleton, U on NOT_DETERMINED); covariance fourier(D_q f) against abs(q) ghat(y/q) |
| 2 real functions | 3000 rfun (1-3 terms, degree up to 6, Re A from 1/8 to 4, abs(Im A) up to 4, radii on 30%); 1.03M checks; 300k evaluations | 0 failures | Parameter balls of translate, dilate, mul, reflect, conj and derivative not containing the exact closure. Fourier: A' = 1/A and B' = iB/A exact, C' numeric, and C eval of F phi at 50 y outside my closed form, which is an independent Gaussian-moment derivation; quadrature agrees with it to 9.6e-61 over 60 integrals. F^2 eval at 10 x not containing phi(-x). Integral, and norm2 by the exact product phi conj(phi). Eval at 100 points per function (balls checked at both ends and a random interior point): max radius about 2^(-p+11) max(1, abs(v)) |
| 2 special cases | 22 calls | All correct | Translate exp(-pi x^2) by 1/3, then F, then eval at 1/4: Im = 0.41086 > 0 certified. Root branch with A in all four quadrants (1 +/- 3i, 1/8 +/- 5i): 20 values against quadrature. Prec 2 with A = 1/64 + 100i: OK statuses with valid enclosures, no write on failure |
| 3 evaluation E1 | 5000 global balls; about 4400 local balls (blocks 8, 9, 5; about 600 not representable); 5000 tensor_eval; 5000 sball on {2,3}, {5}, {}, {2}, {3,5} with NONE or REAL; 20 COMPLEX; 1000 rfun bounds times 1000 points | 0 failures; 0 exclusion failures | A value of f at some rational point (A + Hz)/d, z < L d D, outside the hull. This model enumerates points, not the gcd criterion. A single dyadic value not returned exactly; an unmet value beyond the hull inside the result; phi(x) f(x_f) outside the tensor result; for an sball, my own ultrametric criterion plus zero; COMPLEX not DOMAIN or written; abs(phi(t)) > R. R / max abs(phi) <= 310 |
| 4 Poisson | 300 tensors with (D, M) up to (6, 6), Re A down to 1/8, bits 20/53/80/128, prec 2 to bits+40 (all OK) | 0 failures | My 60-digit left or right value outside its ball; a coordinate diameter above 2^-bits; NL or NR not in {0, 1, 2, 4, ...}; the direct tail majorant beyond the returned cutoff above epsilon/8 |
| 4 special cases | - | All correct | Theta 1.0864348112133080145753 in both sides at bits 30, 53, 100, 200. Witness c in [1, 2]: NOT_DETERMINED, outputs untouched. bits -1 and 2^21+1: DOMAIN, untouched. prec 2^21+1: LIMIT first. Re A = 2^-20, 2^-30, 1e-8: LIMIT in 0.57 s or less; 2^-10: OK |
| 5 text and dump | 3000 texts from my printer; 3000 dump/load identities; 20000 mutated dumps (twice) | Only finding 2 | Printed ball not containing the input decimal; dump then load not OK or not byte-identical; an accepted mutation that does not re-dump byte-identically; a refused one written; inspect status different from load; arb_load_str or arb_set_str called; a non-hex fmpz_set_str argument. Two-rule order: 14 texts (PARSE before LIMIT before DOMAIN; NOT_DETERMINED for Re A = 0.1 +/- 0.0999999999 at prec 2) |
| 6 memory | SAN + INV build with ASAN_OPTIONS=detect_leaks=1, which works here (a planted malloc leak was reported): hunt 1 x 1000, hunt 2 x 1000, hunt 3 x 1000, hunt 4 x 60, hunt 5 with 3000 texts and 20000 mutations | 0 reports | |
| 6 lanes' tests under the leak detector | test_ffun, _ffun_algebra, _rfun, _rfun_fourier, _tensor, _poisson, _ffun_dump, _rfun_dump | All rc 0 with 0 reports; 5 to 30 s each | |

Undecided cases are references within the numeric error (2^-390 for the cyclotomic sums, 1e-55 for mpmath) of a
ball edge. On inspection they are exact outputs (radius 0) equal to the reference. A failing case would need a
violation larger than that error.

## Fault table (hunt 7)

Each fault was planted in a scratch copy of one source file and swapped into a copy of the archive. All 8 lanes'
tests were linked against it. The unfaulted baseline passes all 8. FAIL means the program's nonzero exit; -6 is an
abort on a CHECK.

| Fault | File | Lanes' tests that fail | Own hunt |
|---|---|---|---|
| F01 finite kernel sign E(+jk/L) | ffun.c | ffun, ffun_algebra, poisson | hunt 1: 5196 failures |
| F02 real kernel sign (B', 1/(2 pi i), i/A) | rfun.c | rfun_fourier, poisson | hunt 2: 994 |
| F03 weight 1/D for 1/M | ffun.c | ffun, ffun_algebra, tensor, poisson | hunt 1: 4740 |
| F04 reflection dropped (ffun_reflect) | ffun.c | ffun_algebra | hunt 1: 1520 |
| F05 lcm for gcd in E1 | tensor.c | tensor | hunt 3: 1181 |
| F06 right side derived from left | poisson.c | poisson | hunt 4: 9 |
| F07 tail without the factor 2 | poisson.c | poisson | hunt 4: 0, because my check bounds the true tail, not the certificate |
| F08 width checked before the tail is added | poisson.c | poisson | not run |
| F09 Re(1/A) > 0 not re-checked in rfun_fourier | rfun.c | rfun_fourier | not run |
| F10 rfun loader normalises trailing zeros | dump.c | rfun_dump | not run |
| F11 translation permutes the wrong way | ffun.c | ffun_algebra | hunt 1: 1383 |
| F12 content inverted in dilate_idele | ffun.c | ffun_algebra | hunt 1: 1 of 30 functions (weak) |
| F13 hull sampled at one point | tensor.c | tensor | hunt 3: 1028 |

Every fault is detected by at least one of the lanes' tests. Tests that miss a given fault are not counted against
it; for example test_poisson does not see F04, F11 or F12.

## Files

- Generators: lanes/m4-review1/gen/
  - h.c: harness for every call;
  - t.c: text and dump harness with interposed arb_load_str, arb_set_str and fmpz_set_str;
  - m.py, m2.py, hunt2_enc.py: own models;
  - hunt1.py to hunt5.py, special.py: the hunts;
  - faults.py: the 13 faults and the test runner.
- Notes: lanes/m4-review1/progress.md.
- Deleted at the end: the build trees, the executables and the outputs. Nothing was written in /tmp or outside the
  lane directory.

## Sources pending, findings against the specification

- Against the specification: finding 1 (F4, api-4.md:174-183, omits the rounding of the radii) and finding 3 (the
  cost statement of P1, api-4.md:326-328, omits precision).
- No source pending. The character citations are tate-poonen notes.txt:693-700 and :733-740, read on disk.
