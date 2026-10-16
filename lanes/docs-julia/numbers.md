# lanes/docs-julia/numbers.md: the commands and outputs behind every number of the two pages

Every command is run from the worktree root `/home/tobias/Projects/adelefeld-wt/docs-julia` on 2026-10-16,
Julia 1.12.5, one thread, after `mkdir -p lanes/docs-julia/tmp lanes/docs-julia/depot`. Each output below is
copied verbatim from the file named beside its command by `python3 lanes/docs-julia/make_numbers.py`; the
files are in this directory.

## 1. Build

    timeout 600 make -j2                  # output: lanes/docs-julia/build.log (last two lines below)

```text
ar rcs build/libadelefeld.a build/adele.o build/cap.o build/catalogue.o build/char.o build/common.o build/dump.o build/fball.o build/fball_local.o build/ffun.o build/gfunc.o build/gfunc_log.o build/idclass.o build/idele.o build/idmap.o build/idpow.o build/inlines.o build/lball.o build/lball_decomp.o build/lfunc.o build/linsolve.o build/localfactor.o build/lpow.o build/lroot.o build/modctx.o build/place.o build/poisson.o build/psi.o build/qclass.o build/qclass_arith.o build/qclass_sets.o build/rat.o build/recon.o build/resid.o build/rfun.o build/rfunc.o build/roots.o build/roots_real.o build/sball.o build/scaled.o build/status.o build/symbol.o build/tate.o build/tensor.o build/text.o build/text_idele.o build/ucoset.o
exit 0
```

`make -j2` builds `build/libadelefeld.a` only; the shared object comes from the export check:

    timeout 600 sh tests/test_exports.sh  # output: lanes/docs-julia/exports.log (last line below)

```text
test_exports: passed: 625 of 625 declared functions are exported, 0 are not implemented yet, no exported name is undeclared, no variadic function
```

## 2. The tutorial (julia/Adelefeld/README.md, steps 1 to 8)

    timeout 300 env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true JULIA_DEPOT_PATH="$PWD/lanes/docs-julia/depot:$HOME/.julia:" \
    TMPDIR="$PWD/lanes/docs-julia/tmp" \
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld lanes/docs-julia/tutorial.jl \
    > lanes/docs-julia/tutorial.out 2>&1

Exit status 0. A second run of the same command into `lanes/docs-julia/tutorial.rerun.out` is byte-identical
(`cmp` prints nothing). The README's REPL blocks are written from `tutorial.out` by
`python3 lanes/docs-julia/fill_readme.py` and compared with the second run by `python3 lanes/docs-julia/check.py`
(section 6). The whole output:

```text
#### block 1-rationals
julia> using Adelefeld

julia> Rat(6, 35) + Rat(1, 7)
11/35

julia> Rat(1) / 3
1/3

julia> Rat(2, -4)
-1/2

julia> Rat(1, 3) * 3 == 1
true

julia> Rat(6, 35) == 6//35
true

julia> Rat(1, 0)
ERROR: AdfError: rat returned status DOMAIN

#### block 2-finite-balls
julia> x = FBall(Rat(146), Rat(1000))
(* ; 146 mod 1000)

julia> center(x), radius(x)
(146, 1000)

julia> parse(FBall, "3 mod 12") + parse(FBall, "5 mod 18")
(* ; 2 mod 6)

julia> x - x
(* ; 0 mod 1000)

julia> FBall(Rat(1, 3)) * 3
(* ; 1)

julia> a = parse(FBall, "1 mod 6"); b = parse(FBall, "1 mod 3");

julia> inside(a, b), inside(b, a)
(true, false)

julia> overlaps(a, parse(FBall, "4 mod 9")), equal_set(a, parse(FBall, "7 mod 6"))
(true, true)

julia> compare_points(a, b)
UNDECIDED::PointComparison = 2

julia> compare_points(a, parse(FBall, "0 mod 2"))
DIFFERENT::PointComparison = 1

julia> compare_points(FBall(5), FBall(5))
EQUAL::PointComparison = 0

#### block 3-adeles
julia> u = Adele(Rat(1, 3))
(0.33333333333333333332 +/- 3.2e-20 ; 1/3)

julia> realpart(u)
0.33333333333333333332 +/- 3.2e-20

julia> finitepart(u)
(* ; 1/3)

julia> v = parse(Adele, "(1.5 +/- 0.5 ; 0 mod 2)")
(1.5 +/- 0.5 ; 0 mod 2)

julia> u + v
(1.83 +/- 0.51 ; 1/3 mod 2)

julia> u * 3
(0.99999999999999999995 +/- 8.6e-20 ; 1)

julia> RealBall(Rat(1, 3); prec = 200)
0.33333333333333333333 +/- 3.4e-21

julia> z = parse(ComplexBall, "(1.5) + (-2)*i")
(1.5) + (-2)*i

julia> CAdele(z, FBall(Rat(1), Rat(4)))
((1.5) + (-2)*i ; 1 mod 4)

julia> CAdele(u) * Rat(2)
((0.66666666666666666663 +/- 5.5e-20) + (0)*i ; 2/3)

julia> Adele(RealBall(Rat(2)), parse(FBall, "1 mod 4"))
(2 ; 1 mod 4)

julia> parse(RealBall, "1.5 +/- 0.0009765625")
1.5 +/- 0.00098

#### block 4-reconstruction
julia> reconstruct(parse(Adele, "(1.5 +/- 0.5 ; 0 mod 2)"))
2

julia> reconstruct(parse(FBall, "2 mod 101"), Rat(-50), Rat(50))
2

julia> reconstruct(parse(FBall, "52 mod 101"), Rat(-50), Rat(50))
-49

julia> try_reconstruct(parse(Adele, "(1.5 +/- 2 ; 0 mod 2)")).status
NOT_UNIQUE::AdfStatus = 4

julia> try_reconstruct(parse(Adele, "(1.5 +/- 0.25 ; 0 mod 2)")).status
NO_SOLUTION::AdfStatus = 5

julia> try_reconstruct(FBall(Rat(1, 3)), Rat(0), Rat(1)).value
1/3

#### block 5-statuses
julia> r = try_div(Rat(1), Rat(0))
AdfResult{Rat}(Adelefeld.NOT_UNIT, nothing, nothing, NamedTuple())

julia> r.status, r.value
(Adelefeld.NOT_UNIT, nothing)

julia> unwrap(r)
ERROR: AdfError: unwrap returned status NOT_UNIT

julia> Rat(1) / 0
ERROR: AdfError: div returned status NOT_UNIT

julia> try_parse(Rat, "1/0").status, try_parse(Rat, "one").status
(Adelefeld.DOMAIN, Adelefeld.PARSE)

julia> try_fball(Rat(1), Rat(-2)).status
DOMAIN::AdfStatus = 7

julia> unwrap(try_rat(7, 21))
1/3

julia> collect(instances(AdfStatus))
11-element Vector{AdfStatus}:
 OK::AdfStatus = 0
 NOT_DETERMINED::AdfStatus = 1
 UNIT_NOT_CERTIFIED::AdfStatus = 2
 NEEDS_SPLIT::AdfStatus = 3
 NOT_UNIQUE::AdfStatus = 4
 NO_SOLUTION::AdfStatus = 5
 NOT_UNIT::AdfStatus = 6
 DOMAIN::AdfStatus = 7
 UNSUPPORTED::AdfStatus = 8
 PARSE::AdfStatus = 9
 LIMIT::AdfStatus = 10

#### block 6-text-and-dumps
julia> u = Adele(Rat(1, 3));

julia> value_text(u; digits = 5)
"(0.33333 +/- 3.4e-6 ; 1/3)"

julia> show(IOContext(stdout, :adf_digits => 8), u)
(0.33333333 +/- 3.4e-9 ; 1/3)

julia> d = dump_text(u)
"adf1 Q adele 1 5555555555555555 -40 1 -41 g 1 0 3"

julia> w = load_dump(Adele, d); identical(u, w)
true

julia> s = value_text(u); identical(u, parse(Adele, s))
false

julia> dump_text(parse(Adele, s))
"adf1 Q adele 1 5555555555555555 -40 56bdf97 -5b g 1 0 3"

julia> readvalue("3 mod 12"), readvalue("7/3")
((* ; 3 mod 12), 7/3)

julia> try_readvalue("(1 ; 1/2 mod 3)").details
(kind = Adelefeld.TEXT_ADELE,)

julia> r = try_readvalue("[5 mod 6]"); r.status, r.details
(Adelefeld.UNSUPPORTED, (kind = Adelefeld.TEXT_UCOSET,))

julia> dump_text(parse(FBall, "3 mod 12"))
"adf1 Q fball g 3 c 1"

#### block 7-modctx
julia> ModCtx(blocks = [8, 9, 5])
ModCtx(modulus=360, blocks=[8, 9, 5])

julia> ModCtx(prime_powers = [2 => 3, 3 => 2])
ModCtx(modulus=72, blocks=[8, 9])

julia> ModCtx(modulus = 360)
ModCtx(modulus=360, blocks=[360])

julia> ctx = ModCtx(factorial = 6)
ModCtx(modulus=720, blocks=[16, 9, 5])

julia> ModCtx(primorial_power = (7, 2))
ModCtx(modulus=44100, blocks=[4, 9, 25, 49])

julia> modulus(ctx), nblocks(ctx), Int.(blocks(ctx))
(720, 3, [16, 9, 5])

julia> y = to_local(parse(FBall, "1 mod 5"), ctx)
(* ; 1 mod 5)

julia> is_local(y), context(y) === ctx
(true, true)

julia> g = to_global(y); is_local(g), context(g)
(false, nothing)

julia> try_to_local(parse(FBall, "1 mod 7"), ctx).status
DOMAIN::AdfStatus = 7

julia> enclose_local(readvalue("1 mod 5"), ModCtx(blocks = [8, 9]))
(* ; 0 mod 1)

julia> r = try_enclose_local(parse(FBall, "1 mod 32"), ModCtx(blocks = [8, 9])); r.value, r.details
((* ; 1 mod 8), (lost = true,))

julia> try_modctx(blocks = [4, 6]).status
DOMAIN::AdfStatus = 7

#### block 8-housekeeping
julia> t = Rat(5); close(t); close(t)

julia> t + 1
ERROR: InvalidStateException: the Rat handle is closed

julia> Adelefeld.diagnostics().live >= 0, Adelefeld.diagnostics().finalizer_errors
(true, 0)

julia> keys(Adelefeld.diagnostics())
(:state, :library, :live, :finalizer_errors, :contexts, :context_frees)

julia> c5 = ModCtx(blocks = [5]); y5 = to_local(parse(FBall, "2 mod 5"), c5); finalize(c5); c5
ModCtx(finalized)

julia> y5 + 1
(* ; 3 mod 5)

julia> to_local(parse(FBall, "2 mod 5"), c5)
ERROR: InvalidStateException: the ModCtx was finalized
```

## 3. Pkg.test

    (time timeout 900 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 JULIA_PKG_OFFLINE=true \
        JULIA_DEPOT_PATH="$PWD/lanes/docs-julia/depot:$HOME/.julia:" TMPDIR="$PWD/lanes/docs-julia/tmp" \
        julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld \
        -e 'using Pkg; Pkg.test()') > lanes/docs-julia/pkgtest.log 2>&1

Exit status 0. From `Testing Running tests` to the end:

```text
     Testing Running tests...
Test Summary:        | Pass  Total   Time
Adelefeld step J-a.1 |  904    904  12.3s
Test Summary:        | Pass  Total  Time
Adelefeld step J-a.2 | 1015   1015  5.9s
BASELINE=0
Adele LIVE=0 ERRORS=0
CAdele LIVE=0 ERRORS=0
RealBall LIVE=0 ERRORS=0
ComplexBall LIVE=0 ERRORS=0
ERRORS=24 LIMIT_CALLS=0 LIVE=0
Test Summary:        | Pass  Total   Time
Adelefeld step J-a.3 | 1649   1649  13.1s
DUMPS=16 FREES=16
CONTEXTS=0
DIGIT_ERRORS=64 PRINT_CALLS=0
PREC_ERRORS=5 LIMIT_CALLS=0 CLASSIFY_CALLS=0
LIVE=0
Test Summary:   | Pass  Total  Time
C call counters |   43     43  1.8s
BASELINE=0
ROUNDTRIPS=10000 LIVE=0 ERRORS=0
Test Summary:        | Pass  Total  Time
Adelefeld step J-a.4 | 1313   1313  7.1s
BASELINE=0 CTX=0
ROUNDTRIPS=10000 LIVE=0 CONTEXTS=0 FREES=10000 ERRORS=0
KEPT=true
AFTER LIVE=0 CONTEXTS=0 FREES=10001 ERRORS=0
Test Summary:        | Pass  Total  Time
Adelefeld step J-b.1 |  488    488  3.7s
     Testing Adelefeld tests passed 

real	1m5,107s
user	1m14,797s
sys	0m3,349s
exit 0
```

The five step testsets pass 904 + 1015 + 1649 + 1313 + 488 = 5369 tests (the count of HANDOFF.md). The testset
`C call counters` (43) is a code string run by a child `julia` process against the fixture library
`count-ja4.so` (`julia/Adelefeld/test/runtests.jl:1732-1742`), and prints its own summary. In all, 5412 passing
tests are printed, 0 failed, 0 errored. Pkg.test writes `julia/Adelefeld/Manifest.toml` (gitignored,
`.gitignore:36`); `git status` shows no tracked file changed.

## 4. Julia without the launcher

    timeout 120 env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true JULIA_DEPOT_PATH=... TMPDIR=... \
        julia --startup-file=no --project=julia/Adelefeld \
        -e 'try; @eval using Adelefeld; catch e; println(typeof(e)); showerror(stdout, e.error); println(); end' \
        > lanes/docs-julia/noload.out 2>&1

```text
InitError
Adelefeld.AdfLoadError("dlopen (GMP symbol)", "dlopen to succeed", "could not load library \"/home/tobias/Projects/adelefeld-wt/docs-julia/build/libadelefeld.so\"\n/lib/x86_64-linux-gnu/libflint.so.18: undefined symbol: __gmpn_modexact_1_odd", "/home/tobias/Projects/adelefeld-wt/docs-julia/build/libadelefeld.so", "This Julia loaded its bundled libgmp before the library; start Julia with the system GMP preloaded: LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 julia ..., or use the launcher /home/tobias/Projects/adelefeld-wt/docs-julia/julia/Adelefeld/bin/adelefeld-julia (docs/api-j.md:80-87; tests/julia/README.md).")
```

## 5. The exported names and their docstrings

    timeout 300 env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true JULIA_DEPOT_PATH="$PWD/lanes/docs-julia/depot:$HOME/.julia:" \
    TMPDIR="$PWD/lanes/docs-julia/tmp" \
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld lanes/docs-julia/exports.jl \
    > lanes/docs-julia/exports.out 2>&1

```text
exported: 63
Adele	(none)
AdfAbiError	A C function returned an int that is not a status of include/adelefeld/status.h. The raw value is kept;
AdfError	Thrown by every throwing form on a status other than OK (decision J5, docs/api-j.md:24-25). It carries the
AdfLoadError	A load gate failed. Fields: the gate, the expected and the observed value, the library path and a hint.
AdfResult	The result of a `try_` method (docs/api-j.md:165-168). `status` always exists; `value` is a fresh owner
AdfStatus	(none)
CAdele	(none)
ComplexBall	A numerical complex ball in a private initialized CAdele with finite coordinate exact 0.
FBall	The finite ball `adf_fball`, the set `a + N Zhat` of finite adeles (include/adelefeld/fball.h:1-11; SPEC 4.1):
ModCtx	A modulus context (include/adelefeld/modctx.h; conventions 5.14): an immutable C context made by one of the
PointComparison	(none)
Rat	(none)
RealBall	A numerical real ball. Owns an initialized private Adele whose finite coordinate is exact 0.
TextKind	(none)
TextLimits	The limits of a parser (conventions 8.4; include/adelefeld/text.h:81-103): layout `adf_text_limits_t`,
add	(none)
blocks	The blocks q_1, ..., q_k in their order (`adf_modctx_block` for 0 <= i < k, modctx.h:111-112).
center	The stored centre a = A/d of the canonical triple: in [0, N) for N > 0, the point itself for N = 0
compare_points	`adf_fball_compare` (fball.h:258-265): the comparison of two unknown points, one in each ball. EQUAL if both
complexpart	(none)
context	The context a local value borrows, found by comparing `adf_fball_context(x)` (modctx.h:171-174) with the
divexact	(none)
dump_text	(none)
enclose_local	(none)
equal_set	`adf_fball_equal_set` (fball.h:239-242): the two sets are equal. An int 0/1 read as a Bool, never a status.
finitepart	(none)
identical	`adf_rat_identical` (rat.h:58-60): the same numerator and denominator; for this exact type the same as `==`.
inside	True when the set `x` is inside the set `y`: FIRST INSIDE SECOND (SPEC 4.2; docs/api-j.md:222, 314).
is_canonical	`adf_fball_is_canonical` (fball.h:93-100): predicate G or L holds. A Bool, never a status.
is_local	`adf_fball_is_local` (modctx.h:171-173): the backend is ADF_LOCAL. Not part of the value text, which is the
load_dump	(none)
modulus	The modulus K of the context, through `adf_modctx_get_modulus` (modctx.h:105-106), as an exact integer
mul	(none)
nblocks	The number k >= 0 of word blocks (`adf_modctx_nblocks`, modctx.h:108-109).
neg	(none)
overlaps	`adf_fball_overlaps` (fball.h:244-245): the two sets meet. Not transitive (SPEC 4.2).
radius	The radius N = H/d >= 0 in lowest terms, 0 for an exact value (`adf_fball_get_radius`, fball.h:168-170).
readvalue	(none)
realpart	(none)
reconstruct	(none)
sub	(none)
to_global	(none)
to_local	(none)
try_add	(none)
try_div	`adf_rat_div` (rat.h:109-113): OK, or NOT_UNIT when `q = 0`; the fresh output is cleared on failure.
try_dump_text	Copy exactly the byte length of `adf_x_dump_str`, with `adf_str_free` in a finally (dump.h:49-52).
try_enclose_local	The best enclosure of `x` among the local values of `ctx` (`adf_fball_set_local_enclose`,
try_fball	The ball `c + N Zhat`, global and canonical (centre reduced into [0, N); N = 0 is the point c), through
try_inv	`adf_rat_inv` (rat.h:115-116): OK, or NOT_UNIT when `q = 0`.
try_load_dump	Strict typed C dump loader. Context is NULL; no bindings or working precision. The whole input is
try_modctx	A new modulus context from EXACTLY ONE construction family (docs/api-j.md:255); none or two or more is an
try_mul	(none)
try_neg	(none)
try_parse	Calls `adf_rat_set_str` on the bytes of `s` (`ncodeunits`, not characters). NUL and non-ASCII bytes go to C,
try_rat	`n` alone, when it fits Int64, goes through `adf_rat_set_si` (statusless, OK is synthesized). Otherwise the
try_readvalue	Classify with `adf_text_classify` (text.h:137-143). LIMIT on excess bytes precedes PARSE for forbidden
try_reconstruct	`adf_fball_reconstruct` in the exact closed interval [lo, hi] (recon.h:49-52).
try_sub	(none)
try_to_global	`x` in the global backend (`adf_fball_set_global`, modctx.h:166-169): recombination by CRT and canonical
try_to_local	The set of `x` as a local value of `ctx`, exactly (`adf_fball_set_local`, modctx.h:145-153): the ball
try_value_text	`adf_rat_get_str`: "n" or "n/d" in lowest terms (conventions 9.4). Exactly `len` bytes are copied; the
unwrap	Returns `r.value` on OK; throws `AdfError` on every other status (docs/api-j.md:172).
value_text	(none)
```

## 6. The checks of the two pages

    python3 lanes/docs-julia/fill_readme.py
    python3 lanes/docs-julia/check.py lanes/docs-julia/tutorial.rerun.out > lanes/docs-julia/check.out 2>&1

```text
1. REPL blocks: README 8, tutorial 8; 0 differ
2. Pkg.test block: 13 lines, 0 not in pkgtest.log
3. Exports: 63 names in the export lines (63 distinct), 63 rows in the API table; missing []; extra []
   pending: docs/README.md -> GUIDE.md (written by lane docs-guide)
4. Links: 90 relative links checked in 2 pages, 0 broken, 1 pending
5. Widths: julia/Adelefeld/README.md: 0 prose lines over 116, 0 table or code lines over 116 []
5. Widths: docs/README.md: 0 prose lines over 116, 0 table or code lines over 116 []
check exit 0
```
