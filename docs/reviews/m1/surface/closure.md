# Closure of the milestone 1 surface review

13 CLOSED, 0 CLOSED WITH EDIT, 0 OPEN, 2 SETTLED BY DECISION. NO BLOCKER OPEN.

The commands below ran in `docs/reviews/m1/surface/build-closure/tree`, a copy of the source,
headers, tests, tools, references, and original reproducers. The copy was built with
`make clean && make -j2`. The original reproducers were not edited.

| Finding | Severity | Verdict | Evidence |
|---|---|---|---|
| R1 | MAJOR | CLOSED | s1 and s4 now give one use finding each; a reversed R1 check gives zero for s1. |
| R2 | MAJOR | CLOSED | Mini tree: 12 mutants, both modes give 9 killed, 1 not compiled, 2 timed out. |
| R3 | MAJOR | CLOSED | 108 unique text keys survive a line shift; two old survivors are excused. |
| R4 | MINOR | CLOSED | README names the three limits; s2, s3, s5 still give zero static findings and valgrind 77. |
| R5 | MINOR | CLOSED | Test names ending in `error` count as kills; only pointer addition fails to build. |
| R6 | MINOR | CLOSED | SIGTERM run: exit 143, zero scratch files, zero live mutant programs. |
| R7 | MINOR | SETTLED BY DECISION | M1-D1/D6 use FLINT's stored exponent; 2^99999 prints, 2^100000 is LIMIT. |
| R8 | MINOR | SETTLED BY DECISION | M1-D1 caps `prec` at 100000; 100001 is LIMIT. Its reason is false; see below. |
| R9 | MINOR | CLOSED | CRLF and trailing blank settings pass; two neighbouring `digits` cases pass. |
| R10 | MINOR | CLOSED | The eight old commands give five UNSUPPORTED and three DOMAIN; two order probes pass. |
| R11 | MINOR | CLOSED | A printable `2^51000` operand has a product that returns LIMIT; driver suite passes. |
| R12 | MINOR | CLOSED | Root run builds the missing shared object and makes 29 checks; wrong cwd fails 2. |
| R13 | MINOR | CLOSED | Each injected `_adf_undeclared` and `adf_undeclared` export makes the script exit 1. |
| R14 | MINOR | CLOSED | The comment now names 2^64 - 1 and 2^64 - 2; the array has those values. |
| R15 | MINOR | CLOSED | Two rounding dependent lines are documented; 27 driver cases pass. |

## R7: settled by M1-D1 and M1-D6

The old `hostile.py` still reports three mismatches because it expects `2^100000` to print.
M1-D6 now names the stored FLINT exponent: `x = m 2^e`, `0.5 <= |m| < 1`.
The definition is in `refs/src/flint-3.0.1/arf.rst:14-20`.
Thus `2^99999` has exponent 100000, and `2^100000` has exponent 100001.
The old expectation no longer states the accepted requirement.

`docs/SPEC.md:860,865`, `include/adelefeld/text.h:32-38,64-66`,
`tools/adf/README.md:184-202`, and `tools/adf/adf.c:240-276` now agree on that rule.
The driver has no second numeric print bound. The new 11-case probe covers midpoint, radius,
imaginary part, the two adjacent powers, a result crossing the bound, settings, and status order.
It reports 11 cases, zero failures. `tests/driver/11_guard.cmd` also passed in the 27-case driver suite.

## R8: settled by M1-D1, with a false reason in the specification

The driver now refuses `prec 100001` with LIMIT, as M1-D1 requires. The old `guard_example.cmd`
therefore writes LIMIT for that setting before it computes the next two commands.
The cap is implemented by reading `ADF_PRINT_EXP_MAX`; the old range of 1000000 is gone.

The parenthetical reason in `docs/SPEC.md:860` says a result rounded at `prec` p has a
radius of binary exponent -p and therefore cannot print for p > 100000. This is false.
`closure-checks/prec_reason_closure.c` computes `2^50000 / 3` at p = 100001 by the public
library interface. It returns status 0, radius exponent -50001, and a non-NULL printed
string of length 49. The radius exponent is within the print bound. Step by step:

1. The exact input is `2^50000` and the divisor is the exact rational 3.
2. The quotient is in `(2^49998, 2^49999)`, so its stored exponent is 49999 by the
   definition in `refs/src/flint-3.0.1/arf.rst:14-20`.
3. A precision of 100001 bits measures rounding at the quotient's scale. It does not
   force the radius exponent to equal -100001. The observed enclosing radius has exponent
   -50001 and the printer accepts it.

The choice to cap the driver's setting is accepted; the stated explanation needs correction.
This is a finding against the specification, not a new code finding.

## Repair tests and independent checks

The repair tests in `tools/memcheck/selftest.py`, `tools/mutate/selftest.py`,
`tests/test_driver.sh`, `tests/test_dlopen.c`, and `tests/test_exports.sh` passed.
The expected statuses and boundary values of R7 to R13 follow M1-D1, M1-D6, the headers,
and the driver README. R14 is a comment correction. R15 deliberately pins FLINT 3.0.1's
observed rounding; the test now says that other enclosing results also meet conventions 9.5.

The old review and repair red logs show that the new R1, R2, R5, R6, R9 to R13 checks
would have failed on the old code. I independently reversed the R1 condition in the
scratch copy: the s1 finding changed from one to zero. I did not reverse each other repair.

`make -j2 check` passed all 42 test programs. `sh tests/test_driver.sh` passed 27 cases
and 100376 expected lines. The original driver replay stops at the added script 11;
the closure replay of scripts 01 to 08 and 10 gives 299 lines, 221 modelled agreements,
zero differences, and 78 unmodelled. A 2000-command independent differential run gives
zero failures. The place reproducer tested 156016 numbers and found zero disagreements.

M1-D10 and M1-D11 remain proposals. I found no contrary input in the scenarios tested.
The invariant lifetime test passed 13 tests and 116 checks. The separate decision probe
found release DOMAIN for a noncanonical real ball, an invariant abort for that ball, and
an invariant abort after a hand write over a counted context field: three expected results.

## New code findings

None.

## Findings against the specification

The parenthetical reason for the `prec` cap in M1-D1 is false for the input in R8 above.
The limit itself is implemented as written. I found no counterexample to M1-D10 or M1-D11
in the three targeted decision cases.

## Sources pending

None for the claims of this closure. The equivalence reasons for all 108 mutation excuses
were not proved here from their cited sources; see the scope limit below.

## What I did not examine

I ran two formerly stale mutants through the full tests, not all 108 excuses. The 108-key
check establishes identity and uniqueness, not the truth of every equivalence reason.
I did not run the full `INV=1` suite; its five old out-of-contract tests are already known
and are being repaired in lane adf-6vy. I did not audit unrelated arithmetic, text, dump,
or context implementation beyond the surface paths reached by these runs.
