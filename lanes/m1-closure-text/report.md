# Lane m1-closure-text: report

8 CLOSED, 0 CLOSED WITH EDIT, 0 OPEN, 2 SETTLED BY DECISION. NO BLOCKER OPEN.

## Work done

Read the lane rules, text review, repair reports, SPEC 15, PLAN 1.4, headers, implementation, and tests.
Reran the old reproducers on a fresh normal build and the lifetime reproducer on a fresh `INV=1` build.
Added neighboring checks for printing, decimal bounds, cap aliasing, predicates, exports, fuzz controls,
borrow counts, and the two proposed decisions. Wrote `docs/reviews/m1/text/closure.md`. No accepted SPEC
clause was refuted. M1-D10 and M1-D11 remain proposed; the checks found no input contradicting them.

## Files written

- `docs/reviews/m1/text/closure.md`
- `docs/reviews/m1/text/closure-checks/probe_closure.c`
- `docs/reviews/m1/text/closure-checks/boundaries_closure.py`
- `docs/reviews/m1/text/closure-checks/headers_closure.py`
- `docs/reviews/m1/text/closure-checks/fuzz_controls.c`
- `docs/reviews/m1/text/closure-checks/lifetime_closure.c`
- `lanes/m1-closure-text/report.md`

The old `checks/headers.py` regenerated `checks/declarations.c` and `checks/missing_link.c` with the same
source text. Its object, binary and aux file, the temporary symlinks, Python bytecode, and `build-closure/`
were removed. Five source programs remain in `closure-checks/`; zero binaries remain there or in `checks/`.
Command logs were written under `/tmp/m1-closure-text-*.log` and `/tmp/m1-closure-text-*.out`/`.err`.

## Checks run

All commands below ran from the repository root. Long compiler commands are shown by their source, output
and libraries; the flags were `-std=c11 -O2 -g -Iinclude`, with `-Wall -Wextra -Wpedantic -Werror` for new
closure checks. The invariant links also used `-DADF_CHECK_INVARIANTS -pthread`.

| Command | Result |
|---|---|
| `free -g` before each build | 23 GB available; two cores used. |
| `ps -eo args \| rg '(^\|[ /])(make\|cc\|gcc\|clang)( \|$)'` | 0 other builds shown. |
| `make clean BUILD=build-closure && make -j2 check BUILD=build-closure` | Exit 0; 42 programs. |
| `make -j2 BUILD=build-closure/pic CFLAGS='... -fPIC' all` | Exit 0; 14 objects. |
| `cc ... checks/bridge.c ... -o build-closure/bridge.so` | Exit 0; 555040 bytes. |
| `cc ... checks/probe.c ... -o build-closure/probe` | Exit 0; 530408 bytes. |
| `python3 docs/reviews/m1/text/checks/resources.py` | Exit 0; 17 rows. |
| `build-closure/probe local` | Five cap statuses 0; scaled status 0, lost 0. |
| `python3 docs/reviews/m1/text/checks/headers.py` | Exit 1 at old assertion; 193/193 exports, link 0. |
| `python3 docs/reviews/m1/text/checks/boundaries.py` | Exit 1 at old assertion; C status 0. |
| First `cc ... checks/sentinel_padding.c ...` | Link exit 1; omitted support objects. |
| Corrected `cc ... checks/{sentinel_padding,fuzz_one}.c ...` | Both link exit 0. |
| `valgrind -q --error-exitcode=97 --track-origins=yes build-closure/sentinel_padding` | Exit 0; 0 reports. |
| Same Valgrind command on `build-closure/fuzz_one` | Exit 0; 0 reports. |
| `python3 docs/reviews/m1/text/closure-checks/boundaries_closure.py` | Exit 0; 11 cases. |
| `cc ... closure-checks/probe_closure.c ... && build-closure/probe_closure` | Exit 0; 64/64 checks. |
| Same compilation and run with `INV=1` library | Exit 0; 64/64 checks. |
| Scratch `src/text.c` with old 18-digit guard, then `test_text_limits_red` | Exit 1; 6 failed checks, 3 tests. |
| `make clean BUILD=build-closure/inv && make -j2 check INV=1 BUILD=build-closure/inv` | Exit 0; 42 programs. |
| `build-closure/probe-inv lifetime` | Exit 134; abort says 1 live borrower. |
| First `cc ... closure-checks/lifetime_closure.c ...` | Compile exit 1; ignored `freopen` result. |
| Corrected compile and `build-closure/lifetime_closure` | Exit 0; 7/7 child cases. |
| `cc ... closure-checks/fuzz_controls.c ... && build-closure/fuzz_controls` | Exit 0; 4 inputs. |
| `python3 docs/reviews/m1/text/closure-checks/headers_closure.py` | Exit 0; 193/193; 0 missing. |
| `valgrind -q --error-exitcode=97 build-closure/probe_closure` | Exit 0; 0 reports; 64 checks. |
| Valgrind full `build-closure/test_text_adele` | Exit 0; 20 tests, 84390 checks, 21 possible-loss records. |
| `PYTHONDONTWRITEBYTECODE=1 python3 proto/test_text_grammar.py` | Exit 0; 35 tests. |
| `awk 'length($0)>116 {...}'` on new sources and closure | Exit 0; 0 long lines. |
| Cleanup by `Path.unlink` and `shutil.rmtree` | Exit 0; 0 build directories or generated binaries remain. |

The first scratch red compile also failed once because it lacked `-Isrc` for `invariants.h`; the corrected
compile linked and ran. The first lifetime check compile failed with `-Werror=unused-result`; the source
was corrected and all seven cases passed. Both failed old Python scripts contain assertions for the
previous defects, so their exit 1 is evidence that those expectations became obsolete.

The abbreviated compiler rows above used these commands. Each successful `cc` command exited 0; the
two first attempts noted above exited 1.

```sh
make -j2 BUILD=build-closure/pic \
  CFLAGS='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fPIC' all
cc -std=c11 -O2 -g -Wall -Wextra -Werror -fPIC -shared -Iinclude \
  docs/reviews/m1/text/checks/bridge.c -Wl,--whole-archive build-closure/pic/libadelefeld.a \
  -Wl,--no-whole-archive -lflint -lgmp -lm -o build-closure/bridge.so
cc -std=c11 -O2 -g -Iinclude docs/reviews/m1/text/checks/probe.c \
  build-closure/libadelefeld.a -lflint -lgmp -lm -o build-closure/probe
cc -std=c11 -O0 -g -Iinclude -Itests docs/reviews/m1/text/checks/sentinel_padding.c \
  build-closure/libadelefeld.a -lflint -lgmp -lm -o build-closure/sentinel_padding
cc -std=c11 -O0 -g -Iinclude -Itests docs/reviews/m1/text/checks/sentinel_padding.c \
  build-closure/support/*.o build-closure/libadelefeld.a -lflint -lgmp -lm \
  -o build-closure/sentinel_padding
cc -std=c11 -O0 -g -Iinclude -Itests docs/reviews/m1/text/checks/fuzz_one.c \
  build-closure/support/*.o build-closure/libadelefeld.a -lflint -lgmp -lm -o build-closure/fuzz_one
cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude \
  docs/reviews/m1/text/closure-checks/probe_closure.c build-closure/libadelefeld.a \
  -lflint -lgmp -lm -o build-closure/probe_closure
cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS -Iinclude \
  docs/reviews/m1/text/closure-checks/probe_closure.c build-closure/inv/libadelefeld.a \
  -lflint -lgmp -lm -pthread -o build-closure/probe_closure_inv
cc -std=c11 -O2 -g -DADF_CHECK_INVARIANTS -Iinclude docs/reviews/m1/text/checks/probe.c \
  build-closure/inv/libadelefeld.a -lflint -lgmp -lm -pthread -o build-closure/probe-inv
cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS -Iinclude \
  docs/reviews/m1/text/closure-checks/lifetime_closure.c build-closure/inv/libadelefeld.a \
  -lflint -lgmp -lm -pthread -o build-closure/lifetime_closure
cc -std=c11 -O2 -g -Iinclude -Itests docs/reviews/m1/text/closure-checks/fuzz_controls.c \
  build-closure/libadelefeld.a -lflint -lgmp -lm -o build-closure/fuzz_controls
```

For the red check, a Python standard-library script copied `src/text.c` to `build-closure/text-red.c`
and inserted `if (n->ee - n->eb > 18) return 1;` in `tx_dec_over`. The compile and link commands were:

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Isrc \
  -c build-closure/text-red.c -o build-closure/text-red.o
cp build-closure/libadelefeld.a build-closure/lib-red.a
ar d build-closure/lib-red.a text.o
ar rcs build-closure/lib-red.a build-closure/text-red.o
cc -std=c11 -O2 -g -Iinclude -Itests tests/test_text_limits.c build-closure/support/*.o \
  build-closure/lib-red.a -lflint -lgmp -lm -o build-closure/test_text_limits_red
build-closure/test_text_limits_red
```

The first red compile without `-Isrc` exited 1; the commands above exited 0 until the final program,
which exited 1 with six failed checks in three tests, as intended.

## Not examined

No full independent cap oracle over random contexts, no exhaustive dump grammar run, no long fuzz run, no
sanitizer build, and no broad mutation run. Only one old hunk was restored in a scratch build to check red
test sensitivity. The accepted repair tests were inspected and run; their old-code red logs were read but
not all recreated. Acceptance of M1-D10 and M1-D11 and document edits for M1-D10 are outside this lane.

## Sources pending

- [source pending: exactness of FLINT 3.0.1 `mag_set_ui_2exp_si` for every representable 30-bit
  mantissa]. Carried from the review; not needed for these verdicts.
- [source pending: complete FLINT allocator exhaustion behavior]. Carried from the review; not needed here.

## Findings against the specification

None against an accepted clause of `docs/SPEC.md`. Proposed M1-D10 describes the observed blind spot for
manually assembled canonical values, but `conventions.md:323-328` and `modctx.h:16-19` still state the borrow
count without its proposed library-only qualification. The decision and document edits remain pending.
