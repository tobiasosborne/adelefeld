# Lane j-slice1: result

J-status and the first step of J-a of `docs/api-j.md` are built and their checks pass. The parts are the C
accessor `adf_status_count`, the package `julia/Adelefeld` (skeleton, load gates, statuses, `Rat`), the
launcher, the tests, and one block in `tests/test_julia.sh`.

## What was built

- Part 1 (C): `int adf_status_count(void)` in `include/adelefeld/status.h`, after `adf_status_str`. It is
  header-inline and exported, has a comment block in the same form, and cites `docs/api-j.md:72-76`. It returns
  `ADF_STATUS_COUNT`. Test `the_status_count_accessor_returns_the_macro` in `tests/test_status.c` has 4 checks:
  the value is 11, it equals the macro, the name at count - 1 is `LIMIT`, and the name at count is `UNKNOWN`.
- Part 2 (Julia), `julia/Adelefeld/`:
  - `Project.toml`: version 0.1.0, UUID f544f23f-32fa-4dee-a4d8-b10fa878c316, deps Libdl, test target Test.
  - `src/Adelefeld.jl`, `status.jl`, `load.jl`, `handle.jl`, `rat.jl`.
  - `bin/adelefeld-julia`: a launcher that puts the libgmp.so.10 from `ldconfig -p` into `LD_PRELOAD`, then
    runs `exec julia "$@"`.
  - `test/runtests.jl`, `test/load_fault.jl` (one subprocess per rejection case), `test/fixtures/fault.c`
    (fixture libraries, built by the tests with `cc`; the package never reads a test switch).
  - The load gates run in the order of `docs/api-j.md` section 2:
    1. words;
    2. library path (`ADELEFELD_LIB`, else `build/libadelefeld.so` from `@__DIR__`) and dlopen with
       RTLD_NOW | RTLD_LOCAL;
    3. every symbol from that one handle;
    4. `adf_version_check`;
    5. the literal layout rows, size and alignment;
    6. the 11 names, UNKNOWN at -1 and 11, and `adf_status_count() == 11 == length(instances(AdfStatus))`.

    Each failure throws `AdfLoadError` with the gate, the expected value, the observed value and the path. The
    missing GMP symbol gives the exact command `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia ...` and the
    launcher path. `require_loaded()` comes before every allocation.
  - `Rat`:
    - storage: aligned C heap storage with the alignment and containment proof in the docstring of
      `aligned_alloc`, checked at run time;
    - life cycle: init once, then the finalizer; the release claims LIVE with `@atomicreplace`, then runs the
      cached clear, then `Libc.free(base)`; repeated close and finalize do nothing; a closed handle gives
      `InvalidStateException` before any C call; `GC.@preserve` wraps every ccall;
    - constructors: `Integer` (`set_si` when the value fits Int64) and `Rational`/`(n, d)` (sign normalised,
      read through `set_str`); `parse` and `try_parse` use `TextLimits()` from `adf_text_limits_default`;
    - arithmetic: `+ - * /`, unary `-`, `inv`, and `==` between Rats and with Integer/Rational on either side;
      value-based `hash`; `copy` and `identical`;
    - text: `string`/`show` through `adf_rat_get_str` (the terminator is checked, `adf_str_free` runs in a
      `finally`);
    - results: `try_` methods return `AdfResult`; operators throw `AdfError`; `unwrap`; an unknown raw status
      gives `AdfAbiError`.
- `tests/test_julia.sh`: one block that runs `Pkg.test()` through the launcher with one thread and one precompile
  task, under `timeout 180`. It does not run when Julia is absent, because the script exits earlier in that
  case. The block sits after the last per-fixture block (qclass_text) and before the final smoke run. The
  script's earlier blocks all end in `exit`, so a block at the end of the file would never run.
- `docs/api-ja.md`: what was built, statements S1-S10 that the tests check, and 11 DESIGN-FINDINGs and decisions.

## Commands and results

| Command | Result |
|---|---|
| `make -s -j2 BUILD=lanes/j-slice1/b-plain .../test_status`, red | implicit declaration of `adf_status_count` |
| the same, after the header; `timeout 60 .../test_status` | 8 tests, 126 checks, 0 failed |
| fault: `return 10;` planted by sed | 4 failed checks, 1 failed test (all new checks); restored |
| `test_status` with `SAN=1`, `INV=1`, `CC=clang` (separate BUILD dirs) | each 8 tests, 126 checks, 0 failed |
| `timeout 600 sh tests/test_exports.sh` | passed: 625 of 625 exported, 0 missing, 0 extra, 0 variadic |
| gate (lanes/j-slice1/gate.sh: api-j section 9, depot and TMPDIR in the lane), red | 43 errors of 43 |
| gate, final (final.log) | Adelefeld step J-a.1: 904 passed of 904, "tests passed", exit 0 |
| session through the launcher without a preload set | `using Adelefeld; print(Rat(1)/3 + Rat(1,6))` prints `1/2` |
| `timeout 1200 env ... sh tests/test_julia.sh` (test_julia.log) | exit 0, 3 min 25 s, see below |

`tests/test_julia.sh` ran 40 fixture processes (44 fixture files: smoke.jl includes layouts, resid, linsolve
and roots) and the Pkg.test block (904 of 904).

The C part has one mutation fault, `return 10`, and the test catches it. Mutation testing of the accessor itself
was not run: it is one line, and that fault is its only meaningful mutant.

Julia faults (lanes/j-slice1/faults.py, faults.log). Each was planted in turn, the gate was run, and the file was
restored. 9 of 10 were caught:

| Fault | Result |
|---|---|
| F1 byte count by characters | 1 failed |
| F2 release without the claim (double clear) | test process aborted |
| F3 free the adjusted pointer | NOT caught |
| F4 ignore the status of div | 14 failed |
| F5 return the output on failure | 2 errored |
| F6 layout gate checks size only | 8 failed |
| F7 status gate skips the count | 7 failed |
| F8 negative denominator not normalized | 2 errored |
| F9 closed handle not checked | test process aborted |
| F10 status name gate skipped | 7 failed |

F3 cannot be caught on this machine. glibc malloc returns 16-aligned addresses and the rat alignment is 8, so
the adjusted pointer always equals the base. The proof in `aligned_alloc` covers this case.

Red and green runs are in `lanes/j-slice1/redgreen.md`. The first green attempt had 17 failures. One was a test
bug (a tuple compared with a vector). Sixteen were fixture cases: `--as-needed` dropped the fixture's dependency
on the library, and the subprocess printed the error in an old world age. Both were fixed in the tests.

## Findings against the design and the brief (details in docs/api-ja.md)

1. SPEC 4.1 has no rational arithmetic rows. The five rows are SPEC 4.3 (`docs/SPEC.md:242-248`), and they are
   FBall rows. This step checks only their rational centres and the congruence with the stated radius. The rows
   themselves belong to the FBall step.
2. The design does not define `//` for `Rat`; `/` is used for the session line.
3. A `text_limits` layout row (32/8, `docs/api-j.md:55`) and the Julia field offsets are gated in addition to
   `rat`, because the parsers pass this struct by pointer.
4. `==` with Integer/Rational scalars is implemented as the brief asks; the design states only `(Rat, Rat)`.
   A Julia Rational with zero denominator equals no Rat.
5. The Rat methods take no `prec` keyword and `try_value_text` takes no `digits` keyword. Passing either is an
   error, not an ignored value.
6. `AdfResult.where` is typed `Any` until `Place` lands in J-b.
7. The gate in the brief needs a trailing colon in `JULIA_DEPOT_PATH` (`...:$HOME/.julia:`). Without it, the
   first red run spent its 180 s recompiling Pkg and the standard libraries, and timed out (exit 124).
8. `Pkg.test` writes `julia/Adelefeld/Manifest.toml`. The lane deleted it; an ignore entry is for the
   orchestrator.
9. After a failed `using`, the module is not bound (observed with Julia 1.12.5). The error reaches the user
   wrapped in an `InitError`.

Findings against the specification: none.

## Not done

- Nothing beyond the step: no FBall, Adele, Place, contexts or carriers. There is no per-symbol inventory file;
  `REQUIRED` in `load.jl` lists the 24 symbols this step uses.
- No two-thread test. The design keeps execution to one thread.
- Fuzzing was not run. The 10^4-handle GC run and the 100-pair oracle are bounded smoke and differential checks.

## Sources pending

- glibc `dlsym` documentation of the search through a handle's dependencies. The fixture approach relies on it;
  it was observed working in all six fixture cases.
- Julia documentation is quoted from the installed help of Julia 1.12.5 in `lanes/j-slice1/help.log` (captured
  with `@doc`) and cited by line in `src/*.jl` and `docs/api-ja.md`.

## Files

Changed: `include/adelefeld/status.h` (+13 lines), `tests/test_status.c` (+10), `tests/test_julia.sh` (+12).
New: `docs/api-ja.md`, `julia/Adelefeld/**`, `lanes/j-slice1/{redgreen.md,result.md,gate.sh,faults.py,
faults.log,help.log,exports.log,red.log,green1-4.log,final.log,test_julia.log}`. Deleted at the end: `build/`,
the lane depot and tmp, the C build dirs, the fixture builds, `julia/Adelefeld/Manifest.toml`.
