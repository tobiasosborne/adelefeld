# f-slice13 report

2026-10-04. All four slices are callable in C, the driver and Julia.
Seven new exported functions are in catalogue.h/catalogue.c. Haar volume and idele content reuse existing code.
Final release, SAN and clang tests: 9 tests, 131636 checks, 0 failures.
Final INV tests: 10 tests, 131701 checks, 0 failures, including 13 invalid-input child calls.

The single full check-all run failed at the driver fixture exit metadata, after all 79 C programs passed.
The four new fixtures lacked #!exit 1. This was repaired. The complete driver, exports, Julia and both
tool self-tests then passed separately. The full run was not repeated. No library change followed that run.

## Functions and decisions

Proposed N-D19 is recorded in include/adelefeld/catalogue.h, before each implementation slice.
The interface and proofs are appended to docs/api-1f9.md as Y9 to Y15.

1. Files: catalogue.h and catalogue.c group this second catalogue set. Alternatives: four small headers,
   or declarations in existing input headers. No existing input header was changed.
2. adf_fball_binom and adf_fball_binom_tight return status and write an adf_fball output.
   Alternative: one function with a policy. k is ulong; alternatives: fmpz or an unbounded degree.
   Conservative k<=4096; tight k<=256. One above either bound gives LIMIT before the domain check.
   Inputs may have arbitrary bit length. Integral points are admitted. Nonintegral balls disjoint from
   Zhat give DOMAIN; mixed balls give NOT_DETERMINED, including at k=0. Alternative: extend to rationals.
3. adf_ucoset_profpow is strict at canon(N). Alternative: make coarsening the default.
   adf_ucoset_profpow_coarse returns canon(D), D=gcd(N,c^M-1), without factoring N.
   adf_ucoset_profpow_fine returns canon(F) and its CRT centre. Alternative: a policy argument.
   The exponent is an integral adf_fball with arbitrary fmpz centre e and radius M, including M=0.
   Alternatives: signed-word e and word M, or separate fmpz parameters.
   All three use inverses for negative exact exponents and give [1] at exact zero.
   For exact e fitting slong, strict/coarse reuse adf_ucoset_pow. Fine reuses pow_tight at exact e.
   Fine alone caps g=gcd(e,M), or |e| at M=0, at 256. Its limit follows domain and exact constant cases.
   Alternatives: an unbounded factorisation, caller budget or supplied factorisation.
   A proposed 32-bit N limit was removed before implementation: gcd blocks avoid N factorisation.
   The inside block is gcd(N*g_N,c^M-1). The outside block comes from existing tight(U(1),g).
   Repeated gcd division separates the two blocks. Only modular c^M is formed; CRT fixes the centre.
   Exact bases [1],[-1] are also admitted. Unknown parity at [-1] gives NOT_DETERMINED in strict,
   and [1 mod 1] in coarse/fine. Alternative: restrict to finite cosets only.
4. Haar volume already exists as adf_fball_haar_volume(adf_rat_t,const adf_fball_t), fball.h:188.
   It is reused, not duplicated. Alternatives: a new fmpq wrapper or a status/where accessor.
   It returns void, since every canonical finite ball has an exact rational volume. No limit applies.
   adf_idele_content, idele.h:217, is also reused and checked once.
5. adf_idclass_cyclo_exp_u and adf_idclass_cyclo_exp_uinv write an fmpz exponent modulo fmpz n.
   Names are fixed by CV-53/M0-D10. Alternatives: word n, a convention argument or the unsourced names.
   n<=0 gives DOMAIN. Finite precision requires canon(n)|canon(N); otherwise NOT_DETERMINED.
   Exact units always work. The exponent is reduced modulo original n, including its unique odd lift
   when n=2m with m odd. n=1 gives 0. No limits or real precision apply.

All new status functions preserve the value and where on failure, and preserve where on success.
These functions examine the finite part as a whole and select no prime. where may be NULL.
Outputs of the same type may equal the complete input; cyclotomic j may equal n.
No output overlaps an input subobject. Every new public function checks its canonical value inputs on entry
under INV, before its arithmetic. Cyclotomic checks the whole class, including its unused real coordinate.
The existing Haar function also has its debug check.

## Proofs and ground truth

Y9 implements Proposition 14, docs/proofs/catalogue.md:291-312, including exact k=0 and the smallest radius.
The signed binomial recurrence is proved in Y9 by counting subsets and by the negative-top product identity.
Y10 gives the integral-domain congruence proof: gcd(H,d)|A decides whether (A+H Zhat)/d meets Zhat.
Canonicality at d>1 also proves the presence of a nonintegral point. The mixed witness is 0 versus 1/2.

Y11 implements Definition 11 and Proposition 12, catalogue.md:218-241. Y12 proves the two-block gcd/CRT
implementation of Proposition 13:245-287. Y15 gives an elementary proof of its principal-unit depths and
finite-unit exponents, using binomial valuations, permutation of nonzero residues and polynomial root counts.
It avoids relying on the logarithm and finite-group sources marked pending in that proof file.

Y13 proves Haar volume by finite coset indices, without a local scaling citation. Haar translation invariance
is on disk in refs/src/tate-poonen/notes.txt:370-380. Idele content is Proposition 10:202-208; the decomposition
is in refs/src/milne-cft/CFT.txt:9853-9858. The existing content accessor returns 15/14 in the test, independently
of the real coordinate -7 and exact unit [-1].

Y14 implements Proposition 15, catalogue.md:318-343. The external map attribution is
refs/src/milne-cft/CFT.txt:9883-9886: "the global reciprocity map is the reciprocal of" the canonical isomorphism.
The exponent convention is at :3162-3166. The idele/Frobenius vector is at :9904-9908 and :1307.
The requested vector is quoted in catalogue.md:324-326: the idele has "component p at one prime p,
and component 1 elsewhere". Tests take p=3, unit 19 mod 63, giving inverse exponent 3 mod 7 and 1 mod 9.
It is constructed as an idele of content 3 and converted to a class. Direct exponents are 5 mod 7 and 1 mod 9.

FLINT domains were read on disk: refs/src/flint-3.0.1/fmpz.rst:923-929 for modular powers,
:997-1008 for factorial/binomial, :1040-1048 for gcd, :1154-1160 for inverse, :1292-1303 for CRT,
and :51-55 for aliases. Bin_uiui has a word-sized top entry, so the signed arbitrary-top recurrence is used.
CRT is only called with coprime moduli greater than 1 and representatives in the stated ranges.
The reused integer tight power cites ulong_extras.rst:1126-1130, 1203-1216 and 833-840 for factors/primality.

## Oracle and test numbers

proto/catalogue2_checks.py uses only the standard library. It imports neither catalogue_checks.py nor the C.
All four fixture files are consumed completely through tests/support/jsonl.h. Total size: 274864 bytes.

| Slice | Rows | Oracle or extra tests |
|---|---:|---|
| A | 822 | 36423 parameter samples over proved complete periods; signed polynomial values |
| B | 1362 | 7080 distinct image residues counted over complete unit/exponent enumerations |
| C | 120 | exact quotient/index values, plus 1001 independent manual triples |
| D | 2308 | all unit lifts at lcm(N,n), with direct modular inverses |

For binomials, choose a nonzero difference S and use period k!*S/gcd(N,k!*S). Numerator differences
over this shift are divisible by k!*S, so differences modulo S repeat. Their gcd over a period therefore
equals the gcd over all integer parameters. Continuity gives the same enclosing ideal on Zhat.
This oracle does not use the k-sample formula as its value algorithm.

For powers, each unit's order is computed by multiplication and their lcm gives a complete exponent period.
The level 8*N*g*(g+1)! contains F by catalogue.md:282-283. Its prime divisibility also follows from Y15:
inside prime depths divide N*g, outside primes satisfy p<=g+1, and the factor 8 covers the 2-adic depth.
All unit lifts and exponents e+Mt over their period are enumerated; this is not random sampling.

Each vector call checks status, the exact canonical result, unchanged failed outputs, and unchanged where.
Every permitted same-type alias is exercised. A wrong status, centre, radius/modulus, exponent, report or
sentinel fails a case. Release has 131636 checks; INV adds 65 checks for 13 children, including their names.
The tests include local binomial and Haar inputs and all new operation domains, refusals and limits.

The PLAN binomial case gives radius 2; k=0 is exact 1. a=2,N=3,k=2 gives tight radius 9, larger than N.
The power PLAN vector gives centre 49, F=120. M=2 at base residue 2 mod 5 admits exponent points 0 and 2,
with output residues 1 and 4; strict refuses, coarse gives U(1), fine gives U(24).
Cyclotomic N=3,n=9 admits residues 1 and 4, and inverses 1 and 7, proving ambiguity one digit too coarse.
[2 mod 3] at n=6 returns exponent 5. No wholly integral binomial input has a precision refusal boundary.

Large manual inputs include a 4097-bit binomial centre, 10004-bit radius, signed 4097-bit exact exponents,
4097-bit power moduli, a Haar radius with 5001-bit denominator, and cyclotomic orders through 4098 bits.
Both binomial caps and g=256 succeed; one above each cap fails without changing outputs.
Exact signed exponents -6..6 are also compared with existing pow and pow_tight.

## Red and green

redgreen.log and progress.md record the slices in order, each completed before the next implementation.

| Stage | Red | Green |
|---|---|---|
| A | absent symbols: make exit 2 | 3 tests, 25527 checks, 0 failures |
| B | NOT_DETERMINED stubs: 4654 failed checks | 5 tests, 74478 checks, 0 failures |
| C | command absent: 6 mismatched driver lines | 6 tests, 76487 checks; driver diff 0 |
| D | NOT_DETERMINED stubs: 1988 failed checks | 9 tests, 131636 checks, 0 failures |

Slice A used one initial file link-error red covering both declarations. A separate preimplementation
assertion red for its tight function was not recorded; this is a process omission under COMMON-C rule 1.
For C the existing library already passed: no new library implementation or library assertion red is claimed.
Its preliminary attempted declaration conflicted with fball.h:188; the duplicate was removed.
The initial A driver expectations omitted the finite-ball wrapper, giving 7 differing lines. They were
corrected by hand from text.h:162-164. The first Julia helper had the wrong get_str signature, fixed before
its first run. B's Julia block placement was also fixed before running it.

## Four planted faults per slice

Scratch sources are under lanes/f-slice13/build. A,B,D alter catalogue.c; C alters a scratch fball.c only.
All 16 faults compiled. Every fault was detected. C's denominator fault also changes point denominator state.

| Fault | Failed checks | Failed tests | Exit |
|---|---:|---:|---:|
| A radius gcd -> lcm | 525 | 2 | 1 |
| A exact radius 0 -> 1 | 472 | 2 | 1 |
| A omit last tight sample | 374 | 2 | 1 |
| A overwrite output before reading input | 664 | 1 | 1 |
| B accept every target modulus | 871 | 2 | 1 |
| B omit CRT centre | 9 | 2 | 1 |
| B reverse inverse sign | 74 | 2 | 1 |
| B discard outside block | 80 | 1 | 1 |
| C point volume 1 | 77 | 1 | 1 |
| C reciprocal reversed | 849 | 1 | 1 |
| C ignore radius denominator | 734 | 1 | 1 |
| C centre as volume numerator | 836 | 1 | 1 |
| D reverse divisibility | assertion failure, then SIGABRT | no final count | -6 |
| D omit target canonicalisation | 614 | 2 | 1 |
| D u returns inverse | 81 | 2 | 1 |
| D inverse returns u | 81 | 2 | 1 |

A faults used binomial_vectors and binomial_plan_limits_huge. B faults used power_vectors and
power_plan_boundary_huge_statuses. C faults used haar_and_existing_content. D faults used cyclotomic_vectors
and cyclotomic_spec_boundary_huge. Complete first failing assertions and counts are in faults-A to D.log.

## Bounded mutation run and survivors

One sweep of src/catalogue.c: limit 60, seed 1, jobs 2, SAN and INV. All 60 compiled.
Tool result: 329.2 seconds, 47 killed, 13 survived, 0 not compiled, 0 tool timeouts, 0 excused; exit 1.
The staged make target builds and runs only test_catalogue over a matching SAN/INV archive.
LeakSanitizer is disabled with ASAN_OPTIONS=detect_leaks=0, as required in this sandbox.

Five survivors exposed a debug-test gap: a later helper's abort was accepted as the public entry check.
The tests now capture the public diagnostic name; binomial invalid inputs also test entry before LIMIT.
Only the same 13 survivors were replayed: all compiled, 5 killed, 8 survived. No second sweep was run.
Final effective count over the same 60 distinct mutants is 52 killed and 8 equivalent survivors.
Indices below are the seed-selected kept directory numbers. Each line gives the final disposition.

01 mul(z,z,t)->mul(z,t,z): survives; commutative multiplication with supported alias.
03 omit binom INV_FBALL: killed; must abort before LIMIT and name the public function.
16 mul(t,A,B)->mul(t,B,A): survives; commutative multiplication.
22 inverse test e<0->e<=0: survives; at zero exponent both bases give 1.
24 start tight samples at 0: survives; added difference 0 does not change the gcd.
26 omit zero(R): survives; R is freshly initialised to zero and unwritten.
36 omit residue zero at modulus 1: survives; discarded by gcd(1,*), mod 1 or explicit 1.
38 omit coarse exponent INV_FBALL: killed; the fallback diagnostic names the wrong function.
43 gcd(R,R,b)->gcd(R,b,R): survives; commutative gcd with supported alias.
48 omit fine base INV_UC: killed; the fallback diagnostic names the wrong function.
49 mul(A,A,h)->mul(A,h,A): survives; commutative multiplication with supported alias.
51 omit strict base INV_UC: killed; the fallback diagnostic names the wrong function.
58 omit coarse base INV_UC: killed; the fallback diagnostic names the wrong function.

Replay failures: index 03 has 2 failed checks; indices 38,48,51,58 each have 1. Each make exits 2.
Every arithmetic survivor has 10 tests, 131701 checks, 0 failures and make exit 0.
The tool was not changed, and no read-only equivalent.txt entry was written.

## Commands and results

All commands run from the repository root unless a scratch cwd is stated. Every test/script has a timeout.
B below means lanes/f-slice13/build. Red/green commands and their logs are the named stages above.

1. timeout 180 python3 -B proto/catalogue2_checks.py A, B, C, D (four invocations): exits 0.
   Rows respectively 822,1362,120,2308; A 36423 period samples, B 7080 image residues.
2. timeout 180 make -s -j2 BUILD=B B/test_catalogue, then timeout 120 B/test_catalogue.
   A initial build exits 2 for absent functions. B and D stub tests exit 1 with the counts above.
   All implemented stage builds/tests exit 0. The preliminary C conflicting-header build exits 2.
   Final release is also run inside the broad check: 9 tests/131636 checks, 0 failures.
3. Driver build after each slice, exit 0:

       timeout 180 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude tools/adf/adf.c \
           B/libadelefeld.a -lflint -lgmp -lm -o B/adf

   timeout 60 B/adf tests/driver/catalogue-{binomial,power,volume,cyclotomic}.cmd,
   four invocations: each exits 1 intentionally. diff -u EXPECTED lanes/f-slice13/driver-{A,B,C,D}.out
   exits 0 for the final hand expectations, with 17,24,8,22 output lines. A's first diff exits 1 (7 lines).
   C's command-before-implementation run exits 1 and its diff exits 1 (6 differing lines).
4. Shared build after each completed slice, exit 0:

       timeout 180 cc -shared -fPIC -Iinclude -std=c11 -O1 -g src/*.c \
           -o B/libadelefeld.so -lflint -lgmp -lm
       LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libgmp.so.10 JULIA_NUM_THREADS=1 \
           timeout 60 julia --startup-file=no tests/julia/catalogue.jl B/libadelefeld.so

   Julia exits 0; cumulative assertions 8,18,23,30. Logs julia-A to D.log.
5. timeout 180 python3 -B lanes/f-slice13/faults.py A, B, C, D: four script exits 0.
   Each compiles its four scratch sources with timeout 60 cc, then links with timeout 60 cc using
   tests/test_catalogue.c, B/support/jsonl.o, B/libadelefeld.a, -lflint -lgmp -lm.
   Every compile/link exits 0. Every timeout-60 test exits as in the fault table.
6. timeout 180 python3 -B tools/memcheck/selftest.py: exit 0, 115 tree files, 0 static findings.
   Five valgrind defect snippets give their expected exit 77. Log memcheck-selftest.log.
7. timeout 180 make -s -j2 SAN=1 INV=1 BUILD=lanes/f-slice13/build-mutbase \
       lanes/f-slice13/build-mutbase/test_catalogue
   ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/f-slice13/build-mutbase/test_catalogue
   Both exit 0; initial debug tests 10/131675, 0 failures. This archive supplies the mutation base.
8. timeout 10 python3 -B lanes/f-slice13/prepare_mutation.py: exit 0, stages the matching archive/test.
   A later line-wrapping edit failed timeout-10 py_compile once with SyntaxError (exit 1), then exits 0
   after correction. It did not change the tested product or the already staged mutation root.
9. Exact mutation command, exit 1 with the recorded survivor counts:

       ASAN_OPTIONS=detect_leaks=0 timeout 1300 python3 -B tools/mutate/mutate.py \
           --root lanes/f-slice13/build/mutroot --scratch lanes/f-slice13/build/mutate \
           --files src/catalogue.c --limit 60 --seed 1 --jobs 2 --san --timeout 90 \
           --make 'make -s -j2 check-catalogue INV=1' --copy Makefile include src tests lanes --keep

   The custom target uses timeout-60 SAN/INV cc for catalogue.c, then test_catalogue.c and jsonl.c,
   links the matching archive, and runs ASAN_OPTIONS=detect_leaks=0 timeout 60 ./test_catalogue.
10. timeout 180 python3 -B lanes/f-slice13/replay_survivors.py: exit 0.
    Each kept survivor runs timeout 90 make -s -j2 check-catalogue INV=1 with the strengthened test.
    Replay compiles 13, kills 5, leaves the 8 arithmetic survivors above. Logs survivor-*.log.
11. Final SAN build/run: exits 0, 9/131636 checks, 0 failures:

        timeout 180 make -s -j1 SAN=1 BUILD=lanes/f-slice13/build-san \
            lanes/f-slice13/build-san/test_catalogue
        ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/f-slice13/build-san/test_catalogue

12. Final INV and clang variants: each build/run exits 0. Builds run together with one job each,
    at most two compiler cores. INV is 10/131701 checks; clang is 9/131636; both 0 failures.

        timeout 180 make -s -j1 INV=1 BUILD=lanes/f-slice13/build-inv \
            lanes/f-slice13/build-inv/test_catalogue
        timeout 120 lanes/f-slice13/build-inv/test_catalogue
        timeout 180 make -s -j1 CC=clang BUILD=lanes/f-slice13/build-clang \
            lanes/f-slice13/build-clang/test_catalogue
        timeout 120 lanes/f-slice13/build-clang/test_catalogue

13. timeout 900 make -j2 check-all: exit 2, all 79 C programs pass; driver expects exit 0 for the
    new binomial fixture and receives its intentional exit 1. Fixed #!exit metadata, no value changes.
    The full run was not repeated. check-all.log preserves the failed run.
14. After repair, timeout 180 sh tests/test_driver.sh: exit 0, 61 cases, 101124 expected lines, 0 differences.
    timeout 180 sh tests/test_exports.sh: exit 0, 462/462 exported, 0 missing/undeclared/variadic functions.
    JULIA_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh: exit 0, catalogue 30 assertions, 0 failures.
    Julia uses the existing GMP-preload fallback. Logs final-driver, final-exports and final-julia.log.
15. timeout 180 python3 -B tools/mutate/selftest.py: exit 0. Final strong selftest run reports 54 mutants:
    43 killed, 0 survived, 7 not compiled, 1 timed out, 3 excused. These are tool-selftest counts, not catalogue.
    timeout 180 python3 -B tools/memcheck/selftest.py: exit 0, tree 115 files, 0 static findings;
    all five valgrind defect snippets have expected exit 77. Logs final-*-selftest.log.
16. timeout 60 cc -std=c11 -Wall -Wextra -Werror -Iinclude -fsyntax-only lanes/f-slice13/header.c:
    exit 0. timeout 60 c++ -std=c++17 -Wall -Wextra -Werror -Iinclude -fsyntax-only
    lanes/f-slice13/header.c: exit 0. Header stands alone in both languages.
17. timeout-10 Python newline/line-length/fixture audits: exit 0. Final audit covers 20 new/owned source,
    lane and fixture-script files, plus both new documentation sections: 0 missing newlines, 0 new long lines.
    Fixture data total 274864 bytes. The preexisting brief's long title was not changed.

## Findings against the specification, proofs or brief

1. No mathematical counterexample to SPEC 9.3.7 was found. The specified PLAN vectors pass.
2. The brief calls Slice C "one function" (lanes/f-slice13/brief.md:31), while fball.h:185-188 already says
   "adf_fball_haar_volume(vol, x): vol = 1/N for N > 0, and 0 for an exact value".
   Existing implementation src/fball.c:618-625 has the formula and debug check. The slice therefore reuses it.
3. Catalogue.md:320 says "The arithmetic convention sends" and "the geometric convention sends".
   Conventions.md:972 instead says "both conventions are offered, as two functions named by their formula";
   SPEC 9.3.7 follows this newer decision. The code uses exponent names and preserves both formulas.
4. SPEC:707 permits "NOT_DETERMINED, or the coarser coset"; catalogue.md:231-232 distinguishes strict and
   enclosure interfaces but does not select a default. N-D19 proposes strict plus two separate enclosures.
5. SPEC and the proofs give no k or g resource cap or nonintegral-input interface. The caps and the Y10
   domain rule are proposed interface decisions, with alternatives in the header. No mathematical statement
   or expected result was weakened to accommodate a test.
6. The general requested NOT_DETERMINED precision-boundary test does not apply to integral binomial inputs
   or Haar volume: the former always has both enclosures and the latter always has an exact value.
   Mixed-domain witnesses and resource-limit sentinels cover the actual refusals instead.

## Files written

New product files: include/adelefeld/catalogue.h; src/catalogue.c; tests/test_catalogue.c;
tests/julia/catalogue.jl; proto/catalogue2_checks.py;
tests/ref/vectors/f-slice13/{binomial,power,volume,cyclotomic}.jsonl;
tests/driver/catalogue-{binomial,power,volume,cyclotomic}.{cmd,out}.
Owned existing edits: the new end section of docs/api-1f9.md; tools/adf/adf.c; tools/adf/README.md.
Only one include was added to include/adelefeld.h and two execution lines to tests/test_julia.sh.

Lane files: progress.md, redgreen.log, faults.py, prepare_mutation.py, replay_survivors.py,
survivor-notes.md, header.c, this report, and all named build, oracle, red/green, driver, Julia, fault,
mutation, replay and final-check logs. Build directories contain compiled artifacts, scratch faults,
the mutation staging root and kept mutants. Final checks also use build/ as explicitly authorised.
No state-changing git command, tracker command, package installation or subagent was used.

## What is not done and sources pending

No local zeta factors, complex-ball pole logic, set-valued cyclotomic result, unbounded tight degree,
unbounded finest g, performance benchmark or long differential fuzz campaign was built or run.
The oracle enumerations are complete at their stated finite levels; they are not an hour-long fuzz run.
No second full-suite run was made after fixing fixture exit metadata. Slice A's separate tight assertion-red
step was not recorded, as stated above. LeakSanitizer was not run; ASan and UBSan were run.
Avoidable work includes repeated canonicalisation of unit results and recomputing k falling factors at each
tight-binomial sample. The simple implementation was retained.

[source pending: an on-disk source attributing the arithmetic/geometric labels to these two maps.]
This is the unused naming attribution in catalogue.md:342-343; the interface uses CV-53 exponent names.
[source pending: the external principal-unit logarithm theorem cited in catalogue.md:272-273.]
[source pending: the external finite-unit-group exponent reference cited in catalogue.md:276-278.]
Those read-only proof-file citations remain pending. Y15 proves all depth and exponent claims needed here
without them. Y13 supplies its own index proof of volume, so the local scaling source pending at :209 is
not needed by this slice. The implemented interfaces and their stated formulas have no unresolved proof step.
