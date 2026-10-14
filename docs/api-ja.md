# Step J-a.1: status count, package skeleton, load gates, Rat

This step builds the first two items of `docs/api-j.md` section 9: the C prerequisite J-status and the first
step of J-a (package skeleton, load gates, statuses, the `Rat` handle). Nothing wider: no `FBall`, `Adele`,
`Place`, contexts or numerical carriers. Lane j-slice1. The design `docs/api-j.md` is the contract; where this
step departs from it or where it is silent, the place is marked DESIGN-FINDING below.

## What was built

- C: `int adf_status_count(void)` in `include/adelefeld/status.h`, header-inline and exported (conventions
  12.1), returning `ADF_STATUS_COUNT` = 11 (`docs/api-j.md:72-76`). Test: `tests/test_status.c`, test
  `the_status_count_accessor_returns_the_macro`. `tests/test_exports.sh`: 625 of 625 declared functions exported.
- `julia/Adelefeld/Project.toml`: name Adelefeld, version 0.1.0, deps Libdl only, extras and test target Test.
- `julia/Adelefeld/src/Adelefeld.jl` (module, exports), `status.jl` (statuses, results, errors), `load.jl`
  (the gates of `docs/api-j.md` section 2), `handle.jl` (aligned C heap storage, counters), `rat.jl`
  (`TextLimits`, `Rat`, constructors, text, arithmetic, equality).
- `julia/Adelefeld/bin/adelefeld-julia`: the launcher of `docs/api-j.md:85-87`.
- `julia/Adelefeld/test/runtests.jl`, `test/load_fault.jl` (run in a separate process per load-rejection case),
  `test/fixtures/fault.c` (fixture libraries that falsify one query each; built by the tests with `cc`).
- `tests/test_julia.sh`: one block that runs `Pkg.test()` through the launcher.

## Statements the tests check

S1. Load order (`load.jl`, `__init__`): words, library path and dlopen (RTLD_NOW | RTLD_LOCAL), every symbol of
`REQUIRED` from that one handle, FLINT version, layouts, statuses. A failing gate throws `AdfLoadError` with the
gate, the expected value, the observed value and the library path; `using Adelefeld` then fails with an
`InitError` around it. After a failed gate no allocation and no adf init runs: every owning allocation calls
`require_loaded()` first, and in the observed runs the module is not even bound after the failed `using`.
Tests: six fixture libraries in separate processes, each checked for the gate's words, the library path,
`INIT_CALLS=0` (the fixture counts its `adf_rat_init` calls) and no allocation afterwards:

| Fixture | Falsified query | Gate that rejects it |
|---|---|---|
| FAULT_SIZE | `adf_sizeof_rat` returns 24 | layout: expected 16 (`docs/api-m1.md:35`) |
| FAULT_ALIGN | `adf_alignof_rat` returns 16 | layout: expected 8 (`docs/api-m1.md:45`) |
| FAULT_NAME | `adf_status_str(7)` returns `DOMAIN_ERROR` | status name: expected `DOMAIN` |
| FAULT_COUNT | `adf_status_count` returns 12 | status count: expected 11 |
| FAULT_VERSION | `adf_version_check` returns 8 | FLINT version; the message shows `3.0.1` |
| FAULT_MISSING | no `adf_status_count` (the library before J-status) | symbol: names `adf_status_count` |

Two more separate processes: the real library without the GMP preload (rejected at dlopen naming
`__gmpn_modexact_1_odd`, with the exact command `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia ...` and
the launcher path), and a library path that does not exist.

S2. Layout table (`LAYOUT_ROWS`): literal rows `rat` 16/8 (`docs/api-m1.md:35,45`) and `text_limits` 32/8
(`docs/api-m1.md:41,45`), size AND alignment compared; the Julia `TextLimits` is checked to be 32 bytes with
fields at 0, 8, 16, 24 (`include/adelefeld/text.h:89-90`). `Rat` storage is allocated with the row's size 16 and
alignment 8, not a newly read size.

S3. Statuses: `@enum AdfStatus::Int32` as `docs/api-j.md:161-164`; at load every `adf_status_str(i)`, i = 0..10,
equals the enum name, `adf_status_str(-1)` and `adf_status_str(11)` are `UNKNOWN`, and
`adf_status_count() == 11 == length(instances(AdfStatus))`. A raw C int outside 0..10 is `AdfAbiError` with the
raw value, never a status. `AdfResult{T}(status, value, where, details)`; `value` is `nothing` on every non-OK
status; `unwrap` and every throwing form throw `AdfError` carrying the operation, the status, a copy of the
status name and the whole result.

S4. Storage (`handle.jl`, `aligned_alloc`): `n + a - 1` bytes after checked addition; `p = (b + a - 1) & ~(a - 1)`;
the proof that `p % a == 0` and `b <= p`, `p + n <= b + n + a - 1` is the docstring of `aligned_alloc`, and both
facts are checked at run time; the base is freed, never `p`. Tests: alignments 1, 2, 8, 64, 4096 with sizes 1,
16, 33; a non-power-of-two alignment and a zero size are `ArgumentError`; an overflowing size `OverflowError`.

S5. Life cycle (`rat.jl`): `adf_rat_init` once per handle, then the finalizer; the release claims LIVE by
`@atomicreplace`, calls the cached `adf_rat_clear`, frees the base, decrements the live count; repeated `close`
and a later `finalize` are no-ops; a closed handle throws `InvalidStateException` before any C call and before
any output is allocated (the live count is unchanged by the refused calls); `copy` uses `adf_rat_set` into fresh
storage and the copy clears separately. Every ccall that passes a handle is inside `GC.@preserve` of the owners.

S6. Constructors and statuses: `Rat(n)` with n in Int64 uses `adf_rat_set_si`; otherwise, and for `Rat(n, d)`
and `Rat(::Rational)`, the pair is made to have a positive denominator with BigInt sign changes and read from
"n/d" by `adf_rat_set_str` with `TextLimits()` from `adf_text_limits_default`. `den = 0` is DOMAIN (`Rat(5, 0)`,
`Rat(0, 0)`, `Rat(1//0)`, `parse(Rat, "5/0")`; text.h:147-150, rat.h:74-79); division and inversion by zero are
NOT_UNIT (rat.h:109-116), as results and as `AdfError`. Parser statuses: PARSE for "", "1/", "1/-2", "abc",
"1\0", "é"; LIMIT for "1234" at `max_len = 3` and for "é1" at `max_len = 2` (3 bytes, 2 characters: the byte
count is `ncodeunits`); LIMIT for 10^2000 at `max_len = 100`, OK with the default limits.

S7. Arithmetic and equality: `+ - * /`, unary `-` and `inv` between Rats and with Integer and Rational scalars on
either side, by explicit Rat conversion; `==` by `adf_rat_equal`, and with Integer and Rational scalars; a Julia
Rational with zero denominator equals no Rat; `hash` is value-based and equal to the hash of the same Julia
Rational (so `isequal`, `Set` and `hash(Rat(5)) == hash(5)` agree with `==`). Float64 and Complex operands are
refused. No ambiguity (`Test.detect_ambiguities(Adelefeld)` is empty). Oracle: Julia `Rational{BigInt}` over 10
values (up to 151-bit numerators) in all 100 ordered pairs, for `+ - * /`, `==` and negation.

S8. Exactness and the session: `Rat(1)/3` prints `1/3`; three of it sum to 1; `Rat(1)/3 + Rat(1, 6)` prints
`1/2` (in the package and through the launcher in a separate process). Inputs are unchanged by every operation,
failing ones included, and the same handle may be both operands.

S9. Text: `string`, `show` and `repr` use `adf_rat_get_str`; exactly `len` bytes are copied, `s[len] = 0` is
checked, and `adf_str_free` runs in a `finally`; the round trip `parse(Rat, string(x)) == x` with equal text holds
for six values up to 300 digits.

S10. Handles under GC: 10^4 handles made and dropped in a loop, four forced `GC.gc()`, the live count returns to
its value before; finalizer errors 0.

## DESIGN-FINDINGs and decisions where the design is silent

1. The brief asks for "the five SPEC arithmetic rows that are rational (docs/SPEC.md 4.1)". SPEC 4.1 has no
   arithmetic rows; the five rows are the table of SPEC 4.3 (`docs/SPEC.md:242-248`), and they are finite-ball
   rows (radii by the gcd rules). This step checks their rational content only: the centre computed with Rats
   (8, 15, 60, 5/3, 1/3) and that it differs from the stated result's centre by an integer multiple of the stated
   radius. The rows themselves belong to the FBall step of J-a.
2. The design is silent on `//` for Rat. The session line uses `/`: `Rat(1)/3 + Rat(1, 6)` prints `1/2`.
   `//` is not defined.
3. The brief names only the `rat` layout row. The parsers of this step pass `adf_text_limits_t` by pointer, so
   its row (`docs/api-j.md:55`, 32/8) is checked too, with the Julia field offsets.
4. `docs/api-j.md:229` gives `==` for `(Rat, Rat)` only; `:232` lets the operators take Integer and Rational. The
   brief asks for `==` with them as well; it is implemented, and a Julia Rational with zero denominator (an
   infinity) compares unequal to every Rat instead of throwing.
5. `docs/api-j.md:213` writes `try_add(x, y; prec=64)` for the family. Rat arithmetic has no precision; the Rat
   methods take no `prec` keyword (passing one is a `MethodError`, not a silently ignored value). Likewise
   `try_value_text(x::Rat)` has no `digits` keyword: `adf_rat_get_str` has none.
6. `AdfResult.where` is typed `Any` and is always `nothing` in this step: no function here reports a place, and
   `Place` (`docs/api-j.md:248`) lands in J-b, which should narrow the field type to `Union{Place,Nothing}`.
7. `Base.close(::Rat)` is the public explicit release (`docs/api-j.md:107-109` speaks of an explicit close
   without naming it). `Adelefeld.diagnostics()` is the explicit diagnostic of `:111-112` (live handles,
   finalizer errors, load state, library path); it is not exported.
8. A failed gate is thrown from `__init__`; Julia wraps it in `InitError`, so a user sees
   `InitError: Adelefeld load gate "..." failed for library ...`. Observed with Julia 1.12.5: after the failed
   `using`, `Adelefeld` is not bound in `Main`.
9. The fixture approach relies on `dlsym` on a handle searching the object and then its dependencies, so that a
   fixture linked to the real library overrides one query. Observed on this machine (all six fixture cases);
   [source pending: glibc `dlsym` documentation of the search order through a handle's dependencies]. The
   fixtures must be linked with `-Wl,--no-as-needed`: they call nothing of the library, and the default
   `--as-needed` drops the dependency (observed: the first run reported `adf_version_check` missing).
10. Gate environment: with `JULIA_DEPOT_PATH=lanes/j-slice1/depot:$HOME/.julia` (no trailing colon) Julia does
    not append its bundled depots, recompiles Pkg and two dozen standard libraries, and the gate's 180 s ran
    out. With a trailing colon (`...:$HOME/.julia:`) the bundled caches are used and `Pkg.test` takes about
    40-60 s here. `Pkg.test` writes `julia/Adelefeld/Manifest.toml` (a generated file naming the Julia version);
    the lane deleted it; an ignore entry for it is for the orchestrator.
11. Julia's `dlopen` defaults on Linux are `RTLD_LAZY|RTLD_DEEPBIND|RTLD_LOCAL` (installed help, Julia 1.12.5,
    `lanes/j-slice1/help.log:28`); the package passes `RTLD_NOW | RTLD_LOCAL` explicitly, as the design says.

## Julia sources

Julia's documentation is not under `refs/`. The installed help of Julia 1.12.5 is quoted in
`lanes/j-slice1/help.log` (captured with `@doc`): `dlopen` flags and defaults (lines 1-35); `finalizer` "must not
cause a task switch" (59-94); `GC.@preserve` covers "Passing a pointer to x to ccall" and "Using resources of x
which would be cleaned up in the finalizer" (95-135); `InvalidStateException` (136-157); `@atomicreplace`
returns "(old, success::Bool)" (158-230); `Libc.free` "Only use this on memory obtained from malloc" (238-244);
`ncodeunits` (245-276); `unsafe_string(p, length)` "the string does not have to be NUL-terminated" (277-285).
