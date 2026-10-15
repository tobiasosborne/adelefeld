# Lane c-warn1 (bead adf-3rl): gcc 13.3.0 -O1 -Werror warnings in src/lpow.c and src/rfun.c

## Reproduction (before the change)
`timeout 300 gcc -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -c src/lpow.c`:
    src/lpow.c:259:10: error: 'k' may be used uninitialized [-Werror=maybe-uninitialized]   (via_pow_si inlined into adf_lball_powunit at :307; k declared at :280)
`... -c src/rfun.c`:
    src/rfun.c:339:29: error: 'rf_rat_ok' reading 16 bytes from a region of size 8 [-Werror=stringop-overread]   (adf_rfun_translate_rat)
    src/rfun.c:424:29: same (adf_rfun_dilate_rat)

## Why gcc warns
- lpow.c: `k` is written only inside integer_exponent (`if (ok) *k = ...`, line 245) and read in via_pow_si only after `integer_exponent(&k, s)` returned nonzero (line 306). integer_exponent returns `ok`, and `*k` is set exactly when ok, so the read is never of an uninitialised value: the path gcc reports (returns 1 without setting k) is unreachable. gcc does not correlate the return value with the store after inlining.
- rfun.c: rf_rat_ok(const fmpq_t q) takes `const fmpq[1]` (16 bytes). The callers pass `q->q` / `h->q`, a real 16-byte fmpq inside adf_rat_struct (include/adelefeld/rat.h:31-34), so the read is in bounds; the "region of size 8" is a gcc analysis artefact (it only ever dereferences numerator/denominator, each 8 bytes, through fmpq_numref/denref). No path of real overread.

## Change (behaviour identical on every path)
src/lpow.c line 280: `w0 = 1, k;` -> `w0 = 1, k = 0;` (k is overwritten by integer_exponent before any read, so the value used is unchanged).
src/rfun.c line 144: `rf_rat_ok(const fmpq_t q)` -> `rf_rat_ok(const fmpz * num, const fmpz * den)`, body the same two tests on fmpz_bits(num) and fmpz_bits(den) (not weakened); callers at lines 339 and 424 pass `fmpq_numref(q->q), fmpq_denref(q->q)` and the same for `h->q`.
Diff: `git diff src` in the worktree (3 hunks in rfun.c, 1 in lpow.c; 6 changed lines in all).

## Checks
- Both gcc commands above, after the change: no output, clean.
- `timeout 600 make -j2` (-O2 -Werror): builds, exit 0.
- `timeout 600 make -j2 build/test_lpow build/test_rfun`; `build/test_lpow`: "12 tests, 3074149 checks, 0 failed checks, 0 failed tests", exit 0; `build/test_rfun`: "rfun: 64923 checks", exit 0 (this program prints no failure line; its exit status is the verdict).
- `timeout 900 bash tools/memcheck/run.sh` (note: the script needs bash; `sh` gives a syntax error at line 109). The -O1 -Werror build step now completes for the whole library and all test programs (lpow.o and rfun.o compile in the log). The whole run took 11m11s (over the 10 min bound; the script's own 120 s per-program limit applied): 34 PASS, a number of TIMEOUT (e.g. test_lball, test_ffun, test_gfunc_log, test_dump_*) and many "ERROR ... exit 1" lines, including test_support, test_scaffold, test_lpow, test_rfun. I did not find the cause of the "exit 1" lines in the batch run. Run directly, with the script's flags (`valgrind --error-exitcode=9 --track-origins=yes --leak-check=full --errors-for-leak-kinds=definite,indirect -q`): build-memcheck/test_lpow rc=0 (only "possibly lost" GMP/FLINT blocks in lfunc.c paths, not an error kind), build-memcheck/test_rfun rc=0 (68 s), build-memcheck/test_support rc=0. Three tests (test_char, test_char_class, test_char_eval) report "Conditional jump depends on uninitialised value" in the batch run; they are in no file I touched and I did not look at them.

## Not done
- No change to the runner flags, tests or headers. No mutation testing (6-line warning repair; unchanged values). The cause of the "exit 1" and TIMEOUT lines of the batch memcheck run is not diagnosed; it is outside the two files and may be load/limit related or a separate issue for the orchestrator.
