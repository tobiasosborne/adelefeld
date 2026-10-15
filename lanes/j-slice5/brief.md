# Lane j-slice5: step 1 of J-b of docs/api-j.md: contexts, leases and the local backend

Milestone J, the Julia binding (epic adf-c5e; design `docs/api-j.md`; statements of J-a in `docs/api-ja.md`,
lanes j-slice1 to j-slice4, merged). The package `julia/Adelefeld/` has the load gates, the statuses, the handle
mechanism, `Rat`, `FBall`, `Adele`, `CAdele`, `RealBall`, `ComplexBall`, `readvalue`, dump forms, `reconstruct`;
4881 tests through `Pkg.test`. This lane builds the FIRST item of J-b in section 9 of the design,
"contexts/leases/local backend", end to end, and nothing wider: no `Scaled`, no `cap`, no `Place`, no `LBall`,
no `SBall` (those are steps 2 to 4 of J-b).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `docs/api-j.md` (all; section 3 at lines 90-151 for the handle
mechanism and the CONTEXT PROOF at 126-141, which you implement exactly; section 5 at 242-280 for `try_modctx`,
`ModCtx`, `try_to_local`, `try_enclose_local`, `to_global`), `docs/api-ja.md` (all DESIGN-FINDINGs: keep their
decisions), `julia/Adelefeld/src/*.jl` (`handle.jl` and `carrier.jl` first: the lifecycle states, the finalizer
counter, where a context root would live) and `test/runtests.jl`, `lanes/j-slice4/report.md` and `gate.sh`,
`include/adelefeld/modctx.h` (all: the constructors at 57-101, their statuses and costs at 11-56, the accessors
from 105; `adf_modctx_free`), `include/adelefeld/fball.h` (the local conversions `adf_fball_set_local` or
whatever names the header gives for "exact local conversion", "enclosing variant" and "to_global": read the
header, cite the lines; the `lost` output and on which status it is written), `docs/conventions.md:342-397`
and `1582-1583` (contexts: ownership, the lifetime rule, "a value refers to a context"), `docs/SPEC.md:184-214`
and `285-298` (same-modulus separate contexts are distinct; finite arithmetic may fall back to global).

Build (design sections 3 and 5):
- `ModCtx`: a handle with the state of the context proof (ptr, atomic count = 1 at construction, the owner
  token); `try_modctx(; blocks, prime_powers, modulus, factorial, primorial_power)` requiring EXACTLY ONE
  construction family (two or none: a Julia `ArgumentError` before C, the design's "require exactly one"),
  arrays copied into Julia-owned `Vector{UInt64}` and preserved across the call, the C pointer slot NULL and
  read only on OK; the statuses and stages exactly `modctx.h:11-56` (DOMAIN, LIMIT, UNSUPPORTED as the header
  says, each with `details`); `ModCtx(; kwargs...)` throwing `AdfError`; accessors `modulus(ctx) -> Rat` or
  `BigInt` (through `adf_modctx_get_modulus`; say which type and why), and the block count or prime powers if
  the header has accessors (read it).
- Leases (the context proof, steps 1 to 5): a value that borrows a context reserves one token per distinct
  context state BEFORE the C call and keeps the `ModCtx` in its roots; a constructor failure releases the
  reserved tokens; a value drops its tokens only AFTER its whole C clear; the `ModCtx` finalizer drops the
  owner token; only the decrement that sees count 1 calls `adf_modctx_free`. Both finalizer orders are tested
  (context dropped before the value and after), with forced GC, and a counter of frees asserted (0 live).
- `try_to_local(x::FBall, ctx::ModCtx)` (exact local conversion), `try_enclose_local(x, ctx)` (the enclosing
  variant), `to_global(x::FBall)`: through the fball.h functions, with `lost::Bool` in `details` ONLY on OK
  (design line 258); the result a fresh `FBall` that holds a lease on `ctx`; `value_text` of a local value
  prints what the C printer prints (conventions 9.8 A11: the value form does not record the backend: say what
  the text of a local value looks like and cite).
- tests (`test/runtests.jl`, one appended testset "Adelefeld step J-b.1"): each construction family on a
  valid input (the modulus from the accessor equals the product of the blocks, computed in Julia with BigInt),
  the refusals (two families, none, an empty array, a block of 1, a block above 2^64 as the header says,
  the LIMIT and UNSUPPORTED cases the header names), the five SPEC 4.3 rows converted to local and back
  (`to_global(to_local(x))` `identical` to `x` when exact, `lost` false; a case with `lost` true), the
  same-modulus-distinct-contexts case of SPEC 184-214 (two contexts of the same modulus are different
  objects and a value made in one is not in the other: whatever predicate the C surface gives you, cite it),
  the finalizer orders with 10^4 round trips and 0 live, inputs unchanged, closed handles refused.
- the gate (as `lanes/j-slice4/gate.sh`): `Pkg.test` green, counts before (4881) and after; the session line
  of the design is for step 4 (`project`), so give instead: `ctx = ModCtx(blocks=[8, 9]); to_local(FBall(Rat(5),
  Rat(72)), ctx)` and its printed text.
- `docs/api-ja.md`: statements S35 onwards for this step; DESIGN-FINDINGs numbered after the last.

You own: `julia/Adelefeld/**` (not its `bin/` launcher), `docs/api-ja.md` (append only), `lanes/j-slice5/**`.
Julia is started as `lanes/j-slice4/gate.sh` does (the libgmp preload, the depot with the TRAILING colon,
offline, one thread). No C change; a C function that is missing or wrong is a finding, with the header line.
Report (`lanes/COMMON.md` rule 8): the red and green runs, the test counts, the gate, the faults you planted
(as the earlier lanes did: at least 8, each caught or not), the session line, the DESIGN-FINDINGs.
