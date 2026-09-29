# Running notes

Read COMMON, COMMON-C, CLAUDE, PERF before measurements, SPEC real-root rows and decisions,
PLAN solver rows, real-roots design, implementation, slice1 result and r1 review.
Floor: compulsory input reading and ball writing; exact endpoint signs for the retained certificate.
For no-root lists eval2 is zero and no ratio to it is defined.
Plan: count-guided stopping and certified geometric endpoint contraction.

Red tests: three 2 s checks failed (37.025229, 12.499560, 5.726564 s); thirds already passed.
First green attempt still failed Mignotte and complex (27.846966 and 4.494423 s).
Stage probes: complex count 3.925342 s; Mignotte isolation about 0.1 s, refinement 22 s.
Added local exact evaluation and certified Arb filters with integer fallback. Candidate cells are unchanged.
The count uses an equivalent small rational translate and dyadic scale when the centroid has <=16 bits.
This is necessary for the complex input's 2 s requirement; its FLINT count was the remaining cost.
Certificate endpoint signs use an exact local polynomial only when both mantissas shrink by more than half.
Mignotte public call now 0.406920 s; complex 0.003676 s. More validation remains.
A snapshot compile initially lacked JSONL support objects (link error); corrected compile succeeded.
Prototype's old linear-node lower assertion failed as expected after the contraction (15 nodes at e=100).
Replaced it with the new geometric-node upper assertion; did not relax a correctness condition.

Exact capture: cmp of 4701 isolate-program lines and 2701 public-program lines returned 0.
The three-root minimum-scale test first timed out at 170 s. A stage probe normalised in 3.776466 s,
then timed out while computing FLINT's unscaled cubic count. It was not an isolation timeout.
Count preconditioning now also scales tiny-root polynomials of degree >=3, without needing a small centroid.
The original cubic test was restored as a fourth case, alongside three pairs. All cases passed:
14 tests, 19225 checks, 0 failures; six pair roots and three cubic roots at exponent -16777206.
The two 170 s timeouts remain recorded as red evidence, not passing checks.

Final benchmark (median five, CPU 2): Mignotte 0.111501 s, positive 0.020773 s, complex 0.002827 s,
thirds 0.001259 s, sqrt2 at 2^21 0.371910 s. The endpoint reference now mirrors local certificate signs;
Mignotte eval2 is 0.004330214 s. Empty-list eval2 is 0 and its ratio is undefined.
check-all: exit 0, 61 test programs, final line "check-all passed: make check, driver, exports, julia,
mutate-selftest, memcheck-selftest". The self-test's expected mutant-survivor output is not a lane mutation run.
Default SAN run reports LeakSanitizer fatal ptrace errors after passing assertions; it is not counted as passed.
The supported ASAN/UBSAN rerun will disable the unsupported LeakSanitizer component explicitly.
High precision profile (shared machine): 42 Q steps, 0.496059450 s in Q, 176 exact values,
0.379211108 s in exact values (included in Q time); candidate/refinement 0.496975920 s,
final certificate with normalization 0.201822189 s. Profile is a scratch copy, not production instrumentation.
Profile first tried the review probe's unsupported sqrt2 family and exited 2; the lane copy added that family.
Procedural limitation: auxiliary compiles and short Python checks were not globally pinned throughout.
Some overlapped the two-core build or SAN loop; more than two runnable CPU jobs were briefly possible.
No make used more than -j2. Subsequent full checks run alone with shared affinity to CPUs 0 and 1.

ASAN/UBSAN (detect_leaks=0) and header checks passed. ASAN/UBSAN: 61 test programs, 631 tests,
51950408 checks, 0 failed checks, 0 failed tests. Default SAN had the same assertion totals but exit 2,
with 61 LeakSanitizer ptrace failures. Header final line: check_headers: passed.
Final captures parse as 1318 calls/3383 balls and 665 calls/2036 balls. Both cmp checks exit 0.
Latest prototype unit run: 18 tests, 0 failures (2.270 s); production input transformations are mirrored.
The inherited IVT source is also pending (solvers.md:64-67), in addition to Sturm and Eqir convergence.

Clang full check: exit 0, all 61 programs. Final exports: 325/325, 0 missing or undeclared.
Differential smoke: timeout 180 python3 tests/fuzz/diff_roots_real.py --seconds 120 --seed 20260930,
with bytecode disabled and shared CPU affinity 0,1. Exit 0: 1075 calls, 1054 OK, 21 DOMAIN,
2848 roots, 1180 exact balls, 35 empty lists, 541 reduced inputs, 1054 exact prototype matches,
0 disagreements. This is a smoke test, not the long campaign.

Final planted-fault repeat: 3/3 caught, each 34 checks and 4 failed assertions, exit 1 per mutant.
Focused valgrind: Mignotte and complex completed, each 0 errors and 0 bytes in use at exit.
Minimum-scale cubic under valgrind hit its 50 s timeout during FLINT normalization (SIGTERM, exit 124).
Its termination snapshot had 122342416 bytes in use and 25 leak contexts; no normal-shutdown leak result
is claimed for that case. A smaller e=1000 cubic runs the same count-scaling and point-refinement paths.

The e=1000 cubic valgrind check completed: 0 errors, 4204 allocations/4204 frees, 0 bytes in use at exit.
Thus three completed focused valgrind cases have 0 errors and 0 outstanding bytes; the extreme cubic is
still an incomplete valgrind check. Its native and ASAN/UBSAN test cases passed.
Final changes after full checks are documentation and C proof-comment references only.
