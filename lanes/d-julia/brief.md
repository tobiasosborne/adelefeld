# Lane d-julia: design of the Julia binding before code

A design document before code, the map for thin slices (`docs/workflow.md` rules 1 and 3), in the form of
`docs/api-5.md` (lane d-tate) and `docs/api-4.md`: read `lanes/d-tate/brief.md` and `docs/api-5.md` for the
form and the length (at most 450 lines; declarations with full comment blocks, statements, acceptance tests,
slices; nothing twice). Write for an implementer who builds it in lanes of about an hour each and for a
reviewer who tries to refute you. No change of `src/`, `include/`, `tests/` or `docs/` other than the new
design file and your lane directory.

The epic: bead adf-c5e, "Julia binding on top of the C library" (TJO 2026-09-27: "C core, Julia on top"; the
C interface was kept `ccall`-friendly for it: `docs/SPEC.md` section 10 item 4 (lines about 764-775) and
`docs/conventions.md` section 12 (lines 1553 onwards), decision M0-D12). Milestone 5, the last gate of version
1 of the core, is landing now (`HANDOFF.md`). The library exports 624 functions (`tests/test_exports.sh`) in
36 headers (`include/adelefeld/`); the layouts are in `docs/api-m1.md` ("Layouts"); forty Julia fixtures in
`tests/julia/*.jl` already drive it through raw `ccall` with storage from `adf_sizeof_<type>` (read
`tests/julia/README.md` in full, `smoke.jl`, `layouts.jl`, `tate_fe.jl`, `char.jl`), including the quirk that
Julia's bundled `libgmp` lacks a symbol `libflint.so.18` needs (`LD_PRELOAD` of the system libgmp; README).
Findings of the first interface check are bead adf-7gc (`lanes/m1-julia/report.md`): status codes and other
constants are macros (`include/adelefeld/status.h:17-36`; `adf_status_str` at `:51` IS a function), GMP of
Julia against system FLINT, FLINT values from Julia. Julia here is 1.12.5 (`juliaup`); only the standard
library is installed (`Pkg.status()`: PicoSAT only); Nemo.jl is NOT installed and must not be required.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `docs/SPEC.md` 10, `docs/conventions.md` 12, `docs/api-m1.md`,
`tests/julia/README.md` and the four fixtures named, `include/adelefeld/{common,status,rat,fball,adele,
modctx,place,text}.h` (the first slice's surface), `tools/adf/README.md` (the driver's command grammar: the
Julia API should let a user do what the driver does), `lanes/m1-julia/report.md`.

Decisions taken by the orchestrator for this design (record them in the document; each open to reversal):
- J1. The package lives in this repository under `julia/Adelefeld/` (Project.toml, src, test), version 0.1,
  no registry, loaded by path. It depends on the standard library only; Nemo.jl and FLINT_jll are optional
  through a package extension designed but built in a later slice.
- J2. No C glue: every call is a `ccall` into `libadelefeld.so`; the library path comes from the environment
  variable `ADELEFELD_LIB` or a default relative to the package; `__init__` runs `adf_version_check`, compares
  `adf_sizeof_`/`adf_alignof_` of every wrapped type with a transcribed table (as `layouts.jl`), and fails to
  load on a mismatch with a clear message.
- J3. Every status is surfaced. Design BOTH forms and say which is primary: functions that return the status
  with the output, and functions that throw an `AdfError` carrying the status and `adf_status_str`. Statuses
  are transcribed as an `@enum` from `status.h` and checked at load time against `adf_status_str` of each
  value and `ADF_STATUS_COUNT` (which is a macro: say how the binding learns the count without a header, or
  propose the smallest C addition as a separate slice; `src/` is not yours).
You decide and justify: the value representation (inline storage in a `mutable struct` with a finalizer that
calls `adf_<type>_clear`, as Nemo does for FLINT types, against a heap handle; alignment; GC safety during a
`ccall` with `GC.@preserve`); how a value keeps its modulus context alive (conventions 12.6); places as a
bits type; strings in and out (`adf_str_free`); printing (`show` through the text forms of conventions 9) and
parsing (`parse` through `set_str`); arithmetic operators; how a user writes what the driver's commands write
(`tools/adf/README.md`); thread safety (what FLINT and the library promise: cite); the Nemo bridge (how an
`arb`/`acb`/`fmpq` of Nemo would be passed by pointer: Nemo's layout IS FLINT's; state what must be checked at
load time); errors in finalizers; the `libgmp` quirk (document it; propose the mechanism the package uses so
that a plain `using Adelefeld` works on this machine, or state that it cannot and why).

Prototype, not code: a probe `lanes/d-julia/probe.jl` that proves the chosen representation works on this
machine: build the shared object in your worktree with `timeout 600 sh tests/test_exports.sh` (it writes
`build/libadelefeld.so`; read the script), then allocate 10^5 `adf_rat` values with finalizers, do arithmetic,
force `GC.gc()` repeatedly, and show no crash and no leak (count the clears). Record the command and the
numbers in the design's acceptance section. Delete `build/` at the end.

Slices (the design's last section), each ending in a `Pkg.test` run and a line of a user session: J-a the
package skeleton, load-time checks, `Rat`, `FBall`, `Adele` with arithmetic, text and statuses (the surface
of `docs/SPEC.md` 4.1 to 4.4, the driver's `arith`); J-b contexts, places, `Scaled`, the partial and local
balls; J-c ideles, classes, the quotient, characters; J-d functions and the Tate integrals; J-e the Nemo
extension. Each slice names its headers and the fixtures of `tests/julia/` it supersedes.

Rules: `lanes/COMMON.md` (no git that changes state, no `bd`, at most 2 cores, every command under
`timeout`); plain sober English, lines at most 116 characters, no LaTeX, mathematics in plain text; cite
sources with file and line (`refs/`, `include/`, `docs/`); write `[source pending: ...]` where a Julia or
FLINT fact is not on disk (Julia's own documentation is NOT under `refs/`: cite the installed `julia`'s
`?help` output verbatim with the version, or mark it pending). You own `docs/api-j.md` and `lanes/d-julia/`.
Report `lanes/d-julia/report.md` at the end: what was designed, the probe's numbers, the decisions you took
and why, what you could not settle. Aim to finish within about ninety minutes.
