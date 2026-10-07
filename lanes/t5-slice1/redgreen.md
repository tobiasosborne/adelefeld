# Slice 5a red/green log

1. Oracle: `timeout 120 python3 -B proto/tate_checks.py`: exit 0, 466 checks.
2. Vector generator first run: exit 1, used a nonexistent zeta oracle endpoint helper.
   Corrected to tate_checks.endpoints. Second run: exit 0, 394 rows, 1825 samples, 354837 bytes.
   Corrected LIMIT numeric code from 3 to 10 before any C run. Final size 354849 bytes.
3. Tests written before implementation. First build result follows below.
4. Initial C build: exit 2 (missing declaration and two test helper signatures).
   Corrected the helpers and added the header declaration. Rebuild: exit 2, undefined local Tate symbol.
   Commands: `timeout 180 make -s -j2 BUILD=lanes/t-slice1/build-plain
   lanes/t-slice1/build-plain/test_tate_local` (one shell line).
5. Implemented function. First run: 4 tests, 22630 checks, 59 failures. Found invalid fixtures:
   builtin Python round reduced pole centres to 53 bits; negative odd-prime powers are not dyadic;
   an example reference retained an imaginary component. Corrected without changing valid expectations.
   Run: `timeout 120 lanes/t-slice1/build-plain/test_tate_local`.
   Green: 4 tests, 22438 checks, 0 failures.
6. Added large odd exact pole -2^1000+3 at prec 53. Red: 5 tests, 22454 checks, 1 failure.
   Added pre-rounding odd-integer geometry. Green: 5 tests, 22454 checks, 0 failures.
7. Added driver fixture with hand-written expected lines. Red: `timeout 180 sh tests/test_driver.sh`,
   exit 1, seven PARSE lines instead of values/statuses. Implemented command. Corrected fixture character
   grammar from the design's shorthand s=z(0) to the actual value grammar s=(0)+(0)*i.
   Green: exit 0, 96 cases, 101633 expected lines.
8. Added closed endpoint, general-path precision-cap and first-refusal tests. Green: 6 tests,
   22539 checks, 0 failures. No code change needed.
9. Refined generator to include the irrational E(1/3) in an actual input ball. Added full-image upper
   bounds using scalar intervals and the real derivative bound, and shortened bounds with directed
   dyadic rounding. Final fixtures: 394 rows, 1861 certified samples, 325791 bytes. No expected status changed.
10. First mutation batch: 180 s timeout, baseline passed in 28.8 s. The run was interrupted before
    the 20 selected mutants finished. It reported three survivors and two compile refusals.
    Added exact integer s=+/-1048576, alpha=2, outside the optional power-recognition bound, to kill
    its false DOMAIN survivor. The guard comparator survivor is equivalent at equality.
    The inherited previous-derivative-bound survivor remains an explicit review finding.
11. Completed mutation batches with a prebuilt clang/SAN/INV dependency archive, recompiling
    localfactor.c and each selected test: seed 51001, 20 mutants in 90.5 s (12 killed, 6 survived,
    2 compile refusals); seed 51002, 20 in 80.5 s (11 killed, 5 survived, 4 compile refusals).
    Added odd s=0, an exact power at the recognition boundary, and first-log precision instrumentation.
    Targeted replay kills all three exposed gaps, and also the earlier false DOMAIN gap.
12. Scratch formula/status faults: 14 compiled, 14 killed. See faults.tsv for every failure count.
13. Inherited Gamma-bound differential scans: 755 accepted boxes and 6795 certified exact-point samples
    per mutant, 0 missed samples. This is bounded differential testing, not an equivalence proof.
14. Valgrind exposed undefined inactive arf bytes in raw byte snapshots. Zero-initialize test storage
    before acb_init, keeping every byte-preservation assertion. Final verification follows in report.md.
15. Final Valgrind run after defining inactive bytes and clearing FLINT process caches:
    6 tests, 22747 checks, 0 failures; 638886 allocations and 638886 frees;
    0 bytes in 0 blocks at exit; 0 memory errors. Earlier runs exposed snapshot padding,
    with 804, 220, then 2 errors, before the complete initialization fix.
16. Final matrix: plain/SAN/clang Tate 6 tests, 22747 checks, 0 failures;
    INV Tate 7 tests, 22762 checks, 0 failures. Existing local-factor tests:
    plain/SAN/clang 13 tests, 306730 checks; INV 14 tests, 306749 checks; all 0 failures.
    LeakSanitizer cannot run under ptrace. ASan/UBSan runs with detect_leaks=0 passed.
17. Final fixtures: 394 rows, 1861 samples, 325947 bytes. Exact rational/cyclotomic annotations added;
    sample values and expected statuses unchanged. Driver: 96 cases, 101633 lines, exit 0.
    Julia: 23 checks, 23 passes using the system GMP preload. Direct CLI example: exit 0, encloses 4/3.
