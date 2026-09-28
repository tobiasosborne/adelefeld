# Lane m1-closure-arith report

CLOSED 2; CLOSED WITH EDIT 0; OPEN 2; SETTLED BY DECISION 2. NO BLOCKER OPEN.

## Work done

I judged R1 to R6 against the rebuilt library, the accepted M1-D3 and M1-D4 decisions,
the headers, conventions, the original probes, and new neighbouring inputs. R1 and R2
are SETTLED BY DECISION. R3 and R6 are CLOSED. R4 and R5 remain OPEN while the
m1-repair-tools lane is unlanded. No new arithmetic finding was found. The evidence and
limits are in `docs/reviews/m1/arith/closure.md`.

## Files written

- `docs/reviews/m1/arith/closure.md`.
- `docs/reviews/m1/arith/closure-checks/boundary.c`.
- `docs/reviews/m1/arith/closure-checks/lowprec_neighbor.c`.
- `docs/reviews/m1/arith/closure-checks/swap_public.c`.
- `lanes/m1-closure-arith/report.md`.
- In this lane directory: `recon_huge_exp.out`, `recon_huge_exp_san.out`,
  `recon_huge_exp_san.err`, `recon_big_alloc_1.out`, `recon_big_alloc_2.out`,
  `recon_big_alloc_3.out`, `adele_prec1.out`, `swap_equiv.out`, `swap_public.out`,
  `fball_fuzz_check.out`, `rat_fuzz_check.out`, `recon_fuzz_check.out`,
  `adele_prec_check.out`, `stale_excuses.out`, `citations.out`,
  `probe_mul_2exp.out`, `boundary.out`, `boundary_san.out`, `boundary_san.err`,
  `lowprec_neighbor.out`, `lowprec_neighbor_san.out`, `lowprec_neighbor_san.err`,
  `make_check.out`, `test_recon_limit.out`, `test_adele_lowprec.out`,
  `recon_old_red.out`, and `adele_old_red.out`.
- Temporary scratch source and binaries in `/tmp/arith-closure-bin/`. No original
  reproducer or tracked source outside the owned paths was edited. The required
  `make` commands generated files under `build/`.

## Commands and results

All commands below ran from the worktree root. Paths to probes are under
`docs/reviews/m1/arith/`. The long `cc` commands used `-Iinclude`,
`build/libadelefeld.a -lflint -lgmp -lm` for the library build, or `src/*.c`
for the instrumented build. Each compiler used one process. No parallel build used
more than two jobs.

### Inspection and build

- `pwd; rg --files ...`: exit 0; located the required instructions, review, reports,
  specification, plan, and reproducer files.
- `cat` on `CLAUDE.md`, the three `COMMON.md` files, the review, both repair reports,
  and the lane brief: exit 0 for all eight files. `sed`, `rg`, `head`, and `nl` read the
  named source, test, header, reference, and decision sections, including SPEC 4.1,
  9.2, 15.2 and PLAN work packages 1.3 and 1.6; no inspected file
  was modified. One compound `rg` on `Makefile` returned 1 because its searched
  pattern was absent. Another `rg` of running build processes returned 1 for no match.
- `free -g` twice: 23 GB available each time. `ps -eo pid,comm,args | rg ...`
  found 0 concurrent build processes on both checks.
- `make clean && make -j2`: exit 0; 14 source objects and `build/libadelefeld.a` built.
- The shell `for` loop compiling the nine standalone old C probes to
  `/tmp/arith-closure-bin/`, plus `recon_fuzz.c` linked with `recon_mut111.c`:
  exit 0; 10 binaries. `recon_mut111.c` has no `main` and was used as a replacement
  reconstruction source.
- First `cc ... closure-checks/boundary.c ...`: exit 1; the new probe used a
  nonexistent `adf_rat_equal_si`. I changed it to `adf_rat_equal` and recompiled.

### Original reproducer runs

- `/tmp/arith-closure-bin/recon_huge_exp`: exit 0; 8 of 8 statuses LIMIT,
  8 outputs kept at -99. `ASAN_OPTIONS=detect_leaks=0` on its ASan and UBSan
  build: exit 0; 8 LIMIT statuses, 0 stderr bytes.
- `(ulimit -v 2000000; /tmp/arith-closure-bin/recon_big_alloc i)` for
  `i = 1, 2, 3`: each exit 0; each status LIMIT; no GMP allocation abort.
- `/tmp/arith-closure-bin/adele_prec1`: exit 0; prec 1, 2, 3 all have
  canonical real and complex quotients; at prec 1 the real result is exact 3.
- `/tmp/arith-closure-bin/swap_equiv 20000 1`: exit 0; 3856 `arb_mul`
  and 5536 `acb_mul` order differences; 0 add and finite-ball differences.
- The four unchanged fuzzers piped to their unchanged Python oracles at seed 7:
  `fball_fuzz 2000 7 20`: 2000 cases, 0 failures;
  `rat_fuzz 2000 7 64`: 2000 cases, 0 failures;
  `recon_fuzz 2000 7 20`: 4000 calls, 0 failures;
  `adele_prec 2000 7`: 4000 lines, 0 failures. Each pipeline exited 0.
- `recon_fuzz 2000 7 20` and `recon_mut111_fuzz 2000 7 20`, then `cmp -s`:
  both exit 0 and comparison exit 0, byte-identical. This old mutation probe
  checks the pre-repair mutant, not the landed reconstruction test.
- `python3 checks/stale_excuses.py`: exit 0; 39 entries, 36 stale, including
  all 18 original `fball.c` entries. `python3 checks/citations.py`: exit 0;
  printed the cited lines, which were compared with the source comments.
- `/tmp/arith-closure-bin/probe_mul_2exp`: exit 0; four shifts at and near
  2^62 and 2^63 yielded 1, 2, 1, 32 respectively. The reconstruction guard
  refuses those inputs before this shift is used.

### Repair tests and neighbouring inputs

- `make -j2 check`: exit 0; all 40 test programs passed.
- `build/test_recon_limit`: exit 0; 4 tests, 778 checks, 0 failures.
  `build/test_adele_lowprec`: exit 0; 14 tests, 1426 checks, 0 failures.
- `cc ... closure-checks/boundary.c ...` then
  `(ulimit -v 2000000; /tmp/arith-closure-bin/boundary)`: both exit 0;
  24 cases, 0 failures. Its ASan and UBSan build and run with
  `ASAN_OPTIONS=detect_leaks=0`: exit 0, 24 cases, 0 failures, 0 stderr bytes.
- `cc ... closure-checks/lowprec_neighbor.c ...` then
  `/tmp/arith-closure-bin/lowprec_neighbor`: both exit 0; 24 cases,
  0 failures. Its ASan and UBSan build and run with leak detection disabled:
  exit 0, 24 cases, 0 failures, 0 stderr bytes.
- `cc ... closure-checks/swap_public.c ...` then
  `/tmp/arith-closure-bin/swap_public`: both exit 0; 20000 pairs,
  5113 real and 7222 complex order differences.
- A scratch copy of the pre-repair `recon_mut111.c` was changed only to restore
  `fmpq_canonicalise`, then compiled with `tests/test_recon_limit.c` and all
  source files except `src/recon.c`. `ulimit -v 2000000; timeout 20` run:
  exit 134; four wrong LIMIT statuses before a GMP allocation abort.
- A scratch copy of `src/adele.c` had the M1-D4 clamp and the two exact-integer
  rational-division blocks reversed. It was compiled with unchanged
  `checks/adele_prec1.c` and all source files except `src/adele.c`.
  The run exited 0 and reported `is_canonical = 0` for both adele types at prec 1.
- Two first instrumented runs with `ASAN_OPTIONS=detect_leaks=1` exited 1 before
  the probes started: LeakSanitizer reported that it cannot work under ptrace.
  Both were rerun with `detect_leaks=0`, as above. No leak result is claimed.
- `rg` of old R6 phrases in the four source files: 0 matches. `rg` of stale
  entries: 18 `fball.c`, 14 `adele.c`, 4 `recon.c`.
- `python3` line-length checks of the closure and three C files: 0 lines over
  116 characters. A first `tail -3` command on a brace-expanded pair of files
  returned an option error; `rg` then read the two totals quoted above.
- `test -s` on `closure.md` and `report.md`: exit 0. The final line-length check
  found 0 lines over 116 characters in either file.

## Not done

- No mutation run. The m1-repair-tools lane is unlanded, and a full run exceeds
  the three-minute limit. R4 and R5 remain OPEN. The m1-invariants lane is also
  unlanded; its invariant checks were not run.
- No LeakSanitizer conclusion, no local-backend audit, and no broad complex
  enclosure fuzz. The new complex probe covers rational division only.
- No attempt to form huge endpoints after M1-D3 refuses them. The exact
  implementation of `arb_get_interval_fmpz_2exp` was not inspected.

## Sources pending

- The C implementation of FLINT 3.0.1 `arb_get_interval_fmpz_2exp` is not under
  `refs/`. A general bound on its endpoint size from the two stored exponents
  is [source pending: the FLINT implementation or a proof of that bound]. The
  closure makes no such claim. The warning quoted there is on disk at
  `refs/src/flint-3.0.1/arb.rst:468-477`.

## Findings against the specification

None. The open findings concern mutation excuses, not a mathematical rule in
`docs/SPEC.md`. M1-D3 and M1-D4 are stated consistently in the checked headers,
conventions, and code on the tested inputs.
