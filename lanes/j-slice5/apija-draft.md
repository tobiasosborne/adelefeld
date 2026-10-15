
# Step J-b.1: modulus contexts, leases and the local backend

This step builds the first item of J-b in docs/api-j.md section 9, "contexts/leases/local backend", and
nothing wider: no Scaled, cap, Place, LBall, SBall, context dumps or bindings. Lane j-slice5. The earlier 39
DESIGN-FINDINGs are kept. The specification, the design and the C library are unchanged.

## What was built

- src/context.jl (new): the private CtxState (C pointer, atomic token count) and the ModCtx handle (the
  state and an atomic owner flag). Token operations: _acquire! (compare-and-swap, never from 0); _drop!
  (the decrement that sees the old count 1 calls adf_modctx_free); the ModCtx finalizer _release_owner!;
  _lease! and _drop_roots! for values. The file header writes out the context proof of
  docs/api-j.md:126-141 step by step.
- src/modctx.jl (new): try_modctx/ModCtx with exactly one of five construction families; modulus, nblocks,
  blocks; show; try_to_local/to_local, try_enclose_local/enclose_local, try_to_global/to_global, is_local,
  context; private context_pointer.
- src/fball.jl, src/adele.jl, src/carrier.jl: FBall, Adele and CAdele get a roots field. Every operation
  whose output can borrow an input's context leases the union of its inputs' roots before the C call. Every
  release drops the tokens after the clear and after the free of the storage.
- src/load.jl: 14 new required symbols (modctx.h:57-112, 145-174), 116 in all. No new layout row: the
  context is opaque (docs/api-j.md:63). src/handle.jl: diagnostics() adds contexts and context_frees.
  src/Adelefeld.jl: two includes and 14 exports.
- test/runtests.jl: one appended testset "Adelefeld step J-b.1" (10 testsets, 488 checks).

## Statements the tests check

S35. Families (modctx.h:57-87). try_modctx takes exactly one of blocks=[q...], prime_powers=[p => e, ...],
modulus=K, factorial=n, primorial_power=(n, e). None, or two or more, is ArgumentError before C. Arrays are
copied into Julia-owned Vector{UInt64} and preserved across the call. A later change of the caller's array
does not change the context. Each word must fit a C ulong. ArgumentError, with no context made, for: -1,
2^64, 1.5, a negative exponent, a tuple instead of a pair, factorial -1, primorial (5, -1), modulus 1.0.
The pointer slot starts NULL and is read only on OK. Every result has details = (family = <keyword>,).

S36. Valid input. For six block lists, among them [2^64-1, 2^61-1] and ten primes, modulus(ctx) equals the
BigInt product of the blocks; blocks(ctx) returns them in order and nblocks gives their number.
prime_powers keeps the order ([3 => 2, 2 => 3] gives [9, 8]); [2 => 63] gives 2^63. modulus=72 and 2^64-1
give one block; 1, 2^64 and 10^30 give none, and modulus(ctx) equals K. factorial 5 gives [8, 3, 5]; 65
gives 65! (v_2 = 63); 0 and 1 give none. primorial (5, 2) gives [4, 9, 25]; (2, 63) gives 2^63; (1, 3) and
(5, 0) give none. blocks=[] is OK: modulus 1 without blocks (modctx.h:57-58). modulus(ctx) is a Rat
(finding 44). show prints ModCtx(modulus=72, blocks=[8, 9]).

S37. Refusals.
DOMAIN: blocks [1], [0], [6, 4], [8, 8], [8, 9, 1], [35, 77]; prime powers [4 => 1], [2 => 0],
[2 => 1, 2 => 2], [1 => 1], [4 => 64] and [2 => 64, 4 => 1] (DOMAIN before UNSUPPORTED, modctx.h:31-32);
modulus 0, -5, -2^70.
UNSUPPORTED: prime powers [2 => 64], [3 => 41]; factorial 66, 10^6, 2^64-1; primorial (2, 64), (3, 41),
(821647, 1); 65537 distinct primes as blocks and as prime powers. That is more than
ADF_MODCTX_MAX_BLOCKS = 65536 (modctx.h:23-28, 48); the 65537th prime is 821647.
In every case value is nothing and no context is made. ModCtx(blocks=[1]) throws AdfError with DOMAIN and
the same details.

S38. Lease basics, checked before any later use. A new context has 1 token. One conversion gives 2. A
failing conversion leaves 2. finalize(ctx) gives 1, with the context still live and no free. The value
then prints. Closing it gives 0 tokens and exactly one free. The value is used only while its context has
not been freed, so that a broken lease is reported by these assertions, not by a use after free (finding
51).

S39. Local conversion (modctx.h:145-169). The context of blocks [16, 27, 5] has K = 2160, and all nine
inexact balls of SPEC 4.3 are local in it. For each, try_to_local gives:
- OK with details (lost = false,);
- is_local, context(y) === ctx, and the value's C context pointer equal to the context's;
- is_canonical; equal_set to x but not identical, since the backend differs (fball.h:102-106);
- value_text(y) equal to the global text of x, for example "(* ; 3 mod 12)": the value form does not
  record the backend (conventions 9.8 A11; text.h:37-38).
to_global(to_local(x)) is global and identical to x; each input is identical to its copy afterwards. On
these balls the enclosing variant gives lost false and a value identical to the exact conversion.
The five rows computed on local operands equal_set the stated results and print them; their to_global is
identical to the parsed result. The sum of two values of one context is local in it (fball.h:196-197).
The exact values 12, 1/3 and 0 are DOMAIN in both conversions, with no value and empty details; the
throwing forms throw AdfError.

S40. lost (modctx.h:155-164). The context of blocks [8, 27, 5] has K = 1080. 1/2 mod 8 is not local in it
(K/R = 135, c K/R = 135/2), so to_local is DOMAIN. enclose_local is OK with details (lost = true,), a Bool.
With d0 = lcm(135, 2) = 270 the result is (135 + 1080 Zhat)/270 = 1/2 mod 4. That value is local,
contains x, is not equal_set to it, prints "(* ; 1/2 mod 4)", and its to_global is identical to
parse(FBall, "1/2 mod 4"). Four contexts without blocks (blocks=[], modulus 1, modulus 2^64, factorial 1)
give UNSUPPORTED in both conversions (modctx.h:151, 160), with empty details. modulus 2^64-1 gives a
one-block context in which 1 mod 3 is local.

S41. Same modulus, distinct contexts (SPEC 184-214, 285-298; conventions 4.6, last paragraph). Two
contexts of blocks [8, 9] and one of prime powers [2 => 3, 3 => 2] have equal moduli and blocks and three
different C pointers: there is no interning. Make 5 mod 72 local in the first two:
- context(a) === c1 and context(b) === c2;
- their C pointers (adf_fball_context, modctx.h:171-174) differ;
- identical(a, b) is false and equal_set(a, b) is true.
a + b falls back to global (fball.h:17-20), has no context, and is identical to x + x. a + a2, both in c1,
is local in c1, and its to_global is identical to the global sum.

S42. Leases (docs/api-j.md:126-141). to_local adds one token (roots(y) == [ctx]). copy adds one, and close
gives it back. y + y adds ONE token: one per distinct state. These all root ctx: negation; subtraction;
multiplication; multiplication by a Rat on both sides; addition of a Rat and of an Integer; division by a
Rat. to_global keeps a conservative root until it is cleared. These hold ctx in their roots too:
Adele(RealBall(1), y), its finitepart (identical to y), CAdele of it, Adele sums, copy, and
CAdele(ComplexBall, y). The failing calls leave the token count unchanged: division by 0 of an FBall and of
an Adele (NOT_UNIT); to_local of an exact ball and of a non-local ball (DOMAIN). Closed inputs to
try_to_local, try_enclose_local, to_global and is_local throw InvalidStateException before C, with no
token and no allocation. After every value is closed the count is back to 1, with 0 finalizer errors.

S43. Both finalizer orders, explicitly.
Order 1: finalize(ctx) while a local value lives drops only the owner token: count 1, the context still
live, 0 frees. The value still prints, adds and converts back. The finalized ctx is refused, with
InvalidStateException, as an argument of both conversions and of modulus, blocks and nblocks; it shows
"ModCtx(finalized)". A second finalize is a no-op. Closing the last value (count 0) frees the context
exactly once; further close or finalize of either is a no-op.
Order 2: the value closed first (count 1), then finalize(ctx): one free. Two contexts, two frees.

S44. GC, in a separate process with automatic GC. 10^4 iterations each make a context, a local value and
its global copy, which is identical to the input. In one third of them the context is finalized first and
the value left to the collector. In one third the value is closed and then the context finalized. In the
rest both are left to the collector. After four GC.gc(): 0 live handles, 0 live contexts, 10000 frees, 0
finalizer errors. A value whose only reference to its context is its root survives two forced collections,
with the context live. After it is dropped and four more collections run: 0 live, 0 contexts, 10001 frees,
0 errors.

S45. Dumps and the session. The dump of a local value differs from the dump of the global value with the
same set: the dump records the backend (conventions 9.8 A11). The NULL-context loader refuses it with
DOMAIN (dump.h:41-42) and constructs no context. Test.detect_ambiguities(Adelefeld) is empty. The session
line prints (* ; 5 mod 72).

## DESIGN-FINDINGs

40. Block count before DOMAIN. modctx.h:31-32 says "The order of the checks is: ADF_DOMAIN first, then
    ADF_UNSUPPORTED". src/modctx.c:215-218 decides k > ADF_MODCTX_MAX_BLOCKS before it reads the blocks
    ("a refused call must not touch an array that is shorter than k"), and modctx.h:23-26 says the bound is
    decided from k before a table is built. So 65537 blocks whose first block is 1 give UNSUPPORTED, not
    DOMAIN (lanes/j-slice5/probe.log line 4); prime powers behave the same (src/modctx.c:252-255). The two
    header sentences disagree for that input, and the code's choice is the safer one. The tests assert
    only the unambiguous cases (65537 valid primes). This is a header wording question for the C owner,
    not a binding fault.

41. Empty array. The brief lists "an empty array" among the refusals. modctx.h:57-58 gives k = 0 the
    context of modulus 1 without blocks, with status OK, so the binding does not refuse it. The refusal
    comes at the conversion, UNSUPPORTED (modctx.h:151, 160), tested with four such contexts.

42. A block above 2^64. The C argument is a ulong (modctx.h:62), so such a block cannot reach C. The
    binding gives ArgumentError before C (docs/api-j.md:154-155), as for every other word. The header's way
    to name a modulus K >= 2^64 is modulus=K: OK, without blocks.

43. LIMIT. The brief names LIMIT cases, but modctx.h:57-87 gives no constructor a LIMIT status. Only
    adf_modctx_new_from_dump (:93) has one, and it is not wrapped in this step. The only LIMIT path of
    try_modctx is the Rat conversion of a modulus whose decimal text exceeds the default text limits. It is
    returned as that status, with details.family. That path is not tested.

44. modulus(ctx) is a Rat. adf_modctx_get_modulus writes an fmpz initialized by the caller
    (modctx.h:105-106). The binding writes it into the numerator of a fresh Rat whose denominator stays 1,
    as docs/api-j.md:148-149 prescribes for fmpz data (refs/src/flint-3.0.1/fmpq.rst:9-11, 21-26, 39-41).
    K >= 1 keeps that value canonical. A BigInt would need either a reinterpretation of GMP storage, which
    is forbidden (docs/api-j.md:196), or a text round trip. modulus=K passes the numerator of Rat(K) the
    same way. nblocks and blocks wrap the two other accessors.

45. Argument forms. The design names the five keywords only. prime_powers is a vector of pairs p => e;
    primorial_power is a tuple (n, e); modulus and factorial are Integers. details is (family = keyword,)
    on every status. The C functions report nothing beyond their status, so there is no stage field.

46. Leases on the earlier operations. fball.h:16-20 keeps a result local when all local inputs share one
    context pointer, and adele.h:85, 130-131 copy the context pointer of the finite coordinate. So a local
    value can now reach these operations: copy, the ring operations, negation, the scalar operations,
    division by a Rat, finitepart, CAdele(::Adele), Adele(::RealBall, ::FBall), CAdele(::ComplexBall,
    ::FBall), and the Adele and CAdele arithmetic. Each of them now leases the union of its inputs' roots
    (context proof step 3). Without that, copy(to_local(x, ctx)) would borrow a context it holds no token
    on. Constructors from Rat, from text and from NULL-context dumps take no lease, and neither do the
    numerical getters: their finite coordinate is global (text.h:27-28; dump.h:41-42; adele.h:119-121
    writes only the arb).

47. Conservative roots. Every result keeps the union of its inputs' roots until it is cleared, also when
    it fell back to global; to_global keeps the roots of its input (docs/api-j.md:133-134). For an FBall,
    adf_fball_context would be a verified accessor for pruning, but the design defers pruning (:140), and
    this step does not prune.

48. lost of the exact conversion. docs/api-j.md:258 puts lost in details only on OK. The exact conversion
    has no lost output in C. On OK it reports (lost = false,), since by definition it does not change the
    set (modctx.h:145-148). The enclosing variant passes an invalid sentinel -1 and reads it only on OK. A
    value other than 0 or 1 is AdfAbiError, after the fresh output is released.

49. Membership predicate. The C surface gives adf_fball_context (modctx.h:171-174) and
    adf_fball_identical (fball.h:102-106, "the same context pointer"). context(x) returns the ModCtx among
    x's roots whose pointer equals adf_fball_context(x), and nothing for a global value. It throws if no
    root matches, which would mean a broken lease.

50. Early release. There is no public close of ModCtx (docs/api-j.md:137). Base.finalize(ctx) runs the
    registered finalizer at once, and it is the deliberate early release of the owner token. Repeating it
    is a no-op. Afterwards ctx is refused as an argument, while existing values stay valid. try_to_global
    is added beside to_global for the try_ family (statusless, OK synthesized).

51. A broken lease shows up as a crash, not as a failed test. If a lease fault lets the context be freed
    early, the next use of a value is a use after free in C: FLINT aborts or segfaults (fmpz_mul in
    adf_modctx_recombine, src/modctx.c:595-598; adf_modctx_free at :427). Two consequences for the tests.
    (a) The token and free counters are asserted before the first later use of a value (S38), and the
    value is used only while its context has not been freed. Then the fault fails assertions before it
    can crash. (b) Julia's libuv stdout and stderr queue writes, and an abort discards them, also when the
    output goes to a regular file (lanes/j-slice5 abort probe: neither a println nor a Test Failed line
    survives). The fault harness lanes/j-slice5/jb1_only.jl therefore rebinds Base.stdout and
    Base.stderr to synchronous raw descriptors (Base.Filesystem.File(RawFD(1)), RawFD(2)). The package's
    own tests are unchanged. Planted faults must not leave cores on the shared disk. The core pattern
    pipes to systemd-coredump, which ignores RLIMIT_CORE, so the harness sets
    /proc/self/coredump_filter to 0, which the Julia children inherit. One earlier run filled the
    disk, with a 1.4 GB core per crash.

## Gate, session and planted faults

Commands are run from the worktree root. The gate is lanes/j-slice5/gate.sh: Pkg.test, one thread,
offline, the depot with the trailing colon.
- Before (lanes/j-slice5/before.log): 4881/4881 (J-a.1 904, J-a.2 1015, J-a.3 1649, J-a.4 1313), plus
  the counted-call subprocess's 43.
- Red (red.log): the J-b.1 testset gives 9 errors and 0 passes (FieldError: diagnostics() has no
  contexts field). The other four sections are unchanged.
- Green (gate-final.log): 5369/5369, of which J-b.1 is 488, plus the 43 counted calls. Pkg.test says
  "Adelefeld tests passed".
- Session (session.jl, session.log): ctx = ModCtx(blocks=[8, 9]); to_local(FBall(Rat(5), Rat(72)), ctx)
  prints (* ; 5 mod 72). ctx shows ModCtx(modulus=72, blocks=[8, 9]). is_local is true, context(y) ===
  ctx, modulus 72, and to_global(y) is identical to the global input.

Sixteen faults were planted, each in a fresh copy of the package (lanes/j-slice5/faults.py). The J-b.1
section was run alone on each (jb1_only.jl); the rows are in fault-summary-final.log. 14 are caught by
failed assertions:
- F2, value tokens never dropped: 30 failed.
- F5, duplicate tokens per state: 3.
- F6, lost written on failure: 7.
- F9, two families accepted: 6.
- F10, copy without a lease: 1 failed, 1 errored.
- F11, an adele without a lease: 6 failed, 1 errored.
- F13, a finalized ctx accepted: 14.
- F14, a block wrapped mod 2^64: 8.
- F15, finitepart without a lease: 1 failed, 1 errored.
- F16: 8.
- F1, no token taken: 8 failed, 7 errored, then a segfault.
- F3, the owner frees directly: 6 failed, then an abort.
- F4, off by one, frees at count 2: 2 failed, then an abort.
- F7, the conversion takes no lease on ctx: 34 failed, 14 errored, then an abort.
For F1, F3 and F4 the first failure is in the lease-basics testset (S38).

Two are not caught, and neither can be seen in this build:
- F8 returns a context on a refusal. Every write of the output pointer in src/modctx.c (:210, 226, 247,
  277, 294, 298, 345, 359, 378, 400) is on an ADF_OK path, so the slot stays NULL on a refusal.
- F12 drops a value's tokens before its C clear. adf_fball_clear (src/fball.c:249-259) reads the context
  only through ADF_INV_RELEASE, and that is ((void) 0) without -DADF_CHECK_INVARIANTS
  (src/invariants.h:107). An INV=1 build under ASan would be the oracle. The order of context proof
  step 4 is kept in the code anyway.

Not done: Scaled, cap, Place, LBall, SBall, context dumps, pruning of conservative roots (finding 47),
two-thread support and long differential fuzzing. Findings against the specification: none. Findings 40
to 43 are against the brief or the header wording. No source is pending for this step.
