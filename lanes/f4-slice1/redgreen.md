# Slice 4a red/green record

All test programs and scripts use timeout. Builds use make -j2.

- Oracle: `timeout 120 python3 -B proto/functions4_checks.py`: exit 0, 2670 checks.
- Generator first run: exit 1, 767453-byte function fixture exceeded 400 KB. Reduced only reference
  precision for L=1024 from 320 to 96 bits. Small references remain at 320 bits.
- Generator edit run: exit 1, indentation error; corrected before using the fixtures.
- Generator final initial run: exit 0; 15 function, 1 sum, 4 cap records, 354823 bytes total.
- Initial test build: exit 2, absent adf_ffun declarations. First-file compile red.
- Staged build: exit 2, FLINT 3.0.1 has no _acb_vec_equal. Replaced it by acb_equal per cell.
- Lifecycle green: `timeout 60 lanes/f4-slice1/build/test_ffun life`: exit 0, 32 checks.
- Text red: `timeout 60 lanes/f4-slice1/build/test_ffun text`: exit 134, expected OK at first golden.
- Refinement red: `timeout 60 lanes/f4-slice1/build/test_ffun sum`: exit 134, expected refine OK.
- Fourier red: `timeout 60 lanes/f4-slice1/build/test_ffun fourier`: exit 134, expected Fourier OK.
- Text/refinement build: exit 2, missing SIZE_MAX include in appended text block; added local stdint include.
- Refinement green/sum red: test sum exit 134 at adf_ffun_add after refined cells succeeded.
- Sum green: test sum exit 0, 627 checks.
- Text test correction: digits=0 violated text.h printer precondition. Changed test to ADF_DIGITS_DEFAULT.
  Text green: exit 0, 79 checks, all 18 goldens.
- Fourier implementation red against an overwide reference: (D,M)=(3,2), k=1 has real part exactly -5/8;
  C produced that exact coordinate, while the independent reference retained a positive radius.
  Generator now reduces cyclotomic polynomials and writes rational coordinates exactly. No containment
  or radius assertion was relaxed. Generator Poly scalar API correction: one AttributeError, exit 1.
- Caps fixture correction: LIMIT is 10, not 9 (read status.h). The previous cap run failed at the status.
- Generator green: exit 0; 15+1+4 records, 336764 bytes.
- Fourier green: `timeout 120 lanes/f4-slice1/build/test_ffun fourier`: exit 0, 37742 checks.
- Caps green: `timeout 120 lanes/f4-slice1/build/test_ffun caps`: exit 0, 65 checks.
- Additional alias/radius/2002-bit/three-precision/allocation tests: plain exit 0, 38643 total checks.
- Driver red: `timeout 30 build/adf < tests/driver/ffun-fourier.cmd`: exit 1, 4 PARSE lines.
  Expected fixture status spelling corrected to the driver's existing `error: STATUS` format before coding.
- All 18 oracle text fixtures and six golden transforms added; plain/clang green: 39567 checks.
- SAN with detect_leaks=1: exit 1, LeakSanitizer fatal ptrace error. SAN with detect_leaks=0: exit 0.
- INV before final predicate extensions: exit 0, 39597 checks, 10 expected SIGABRT children.
- Combined clang SAN+INV: exit 0, 39597 checks, 10 expected SIGABRT children.
- Driver green: 82 cases, 101468 expected lines, exit 0. Three new fixtures contribute 15 lines.
- Exports green: 537/537 declared functions exported, 0 missing, 0 undeclared, 0 variadic.
- Julia green: 20/20 tests, exit 0, system libgmp preload, one Julia/OpenBLAS thread.
- Named faults first run: 11 killed, 1 not compiled (unused h in wrong-denominator fault).
  Added (void)h only to that scratch fault; changed lost repetition to zero extension literally.
  Final plain and SAN+INV runs: all 12 compile, all 12 fail; details in fault-results.json.
- First mutation sweep: exit 1, baseline 19.4 s; 32 attempts in 719.9 s, 28 killed, 4 survived,
  0 compile failures, 0 timeouts. Three survivors exposed test gaps, one cleanup branch is uncovered.
- Reference-loader red: after asserting containment of each declared exact rational midpoint,
  `timeout 120 lanes/f4-slice1/build/test_ffun fourier` exited 134 at arb_contains_fmpq(a,q).
  The loader had overwritten its rational conversion error with the supplied radius.
  It now adds the supplied radius to that conversion error. No C library change was required.
- Added uncertain oracle certificates, exact-sum assertions, one-dimension identity assertions,
  all input/output INV argument positions, and 1000 observed init/clear allocations and frees.
  Generator: 22 function, 18 text, 1 sum, 4 cap records; 359328 bytes, exit 0.
  Plain green: 57439 checks, exit 0. Combined SAN+INV green: 57490 checks, exit 0.
- SAN+INV scratch verification of all original faults and the three test-gap survivors:
  15 compile, all 15 fail. Actual assertion/INV diagnostics are retained in fault-results.json.
- Follow-up sweep: same seed, first 12 selected mutants; exit 0, baseline 37.5 s;
  12 attempts in 296.2 s, 12 killed, 0 survived, 0 compile failures, 0 timeouts.
  Across both sweeps: 44 attempts, 32 distinct mutations, 1016.1 s of sweep execution.
- Cap-phase probe: compile exit 0; run exit 0, setter OK, Fourier NOT_DETERMINED,
  destination unchanged=1, CPU time 0.020 s. Added distinct-output and self-alias failure snapshots.
- Final tests: plain/SAN/clang 57465 checks; INV and clang SAN+INV 57516 checks, 17 expected abort children.
  All exit 0. Named SAN+INV faults now total 16; all compile and all return SIGABRT.
- Final driver rerun: exit 0, 82 cases and 101468 expected lines. Julia rerun: exit 0, 20/20 tests.
  Direct shell Fourier delta call: exit 0, hand-derived [1,-i,-1,i] at layout (1,4).
