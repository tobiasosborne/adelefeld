# Lane j-slice1: J-status and the first step of J-a of docs/api-j.md (the package skeleton, Rat, statuses)

Milestone J, the Julia binding (epic adf-c5e; design `docs/api-j.md`, lane d-julia, 445 lines, decisions J1 to
J8 recorded in its section 1). This lane builds, end to end, the two first items of its section 9: the C
prerequisite J-status, then the first step of J-a (skeleton, load gates, `Rat`, statuses). Nothing wider: no
`FBall`, no `Adele`, no text forms beyond what `Rat` needs. The design is the contract; read it whole first.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (for the C part), `docs/api-j.md` (all),
`docs/conventions.md` section 12 (lines 1553 onwards), `include/adelefeld/{common,status,rat}.h`,
`src/inlines.c` (how header-inline functions are exported), `tests/test_status.c`, `tests/test_exports.sh`,
`tests/test_julia.sh` (the `LD_PRELOAD` retry and how the shared object is built), `tests/julia/README.md`,
`tests/julia/smoke.jl`, `tests/julia/layouts.jl`, `lanes/d-julia/probe.jl` and `abi_probe.jl` (the probes
whose mechanism you turn into the package), `docs/api-m1.md` ("Layouts").

**Part 1, J-status (C).** In `include/adelefeld/status.h`, after `adf_status_str`, add the header-inline and
exported `int adf_status_count(void)` returning `ADF_STATUS_COUNT`, with a comment block in the form of
`adf_status_str`'s (convention, aliasing, status, cost, "header-inline and exported (conventions 12.1)";
cite `docs/api-j.md:72-76`). Red first: a test in `tests/test_status.c` that calls it and checks 11 and
`adf_status_count() == ADF_STATUS_COUNT`, seen failing (a link error counts for the first test of a new
symbol). `tests/test_exports.sh` must then count 625 of 625. Mutation testing is not useful for a one-line
accessor: say so; the fault table has one fault (return 10) that the test must catch.

**Part 2, J-a step 1 (Julia).** Create `julia/Adelefeld/` with `Project.toml` (name Adelefeld, version 0.1.0,
a fresh UUID from `using UUIDs; uuid4()`, deps: Libdl only; `[extras]` Test; `[targets] test = ["Test"]`),
`src/Adelefeld.jl` and submodule files as you see fit, `test/runtests.jl`, and `bin/adelefeld-julia` (design
section 2, last paragraph: a launcher that sets the system libgmp preload the way `tests/test_julia.sh` does,
then execs `julia` with the arguments given). Implement exactly:
- loading (design section 2): the library path from `ENV["ADELEFELD_LIB"]`, else `build/libadelefeld.so`
  relative to the repository root (the package knows its own path: `@__DIR__`); `dlopen` NOW|LOCAL at `__init__`;
  `adf_version_check` must be OK; the word-size and `Cint`/`Csize_t` requirements; the layout gate for `rat`
  only in this step (size 16, alignment 8 from `docs/api-m1.md:35`, a literal row, not a live read) with the
  table structured so that later steps add rows; the status gate: `@enum AdfStatus` as section 4 writes it,
  every `adf_status_str(i)` compared with the enum name, `UNKNOWN` at -1 and 11, and `adf_status_count() ==
  11 == length(instances(AdfStatus))`. Each failed gate throws an error naming the gate, the expected and the
  observed value, and (for the missing GMP symbol) the exact preload command. No adf init runs after a failed
  gate.
- `Rat` (design section 3 and the `Rat` part of section 4): aligned C heap storage owned by a mutable handle
  (the proof of alignment and containment in a comment), `adf_rat_init` once, finalizer calling the cached
  clear then `Libc.free`, repeated close a no-op, closed handles throw `InvalidStateException` before C,
  `GC.@preserve` around every ccall that passes a handle; constructors from `Integer`, `Rational`, and
  `parse(Rat, s)` through `adf_rat_set_str` with `adf_text_limits_default`; `+ - * /` and `==` between `Rat`s
  and with `Integer`/`Rational` as the design states; `string`/`show` through the text form with
  `adf_str_free`; `try_` methods returning `AdfResult`, operators throwing `AdfError`, `unwrap`. Read the
  `rat.h` comment of each function you call for its statuses (`den = 0` is `DOMAIN`; division by zero is
  `NOT_UNIT`) and make the tests assert them.
- tests (`test/runtests.jl`, design section 8, the J-a lines that concern `Rat` and the load gates): the five
  SPEC arithmetic rows that are rational (`docs/SPEC.md` 4.1: find them), `1/3` staying exact, `den = 0` DOMAIN
  against division by zero NOT_UNIT, inputs unchanged by every operation, copies clearing separately, the
  string round trip, 10^4 handles with forced GC and the live-handle count returning to 0 (as `probe.jl`), the
  load-rejection tests in SEPARATE subprocesses (wrong size row, wrong status name, missing symbol: through a
  test fixture library or an environment variable that the package honours ONLY under a documented test
  switch, never in production paths; design section 8 first paragraph). Red-green: write the tests first and
  record the red run.
- the gate command of design section 9, from the repository root, with `build/libadelefeld.so` built by
  `timeout 600 sh tests/test_exports.sh`:
  `timeout 180 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10
  julia --startup-file=no --project=julia/Adelefeld -e 'using Pkg; Pkg.test()'`; if `Pkg.test` needs a writable
  depot in this environment, set `JULIA_DEPOT_PATH=lanes/j-slice1/depot:$HOME/.julia` and
  `JULIA_PKG_OFFLINE=true`, and delete the depot at the end. The session line of section 9 for this step:
  `using Adelefeld; Rat(1)//3 + Rat(1,6)` prints `1/2` (choose the operator the design states for a quotient of
  Rats; if the design is silent, use `/` and say so). Add the gate to `tests/test_julia.sh` as one block after
  the existing blocks (the script already handles the preload retry), skipped when `julia` is absent.

**You own:** `include/adelefeld/status.h` (the one addition), `tests/test_status.c` (append), `julia/` (new),
`tests/test_julia.sh` (one appended block), `lanes/j-slice1/`, and the statements file `docs/api-ja.md` (new:
what was built, the statements the tests check, the HEADER-FINDINGs against `docs/api-j.md` if any, in the
form of `docs/api-5a.md`). Everything else is read-only; do not edit `docs/api-j.md` (a design is not edited
by lanes: mark `DESIGN-FINDING` in `docs/api-ja.md`). No git command that changes state, no `bd`. At most 2
cores, one Julia thread and one precompile task, every command under `timeout`, nothing in `/tmp` or outside
your files; build the C library into directories under your lane directory except the `build/libadelefeld.so`
that `tests/test_exports.sh` writes (delete `build/` at the end). Lines at most 116 characters, files end with
a newline, plain sober prose, no LaTeX. The INV abort children rule of `tests/` applies to the C test.

Checks at the end: `test_status` under plain, `SAN=1`, `INV=1`, `CC=clang`; `tests/test_exports.sh` (625);
the Julia gate; `sh tests/test_julia.sh` in full (it runs the 44 fixtures too: report the count). Report:
`lanes/j-slice1/result.md` (a Claude subagent cannot write `report.md`): what was built, every command with
its numbers, the red and green runs, the fault table, findings against the design, what is not done, sources
pending (Julia documentation is not under `refs/`: quote `?help` output with the version, or mark pending).
Aim to finish within about ninety minutes; part 1 first, then the load gates, then `Rat`.
