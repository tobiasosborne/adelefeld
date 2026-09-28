# Milestone 1 dump review closure
3 CLOSED; 0 CLOSED WITH EDIT; 0 OPEN; 1 SETTLED BY DECISION. NO BLOCKER OPEN.

| Finding | Review severity | Verdict | Evidence |
|---|---|---|---|
| R1 | MAJOR | CLOSED | 65,537 blocks rejected; 180 boundary cases and full cap tests pass. |
| R2 | MAJOR | SETTLED BY DECISION | M1-D9; 152 bounded cases and 12 unbounded controls pass. |
| R3 | MAJOR | CLOSED | Original seed and 425 prefix cases pass Valgrind with 0 errors. |
| R4 | MAJOR | CLOSED | Original inputs return UNSUPPORTED; 7,692 byte/header cases pass. |

## R2: M1-D9

The accepted decision is at `docs/SPEC.md:868`. It permits the fixed bound of 2^20 on the absolute
dumped midpoint and radius exponents of every qclass piece. It requires LIMIT at stage 4, before
semantic checks. A qclass lift and other bodies have no such bound.

The original `status_findings.py` was rerun unchanged. Its above-bound valid piece and its two
above-bound invalid pieces now return LIMIT. Its three old expected statuses are superseded by M1-D9.
All seven original cases leave failed outputs untouched.

The replacement is `closure-checks/status_findings_closure.py`. It checks both signs, both exponent
fields, bound minus one, the bound, bound plus one, 160-bit exponents, zero and invalid mantissas,
both finite backends, later pieces, invalid earlier pieces, syntax errors and an out-of-range
occurrence index. There are 152 bounded cases and 12 lift/adele/cadele controls, with 0 failures.

The decision, `include/adelefeld/dump.h:26` and `:69`, `docs/conventions.md:1075`, and
`src/dump.c:549` and `:796` agree. No contract disagreement remains on this bound.
The repair report's statement that M1-D9 is still proposed is superseded by the accepted decision.
The comment at `tests/test_dump_limits.c:61` saying the header lacks the constant is also stale;
the constant is defined at `include/adelefeld/dump.h:72`.

## Evidence for the closed findings

R1: `src/dump.c:608` enforces M1-D5 before context construction. The unchanged 682,067-byte
reproducer returns UNSUPPORTED in 0.006549 CPU seconds; its scaled inspector returns the same status.
The new 180 cases cover counts 65,535 through 65,538, nine body forms, count limits, grammar failures,
length limits and a later context after an invalid piece. Matching typed loaders are called even when
construction fails. No neighbouring failure was found.

The full admitted-cap test ran with `ADF_DUMP_LIMITS_FULL=1`: 9 tests, 261 checks, 0 failures.
The unchanged byte-budget searches also accept 65,536-block contexts: modctx at 1,048,571 bytes
in 30.084275 CPU seconds; qclass at 1,048,574 bytes in 33.153818 CPU seconds.
These are individual measurements on the shared machine, not benchmark medians.

R3: the dump call now precedes the length read at all five typed sites in
`tests/fuzz/fuzz_dump.c:141`, `:165`, `:201`, `:230` and `:253`.
The context-dump site at `:352` also initializes its length.
The unchanged GCC O0 seed completes. Fourteen seeds covering all five value types, contexts,
both signs and both finite backends give 425 full or truncated inputs. Both runs report
0 Valgrind errors and 0 bytes in use at exit. No neighbouring failure was found.

R4: `src/dump.c:1058` admits TAB, LF and CR through the alphabet stage. Unknown versions and
fields then return UNSUPPORTED before body grammar. All 256 byte values were tested at two body
positions, with three headers and five typed entry points, plus 12 header/length cases:
7,692 cases, 0 failures. The known header rejects forbidden dump whitespace as PARSE.
No neighbouring failure was found.

## Regression tests and other review observations

The repaired tests were checked against isolated reversions, leaving production files untouched.
The selected R1, R2 and R4 regression functions have respectively 26, 19 and 72 checks.
They fail 2, 5 and 12 checks against the reversions and 0 against the repaired library.
Restoring R3's old length expression gives SIGABRT and 1 Valgrind uninitialized-value error.
Their expected statuses follow M1-D5, M1-D9 and conventions 8.2/8.5; byte identity follows CV-38.

The R1 test helper `all_load` skips typed loaders after failed construction. Its original coverage
claim is therefore too broad. The closure cases supply those direct loader calls. The historical
claim that the original tests were written before implementation cannot be established retrospectively.

The three original dump test programs pass 32 tests and 41,607 checks. The unchanged public-API
roundtrip program passes 6,000 round trips and 27,029 checks. The unchanged guard-page program passes
561 cases, with 223 allocator calls and 0 retained allocations. The latter two also report
0 Valgrind errors and 0 live bytes.

The four dump test programs, guard-page program, roundtrip program and 425-case fuzz driver also pass
under ASan and UBSan: 7 executables, 0 sanitizer reports. Leak detection was disabled for ASan;
the separate Valgrind runs above checked leaks.

A 30-second fuzz campaign completed 362,060 inputs, then exited 2 because LeakSanitizer failed at
shutdown under ptrace. Its empty artifact replayed with exit 0 when leak detection was disabled.
The repeated campaign completed 237,937 inputs in 31 seconds, with exit 0 and no sanitizer report.
The failed first command and both logs are retained; neither campaign establishes all-input correctness.

The unchanged differential program exits 1: 150,000 texts, 28 reference mismatches, 0 typed mismatches,
0 contract failures, and 24,150 independent predicate checks with 0 mismatches.
All 28 mismatches are the already reported character-modulus restriction of the Python reference
(`proto/text_grammar.py:19` and `:791`); they are not new C findings. The assertion was kept.
The three Python repair regression tests pass.

## New findings

None. No counterexample to the accepted specification was found.

## Not examined

The unlanded m1-invariants and m1-repair-tools lanes were not examined. This closure does not approve
ADF_CHECK_INVARIANTS, the mutation runner or the memory-checker tool. None of R1 to R4 depends on
those lanes. No finding awaiting those lanes has been closed.

Not examined: all-input correctness, allocation-failure injection, thread safety, unrelated context
constructors, arithmetic enclosure correctness beyond restoration, or a full mutation campaign.
There is no typed qclass loader here, so qclass cases use context extraction. Old orchestration and
historical audit/cleanup scripts were read but not rerun over the original review evidence.
All seven substantive reproducer sources were copied unchanged and rerun.

The original source gap remains:
[source pending: a FLINT 3.0.1 source under refs stating the exact MAG_MAN/MAG_EXP representation
used by src/dump.c:1297]. The local `refs/src/flint-3.0.1/mag.rst:6` states the mantissa width and
exponent type, but not that representation. This is not evidence of a runtime failure.

Commands, exit statuses and full logs are under `closure-checks/`; the complete command inventory,
written-file list and limitations are in `lanes/m1-closure-dump/report.md`.
