# Slice 4b red/green record

- `timeout 120 python3 -B lanes/f4-slice4/gen_vectors.py`: two Python type failures (Fraction conversion
  and multiplication), then two size refusals (2465618 and 510000 bytes) while removing repeated input arrays.
  Final exit 0: 16 inputs, 218 unary, 14 products, 48 ideles, 4 covariance, 5 caps; 392641 bytes.
- `timeout 120 python3 -B proto/functions4_checks.py`: exit 0, 2670 checks.
- Initial `timeout 180 make -s -j2 BUILD=lanes/f4-slice4/build
  lanes/f4-slice4/build/test_ffun_algebra`: two compile failures in the new test helpers.
  Fixed the FLINT 3.0.1 mag call and included ulong_extras.h. The next build gave the expected link failure
  for the six unimplemented functions. No library implementation existed yet.
- Red-phase stubs returned LIMIT. `timeout 30 lanes/f4-slice4/build/test_ffun_algebra mul`:
  exit 134 at the first mul OK assertion. Other groups use the same command with their group argument.
- Product first green attempt failed representation commutativity on an uncertain (2,3) cell.
  Deterministic stored-ball operand ordering fixed it. Mul green: exit 0, 135207 checks, 14 records.
- Translation red: exit 134 at the unary OK assertion. Translation green: 74908 checks, 87 records.
- Rational dilation red: exit 134. One build used a nonexistent fmpz_cmpabs_ui; corrected the bounds
  using signed comparisons and signed conversion only after proving a 2^20 bound.
  Dilation green: 256956 checks, 101 records. Covariance green: 7127 checks, 4 records.
- Idele red: exit 134 at the expected status comparison. Idele green: 125538 checks, 48 records.
- Reflection and conjugation red: each exit 134 at the unary OK assertion.
  Reflection green: 12093 checks, 15 records. Conjugation green: 12423 checks, 15 records.
- Initial full algebra green: 621759 checks, 289 records, 95120 cell comparisons; exit 0.
- Initial INV green: 621816 checks, with 19 expected SIGABRT children; exit 0.
- Driver red: `timeout 30 build/adf < tests/driver/ffun-algebra.cmd`: exit 1, 14 PARSE lines.
  Driver green after handlers: exit 1 as prescribed, 14/14 expected lines equal.
- Added two-pass translation work preflight, huge-coefficient product and cap/precision tests.
  Plain algebra: 621828 checks, exit 0. Slice-4a regression: 57465 checks, exit 0.
- Equal complex rectangles as aliased inputs versus distinct copies exposed a final-radius identity difference:
  mul red exit 134 at acb_equal. Copying one same-pointer operand avoids FLINT's squaring shortcut.
  Mul green: 135309 checks, 14 records, 7132 cell comparisons; exit 0.
- `ASAN_OPTIONS=detect_leaks=1 timeout 120 lanes/f4-slice4/san/test_ffun_algebra`: exit 1.
  LeakSanitizer reported its fatal ptrace incompatibility. No leak count is claimed.
- Driver acceptance: 87 cases, 101519 expected lines, exit 0. Julia: 38/38 tests, exit 0.
- Exports: 586/586 declared functions exported, 0 missing, 0 undeclared, 0 variadic; exit 0.
- Named faults plain: 14 compiled, 14 SIGABRT failures, 0 survivors; exit 0 from the harness.
- Four-cell guard allocation initially failed to compile because stdint.h was missing.
  The old binary accepted the unknown guard group and printed 438 checks; this is not a valid guard run.
  The accompanying scratch attempt had 16 compile failures and is not counted as killed faults.
  After including stdint.h, guard green: 461 checks, exit 0. Before-array mutant: sanitizer exit 1.
- Mutation entry-check survivor closed with operation-name diagnostics: scratch mutant SIGABRT,
  parent assertion failure at the diagnostic check. Identity skipping cell zero closed with unequal one-cell inputs.
- The lcm division-to-multiplication survivor exposed a missing overflow projection test.
  Exact product 274177*67280421310721 = 2^64+1 is checked with fmpz arithmetic.
  Original returns LIMIT before INV; the scratch mutant aborts at entry instead. Caps green: 589 checks.
- Exact integer-bit boundary cases and five explicit delta idele records were added.
  Final vectors: 16 inputs, 218 unary, 14 products, 53 ideles, 4 covariance, 5 caps; 394497 bytes.
- Final named SAN+INV fault run: 18 compiled, 18 failures, 0 survivors. Plain named run: 14/14 killed.
- Seven completed mutation runs: 30 attempts, 29 distinct, 21 killed, 6 survivors, 3 compile failures,
  0 timeouts, 538.8 s. The separate 24-request batch reached its outer 180 s timeout without final counts.
  Four observed test gaps are closed by named scratch regressions. Three redundant/equal-data cases remain listed.
- Final plain/SAN/clang: test_ffun 57465 checks, algebra 626743 checks, 294 records, 95393 cells; exits 0.
  Final INV and clang SAN+INV: test_ffun 57516 checks, algebra 626857 checks; exits 0.
  The former has 17 abort children; the latter has 19, including entry diagnostics.
- Final driver: 87 cases, 101519 expected lines, exit 0. Direct shell calls: 12/12, exit 0.
  Final exports: 586/586, 0 missing, 0 undeclared, 0 variadic, exit 0. Julia: 38/38, exit 0.
- Final leak-detection attempts for both C tests: each exit 1 with LeakSanitizer's ptrace fatal error.
  Address/undefined sanitizers with detect_leaks=0 pass. No leak count is claimed.
- Style audit: 9 source/statement/helper files, 0 lines above 116, 0 missing final newlines.
  All 16 input records are referenced; 310 total vector records. Authored logs: 37, largest 12999 bytes.
