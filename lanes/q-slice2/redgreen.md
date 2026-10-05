# Red and green

- A: `timeout 30 python3 lanes/q-slice2/gen_vectors.py`: 65 integer and 60 fractional vectors; 162442 bytes.
- B red: `timeout 180 make -j2 BUILD=lanes/q-slice2/build lanes/q-slice2/build/test_qclass_reduce`:
  exit 2, absent reduce declaration. Log red-b.log. This is the first test of the new file.
- B green: `timeout 60 lanes/q-slice2/build/test_qclass_reduce`: exit 0, 32276 checks, 65 vectors.
  Input mag conversion was corrected to exact field construction after an input identity assertion failed.
- D red: same binary, after adding printer test: exit 134, union printer returned NULL. Log red-printer.log.
- D green: same binary: exit 0, 32278 checks. Log green-printer.log.
- E red: `timeout 30 build/adf < tests/driver/qclass-reduce.cmd`: exit 1, qreduce produced PARSE.
  Expected lines preceded implementation. Log red-driver.log.
- E green: same command, exit 1 as required by error rows; byte diff exit 0. Log green-driver.log.
- C fractional red: `timeout 60 lanes/q-slice2/build/test_qclass_reduce fractional`: exit 134,
  expected OK failed on the first fractional vector. Log red-c.log.
- C fractional green: `timeout 60 lanes/q-slice2/build/test_qclass_reduce`: exit 0, 68872 checks.
- Spill extension: same binary, exit 0, 147397 checks, all 24 oracle PIECES inputs checked.
- Boundary red: the first bounds run aborted at the allowed negative midpoint exponent edge.
  The projected product check needlessly squared equal dyadic denominators.
- Boundary green: same binary, exit 0, 147481 checks after using the equal-denominator sum bound.
- Golden union extension: same binary, exit 0, 147555 checks, 7 exact golden text comparisons.
- Final ordinary reducer: `timeout 60 lanes/q-slice2/build/test_qclass_reduce`: exit 0, 147560 checks.
- Named faults: `timeout 170 python3 lanes/q-slice2/plant_faults.py`: exit 0; 11 of 11 compiled,
  each test process aborted (SIGABRT). O3 fails exact Q1 storage; O4 fails the zero-allocation assertion.
- SAN: both qclass binaries with ASAN_OPTIONS=detect_leaks=0 and timeout 60: exits 0,
  35384 and 147560 checks. LeakSanitizer cannot run in this sandbox.
- INV: both qclass binaries with timeout 60: exits 0, 35439 and 147563 checks.
- clang: both qclass binaries with timeout 60: exits 0, 35384 and 147560 checks.
- `timeout 1700 make -j2 check-all`: exit 0; 84 C programs, 70 driver cases, 101325 expected lines,
  500 exported functions, qclass Julia 30/30, both tool self-tests passed.
  Two later exact-work projection guards were checked by the final ordinary/SAN/INV/clang qclass runs.
- The first projected-bit allocation probe returned OK as a test (147557 checks), but it observed only
  FLINT allocation hooks, which do not measure GMP limb allocations. That probe was removed as evidence.
  The public projected-bit LIMIT/untouched test remains; no allocation claim is made for that test.
- Full-suite mutation attempt: SAN+INV baseline exceeded 170 s at test_resid_rest; exit 2, 0 judged mutants.
- Scoped mutation: 32 selected mutants, seed 310206, SAN+INV and the two qclass programs:
  exit 1; 22 killed, 7 survived, 3 did not compile, 0 timed out; 507.8 s.
- Survivor tests: exact denominator cap, pre-conversion bit count, and no fibre copy after budget exhaustion.
  The original reducer passes 147662 checks. Four formerly surviving faults now abort in scratch copies.
- Golden expansion: 12 union rows, preserving the seven original fixtures and adding five tighter lifts.
- Final named-fault run: `timeout 170 python3 lanes/q-slice2/plant_faults.py`: exit 0,
  15 compiled scratch faults, 15 SIGABRT test failures, including the four repaired survivors.
- Bounded 20-mutant follow-up: stopped by timeout after 410 s; exit 124, one active command killed,
  scratch removed. No aggregate count is inferred from the partial run.
- Four repaired survivors under CC=clang SAN=1 INV=1: compiled 4, failed 4 (SIGABRT), script exit 0.
- O1 under CC=clang SAN=1 INV=1: exit 1, AddressSanitizer heap-buffer-overflow; script exit 0.
- Printed-order control: removing the qclass printer's qsort compiles, then aborts at the order assertion.
  The fixture's first attempt used arb_set_d after assigning its radii, which cleared those radii;
  the fixture was corrected to assign its midpoint with arf_set_d. No production change was needed.
- Header red: standalone qclass.h probe fails to compile, exit 1, missing ADF_REAL_PREC_MAX.
- Header green: compile exit 0; `timeout 30 lanes/q-slice2/build/header_probe`: status 0, cap 2097152.
- Final vectors: 66 integer, 60 fractional, 26 spill, 243981 bytes, 6080 labels.
  Each case has 40 distinct rational representative tuples; glued tuples may denote the same class.
  Two isolated spill pieces make clipping lose points even without an adjacent covering piece.
  A new radius-carry fixture makes RU30 land on 1/2 from below.
- Final reducer runs: ordinary/SAN/clang exit 0, 150122 checks each; INV exit 0, 150125 checks.
- Final driver: 17 exact expected lines, exit 1 as specified; diff exit 0.
- Final exported symbols: 500/500 implemented and exported, 0 undeclared, 0 variadic; exit 0.
- Final qclass Julia: 30/30; exit 0 with the existing system-GMP preload workaround.
