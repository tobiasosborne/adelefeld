# tests/julia/smoke.jl: a Julia ccall smoke test of the public interface of adelefeld, run by
# tests/test_julia.sh (work package 1.9 of docs/PLAN.md, row 1.9: "a Julia ccall smoke test where
# Julia is installed"; docs/SPEC.md 10 item 4, M0-D12; docs/conventions.md section 12).
#
# Rules followed here (lane m1-julia's brief):
#   - the library is reached only through Libdl.dlopen and ccall on the resulting handle;
#   - every value is either a Julia bit pattern (Cint, Clong, Culong, Csize_t, Ptr) or a block of
#     raw memory from Libc.malloc, sized by a live call to adf_sizeof_<type>();
#   - include/adelefeld/*.h is never read by this file or by anything it loads; the only source of
#     truth for a struct's size is the library itself (adf_sizeof_*), checked against
#     tests/julia/layouts.jl, which is typed in from docs/api-m1.md by hand;
#   - no Julia package beyond the standard library (Libdl, Test) is used, and none is installed
#     from the network.
#
# Usage: julia --startup-file=no tests/julia/smoke.jl <path-to-libadelefeld.so>
#
# FLINT's slong and ulong are `long` and `unsigned long` on the supported platforms (64-bit only,
# conventions 12.10, CV-42; flint.h:110-111, :193); on x86-64 Linux those are Int64 and UInt64, the
# same bit pattern and calling convention as Clong/Culong. Named here once so that every ccall
# signature below reads the same as the C declaration it mirrors.
const Slong = Clong
const Ulong = Culong

using Test
using Libdl

if length(ARGS) != 1
    println(stderr, "usage: julia smoke.jl <path-to-libadelefeld.so>")
    exit(2)
end

const LIBPATH = ARGS[1]

include(joinpath(@__DIR__, "layouts.jl"))

const LIB = Libdl.dlopen(LIBPATH)

# A thin wrapper: look the symbol up once, fail loudly (not with a segfault) if it is missing.
function sym(name::Symbol)
    p = Libdl.dlsym(LIB, name; throw_error = false)
    p === nothing && error("adelefeld: symbol $name not found in $LIBPATH")
    return p
end

# ---- layout function pointers, looked up once ----

const F_VERSION_CHECK          = sym(:adf_version_check)
const F_FLINT_VERSION_COMPILED = sym(:adf_flint_version_compiled)
const F_STATUS_STR             = sym(:adf_status_str)

const F_RAT_INIT      = sym(:adf_rat_init)
const F_RAT_CLEAR     = sym(:adf_rat_clear)
const F_RAT_SET_SI    = sym(:adf_rat_set_si)
const F_RAT_ADD       = sym(:adf_rat_add)
const F_RAT_MUL       = sym(:adf_rat_mul)
const F_RAT_EQUAL     = sym(:adf_rat_equal)
const F_RAT_GET_STR   = sym(:adf_rat_get_str)
const F_RAT_SET_STR   = sym(:adf_rat_set_str)

const F_STR_FREE = sym(:adf_str_free)

const F_FBALL_INIT     = sym(:adf_fball_init)
const F_FBALL_CLEAR    = sym(:adf_fball_clear)
const F_FBALL_SET_STR  = sym(:adf_fball_set_str)
const F_FBALL_ADD      = sym(:adf_fball_add)
const F_FBALL_MUL      = sym(:adf_fball_mul)
const F_FBALL_GET_STR  = sym(:adf_fball_get_str)

const F_ADELE_INIT    = sym(:adf_adele_init)
const F_ADELE_CLEAR   = sym(:adf_adele_clear)
const F_ADELE_SET_STR = sym(:adf_adele_set_str)
const F_ADELE_MUL     = sym(:adf_adele_mul)
const F_ADELE_GET_STR = sym(:adf_adele_get_str)

const F_MODCTX_NEW_BLOCKS = sym(:adf_modctx_new_blocks)
const F_MODCTX_FREE       = sym(:adf_modctx_free)
const F_MODCTX_NBLOCKS    = sym(:adf_modctx_nblocks)
const F_MODCTX_BLOCK      = sym(:adf_modctx_block)

# adf_sizeof_*/adf_alignof_* for every layout of tests/julia/layouts.jl.
sizeof_of(fn::Symbol)  = ccall(sym(fn), Csize_t, ())
alignof_of(fn::Symbol) = ccall(sym(fn), Csize_t, ())

@testset "adelefeld Julia smoke test ($LIBPATH)" begin

    @testset "version" begin
        st = ccall(F_VERSION_CHECK, Cint, ())
        @test st == 0   # ADF_OK (status.h:17); the .so is built against the FLINT that is installed
        cver = ccall(F_FLINT_VERSION_COMPILED, Ptr{UInt8}, ())
        @test cver != C_NULL
        ver = unsafe_string(cver)
        @test startswith(ver, "3.0")   # conventions 12.11: FLINT 3.0.x
    end

    @testset "layouts (adf_sizeof_*/adf_alignof_* against tests/julia/layouts.jl)" begin
        for (name, szsym, alsym, want_size, want_align) in ADF_LAYOUTS
            got_size = sizeof_of(szsym)
            got_align = alignof_of(alsym)
            @test got_size == want_size
            @test got_align == want_align
        end
    end

    @testset "adf_rat: init, set from two integers, add, mul, compare, print, clear" begin
        sz = sizeof_of(:adf_sizeof_rat)
        x = Libc.malloc(sz); y = Libc.malloc(sz); z = Libc.malloc(sz); w = Libc.malloc(sz)
        @test x != C_NULL && y != C_NULL && z != C_NULL && w != C_NULL
        ccall(F_RAT_INIT, Cvoid, (Ptr{Cvoid},), x)
        ccall(F_RAT_INIT, Cvoid, (Ptr{Cvoid},), y)
        ccall(F_RAT_INIT, Cvoid, (Ptr{Cvoid},), z)
        ccall(F_RAT_INIT, Cvoid, (Ptr{Cvoid},), w)

        ccall(F_RAT_SET_SI, Cvoid, (Ptr{Cvoid}, Slong), x, 3)   # x = 3
        ccall(F_RAT_SET_SI, Cvoid, (Ptr{Cvoid}, Slong), y, 4)   # y = 4

        ccall(F_RAT_ADD, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), z, x, y)  # z = x + y = 7
        ccall(F_RAT_MUL, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), w, x, y)  # w = x * y = 12

        eq_xy = ccall(F_RAT_EQUAL, Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, y)
        eq_xx = ccall(F_RAT_EQUAL, Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, x)
        @test eq_xy == 0
        @test eq_xx == 1

        len = Ref{Csize_t}(0)
        sptr = ccall(F_RAT_GET_STR, Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len, z)
        @test sptr != C_NULL
        s = unsafe_string(sptr, len[])
        @test s == "7"
        ccall(F_STR_FREE, Cvoid, (Ptr{UInt8},), sptr)

        len2 = Ref{Csize_t}(0)
        sptr2 = ccall(F_RAT_GET_STR, Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len2, w)
        s2 = unsafe_string(sptr2, len2[])
        @test s2 == "12"
        ccall(F_STR_FREE, Cvoid, (Ptr{UInt8},), sptr2)

        ccall(F_RAT_CLEAR, Cvoid, (Ptr{Cvoid},), x)
        ccall(F_RAT_CLEAR, Cvoid, (Ptr{Cvoid},), y)
        ccall(F_RAT_CLEAR, Cvoid, (Ptr{Cvoid},), z)
        ccall(F_RAT_CLEAR, Cvoid, (Ptr{Cvoid},), w)
        Libc.free(x); Libc.free(y); Libc.free(z); Libc.free(w)
    end

    @testset "adf_fball: parse, add, mul, print, raw offsets (docs/SPEC.md 4.3)" begin
        sz = sizeof_of(:adf_sizeof_fball)
        x = Libc.malloc(sz); y = Libc.malloc(sz); z = Libc.malloc(sz)
        @test x != C_NULL && y != C_NULL && z != C_NULL
        ccall(F_FBALL_INIT, Cvoid, (Ptr{Cvoid},), x)
        ccall(F_FBALL_INIT, Cvoid, (Ptr{Cvoid},), y)
        ccall(F_FBALL_INIT, Cvoid, (Ptr{Cvoid},), z)

        sx = "3 mod 12"
        bx = Vector{UInt8}(sx)
        stx = ccall(F_FBALL_SET_STR, Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
                    x, bx, Csize_t(length(bx)), C_NULL)
        @test stx == 0   # ADF_OK

        sy = "5 mod 18"
        by = Vector{UInt8}(sy)
        sty = ccall(F_FBALL_SET_STR, Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
                    y, by, Csize_t(length(by)), C_NULL)
        @test sty == 0

        # Raw offsets (conventions 12.4, CV-40; docs/api-m1.md "Layouts"): A at 0, H at 8, both
        # fmpz. "3 mod 12" canonicalises to (A, H, d) = (3, 12, 1); both 3 and 12 are small enough
        # that FLINT stores an fmpz inline as the slong value itself (conventions 12.11: "small
        # values inline"), so the raw word at the documented offset is the value itself. This is
        # the check that a sizeof/alignof comparison alone cannot make: two structs of the same
        # size and alignment can still disagree about where a field is.
        raw_A = unsafe_load(Ptr{Int64}(x + ADF_FBALL_OFFSET_A))
        raw_H = unsafe_load(Ptr{Int64}(x + ADF_FBALL_OFFSET_H))
        raw_d = unsafe_load(Ptr{Int64}(x + ADF_FBALL_OFFSET_D))
        @test raw_A == 3
        @test raw_H == 12
        @test raw_d == 1

        len = Ref{Csize_t}(0)
        sptr = ccall(F_FBALL_GET_STR, Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len, x)
        @test unsafe_string(sptr, len[]) == "(* ; 3 mod 12)"
        ccall(F_STR_FREE, Cvoid, (Ptr{UInt8},), sptr)

        ccall(F_FBALL_ADD, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), z, x, y)
        len_sum = Ref{Csize_t}(0)
        sum_ptr = ccall(F_FBALL_GET_STR, Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len_sum, z)
        sum_text = unsafe_string(sum_ptr, len_sum[])
        # docs/SPEC.md 4.3: (3 mod 12) + (5 mod 18) = 2 mod 6; printed "(* ; 2 mod 6)" (9.4).
        @test sum_text == "(* ; 2 mod 6)"
        ccall(F_STR_FREE, Cvoid, (Ptr{UInt8},), sum_ptr)

        ccall(F_FBALL_MUL, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), z, x, y)
        len_prod = Ref{Csize_t}(0)
        prod_ptr = ccall(F_FBALL_GET_STR, Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len_prod, z)
        prod_text = unsafe_string(prod_ptr, len_prod[])
        # docs/SPEC.md 4.3: (3 mod 12) * (5 mod 18) = 3 mod 6; printed "(* ; 3 mod 6)".
        @test prod_text == "(* ; 3 mod 6)"
        ccall(F_STR_FREE, Cvoid, (Ptr{UInt8},), prod_ptr)

        ccall(F_FBALL_CLEAR, Cvoid, (Ptr{Cvoid},), x)
        ccall(F_FBALL_CLEAR, Cvoid, (Ptr{Cvoid},), y)
        ccall(F_FBALL_CLEAR, Cvoid, (Ptr{Cvoid},), z)
        Libc.free(x); Libc.free(y); Libc.free(z)
    end

    @testset "adf_adele: parse at prec 128, multiply by itself, print with 20 digits" begin
        sz = sizeof_of(:adf_sizeof_adele)
        x = Libc.malloc(sz); y = Libc.malloc(sz)
        @test x != C_NULL && y != C_NULL
        ccall(F_ADELE_INIT, Cvoid, (Ptr{Cvoid},), x)
        ccall(F_ADELE_INIT, Cvoid, (Ptr{Cvoid},), y)

        # "(0.5 ; 1/2)": real part the dyadic point 0.5, exact at any prec >= 1; finite part the
        # exact rational 1/2 (no "mod": radius 0). Both coordinates are exact, so x * x is exact
        # too, and no precision loss enters the check below.
        text = "(0.5 ; 1/2)"
        b = Vector{UInt8}(text)
        st = ccall(F_ADELE_SET_STR, Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Slong, Ptr{Cvoid}),
                   x, b, Csize_t(length(b)), Slong(128), C_NULL)
        @test st == 0   # ADF_OK

        ccall(F_ADELE_MUL, Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Slong), y, x, x, Slong(128))

        len = Ref{Csize_t}(0)
        sptr = ccall(F_ADELE_GET_STR, Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}, Slong), len, y, Slong(20))
        @test sptr != C_NULL
        s = unsafe_string(sptr, len[])
        # 0.5^2 = 0.25 (real, exact); (1/2)^2 = 1/4 (finite, exact): "(0.25 ; 1/4)" (conventions 9.4).
        @test s == "(0.25 ; 1/4)"

        semi = findfirst(==(';'), s)
        close_paren = findlast(==(')'), s)
        @test semi !== nothing && close_paren !== nothing
        finite_part = strip(s[semi+1:close_paren-1])
        @test finite_part == "1/4"

        ccall(F_STR_FREE, Cvoid, (Ptr{UInt8},), sptr)
        ccall(F_ADELE_CLEAR, Cvoid, (Ptr{Cvoid},), x)
        ccall(F_ADELE_CLEAR, Cvoid, (Ptr{Cvoid},), y)
        Libc.free(x); Libc.free(y)
    end

    @testset "a status other than OK: \"1/0\" as a rational" begin
        sz = sizeof_of(:adf_sizeof_rat)
        q = Libc.malloc(sz)
        @test q != C_NULL
        ccall(F_RAT_INIT, Cvoid, (Ptr{Cvoid},), q)

        text = "1/0"
        b = Vector{UInt8}(text)
        st = ccall(F_RAT_SET_STR, Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
                   q, b, Csize_t(length(b)), C_NULL)
        @test st == 7   # ADF_DOMAIN (status.h:25; text.h: zero denominator, conventions 9.3)

        namep = ccall(F_STATUS_STR, Ptr{UInt8}, (Cint,), st)
        @test namep != C_NULL
        @test unsafe_string(namep) == "DOMAIN"

        ccall(F_RAT_CLEAR, Cvoid, (Ptr{Cvoid},), q)
        Libc.free(q)
    end

    @testset "a context: adf_modctx_new_blocks, nblocks, block, free" begin
        blocks = Ulong[3, 4, 5]   # pairwise coprime word blocks (conventions 5.14): K = 60
        out = Ref{Ptr{Cvoid}}(C_NULL)
        st = ccall(F_MODCTX_NEW_BLOCKS, Cint, (Ptr{Ptr{Cvoid}}, Ptr{Ulong}, Slong),
                   out, blocks, Slong(length(blocks)))
        @test st == 0   # ADF_OK
        ctx = out[]
        @test ctx != C_NULL

        n = ccall(F_MODCTX_NBLOCKS, Slong, (Ptr{Cvoid},), ctx)
        @test n == 3
        b0 = ccall(F_MODCTX_BLOCK, Ulong, (Ptr{Cvoid}, Slong), ctx, Slong(0))
        b1 = ccall(F_MODCTX_BLOCK, Ulong, (Ptr{Cvoid}, Slong), ctx, Slong(1))
        b2 = ccall(F_MODCTX_BLOCK, Ulong, (Ptr{Cvoid}, Slong), ctx, Slong(2))
        @test (b0, b1, b2) == (3, 4, 5)

        ccall(F_MODCTX_FREE, Cvoid, (Ptr{Cvoid},), ctx)
    end
end

# Slice 1 of milestone S (lane s3-slice1): adf_resid_reconstruct through ccall.
include(joinpath(@__DIR__, "resid.jl"))

# Slice 1 of S.1 (lane s1-slice1): adf_linsolve_mod through ccall.
include(joinpath(@__DIR__, "linsolve.jl"))
