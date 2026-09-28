# Lane m1-julia: report (Claude sonnet; summary by the orchestrator of the final message of the lane)

Work package 1.9, the rest: the Julia-friendly interface check (M0-D12). Julia 1.12.5.

## Files

`tests/test_layouts.c` (1 test, 10 checks: the alignments that `tests/test_abi.c` left out: `adf_text_limits_t`,
`adf_ctx_desc_t`, `adf_text_kind`, the five array-of-one types); `tests/julia/layouts.jl` (the layout table of
`docs/api-m1.md`, typed by hand); `tests/julia/smoke.jl` (48 tests through `Libdl.dlopen` and `ccall`, no C
header: version, layouts, `adf_rat`, `adf_fball` with a raw read of `A`, `H`, `d` at their offsets,
`adf_adele`, a status, a context); `tests/julia/README.md`; `tests/test_julia.sh`;
`lanes/m1-julia/redgreen.log`.

## Checks (by the lane)

- `sh tests/test_julia.sh`: 48 of 48 pass. Without `julia`: `julia: not installed, skipped`, exit 0.
- `make clean && make -j2 check`, and with `SAN=1`: 24 programs pass (in the worktree of the lane).
- Red-green in scratch copies: fields `A` and `H` of `adf_fball_struct` exchanged: 2 of 48 fail (the raw
  reads); `adf_rat_struct` grown by 8 bytes: 1 of 48 fails (the size).

## Environment

Julia from juliaup loads its own `libgmp.so.10`, which lacks `__gmpn_modexact_1_odd`; the system
`libflint.so.18` needs it, so `dlopen` of our library fails inside Julia. `tests/test_julia.sh` sets
`LD_PRELOAD` to the system `libgmp.so.10` on that symptom. A Julia package will have to solve this (build
against the GMP and FLINT of Julia's artifacts, `GMP_jll`, `FLINT_jll`).

## Findings on the interface

1. `adf_place_t` is passed and returned by value (`place.h:24`, `:28`, `:40` to `:53`); deliberate
   (conventions 12.5); a binding declares a bits type of one `UInt64`.
2. Status codes, comparison values, backend tags, limits and version numbers are preprocessor macros
   (`status.h:17-37`, `fball.h:45-46`, `text.h:49-54`, `adelefeld.h:30-36`); no exported accessor; a binding
   copies the numbers by hand and nothing catches a stale copy.
3. The array-of-one types hide the pointer in the declaration; a hazard for a generator of bindings only.
4. `adf_text_kind` has no `adf_sizeof_` or `adf_alignof_` function.
5. Functions that take `fmpz_t`, `fmpq_t`, `arb_t`, `acb_t`: the binding must make and clear these through
   `libflint` itself; a zero-filled `fmpq` is `0/0` and not canonical, so `fmpq_init` must run.
6. Clean: no variadic function, no function pointer, every inline function exported, `size_t` and `slong`
   used as conventions 12.10 says.

## Not done

Nothing of `adf_scaled`, the dump form or the local backend (not on master when the lane ran). No FLINT
value is made from Julia.
