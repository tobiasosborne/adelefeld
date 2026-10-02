# f-fixture1 running notes

Required sources read. Library changes and the optional benchmark are out of scope for this battery run.
The new generator was written first. It uses seed 20261002 and the OLD code at 1cf0e42.

The actual route rule uses W = K + e(T), not K + e(2K). The exact tagged-word check is
bits(p^W) <= FLINT_BITS - 2. The boundary depends on the valuation used for T.
At 2, F9 never enters with v(z)=1: Log_centre changes the sign to give v(z)>=2.
At the word prime no positive working exponent fits a tagged word; K=0 and K=1 are shortcuts.
The generator includes those boundary requests and three K=2 rows for the first actual F8 entry.

Initial command: timeout 180 make -j2 BUILD=lanes/f-fixture1/build
lanes/f-fixture1/build/libadelefeld.a lanes/f-fixture1/build/support/jsonl.o
lanes/f-fixture1/build/support/golden.o. Output: build.log; exit 0.

timeout 120 sh lanes/f-fixture1/build.sh: exit 0. probe-build.log gives old source SHA256
e39a78444a4123b3875a16f6b9c3aa06a8edc61dfcf5647691e3e4eb763dcfdd.
timeout 30 python3 -B lanes/f-fixture1/gen_stored_large.py --plan: exit 0, 465 inputs.
plan.log: 240 F8, 168 F9, 23 word, 6 shortcut and 28 exp rows.
Actual word boundaries for v(z)=c: 57,36,24,21,16,3,0 at 2,3,5,7,11,65537,2^64-59.

timeout 180 python3 -B lanes/f-fixture1/gen_stored_large.py: exit 0; 465 rows, 534848 bytes,
30.58 s elapsed. Old output and alias calls: 930. Alias/unchanged/canonical failures: 0.
Generation took 18.384 s for all 138 calls at the word prime; no individual long-N benchmark was run.

Added stored_large_before_optimisation before the old comparison; all existing test bodies are unchanged.
timeout 10 sh lanes/f-fixture1/prepare_faults.sh: exit 0. Review scripts and read-only repository paths
are linked into build/harness; its lanes/f-review4/build points to this lane's build directory.

From build/harness, with core dumps disabled and a 2 GiB address-space limit:
timeout 180 python3 -B lanes/f-review4/faults.py ../faults f8_digit_K40: script exit 0.
Faulty test exit 1; new comparison 346 failed checks; old comparison 0; full suite 387 failures.
The script also ran its mandatory small oracle: 7440 cases, 1621 old/new differences.
fault-f8_digit_K40.log: 45.93 s elapsed. This is the red run.

timeout 60 make -j2 BUILD=lanes/f-fixture1/build lanes/f-fixture1/build/test_lfunc: exit 0.
timeout 60 lanes/f-fixture1/build/test_lfunc: exit 0; 13 tests, 473031 checks, 0 failures; 3.99 s.
green.log records the unmodified library. The isolated new test has 18602 checks and 0 failures.

timeout 90 python3 -B lanes/f-fixture1/route_audit.py: exit 0. A scratch C copy prints each selected
route and its actual K, W, vz. All 862 entries (431 rows and their alias calls) match the Python audit.
route-audit.log and route-trace.log retain the evidence. No library source was changed.

timeout 180 python3 -B lanes/f-review4/faults.py ../faults f9_inverse (same scratch cwd): exit 0.
Faulty test exit 1; new comparison 242 failures; old 10; full suite 520. Small oracle: 7440 inputs,
824 old/new differences. Time 46.54 s. The brief calls this fault a survivor, but the review table
correctly gives 10 old-fixture failures; f9_inverse_bigp is the old-fixture survivor.

timeout 10 sha256sum -c lanes/f-fixture1/unchanged.sha256: 3 matches, 0 mismatches.
git diff --check: exit 0. git diff --stat: tests/test_lfunc.c only, 33 inserted lines (new files untracked).

The isolated test command was timeout 60 lanes/f-fixture1/build/stored_large_only: exit 0,
18602 checks, 0 failures, 0.95 s (stored-only-green.log). Build flags are those of test_lfunc;
the executable includes the test file and calls only the new test body.

timeout 180 python3 -B lanes/f-review4/faults.py ../faults f9_inverse_bigp: script exit 0;
test exit 1; new comparison 42 failures, old 0, all 44. Small oracle 7440 inputs, 160 differences.
Elapsed 65.58 s. f9_valuation: script exit 0, test signal 11 while executing the NEW comparison.
Its small oracle exits 1 because the probe crashes. Script time 5.84 s.
The direct timeout 60 build/faults/f9_valuation/test_lfunc rerun (with the full lane prefix) exits 139.
fault-f9_valuation-assertions.log records 4 assertion failures at rows 33 and 34, then signal 11 at row 35.
The count is a prefix, not a completed run. Core dumps were disabled; elapsed 0.29 s.

An initial inline Python statistics check exited 1 at Python's 4300-digit JSON integer limit.
check_artifacts.py explicitly removes that limit, as the generator already does.
timeout 10 python3 -B lanes/f-fixture1/check_artifacts.py: exit 0, 0 input mismatches, 0 long lines.
Functions: 220 log, 217 Log, 28 exp. Inputs: 241 exact, 224 balls; 168 negative Log valuations.

timeout 180 python3 -B lanes/f-review4/faults.py ../faults f9_tree: script exit 0; test exit 1.
New comparison: 336 failed checks (all 168 F9 rows, value and alias); old 18; full suite 700.
Small oracle: 7440 cases, 1408 old/new differences; 64.32 s elapsed.

The six actual faults from the review are being run individually, each under an outer 180 s timeout.
The referee script unconditionally runs its small oracle; no exhaustive/random/large mode is requested.
The unmodified control uses control.py to add an identity replacement to the original referee harness.
The source, archive build, test and oracle logic of the harness are unchanged.
At most two single-threaded processes overlap; no build uses more than -j2.

f8_digit: script exit 0, test exit 1; new 390 failed checks, old 650, full suite 1708.
Its small oracle has 7440 inputs and 3272 old/new differences; elapsed 65.44 s.
timeout 180 python3 -B ../../control.py from build/harness: exit 0; unmodified scratch test exit 0,
13 tests, 473031 checks, 0 failures; small oracle 7440 cases with 0 reported mismatches; 64.71 s.
cmp src/lfunc.c lanes/f-fixture1/build/faults/control_unmodified/lfunc.c: exit 0, byte-identical.

All six faults are detected by the NEW comparison. The control_lower_bound variant is not a fault;
it was not rerun. The required control is the byte-identical, unmodified source copy instead.
The optional MINOR 2 optimization and every benchmark are omitted to keep compute small.
Final suites use affinity 0,1, so subprocesses of the self-tests also stay on at most two cores.

The ONE final normal command was:
timeout 180 taskset -c 0,1 sh -c 'make clean && make -j2 check-all'
It reached the 180.00 s limit, exit 124, at build/test_resid_rest. No assertion failure appears in
check-all.log; test_lfunc had already completed with 13 tests, 473031 checks and 0 failed checks.
Last line: make: *** [Makefile:123: check-all] Terminated
The normal suite is incomplete; it is not reported as a pass and will not be restarted.

timeout 60 taskset -c 0,1 build/test_lfunc: exit 0, 13 tests, 473031 checks, 0 failures; 3.82 s.
final-lfunc.log and final-lfunc.time record the requested test time in the final build directory.
The ONE sanitizer command is running with ASAN_OPTIONS=detect_leaks=0:
timeout 180 taskset -c 0,1 make -j2 check SAN=1 BUILD=build/san
The distinct build directory forces sanitizer compilation without a second clean or stale normal objects.

The sanitizer suite also hit 180.00 s, exit 124, at build/san/test_dump_limits.
Last line: make: *** [Makefile:102: check] Terminated
Its test_lfunc binary was built but not reached, so that already-built binary is now being run directly
under timeout 60 with ASAN_OPTIONS=detect_leaks=0. Neither full suite is restarted.

ASAN_OPTIONS=detect_leaks=0 timeout 60 taskset -c 0,1 build/san/test_lfunc: exit 0; 7.08 s;
13 tests, 473031 checks, 0 failed checks, 0 failed tests; 0 ASan/UBSan diagnostics.
The incomplete normal suite completed 46 programs and 38613598 checks with 0 failed checks.
The incomplete sanitizer suite completed 15 programs and 428223 checks with 0 failed checks.
Counts come from completed per-program summary lines; an interrupted program is excluded.
final-summary.log records the extraction (timeout 10 python3 -B with standard-library regexes).

Items 1 to 3 are complete. Full-suite completion remains for the orchestrator because both one-shot
acceptance commands reached the required three-minute cap. No further rebuild or full-suite run is made.
No finding against docs/SPEC.md or the numerical implementation was found. The report will distinguish
the brief's v(z)=1 at 2, its approximate working precision, and its name for the surviving inverse fault.
