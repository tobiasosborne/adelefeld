# tests/julia/roots.jl: a Julia ccall of adf_root_padic_from_seed (slice 1 of S.2, lane s2-slice1).
#
# Usage: julia --startup-file=no tests/julia/roots.jl build/libadelefeld.so
# (with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a symbol;
# see the header of tests/test_julia.sh).
#
# Only Libdl and Test of the standard library are used. A list is raw memory sized by the library's own
# adf_sizeof_rootlist; an fmpz_poly_struct is 24 bytes and an fmpz 8 bytes (FLINT 3.0.1, fmpz_types.h).
# An adf_place_t is a struct of one ulong passed by value (include/adelefeld/place.h), made here by
# adf_place_prime. The certificate is read through adf_rootlist_get_cert and checked in Julia with BigInt.
# Statuses: OK 0, NOT_DETERMINED 1, DOMAIN 7 (include/adelefeld/status.h).

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia roots.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN = 0, 1, 7
const SIZEOF_FMPZ_POLY = 24

struct Place
    opaque::UInt64
end

function place(p::Integer)
    v = Ref(Place(0))
    st = ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, p)
    st == OK || error("not a prime: $p")
    return v[]
end

# an fmpz (8 bytes) holding x, and its value as a BigInt
function fmpz_new(x::Integer)
    r = Ref{Clong}(0)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), r)
    ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), r, string(x), 10) == 0 || error("bad integer")
    return r
end

function fmpz_get(r)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Clong}), C_NULL, 10, r)
    v = parse(BigInt, unsafe_string(s))
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return v
end

# an fmpz_poly with the coefficients c[1] + c[2] X + ...
function poly_new(c::AbstractVector)
    p = Libc.malloc(SIZEOF_FMPZ_POLY)
    ccall(fl(:fmpz_poly_init), Cvoid, (Ptr{Cvoid},), p)
    for (i, x) in enumerate(c)
        z = fmpz_new(x)
        ccall(fl(:fmpz_poly_set_coeff_fmpz), Cvoid, (Ptr{Cvoid}, Clong, Ptr{Clong}), p, i - 1, z)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), z)
    end
    return p
end

function poly_free(p)
    ccall(fl(:fmpz_poly_clear), Cvoid, (Ptr{Cvoid},), p)
    Libc.free(p)
end

# the seed function; returns (status, a', K, s, verified) with a' a BigInt, or nothing for a status other than OK
function seed(c, p, a, prec)
    L = Libc.malloc(ccall(ad(:adf_sizeof_rootlist), Csize_t, ()))
    ccall(ad(:adf_rootlist_init), Cvoid, (Ptr{Cvoid},), L)
    f = poly_new(c)
    za = fmpz_new(a)
    st = ccall(ad(:adf_root_padic_from_seed), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Place, Ptr{Clong}, Clong),
               L, f, place(p), za, prec)
    out = nothing
    if st == OK
        zap = fmpz_new(0)
        K = Ref{Clong}(0)
        s = Ref{Clong}(0)
        got = ccall(ad(:adf_rootlist_get_cert), Cint, (Ptr{Clong}, Ref{Clong}, Ref{Clong}, Ptr{Cvoid}, Clong),
                    zap, K, s, L, 0)
        got == 1 || error("get_cert returned $got")
        ver = ccall(ad(:adf_rootlist_verify_entries), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), L, f)
        n = ccall(ad(:adf_rootlist_length), Clong, (Ptr{Cvoid},), L)
        out = (fmpz_get(zap), K[], s[], ver, n)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), zap)
    end
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), za)
    poly_free(f)
    ccall(ad(:adf_rootlist_clear), Cvoid, (Ptr{Cvoid},), L)
    Libc.free(L)
    return st, out
end

@testset "adf_root_padic_from_seed through ccall" begin
    # the square root of 2 in Z_7 to 20 digits from the seed 3 (3^2 = 9 = 2 modulo 7; f'(3) = 6, s = 0)
    st, out = seed([-2, 0, 1], 7, 3, 20)
    @test st == OK
    a, K, s, ver, n = out
    @test K == 20 && s == 0 && ver == 1 && n == 1
    @test 0 <= a < big(7)^20
    @test mod(a, 7) == 3
    @test mod(a^2 - 2, big(7)^20) == 0          # squared in Julia
    # the other square root, from the seed 4, is -a modulo 7^20
    st2, out2 = seed([-2, 0, 1], 7, 4, 20)
    @test st2 == OK && mod(out2[1] + a, big(7)^20) == 0
    # 27 X at 3, seed 0: the certificate refers to g = X: (0, K, 0) (solvers P3.12(3))
    st, out = seed([0, 27], 3, 0, 5)
    @test st == OK && out[1] == 0 && out[2] == 5 && out[3] == 0
    # X^2 + 1 at 2, seed 1: refused (the root 1 modulo 2 does not lift)
    st, out = seed([1, 0, 1], 2, 1, 5)
    @test st == NOT_DETERMINED && out === nothing
    # f = 0 and prec 0 are DOMAIN
    @test seed([0], 7, 3, 20)[1] == DOMAIN
    @test seed([-2, 0, 1], 7, 3, 0)[1] == DOMAIN
end
