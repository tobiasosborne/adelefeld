# Report of lane m1-invariants (adf-xk4): the debug build -DADF_CHECK_INVARIANTS

Written by the lane (Claude Sonnet 5.5, effort medium) as its final message; saved to this file by the
orchestrator, who added the last section. The lane was cut off once by a network failure and resumed.

Status: finished. Five existing test programs abort or segfault under `INV=1` because they step outside the
contract. They are listed below and not edited.

## What was done

1. `lanes/m1-invariants/functions.tsv` has 193 rows, one per exported function of `tests/test_exports.sh`.
   - 105 rows have checked inputs; 88 have none.
   - init, clear and the `is_canonical` predicates are EXEMPT, with header lines.
   - Raw setters, output-only writers, text readers and functions without typed arguments are N/A, with the
     reason.
   - Columns are as the brief asks, plus borrow begins and borrow ends.
   - `adf_fball_canonicalise` is checked only for a local x. fball.h:138-141 grants raw global input;
     fball.h:141-143 requires L for local input.
2. Red: `tests/test_invariants.c` and `tests/test_invariants_lifetime.c` were run with `make INV=1` before any
   code in `src/`.
   - `red-invariants.log`: 1432 break runs. 86 of them were aborted by FLINT (d=0) without the function name.
     7076 checks failed. The R3 case fails.
   - `red-lifetime.log`: 64 failed checks, 10 failed tests.
3. Green:
   - `src/invariants.h` holds the macros. They expand to `((void) 0)` without the flag.
   - The macros are placed in 10 files of `src/`.
   - The count field, `adf_inv_borrow`/`adf_inv_release` and the free check are in `src/modctx.c`, inside
     `#ifdef`.
   - `Makefile`: variable `INV` only. It adds `-DADF_CHECK_INVARIANTS` and `-pthread`.
   - `tests/README.md`: one paragraph.
   - `green-invariants.log`: 9 tests, 1432 of 1432 break runs ended by SIGABRT.
   - The abort line is `adelefeld: ADF_CHECK_INVARIANTS: <function>: argument <arg> is not a canonical <type>`.
   - `green-lifetime.log` is green as well.
4. Borrow count:
   - It is an `atomic_long` in `adf_modctx_struct`.
   - `ADF_INV_RETARGET(field, ctx)` takes the new borrow, then releases the old one, before `field = ctx`. It
     sits at every write of `mctx` in fball.c, fball_local.c, scaled.c and dump.c.
   - `ADF_INV_RELEASE` is in `adf_fball_clear` and `adf_scaled_clear`, and wherever a global value overwrites
     a local one.
   - `ADF_INV_BORROW` is in `adf_scaled_init`.
   - `adf_modctx_free` aborts with one line if the count is not zero.
   - A release never takes the count below zero (see findings).

## Tests written

- `tests/test_invariants.c` (9 tests, 5376 checks):
  - It reads functions.tsv and requires the table and the list to agree in both directions.
  - For each checked argument of each of the 105 functions, and each break of its type, a forked child must
    die by SIGABRT with the exact line.
  - Breaks per type: rat 5, fball 14 (7 global, 7 local), scaled 9, adele 7, cadele 8.
  - Canonical inputs run in 4 modes: global balls; local balls; outputs that start local in a second context;
    local inputs with such outputs.
  - Each canonical run must return, write nothing to stderr, and then free both contexts. That shows every
    function leaves the count right.
  - Raw setters and a global `canonicalise` are accepted. Predicates never abort. The finding R3 reproducer is
    included.
- `tests/test_invariants_lifetime.c` (13 tests, 116 checks):
  - Free with one live borrower of each kind aborts: fball local, scaled, adele fin, cadele fin, and the R7
    reproducer.
  - Free after clear returns.
  - Covered scenarios: set, swap, swap with a global, set_context, set_global and global-to-local, overwrite
    by another context, overwrite by arithmetic, constructors, loaders, parsers, `set_fball` into another
    context, and aliased calls (`y = x`).
  - Threads: 2 x 100000 scaled init/clear and 2 x 100000 fball copies, then free returns. With one borrower
    held, free aborts (exact count).
- Without the flag both programs print "skipped" and pass (seen in `check-release.log`).
- Children set `PR_SET_DUMPABLE 0`. With systemd-coredump each abort otherwise cost 0.15 s.

## Checks run

All ran from the worktree root, one build at a time, with 23 GB available before each.

| command | result |
|---|---|
| `make clean && make -j2 check` | passed, 42 programs, 21.2 s (`check-release.log`) |
| `make clean && make -j2 check SAN=1` | passed, 42 programs, 35.2 s (`check-san.log`) |
| `make clean && make -j2 check CC=clang` | passed, 42 programs, 20.0 s (`check-clang.log`) |
| `make clean && make -j2 check INV=1` | FAILED: 5 of 43 programs abort or segfault (listed below); the 2 new programs pass; 24.3 s (`check-inv.log`) |
| `make clean && make -j2 check INV=1 SAN=1` | the same 5 programs fail (2 ASan SEGV, 3 abort); the other 37 pass, the 2 new ones included (`check-inv-san.log`) |
| `sh tests/test_exports.sh` | passed: 193 of 193 exported, no undeclared export, no variadic |
| release object comparison | all 14 objects identical (below) |
| final `make clean && make -j2 check` after all edits | passed, 42 programs |

Release object comparison:
- Command: `sh lanes/m1-invariants/compare-release.sh build-base/tree/out build`, which compares `objdump -d`
  without `-g`.
- Baseline objects come from `git archive 4fbb142 src include Makefile` built with `make all BUILD=out`.
- Lines of disassembly compared: adele 1326, cap 327, common 119, dump 5026, fball 4075, fball_local 604,
  inlines 161, modctx 1222, place 67, rat 376, recon 498, scaled 2053, status 0, text 4853.
- Result in `compare-release.log`.

`make bench` was not run. The mutation tool was not run.

## Cost

- `make check` took 21.2 s (14 s of it build); `make check INV=1` took 24.3 s.
- This is not a like-for-like number. Under INV=1 the five programs below abort early, and about 4 s belong
  to the two new test programs.
- Per call, one predicate runs per checked input (a gcd for a global ball). Nested public calls check again,
  for example `adf_adele_add` checks x and y, then `adf_fball_add` checks the fins.

## Mutation (step 5, second way)

- `lanes/m1-invariants/hand-mutation.py` uses seed 20260928.
  - It lists 160 entry-check lines and 27 count sites.
  - It draws 10 checks and 8 count sites.
  - For each, in turn, it deletes the line, rebuilds with INV=1, runs both programs and restores the file.
- Final run (`hand-mutation.log`): 18 mutants, 18 killed, baseline green before and after.
- The first valid run had 2 survivors: scaled.c:207 and :296 (`set_fball` and `set_context` into another
  context).
  - Outputs that start in a second context were added to `test_invariants.c`.
  - A `set_fball` scenario was added to the lifetime test.
  - Both sites are now killed.
- An even earlier run was invalid: a restored file kept an old mtime, so stale mutant objects stayed linked.
  Fixed with `os.utime`. The shown log is from the fixed script.
- `tools/mutate/mutate.py` itself was not run.

## Tests that step outside the contract under INV=1 (not edited)

Found with a scan build (checks print and continue; patched copy in `build-scan/`, its build output removed)
plus gdb backtraces. All other programs are clean.

- `tests/test_dump_ctx.c`:
  - The helpers `sc_init`/`sc_clear` (lines 168-181) write `mctx` by hand and never release. They are used at
    424, 481, 555, 724, 1024, 1274 and 1437.
  - Sentinel context pointers 0x10 (line 481) and 0x30 (line 1024) sit in a scaled value. A successful load
    then releases the old pointer, which dereferences a non-object and segfaults. Contract: M1-D2 (pointer
    fields are NULL or live objects).
  - A load into such a hand-built value counts a borrow that `sc_clear` never returns, so the later
    `adf_modctx_free` aborts. This happens at 539/541, 731, 1078 (100 times, via `typed_check`) and 1164.
    Contract: 4.6 (values refer to contexts only through the library).
- `tests/test_dump_golden.c`:
  - Line 139 puts sentinel `0x40` in a hand-built scaled value (segfault at load).
  - The free at 197 (via `typed_row`, 147/254) runs while a loaded value is still counted (hand clear). Same
    contracts.
- `tests/test_fball_local.c`:
  - Line 415: `adf_modctx_free(c3)` while `y`, a local value of c3, is alive. The abort is right (4.6).
  - Line 1650 (`local_output_of_another_context_is_resized`): `mklocal_si(z, c1, ...)` overwrites the context
    field of a live counted value by hand, in a loop. The count stays too high and the frees abort.
- `tests/test_fball.c:1100` (`identical_local_guard`):
  - `adf_fball_identical` is called on a value that is not canonical (`make_local_shaped`, context field
    copied by hand).
  - identical is not exempt by its header (fball.h:100-104).
- `tests/test_recon.c:1313` (`an_adele_with_an_infinite_real_ball_is_rejected`):
  - `adf_adele_reconstruct` is called on a non-canonical adele (infinite real part).
  - recon.h does not exempt it, and the adele predicate requires a finite arb.

## Questions

**Moves the count cannot see**
- `memcpy` or struct assignment of a value: the copy is uncounted. If both are cleared, the second release is
  clamped at 0. That is a missed detection, not an abort.
- Writing `mctx` by hand: the value is uncounted. If the write overwrites a counted value, the count stays
  too high and free gives a false "still borrowed" abort (seen at test_fball_local.c:1650).
- Struct assignment over a live counted value: same false abort.
- Library code does none of these. The only struct copy in `src/` is the parser-internal `tx_fin` at
  text.c:490. `swap` exchanges fields, so the multiset of borrows is kept. `dp_set_fb` writes a temporary and
  uses RETARGET.

**Inline functions (`src/inlines.c`)**
- None takes a value with a predicate: adf_sizeof_*, adf_alignof_*, adf_status_str,
  adf_text_limits_default.
- There is nothing to check, so no HEADER-FINDING is needed. All 18 are N/A rows in the tsv.

**Cost**
- See the Cost section above.

## Findings

- HEADER-FINDING (contract gap):
  - A local `adf_fball` or `adf_scaled` is a public struct, so a caller or test can build one by writing
    `mctx`. The tests do (`mklocal_*`, `sc_init`, `make_local_shaped`).
  - conventions 4.6 promises a count of "the values that refer to it". That holds only for values made
    through the library.
  - The lane chose that a release never underflows. An abort at `adf_fball_clear` of a canonical hand-built
    value would change behaviour on canonical inputs.
  - The header should say that local values must come from `adf_fball_set_local` or the loaders, or offer a
    raw local setter that counts.
- Header/test conflict on reconstruct:
  - recon.h does not exempt non-canonical input for `adf_adele_reconstruct`, yet test_recon.c:1313 expects a
    status for an infinite real ball.
  - Under the flag the abort wins. The orchestrator must decide.
- Review R4:
  - The fixture in test_fball.c is an invalid input to `adf_fball_canonicalise` (a local value that does not
    satisfy L).
  - With the flag such a fixture aborts. It was not reached, because the suite aborts earlier at :1100.
- gcc 13 gives a false `-Wstringop-overread` in `adf_cadele_mul` when the predicate is inlined into the
  check. It was avoided by making each per-type check a non-inlined static function (noted in invariants.h).

## Not done / pending

- `make bench` was not run (not asked). `make mutate` was forbidden in this run. No fuzzing was done, since
  nothing parses new input.
- The five programs above cannot run to the end under INV=1. Their later tests ran only in the scan build
  (checks print and continue), where nothing else failed. That patched copy is not kept.
- Sources pending: none. `flint_abort` is at flint.h:219 of the installed FLINT header, not under refs/.

## Files

Written or changed:
- `src/invariants.h` (new)
- `src/{adele,cap,dump,fball,fball_local,modctx,rat,recon,scaled,text}.c`
- `Makefile` (INV only)
- `tests/test_invariants.c`, `tests/test_invariants_lifetime.c`, `tests/README.md`
- `lanes/m1-invariants/{functions.tsv, compare-release.sh, hand-mutation.py, *.log}`

## Checks of the orchestrator before the merge (2026-09-28, 23:57 to 00:05)

- `make clean && make -j2 check` in the lane's worktree: 42 test programs pass.
- Release objects: a baseline built by the orchestrator from `git archive 4fbb142` in a scratch directory,
  compared with `objdump -d`: all 14 objects are identical to those of the lane's tree.
- `make clean && make -j2 -k check INV=1`: 37 programs pass, 3 abort, 2 end with a segmentation fault, as the
  lane says. The two new programs pass.
- Not repeated by the orchestrator: the runs with `SAN=1` and `CC=clang` in the worktree (they are run on
  master after the merge), the hand mutation, the red runs.
- The five test programs that fail under the flag and the two findings on the headers are open issues; the
  flag is not part of `make check`.
