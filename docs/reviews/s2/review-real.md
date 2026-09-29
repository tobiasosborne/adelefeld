# S.2 real-root adversarial review

## Findings

### 1. MAJOR: a small quadratic takes minutes at precision 2

Input: `f = 1234567 (X - 2^e)(X - 2^e - 1)`, `prec = 2`. It is a squarefree quadratic.
Its only real roots are the two planted integers. At `e = 1800`, every coefficient has fewer
than 3622 bits. `adf_roots_real` did not return within 175 seconds. The command was
`timeout 175 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/s2r-review/planted 4 2 1800 skip-verifier`.
It exited 124 with no output. The probe flushes a line immediately after `adf_roots_real`,
before its own oracle checks. No line appeared, so the call itself exceeded the timeout.

At `e = 1500`, the same function returned `ADF_OK`, `n = count = 2`; the program including
the oracle and verifiers took 97.30 seconds. At `e = 1200` the function and oracle took
9.40 seconds; at `e = 600`, 0.03 seconds. At `e = 3000`, the call did not finish within
170 seconds. These inputs have degree 2 and ask for only two bits of accuracy. The header
sets a precision limit but no coefficient-size or separation limit that admits these
times. FLINT's documentation says its isolation can use more internal precision than
requested (`refs/src/flint-3.0.1/arb_fmpz_poly.rst:98-105`). The code invokes that routine
at `src/roots.c:1497`. I did not attribute the full time to any narrower step.

Reproduce the completed case with
`timeout 120 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/s2r-review/planted 4 2 1500`.
It prints `status=0 n=2 count=2`, `errors=0`, `elapsed=97.30 peak_kb=47236` on this laptop.
The timing is a measurement, not a portable lower bound.

### 2. MINOR: the header understates the size of exact endpoint evaluations

Input: the same polynomial with `e = 1200`, `prec = 2`. The code returns `ADF_OK`, with
two accurate balls. The exact endpoint integers from `arb_get_interval_fmpz_2exp` have
1,048,572 to 1,048,576 bits; the midpoint mantissas have 1,048,573 and 1,048,576 bits.
The header says the exact evaluations are at dyadic points of about `prec` bits
(`include/adelefeld/roots.h:359-363`). Two bits and over one million bits are not close.
These are the exact endpoint integers as specified by
`refs/src/flint-3.0.1/arb.rst:461-466`. This does not make the answer false.

Reproduce with
`timeout 60 /usr/bin/time -f 'elapsed=%e peak_kb=%M' lanes/s2r-review/planted 4 2 1200 skip-verifier`.
It prints `status=0 n=2 count=2`, endpoint bit lengths `1048572,1048573` and
`1048576,1048576`, `errors=0`, `elapsed=9.35 peak_kb=10004`.

## Attacks without another result

- `planted.c` made 9 completed calls on 8 other planted polynomials at precision 2 or 53:
  a constant; a double root at 0 next to `2^-200`; roots `2^3000` and `2^-3000`;
  three small integers; `X^60 - 1`; `X^2 + 1`; and two rational roots with an extra
  `X^2 + 1` factor. It compared the list length with the count proved by the factors,
  checked each planted root lies in exactly one ball, called both verifiers, and checked
  output accuracy. All 9 calls returned `ADF_OK`, with 0 oracle errors. A wrong count,
  missing or duplicate planted root, rejected list, or low accuracy would have failed.
- The completed pair cases at `e = 300`, `600`, `1200`, and `1500` returned two balls and
  0 oracle errors. Their exact roots are `2^e` and `2^e + 1`. The `e = 1800` and `3000`
  cases gave no answer within their timeouts, so no correctness claim is made for them.
- `verify_hand.c` made 11 hand-built lists for `(X - 1)(X - 2)(X - 3)`. It tested a ball
  holding three roots, an omitted pair, a root at an endpoint, infinity, NaN, a radius
  of `2^1000000000`, a false count, wrong `g`, three exact roots, balls out of order,
  and touching balls. Expected predicate, entries, and complete results matched all
  11 cases, with 0 errors. A false complete acceptance or an abort would have failed.
- Inspection found no floating-point sign decision in the reviewed code. Candidate
  selection tests an exactly zero imaginary ball. The real sign check forms an integer
  numerator of `g` at each dyadic endpoint, and ordering uses exact `arf` comparisons.
  This is a code inspection result, not a proof for every malformed object.
- `build/test_roots_real` ran 8 tests and 10,666 checks with 0 failures. No test was found
  whose assertion is tautologically true. The test suite is not used as this review's
  oracle.

## Checks run

Every executable below was run under `timeout`. Builds used at most two cores.
The compile template for both probes was
`timeout 60 cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror FILE`
`build/libadelefeld.a -lflint -lgmp -lm -o OUT`.

Here `planted` abbreviates `lanes/s2r-review/planted` in command descriptions.

- `timeout 180 make -j2`: exit 0; library built.
- Compile `planted.c` using the template: 8 builds, all exit 0.
- `timeout 20 planted 0 2`: exit 0; 0 roots, 0 errors.
- `timeout 30 planted 1 2` and `timeout 30 planted 1 53`: both exit 0;
  2 roots each, 0 errors.
- `timeout 30 planted 2 2`, `timeout 30 planted 3 2`, `timeout 30 planted 5 2`:
  all exit 0; counts 1, 1, 3; 0 errors.
- `timeout 30 planted 7 2` and `timeout 30 planted 8 2`: both exit 0;
  counts 0 and 2; 0 errors.
- `timeout 120 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 6 2`:
  exit 0; 2 roots, 0 errors; 0.01 s.
- First `timeout 30 planted 4 2 300` and `timeout 40 planted 4 2 600`:
  exit 2; the probe rejected the fourth argument before calling the library. Fixed.
- Retried `timeout 30 planted 4 2 300` and `timeout 40 planted 4 2 600`:
  exit 0; 2 roots, 0 errors each; 0.00 s and 0.03 s.
- `timeout 60 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 4 2 1200`:
  exit 0; 2 roots, 0 errors; 9.40 s, 9788 KiB.
- `timeout 60 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 4 2 1500`:
  exit 124; no output within 60 s.
- `timeout 120 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 4 2 1500`:
  exit 0; 2 roots, 0 errors; 97.30 s, 47236 KiB.
- `timeout 170 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 4 2`:
  exit 124; no output within 170 s (`e = 3000`).
- `timeout 175 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 4 2 1800 skip-verifier`:
  exit 124; no return marker within 175 s.
- `timeout 60 /usr/bin/time -f 'elapsed=%e peak_kb=%M' planted 4 2 1200 skip-verifier`:
  two runs, both exit 0; 2 roots, 0 errors; 9.38 s and 9.35 s.
  The last printed endpoint sizes.
- Compile `verify_hand.c` using the template: exit 0.
- `timeout 20 lanes/s2r-review/verify_hand`: exit 0; 11 cases, 0 errors.
- `timeout 180 make -j2 build/test_roots_real`: exit 0.
- `timeout 170 ./build/test_roots_real`: exit 0; 8 tests, 10,666 checks, 0 failures.

The actual commands used the full `planted` path. Case numbers and precision arguments
are literal.

## Files written

`lanes/s2r-review/planted.c`, `lanes/s2r-review/verify_hand.c`, their compiled executables,
and this report. The required builds also produced files under `build/`.

## Not done

No sanitizer or leak run, no random fuzzing, and no independent reimplementation of
FLINT's real-root count. The timeouts give no correctness result for `e = 1800` or `3000`.

## Sources pending

None. The planted-root counts follow directly from the displayed factorizations and
`x^2 + 1 > 0` for real `x`.

## Findings against the specification

None found. The measured false claim is in the header's cost description, not in
`docs/SPEC.md` or Propositions 3.8 to 3.10.
