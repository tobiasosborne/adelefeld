# Lane docs-julia: report

Worktree `/home/tobias/Projects/adelefeld-wt/docs-julia` (branch lane/docs-julia). No git command that changes
state was run, and no `bd`. No C code, Julia source or test was changed.

## What was done

A. **`julia/Adelefeld/README.md`** (new, 602 lines), the front page and tutorial of the package. It has these
sections:
- What the package is: a thin, safe binding; `docs/api-j.md:3`.
- The terms place, p-adic, Zhat, adele, ball, enclosure and status, each with a concrete number. They are cited
  from `refs/src/baker-padic/padicnotes.txt:760-763,1175-1180`, `refs/src/hertogh-thesis/thesis.txt:231-236`,
  `refs/src/milne-cft/CFT.txt:9372-9376`, SPEC 2, 4.1 and 4.3.
- The launcher and why it exists: the libgmp preload, `docs/api-j.md:80-87`, and the env of
  `ux/notebook/julia.sh`.
- A REPL tutorial in eight steps with real outputs:
  - rationals;
  - finite balls and the set predicates;
  - adeles, real balls and `parse`;
  - reconstruction, with the progression contract of `include/adelefeld/recon.h:8-12,15-16,49-57`;
  - statuses, `AdfResult`, `unwrap`, `AdfError`, with the table of `status.h:17-28`;
  - text and dumps, with a round trip and the lossy contrast;
  - `ModCtx`: the five families, `to_local`, `enclose_local`, `to_global`, `is_local`, `context`;
  - housekeeping: `close`, `finalize(ctx)`, `Adelefeld.diagnostics()`.
- The API table: all 63 exported names, one line each.
- What the package does not do yet, in the order of `docs/api-j.md` section 9.
- The notebook slice, as the way to reach the driver from Julia today.
- How to run the tests, with the Pkg.test summaries.
- A table of linked files.

  The five lines that the orchestrator verified were rerun, not pasted. All five match:
  - `Rat(6, 35) + Rat(1, 7)` gives `11/35`;
  - `FBall(Rat(146), Rat(1000))` gives `(* ; 146 mod 1000)`;
  - the reconstruction of `(1.5 +/- 0.5 ; 0 mod 2)` gives `2`;
  - `ModCtx(factorial = 6)` gives `ModCtx(modulus=720, blocks=[16, 9, 5])`;
  - `enclose_local(readvalue("1 mod 5"), ModCtx(blocks = [8, 9]))` gives `(* ; 0 mod 1)`.

B. **`docs/README.md`** (new, 126 lines), the reader's map of `docs/`. It has a table for each group:
- start here (GUIDE.md, the root README, the driver manual, the package README, the UX visions);
- the specification and its rules (SPEC, conventions, PLAN, PERF, seams, sources);
- the 24 `api-*.md` files by milestone, one line each, written from each file's first paragraph (and its section
  headings where the first paragraph names only a slice);
- the proofs by topic, the design notes, the reviews (one row per directory), the UX briefs;
- state and process (HANDOFF, worklog, workflow, lanes/COMMON.md).

## Files written

Pages: `julia/Adelefeld/README.md`, `docs/README.md`.

Lane files in `lanes/docs-julia/`:
- `tutorial.jl`: the eight steps as a REPL emulator. Each input is printed after `julia> ` and evaluated in
  `Main`; the output is shown as text/plain, and an error as `ERROR: ` plus `showerror`.
- `README.src.md`: the page source with block markers.
- `exports_table.md`: the 63 table rows.
- `fill_readme.py`: writes the README from the source, `tutorial.out`, the table and `pkgtest.log`.
- `check.py`: the second script. It runs the block diff, the Pkg.test lines, export coverage, links and widths.
- `exports.jl`: the exported names with the first line of each docstring.
- `make_numbers.py`: assembles `numbers.md`.
- `numbers.md`: every command and its verbatim output.
- Recorded outputs: `build.log`, `exports.log`, `pkgtest.log`, `tutorial.out`, `tutorial.rerun.out`,
  `noload.out`, `exports.out`, `check.out`.
- `depot/` and `tmp/`: the private Julia depot (2.9 MB) and TMPDIR. Do not commit them.

## Checks run (commands from the worktree root; full outputs in `lanes/docs-julia/numbers.md`)

1. `timeout 600 make -j2` -> `exit 0`. This built `build/libadelefeld.a` only, with no `.so`.
   `timeout 600 sh tests/test_exports.sh` -> exit 0, "passed: 625 of 625 declared functions are exported",
   and it built `build/libadelefeld.so`.
2. The tutorial was run through the launcher:
   `timeout 300 env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true
   JULIA_DEPOT_PATH="$PWD/lanes/docs-julia/depot:$HOME/.julia:" TMPDIR="$PWD/lanes/docs-julia/tmp"
   julia/Adelefeld/bin/adelefeld-julia --startup-file=no
   --project=julia/Adelefeld lanes/docs-julia/tutorial.jl > lanes/docs-julia/tutorial.out` -> exit 0, 240 lines,
   8 blocks, 75 inputs. The full output is pasted in `numbers.md` section 2. A second run into `tutorial.rerun.out`
   is byte-identical (`cmp` silent).
3. `python3 lanes/docs-julia/check.py lanes/docs-julia/tutorial.rerun.out` -> exit 0:
   - `1. REPL blocks: README 8, tutorial 8; 0 differ` (N = 8, M = 0).
   - `2. Pkg.test block: 13 lines, 0 not in pkgtest.log`.
   - `3. Exports: 63 names in the export lines (63 distinct), 63 rows in the API table; missing []; extra []`.
   - `4. Links: 90 relative links checked in 2 pages, 0 broken, 1 pending`. The pending link is `docs/README.md`
     -> `GUIDE.md`, which lane docs-guide writes. It is not in this worktree, so it resolves only after that
     lane's merge.
   - `5. Widths`: 0 lines over 116 characters in either page (prose, tables and code).
4. Pkg.test, run once: `timeout 900 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 JULIA_PKG_OFFLINE=true
   JULIA_DEPOT_PATH=... TMPDIR=... julia/Adelefeld/bin/adelefeld-julia --startup-file=no
   --project=julia/Adelefeld -e 'using Pkg; Pkg.test()'` -> exit 0, "Testing Adelefeld tests passed", 1m5s wall.
   - The step testsets pass J-a.1 904, J-a.2 1015, J-a.3 1649, J-a.4 1313 and J-b.1 488, which sum to
     **5369**, as expected.
   - The testset `C call counters` passes 43 more. It runs in a child Julia process against a fixture library
     (`runtests.jl:1732-1742`). In all, 5412 passed, 0 failed.
   - Pkg.test created `julia/Adelefeld/Manifest.toml`, which is gitignored (`.gitignore:36`). `git status` shows
     no tracked file changed.
5. Julia without the launcher: `julia --startup-file=no --project=julia/Adelefeld -e '... using Adelefeld ...'`.
   The load fails with an `InitError` around `AdfLoadError("dlopen (GMP symbol)", ...)` with the undefined
   symbol `__gmpn_modexact_1_odd`; the hint names the `LD_PRELOAD` command and the launcher. This is the
   README's claim, now checked (`noload.out`).
6. `exports.jl`: 63 exported names. 26 of them have no docstring of their own (list below); these are the 26 rows
   marked `*` in the API table.
7. `git status`: the only new paths are the two pages and `lanes/docs-julia/**`. `lanes/brief.md` and
   `lanes/DOCS-COMMON.md` were already there.

Every value in the README's prose was checked by hand against the outputs: 13 = 1 mod 6 = 4 mod 9; k = -1 for
52 mod 101; 720/5 = 144; 72/32 = 9/4, giving `1 mod 8`, which is computed and shown in step 7. Every line of
`numbers.md` that is over 116 characters (11 of them) is a verbatim output: the load error and the docstring
listing.

## Not done

- `docs/GUIDE.md` is linked but does not exist in this worktree (see check 3).
- The two pages were not rendered on GitHub. No browser is available (DOCS-COMMON D-DOC4).
- The notebook commands in the README (`sh ux/notebook/run.sh`, `sh ux/notebook/test.sh`) are quoted from
  `ux/notebook/README.md` and were not run; the page shows no output of them.
- `tests/test_julia.sh` was not run; only `Pkg.test`, once, as the brief says.

## Sources pending

None. No statement on the two pages is marked `[source pending]`.

## Findings against the specification

None.

## Findings against the lane instructions

- DOCS-COMMON says `timeout 600 make -j2 && timeout 600 make -C tools/adf` gives `build/libadelefeld.so`. It does
  not: `make -j2` builds `build/libadelefeld.a` (Makefile:81 `all: $(LIB)`), and `tools/adf/Makefile` builds
  only `build/adf`. The `.so` comes from `sh tests/test_exports.sh`, as `tests/test_julia.sh` and
  `ux/notebook/run.sh:8` also do. The package README says so.

## Library demands (D-DOC6; not built)

1. Two docstring problems in the package:
   - The docstring of `Rat` in `src/rat.jl` stands above `struct _Raw end`, so it documents the private
     `_Raw` and `?Rat` finds nothing.
   - 26 exported names have no docstring of their own: `Adele, AdfStatus, CAdele, PointComparison, Rat,
     TextKind, add, complexpart, divexact, dump_text, enclose_local, finitepart, load_dump, mul, neg, readvalue,
     realpart, reconstruct, sub, to_global, to_local, try_add, try_mul, try_neg, try_sub, value_text`.
     Some of these, for example `reconstruct` and `dump_text`, are documented only under their `try_` form.
   - The API table marks all such names with `*`.
2. `AdfLoadError` has no `showerror` method. A user who starts Julia without the launcher sees the raw struct,
   with its five fields in one line (`noload.out`), and not a readable message.
3. The REPL output of some values is hard to read:
   - `blocks(ctx)` returns `Vector{UInt64}`, which the REPL prints in hexadecimal (`0x0000000000000010`); the
     tutorial uses `Int.(blocks(ctx))`.
   - `AdfResult` has no `show` method, so the REPL prints `AdfResult{Rat}(Adelefeld.NOT_UNIT, nothing, nothing,
     NamedTuple())`.
   - The status values are not exported, so tuples print `Adelefeld.DOMAIN`.
4. `Rat(10)^2` returns `100` through Julia's generic `^` for `Number`, not through a method of the design. The
   balls refuse `^` explicitly. The tutorial does not show `^` on `Rat`.

## Time

About 1 h 40 min in the lane.
