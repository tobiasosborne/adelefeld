# Red and green log

All commands run at repository root, with timeout and at most two compiler jobs.

- `timeout 120 python3 -B lanes/c-slice2/gen_vectors.py`: exit 0, 6239 JSONL records, 143639 bytes.
- Initial test build: exit 2, missing stdint.h and unsupported arf_cmp_fmpq test helper.
  Corrected those test compilation errors before implementation.
- `timeout 60 make -s -j2 BUILD=lanes/c-slice2/build lanes/c-slice2/build/test_char_eval`:
  exit 2; unresolved slice b functions (first-file link red).
- Weak lane stubs permit independent assertion reds for conjugation and dumps.
  The compile command links tests/test_char_eval.c, build/support/*.o, the lane archive,
  and lanes/c-slice2/red_stubs.c with cc -Iinclude -Itests -std=c11 -O2 and -lflint -lgmp -lm.
- `timeout 60 lanes/c-slice2/build/test_char_eval unit`: exit 0, 1040139 checks (before p=128 addition).
- `timeout 60 lanes/c-slice2/build/test_char_eval conj`: exit 134; conjugated s assertion rejects copy stub.
- `timeout 60 lanes/c-slice2/build/test_char_eval dump`: exit 134; first status rejects unsupported stub.
- After exponent negation and strict dump code, same conj/dump commands: exits 0, 238537/710 checks.
- Complete test: exit 0, 1279386 checks; test_char: exit 0, 129725 checks.
- Driver fixtures were written by hand first. Old driver: process exit 1 and diff exit 1 (unknown commands).
- Driver patch first attempted: apply_patch failed before changing any file (TEXT vs DRV case label).
- After driver code: two processes exit 1 as required by error rows; both diff commands exit 0.
- `timeout 180 sh tests/test_driver.sh`: exit 0, 82 cases, 101492 expected lines.
- First Julia suite: exit 1, relative dlopen path before new test reached any assertion.
  Fixed the path with abspath and corrected the strict status literal from 6 to the header's 1.
- `JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh`:
  exit 0; new test 16/16, shared exports 552/552, no missing symbols.
- Added p=128 on every coset, huge ambiguous N, inclusive cap on a general root, semantic precedence cases,
  and optional setup/phase injection checks. Complete plain test: exit 0, 1737303 checks.
- New warmed-allocation test initially failed after clearing FLINT caches immediately before counting.
  Replaced that setup with ten warmed cycles, matching the documented global-cache contract
  (refs/src/flint-3.0.1/memory.rst:26-44). The zero-live assertion was retained.
  Final counts: plain/SAN/clang 3300 blocks, INV 8100 blocks, zero live after every one of 100 cycles.
- Added a canonical q=65537 conjugation input and two more INV child failures; final plain/SAN/clang
  test_char_eval: 1738068 checks; INV: 1738095; wrapped INV: 1738116. Each exits 0.
- Final required configurations: all four character tests pass; test_dump_ctx exits 1 in each
  configuration at its read-only line 1363. D1 LIMIT disagrees with its older DOMAIN expectation.
  All six other dump/qclass programs pass in each configuration. No test was changed to hide this.
- Final eleven named fault builds: all compile; each is rejected by at least one of the two tests.
  test_char catches lowering/parity; test_char_eval catches the new unit/conjugation/dump faults.
- Direct mutation baseline with SAN+INV and wrappers: exit 0, char 129825 checks, eval 1738116 checks.
- Closed the dump_ctx mismatch without changing its test. Header dirichlet.h:144-147 says label 1
  is principal, so q>1,n=1 is cheaply imprimitive and DOMAIN precedes D1. A new word-max principal
  vector first fails the current loader (exit 134), then passes after the cheap semantic check.
  The above-cap loader test now uses verified primitive (65537,3); it still returns LIMIT.
- Added (27,2,2,9), whose asymmetric exact phase set is {1/18,7/18,13/18}, to reject sign reversal
  in the distance calculation. Added principal setup counts, borderline cosine width and nonfinite
  sqrt injections. Four selected survivor rechecks abort (-6); the selected infinite macro times out (124).
- The first aggregate final configuration run ended before the last two Clang results. The shell's
  final tail did not retain the wrapped script's exit status. Completed Clang separately with
  `timeout 180 python3 -B lanes/c-slice2/run_checks.py clang`: exit 0, all nine programs exit 0.
  Final logs contain 38 completed executions: 36 required tests, the INV wrapper, and the failed
  LeakSanitizer environment attempt. No required test fails in the final four configurations.
- Final test_char_eval counts: 1738298 plain/SAN/clang, 1738325 INV, 1738351 wrapped INV/SAN+INV.
  The six other dump tests and qclass_dump pass; dump_ctx now has 0 failed checks/tests.
- Guarded deferred generic character arithmetic at the driver kind stage. The fixture first differs
  (DOMAIN versus expected UNSUPPORTED, diff exit 1), then passes with the character guard.
  `timeout 180 sh tests/test_driver.sh`: exit 0; 82 cases, 101494 expected lines, 0 differences.
- Final Julia suite: exit 0; new test 16/16, exports 552/552. C11/C++17 header probes compile and
  run with exit 0; sizeof=120, align=8, general-root cap status=1, other status=0.
- Changed the ball-before-D1 test to nonprincipal label 3 so the principal rejection cannot mask
  a missing ball check. Rebuilt and reran all four eval tests and the combined wrapped baseline:
  all exit 0 with the same final counts above.
