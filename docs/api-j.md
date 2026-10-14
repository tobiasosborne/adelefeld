# Julia binding: design before code

Epic adf-c5e. This is a wrapper of the C contracts, not a second arithmetic implementation.
SPEC 4.1-4.4 and 10, conventions 12 and PLAN 1.9/5 remain fixed
(`docs/SPEC.md:148-299,739-780`; `docs/conventions.md:1553-1617`; `docs/PLAN.md:265,335-343`).
Declarations below are Julia interface notation, not package code. Each family has one common contract.
The C headers remain the contract for its mathematical result, allowed statuses, limits and cost.

## 1. Decisions and scope

J1-J3 are orchestrator decisions; J4-J8 below are this design's choices. Each is open to reversal.

- J1: `julia/Adelefeld/{Project.toml,src,test}`, package version 0.1.0, loaded by path, no registry.
  Production dependencies are standard libraries only. Nemo and FLINT_jll are optional weak dependencies
  of a later extension. The initial tested Julia series is 1.12; this machine has 1.12.5.
- J2: no C glue. Arithmetic calls use `ccall` through symbols from one `libadelefeld.so` handle.
  `ADELEFELD_LIB` selects the library; otherwise use `build/libadelefeld.so` relative to the repository.
  Load-time version and transcribed size/alignment checks precede every owning allocation.
- J3: transcribe every status as `@enum AdfStatus::Int32`, preserving the numbers and names in
  `include/adelefeld/status.h:17-33`. Offer status results and throwing methods; neither suppresses failures.
- J4: mutable Julia handles own aligned C heap storage. Inline Julia fields are a later performance option.
  One extra allocation buys an address independent of Julia object layout and a separate place for roots.
  The probe below measures correctness, not an allocation-speed comparison with inline storage or Nemo.
- J5: `try_` methods returning `AdfResult` are primary. Constructors, named methods and operators unwrap them
  and throw `AdfError` on every non-OK status. This makes refusal visible during exploratory arithmetic.
- J6: a value holds strong context references and leases on their Julia ownership states. A context is freed
  only when its owner and all leases are released; no assumption about finalizer order is needed.
- J7: plain real/complex results use private initialized Adele/CAdele carriers, with finite coordinate zero.
  Their first members are the required arb/acb. No raw FLINT init/clear call or mandatory Nemo is needed.
- J8: working precision is an explicit keyword, default 64 bits as in the driver. There is no mutable global
  precision setting. Operators use 64; named operations let the caller choose. Printing defaults to 20 digits.

The core five groups are J-a to J-e in section 9. They are maps of hour-sized steps, not five hour-long promises
to wrap 624 functions. Optional milestone S driver commands have a separate follow-up in that section.

## 2. Loading and ABI refusal

`__init__` resolves an absolute library path, calls `Libdl.dlopen` with NOW|LOCAL, and caches required function
pointers at runtime. Never serialize pointers into precompiled code. Keep the handle open until process exit;
finalizers use it. A missing symbol reports its name and the path. Do not search a second library after failure.
Installed Julia 1.12.5 help says "Load a shared library, returning an opaque handle."
(`lanes/d-julia/help.log:143-169`); dlsym returns a callable function pointer (`:178-186`).

Call `adf_version_check` and require OK; display `adf_flint_version_compiled` on failure
(`include/adelefeld/common.h:72-83`). This checks FLINT major/minor, not the adelefeld package version.
Require 64-bit pointers, Int64/UInt64 words, Csize_t=8 and Cint=4. Use Int64 for slong and UInt64 for ulong,
not an assumption that C long is 64 bits everywhere (`docs/conventions.md:1590-1594`).
For every wrapped adf type, query BOTH size and alignment and compare against independent literal rows.
Allocate the checked documented size, not a newly observed size that silently accepts an ABI change.
The first table is `docs/api-m1.md:31-45`; later rows are transcribed from the headers named here:

| C query suffix | Bytes | Alignment | Source |
|---|---:|---:|---|
| rat, fball, adele, cadele | 16, 48, 96, 144 | 8 | `docs/api-m1.md:35-39` |
| scaled, place, text_limits, ctx_desc | 40, 8, 32, 24 | 8 | `docs/api-m1.md:37,40-42` |
| text_kind | 4 | 4 | `include/adelefeld/text.h:366-369` |
| lball, sball | 48, 120 | 8 | `include/adelefeld/lball.h:74-75`; `sball.h:77-78` |
| ucoset, idele, idclass | 16, 80, 64 | 8 | `ucoset.h:42`; `idele.h:78-79`; `idclass.h:55-56` |
| qclass, char | 24, 120 | 8 | `docs/api-3.md:55`; `docs/api-3c.md:18` |
| ffun, rfun, rterm | 24, 16, 312 | 8 | `ffun.h:19`; `rfun.h:46-60`; `docs/api-4.md:11-15` |

The last three sizes follow by field packing on the fixed eight-byte ABI: ffun has three words; rfun two;
rterm has a three-word acb_poly and three 96-byte acb values (`refs/src/flint-3.0.1/acb_poly.rst:19-28`).
The optional solver follow-up adds its own rows.
The context is opaque; never allocate it from a guessed layout. RealBall/ComplexBall check their carrier rows.
Size/alignment do not prove field offsets. Avoid field reads except the stated first-member carriers; acceptance
adds independent offset sentinels and the C ABI suite. A changed equal-sized layout can evade a size check.
`text_kind` now has exported queries; the older absence in `lanes/m1-julia/report.md:36` is stale.

Compare all eleven `adf_status_str(i)` results to the transcribed enum names; also require UNKNOWN at -1 and 11
(`include/adelefeld/status.h:45-72`). The first UNKNOWN after contiguous valid codes learns the effective count.
It cannot verify the macro ADF_STATUS_COUNT itself: changing only that macro leaves every exported call unchanged.
Small prerequisite J-status, in a C-owned lane: add exported `int adf_status_count(void)` returning that macro,
with a header comment, export check and test. It is a core accessor, not glue. Full J3 loading additionally
requires its result to equal 11 and `length(instances(AdfStatus))`. Without it the package must report the missing
gate; it must not claim that the present 624-symbol core exposes the macro. No C change is made by this design.
Other macros are copied only when needed, with a header citation and boundary test. Defaults for text limits
come from `adf_text_limits_default`, not duplicate literals (`include/adelefeld/text.h:105-115`).

On this machine, unpreloaded dlopen fails before version checks with missing `__gmpn_modexact_1_odd`.
Julia already loaded its bundled GMP; the system FLINT needs the system GMP's symbol. The existing retry and
the limits of DEEPBIND are documented in `tests/julia/README.md:39-69`; the probe reproduces the failure.
Under J1/J2, plain `using Adelefeld` in that already-started process cannot be promised to work. Setting
LD_PRELOAD in `__init__` is too late. Do not inject a second GMP, silently respawn Julia, or change its runtime.
J-a supplies `bin/adelefeld-julia`, following `tests/test_julia.sh`: a pre-startup launcher which sets the system
GMP preload on this diagnosed environment, then execs Julia. A plain `using Adelefeld` works inside that process.
An ordinary launch gets a clear load error with the exact preload command. A compatible artifact-linked core is
the later alternative; private linker namespaces and their safety are not designed here.

## 3. Values, lifetime and calls

Use distinct Rat, FBall, Adele, CAdele, Scaled, LBall, SBall, Ucoset, Idele, IdClass, QClass, Character,
FFun and RFun handles. Their public values satisfy their C canonical predicates. Private storage contains
base pointer, aligned pointer, lifecycle state, and context roots/leases; it is never exposed as a raw pointer.
Rat/FBall/Adele/CAdele subtype Number with only the methods explicitly specified here; do not infer
ordering, approximate equality, Complex scalar coercion, or a field structure from that subtype.
Base overloads dispatch only on binding types and their stated scalar pairs; never define untyped Base methods.
RealBall/ComplexBall are distinct numerical outputs, not adeles (`docs/SPEC.md:169-175`).

Allocate n+a-1 bytes using Libc.malloc after checked size arithmetic; reject NULL with OutOfMemoryError.
For power-of-two alignment a, p=(base+a-1)&~(a-1) in unsigned address arithmetic. Check p%a=0 and p+n<=base+n+a-1.
This is a proof of alignment and containment, independent of malloc's alignment. Free BASE with Libc.free.
Call adf init once, then register a finalizer; registration failure immediately clears and frees.
Never copy C bytes as an owning value. `copy` calls adf set on freshly initialized storage; each has its own clear.
Arrays owned by C stay owned by that enclosing value. Borrowed Julia input arrays are rooted and never retained.

The private release operation claims LIVE once, calls the cached void adf clear, frees base, then releases
context leases. Repeated close/finalize is a no-op. Explicit close must not race with use. Closed handles give
InvalidStateException before C. A finalizer does no I/O, scheduling, symbol resolution or status translation.
Installed Julia 1.12.5 help says "f must not cause a task switch" (`lanes/d-julia/help.log:10-13`).
Unexpected Julia exceptions increment an atomic finalizer-error counter and leave the allocation and leases
unreleased; the next explicit diagnostic reports this leak. Never retry an uncertain clear or free a context
whose value may still be live. C aborts/signals cannot be converted to Julia exceptions; preconditions matter.
Clear has no status to ignore (`include/adelefeld/rat.h:45-46`; `fball.h:83-84`).

Every pointer-producing expression and its ccall are inside `GC.@preserve` for output, every input owner,
context owners, Ref outputs, input strings and array owners. Preserve the owners, not only Ptr objects.
Julia 1.12.5 help explicitly includes "Using resources of x which would be cleaned up in the finalizer."
(`lanes/d-julia/help.log:48-60`). For example the non-failing Rat sum's ABI is:

```julia
GC.@preserve z x y begin
    ccall(f_rat_add, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), z.data, x.data, y.data)
end
```

Public arithmetic returns fresh owners. No public arithmetic mutators in 0.1; internal C alias tests still run.
Projection returns a copy, never an independently finalized pointer to a member. Raw prefix borrows exist only
inside a preserve block and only for signatures accepting that FLINT type; never pass a member as a whole adf.

Context proof, implementing `docs/conventions.md:342-397,1582-1583`:
1. A successful ModCtx constructor creates state (ptr, atomic count=1), one token owned by the context handle.
   Refuse finalized ModCtx constructor arguments; copying a live value may still clone its existing lease.
2. Before a value can borrow it, reserve one token per distinct state and retain the actual ModCtx in its roots.
   Constructor failure releases reserved tokens. The C pointer slot starts NULL and is read only on OK.
3. A fresh result reserves the union of input/explicit-binding context states before C; no C operation creates
   a hidden context. Conservative roots may retain a context after global fallback; they never retain inputs.
4. A value drops its tokens only AFTER its whole C clear. The ModCtx finalizer drops the owner token.
5. Only the atomic decrement seeing old count=1 calls adf_modctx_free. Before that all borrowers have cleared.
   Thus either finalizer order is safe and exactly one thread frees the context. C's immutable state is unchanged.
No public close of ModCtx in 0.1. A copy reserves its own tokens. QClass owns all piece storage under one clear
and holds every binding root, including repeated dump occurrences. Conservative retention can accumulate
distinct unused contexts in long expression chains; prune only after a verified C-accessor traversal later.

RealBall owns an initialized Adele with fin=0; ComplexBall owns an initialized CAdele with fin=0.
Use the first-member arb/acb address only as an independent numerical output; clear the whole carrier exactly
once. Set them from value text `(R ; 0)` / `(Z ; 0)` and project through the C getters. Internal fmpq storage
can similarly use a Rat's first member; an integer fmpz uses its numerator with denominator fixed to 1
(`docs/api-m1.md:35,38-39`; `docs/conventions.md:1598-1617`). Never expose these carriers as mathematical adeles.
This consumes extra zero storage but obeys J2 and works without Nemo. No FLINT_jll is loaded in the base package.

## 4. Status, text and the first callable surface

All following families check live owners and representable scalar/byte counts before C. Invalid Julia argument
types give MethodError/ArgumentError; out-of-range conversion gives an explicit argument error, never word wrap.
Once a C call runs, its exact status is retained. A void operation synthesizes OK, documented as such.
C costs below exclude Julia allocation and copying text; the binding adds O(number of roots) lifetime work.

```julia
# Plain C int values 0..10, named exactly as status.h:17-33; no bit flags.
@enum AdfStatus::Int32 begin
    OK=0; NOT_DETERMINED=1; UNIT_NOT_CERTIFIED=2; NEEDS_SPLIT=3; NOT_UNIQUE=4; NO_SOLUTION=5
    NOT_UNIT=6; DOMAIN=7; UNSUPPORTED=8; PARSE=9; LIMIT=10
end
# status always exists; value only when the C header certifies that output on this status.
# where is a certified failing place when supplied, else nothing; details holds lost/count/certificate data.
# Ordinary failure clears the private temporary and returns value=nothing. No dummy zero is returned as a result.
AdfResult{T}(status::AdfStatus, value::Union{T,Nothing}, where::Union{Place,Nothing}, details::NamedTuple)
# Carry the whole result and operation, including status and a COPIED adf_status_str message.
# Any non-OK unwrap throws this; unknown numeric status is an ABI error retaining its raw Cint, not coerced to OK.
AdfError <: Exception
unwrap(r::AdfResult)  # return r.value only on OK; throw AdfError otherwise

# Typed C set_str, limits default from C. Input String is copied if needed from AbstractString;
# ncodeunits, not characters, is the byte count. NUL/non-ASCII go to C and retain PARSE/LIMIT check order.
# Working precision only for types whose set_str has it; OK gives a new canonical owner, global for value text.
# Cost C reader plus byte copy; failures retain the parser's actual status. No Julia grammar substitute.
# T is a value-form kind or a framed numerical carrier. Scaled/ModCtx use explicit constructors and dumps.
try_parse(::Type{T}, s::AbstractString; prec=64, limits=TextLimits()) -> AdfResult{T}
Base.parse(::Type{T}, s::AbstractString; kwargs...) -> T
# classify first for untyped text; kind output read only on OK; enum representation checked at load time.
# Cost O(bytes); stages and statuses exactly text.h:137-143. Supported kind dispatches to its typed parser.
try_readvalue(s; prec=64, limits=TextLimits()) -> AdfResult
readvalue(s; kwargs...) -> AdfValue

# C get_str, byte length Ref{Csize_t}; NULL/length=0 becomes LIMIT, as the driver does.
# Copy exactly len bytes with unsafe_string, check terminator in tests, always adf_str_free in finally.
# digits in 1..10^6; validation happens before C. Statusless printer success synthesizes OK.
# Cost C printer plus O(len) copy. Static status/version strings are copied but NEVER freed.
try_value_text(x; digits=20) -> AdfResult{String}
value_text(x; kwargs...) -> String
Base.show(io::IO, x)  # write value_text using :adf_digits from IOContext, default 20
# show throws on printer refusal; no decimal string is claimed to be lossless.

# Exact integer/Rational constructors use C set_si when Int64 fits; otherwise decimal rational set_str.
# No reinterpretation of Julia BigInt's GMP storage. Allow explicit larger TextLimits for large exact inputs.
# Rat(n,d) normalizes negative d by exact BigInt sign changes, then uses C text; den=0 gives DOMAIN.
# FBall(q) is radius zero through set_rat; FBall(c,N) calls set_center_radius, negative N gives DOMAIN.
# Adele(q;prec) calls set_rat; Adele(r::RealBall,f::FBall) calls set_arb_fball, requiring independent storage.
# Statusless setters synthesize OK; other statuses/costs are their headers' constructor contracts.
try_rat(n::Integer, d::Integer=1; limits=TextLimits()) -> AdfResult{Rat}
Rat(n::Integer, d::Integer=1; kwargs...) -> Rat
try_fball(c::Rat, N::Rat) -> AdfResult{FBall}
FBall(q::Rat); FBall(c::Rat, N::Rat)
Adele(q::Rat; prec=64); Adele(r::RealBall, f::FBall)
CAdele(q::Rat; prec=64); CAdele(a::Adele); CAdele(z::ComplexBall, f::FBall)

# Rat/Rat and FBall/FBall use their add/sub/mul/neg; Adele/CAdele pass prec, neg has no prec.
# Rat mixed with FBall uses radius-zero conversion for add/sub and mul_rat for scaling.
# Rat mixed with Adele/CAdele uses *_rat, preserving exact finite data; real conversion only at prec.
# Adele mixed with CAdele embeds via set_adele. Every other mixed ring pair is refused, not guessed.
# No input changes. Cost inherited from rat.h:100-116, fball.h:192-233, adele.h:136-164,196-204.
try_add(x,y;prec=64); try_sub(x,y;prec=64); try_mul(x,y;prec=64); try_neg(x)
add(x,y;kwargs...); sub(x,y;kwargs...); mul(x,y;kwargs...); neg(x)
Base.:+(x,y); Base.:-(x,y); Base.:*(x,y); Base.:-(x)
# Divide Rat/FBall/Adele/CAdele by Rat only; zero divisor returns NOT_UNIT. Rat inv likewise.
# No division by a finite ball or an Adele. Later Idele/LBall division uses its separate C contract.
# Cost the appropriate div_rat or rat_div; fresh output is destroyed on failure.
try_div(x,q::Rat;prec=64); divexact(x,q;kwargs...); Base.:/(x,q::Rat)
try_inv(q::Rat); Base.inv(q::Rat)

# Set equality/overlap and FIRST INSIDE SECOND, matching fball.h:236-265, not Base.contains text semantics.
# Rat inputs become points. Point comparison returns PointComparison with EQUAL/DIFFERENT/UNDECIDED, not status.
# Predicate int 0/1 is Bool; no AdfStatus cast. Copy and identical use their C set/identical, never memcpy.
# No == meaning "the unknown points are equal" on balls: use compare_points. Rat == uses rat_equal.
equal_set(x::FBall,y::FBall); overlaps(x::FBall,y::FBall); inside(x::FBall,y::FBall)
compare_points(x::FBall,y::FBall) -> PointComparison
center(x::FBall) -> Rat; radius(x::FBall) -> Rat; finitepart(x::Adele) -> FBall
Base.copy(x); identical(x,y); Base.:(==)(x::Rat,y::Rat)
```

Arithmetic operators accept Integer and Rational scalars by explicit Rat conversion; do not convert Float64,
BigFloat or Complex implicitly. Use RealBall/ComplexBall text constructors for numerical coordinates.
Define ball `==` to raise ArgumentError directing the user to equal_set/compare_points; ordering is unavailable.
Use identity-based isequal/hash for ball handles and value-based isequal/hash for Rat, consistent with ==.
The printed real interval encloses the stored ball; reparsing may widen it (`docs/conventions.md:1331-1343`).
Printing RealBall/ComplexBall uses the carrier C printer and removes only its fixed outer ` ; 0` frame.
Input uses that same frame. Their arithmetic uses carrier add/sub/mul with zero finite parts.
For numerical set predicates, reframe C-generated carrier dumps as exact arch-only SBall dumps, then use its
C queries once J-b lands; there is no raw complex SBall setter. No decimal round trip or midpoint comparison.

## 5. Contexts, places and policies

```julia
# Bits type of one UInt64, passed BY VALUE. Inner construction is private; do not infer its encoding.
# infplace wraps adf_place_inf; primeplace uses Ref{Place}, primality status retained, read Ref only on OK.
# p must fit UInt64 before C; cost one C primality test. Readers/order use place.h:40-59.
struct Place; opaque::UInt64; end
infplace() -> Place; try_primeplace(p::Integer) -> AdfResult{Place}; primeplace(p::Integer) -> Place
is_archimedean(v::Place); prime(v::Place); Base.isless(v::Place,w::Place)
# Explicit constructors, fresh NULL pointer slot; only OK creates a ModCtx owner/token.
# Arrays copied and preserved; no interning by modulus, since equal moduli are not equal context pointers.
# Domain/unsupported checks and costs exactly modctx.h:58-111; lifetime is section 3's token proof.
try_modctx(;blocks,prime_powers,modulus,factorial,primorial_power) -> AdfResult{ModCtx}
ModCtx(;kwargs...) -> ModCtx  # require exactly one construction family
# Exact local conversion and explicit enclosing variant; lost is Bool in details ONLY on OK.
# to_global copies the set; Scaled(ctx,x) encloses and reports loss, keeps exact tags.
# Binary Scaled operations require the same ctx POINTER, including exact operands; no automatic lcm context.
# mul_tight is separate. Cap never widens an exact value. Costs/statuses are scaled.h:94-180, modctx.h:145-174.
try_to_local(x::FBall,ctx::ModCtx); try_enclose_local(x::FBall,ctx::ModCtx)
to_global(x::FBall); try_scaled(ctx::ModCtx,x); try_recontext(x::Scaled,ctx::ModCtx)
try_mul_tight(x::Scaled,y::Scaled); try_cap(x::FBall,C::Rat)
# LBall(place,c,N) and project(x,places) call lball/sball constructors; no missing prime is filled with Z_p.
# Copies of components have independent storage and their own roots. Preserve the certified Place vector.
# Retain all status/where/precision data; finite precision means absolute p-adic digits, real prec means bits.
try_lball(v::Place,c::Rat,N::Integer); try_project(x,places::AbstractVector{Place};prec=64)
LBall(v,c,N); project(x,places;kwargs...); component(x::SBall,v::Place)
# Dump inspection validates fully before constructing missing contexts; binds count occurrences in traversal
# order, not unique moduli. Default creates one context per descriptor; explicit bindings permit identity.
# Freeze/reserve roots before load; every failure releases temporary owners and keeps input bindings alive.
# Dump is lossless within C limits; decimal show is not. Cost/status exactly dump.h, SPEC 10.2.
try_dump_text(x); dump_text(x)
try_load_dump(::Type{T},s;bindings=nothing,limits=TextLimits()) -> AdfResult{T}
load_dump(::Type{T},s;kwargs...) -> T
```

The local numerator residues and denominator follow C unchanged. Same-modulus separate ModCtx objects remain
distinct; finite arithmetic may fall back global, scaled arithmetic returns DOMAIN. Acceptance uses both cases
(`docs/SPEC.md:184-214,285-298`). No hand write of a C context pointer is allowed.

## 6. Remaining surface and driver sessions

Every named operation below has `try_name` and throwing `name` forms under section 4. Constructors parse C
value text or call typed setters. Dispatch selects a documented C function, never a mathematical fallback.
Tuple outputs commit together; auxiliary booleans/counts are read only on the statuses their headers allow.
`where` needs an initialized sentinel and header-specific interpretation: some failures leave it untouched.
Supply a certified sentinel not equal to any candidate failing place; unchanged means nothing, not infinity.

All source ranges in this table refer to `tools/adf/README.md`; headers are assigned once in section 9.
| Driver commands | Julia surface | Source lines |
|---|---|---|
| show/type, ring arithmetic, set queries | Section 4 | 52-90,321-344 |
| reconstruct, cap, dump, load | reconstruct(x,lo,hi), reconstruct(a), cap, dump_text/load_dump | 92-137 |
| project | project(x,places), component(s,v) | 214-271 |
| idele, class, hull/hullsimple, unitof | Idele(q), IdClass(x), hull(x;simple=false), unitof(a) | 346-377 |
| inv, pow/powtight, norm, valuation, abs | inv, pow(x,k;tight=false), norm, valuation, abs_at | 357-365 |
| qreduce/qneg/qadd, qadd_rat | reduce_class, neg_class, add_class, translate_class | 723-770 |
| qequal/qcontains/qoverlaps | equal_set/inside/overlaps with work_limit | 807-822 |
| psi/psi_strict, psi_at/psi_strict_at, psi_phase | psi/psi_at(...;strict=false), phase | 772-805 |
| char/chi/gauss/conj/unit/eval/strict | Character, chi, gauss_sum, root_number, conj, evaluate | 824-915 |
| local series, roots_at/root_at, powers_at | Same names with Place, N, seed | 463-546 |
| root/series/Log/logabs/refinements | root(...;sign), series, Log, log_abs, Log_at, Log_refine | 547-653 |
| legendre/jacobi/kronecker/hilbert_at | Same names, Place for local symbols | 655-676 |
| binom/binomtight, profpow variants | binomial_ball(...;tight), profpow(...;mode) | 677-691 |
| haar_volume, cyclo_exp_u/uinv | haar_volume, cyclo_exp(...;convention) | 692-702 |
| ffun/rfun, algebra/transforms | FFun/RFun, add/mul, translate/dilate/reflect/conj/fourier/derivative | 844-948 |
| evaluations, integrals, norm2, poisson | evaluate, integral, norm2, tensor_*, poisson(...;bits,prec) | 917-985 |
| local_zeta_factor_at, tate_local/gamma/epsilon | Same factor names, s,v,alpha,eta0 | 704-721,987-1022 |
| tate vector/value/continue/completed/fe | tate_vector(c), tate_*(c,s;bits,prec), tate_fe | 963-985,1024-1058 |
| roots, realroots, recover | roots_padic(...;depth), roots_real(...;prec), recover(...;limit) | 144-212 |

The table is a map, not a driver-line interpreter. Settings become keywords or IOContext; `with` becomes an
argument separator. The arithmetic example is `parse(FBall,"3 mod 12") + parse(FBall,"5 mod 18")`.
It prints `(* ; 2 mod 6)` (`docs/SPEC.md:242-246`). `inside(x,y)` retains the driver's first-inside-second order.
PointComparison is separate from AdfStatus. Strict character and psi methods cannot silently call hull variants.
Use ComplexBall for spectral s/alpha, RealBall for real evaluation inputs; the finite coordinate is not an input.
`try_poisson` returns (left,right,NL,NR); `try_tate_vector` returns (phi,f), both as one result.
`try_tate_fe` computes A, B at 1-s with conj(chi), W and R=W*B independently, following the driver's guard bits.
Return (A,R,overlap,s,bits); preserve the first failing step in details. No copy/intersection of A and R.
Exact radius/width checks decode ONLY C-generated carrier dumps as dyadic integers, including both coordinates
(`docs/conventions.md:1392-1394`; `tools/adf/README.md:1037-1058`). This is an enclosure check, not L evaluation.

Thread contract: C functions are reentrant; concurrent reads of values and immutable contexts are allowed;
writing/closing requires exclusive use (`docs/conventions.md:330-340`). Pure fresh-result operations satisfy
this rule; sharing a handle then closing it during a call violates it. Never set FLINT's thread pool implicitly.
FLINT's local source says "If FLINT is built in threadsafe mode, most caches are thread-local"
(`refs/src/flint-3.0.1/memory.rst:29-32`). Do not claim unconditional FLINT thread safety from that sentence.
[source pending: verification of the linked FLINT build's threadsafe configuration]
Initial supported execution is one Julia thread; a two-thread stress test and that build check gate widening.
FLINT recommends current-thread cleanup and one master cleanup before exit (`memory.rst:34-44`). The package
does not run master cleanup in value finalizers or an exit hook: Julia/Nemo may still own FLINT data.
Process caches are separate from handle leaks. J2 exposes no adelefeld cleanup wrapper; host cleanup is deferred.

## 7. Optional Nemo bridge

Design `ext/AdelefeldNemoExt.jl`, triggered by Nemo and FLINT_jll weak dependencies; no base-package import.
[source pending: installed Julia extension Project.toml trigger/weakdep documentation]
Julia 1.12.5 help says get_extension returns the module "or return nothing if the extension is not loaded"
(`lanes/d-julia/help.log:199-203`). Test base loading with those packages absent first.
Nemo's mathematical fmpq/arb/acb objects are intended as direct FLINT pointer inputs; names of its concrete
Julia types and supported pointer conversion must come from its source, not a guessed `pointer_from_objref`.
[source pending: Nemo source, exact version, fmpq/arb/acb layouts, pointer conversion and finalizers]

```julia
# Extension-only, ABI-gated copies via rat_set_fmpq / adele_set_arb_fball / cadele_set_acb_fball.
# Preserve the Nemo owner, its parent and all binding owners; borrow its FLINT pointer for this call only.
# Never attach an Adelefeld finalizer to Nemo storage, clear it, or memcpy a tagged fmpz as a new owner.
# Status/cost are the C setter's; no loss except the explicitly requested numerical conversions.
try_from_nemo(::Type{T},x) -> AdfResult{T}; from_nemo(::Type{T},x) -> T
# Allocate a Nemo output using its own verified constructor, then C get_fmpq/get_real/get_complex into it.
# Output cannot alias an Adelefeld member. Nemo owns its clear; Adelefeld retains no pointer after return.
to_nemo(x::Union{Rat,RealBall,ComplexBall})
```

Gate the bridge on matching FLINT major/minor, sizeof/alignment/offsets of fmpq=16, arb=48, acb=96, word widths,
and the SAME loaded FLINT/GMP instances and allocator configuration. Version strings alone are insufficient.
Check actual loaded library identities and the pointer methods against the selected Nemo version's source.
Refuse the bridge if identity cannot be proved; do not guess tagged-integer ownership across two FLINTs.
An artifact selection requires a libadelefeld built against that compatible artifact and ADELEFELD_LIB before
startup. The extension never switches the base library after objects exist. Its build/origin checks remain
conditional until that source is obtained; no Nemo test was run here and no installation is requested.

## 8. Acceptance, faults and present evidence

Every future step starts with its failing tests and ends with a bounded Pkg.test and the session in section 9.
Test success and every allowed refusal against C/reference fixtures, including useful outputs on non-OK when
the header specifies them. `AdfError` preserves the same status, place and details as its try_ result.
Use separate subprocesses for load rejection: wrong size, alignment, status name/count, missing symbol,
FLINT mismatch and missing GMP symbol. No adf init may run after a load rejection. A test library may falsify
queries; it is a test fixture, not production glue. Verify ASCII byte counts, NUL rejection, NULL printer,
string-free counts even after copying/printing errors, decimal enclosure versus lossless dump identity.

J-a: reproduce SPEC's five arithmetic rows, fractional/zero radii, Rat 1/3 staying exact, q-x sign, x-x as
independent balls, den=0 DOMAIN versus divide-by-zero NOT_UNIT. Inputs unchanged; copies clear separately.
J-b: both context finalizer orders, forced GC after original owner drops, copy/projection/nested dump roots,
repeated bindings, two equal-modulus different-pointer contexts, scaled DOMAIN, local global fallback, lost
flags and exact cap exemption. Test atomic token count/free once and deliberately repeated close/finalize.
J-c: port character/quotient phases and strict refusals; distinguish characters from complex adelic values.
J-d: numerical results enclose fixture values over whole balls, with exact dump-derived widths; both parities,
non-real chi, exact versus mixed poles, independent functional equation sides. Keep original C oracles.
J-e: absent extension load, mismatched ABI refusal, large tagged fmpq and nonzero arb/acb radii through copies,
owner collection during calls, returned Nemo values surviving Adelefeld GC. No skip counts as bridge acceptance.
Faults to plant across these steps: omit preserve; double clear; free adjusted pointer; drop a context token;
use character count for UTF-8 bytes; free output with Libc; read failure output; cast predicate to status;
reverse inside; narrow printed intervals; ignore status; accept mismatched Nemo library; copy FE's second side.

Executed in this lane, Julia 1.12.5, one thread, no external Julia packages:
`timeout 600 sh tests/test_exports.sh` -> 624 declared/exported, 0 missing, 0 extra, 0 variadic.
`timeout 180 env LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia --startup-file=no --threads=1
lanes/d-julia/probe.jl build/libadelefeld.so` -> exit 0; logs in `lanes/d-julia/probe.log`.
100000 init/clear/malloc/free pairs; 200000 add/mul calls; 100000 canonical and independent Rational{BigInt}
equality checks; 100000 output-string allocations/frees; 100000 aligned pointers; 40 GC calls; 10 repeated
finalize checks; 0 live handles/buffers; arithmetic/GC loop 7.271 seconds. No crash or unmatched ownership clear.
The operands have a 151-bit numerator and 127-bit denominator; this exercises heap integers, not only small tags.
The separate abi_probe.jl exits 0: 19 layout rows/38 comparisons, 11 status names, 2 invalid-status sentinels,
1 version check; adf_status_count is absent. Without preload, the Rat probe exits 1 before allocating values.
This is a bounded ownership/GC smoke probe, not a proof that FLINT has no internal leak or a fuzzing campaign.
The context scheme, carrier scheme, package load gates and extension are acceptance obligations, not probed here.

Findings against the specification: no counterexample found to the sections used. Interface gap: the status
count macro cannot be verified by the present exports; J-status is required for the literal J3 gate.
The historical report's missing text_kind queries have been repaired in the current header, not by this lane.
Sources pending are exactly the three marked obligations in sections 6 and 7. Inline performance, automatic
artifact builds, host cache cleanup, full solver input adapters and untested platforms are not settled here.

## 9. Thin slices, in implementation order

Each semicolon-separated step below is a separate lane of about an hour; do not widen before its session works.
Run `timeout 180 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 julia --project=julia/Adelefeld
-e 'using Pkg; Pkg.test()'` after every step, with the pre-startup GMP preload and a writable depot if sandboxed.
This command is a future gate; no package exists yet and Pkg.test has not been run by this design lane.

- J-status (C-owned prerequisite): status.h accessor/export/test; then J-a can honestly check ADF_STATUS_COUNT.
- J-a: skeleton/load/Rat/statuses; FBall and scalar mixed arithmetic; Adele/CAdele and numerical carriers;
  text/dump/reconstruction. Headers common,status,rat,fball,adele,place,text,recon,dump. Modctx is read for roots,
  with local constructors added in J-b. Supersedes layouts.jl and smoke.jl's core testsets, not its solver includes.
  Keep an explicit per-symbol inventory as methods land. Session after Pkg.test:
  `using Adelefeld; parse(FBall,"3 mod 12") + parse(FBall,"5 mod 18")` -> `(* ; 2 mod 6)`.
- J-b: contexts/leases/local backend; Scaled/cap; places/LBall; SBall/projection/local text and dumps.
  Headers modctx,scaled,place,lball,sball,text,dump. Supersedes lball.jl,lball2.jl,sball.jl,text_local.jl.
  Session after Pkg.test: `project(Adele(Rat(1)),[primeplace(5)];prec=64)` -> `{p=5: 1}`.
- J-c: unit cosets/ideles/classes and powers; QClass reduction/sets/arithmetic; psi; Character and evaluations.
  Headers ucoset,idele,idclass,idmap,idpow,qclass,psi,char,text,dump. Supersedes ideles.jl,idclass.jl,idmap.jl,
  text_idele.jl,qclass.jl,qclass_text.jl,qclass_sets.jl,qclass_arith.jl,psi.jl,psi_class.jl,
  char.jl,char_eval.jl,char_class.jl. Session after Pkg.test:
  `root_number(parse(Character,"char(q=8, n=7, s=(0) + (0)*i)"))` -> `(1) + (0)*i`.
- J-d: local/all-place functions; symbols/catalogue/local factor; FFun algebra/transform; RFun; tensor/Poisson;
  local Tate; global value; continuation/completion/FE. Headers lfunc,rfunc,lroot,lpow,gfunc,symbol,catalogue,
  localfactor,ffun,rfun,tensor,tate. Supersedes lfunc.jl,lfunc_trig.jl,lroot.jl,lpow.jl,f_at.jl,gfunc.jl,
  gfunc_log.jl,symbol.jl,catalogue.jl,localfactor.jl,ffun.jl,ffun_algebra.jl,ffun_dump.jl,rfun.jl,
  rfun_fourier.jl,tensor.jl,poisson.jl,tate_local.jl,tate_gamma.jl,tate.jl,tate_continue.jl,tate_fe.jl.
  Session after Pkg.test: `tate_completed(parse(Character,"char(q=4, n=3, s=(0) + (0)*i)"),
  parse(ComplexBall,"(-1) + (0)*i");bits=80,prec=160)` encloses 1.1662436161232751, as tate_fe.jl does.
- J-e: source/ABI identity gate; fmpq copy; arb/acb copy; optional artifact selection and extension tests.
  Headers common,rat,adele; supersedes no base fixture, adds its own Nemo tests in a preprovisioned environment.
  Session after Pkg.test with compatible Nemo: `to_nemo(from_nemo(Rat,nemo_q)) == nemo_q` -> true.
  The fixture must define nemo_q with a numerator above 2^100; concrete Nemo constructor names are source pending.

Follow-up J-S is required for the last driver-table row, not a hidden part of the core or a Nemo requirement.
Headers resid,roots,linsolve; fixtures resid.jl,roots.jl,linsolve.jl remain raw C tests until it lands.
Design borrowed const polynomial/matrix inputs from initialized integer owners, or request core input adapters;
never clear a borrowed tagged integer array. These input layouts and output-on-status exceptions need their own
design before code: resid.h:186-192 writes cert on failure; linsolve.h:126-131 writes a report on NO_SOLUTION.
Its exit is bounded Pkg.test plus `recover(parse(FBall,"2 mod 101"),Rat(2),Rat(1);limit=1000)` -> Rat(2).
Do not advertise complete driver parity until J-S is designed, implemented and tested.
