# f-slice8 report

Completed local roots for milestone 1F.5, with named branches, exact image radii, named-prime wrappers,
driver commands, exact integer oracles, Julia examples and five deliberate fault checks.
No state-changing git command, tracker command, benchmark, mutation run, clang suite or INV suite was run.
Builds used at most two jobs. Test programs and scripts ran under timeout. The final check-all ran once.

## Functions built

- adf_lball_root_count: certify existence on the whole input and return the branch count.
- adf_lball_root_seed: select a branch by its unit residue or sign identifier.
- adf_lball_sqrt_seed: the same operation at degree 2.
- adf_lball_roots: all branches in increasing identifier order, in caller-owned arrays.
- adf_sball_root_seed_at and adf_sball_sqrt_seed_at: one branch at a named prime.
- adf_sball_roots_at: every local branch at a named prime.
- Driver roots_at and root_at: list every branch or select one by seed.

The local implementation uses exp(Log(unit centre)/n) and the selected Teichmueller factor.
It does not use a root iteration with a nonunit derivative. All failure paths are transactional.
Existing seedless adf_sball_sqrt_at/adf_sball_root_at remain UNSUPPORTED at primes.

## Decisions and alternatives

These decisions are documented with proofs in docs/api-1f5.md for the orchestrator's SPEC 15.4 record.

1. New lroot.h/lroot.c instead of additions to lfunc.h/lfunc.c.
2. At odd primes identifiers are nonzero unit residues modulo p. At 2, identifiers 1 and 3 denote
   sign +1 and -1, as in decomposition. Alternative: a signed identifier type. The unsigned type
   also handles primes above WORD_MAX. Zero and degree 1 use identifier 0; degree 1 ignores its seed.
3. Caller-owned arrays plus a separate count function instead of a new allocated list type.
   Insufficient capacity gives LIMIT and changes no output. Input may alias any local output slot.
4. Log(unit) instead of explicitly lifting and removing the input torsion factor before log.
   Proposition 11 proves these logarithms equal. The root's torsion factor is still explicitly selected.
   Newton iteration was not needed.
5. Enumerate roots of a degree gcd(n,p-1) finite-field polynomial instead of degree n, a discrete
   logarithm, or a search through every residue. The proof of the degree reduction is R5.
6. Listing has coefficient-count bound ADF_LROOT_BRANCH_MAX=2^26/64-1. Alternative: an unbounded
   coefficient array. Count and seed evaluation do not have this enumeration cost or limit.
7. Ball inputs return the exact image exponent E, independent of requested N. Exact inputs use N,
   unless the selected root is rational. Alternative: min(N,E) for ball inputs as in lfunc.h.
   The choice preserves the brief's exact-image contract even when a caller requests coarser output.
8. Rational exact branches are detected by numerator and denominator integer roots of the unit part,
   assigning each rational candidate to its identifier. Other branches of the same input may be irrational.
   Alternative: always return balls. Exact rational branches ignore N and preserve the separate valuation.
9. Degree 1 is the identity, including balls containing zero. Degrees >=2 keep the strong guard.
   Remark 15r is not implemented; its coarse odd-degree case at 2 needs a separate sign-changing convention.
10. New seeded _at functions instead of changing existing signatures or silently choosing a branch.
    These new wrappers are prime-only; real-place requests are UNSUPPORTED after membership is checked.
11. Outside the guard return NOT_DETERMINED, even if another obstruction could prove no root.
    Invalid degree or seed is DOMAIN as an invalid argument, not a claim about other valid branches.
12. Driver syntax is roots_at X with PRIME with DEGREE, or root_at X with PRIME with DEGREE with SEED.
    At 2, -1 is accepted as identifier 3. Output shows the sign or residue on every branch.
    The parser's spare operand slot was extended to reject a fifth operand. A regression also fixed
    an inherited acceptance of a trailing separator with no operand.

Avoidable cost: all-branch evaluation repeats the principal-unit logarithm and exponential for each branch.
It also copies temporary results at commit. No optimisation or timing claim is made.

## What is proved

R1: the finite existence criterion is constant on a guarded ball; DOMAIN excludes every input point.
R2: each result is the exact branch image, with E=M-v_p(n)-(n-1)v_p(b), including negative valuations.
R3: rational-branch detection is complete and does not overflow on unsigned word-size degrees.
R4: log precision L+v_p(n), exact division, exp and torsion multiplication give the requested root precision.
R5: a degree gcd(n,p-1) finite-field polynomial lists exactly the torsion branches.
R6: exponent arithmetic, limits, transactions, aliasing and projection to one prime satisfy the headers.
R7: the finite-ring oracle's comparison precision distinguishes exact image exponents from safe enlargements.

These use the existing stepwise proofs in docs/proofs/functions.md:55,92,265,336,410,463,538,725.
They are new lane proofs, not changes to the specification. No unproved root iteration was introduced.

## Test scope and failure conditions

The oracle imports only Python's standard library. It uses exhaustive modular integer powers.
It does not import the C code, a p-adic library, log/exp or finite-field factorisation.

- Ball fixtures: 8110 rows; p=2,3,5,7; n=1..12; every integral input ball at M=1..5 for p=2,
  and M=1..3 for odd p. All 45708 b modulo p^(M+1) are bucketed by b^n modulo p^M.
  Additional rows scale unit balls to negative and positive root valuations.
  A set difference, wrong branch count or missing distance-p^E witness fails generation.
  There are 5062 distance witnesses. H=M+1>E resolves the digit after the claimed result radius.
- Exact fixtures: 1408 rational inputs. Output precision is N=4; root search is modulo p^(4+v_p(n)).
  The oracle checks a unique residue per identifier modulo p^4. Rational roots are detected by small
  integer numerator and denominator powers. A wrong residue, rationality flag or status fails C comparison.
- Both fixtures are fully consumed in test_lroot. Every ball row is also consumed through the _at wrappers.
  Two fixture files total 643421 bytes, below 1 MB.
- 2400 reproducible seeded comparisons use small balls, degrees <=12 and relative exponents <=7.
  They check a prescribed root, a second point exactly p^E away, the exponent, the count and the powered image.
  This is finite sampling, not fuzzing or a claim about all rational inputs.
- Directed tests include 3 at 2, 9 at 2, 2 at 7 with degree 3, both guard failures from Proposition 15,
  negative valuations, degree 1, exact zero, uncertain zero, rational denominators and 4096-bit operands.
- Precision tests include exact irrational inputs, coarse requested N=-2 and 1, exact N=4 and 8,
  and p=2^64-59 at N=200. Large-degree tests include WORD_MAX, UWORD_MAX, p and 2^62 at p=2.
- Transaction checks include all local output/input aliases, every all-root array slot including unused
  slots, seeded partial-ball aliasing, insufficient capacity, invalid seeds, exponent/working limits,
  and a late LIMIT after an earlier branch has already computed exactly.
- The driver has 10 successful-root output lines and 15 status output lines. Goldens are handwritten
  from exact powers and Propositions 13/15. Any byte or process-status difference fails the comparison.
- Julia computes both square roots of 9 at 5 to requested precision 20, prints their identifiers,
  and checks DOMAIN at sqrt(3) in Q_2 and cube-root(2) in Q_7, plus NOT_DETERMINED on 1+4 Z_2.

Final normal and sanitizer totals are identical:

| Program | Tests | Checks | Failed checks | Failed tests |
|---|---:|---:|---:|---:|
| test_lroot | 6 | 380751 | 0 | 0 |
| test_rfunc_prime | 14 | 521939 | 0 | 0 |
| Julia lroot.jl | 1 test set | 8 | 0 | 0 |

## Red-green and command ledger

Commands ran from the repository root. Build/test logs are in this lane.
Repeated commands below refer to distinct incremental stages; no clean rebuild was requested.
The ledger records mistakes as well as successful runs.

Local build command:

    timeout 60 make -j2 BUILD=lanes/f-slice8/build lanes/f-slice8/build/test_lroot

- red-first.log: exit 2, wrong JSON helper names in the test. No implementation result.
- red-link.log: exit 2 after correcting those names; undefined root symbols, the intended first red test.
- red-stub-build.log: exit 0. All four functions were stubs before their assertions were exercised.
- green-build.log: first invocation failed with exit 2 for missing stdlib.h; the subsequent test ran
  the stale stub executable. green-first.log is that failed stub run, not a successful implementation.
  After adding the include, the same build command exited 0 and green-second.log passed.
- extended-build.log, exact-build.log: exit 0 at each extension.

Local test command:

    timeout 60 lanes/f-slice8/build/test_lroot

| Log | Tests | Checks | Failed checks | Exit |
|---|---:|---:|---:|---:|
| red-stubs.log | 3 | 204024 | 25948 | 1 |
| green-first.log, stale binary | 3 | 204024 | 25948 | 1 |
| green-second.log | 3 | 276136 | 0 | 0 |
| extended-tests.log | 5 | 295394 | 0 | 0 |
| full-grid.log | 5 | 339966 | 0 | 0 |
| exact-tests.log | 6 | 380751 | 0 | 0 |

Wrapper build and test commands:

    timeout 60 make -j2 BUILD=lanes/f-slice8/build lanes/f-slice8/build/test_rfunc_prime
    timeout 60 lanes/f-slice8/build/test_rfunc_prime

- red-at-build.log and green-at-build.log: build exit 0.
- red-at.log: 13 tests, 279181 checks, 53 failed checks, 1 failed test, exit 1.
- green-at.log: 13 tests, 279181 checks, 0 failed checks, 0 failed tests, exit 0.
- reference-at.log: 14 tests, 521939 checks, 0 failed checks, 0 failed tests, exit 0.

The combined incremental builds, reference-build.log and pre-fault-build.log, exited 0:

    timeout 60 make -j2 BUILD=lanes/f-slice8/build lanes/f-slice8/build/test_lroot \
      lanes/f-slice8/build/test_rfunc_prime

Oracle generation, always exit 0:

    timeout 60 python3 -B proto/lroot_checks.py

- oracle.log: initial unit grid, 6694 rows, 45708 residues, 5046 witnesses, 443052 bytes.
- oracle-full.log: every integral ball, 8110 rows, 45708 residues, 5062 witnesses, 530722 bytes.
- oracle-final.log: the same ball rows plus 1408 exact rows, 112699 more bytes.

Driver compilation, exit 0 each time (red stage, driver-build.log and trailing-build.log):

    timeout 60 cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude tools/adf/adf.c \
      lanes/f-slice8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-slice8/build/adf

Driver runs and comparisons:

    timeout 60 lanes/f-slice8/build/adf tests/driver/root-values.cmd
    timeout 60 lanes/f-slice8/build/adf tests/driver/root-status.cmd
    diff -u tests/driver/root-values.out lanes/f-slice8/driver-values.out
    diff -u tests/driver/root-status.out lanes/f-slice8/green-trailing.out

- Before implementation, root-values exited 1 and red-driver.diff differed (diff exit 1).
  Its initial mathematical Zhat input notation was corrected to existing driver value syntax before coding.
  No expected numeric output was changed.
- Implemented root-values: exit 0, 10 matching lines, diff exit 0.
- Initial root-status: expected exit 1, 14 matching lines, diff exit 0 against driver-status.out.
- Added trailing-separator regression: red-trailing.diff exit 1; expected PARSE, got the selected root.
- After parser repair: expected driver exit 1, 15 matching lines, diff exit 0 in green-trailing.out.
  This repair was made during the C-test phase of check-all, before its driver phase compiled the driver.

Formatting checks used timeout 10 awk on new files and timeout 10 python3 on added diff lines.
Results: 0 lines over 116 columns. The final audit also checked 2 fixtures totalling 643421 bytes
and the 4 final exit records, all 0. Read-only git diff/status confirmed source paths were within ownership.
After final numerical runs, only the oracle's scaling comment and README separator prose were clarified.

## Five deliberate faults

Command:

    timeout 60 python3 -B lanes/f-slice8/faults.py

Exit 0. The script builds each scratch copy with timeout 60 cc and runs it with timeout 60.
The source copies, binaries and their build/test logs are under lanes/f-slice8/build/faults/.
It links the faulty lroot translation unit with test_lroot and the normal archive.
No mutation tool, fuzz target or benchmark was run.

| Fault | Failed checks | Failed tests | Test exit |
|---|---:|---:|---:|
| Guard lower by one | 698 | 2 | 1 |
| Omit (n-1)v_p(b) in result exponent | 7799 | 2 | 1 |
| Drop a branch when count >1 | 1808 | 4 | 1 |
| Square criterion at 2 uses modulo 4 | 16 | 1 | 1 |
| Omit division of Log by n | 23146 | 6 | 1 |

Last line: 5 faults, 5 caught by assertions, 0 timeouts.
The missing-branch test executed 356333 checks; each other fault executed 380751.

## Final checks

Exactly one full run, exit 0, check-all.log:

    timeout 900 make -j2 check-all

Last line:

    check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest

The driver checked 47 cases and 100867 expected lines, with 0 differences.
Exports: 420 declared and exported; 0 declared but missing; 0 undeclared exports.
Julia roots: 8/8 assertions. The existing Julia script used its system-GMP preload retry:

    test_julia: passed (with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10)

Exactly one requested sanitizer build, exit 0, san-build.log:

    ASAN_OPTIONS=detect_leaks=0 timeout 120 make -j2 BUILD=build/san SAN=1 \
      build/san/test_lroot build/san/test_rfunc_prime

Final build line, wrapped:

    cc -Iinclude -Itests -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
      -fsanitize=address,undefined -fno-omit-frame-pointer -MMD -MP -fsanitize=address,undefined \
      tests/test_rfunc_prime.c build/san/support/golden.o build/san/support/jsonl.o \
      build/san/libadelefeld.a -lflint -lgmp -lm -o build/san/test_rfunc_prime

Both binaries ran, each exit 0:

    ASAN_OPTIONS=detect_leaks=0 timeout 60 build/san/test_lroot
    ASAN_OPTIONS=detect_leaks=0 timeout 60 build/san/test_rfunc_prime

Their last lines, respectively:

    6 tests, 380751 checks, 0 failed checks, 0 failed tests
    14 tests, 521939 checks, 0 failed checks, 0 failed tests

Address/undefined-behaviour diagnostics: 0. LeakSanitizer was disabled as requested.
The full suite's mutation and memory tool self-tests are included in check-all; no lane mutation run was made.

## Files written

New public/source files: include/adelefeld/lroot.h; src/lroot.c.
Changed public/source files: include/adelefeld.h; include/adelefeld/rfunc.h; src/rfunc.c.
Tests: tests/test_lroot.c; tests/test_rfunc_prime.c; tests/julia/lroot.jl; tests/test_julia.sh.
Oracle: proto/lroot_checks.py; tests/ref/vectors/f-slice8/balls.jsonl and exact.jsonl.
Proofs: docs/api-1f5.md.
Driver: tools/adf/adf.c; tools/adf/README.md; tests/driver/root-values.cmd and .out;
tests/driver/root-status.cmd and .out.

Lane records and harness: progress.md; redgreen.log; faults.py; this report.md, written once at completion.
Evidence logs: red-first.log, red-link.log, red-stub-build.log, red-stubs.log, green-build.log,
green-first.log, green-second.log, red-at-build.log, red-at.log, green-at-build.log, green-at.log,
extended-build.log, extended-tests.log, reference-build.log, reference-at.log, full-grid.log,
exact-build.log, exact-tests.log, pre-fault-build.log, faults.log, oracle.log, oracle-full.log,
oracle-final.log, driver-build.log, red-driver.out, red-driver.diff, driver-values.out, driver-status.out,
red-trailing.out, red-trailing.diff, trailing-build.log, green-trailing.out, final-audit.log,
check-all.log and .exit, san-build.log and .exit, san-lroot.log and .exit, san-rfunc-prime.log and .exit.
Build products are in the lane build directory and the explicitly requested final build directories.
Pre-existing brief.md, lane.log and stdout.log were not edited by this task's file-writing commands.

## Not done and next slice

- Optional Remark 15r; the strong guard is retained except the degree-1 identity.
- Rational powers, principal-unit variable-exponent powers and all-places roots.
- A proof that every possible request finishes within a particular time; the headers make no such promise.
- A long fuzz campaign, benchmark, default LeakSanitizer run, clang, INV or check_headers.sh.
  The orchestrator owns the latter checks. Finite enumeration and sampling do not exhaust Q_p.

Proposed next slice: 1F.6 rational powers through the explicit root branches, then principal-unit powers
through exp(s Log(u)), with independent base/exponent uncertainty and the correct precision guard.

## Sources pending

No new external mathematical assertion is cited from memory. FLINT contracts were read on disk:
refs/src/flint-3.0.1/fmpz.rst:983-988 and nmod_poly.rst:2392-2398; arithmetic references are in lroot.c.
Existing naming references remain pending in docs/proofs/functions.md; their definitions are proved there:

[source pending: the name and convention for the Teichmueller representative, functions.md:100]
[source pending: the name and normalisation of the Iwasawa logarithm, functions.md:343]

## Findings against the specification

None. The exact branch-image formula, strong guard and counts passed the stated finite checks.
The trailing-separator finding concerned driver syntax; it was repaired and its regression passed.
The precision policy for ball inputs is a recorded interface decision, not a change to SPEC.md.
