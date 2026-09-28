# Lane m1-review-dump report

NO BLOCKER FOUND; 0 BLOCKER, 4 MAJOR, 0 MINOR.

- R1 MAJOR: adf_modctx_new_from_dump accepts 65,537 blocks despite the 65,536-block cap in M1-D5.
- R2 MAJOR: an undocumented qclass binary-exponent limit rejects a valid context occurrence.
- R3 MAJOR: the unchanged dump fuzz target reads an unset dump length and aborts on a valid rational seed.
- R4 MAJOR: TAB/LF body syntax errors override an unsupported dump header, contrary to check ordering.

The complete review is `docs/reviews/m1/dump/review.md`. Each finding has the input, contract and source
locations, and a reproducer that was run. No implementation, specification, header or project test was changed.
No git command or tracker command was run. Builds used make -j2; measured constructors ran one at a time.

Results: 30 project tests, 41,487 checks, 0 failures; also 0 failures under ASan and UBSan.
Public-API round trips: 6,000, with 27,029 checks and 0 failures. Guarded-page inputs: 561, with 0 contract
errors. FLINT counting allocator: 223 calls, 0 retained allocations. Both review programs have 0 Valgrind
errors and 0 live bytes after cleanup. LeakSanitizer was disabled; Valgrind supplied the leak check.

Differential: 150,000 texts across all 15 bodies. The second run adds typed inspection and independent
semantic checks: 50,000 typed comparisons, 0 mismatches; 24,150 independent predicate checks, 0 mismatches.
There are 28 constructor/reference mismatches from the Python character-modulus cutoff; both differential
runs exit 1. They are not reported as passing. The cutoff is a declared reference limitation, not a C defect.

The status diagnostic prints 4 differences in 7 cases and exits 0. Two differences demonstrate R2; two
demonstrate R4. The unchanged fuzz driver terminates with SIGABRT on `adf1 Q rat 1 1`; Valgrind reports
one uninitialized-value error originating in load_one. That is a test-harness defect, not a library crash.

Cost: the 682,067-byte dump with 65,537 blocks returns OK in 65.005279 CPU seconds (R1).
At the allowed block cap, a 1,048,571-byte modctx dump takes 59.495496 CPU seconds. A 1,048,574-byte
qclass dump takes 68.804485 CPU seconds, the slowest admitted candidate measured. These are single
measurements, not a proven worst case or benchmark medians. The longest complete command took 134.982010
seconds. The scripts generate their inputs independently from a prime sieve and exact integer products.

Files written or updated:

- `docs/reviews/m1/dump/review.md` and this report.
- In `docs/reviews/m1/dump/checks/`: bridge.c, differential.py, cost.py, cost_near_limit.py, verify.py,
  finish.py, README.md, commands.md, verified-commands.jsonl, verified-*.log, cleanup.log,
  reviewed-files.sha256, audit.py and final-audit.log.
- Existing roundtrip.c, status_findings.py and fuzz_driver.c were inspected and run without changes.
  Earlier unprefixed logs and run.sh were retained and are not evidence for this continuation.

Cleanup: finish.py removed the build and san directories and 7 standalone binaries. It found 0 remaining
binaries. Sources, scripts and logs remain. Empty early compile logs named verified-bridge.log and
verified-roundtrip.log were overwritten during rebuilding; their runtime output is retained in the
sanitizer and Valgrind logs. All 31 executed child commands are inventoried below.

Not done: allocation-failure injection; a complete proof over all inputs; thread safety; other context
constructors; arithmetic enclosure correctness outside restoration; a repaired coverage-guided fuzz run.
The five typed APIs have at most one occurrence, so repeated bindings cannot be tested through them.
Multi-occurrence qclass context extraction is covered by the existing tests. There is no qclass value
loader in this milestone surface. The local and scaled tests still use field-by-field helpers; this review
supplements them with current public-API identity checks. No assertion was weakened.

Sources pending:

- [source pending: a FLINT 3.0.1 source under refs specifying the exact MAG_MAN/MAG_EXP field equation].
- [source pending: the C language text under refs on evaluation order of function arguments].

The FLINT mag documentation confirms 30 mantissa bits but not the field equation. The fuzz defect is
confirmed by execution independently of the missing language citation. The old pending multi-CRT reference
now exists at `refs/src/flint-3.0.1/fmpz.rst:1354`; the review checked it.

Findings against the specification: no mathematical counterexample to SPEC was found. R1 violates M1-D5;
R2 and R4 violate the loader contract. The Python reference shares R2 and R4. The specification was not edited.

Commands and results follow. These are the actual expanded commands from verified-commands.jsonl.
The phase runner records child failures even when its own process exits 0.

1. build: exit 0; elapsed 0.033664 seconds.

```sh
make -j2 BUILD=docs/reviews/m1/dump/checks/build \
    'CFLAGS=-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror' \
    docs/reviews/m1/dump/checks/build/test_dump docs/reviews/m1/dump/checks/build/test_dump_ctx \
    docs/reviews/m1/dump/checks/build/test_dump_golden
```

2. bridge.so: exit 0; elapsed 0.617659 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude -fPIC -shared docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/bridge.so
```

3. bridge: exit 0; elapsed 0.617263 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o docs/reviews/m1/dump/checks/bridge
```

4. roundtrip: exit 0; elapsed 0.566698 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/roundtrip.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/roundtrip
```

5. test_dump: exit 0; elapsed 0.064788 seconds.

```sh
docs/reviews/m1/dump/checks/build/test_dump
```

6. test_dump_ctx: exit 0; elapsed 0.165331 seconds.

```sh
docs/reviews/m1/dump/checks/build/test_dump_ctx
```

7. test_dump_golden: exit 0; elapsed 0.032722 seconds.

```sh
docs/reviews/m1/dump/checks/build/test_dump_golden
```

8. bridge: exit 0; elapsed 0.00866 seconds.

```sh
docs/reviews/m1/dump/checks/bridge
```

9. roundtrip: exit 0; elapsed 0.033248 seconds.

```sh
docs/reviews/m1/dump/checks/roundtrip
```

10. status: exit 0; elapsed 0.065363 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/status_findings.py
```

11. differential: exit 1; elapsed 10.051575 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/differential.py
```

12. valgrind-bridge: exit 0; elapsed 1.219163 seconds.

```sh
valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    docs/reviews/m1/dump/checks/bridge
```

13. valgrind-roundtrip: exit 0; elapsed 2.27269 seconds.

```sh
valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    docs/reviews/m1/dump/checks/roundtrip
```

14. fuzz-build: exit 0; elapsed 0.516906 seconds.

```sh
cc -std=c11 -O0 -g -I. -Iinclude docs/reviews/m1/dump/checks/fuzz_driver.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/fuzz-gcc-o0
```

15. fuzz-seed: exit -6; elapsed 2.624828 seconds.

```sh
valgrind --track-origins=yes --error-exitcode=99 docs/reviews/m1/dump/checks/fuzz-gcc-o0
```

16. build: exit 0; elapsed 0.016796 seconds.

```sh
make -j2 BUILD=docs/reviews/m1/dump/checks/build \
    'CFLAGS=-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror' \
    docs/reviews/m1/dump/checks/build/test_dump docs/reviews/m1/dump/checks/build/test_dump_ctx \
    docs/reviews/m1/dump/checks/build/test_dump_golden
```

17. bridge.so: exit 0; elapsed 0.717525 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude -fPIC -shared docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/bridge.so
```

18. bridge: exit 0; elapsed 0.618457 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o docs/reviews/m1/dump/checks/bridge
```

19. roundtrip: exit 0; elapsed 0.718442 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/roundtrip.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/roundtrip
```

20. differential: exit 1; elapsed 13.962481 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/differential.py
```

21. san-build: exit 0; elapsed 0.016503 seconds.

```sh
make -j2 BUILD=docs/reviews/m1/dump/checks/san SAN=1 docs/reviews/m1/dump/checks/san/test_dump \
    docs/reviews/m1/dump/checks/san/test_dump_ctx docs/reviews/m1/dump/checks/san/test_dump_golden
```

22. bridge-san-build: exit 0; elapsed 0.566738 seconds.

```sh
cc -std=c11 -O1 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer \
    docs/reviews/m1/dump/checks/bridge.c docs/reviews/m1/dump/checks/san/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/bridge-san
```

23. roundtrip-san-build: exit 0; elapsed 0.617011 seconds.

```sh
cc -std=c11 -O1 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer \
    docs/reviews/m1/dump/checks/roundtrip.c docs/reviews/m1/dump/checks/san/libadelefeld.a -lflint -lgmp -lm \
    -o docs/reviews/m1/dump/checks/roundtrip-san
```

24. san-test_dump: exit 0; elapsed 0.116015 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/san/test_dump
```

25. san-test_dump_ctx: exit 0; elapsed 0.316192 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/san/test_dump_ctx
```

26. san-test_dump_golden: exit 0; elapsed 0.03259 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/san/test_dump_golden
```

27. san-bridge: exit 0; elapsed 0.032578 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/bridge-san
```

28. san-roundtrip: exit 0; elapsed 0.115212 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/roundtrip-san
```

29. cost: exit 0; elapsed 71.933129 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/cost.py 65537
```

30. near-modctx: exit 0; elapsed 134.98201 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/cost_near_limit.py modctx
```

31. near-qclass: exit 0; elapsed 74.817593 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/cost_near_limit.py qclass
```
Additional artifact checks:

- Python ast.parse of finish.py: exit 0, 0 syntax errors.
- sha256sum of the 8 reviewed source/header/test files: exit 0; hashes retained.
- rg --files for core and core.*: exit 1, 0 matches.
- python3 -B docs/reviews/m1/dump/checks/finish.py: exit 0; 2 directories and 7 binaries removed.
- python3 -B docs/reviews/m1/dump/checks/audit.py: exit 0; 4 documents, 0 missing, 0 lines over 116.
  Seven Python scripts parse with 0 errors; 8 source hashes match; 0 build directories and 0 binaries remain.
  Full output is in docs/reviews/m1/dump/checks/final-audit.log.
