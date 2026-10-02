# tests/julia/f_at.jl: Julia ccalls of exp_at, log_at, Log_at at a prime and at the real place of a partial ball (lane
# f-slice6; include/adelefeld/rfunc.h "A PRIME", include/adelefeld/lfunc.h, docs/api-1f.md "Slice 1F.4-b").
#
# What a user does: makes the adele of 5/3 (200 bits), projects it to {5, real}, and takes exp at 5 (to 12 5-adic
# digits: at a prime the last argument is the ABSOLUTE precision N) and at the real place (200 bits); then log at 5 and
# Log at 5, and meets the statuses and the reported place (3 is not a place of the projection; exp of a unit at 5).
# Every value is checked in Julia with its own exact arithmetic: the series summed with Rational{BigInt} past the
# needed number of terms, reduced modulo 5^12; the real value with BigFloat at 300 bits.
#
# Usage: julia --startup-file=no tests/julia/f_at.jl build/libadelefeld.so
# (tests/test_julia.sh adds the LD_PRELOAD of the system libgmp.so.10 when Julia's bundled libgmp lacks a symbol.)
# Only Libdl and Test. Layouts: adf_lball_struct (conventions 5.8): p at 0, u = fmpq at 8 (num at 8, den at 16), v at
# 24, N at 32, exact (int) at 40, size 48; adf_sball_struct (conventions 5.9): arch (int) at 0, len (slong) at 104.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia f_at.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN, UNSUPPORTED, LIMIT = 0, 1, 7, 8, 10
const ARCH_NONE, ARCH_REAL = 0, 1
const OFF_NUM, OFF_DEN, OFF_V, OFF_N, OFF_EXACT = 8, 16, 24, 32, 40

struct Place
    opaque::UInt64
end
function place(p::Integer)
    v = Ref(Place(0))
    ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, p) == OK || error("not a prime: $p")
    return v[]
end
const ARCH = ccall(ad(:adf_place_inf), Place, ())
isarch(v::Place) = ccall(ad(:adf_place_is_archimedean), Cint, (Place,), v) != 0
primeof(v::Place) = ccall(ad(:adf_place_prime_get), Culong, (Place,), v)

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# an adf_lball in raw memory
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
prime(x::LB) = Int(unsafe_load(Ptr{UInt64}(x.ptr)))
num(x::LB) = parse(BigInt, fmpz_str(x.ptr + OFF_NUM))
den(x::LB) = parse(BigInt, fmpz_str(x.ptr + OFF_DEN))
val(x::LB) = Int(unsafe_load(Ptr{Int64}(x.ptr + OFF_V)))
prec(x::LB) = Int(unsafe_load(Ptr{Int64}(x.ptr + OFF_N)))
isexact(x::LB) = unsafe_load(Ptr{Cint}(x.ptr + OFF_EXACT)) != 0

# an adf_sball
mutable struct SB
    ptr::Ptr{UInt8}
    function SB()
        p = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_sball), Csize_t, ())))
        ccall(ad(:adf_sball_init), Cvoid, (Ptr{UInt8},), p)
        x = new(p)
        finalizer(x -> (ccall(ad(:adf_sball_clear), Cvoid, (Ptr{UInt8},), x.ptr); Libc.free(x.ptr)), x)
        return x
    end
end
arch(s::SB) = ccall(ad(:adf_sball_arch), Cint, (Ptr{UInt8},), s.ptr)
nplaces(s::SB) = Int(ccall(ad(:adf_sball_num_places), Clong, (Ptr{UInt8},), s.ptr))
function getlball(s::SB, v::Place)
    c = LB()
    st = ccall(ad(:adf_sball_get_lball), Cint, (Ptr{UInt8}, Ptr{UInt8}, Place), c.ptr, s.ptr, v)
    return st, c
end

# an arb as text through FLINT
function arbtext(s::SB, digits = 70)
    r = Ptr{UInt8}(Libc.calloc(1, 48))
    ccall(fl(:arb_init), Cvoid, (Ptr{UInt8},), r)
    st = ccall(ad(:adf_sball_get_arb), Cint, (Ptr{UInt8}, Ptr{UInt8}, Place), r, s.ptr, ARCH)
    txt = ""
    if st == OK
        c = ccall(fl(:arb_get_str), Ptr{UInt8}, (Ptr{UInt8}, Clong, Culong), r, digits, 0)
        txt = unsafe_string(c)
        ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), c)
    end
    ccall(fl(:arb_clear), Cvoid, (Ptr{UInt8},), r)
    Libc.free(r)
    return st, txt
end
setprecision(BigFloat, 300)
function arbval(txt)
    m = match(r"^\[?([-0-9.e+]+)(?: \+/- ([-0-9.e+]+))?\]?$", txt)
    m === nothing && error("cannot read an arb text: $txt")
    return parse(BigFloat, m.captures[1]), m.captures[2] === nothing ? BigFloat(0) : parse(BigFloat, m.captures[2])
end
# the text describes a ball that contains `want` (up to the 70 printed digits) and is narrower than 2^-180
function ball_around(txt, want)
    mid, rad = arbval(txt)
    return abs(mid - want) <= rad + abs(want) * BigFloat(10)^-66 && rad < abs(want) * BigFloat(2)^-180
end

# the adele of n/d at `prec` bits
function adele_of(n::Integer, d::Integer, prec = 200)
    r = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_rat), Csize_t, ())))
    ccall(ad(:adf_rat_init), Cvoid, (Ptr{UInt8},), r)
    fn, fd = Ref{Clong}(0), Ref{Clong}(0)
    for (f, x) in ((fn, n), (fd, d))
        ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), f)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), f, string(x), 10) == 0 || error("bad integer")
    end
    ccall(ad(:adf_rat_set_fmpz2), Cint, (Ptr{UInt8}, Ptr{Clong}, Ptr{Clong}), r, fn, fd) == OK || error("rat")
    a = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_adele), Csize_t, ())))
    ccall(ad(:adf_adele_init), Cvoid, (Ptr{UInt8},), a)
    ccall(ad(:adf_adele_set_rat), Cvoid, (Ptr{UInt8}, Ptr{UInt8}, Clong), a, r, prec)
    ccall(ad(:adf_rat_clear), Cvoid, (Ptr{UInt8},), r)
    Libc.free(r)
    for f in (fn, fd)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), f)
    end
    return a
end

function project(a, places::Vector{Place})
    s, where = SB(), Ref(Place(0))
    st = ccall(ad(:adf_sball_project), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Ptr{Place}, Clong),
               s.ptr, where, a, places, length(places))
    return st, s, where[]
end
function at(name::Symbol, x::SB, v::Place, prec)
    z, where = SB(), Ref(Place(0))
    st = ccall(ad(name), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Place, Clong), z.ptr, where, x.ptr, v, prec)
    return st, z, where[]
end

# ---- Julia's own values ----
vp(n::Integer, p) = (e = 0; while n % p == 0; n = div(n, p); e += 1; end; e)
vpq(x::Rational{BigInt}, p) = vp(numerator(x), p) - vp(denominator(x), p)
# x mod p^K in [0, p^K) for a rational that is a p-adic integer
function modp(x::Rational{BigInt}, p, K)
    m = big(p)^K
    return mod(numerator(x) * invmod(denominator(x), m), m)
end
# exp(x) at p, x in p Z_p (p odd), modulo p^K
function expmod(x::Rational{BigInt}, p, K)
    s, t = big(0)//1, big(1)//1
    for k in 0:120
        s += t
        t = t * x / (k + 1)
    end
    return modp(s, p, K)
end
# log(u) for a principal unit u = 1 modulo p, given as a rational, modulo p^K
function logmod(u::Rational{BigInt}, p, K)
    y = u - 1
    s, pw = big(0)//1, big(1)//1
    for k in 1:200
        pw *= y
        s += (isodd(k) ? 1 : -1) * pw / k
    end
    return modp(s, p, K)
end
# Log(x) at an odd prime p: x = p^m w u, the Teichmueller factor w = a^(p^n) (a the unit part), u = a/w
function Logmod(x::Rational{BigInt}, p, K)
    m = vpq(x, p)
    a = x / big(p)^m
    n = K + 6
    mod_n = big(p)^n
    ai = modp(a, p, n)
    w = powermod(ai, big(p)^n, mod_n)
    u = (ai * invmod(w, mod_n)) % mod_n
    return logmod(u // 1, p, K)
end
@testset "exp_at, log_at, Log_at at a prime and at the real place through ccall" begin
    p3, p5 = place(3), place(5)
    a = adele_of(5, 3)
    st, s, _ = project(a, [p5, ARCH])
    @test st == OK && nplaces(s) == 2 && arch(s) == ARCH_REAL

    # exp at 5, twelve 5-adic digits: the last argument is the absolute precision N, not a number of bits
    x = big(5) // 3
    st, e, where = at(:adf_sball_exp_at, s, p5, 12)
    @test st == OK && nplaces(e) == 1 && arch(e) == ARCH_NONE
    stc, ce = getlball(e, p5)
    @test stc == OK && !isexact(ce) && prime(ce) == 5 && prec(ce) == 12 && val(ce) == 0 && den(ce) == 1
    @test num(ce) == expmod(x, 5, 12)
    # the same call at the real place, 200 bits: a real ball around exp(5/3) = 5.2944900...
    st, er, where = at(:adf_sball_exp_at, s, ARCH, 200)
    @test st == OK && nplaces(er) == 1 && arch(er) == ARCH_REAL
    stt, txt = arbtext(er)
    @test stt == OK && ball_around(txt, exp(BigFloat(5) / 3))
    @test getlball(er, p5)[1] == 7   # the result has no component at 5: DOMAIN

    # log at 5 of 5/3: 5/3 - 1 = 2/3 is a 5-adic unit, outside 1 + 5 Z_5: DOMAIN, where = 5; y untouched
    st, _, where = at(:adf_sball_log_at, s, p5, 12)
    @test st == DOMAIN && !isarch(where) && primeof(where) == 5
    # Log at 5 of 5/3: defined for every non-zero x; the value is log of the principal unit of (1/3) (Log(5) = 0)
    st, l, where = at(:adf_sball_Log_at, s, p5, 12)
    @test st == OK
    stc, c = getlball(l, p5)
    @test stc == OK && prime(c) == 5 && prec(c) == 12
    @test den(c) == 1 && val(c) >= 0 && big(5)^val(c) * num(c) == Logmod(x, 5, 12)
    # log at the real place: log(5/3) = 0.5108256237659907...
    st, lr, where = at(:adf_sball_log_at, s, ARCH, 200)
    @test st == OK && ball_around(arbtext(lr)[2], log(BigFloat(5) / 3))
    st, lr2, where = at(:adf_sball_Log_at, s, ARCH, 200)   # at the real place Log is log (positive input)
    @test st == OK && ball_around(arbtext(lr2)[2], log(BigFloat(5) / 3))

    # the statuses and the place reported: 3 is not a place of the projection
    st, _, where = at(:adf_sball_exp_at, s, p3, 12)
    @test st == DOMAIN && primeof(where) == 3
    # sin(5/3) at 5, absolute N=12. Lemma 5 bounds the tail after degree 81 above 60.
    st, sine, _ = at(:adf_sball_sin_at, s, p5, 12)
    @test st == OK && nplaces(sine) == 1 && arch(sine) == ARCH_NONE
    stc, cs = getlball(sine, p5)
    partial = sum((-1)^div(k-1, 2) * (big(5)//3)^k / factorial(big(k)) for k in 1:2:81)
    @test stc == OK && !isexact(cs) && prec(cs) == 12 && den(cs) == 1
    @test big(5)^val(cs) * num(cs) == modp(partial, 5, 12)

    # exp at 5 of 6 (a unit at 5): DOMAIN; of 25/3: v_5 = 2, OK, and the value differs from that of 5/3 at digit 1
    a6 = adele_of(6, 1)
    st, s6, _ = project(a6, [p5, ARCH])
    st, _, where = at(:adf_sball_exp_at, s6, p5, 12)
    @test st == DOMAIN && primeof(where) == 5
    a25 = adele_of(25, 3)
    st, s25, _ = project(a25, [p5])
    st, e25, _ = at(:adf_sball_exp_at, s25, p5, 12)
    stc, c25 = getlball(e25, p5)
    @test st == OK && num(c25) == expmod(big(25) // 3, 5, 12)
    @test num(c25) != num(ce)
    # a projection without the real place: the real place is not a place of it
    st, _, where = at(:adf_sball_exp_at, s25, ARCH, 200)
    @test st == DOMAIN && isarch(where)
    # the precision at a prime is not limited by the 2^21 bits of the real place: exp(0) is the exact 1 at N = 2^22
    a0 = adele_of(0, 1)
    st, s0, _ = project(a0, [p5, ARCH])
    st, e0, where = at(:adf_sball_exp_at, s0, p5, 4194304)
    stc, c0 = getlball(e0, p5)
    @test st == OK && isexact(c0) && num(c0) == 1 && den(c0) == 1 && val(c0) == 0
    st, _, where = at(:adf_sball_exp_at, s0, ARCH, 4194304)
    @test st == LIMIT && isarch(where)
end
