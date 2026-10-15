# t-timing1 (bead adf-mis2): the factor on the wall-clock bounds under the sanitizer build

## Changed
- tests/support/timing.h (new): ADF_TIMING_FACTOR is 4 if __SANITIZE_ADDRESS__ (gcc) or
  __has_feature(address_sanitizer) (clang, guarded by defined(__has_feature)) is on, else 1. The comment cites
  Makefile lines 47-51 and the reason.
- tests/test_lball.c: include; the three bounds (old lines 1724, 1730, 1901) are `3.5 * ADF_TIMING_FACTOR`, and the
  message prints the bound used (`bound %.1f s`).
- tests/test_lfunc.c: include; the child's `alarm(5)` (SIGALRM kills the child, the parent sees a non-zero wait
  status) is `alarm(5 * ADF_TIMING_FACTOR)`; the message prints `5 * ADF_TIMING_FACTOR` seconds.
- tests/test_roots_real_isolate.c: include; ISOLATION_SECONDS_MAX is (2.0 * ADF_TIMING_FACTOR); the message
  already printed the macro, so it prints the bound used.
No other line was touched. Plain build: all five bounds equal their old values (factor 1).

## Checks (host load average about 4 to 5 during the runs)
- `timeout 900 make -j2 build/test_lball build/test_lfunc build/test_roots_real_isolate`: built, no warnings (-Werror).
  Run under `timeout 600`, exit 0: test_lball 30 tests, 3982758 checks, 0 failed, 2.4 s; test_lfunc 13 tests,
  473031 checks, 0 failed, 2.1 s; test_roots_real_isolate 15 tests, 19320 checks, 0 failed, 12.9 s.
  The isolation test prints review input 0..3 (plain): 0.2897, 0.0280, 0.0038, 0.0019 s (bound 2.0).
- `timeout 900 make -j2 BUILD=build/suite-san SAN=1 build/suite-san/test_lball build/suite-san/test_lfunc
  build/suite-san/test_roots_real_isolate`: built; run, exit 0: test_lball 0 failed, 3.3 s; test_lfunc 0 failed,
  3.0 s; test_roots_real_isolate 0 failed, 30.4 s; review input 0..3: 0.5486, 0.0299, 0.0045, 0.0051 s (bound 8.0).
- The lball and lfunc checks print nothing on success, so their individual times are not read; only the whole
  program times above (both are below the plain bounds, since the whole program ran in 3.3 s and 3.0 s).
- The factor 4 was not shown to be needed or sufficient under load: I did not reproduce the failures at load 6 to 17.

## Other timing bounds in tests/ (grep for now_s(, now(, alarm(, SECONDS, clock(, elapsed): NOT changed
The brief named five; these exist as well and were left alone because they were not in the brief and I own no
such files: alarm(5) in test_qclass.c:282, test_qclass_sets.c:285, test_qclass_arith.c:423, test_qclass_reduce.c:370
(crash guards); wall or CPU bounds in test_char.c:192 (0.010 s), test_dump_limits.c:338 (1.0 s),
test_modctx_limits.c:153,183,206,256,265 (1.0 s), test_qclass_sets.c:184,270 (1.0 s), test_gfunc_log.c:44 (1.0 s),
test_lroot.c:589,614,617,629,714,786 (5.0, 2.0, 10.0, GUARD_R9, 0.02 s), test_poisson.c:403 (1.0 s); the list from
grep of other files (test_resid_full, test_s13_repair, test_tate*, test_roots_real, test_text_idele) was not read
line by line. Whether any of these fails under SAN=1 at load is unknown; they are candidates for the same header.

## Not done
No full `make check`, no mutation test, no git, no bd.
