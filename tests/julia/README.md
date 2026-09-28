# tests/julia: the Julia ccall smoke test

Work package 1.9 of `docs/PLAN.md` (row 1.9: "a Julia ccall smoke test where Julia is
installed"), `docs/SPEC.md` 10 item 4 (M0-D12), `docs/conventions.md` section 12. Lane `m1-julia`.

This proves that the library, as it stands, can be driven from Julia through `ccall` alone,
without `include/adelefeld/*.h` and without any C glue code: every value is either a plain Julia
bit pattern or a block of memory from `Libc.malloc`, sized by a live call to `adf_sizeof_<type>()`.

## Running it

    sh tests/test_julia.sh

from the repository root. It builds `build/libadelefeld.so` the way `tests/test_exports.sh` does
(calling that script if the shared object is missing or older than `src/*.c` or
`include/adelefeld/*.h`, reusing it otherwise), then runs

    julia --startup-file=no tests/julia/smoke.jl build/libadelefeld.so

If `julia` is not on `PATH`, the script prints `julia: not installed, skipped` and exits 0.

Only the Julia standard library is used (`Libdl`, `Test`); no package is installed from the
network, and none is required. If Nemo.jl or FLINT_jll happen to be installed in this Julia, they
are not imported and not relied upon.

## Files

- `layouts.jl`: the struct-layout table, typed in by hand from `docs/api-m1.md` ("Layouts (64-bit,
  FLINT 3.0.1)", the table and the note "All alignments are 8, except `adf_text_kind` (4)"). Not
  generated from the C headers, not read from the library: an independent transcription, so that a
  mismatch between this file and a live `ccall` of `adf_sizeof_*`/`adf_alignof_*` is evidence of a
  real disagreement between the documentation and the library, not a copy of the same mistake.
  Also carries the field offsets of `adf_fball_struct` (`A` at 0, `H` at 8, `d` at 16), used by
  `smoke.jl` for one raw-memory field read.
- `smoke.jl`: the test itself, one `@testset` per area of the interface (version, layouts,
  `adf_rat`, `adf_fball`, `adf_adele`, a non-`OK` status, a modulus context), using `Test`.
  `julia smoke.jl <path-to-libadelefeld.so>` exits non-zero if any `@test` fails.

## Known environment quirk: LD_PRELOAD of the system libgmp

This is not a fault of the adelefeld interface; it is recorded here and in
`lanes/m1-julia/report.md` because it changes how the smoke test must be launched on this
machine, and a reader who tries `julia tests/julia/smoke.jl build/libadelefeld.so` directly (as
the brief phrases the command) hits it immediately.

Julia (installed here through juliaup) bundles its own `libgmp.so.10`
(`~/.julia/juliaup/*/lib/julia/libgmp.so.10`), built without the plain symbol
`__gmpn_modexact_1_odd` (only a CPU-dispatched `__gmpn_modexact_1_odd_x86_64`; checked with
`nm -D` on both copies). Julia's runtime loads its bundled copy before any user code runs (checked
with `Libdl.dllist()` at the top of an otherwise empty script). The system `libflint.so.18` was
linked against the system `libgmp.so.10` (`/lib/x86_64-linux-gnu/libgmp.so.10`), which does export
the plain symbol. Both share the SONAME `libgmp.so.10`, and the dynamic linker's ordinary
soname-based reuse then makes `dlopen("build/libadelefeld.so")` fail with

    /lib/x86_64-linux-gnu/libflint.so.18: undefined symbol: __gmpn_modexact_1_odd

`Libdl.RTLD_DEEPBIND` does not help (checked): the missing symbol is undefined in the *already
loaded* `libgmp.so.10`, so no binding order within the newly opened library changes which object
satisfies that SONAME. The fix has to happen before Julia's own startup loads its bundled copy,
i.e. `LD_PRELOAD` of the system `libgmp.so.10`, set on the process that execs `julia`:

    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia --startup-file=no tests/julia/smoke.jl build/libadelefeld.so

`tests/test_julia.sh` does this automatically: it runs Julia once, and only on exactly this
failure (matched on the symbol name in the output) retries once with `LD_PRELOAD` set to the
system `libgmp.so.10` that `ldconfig -p` reports. A machine without this particular Julia/GMP
mismatch never takes the retry path. A future binding that ships its own Julia environment would
need the same `LD_PRELOAD`, or to link `libadelefeld.so`/`libflint.so` against a self-contained
GMP, or to load everything through `dlmopen` in a private namespace (not exposed by `Libdl`).
