<!-- ROLE: record of a review. The text below the rule is the report of lane f-review9 as written
     (lanes/f-review9/report.md, codex gpt-6.1-sol xhigh, 2026-10-04), not edited. Paths are relative to
     the worktree of the lane at commit 058a32c. Repairs: lane f-repair7. The binaries, rat.tsv, real.tsv
     and the fault build trees of the lane are not committed; oracle.py and faults.py regenerate them. -->

# f-review9

Review of lane f-slice10, WP 1F.8. Work ran on 2026-10-03 and 2026-10-04.
Only lanes/f-review9 was written. No git state changes, tracker commands, or subagents were used.

## Findings

### F1. MAJOR: the tests accept a false certificate for inexact ideles at higher degrees

This is a test gap. The unmodified implementation returns the documented status.

Input: x = (1 ; 1 * [1 mod 8]), n = 4, sign = 1, prec = 64.
Baseline: NOT_DETERMINED. Fault F13: OK, with real part 1, content 1, and exact unit [1].
The mutant replaces src/gfunc.c:228 with:

    if (adf_ucoset_is_exact(&x->u) || n >= 4)

The lane's test_gfunc returns exit 0: 10 tests, 1088693 checks, 0 failed checks, 0 failed tests.
tests/test_gfunc.c:748-765 exercises valid higher-degree inexact units only at degrees 2 and 3.
The rational fixture conversions all have exact units.

Why this fault is false, step by step:

1. Take a finite unit w with w_3 = 2 and w_p = 1 at every other prime.
2. Its 2-adic coordinate is 1, so w belongs to [1 mod 8]. Thus (1, w) belongs to x.
3. A fourth root of 2 in Q_3 would have valuation 0. Reducing its unit modulo 3 gives z^4 = 2.
4. The two nonzero residues modulo 3 are 1 and 2. Both have fourth power 1.
5. Thus this point of x has no fourth root. The mutant cannot certify the whole input domain.

Reproduce the missed fault:

```sh
timeout 180 python3 lanes/f-review9/faults.py F13_inexact_idele_high_degree
timeout 180 lanes/f-review9/faults/F13_inexact_idele_high_degree/build/test_gfunc
timeout 180 lanes/f-review9/repro_inexact
timeout 180 lanes/f-review9/repro_inexact-fault
```

The two reproducers return exits 0 and 1 respectively. Their logs are inexact-baseline.log and
inexact-fault.log. The original tests' log is faults/F13_inexact_idele_high_degree/test.log.

### F2. MINOR: degree-1 identity tests miss changes to a nonnormal unit representation

This is a test gap. The unmodified implementation preserves the representation.

Input: x = (1 ; 1 * [5 mod 6]), n = 1, sign = 0, prec = 2.
Baseline: OK, output unit [5 mod 6], identical = 1, canonical = 1.
Fault F11: OK, output unit [2 mod 3], identical = 0, canonical = 1.
The mutant calls adf_ucoset_normalise on the output after the degree-1 copy.
test_gfunc still returns exit 0: 10 tests, 1088693 checks, 0 failed checks, 0 failed tests.

The sets coincide, but the header promises an exact copy at degree 1: include/adelefeld/gfunc.h:129-130.
The input is canonical and deliberately not normal. No arithmetic or rounding is needed for the identity.
The existing identity test uses [1 mod 8], which is already normal.

Reproduce:

```sh
timeout 180 python3 lanes/f-review9/faults.py F11_inexact_idele_degree_one_normalised
timeout 180 lanes/f-review9/faults/F11_inexact_idele_degree_one_normalised/build/test_gfunc
timeout 180 lanes/f-review9/repro_identity
timeout 180 lanes/f-review9/repro_identity-fault
```

The reproducers return exits 0 and 1. See identity-baseline.log and identity-fault.log.
The independent status matrix additionally checks 144 calls on a canonical nonnormal idele.

### F3. MINOR: the new functions omit the promised debug invariant entry checks

This concerns ADF_CHECK_INVARIANTS. Invalid inputs to a release build are outside the contract.

Inputs: an initialized adele with finite part exactly 0 and a NaN real part; an initialized idele with
content 1, exact unit [1], and a NaN real part. Both canonical predicates return 0.
Call adf_adele_root or adf_idele_root with n = 2, sign = 1, prec = 64; also call adf_adele_exp.
All three return DOMAIN and exit 0 in the debug build used below.

docs/conventions.md:320 requires a noncanonical input to trigger an entry check and flint_abort with a message
under ADF_CHECK_INVARIANTS. gfunc.c has no such checks. A control call to adf_adele_set on the same input aborts
with the invariant diagnostic and exit 134. The direct callees were also compiled with the flag.

Build and reproduce:

```sh
timeout 180 cc -std=gnu11 -O1 -g -DADF_CHECK_INVARIANTS -Iinclude \
  lanes/f-review9/repro_invariants.c src/gfunc.c src/adele.c src/fball.c src/rat.c \
  src/ucoset.c src/idele.c src/rfunc.c src/sball.c src/modctx.c \
  lanes/f-review9/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review9/repro_invariants
timeout 180 lanes/f-review9/repro_invariants
timeout 180 lanes/f-review9/repro_invariants idele
timeout 180 lanes/f-review9/repro_invariants series
ulimit -c 0
timeout 180 lanes/f-review9/repro_invariants control
```

Logs: invariants-root.log, invariants-idele.log, invariants-series.log, invariants-control.log.
This is not a claim of a wrong result on valid inputs or of undefined behaviour on valid inputs.

### F4. MINOR: a false inequality in G1's proof

docs/api-1f8.md:71 says that q = r^n with integer r and q >= 2 implies r >= 2.
Counterexample: q = 4, n = 2, r = -2. Then r^n = 4 but r >= 2 is false.
The unmodified library returns OK and the root -2 for sign = -1.

The required step is abs(r) >= 2. It still gives q >= 2^n and bits(q) >= n + 1.
This repairs the proof step without changing the statement or implementation.

```sh
timeout 180 lanes/f-review9/repro_proof
```

Exit 0. Output: q=4 n=2 r=-2 r^n=4 r>=2=0 abs(r)>=2=1 status=OK. See proof.log.

## Attacks without an established baseline failure

Own exact oracle: oracle.py imports no repository oracle, C code, FLINT, or Arb.
It reduces rational inputs with Fraction and finds integer roots by bisection using exact integer powers.
There are 6168 rational rows: 764 OK and 5404 DOMAIN. Numerators reach 20850 bits; denominators 20803 bits.
Cases include 0, 1, -1, positive and negative perfect powers and their neighbours, and independently large
denominators. Degrees include 0, 1, degrees around the operand bit length, WORD_MAX, WORD_MAX + 1, and UWORD_MAX.
Selectors include INT_MIN, -1, 0, 1, 2, and INT_MAX.

attack.c applies the rows to rational roots, adeles with an independent real coordinate 1, and exact-unit
ideles with real coordinate 1. Each case has a distinct output, y = x, and where = NULL variant.
A case fails on a status, named place, exact finite root, content, unit, canonical predicate, or identity
different from the oracle. Status failures must preserve the output. OK must preserve where.

The real oracle has 14652 rows: 10752 roots and 780 samples for each of the five series.
It uses outward interval endpoints at 1200 bits for roots. Exact rational endpoint roots use exact integers.
For tiny series arguments it uses 1200 + 2 * max(0, -e, -re) bits, up to 9392 bits.
Inputs are exact dyadic midpoint/radius data. Exponents range from -4096 to 4096.
Root degrees include the three word boundaries above. Precisions are -7, 0, 1, 2, 3, 24, 64, and 257.
Positive, negative, exact-zero, zero-touching, and zero-crossing balls are included.

For roots, both endpoint images must be enclosed. For series, five points per input are checked:
the endpoints, midpoint, and two quarter points. This sampling is not a proof of the full trigonometric image.
Both outward endpoints of each interval value must be contained without adding a tolerance to the C result.
Finite constants must be the exact integers 1, 0, 0, 1, 1.
The final attack has 98604 calls, 684324 checks, 0 failures.

status_attack.c has 29880 adele-root calls, 3240 series calls, and 144 idele-identity calls.
The total is 33264 calls, 155134 checks, 0 failures.
It combines five real inputs with exact finite parts 0, 1, -1, balls 0 + 4 Zhat and 4 + 8 Zhat, and a local
backend. It crosses seven degrees, six selectors, and eight precisions, including LONG_MIN, the cap, and
one above the cap. It checks ordering, status maxima, place ties, aliases, and unchanged outputs.
The huge real input is 2^1000. exp, sinh, cosh with finite 0 return NOT_DETERMINED at the real place;
with finite 1 they return DOMAIN at 2. These agree with rfunc.h and conventions 3.3.
There were 720 explicitly skipped costly evaluations exactly at the precision cap. They are not passes.

place_attack.c uses its own sieve of 1229 primes. It constructs and reduces 500 rationals, including
large denominators, signed numerators, and products of up to 199 initial odd primes.
The largest denominator has 14971 bits. All five series, aliasing, and where = NULL give 7500 calls,
0 failures. A case fails if the first failed prime differs, the status differs, or the output changes.

One cost measurement: cost.c constructed a 100000-bit numerator divisible by the first 6913 odd primes.
Its independent sieve predicted prime 69761. adf_adele_exp returned DOMAIN at 69761 in 0.014608318 seconds.
This measured input does not make a caller wait minutes. No extrapolation to untested sizes is claimed.
The previously reported cost itself is not reported again as a new finding.

## Driver and Julia

driver_attacks.py has 156 own cases, 0 mismatches. The driver exits 1 because the script includes errors.
All thirteen operand kinds were tried. Root accepts rationals, adeles, and ideles.
The series accept rationals and adeles. The four other implemented kinds are DOMAIN for these commands;
the six kinds without a typed parser are UNSUPPORTED, before operation dispatch.
The latter are local ball, partial ball, quotient class, finite function, real function, and character.

The cases cover signs 0, 2, -1 at odd degree, nonintegers, INT_MIN, INT_MAX, and integers outside int;
degrees 0, negative, noninteger, 2^64, and the word boundaries; extra and missing operands; a NUL byte;
unbalanced delimiters; a trailing comment-like string; and a 10000-digit fourth operand.
Degree 1 accepts the tested signs within int. An oversized sign is DOMAIN during driver operand conversion.
That restriction is stated in tools/adf/adf.c:2477-2479.

Three old-command trials per arity were checked with one too few and one too many operands.
Arity 1: show, neg, inv. Arity 2: add, mul, cap. Arity 3: roots_at, powunit_at, recover.
Only two distinct commands have arity 4: root_at and powrat_at; the third trial repeats root_at.
All 24 wrong-arity lines give PARSE.

Six selected golden files were compared byte for byte, without running the repository driver suite:

| Golden | Expected lines | Exit | Different lines |
|---|---:|---:|---:|
| gfunc-root | 43 | 1 | 0 |
| gfunc-series | 23 | 1 | 0 |
| 10_line_language | 6 | 0 | 0 |
| 12_status_order | 13 | 1 | 0 |
| root-status | 15 | 1 | 0 |
| pow-status | 20 | 1 | 0 |

These comparisons include the new printed roots and constants, and existing status/arity expectations.
The place reported by the library is not printed by the driver, as documented.
tests/julia/gfunc.jl ran against a lane-local shared library: 4 series assertions and 7 root assertions,
0 failures. No additional Julia binding layer is introduced by this slice.

## Planted faults

All faults are scratch changes to gfunc.c. The repository source remains unchanged.
Each scratch directory has a copy of the archive, a copied Makefile, its own gfunc.c and build directory,
and read-only symlinks to include and tests. make replaces only the scratch gfunc archive member.
Every fault compiled with exit 0. Every test had a timeout of 180 seconds.

| Fault | Change | Failed checks | Failed tests | Test exit |
|---|---|---:|---:|---:|
| F01 | Apply the even selector to the real part only | 1078 | 7 | 1 |
| F02 | Write where on OK in combine | 3940 | 5 | 1 |
| F03 | Always set the idele result unit to [1] | 471 | 2 | 1 |
| F04 | Take the real root at prec - 1 | 1028 | 2 | 1 |
| F05 | Start the failing-prime search at 3 | 3097 | 3 | 1 |
| F06 | Round the real ball at adele degree 1 | 160 | 1 | 1 |
| F07 | Check degree 0 before LIMIT for adele roots | 2 | 1 | 1 |
| F08 | Leave where untouched on equal failed statuses | 4408 | 4 | 1 |
| F09 | Store signed rho as the idele content | 941 | 2 | 1 |
| F10 | Reject sign -1 on the exact zero adele at odd degree | 13 | 3 | 1 |
| F11 | Normalize an idele's unit at degree 1 | 0 | 0 | 0 |
| F12 | Reject series precision below 2 after real evaluation | 20 | 1 | 1 |
| F13 | Treat inexact idele units as exact when n >= 4 | 0 | 0 | 0 |

13 faults, 11 detected, 2 survived. F11 and F13 have independent reproducers above.
The complete per-fault test totals and logs are retained in fault-results.json and faults/.

## Proof audit

G1(a): checked reduced powers, uniqueness of reduced rational form, coprimality of the extracted roots,
the even sign condition, and the two rational branches. No counterexample found.
G1(b): step 2 has F4. Using abs(r) repairs it. The conclusions for 0 and 1 and the bit-length cutoff hold.
For q = 2 and n = 1, 1 < bits(2); the cutoff does not reject it. Degree 1 returns before int_root.
G1(c): the cutoff precedes the cast. Huge tested degrees never reach fmpz_root.
The printed universal bound bits(f) <= WORD_MAX still needs the representation bound listed below.

G2: checked each step independently. A local root gives n * v_p(root) = v_p(a).
If any exponent of a is not divisible by n, its own prime is a failed finite place, including prime 2.
If all exponents are divisible, abs(a) = r^n for positive rational r.
Odd degree supplies the sign. For even degree and a = -r^n, divide a hypothetical Q_3 root by r.
Raising to n/2 gives a square root of -1. Its valuation is 0 and its residue would square to 2 modulo 3.
The squares modulo 3 are 0 and 1. This proves the remaining obstruction even when r is divisible by 3.
For the other direction, a rational root embeds everywhere. For a family of local roots, valuation is 0
outside finitely many scale primes, so the family is in the restricted product. No root of zero is nonzero.
The independent real coordinate cannot remove a failed finite coordinate. No counterexample found.

G3(a): raising the selected real root and exact rational root to n gives the two input coordinates.
Negation at even degree changes both roots and preserves their powers. At odd degree the unique roots may
have different signs because the two input coordinates are independent. Enclosures may be wider than a branch.
G3(b): checked every status pair reached by the matrix, including a finite DOMAIN above real uncertainty,
and tied failures with the real place first. Invalid selectors are checked before value domains.
G3(c): a positive-radius finite ball projects to Z_p outside finitely many scale primes.
Choose such a p and a member with valuation 1. It has no degree-n root for n >= 2.
This supplies a valuation argument without relying on the proof's unspecified phrase "sign class".
Returning NOT_DETERMINED makes no false existence or nonexistence certificate.

G4(a): the finite-precision obstruction follows by the construction in Proposition 16 steps 1-2.
A prime divisor of 1 + A + ... + A^(ell-1) is outside the restricted list; A has order ell there.
Thus ell divides p-1 and the n-th-power map on the nonzero residues is not onto.
A point of the unit coset can choose a failed residue there. This obstructs a whole-input certificate.
The real DOMAIN exception must be combined as in G3. The unqualified G4(a) status wording still describes
the finite obstruction alone; the real-negative exception is already in the lane's known findings.
G4(b): checked q = c r, abs(rho) > 0, and the exact unit [sign(rho)]. In particular a negative odd rational
root needs unit [-1], even though the requested selector is +1. The numerator sign test implements this.
G4(c): checked nonzero output before storing an idele. The known low-precision widening is not a new finding.

G5(a): the finite values are the constant terms. The real image is delegated to the real functions.
For the hyperbolic oracle, add or subtract the exponential series at x and -x and divide by 2.
Even terms remain in the sum; odd terms remain in the difference. This gives cosh and sinh respectively.
G5(b): a nonzero rational has valuation 0 outside its finitely many numerator and denominator primes,
so it fails at an odd prime. G5(c): a positive-radius ball has points outside D; NOT_DETERMINED is not a proof.
No new counterexample to the statements was established.

G6(a): for reduced A/B, a prime dividing B cannot divide A. The numerator conditions decide each domain.
G6(b): the independent sieve agrees with the search, including negative numerators and reduced fractions.
G6(c): if the first k odd primes divide nonzero A, their product divides abs(A) and is at least 3^k.
Consequently one of the first floor(log_3 abs(A)) + 1 candidates fails. This bounds the candidate count.
It does not itself prove the extra machine-word claim in src/gfunc.c:259-261; see sources pending.

Local source text checked: refs/src/flint-3.0.1/fmpz.rst:982-988 specifies the signed degree and exactness result
of fmpz_root. fmpz.rst:605-608 specifies the bit count. ulong_extras.rst:688-692 requires the next prime to fit
ulong. arb.rst:979-987 describes the real-root algorithms. arb.rst:593-600 defines exact zero and exclusion
of zero; arb.rst:639-649 gives the real sign predicates; arb.rst:1209-1219 gives sinh/cosh evaluation.
The mathematical arguments above are written out here; they do not rely on remembered external conventions.

## Commands and results

Commands ran from the repository root unless stated otherwise. All test programs and Python scripts used
timeout. The archive was built once with the prescribed two-job command:

```sh
timeout 600 make -j2 BUILD=lanes/f-review9/build lanes/f-review9/build/libadelefeld.a
timeout 180 make -j2 BUILD=lanes/f-review9/build lanes/f-review9/build/test_gfunc
timeout 180 lanes/f-review9/build/test_gfunc
```

Exits 0, 0, 0. Baseline: 10 tests, 1088693 checks, 0 failed checks, 0 failed tests.
Logs: build.log, baseline-build.log, baseline-test.log.

Driver and shared-library builds:

```sh
timeout 180 cc -std=gnu11 -O2 -g -Iinclude tools/adf/adf.c \
  lanes/f-review9/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review9/adf
timeout 180 cc -shared -fPIC -Iinclude -std=c11 -O1 -g src/*.c \
  -o lanes/f-review9/libadelefeld.so -lflint -lgmp -lm
```

Both exit 0. The shared build is separate from the archive and serves only the Julia call.

Own oracle generation:

```sh
timeout 180 python3 lanes/f-review9/oracle.py
timeout 180 python3 lanes/f-review9/oracle.py --real-only
```

The first attempt exits 1 after writing rat.tsv: this mpmath interval context has no sinh/cosh methods.
The first real-only run exits 0, producing 14652 rows, but its numerical oracle needs the corrections below.
The final real-only run exits 0 with exact dyadic roots and adaptive tiny-argument precision.
See oracle.log and oracle-real.log. No repository oracle or fixture was changed.

For attack, status_attack, and place_attack, each compile used this form, with P replaced by the program name:

```sh
timeout 180 cc -std=gnu11 -O2 -g -fsanitize=address,undefined -Iinclude \
  lanes/f-review9/P.c src/gfunc.c lanes/f-review9/build/libadelefeld.a \
  -lflint -lgmp -lm -o lanes/f-review9/P
ASAN_OPTIONS=detect_leaks=0 timeout 180 lanes/f-review9/P
```

attack additionally used -fno-omit-frame-pointer. Each final compile and run exits 0.
Final numerical results: attack 98604 calls, 684324 checks, 0 failures; status_attack 33264 calls,
155134 checks, 0 failures; place_attack 7500 calls, 0 failures.
ASan and UBSan instrument the attack programs and gfunc.c. The other archive objects are release builds.
No address or undefined-behaviour sanitizer diagnostic appeared in these final runs.
Leak checking was not completed: the initial attack run exits 1 with a fatal LeakSanitizer ptrace diagnostic.

Earlier attempts are not hidden:

- attack's first compile exits 1 on nonexistent adf_adele_zero, adf_adele_one, and adf_idele_one symbols.
  Field setters replaced them.
- The first completed numerical attack exits 1 with 2832 failed checks. The interval oracle widened exact
  dyadic roots and lost tiny sinh/cosh cancellation. Exact integer roots and higher interval precision fixed
  the oracle. No tolerance was added. The final run contains the full outward oracle endpoints.
- The first status_attack run exits 124 at 180 seconds, before buffered results were printed.
  The final run explicitly skips 720 costly precision-cap cases. Their successful completion is not claimed.
- repro_identity's first run exits 2 because its literal input length was wrong. strlen replaced it.
- repro_invariants's first link exits 1 for missing debug borrow/release symbols. Adding src/modctx.c fixes it.
- repro_proof's first link exits 1 for nonexistent fmpz_cmpabs_ui. fmpq_abs and comparison replace it.

The small reproducers and cost.c used:

```sh
timeout 180 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review9/P.c \
  lanes/f-review9/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review9/P
timeout 180 lanes/f-review9/P
```

P = repro_identity, repro_inexact, repro_proof, cost: final compile and baseline run exits 0.
For the two fault reproducers the same compile substitutes the corresponding scratch archive and output
name with the suffix -fault. Compiles exit 0; runs exit 1 as reported in F1 and F2.
The cost result is the single timing given above, in cost.log.

Fault runs:

```sh
timeout 180 python3 lanes/f-review9/faults.py
timeout 180 python3 lanes/f-review9/faults.py F13_inexact_idele_high_degree
```

The first invocation at that time contains F01-F12; the second adds F13. Both script exits are 0.
For each fault NAME, the script runs from lanes/f-review9/faults/NAME:

```sh
timeout 180 make -j2 BUILD=ABSOLUTE_SCRATCH_BUILD ABSOLUTE_SCRATCH_BUILD/test_gfunc
timeout 180 ABSOLUTE_SCRATCH_BUILD/test_gfunc
```

ABSOLUTE_SCRATCH_BUILD is the absolute path of that scratch directory's build subdirectory.
Build and individual test results are in the fault table. No fault test times out.

Driver and Julia runs:

```sh
timeout 180 python3 lanes/f-review9/driver_attacks.py
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libgmp.so.10 JULIA_NUM_THREADS=1 \
  timeout 180 julia --startup-file=no tests/julia/gfunc.jl lanes/f-review9/libadelefeld.so
```

Both final commands exit 0. The Python script records child driver exits and output comparisons.
Initial driver expectations had 12 mismatches because two test texts used invalid complex/class grammar.
Valid texts replaced them; the six remaining kinds were then added. Final: 156 cases, 0 mismatches.
The script internally runs every driver subprocess under timeout 180.
Logs: driver-attacks.log, driver-attacks-final.log, driver-*.out, julia.log.
The reported fixture counts and maximum rational bit sizes were also checked by a timeout-180 Python
standard-library scan of rat.tsv and real.tsv; exit 0, counts as listed above.

## Files written

Sources and scripts: progress.md, oracle.py, attack.c, status_attack.c, place_attack.c, cost.c, faults.py,
driver_attacks.py, repro_identity.c, repro_inexact.c, repro_invariants.c, repro_proof.c, and this report.md.
Own fixtures: rat.tsv, real.tsv, driver-attacks.cmd, driver-attacks.expected.
Evidence: the build, oracle, attack, status, place, cost, driver, Julia, proof, identity, inexact, and
invariant logs named above; fault-results.json; faults.log; fault-F13.log.
Artifacts: build/, faults/, adf, libadelefeld.so, the four attack/measurement executables, the four reproducers,
and the two fault reproducer executables. The read-only scratch symlinks point to the original include/tests.
No owned implementation file, repository test, specification, or reference file was edited.

## Not done

No repository-wide test suite, long fuzz run, or repository mutation-tool sweep was run.
The series numerical oracle samples five points per ball; it does not certify every interior extremum.
The 720 precision-cap skips and the timeout are recorded above. Higher-precision expensive real cases
at the full cap were not completed. NaN inputs were used only for the debug-entry finding.
Infinite inputs were not used for a claimed result because canonical adeles and ideles require finite balls.
LeakSanitizer was unavailable in this environment; the full archive was not sanitizer-instrumented.
No source or test repair was made, since only the review lane is writable.

## Sources pending

- [source pending: explicit FLINT/GMP representation bound implying fmpz_bits(f) <= WORD_MAX on this target].
  G1(c) and its slong-cast argument assert this bound. The local FLINT bit-count documentation does not state it.
- [source pending: a proved bound that the G6 candidate prime fits ulong for every representable numerator].
  The candidate-count proof alone does not give this word-size guarantee; n_nextprime requires it.

## Findings against the specification

No new counterexample to SPEC 9.3.1, 9.3.2, 9.3.3, or N-D8/N-D12/N-D16 was established.
F3 conflicts with conventions 4.4's debug diagnostic contract. F4 is a false step in api-1f8.md.
F1 and F2 are missed non-equivalent faults in the lane's tests. They are not failures of the baseline code.
The known findings listed in this review's brief are not counted again.
