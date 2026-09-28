# Lane m1-closure-dump report

3 CLOSED; 0 CLOSED WITH EDIT; 0 OPEN; 1 SETTLED BY DECISION. NO BLOCKER OPEN.

## What was done

Read the binding lane rules, dump review, repair report, SPEC section 10 and all M1 decisions,
PLAN milestone 1, the relevant headers, conventions and repaired functions and tests.
Built the current source with make -j2 after a clean, using an owned BUILD directory.
Memory checks before builds found 23 or 24 GB available and 0 other compiler/build processes.
No git command, tracker command, package installation or production-file edit was made.
At most two CPU tasks ran concurrently; builds did not overlap. Child commands had a 170-second limit.

R1, R3 and R4 are CLOSED. R2 is SETTLED BY DECISION M1-D9. There are 0 new findings.
The complete reasoning and coverage are in docs/reviews/m1/dump/closure.md.

## Check results

| Check | Result |
|---|---|
| Original dump suites | 32 tests, 41,607 checks, 0 failures |
| Limit suite, default | 9 tests, 243 checks, 0 failures; slow full case skipped |
| Limit suite, ADF_DUMP_LIMITS_FULL=1 | 9 tests, 261 checks, 0 failures; 97.609768 seconds |
| Independent closure statuses | R1: 180; R2: 152 plus 12 controls; R4: 7,692; 8,036 total, 0 failures |
| Original public-API round trips | 6,000 round trips, 27,029 checks, 0 failures |
| Original guard-page check | 561 cases, 0 errors, 223 allocation calls, 0 retained allocations |
| Original status cases | 7 cases, 0 untouched-output errors; 3 old R2 expectations superseded by M1-D9 |
| Original 65,537-block cost case | 682,067 bytes, UNSUPPORTED, 0.006549 constructor CPU seconds |
| Near-limit modctx | 65,536 blocks, 1,048,571 bytes, OK, 30.084275 constructor CPU seconds |
| Near-limit qclass | 65,536 blocks, 1,048,574 bytes, OK, 33.153818 constructor CPU seconds |
| Original differential | 150,000 texts, 28 known reference mismatches; exit 1 retained |
| Differential independent checks | 24,150 predicates, 0 mismatches; 0 typed or output-contract errors |
| Valgrind | 4 programs, 0 errors, 0 live bytes for each |
| Fuzz harness neighbours | 14 seeds, 425 prefix cases, 0 failures under Valgrind and ASan/UBSan |
| Isolated R1 reversion | 26 checks, 2 failures; repaired version 0 failures |
| Isolated R2 reversion | 19 checks, 5 failures; repaired version 0 failures |
| Isolated R4 reversion | 72 checks, 12 failures; repaired version 0 failures |
| Isolated R3 reversion | SIGABRT, 1 uninitialized-value error; repaired seed 0 errors |
| Python repair regression class | 3 tests, 0 failures |
| ASan and UBSan | 7 executables, 0 reports; ASan leak detection disabled, leaks checked with Valgrind |

The first fuzz command completed 362,060 inputs in 31 seconds, then exited 2 because LeakSanitizer
failed at shutdown under ptrace. Its empty artifact replayed with exit 0 when leak detection was disabled.
The repeated campaign used ASAN_OPTIONS=detect_leaks=0. Its exact counts appear at the end of this report.
Both logs and the first failure are retained. The separate Valgrind leak results are reported above.

The differential's 28 discrepancies are the original character-modulus reference limit, not new C findings.
All old assertions and all seven substantive reproducer sources are unchanged. Copies in closure-checks
preserve the original review logs. The old runners, historical audit and cleanup scripts were read but not
executed; they overwrite old evidence and are not finding reproducers.

The R1 test helper all_load skips typed loaders if context construction fails. New closure cases invoke
those loaders directly. The selected regression expectations come from the contract, not program output.
Isolated reversions demonstrate detection; they do not prove the original historical TDD process.

## Not examined or not done

The m1-invariants and m1-repair-tools lanes have not landed. Their invariant checks, mutation runner and
memory-checker tool were not approved here. None of this review's four findings waits for those lanes.
No complete proof over all strings, allocation-failure injection, thread-safety review, unrelated context
constructor review, full mutation sweep, or arithmetic enclosure review beyond restoration was performed.
There is no typed qclass loader; those inputs were checked through context extraction.
The complete repository make check was not repeated; the changed dump surface and its original checks were run.
No production repair or specification change was made. The stale test comment saying the header lacks the
exponent constant was recorded in closure.md, not edited in this lane.

## Sources pending

[source pending: a FLINT 3.0.1 source under refs stating the exact MAG_MAN/MAG_EXP representation
used by src/dump.c:1297]. The original gap remains. refs/src/flint-3.0.1/mag.rst:6 states a 30-bit
mantissa and arbitrary-precision exponent but does not state the representation formula.

## Findings against the specification

None found. M1-D5 is now enforced. M1-D9 permits R2's limit and specifies its stage and scope.
The old report's proposed-decision wording is superseded by the accepted SPEC decision.

## Files written

Every retained path below was created or written in this lane. The only edits used apply_patch and the
owned check scripts. No original review/check file was changed. Temporary products removed at the end were
the build/ and san/ trees under closure-checks and the standalone binaries listed in final-audit.json.
The red/ source files remain so the isolated reversions are reviewable.

- docs/reviews/m1/dump/closure-checks/bridge.c
- docs/reviews/m1/dump/closure-checks/bridge.log
- docs/reviews/m1/dump/closure-checks/build.log
- docs/reviews/m1/dump/closure-checks/clean.log
- docs/reviews/m1/dump/closure-checks/closure-status.log
- docs/reviews/m1/dump/closure-checks/commands.jsonl
- docs/reviews/m1/dump/closure-checks/compile-bridge-so.log
- docs/reviews/m1/dump/closure-checks/compile-bridge.log
- docs/reviews/m1/dump/closure-checks/compile-fuzz-driver.log
- docs/reviews/m1/dump/closure-checks/compile-fuzz-neighbours.log
- docs/reviews/m1/dump/closure-checks/compile-green-R1.log
- docs/reviews/m1/dump/closure-checks/compile-green-R2.log
- docs/reviews/m1/dump/closure-checks/compile-green-R4.log
- docs/reviews/m1/dump/closure-checks/compile-red-R1.log
- docs/reviews/m1/dump/closure-checks/compile-red-R2.log
- docs/reviews/m1/dump/closure-checks/compile-red-R3.log
- docs/reviews/m1/dump/closure-checks/compile-red-R4.log
- docs/reviews/m1/dump/closure-checks/compile-roundtrip.log
- docs/reviews/m1/dump/closure-checks/compile-san-bridge.log
- docs/reviews/m1/dump/closure-checks/compile-san-fuzz_driver_closure.log
- docs/reviews/m1/dump/closure-checks/compile-san-roundtrip.log
- docs/reviews/m1/dump/closure-checks/cost-65537.log
- docs/reviews/m1/dump/closure-checks/cost.py
- docs/reviews/m1/dump/closure-checks/cost_near_limit.py
- docs/reviews/m1/dump/closure-checks/differential.py
- docs/reviews/m1/dump/closure-checks/final-audit.json
- docs/reviews/m1/dump/closure-checks/finish_closure.py
- docs/reviews/m1/dump/closure-checks/fuzz-empty-replay.log
- docs/reviews/m1/dump/closure-checks/fuzz-no-lsan.log
- docs/reviews/m1/dump/closure-checks/fuzz.log
- docs/reviews/m1/dump/closure-checks/fuzz_driver.c
- docs/reviews/m1/dump/closure-checks/fuzz_driver_closure.c
- docs/reviews/m1/dump/closure-checks/green-R1.log
- docs/reviews/m1/dump/closure-checks/green-R2.log
- docs/reviews/m1/dump/closure-checks/green-R4.log
- docs/reviews/m1/dump/closure-checks/limits-full.log
- docs/reviews/m1/dump/closure-checks/near-modctx.log
- docs/reviews/m1/dump/closure-checks/near-qclass.log
- docs/reviews/m1/dump/closure-checks/original-differential.log
- docs/reviews/m1/dump/closure-checks/original-status.log
- docs/reviews/m1/dump/closure-checks/red/R1.c
- docs/reviews/m1/dump/closure-checks/red/R1_test.c
- docs/reviews/m1/dump/closure-checks/red/R2.c
- docs/reviews/m1/dump/closure-checks/red/R2_test.c
- docs/reviews/m1/dump/closure-checks/red/R3_fuzz.c
- docs/reviews/m1/dump/closure-checks/red/R3_test.c
- docs/reviews/m1/dump/closure-checks/red/R4.c
- docs/reviews/m1/dump/closure-checks/red/R4_test.c
- docs/reviews/m1/dump/closure-checks/red-R1.log
- docs/reviews/m1/dump/closure-checks/red-R2.log
- docs/reviews/m1/dump/closure-checks/red-R3.log
- docs/reviews/m1/dump/closure-checks/red-R4.log
- docs/reviews/m1/dump/closure-checks/red-prepare.log
- docs/reviews/m1/dump/closure-checks/red_hunks.py
- docs/reviews/m1/dump/closure-checks/reference-repairs.log
- docs/reviews/m1/dump/closure-checks/reviewed-inputs.sha256
- docs/reviews/m1/dump/closure-checks/roundtrip.c
- docs/reviews/m1/dump/closure-checks/roundtrip.log
- docs/reviews/m1/dump/closure-checks/run.py
- docs/reviews/m1/dump/closure-checks/san-bridge.log
- docs/reviews/m1/dump/closure-checks/san-build.log
- docs/reviews/m1/dump/closure-checks/san-fuzz_driver_closure.log
- docs/reviews/m1/dump/closure-checks/san-roundtrip.log
- docs/reviews/m1/dump/closure-checks/san-test_dump.log
- docs/reviews/m1/dump/closure-checks/san-test_dump_ctx.log
- docs/reviews/m1/dump/closure-checks/san-test_dump_golden.log
- docs/reviews/m1/dump/closure-checks/san-test_dump_limits.log
- docs/reviews/m1/dump/closure-checks/status_findings.py
- docs/reviews/m1/dump/closure-checks/status_findings_closure.py
- docs/reviews/m1/dump/closure-checks/test_dump.log
- docs/reviews/m1/dump/closure-checks/test_dump_ctx.log
- docs/reviews/m1/dump/closure-checks/test_dump_golden.log
- docs/reviews/m1/dump/closure-checks/test_dump_limits.log
- docs/reviews/m1/dump/closure-checks/valgrind-bridge.log
- docs/reviews/m1/dump/closure-checks/valgrind-neighbours.log
- docs/reviews/m1/dump/closure-checks/valgrind-original.log
- docs/reviews/m1/dump/closure-checks/valgrind-roundtrip.log
- docs/reviews/m1/dump/closure.md
- lanes/m1-closure-dump/brief.md
- lanes/m1-closure-dump/lane.log
- lanes/m1-closure-dump/report.md
- lanes/m1-closure-dump/session-commands.json
- lanes/m1-closure-dump/stdout.log

## Every shell command and result

The working directory for each command is the repository root. Full read outputs are in
session-commands.json; child command outputs and timings are in closure-checks/*.log and
closure-checks/commands.jsonl. Build logs retain each expanded compiler command.

1. exit 0; cwd and matching paths listed.

```sh
pwd; rg --files -g 'AGENTS.md' -g 'CLAUDE.md' -g 'COMMON.md' -g 'review.md' -g 'report.md' -g 'SPEC.md' -g \
    'PLAN.md' | sort
```

2. exit 0; CLAUDE.md read.

```sh
cat CLAUDE.md
```

3. exit 0; output lines 58.

```sh
cat lanes/m1-closure/COMMON.md
```

4. exit 0; output lines 33.

```sh
cat lanes/m1-review/COMMON.md
```

5. exit 0; output lines 28.

```sh
cat lanes/COMMON.md
```

6. exit 0; output lines 151.

```sh
cat docs/reviews/m1/dump/review.md
```

7. exit 0; output lines 211.

```sh
cat lanes/m1-repair-dump/report.md
```

8. exit 0; output lines 94.

```sh
rg -n '^#|M1-D[1-9]|dump|M1|milestone 1' docs/SPEC.md docs/PLAN.md
```

9. exit 0; output lines 163.

```sh
rg --files -g 'AGENTS.md' -g '!deps/**' -g '!build/**' /home/tobias/Projects /home/tobias 2>/dev/null
```

10. exit 0; output lines 211.

```sh
cat lanes/m1-repair-dump/report.md
```

11. exit 0; output lines 12.

```sh
tail -12 docs/reviews/m1/dump/review.md
```

12. exit 0; output lines 380.

```sh
cat docs/PLAN.md
```

13. exit 0; output lines 237.

```sh
cat docs/reviews/m0-gate/closure.md
```

14. exit 0; output lines 231.

```sh
cat Makefile
```

15. exit 0; output lines 112.

```sh
rg --files docs/reviews/m1/dump/checks lanes/m1-repair-dump
```

16. exit 0; output lines 73.

```sh
sed -n '725,768p;840,875p' docs/SPEC.md
```

17. exit 0; output lines 146.

```sh
sed -n '80,225p' docs/PLAN.md
```

18. exit 0; output lines 148.

```sh
cat include/adelefeld/dump.h
```

19. exit 0; output lines 145.

```sh
sed -n '1,145p' include/adelefeld/modctx.h
```

20. exit 0; output lines 71.

```sh
cat docs/reviews/m1/dump/checks/verify.py
```

21. exit 0; output lines 48.

```sh
sed -n '1,48p' include/adelefeld/dump.h
```

22. exit 0; output lines 101.

```sh
sed -n '195,295p' docs/PLAN.md
```

23. exit 0; output lines 267.

```sh
sed -n '1032,1122p;1300,1475p' docs/conventions.md
```

24. exit 0; output lines 110.

```sh
cat docs/reviews/m1/dump/checks/status_findings.py docs/reviews/m1/dump/checks/cost.py \
    docs/reviews/m1/dump/checks/cost_near_limit.py
```

25. exit 0; output lines 34.

```sh
cat docs/reviews/m1/dump/checks/fuzz_driver.c docs/reviews/m1/dump/checks/README.md
```

26. exit 0; output lines 121.

```sh
cat docs/reviews/m1/dump/checks/audit.py docs/reviews/m1/dump/checks/finish.py \
    docs/reviews/m1/dump/checks/run.sh
```

27. exit 0; output lines 74.

```sh
sed -n '1034,1107p' docs/conventions.md
```

28. exit 0; output lines 260.

```sh
sed -n '1,260p' tests/test_dump_limits.c
```

29. exit 0; output lines 3.

```sh
free -g
```

30. exit 1; output lines 0. Zero matching build/compiler processes.

```sh
ps -eo pid,ppid,comm,args | rg '(^|/)(make|cc1|clang|gcc|cc)( |$)|[v]algrind'
```

31. exit 0; output lines 105.

```sh
sed -n '1,105p;320,460p' docs/reviews/m1/dump/checks/differential.py
```

32. exit 0; output lines 150.

```sh
cat docs/reviews/m1/dump/checks/bridge.c
```

33. exit 0; output lines 371.

```sh
sed -n '280,650p' tests/test_dump_limits.c
```

34. exit 0; output lines 434.

```sh
sed -n '340,455p;535,635p;725,835p;1040,1145p' src/dump.c
```

35. exit 0; output lines 1.

```sh
python3 -B - <<'PY'
from pathlib import Path
import shutil
p = Path('docs/reviews/m1/dump')
for name in ('bridge.c', 'roundtrip.c', 'fuzz_driver.c', 'status_findings.py', 'cost.py',
             'cost_near_limit.py', 'differential.py'):
    shutil.copyfile(p/'checks'/name, p/'closure-checks'/name)
print('unchanged_reproducer_copies=7')
PY
```

36. child exit 0; 0.016449 seconds; log clean.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py clean make \
    BUILD=docs/reviews/m1/dump/closure-checks/build clean
```

37. child exit 0; 2.92138 seconds; log build.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py build make -j2 \
    BUILD=docs/reviews/m1/dump/closure-checks/build \
    'CFLAGS=-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror' \
    docs/reviews/m1/dump/closure-checks/build/test_dump \
    docs/reviews/m1/dump/closure-checks/build/test_dump_ctx \
    docs/reviews/m1/dump/closure-checks/build/test_dump_golden \
    docs/reviews/m1/dump/closure-checks/build/test_dump_limits
```

38. exit 0; output lines 285.

```sh
sed -n '1,285p' tests/fuzz/fuzz_dump.c
```

39. exit 0; output lines 58.

```sh
sed -n '530,590p' src/modctx.c
```

40. exit 0; output lines 24.

```sh
rg -n 'header_before|grammar_before|TestDumpReview|QCLASS|qclass_exp|65536' tests/test_dump.c \
    proto/test_text_grammar.py proto/text_grammar.py
```

41. exit 0; output lines 50.

```sh
sed -n '1,50p' docs/reviews/m0-gate/closure.md
```

42. exit 0; output lines 127.

```sh
sed -n '1120,1200p;1275,1320p' src/dump.c
```

43. exit 0; output lines 3.

```sh
rg --files refs/src/flint-3.0.1 | rg 'mag|arf|arb'
```

44. exit 0; output lines 111.

```sh
sed -n '375,485p' tests/test_dump.c
```

45. exit 0; output lines 47.

```sh
sed -n '617,690p' proto/test_text_grammar.py
```

46. exit 0; output lines 65.

```sh
tail -65 docs/reviews/m1/dump/checks/differential.py
```

47. child exit 0; 0.215139 seconds; log compile-bridge-so.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-bridge-so cc -std=c11 -O2 -g -Wall -Wextra \
    -Werror -Iinclude -fPIC -shared docs/reviews/m1/dump/closure-checks/bridge.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/bridge.so
```

48. child exit 0; 0.215161 seconds; log compile-bridge.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-bridge cc -std=c11 -O2 -g -Wall -Wextra \
    -Werror -Iinclude docs/reviews/m1/dump/closure-checks/bridge.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/bridge
```

49. child exit 0; 0.214881 seconds; log compile-roundtrip.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-roundtrip cc -std=c11 -O2 -g -Wall -Wextra \
    -Werror -Iinclude docs/reviews/m1/dump/closure-checks/roundtrip.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/roundtrip
```

50. child exit 0; 0.265249 seconds; log compile-fuzz-driver.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-fuzz-driver cc -std=c11 -O0 -g -I. -Iinclude \
    docs/reviews/m1/dump/closure-checks/fuzz_driver.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/fuzz_driver
```

51. child exit 0; 0.015966 seconds; log test_dump.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py test_dump \
    docs/reviews/m1/dump/closure-checks/build/test_dump
```

52. child exit 0; 0.064725 seconds; log test_dump_ctx.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py test_dump_ctx \
    docs/reviews/m1/dump/closure-checks/build/test_dump_ctx
```

53. child exit 0; 0.016592 seconds; log test_dump_golden.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py test_dump_golden \
    docs/reviews/m1/dump/closure-checks/build/test_dump_golden
```

54. child exit 0; 9.587639 seconds; log test_dump_limits.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py test_dump_limits \
    docs/reviews/m1/dump/closure-checks/build/test_dump_limits
```

55. child exit 0; 0.004033 seconds; log bridge.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py bridge docs/reviews/m1/dump/closure-checks/bridge
```

56. child exit 0; 0.008249 seconds; log roundtrip.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py roundtrip \
    docs/reviews/m1/dump/closure-checks/roundtrip
```

57. child exit 0; 0.032619 seconds; log original-status.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py original-status python3 -B \
    docs/reviews/m1/dump/closure-checks/status_findings.py
```

58. child exit 0; 1.718376 seconds; log cost-65537.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py cost-65537 python3 -B \
    docs/reviews/m1/dump/closure-checks/cost.py 65537
```

59. exit 0; output lines 127.

```sh
sed -n '285,450p' tests/fuzz/fuzz_dump.c
```

60. child exit 0; 0.315344 seconds; log closure-status.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py closure-status python3 -B \
    docs/reviews/m1/dump/closure-checks/status_findings_closure.py
```

61. exit 0; output lines 181.

```sh
cat tests/test_runner.h
```

62. exit 0; output lines 60.

```sh
sed -n '665,700p;1045,1068p' lanes/m1-repair-dump/checks/red/dump_before.c
```

63. exit 0; output lines 29.

```sh
sed -n '1,22p;149,155p' refs/src/flint-3.0.1/mag.rst
```

64. exit 0; output lines 3.

```sh
free -g
```

65. exit 1; output lines 0. Zero matching build/compiler processes.

```sh
ps -eo pid,ppid,comm,args | rg '(^|/)(make|cc1|clang|gcc|cc)( |$)|[v]algrind'
```

66. child exit 1; 5.126498 seconds; log original-differential.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py original-differential python3 -B \
    docs/reviews/m1/dump/closure-checks/differential.py
```

67. exit 0; output lines 59.

```sh
sed -n '697,755p' lanes/m1-repair-dump/checks/red/dump_before.c
```

68. child exit 0; 33.849319 seconds; log near-modctx.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py near-modctx python3 -B \
    docs/reviews/m1/dump/closure-checks/cost_near_limit.py modctx
```

69. child exit 0; 0.032215 seconds; log red-prepare.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py red-prepare python3 -B \
    docs/reviews/m1/dump/closure-checks/red_hunks.py
```

70. child exit 0; 0.265121 seconds; log compile-fuzz-neighbours.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-fuzz-neighbours cc -std=c11 -O0 -g -I. \
    -Iinclude docs/reviews/m1/dump/closure-checks/fuzz_driver_closure.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/fuzz_driver_closure
```

71. child exit 0; 0.565707 seconds; log valgrind-original.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py valgrind-original /home/tobias/.local/bin/valgrind \
    --track-origins=yes --leak-check=full --error-exitcode=99 \
    docs/reviews/m1/dump/closure-checks/fuzz_driver
```

72. child exit 0; 1.116986 seconds; log valgrind-neighbours.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py valgrind-neighbours /home/tobias/.local/bin/valgrind \
    --track-origins=yes --leak-check=full --error-exitcode=99 \
    docs/reviews/m1/dump/closure-checks/fuzz_driver_closure
```

73. child exit 0; 0.91714 seconds; log compile-red-R1.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-red-R1 cc -std=c11 -O2 -g -I. -Iinclude -Isrc \
    -Itests docs/reviews/m1/dump/closure-checks/red/R1.c docs/reviews/m1/dump/closure-checks/red/R1_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R1
```

74. child exit 1; 1.618458 seconds; log red-R1.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py red-R1 docs/reviews/m1/dump/closure-checks/red/R1
```

75. child exit 0; 0.916529 seconds; log compile-red-R2.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-red-R2 cc -std=c11 -O2 -g -I. -Iinclude -Isrc \
    -Itests docs/reviews/m1/dump/closure-checks/red/R2.c docs/reviews/m1/dump/closure-checks/red/R2_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R2
```

76. child exit 1; 0.465317 seconds; log red-R2.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py red-R2 docs/reviews/m1/dump/closure-checks/red/R2
```

77. child exit 0; 1.066977 seconds; log compile-red-R4.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-red-R4 cc -std=c11 -O2 -g -I. -Iinclude -Isrc \
    -Itests docs/reviews/m1/dump/closure-checks/red/R4.c docs/reviews/m1/dump/closure-checks/red/R4_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R4
```

78. child exit 1; 0.004016 seconds; log red-R4.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py red-R4 docs/reviews/m1/dump/closure-checks/red/R4
```

79. child exit 0; 0.265212 seconds; log compile-red-R3.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-red-R3 cc -std=c11 -O0 -g -I. -Iinclude \
    docs/reviews/m1/dump/closure-checks/red/R3_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R3
```

80. child exit -6; 0.866265 seconds; log red-R3.log. SIGABRT; wrapper exit 250.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py red-R3 /home/tobias/.local/bin/valgrind \
    --track-origins=yes --error-exitcode=99 docs/reviews/m1/dump/closure-checks/red/R3
```

81. child exit 0; 36.407336 seconds; log near-qclass.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py near-qclass python3 -B \
    docs/reviews/m1/dump/closure-checks/cost_near_limit.py qclass
```

82. exit 0; output lines 18.

```sh
sed -n '636,705p' tests/test_dump_limits.c
```

83. exit 0; output lines 16.

```sh
rg -n '100000|reference|limit' proto/text_grammar.py | head -16
```

84. child exit 0; 97.609768 seconds; log limits-full.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py limits-full env ADF_DUMP_LIMITS_FULL=1 \
    docs/reviews/m1/dump/closure-checks/build/test_dump_limits
```

85. exit 0; output lines 25.

```sh
nl -ba include/adelefeld/dump.h | sed -n '20,33p;63,73p'
```

86. exit 0; output lines 35.

```sh
nl -ba docs/conventions.md | sed -n '1066,1100p'
```

87. exit 0; output lines 21.

```sh
rg -n \
'dp_w_arb\(|ADF_MODCTX_MAX_BLOCKS|st->mode == DP_LIMITS && pieces|b < 0x20|char \* t = adf_'\
'.*dump_str|size_t tl = ' src/dump.c tests/fuzz/fuzz_dump.c
```

88. exit 0; output lines 39.

```sh
sed -n '1,24p;782,796p' proto/text_grammar.py
```

89. exit 0; output lines 13.

```sh
rg -n 'Sources pending|Not examined|not done|Finding|R[1-9]' docs/reviews/m1/dump/review.md
```

90. child exit 0; 0.415416 seconds; log compile-green-R1.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-green-R1 cc -std=c11 -O2 -g -I. -Iinclude \
    -Isrc -Itests docs/reviews/m1/dump/closure-checks/red/R1_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R1_green
```

91. child exit 0; 1.618361 seconds; log green-R1.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py green-R1 \
    docs/reviews/m1/dump/closure-checks/red/R1_green
```

92. child exit 0; 0.41582 seconds; log compile-green-R2.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-green-R2 cc -std=c11 -O2 -g -I. -Iinclude \
    -Isrc -Itests docs/reviews/m1/dump/closure-checks/red/R2_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R2_green
```

93. child exit 0; 0.415451 seconds; log green-R2.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py green-R2 \
    docs/reviews/m1/dump/closure-checks/red/R2_green
```

94. child exit 0; 0.566296 seconds; log compile-green-R4.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-green-R4 cc -std=c11 -O2 -g -I. -Iinclude \
    -Isrc -Itests docs/reviews/m1/dump/closure-checks/red/R4_test.c \
    docs/reviews/m1/dump/closure-checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/red/R4_green
```

95. child exit 0; 0.003987 seconds; log green-R4.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py green-R4 \
    docs/reviews/m1/dump/closure-checks/red/R4_green
```

96. child exit 0; 0.565808 seconds; log valgrind-bridge.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py valgrind-bridge /home/tobias/.local/bin/valgrind \
    --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    docs/reviews/m1/dump/closure-checks/bridge
```

97. child exit 0; 0.966403 seconds; log valgrind-roundtrip.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py valgrind-roundtrip /home/tobias/.local/bin/valgrind \
    --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    docs/reviews/m1/dump/closure-checks/roundtrip
```

98. child exit 0; 0.315186 seconds; log reference-repairs.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py reference-repairs python3 -B -m unittest \
    proto.test_text_grammar.TestDumpReviewFindings
```

99. exit 0; output lines 3.

```sh
free -g
```

100. exit 1; output lines 0. Zero matching build/compiler processes.

```sh
ps -eo pid,ppid,comm,args | rg '(^|/)(make|cc1|clang|gcc|cc)( |$)|[v]algrind'
```

101. child exit 0; 4.776357 seconds; log san-build.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-build make -j2 \
    BUILD=docs/reviews/m1/dump/closure-checks/san SAN=1 docs/reviews/m1/dump/closure-checks/san/test_dump \
    docs/reviews/m1/dump/closure-checks/san/test_dump_ctx \
    docs/reviews/m1/dump/closure-checks/san/test_dump_golden \
    docs/reviews/m1/dump/closure-checks/san/test_dump_limits
```

102. child exit 0; 0.265504 seconds; log compile-san-bridge.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-san-bridge cc -std=c11 -O1 -g -I. -Iinclude \
    -fsanitize=address,undefined -fno-omit-frame-pointer docs/reviews/m1/dump/closure-checks/bridge.c \
    docs/reviews/m1/dump/closure-checks/san/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/bridge-san
```

103. child exit 0; 0.265293 seconds; log compile-san-roundtrip.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-san-roundtrip cc -std=c11 -O1 -g -I. -Iinclude \
    -fsanitize=address,undefined -fno-omit-frame-pointer docs/reviews/m1/dump/closure-checks/roundtrip.c \
    docs/reviews/m1/dump/closure-checks/san/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/roundtrip-san
```

104. child exit 0; 1.017131 seconds; log compile-san-fuzz_driver_closure.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py compile-san-fuzz_driver_closure cc -std=c11 -O1 -g -I. \
    -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer \
    docs/reviews/m1/dump/closure-checks/fuzz_driver_closure.c \
    docs/reviews/m1/dump/closure-checks/san/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/closure-checks/fuzz_driver_closure-san
```

105. child exit 0; 0.064615 seconds; log san-test_dump.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-test_dump env ASAN_OPTIONS=detect_leaks=0 \
    UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/closure-checks/san/test_dump
```

106. child exit 0; 0.165236 seconds; log san-test_dump_ctx.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-test_dump_ctx env ASAN_OPTIONS=detect_leaks=0 \
    UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/closure-checks/san/test_dump_ctx
```

107. child exit 0; 0.032662 seconds; log san-test_dump_golden.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-test_dump_golden env ASAN_OPTIONS=detect_leaks=0 \
    UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/closure-checks/san/test_dump_golden
```

108. child exit 0; 15.855307 seconds; log san-test_dump_limits.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-test_dump_limits env ASAN_OPTIONS=detect_leaks=0 \
    UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/closure-checks/san/test_dump_limits
```

109. child exit 0; 0.032389 seconds; log san-bridge.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-bridge env ASAN_OPTIONS=detect_leaks=0 \
    UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/closure-checks/bridge-san
```

110. child exit 0; 0.064568 seconds; log san-roundtrip.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-roundtrip env ASAN_OPTIONS=detect_leaks=0 \
    UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/closure-checks/roundtrip-san
```

111. child exit 0; 0.064787 seconds; log san-fuzz_driver_closure.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py san-fuzz_driver_closure env \
    ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
    docs/reviews/m1/dump/closure-checks/fuzz_driver_closure-san
```

112. exit 0; output lines 3.

```sh
free -g
```

113. exit 1; output lines 0. Zero matching build/compiler processes.

```sh
ps -eo pid,ppid,comm,args | rg '(^|/)(make|cc1|clang|gcc|cc)( |$)|[v]algrind'
```

114. child exit 2; 32.341704 seconds; log fuzz.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py fuzz make -j2 \
    BUILD=docs/reviews/m1/dump/closure-checks/build fuzz FUZZ_TARGET=dump FUZZ_SECONDS=30 FUZZ_WORKERS=1 \
    COV=0
```

115. exit 0; output lines 1.

```sh
python3 -B - <<'PY'
from pathlib import Path
p = Path('docs/reviews/m1/dump/closure.md')
for i, line in enumerate(p.read_text().splitlines(), 1):
    if len(line) > 116:
        print(i, len(line), line)
print('closure_lines=' + str(len(p.read_text().splitlines())))
PY
```

116. exit 0; output lines 4.

```sh
python3 -B - <<'PY'
import ast
from pathlib import Path
p = Path('docs/reviews/m1/dump/closure-checks/finish_closure.py')
s = p.read_text()
ast.parse(s)
for node in ast.walk(ast.parse(s)):
    if isinstance(node, ast.Constant) and isinstance(node.value, str) and len(node.value) > 1000:
        for i, line in enumerate(node.value.splitlines(), 1):
            if len(line) > 116:
                print('report_template_line', i, 'length', len(line), line)
for path in (Path('tests/test_dump_limits.c'), Path('src/dump.c')):
    for i, line in enumerate(path.read_text().splitlines(), 1):
        if 'header does not define' in line or 'dp_mag_set_exact(' in line:
            print(path, i, line)
print('syntax_errors=0')
PY
```

117. child exit 0; 0.032334 seconds; log fuzz-empty-replay.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py fuzz-empty-replay env ASAN_OPTIONS=detect_leaks=0 \
    docs/reviews/m1/dump/closure-checks/build/fuzz/dump \
    docs/reviews/m1/dump/closure-checks/build/fuzz/artifacts/crash-da39a3ee5e6b4b0d3255bfef95601890afd80709 \
    -runs=1
```

118. child exit 0; 31.042462 seconds; log fuzz-no-lsan.log.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/run.py fuzz-no-lsan env ASAN_OPTIONS=detect_leaks=0 \
    docs/reviews/m1/dump/closure-checks/build/fuzz/dump \
    docs/reviews/m1/dump/closure-checks/build/fuzz/corpus/dump -max_total_time=30 -workers=1 \
    -rss_limit_mb=2048 -print_final_stats=1 \
    -artifact_prefix=docs/reviews/m1/dump/closure-checks/build/fuzz/artifacts/
```

119. exit 0; output lines 2.

```sh
python3 -B - <<'PY'
import ast
from pathlib import Path
p = Path('docs/reviews/m1/dump/closure-checks/finish_closure.py')
for node in ast.walk(ast.parse(p.read_text())):
    if isinstance(node, ast.Constant) and isinstance(node.value, str) and len(node.value) > 1000:
        long = [(i, len(x)) for i, x in enumerate(node.value.splitlines(), 1) if len(x) > 116]
        print('report_template_lines_over_116=' + str(len(long)))
        assert not long, long
print('syntax_errors=0')
PY
```

120. exit 1; output lines 5.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/finish_closure.py
```

121. exit 0; output lines 1.

```sh
python3 -B - <<'PY'
import ast
import json
from pathlib import Path
import re
p = Path('docs/reviews/m1/dump/closure-checks/finish_closure.py')
node = next(n for n in ast.parse(p.read_text()).body if isinstance(n, ast.FunctionDef))
exec(compile(ast.Module(body=[node], type_ignores=[]), str(p), 'exec'))
rows = json.loads(Path('lanes/m1-closure-dump/session-commands.json').read_text())
for row in rows:
    for line in command_block(row['cmd']):
        if len(line) > 116:
            print(len(line), line)
PY
```

122. exit 0; output lines 2.

```sh
python3 -B docs/reviews/m1/dump/closure-checks/finish_closure.py
```

123. exit 0; output lines 112.

```sh
cat docs/reviews/m1/dump/closure.md
```

124. exit 0; output lines 83.

```sh
sed -n '1,83p' lanes/m1-closure-dump/report.md
```

125. exit 0; final audit and cleanup results:

```sh
python3 -B docs/reviews/m1/dump/closure-checks/finish_closure.py
```

```text
{
  "unchanged_reproducer_copies": 7,
  "python_scripts_parsed": 8,
  "syntax_errors": 0,
  "reviewed_input_hashes": 40,
  "child_commands": 54,
  "explained_nonzero_children": 6,
  "unexplained_nonzero_children": 0,
  "maximum_child_seconds": 97.609768,
  "removed_build_directories": [
    "build",
    "san"
  ],
  "removed_binaries": [
    "bridge",
    "bridge-san",
    "bridge.so",
    "fuzz_driver",
    "fuzz_driver_closure",
    "fuzz_driver_closure-san",
    "red/R1",
    "red/R1_green",
    "red/R2",
    "red/R2_green",
    "red/R3",
    "red/R4",
    "red/R4_green",
    "roundtrip",
    "roundtrip-san"
  ],
  "remaining_binaries": 0,
  "closure_lines_over_116": 0
}
```

## Fuzz campaign output

```text
Done 237937 runs in 31 second(s)
stat::number_of_executed_units: 237937
stat::average_exec_per_sec:     7675
stat::new_units_added:          150
stat::slowest_unit_time_sec:    0
stat::peak_rss_mb:              188
```
