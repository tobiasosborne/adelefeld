# Red and green

- Reference baseline: `timeout 180 python3 -B proto/test_text_grammar.py`: 35 tests, 0 failures.
- A: `timeout 60 python3 -B lanes/f4-slice7/gen_vectors.py`: 53 rows per type, 336991 bytes total.
- B red: `timeout 180 make -s -j2 BUILD=lanes/f4-slice7/plain` with both test targets:
  declarations absent, compilation failed. After adding declarations, both targets failed to link
  against the eight absent symbols (`red-link.log`). No implementation existed at this point.
- C green: both tests built. ffun: 17062 checks; rfun: 17222 checks; 0 failures each.
  Each includes 2000 random identity round trips and D1 cap - 1, cap, cap + 1.
  A handwritten rfun exponent fixture initially lacked one token (PARSE); it was corrected.
- D red: manually derived driver fixtures differ on 6 ffun and 7 rfun lines, all previously UNSUPPORTED.
  D green: both fixtures match byte for byte after typed dispatch was added (diff exit 0).
- INV red: the rfun load-abort test gave a truncated 12-byte span for its 13-byte zero dump.
  Corrected that fixture length; no library behavior was changed.
- Preflight strengthening: rejected complete cap+1 dumps now trap allocations >= 1 MiB.
  This catches construction before the limit check, while allowing validator integer temporaries.
- SAN red: LeakSanitizer failed under ptrace; UBSan also found NULL passed to a zero-length test memcpy.
  The test now skips that memcpy. SAN with detect_leaks=0 passed all 10 programs.
  FLINT allocation counters were added; each complete new test has 0 retained FLINT blocks.
- Mutation baseline red: linking an INV dump object to a SAN-only archive failed on adf_inv_borrow/release.
  The scratch base was rebuilt with SAN=1 INV=1 CC=clang. Baseline then passed both full tests.
- F green: all nine scratch faults build; every faulty test exits 1.
  Reordering, normalization, unsafe FLINT load, late count cap, count mismatch, missing Re(A) check,
  cap off by one, early output write, and omitted nctx write each have a distinct failing assertion.
- Driver acceptance: `timeout 180 env MAKEFLAGS=-j1 sh tests/test_driver.sh` in driverroot:
  93 cases, 101594 expected lines, 0 differences, exit 0.
- Julia: the initial run failed on the documented GMP symbol mismatch; system-GMP preload retry:
  18/18 checks passed, exit 0. The script covers both types and frees each returned string.
- Mutation sweep: 60 selected mutants, 853.1 seconds; 43 killed, 8 survived, 9 not compiled.
  Three survivors exposed missing inputs: a pure imaginary last coefficient, and two cursor-skip errors
  masked by a large first coefficient list. Added pure imaginary point/ball reference values and total
  coefficient counts split across two terms at 65535, 65536 and 65537.
- Retest: `timeout 180 python3 -B lanes/f4-slice7/retest_survivors.py`: three make exits 2, caused by
  executed test assertions (test exits 1). All three previously selected gap mutants are now killed.
  No additional mutation candidate was selected. Five documented survivors remain.
- Differential smoke: `timeout 180 python3 -B lanes/f4-slice7/differential.py`: 15000 texts,
  11250 edited inputs, 4556 accepted inspections/loads, 0 mismatches, exit 0.
