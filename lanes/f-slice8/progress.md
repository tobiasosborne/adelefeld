# Progress: f-slice8

Read CLAUDE.md, workflow and lane rules; SPEC 9.3.1, 9.3.3 and 15.4; PLAN 1F.5/1F.6;
functions.md Lemmas 3/9 and Propositions 4/11/13/15/16; the local and partial-ball interfaces,
series/decomposition implementations, existing tests, driver and the local-function review.

Design in progress: separate lroot.h/lroot.c; seeded roots and caller-owned arrays of all branches.
Odd-prime identifiers are nonzero unit residues; at 2, 1 and 3 mean sign +1 and -1.
Exact zero and degree 1 use identifier 0. Degree 1 is the identity, including uncertain zero.
Keep the strong guard for degrees >=2. Seedless existing _at roots remain UNSUPPORTED at primes.
Compute principal-unit roots by exp(Log(unit)/n), with the proved torsion factor supplied separately.
No Newton iteration for roots with nonunit derivative. No specification edits.

Implemented lroot.h/lroot.c and three prime _at wrappers. Existing seedless root_at/sqrt_at stay
UNSUPPORTED at primes. Root count and seed evaluation avoid enumerating the torsion group.
All branches use a polynomial of degree gcd(n,p-1), with a documented coefficient-count limit.
The principal-unit root uses exp(Log(unit centre)/n); Log avoids forming the input torsion factor.
Output torsion factors use adf_lball_teichmuller. No p-adic Newton root iteration was introduced.

Oracle: 6694 rows, 45708 finite residues, 5046 distance witnesses, 443052 fixture bytes.
Initial test compile had wrong JSON helper names; corrected before the intentional link failure.
Stub run: 3 tests, 204024 checks, 25948 failures. First implementation missed stdlib.h;
that build failed and the subsequent run was the stale stub executable (not a green result).
After the include fix: 3 tests, 276136 checks, 0 failures.
Extended local tests: 5 tests, 295394 checks, 0 failures; includes 2400 seeded comparisons.
_at red: 13 tests, 279181 checks, 53 failures. _at green: same totals, 0 failures.
Driver root-values and root-status handwritten goldens now match (10 and 14 output lines).
The initial golden command text used mathematical Zhat notation; corrected to the driver's
existing finite-ball syntax before implementing the commands. No expected value was changed.

Extended the grid to every integral ball at M=1..5 (p=2) and M=1..3 (p=3,5,7), n=1..12.
Final fixtures: 8110 ball rows and 1408 exact-input rows; 643421 bytes total.
Exact-input oracle uses N=4 and exhaustive roots modulo p^(4+v_p(n)), independently of exp/log.
The _at reference test consumes all 8110 ball rows. Local tests consume both files completely.
Normal local result: 6 tests, 380751 checks, 0 failures. _at: 14 tests, 521939 checks, 0 failures.

Five scratch faults were caught by assertions, with no timeouts:
- Guard one lower: 698 failed checks, 2 failed tests.
- Omit the (n-1)j term: 7799 failed checks, 2 failed tests.
- Missing branch: 1808 failed checks, 4 failed tests.
- Square criterion at 2 modulo 4: 16 failed checks, 1 failed test.
- Omit logarithm division: 23146 failed checks, 6 failed tests.
Fault copies and executables live under build/faults. No mutation tool or fuzz target was run.

API proofs R1-R7 written in docs/api-1f5.md. No counterexample to the specification found.
Read-only git diff/status confirmed that source changes are within the owned paths.
No state-changing git command or tracker command was run.

The final parser review found a missing operand after a trailing `with` was accepted. The new regression
first printed a root instead of PARSE (diff exit 1); the parser now rejects the missing operand.
The 15-line root-status golden matches. This repair landed before check-all's driver phase.

Final checks completed, no rerun of check-all:
- timeout 900 make -j2 check-all: exit 0.
  Last line: check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest
  Driver: 47 cases, 100867 expected lines; 0 differences. Exports: 420, missing 0, undeclared 0.
  Julia local roots: 8/8 assertions, using the existing system-GMP preload retry.
- ASAN_OPTIONS=detect_leaks=0 timeout 120 make -j2 BUILD=build/san SAN=1
  build/san/test_lroot build/san/test_rfunc_prime: exit 0.
- ASAN_OPTIONS=detect_leaks=0 timeout 60 build/san/test_lroot: exit 0.
  Last line: 6 tests, 380751 checks, 0 failed checks, 0 failed tests
- ASAN_OPTIONS=detect_leaks=0 timeout 60 build/san/test_rfunc_prime: exit 0.
  Last line: 14 tests, 521939 checks, 0 failed checks, 0 failed tests

No address/undefined-behaviour diagnostic occurred. LeakSanitizer was disabled as requested.
After the final runs only two prose comments were clarified (oracle scaling notation and README separators).
No implementation changed after the final runs. Remaining work is the final report and formatting audit.
