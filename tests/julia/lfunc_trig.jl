# 1F.7: a user computes sin(5), cos(5) at 5 to 20 absolute digits, and cosh(4) at 2.
# Own exact Rational{BigInt} sums through degree 80 are reduced at the stated N=20.
# Lemma 5, docs/proofs/functions.md:117: at 5, k-v_5(k!) >= (3k+1)/4;
# at 2, 2k-v_2(k!) >= k+1. Every omitted term has valuation >20.
# No exp identity is the value oracle. Only Libdl and Test are needed.
using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia lfunc_trig.jl <path-to-libadelefeld.so>"); exit(2))

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
    for f in (fn, fd)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), f)
    end
    return r
end
freerat(r) = (ccall(ad(:adf_rat_clear), Cvoid, (Ptr{UInt8},), r); Libc.free(r))

function exact(p::Integer, n::Integer, d::Integer = 1)
    x, r = LB(), rat(n, d)
    ccall(ad(:adf_lball_set_rat), Cint, (Ptr{UInt8}, Place, Ptr{UInt8}),
          x.ptr, place(p), r) == OK || error("set_rat")
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

function parity_series(name, x)
    odd = name in (:sin, :sinh)
    alternating = name in (:sin, :cos)
    sum(Rational{BigInt}((alternating ? (-1)^div(k, 2) : 1) * big(x)^k,
                        factorial(big(k))) for k in (odd ? 1 : 0):2:80)
end

@testset "local sin, cos, sinh, cosh through ccall" begin
    for (name, p, a) in ((:sin, 5, 5), (:cos, 5, 5), (:sinh, 5, 5), (:cosh, 2, 4))
        fn = Symbol("adf_lball_", name)
        st, y = lfun(fn, exact(p, a), 20)
        @test st == OK && !isexact(y) && prec(y) == 20 && den(y) == 1
        @test centre(y, p) == modq(parity_series(name, a), big(p)^20)
        println(name, "(", a, ") at ", p, ": ", centre(y, p), " + O(", p, "^20)")
        zero = exact(p, 0)
        st, z = lfun(fn, zero, 20)
        @test st == OK && isexact(z) && num(z) == (name in (:cos, :cosh) ? 1 : 0)
        st, b = lfun(fn, ball(p, 0, 1, 3), 20)
        E = name in (:cos, :cosh) ? 6-(p == 2) : 3
        @test st == OK && !isexact(b) && prec(b) == E
        @test lfun(fn, ball(2, 0, 1, 1), 20)[1] == NOT_DETERMINED
        @test lfun(fn, exact(2, 2), 20)[1] == DOMAIN
    end
    st, _ = lfun(:adf_lball_sin, exact(5, 1), 20)
    @test st == DOMAIN
    println("sin(1) at 5: DOMAIN")
end
