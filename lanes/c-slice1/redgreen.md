# Slice a red and green runs

The oracle completed 323350 checks with exit 0 before implementation.
Source acquisition: `timeout 20 curl` for FLINT 3.0.1 group_init.c returned 6 (DNS unavailable).
The design's missing pairing and group-init source remains pending.

- First build: `timeout 180 make -j2 BUILD=lanes/c-slice1/build .../test_char`, exit 2:
  character declarations/type absent. Scaffold build then exposed nonexistent test helper
  acb_contains_si, exit 2; replaced it by the two coordinate containment predicates.
- Lifecycle red: `timeout 30 .../test_char life`, exit 134 at is_canonical.
  Green after predicate implementation: exit 0, 22 checks.
- Constructor red: `timeout 30 .../test_char constructors`, exit 134 at set_conrey_acb OK.
  Green after lowering: exit 0, 31481 checks, cap refusal 0.000000106 s.
- Phase red: `timeout 30 .../test_char phases`, exit 134 at chi_phase OK.
  Green after exact phase/chi: exit 0, 78921 checks.
- Gauss red: `timeout 30 .../test_char gauss`, exit 134 at gauss_sum OK.
  Green after P4: exit 0, 5592 checks.
- Text red: `timeout 30 .../test_char texts`, exit 134 at set_str OK.
  First implemented run: exit 134 at an unjustified nested-ball assertion.
  Replaced it with containment of the four exact input endpoints, obtained from the text oracle.
  Generator first used bytes for its string-only internal parser: exit 1; corrected to str.
  Full green: `timeout 60 .../test_char`, exit 0, 116817 checks.
- Driver expected lines were written before command implementation.
  `timeout 30 build/adf < tests/driver/char-values.cmd`: red output differs in all 17 rows.
  After implementation: exit 1 as the fixture requests; 17 lines, diff exit 0.


- Extended checks before mutation: plain/SAN/clang 129694; INV 129757; wrapped INV 129781.
  Allocation hooks: 1800 blocks (plain/SAN/clang), 5800 (INV), 0 live after 100 warmed cycles.
  LeakSanitizer with detect_leaks=1 exited 1: unavailable under ptrace. detect_leaks=0 passed.
  First INV build exited 2 for an unchecked freopen return in the test; corrected, then green.
- The direct-sum modulus-boundary probe (65536,5), prec=2 returned OK in 0.048656 CPU seconds.
  This became a default test with radius and magnitude assertions for both tau and W.
- Mutation sweep: 60 selected from 315; 1018.2 s; 44 killed, 5 survived,
  9 not compiled, 2 timed out, 0 excused; tool exit 1.
  Three survivors were test gaps: swap's first input, both radius predicates, inclusive phase cap.
  Added the missing INV arguments, enclosing-overwide numerical injections, and (65536,5) phase/order calls.
  All three SAN+INV scratch rechecks compiled (0) and aborted (-6); 12.529 s for the first recheck script.
  The two remaining survivors are the symmetric gcd argument swap and the principal root general formula.
  No implementation statement or test requirement was weakened.

Final golden checks also compare W with normalized FLINT tau at 256 bits (17 further checks).
Final plain/SAN/clang: 129725, INV: 129797, wrapped INV: 129825; every run exits 0.
Final text/driver/Julia suites exit 0. Five lane build trees and all fault/survivor copies were removed.
