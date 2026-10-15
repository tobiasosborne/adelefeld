# Lane j-slice5 result: step J-b.1 (contexts, leases, local backend)

## Done
Step J-b.1 of docs/api-j.md section 9 is built end to end, and nothing wider.
- ModCtx is a handle with the state of the context proof: a C pointer, an atomic token count, and the
  owner token.
- try_modctx and ModCtx take exactly one of five families: blocks, prime_powers, modulus, factorial and
  primorial_power.
- Accessors: modulus (a Rat, finding 44), nblocks, blocks.
- Conversions: try_to_local/to_local, try_enclose_local/enclose_local (details.lost only on OK) and
  try_to_global/to_global.
- Predicates: is_local, and context, which is backed by adf_fball_context.
- Leases implement context proof steps 1 to 5 of docs/api-j.md:126-141. FBall, Adele and CAdele carry
  roots, and every operation that can borrow a context leases first.
- No C change. The specification and the design are unchanged.

## Files
- New: julia/Adelefeld/src/context.jl (172 lines), julia/Adelefeld/src/modctx.jl (266 lines).
- Changed: src/Adelefeld.jl, adele.jl, carrier.jl, fball.jl, handle.jl, load.jl (14 new required
  symbols, 116 in all).
- Tests: test/runtests.jl gets one appended testset, "Adelefeld step J-b.1": 10 testsets, 488 checks.
- docs/api-ja.md: an append-only section (lines 559-800). Statements S35-S45; DESIGN-FINDINGs 40-51,
  after the last earlier one (39). The first 557 lines are byte-identical, checked with cmp.
- Lane: gate.sh, run-julia.sh, jb1_only.jl, faults.py, session.jl, and the logs listed below.

## Checks (run from /home/tobias/Projects/adelefeld-wt/j-slice5)
- Before, `sh lanes/j-slice5/gate.sh` (before.log): 4881/4881 (J-a.1 904, J-a.2 1015, J-a.3 1649,
  J-a.4 1313), plus the counted-call subprocess's 43.
- Red (red.log): J-b.1 gives 0 passes and 9 errors (diagnostics() had no contexts field). The other
  sections pass.
- Green, `sh lanes/j-slice5/gate.sh` (gate-final.log): 5369/5369, of which J-b.1 is 488, plus the 43.
  The exit is 0 and Pkg.test says "Adelefeld tests passed".
- J-b.1 alone, `sh lanes/j-slice5/run-julia.sh lanes/j-slice5/jb1_only.jl` (jb1-clean.log): 488/488.
- Session, `sh lanes/j-slice5/run-julia.sh lanes/j-slice5/session.jl` (session.log):
  `ctx = ModCtx(blocks=[8, 9]); to_local(FBall(Rat(5), Rat(72)), ctx)` prints `(* ; 5 mod 72)`. ctx shows
  `ModCtx(modulus=72, blocks=[8, 9])`. is_local is true, context(y) === ctx, and to_global(y) is identical
  to the global input.
- Planted faults, `python3 lanes/j-slice5/faults.py F<n>` (16 faults, fault-summary-final.log):
  - 14 are caught by failed assertions: F1-F7, F9-F11 and F13-F16.
  - F1, F3, F4 and F7 then crash. Their first failure is in the lease-basics testset (S38).
  - 2 are not caught, and neither can be seen in this build. F8 would return a context on a refusal, but
    every write of the C output pointer is on an ADF_OK path (src/modctx.c:210-400). F12 drops the tokens
    before the clear, but adf_fball_clear reads the context only through ADF_INV_RELEASE, which is a no-op
    without -DADF_CHECK_INVARIANTS (src/invariants.h:107).
  - Per-fault counts are in docs/api-ja.md, section "Gate, session and planted faults".

## Incidents this session
- An earlier fault run filled the disk: each crash left a core of up to 1.4 GB in
  /var/lib/systemd/coredump. The core pattern pipes to systemd-coredump, which ignores RLIMIT_CORE.
  faults.py now sets /proc/self/coredump_filter to 0, which the Julia children inherit, and cores are about
  7 KB. 101 GB are free now. The lane's own cores and scratch copies were deleted. The older cores in
  /var/lib/systemd/coredump belong to root and were left alone.
- In the earlier rounds the crashing faults showed 0 failed assertions. That was an artefact: Julia's
  libuv stdout and stderr lose queued writes on abort, even into a regular file. jb1_only.jl now rebinds
  them to raw descriptors (finding 51). A lease-basics testset (S38, 12 checks) was added before any later
  use of a value; that raised J-b.1 from 476 to 488 checks.

## Not done
Scaled, cap, Place, LBall, SBall (J-b steps 2-4), context dumps, pruning of roots (finding 47), the LIMIT
path of try_modctx (finding 43, untested), two-thread support, long differential fuzzing, and an INV=1/ASan
oracle for F12.

## Findings
- Against the specification: none.
- Findings 40-43 are against the brief or the header wording:
  - 40: modctx.h:31-32 says DOMAIN before UNSUPPORTED, but 65537 blocks give UNSUPPORTED.
  - 41: an empty array is OK.
  - 42: a block above 2^64 cannot reach C.
  - 43: no constructor has a LIMIT status.
- Findings 44-51 record binding decisions.
- Sources pending: none new.
