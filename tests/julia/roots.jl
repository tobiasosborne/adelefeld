# tests/julia/roots.jl: a Julia ccall of adf_root_padic_from_seed (slice 1 of S.2, lane s2-slice1), of
# adf_roots_padic (slice 2) and of adf_roots_real with adf_rootlist_get_arb (slice 3, lane s2-slice3).
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

# ---- slice 2 of S.2 (lane s2-slice2): all roots in Z_p through adf_roots_padic and the accessors ----

const UNSUPPORTED = 8

# all roots of c at p through the strict (strict = true) or the partial function; returns (status, certs,
# classes, complete, verify_complete) with certs a vector of (a, K, s) and classes a vector of (a, e)
function roots_padic(c, p, prec, depth; strict = true)
    L = Libc.malloc(ccall(ad(:adf_sizeof_rootlist), Csize_t, ()))
    ccall(ad(:adf_rootlist_init), Cvoid, (Ptr{Cvoid},), L)
    f = poly_new(c)
    fun = strict ? :adf_roots_padic : :adf_roots_padic_partial
    st = ccall(ad(fun), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Place, Clong, Clong), L, f, place(p), prec, depth)
    certs, classes, complete, vc = Tuple{BigInt, Int, Int}[], Tuple{BigInt, Int}[], -1, -1
    if st == OK
        z = fmpz_new(0)
        K = Ref{Clong}(0)
        s = Ref{Clong}(0)
        n = ccall(ad(:adf_rootlist_length), Clong, (Ptr{Cvoid},), L)
        for i in 0:n-1
            ccall(ad(:adf_rootlist_get_cert), Cint, (Ptr{Clong}, Ref{Clong}, Ref{Clong}, Ptr{Cvoid}, Clong),
                  z, K, s, L, i) == 1 || error("get_cert")
            push!(certs, (fmpz_get(z), K[], s[]))
        end
        nu = ccall(ad(:adf_rootlist_unresolved_length), Clong, (Ptr{Cvoid},), L)
        for i in 0:nu-1
            ccall(ad(:adf_rootlist_get_unresolved), Cint, (Ptr{Clong}, Ref{Clong}, Ptr{Cvoid}, Clong),
                  z, K, L, i) == 1 || error("get_unresolved")
            push!(classes, (fmpz_get(z), K[]))
        end
        complete = ccall(ad(:adf_rootlist_is_complete), Cint, (Ptr{Cvoid},), L)
        vc = ccall(ad(:adf_rootlist_verify_complete), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), L, f, depth)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), z)
    end
    poly_free(f)
    ccall(ad(:adf_rootlist_clear), Cvoid, (Ptr{Cvoid},), L)
    Libc.free(L)
    return st, certs, classes, complete, vc
end

peval(c, x) = foldr((ci, acc) -> acc * x + ci, c; init = big(0))
vp(x, p) = (x == 0 ? typemax(Int) : (v = 0; while mod(x, p) == 0; x = div(x, p); v += 1; end; v))

@testset "adf_roots_padic through ccall: (X^2 - 2)(X - 3) in Z_7" begin
    c = [6, -2, -3, 1]                           # (X^2 - 2)(X - 3); 3 = sqrt(2) modulo 7
    dc = [-2, -6, 3]                             # the derivative
    st, certs, classes, complete, vc = roots_padic(c, 7, 10, 8)
    @test st == OK && length(certs) == 3 && isempty(classes) && complete == 1 && vc == 1
    for (a, K, s) in certs
        @test 0 <= a < big(7)^K && K == max(10, s + 1)
        @test mod(peval(c, a), big(7)^(K + s)) == 0          # (R3), in Julia
        @test vp(peval(dc, a), 7) == s                        # (R2)
    end
    @test sort([mod(a, 7) for (a, _, _) in certs]) == [3, 3, 4]
    r3 = [a for (a, _, s) in certs if a == 3]
    @test length(r3) == 1                                    # the root 3 itself, s = v(g'(3)) = v(7) = 1
    sq = [a for (a, _, _) in certs if a != 3]
    @test all(mod(a^2 - 2, big(7)^10) == 0 for a in sq)      # the two square roots of 2
    @test sort([s for (_, _, s) in certs]) == [0, 1, 1]
    # depth 0: 3 is a double root modulo 7; the strict function is NOT_DETERMINED, the partial one gives
    # the root near 4 and the class 3 + 7 Z_7
    @test roots_padic(c, 7, 10, 0)[1] == NOT_DETERMINED
    st, certs, classes, complete, vc = roots_padic(c, 7, 10, 0; strict = false)
    @test st == OK && length(certs) == 1 && mod(certs[1][1], 7) == 4 && classes == [(big(3), 1)]
    @test complete == 0 && vc == 0
    @test roots_padic([0], 7, 10, 8)[1] == DOMAIN
    @test roots_padic(c, 7, 10, -1)[1] == DOMAIN
    # above the old bound 2^20 (lane s2-slice4): 1048583 = 7 mod 8, so 2 is a square and there are three roots
    p = big(1048583)
    st, certs, classes, complete, vc = roots_padic(c, 1048583, 3, 2)
    @test st == OK && length(certs) == 3 && isempty(classes) && complete == 1 && vc == 1
    @test all(mod(peval(c, a), p^(K + s)) == 0 for (a, K, s) in certs)
    @test any(a == 3 for (a, _, _) in certs)
end

# ---- slice 3 of S.2 (lane s2-slice3): the real roots through adf_roots_real and adf_rootlist_get_arb ----

const SIZEOF_ARB = 48                            # FLINT 3.0.1: an arf of 32 bytes and a mag of 16 bytes

# an arb (48 bytes of raw memory, initialised by FLINT)
function arb_new()
    x = Libc.malloc(SIZEOF_ARB)
    ccall(fl(:arb_init), Cvoid, (Ptr{Cvoid},), x)
    return x
end

function arb_free(x)
    ccall(fl(:arb_clear), Cvoid, (Ptr{Cvoid},), x)
    Libc.free(x)
end

# the exact interval [a, b] 2^e of an arb, as two Rational{BigInt}
function arb_ends(x)
    a, b, e = fmpz_new(0), fmpz_new(0), fmpz_new(0)
    ccall(fl(:arb_get_interval_fmpz_2exp), Cvoid, (Ptr{Clong}, Ptr{Clong}, Ptr{Clong}, Ptr{Cvoid}), a, b, e, x)
    k = fmpz_get(e)
    s = k >= 0 ? big(2)^k // 1 : 1 // big(2)^(-k)
    out = (fmpz_get(a) * s, fmpz_get(b) * s)
    for z in (a, b, e)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), z)
    end
    return out
end

# the real roots of c: (status, [(midpoint, radius, lo, hi, exact)], count, verify_entries, verify_complete); the
# midpoint and the radius are read through arb_get_mid_arb and arb_get_rad_arb, each an exact ball
function roots_real(c, prec)
    L = Libc.malloc(ccall(ad(:adf_sizeof_rootlist), Csize_t, ()))
    ccall(ad(:adf_rootlist_init), Cvoid, (Ptr{Cvoid},), L)
    f = poly_new(c)
    st = ccall(ad(:adf_roots_real), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), L, f, prec)
    balls, ve, vc = [], -1, -1
    if st == OK
        x, m, r = arb_new(), arb_new(), arb_new()
        n = ccall(ad(:adf_rootlist_length), Clong, (Ptr{Cvoid},), L)
        for i in 0:n-1
            got = ccall(ad(:adf_rootlist_get_arb), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), x, L, i)
            got == 1 || error("get_arb")
            ccall(fl(:arb_get_mid_arb), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), m, x)
            ccall(fl(:arb_get_rad_arb), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), r, x)
            mid, mid2 = arb_ends(m)
            rad, rad2 = arb_ends(r)
            (mid == mid2 && rad == rad2) || error("midpoint or radius not exact")
            lo, hi = arb_ends(x)
            ex = ccall(fl(:arb_is_exact), Cint, (Ptr{Cvoid},), x)
            push!(balls, (mid, rad, lo, hi, ex))
        end
        ccall(ad(:adf_rootlist_get_arb), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), x, L, n) == 0 || error("get_arb n")
        ve = ccall(ad(:adf_rootlist_verify_entries), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), L, f)
        vc = ccall(ad(:adf_rootlist_verify_complete), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), L, f, 0)
        arb_free(x); arb_free(m); arb_free(r)
    end
    poly_free(f)
    ccall(ad(:adf_rootlist_clear), Cvoid, (Ptr{Cvoid},), L)
    Libc.free(L)
    return st, balls, ve, vc
end

@testset "adf_roots_real through ccall: X^3 - 2 X to 100 bits" begin
    c = [0, -2, 0, 1]                            # X^3 - 2 X: the roots -sqrt(2), 0, sqrt(2)
    st, balls, ve, vc = roots_real(c, 100)
    @test st == OK && length(balls) == 3 && ve == 1 && vc == 1
    (m1, r1, lo1, hi1, _), (m2, r2, lo2, hi2, ex2), (m3, r3, lo3, hi3, _) = balls
    @test hi1 < lo2 && hi2 < lo3                 # increasing and disjoint
    @test lo1 == m1 - r1 && hi1 == m1 + r1       # the end points are midpoint -+ radius
    @test ex2 == 1 && m2 == 0 && r2 == 0         # the root 0, exact
    for (m, r, lo, hi) in ((m1, r1, lo1, hi1), (m3, r3, lo3, hi3))
        @test sign(peval(c, lo)) * sign(peval(c, hi)) < 0      # the exact test of P3.8, in Julia
        @test min(lo^2, hi^2) < 2 < max(lo^2, hi^2) && lo * hi > 0   # it holds sqrt(2) or -sqrt(2)
        @test r > 0 && r < abs(m) / big(2)^100                 # 100 bits of relative accuracy
    end
    @test m1 < 0 < m3
    @test roots_real([0], 100)[1] == DOMAIN
    st, balls, ve, vc = roots_real([1, 0, 1], 100)             # X^2 + 1: no real root, OK, empty
    @test st == OK && isempty(balls) && ve == 1 && vc == 1
end
