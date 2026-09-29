# tests/julia/lfunc.jl: Julia ccalls of exp, log and Log on local balls (lane f-slice4; include/adelefeld/lfunc.h,
# docs/api-1f4.md).
#
# What a user does: computes exp(5) and log(6) at p = 5 to 20 digits, log(-1) at p = 2, and sees the DOMAIN of exp(1)
# at 5; then applies exp to a ball and sees the radius of the ball kept (not the requested 20 digits). Every value
# the library returns is checked in Julia with its own exact arithmetic: the series summed with Rational{BigInt} well
# past the needed number of terms (the degree-k term of exp(5) has 5-adic valuation k - v_5(k!) >= k - (k-1)/4, of
# log(1 + 5) valuation k - v_5(k); 60 terms leave a tail of valuation above 40), reduced modulo 5^20.
#
# Usage: julia --startup-file=no tests/julia/lfunc.jl build/libadelefeld.so
# (tests/test_julia.sh adds the LD_PRELOAD of the system libgmp.so.10 when Julia's bundled libgmp lacks a symbol.)
# Only Libdl and Test. The layout of adf_lball_struct (conventions 5.8, lball.h): p at 0, u = fmpq at 8 (num at 8,
# den at 16), v at 24, N at 32, exact (int) at 40; size 48.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia lfunc.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN, LIMIT = 0, 1, 7, 10
const OFF_NUM, OFF_DEN, OFF_V, OFF_N, OFF_EXACT = 8, 16, 24, 32, 40

struct Place
    opaque::UInt64
end

function place(p::Integer)
    v = Ref(Place(0))
    ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, p) == OK || error("not a prime: $p")
    return v[]
end

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

mutable struct LB
    ptr::Ptr{UInt8}
    function LB()
        p = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_lball), Csize_t, ())))
        ccall(ad(:adf_lball_init), Cvoid, (Ptr{UInt8},), p)
        x = new(p)
        finalizer(x -> (ccall(ad(:adf_lball_clear), Cvoid, (Ptr{UInt8},), x.ptr); Libc.free(x.ptr)), x)
        return x
    end
end

num(x::LB) = parse(BigInt, fmpz_str(x.ptr + OFF_NUM))
den(x::LB) = parse(BigInt, fmpz_str(x.ptr + OFF_DEN))
val(x::LB) = Int(unsafe_load(Ptr{Int64}(x.ptr + OFF_V)))
prec(x::LB) = Int(unsafe_load(Ptr{Int64}(x.ptr + OFF_N)))
isexact(x::LB) = unsafe_load(Ptr{Cint}(x.ptr + OFF_EXACT)) != 0

function rat(n::Integer, d::Integer)
    r = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_rat), Csize_t, ())))
    ccall(ad(:adf_rat_init), Cvoid, (Ptr{UInt8},), r)
    fn, fd = Ref{Clong}(0), Ref{Clong}(0)
    for (f, x) in ((fn, n), (fd, d))
        ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), f)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), f, string(x), 10) == 0 || error("bad integer")
    end
    ccall(ad(:adf_rat_set_fmpz2), Cint, (Ptr{UInt8}, Ptr{Clong}, Ptr{Clong}), r, fn, fd) == OK || error("rat")
    return r
end
freerat(r) = (ccall(ad(:adf_rat_clear), Cvoid, (Ptr{UInt8},), r); Libc.free(r))

function exact(p::Integer, n::Integer, d::Integer = 1)
    x, r = LB(), rat(n, d)
    ccall(ad(:adf_lball_set_rat), Cint, (Ptr{UInt8}, Place, Ptr{UInt8}), x.ptr, place(p), r) == OK || error("set_rat")
    freerat(r)
    return x
end
function ball(p::Integer, n::Integer, d::Integer, N::Integer)
    x, r = LB(), rat(n, d)
    st = ccall(ad(:adf_lball_set_rat_ball), Cint, (Ptr{UInt8}, Place, Ptr{UInt8}, Clong), x.ptr, place(p), r, N)
    st == OK || error("set_rat_ball")
    freerat(r)
    return x
end
function lfun(name::Symbol, x::LB, N::Integer)
    y = LB()
    st = ccall(ad(name), Cint, (Ptr{UInt8}, Ptr{UInt8}, Clong), y.ptr, x.ptr, N)
    return st, y
end
contains(x::LB, y::LB) = ccall(ad(:adf_lball_contains), Cint, (Ptr{UInt8}, Ptr{UInt8}), x.ptr, y.ptr) != 0

# the residue modulo m of a rational with denominator prime to m
modq(q::Rational{BigInt}, m::BigInt) = mod(numerator(q) * invmod(denominator(q), m), m)
# the value p^v u of a ball's centre modulo p^N (u an integer)
centre(x::LB, p::Integer) = mod(BigInt(p)^val(x) * num(x), BigInt(p)^prec(x))

@testset "exp, log, Log through ccall" begin
    P20 = BigInt(5)^20
    # exp(5) at p = 5 to 20 digits: a ball of exponent 20, a unit, equal to the series modulo 5^20
    st, e = lfun(:adf_lball_exp, exact(5, 5), 20)
    @test st == OK && !isexact(e) && prec(e) == 20 && val(e) == 0 && den(e) == 1 && 0 < num(e) < P20
    series_exp = sum(Rational{BigInt}(BigInt(5)^k, factorial(BigInt(k))) for k in 0:60)
    @test num(e) == modq(series_exp, P20)

    # log(6) at p = 5 to 20 digits: valuation 1 (log(1 + 5) = 5 - 25/2 + ...), unit part modulo 5^19
    st, l = lfun(:adf_lball_log, exact(5, 6), 20)
    @test st == OK && !isexact(l) && prec(l) == 20 && val(l) == 1 && 0 < num(l) < BigInt(5)^19
    series_log = sum(Rational{BigInt}((-1)^(k + 1) * BigInt(5)^k, k) for k in 1:60)
    @test centre(l, 5) == modq(series_log, P20)
    # Log(6) = log(6), and Log(30) = Log(6): Log(5) = 0
    st, L6 = lfun(:adf_lball_Log, exact(5, 6), 20)
    st2, L30 = lfun(:adf_lball_Log, exact(5, 30), 20)
    @test st == OK && st2 == OK && centre(L6, 5) == centre(l, 5) && centre(L30, 5) == centre(l, 5)
    st, L5 = lfun(:adf_lball_Log, exact(5, 5), 20)
    @test st == OK && isexact(L5) && num(L5) == 0

    # log(-1) at p = 2 is exactly 0 (FLINT's padic_log refuses -1; the series converges on 1 + 2 Z_2)
    st, m1 = lfun(:adf_lball_log, exact(2, -1), 30)
    @test st == OK && isexact(m1) && num(m1) == 0

    # exp(1) at p = 5 is outside the domain 5 Z_5: DOMAIN, and the output is not a value
    st, _ = lfun(:adf_lball_exp, exact(5, 1), 20)
    @test st == DOMAIN
    # exp of the ball 5 Z_5 + ... around 0 of exponent 0 meets the domain and its complement
    @test lfun(:adf_lball_exp, ball(5, 0, 1, 0), 20)[1] == NOT_DETERMINED
    # Log of a ball that contains 0: an uncertain zero
    @test lfun(:adf_lball_Log, ball(5, 0, 1, 7), 20)[1] == NOT_DETERMINED

    # exp of the ball 5 + 5^3 Z_5 asked to 20 digits: the result keeps the radius 5^3, and contains exp(5)
    st, eb = lfun(:adf_lball_exp, ball(5, 5, 1, 3), 20)
    @test st == OK && prec(eb) == 3 && contains(e, eb)
    # log(exp(5)) contains 5
    st, back = lfun(:adf_lball_log, e, 20)
    @test st == OK && contains(exact(5, 5), back)
end
