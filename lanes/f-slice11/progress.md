# f-slice11 progress

Read the lane rules, design IL1-IL8 and the requested contracts and patterns.
No state-changing git command or bd is used. Builds use two jobs.
Slice A starts with the four declarations. No cross-type alias is permitted.
The implementation will use a private local descriptor and compact integer unit centres.
The exact-zero shortcut remains confined to Log_at.

Slice A library, fixture comparisons, real endpoints, driver and Julia are green.
C: 6 tests, 318552 checks, zero failed checks. Driver: 12 value lines (exit 0),
7 status lines (exit 1), both exact golden matches. Julia: 59/59 checks after the
documented system GMP preload. The initial loader failure is retained in julia-A.log.
All 8 design faults and 4 wrapper faults compiled and failed assertions in scratch copies.
The real evaluator failure is injected: 4 calls, zero preservation/status failures.
The 6776 selected rows plus real rows occupy 874650 bytes, below 1 MB.
Mutation follows before slice A is declared finished.

The full INV=1 build fails in read-only src/text.c at lines 2623 and 2943: stray #x.
The mutation tool correctly rejects that unmutated baseline (exit 2, zero judged mutants).
Fallback: every object is sanitized, and gfunc_log.c plus its test are separately compiled
with ADF_CHECK_INVARIANTS. Other translation units use INV=0. No source outside this lane
is repaired. Mutation uses two jobs and one make job per worker to respect the two-core cap.
The scratch root is a snapshot outside the mutation scratch itself; copying lanes into a
scratch under the live lanes directory would recurse (mutate.py copy_tree ignores only pycache).

Slice A finished before starting slice B.
Library: four exported declarations, source, all selected oracle rows, exact fields and complete
residue-set tests, real identity/containment, resource/status preservation, large primes,
thousands-bit content and compact exact lifts. Driver: 12+7 golden lines. Julia: 59 checks.
Expanded C result: 8 tests, 318570 checks, zero failed checks.
Automatic mutation A: 30 selected, 23 compiled, 19 killed, 4 survived, 7 did not compile,
0 timeouts. The real-precision-ceiling survivor now fails real_precision_ceiling.
The m=a+b survivor preserves |m|: coprimality means at most one valuation is positive.
The k==0 to k==1 survivor is covered by K<=d in both cases, so no result changes.
The positive exponent boundary survivor cannot be witnessed with a feasible finite unit
modulus in slice A. Slice B's exact-zero rounding exposes the same helper boundary by status.
A sanitizer baseline reached 318563 zero-failure assertions, then LSan failed under ptrace.
Mutation A3 disables only leak detection; address/UB instrumentation and lane invariant checks remain.
Full leak checking is not claimed. Final required detect_leaks=1 run remains to be attempted.

Slice B code is green: 12 tests, 2193803 checks before adding the eight verbatim fault
witness rows. CRT: 280 rows, 525 projections, 73056 membership checks. Both B driver
goldens match (11 value lines, 7 status lines). Julia A+B: 59+22 checks, zero failures.
All 8 finite faults and 4 additional B wrapper faults compiled and were rejected again.
The preflight evaluates earlier local candidates before naming a later known LIMIT,
so an earlier working-power LIMIT is not hidden by a later compact-power refusal.
The design's unspecified aggregate order is now explicit in the header and G11-G12.

Mutation B: 30 selected, 20 compiled, 19 killed, 1 survived, 10 did not compile,
zero timeouts. Its survivor is max_exp's > to >=, identical when arguments are equal.
A's positive-exponent-boundary survivor is now rejected by the B aggregate/place test.
No automatic sweep was repeated. Both test-gap survivors were recompiled and rejected
separately after the tests were added. Remaining survivors have direct reasons in the code.
Final fixture selection: 6784 local rows, 6 real rows, 280 CRT rows, 913102 bytes total.
The eight design fault witnesses are now included verbatim as extra oracle rows.
Injected real evaluator failure: 6 calls, zero failed preservation/status checks.
All implementation and interface work is finished. The one acceptance run and required
sanitizer build/run follow. No report.md exists yet; it will be written once after those checks.

The one acceptance run passed: 77 C programs, 55 driver cases / 101019 expected lines,
442/442 exports, Julia including 81 new checks, and both tool self-tests. Exit 0.
Required sanitizer build: exit 0. Required detect_leaks=1 run: all 2194049 assertions
passed, then LeakSanitizer failed under ptrace (exit 1). This is not a passing leak scan.
The supplemental Valgrind run exposed a test harness issue: byte snapshots included unset
struct padding and inactive small-arf mantissa slots. These are now initialized before
snapshots; all full-byte comparisons and all value/status assertions remain in place.
FLINT's final cache cleanup follows refs/src/flint-3.0.1/memory.rst:26-43.
The first Valgrind run had 100 errors / 57 contexts and 577640 retained bytes. After
struct padding and cleanup it had 36 errors / 9 contexts and zero retained bytes.
Only the test harness changed after the one full acceptance run; src/gfunc_log.c did not.
No second full check-all or sanitizer build is run, in accordance with the one-run brief.

Final Valgrind result after defining all inactive bytes: exit 0, 12 tests / 2194049 checks,
zero failed checks, zero errors from zero contexts, 613570 allocations and 613570 frees,
zero bytes and zero blocks in use at exit. No suppression file was used.
The complete byte checks still compare every struct byte. Active fields are unchanged by
snapshot preparation. FLINT's inactive small-arf representation is documented at
refs/src/flint-3.0.1/arf.rst:43-54. All requested implementation work is done.
