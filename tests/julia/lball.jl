# tests/julia/lball.jl: Julia ccalls of adf_lball (lane f-slice1; include/adelefeld/lball.h, docs/api-1f.md L0 to L8).
#
# What a user does: makes 1/3 at p = 5 to precision 10, multiplies and inverts it, reads the valuation and the unit
# part of 50/3 + O(5^7), and meets the statuses. Every number the library returns is read back through FLINT's
# fmpz_get_str and checked in Julia with BigInt arithmetic (the definition of the centre: u in (0, p^k),
# 3 u = 1 modulo p^k), not against a table of results.
#
# Usage: julia --startup-file=no tests/julia/lball.jl build/libadelefeld.so
# (with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a symbol; tests/test_julia.sh
# does that.) Only Libdl and Test of the standard library. Values are raw memory sized by the library's own
# adf_sizeof_lball / adf_sizeof_rat. The layout of adf_lball_struct is read from conventions 5.8 and lball.h:
# p at 0, u = fmpq at 8 (num at 8, den at 16), v at 24, N at 32, exact (int) at 40; size 48.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia lball.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

# statuses (include/adelefeld/status.h)
const OK, NOT_DETERMINED, UNIT_NOT_CERTIFIED, NOT_UNIT, DOMAIN, LIMIT = 0, 1, 2, 6, 7, 10

const OFF_NUM, OFF_DEN, OFF_V, OFF_N, OFF_EXACT = 8, 16, 24, 32, 40

struct Place
    opaque::UInt64
end

function place(p::Integer)
    v = Ref(Place(0))
    st = ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, p)
    st == OK || error("not a prime: $p")
    return v[]
end

const ARCH = ccall(ad(:adf_place_inf), Place, ())

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# an lball in raw memory
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

prime(x::LB) = BigInt(unsafe_load(Ptr{UInt64}(x.ptr)))
num(x::LB) = parse(BigInt, fmpz_str(x.ptr + OFF_NUM))
den(x::LB) = parse(BigInt, fmpz_str(x.ptr + OFF_DEN))
val(x::LB) = Int(unsafe_load(Ptr{Int64}(x.ptr + OFF_V)))
prec(x::LB) = Int(unsafe_load(Ptr{Int64}(x.ptr + OFF_N)))
isexact(x::LB) = unsafe_load(Ptr{Cint}(x.ptr + OFF_EXACT)) != 0

# an adf_rat holding n/d
function rat(n::Integer, d::Integer)
    r = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_rat), Csize_t, ())))
    ccall(ad(:adf_rat_init), Cvoid, (Ptr{UInt8},), r)
    fn, fd = Ref{Clong}(0), Ref{Clong}(0)
    for (f, x) in ((fn, n), (fd, d))
        ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), f)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), f, string(x), 10) == 0 || error("bad integer")
    end
    st = ccall(ad(:adf_rat_set_fmpz2), Cint, (Ptr{UInt8}, Ptr{Clong}, Ptr{Clong}), r, fn, fd)
    st == OK || error("rat")
    return r
end
freerat(r) = (ccall(ad(:adf_rat_clear), Cvoid, (Ptr{UInt8},), r); Libc.free(r))

function set_rat_ball(pl::Place, n, d, N)
    x, r = LB(), rat(n, d)
    st = ccall(ad(:adf_lball_set_rat_ball), Cint, (Ptr{UInt8}, Place, Ptr{UInt8}, Clong), x.ptr, pl, r, N)
    freerat(r)
    return st, x
end
function set_rat(pl::Place, n, d)
    x, r = LB(), rat(n, d)
    st = ccall(ad(:adf_lball_set_rat), Cint, (Ptr{UInt8}, Place, Ptr{UInt8}), x.ptr, pl, r)
    freerat(r)
    return st, x
end
function binop(name::Symbol, x::LB, y::LB)
    z = LB()
    st = ccall(ad(name), Cint, (Ptr{UInt8}, Ptr{UInt8}, Ptr{UInt8}), z.ptr, x.ptr, y.ptr)
    return st, z
end
function unop(name::Symbol, x::LB)
    z = LB()
    st = ccall(ad(name), Cint, (Ptr{UInt8}, Ptr{UInt8}), z.ptr, x.ptr)
    return st, z
end
pred(name::Symbol, x::LB, y::LB) = ccall(ad(name), Cint, (Ptr{UInt8}, Ptr{UInt8}), x.ptr, y.ptr) != 0
function valuation(x::LB)
    v, inf = Ref{Clong}(0), Ref{Cint}(0)
    st = ccall(ad(:adf_lball_valuation), Cint, (Ptr{Clong}, Ptr{Cint}, Ptr{UInt8}), v, inf, x.ptr)
    return st, v[], inf[]
end
function decompose(x::LB)
    m, u = Ref{Clong}(0), LB()
    st = ccall(ad(:adf_lball_decompose), Cint, (Ptr{Clong}, Ptr{UInt8}, Ptr{UInt8}), m, u.ptr, x.ptr)
    return st, m[], u
end

# the unit part of a ball as an integer, checked against the definition
centre_ok(x::LB) = !isexact(x) && den(x) == 1 && 0 < num(x) < BigInt(prime(x))^(prec(x) - val(x))

@testset "adf_lball through ccall" begin
    @test ccall(ad(:adf_sizeof_lball), Csize_t, ()) == 48
    @test ccall(ad(:adf_alignof_lball), Csize_t, ()) == 8

    p5 = place(5)
    # 1/3 at p = 5 to precision 10: u in (0, 5^10) with 3 u = 1 modulo 5^10, valuation 0
    st, x = set_rat_ball(p5, 1, 3, 10)
    @test st == OK
    @test prime(x) == 5 && val(x) == 0 && prec(x) == 10 && !isexact(x)
    @test centre_ok(x) && mod(3 * num(x), BigInt(5)^10) == 1
    @test ccall(ad(:adf_lball_is_canonical), Cint, (Ptr{UInt8},), x.ptr) == 1

    # the product (1/3)^2 = 1/9: 9 u = 1 modulo 5^10, the precision stays 10
    st, y = binop(:adf_lball_mul, x, x)
    @test st == OK && val(y) == 0 && prec(y) == 10 && centre_ok(y) && mod(9 * num(y), BigInt(5)^10) == 1

    # the inverse of 1/3 + O(5^10) is 3 + O(5^10)
    st, z = unop(:adf_lball_inv, x)
    @test st == OK && val(z) == 0 && prec(z) == 10 && num(z) == 3
    # x * (1/x) contains 1: the ball 1 + O(5^10)
    st, w = binop(:adf_lball_mul, x, z)
    @test st == OK && num(w) == 1 && prec(w) == 10 && val(w) == 0
    st, one = set_rat(p5, 1, 1)
    @test st == OK && isexact(one) && pred(:adf_lball_contains, one, w) && !pred(:adf_lball_contains, w, one)

    # sum with a coarser ball: precision min(10, 3) = 3
    st, c = set_rat_ball(p5, 2, 7, 3)
    @test st == OK
    st, s = binop(:adf_lball_add, x, c)
    @test st == OK && prec(s) == 3 && centre_ok(s)
    # (1/3 + 2/7) mod 5^3: 13/21; 21 u = 13 modulo 125
    @test mod(21 * num(s), BigInt(5)^3) == 13

    # valuation and decomposition: 50/3 + O(5^7) has valuation 2 and unit part 10/3... (50/3 = 5^2 * 2/3)
    st, f = set_rat_ball(p5, 50, 3, 7)
    @test st == OK && val(f) == 2 && prec(f) == 7
    st, v, inf = valuation(f)
    @test st == OK && v == 2 && inf == 0
    st, m, u = decompose(f)
    @test st == OK && m == 2 && val(u) == 0 && prec(u) == 5 && mod(3 * num(u), BigInt(5)^5) == 2
    # the absolute value is the exact rational 5^-2: read through adf_lball_abs
    a = rat(0, 1)
    st = ccall(ad(:adf_lball_abs), Cint, (Ptr{UInt8}, Ptr{UInt8}), a, f.ptr)
    # an adf_rat is one fmpq: numerator at 0, denominator at 8
    @test st == OK && parse(BigInt, fmpz_str(a)) == 1 && parse(BigInt, fmpz_str(a + 8)) == 25
    freerat(a)

    # statuses: a ball around 0 has no inverse and no valuation; the exact 0 is not a unit
    st, zb = set_rat_ball(p5, 0, 1, 4)
    @test st == OK && !isexact(zb) && num(zb) == 0
    @test unop(:adf_lball_inv, zb)[1] == UNIT_NOT_CERTIFIED
    @test valuation(zb)[1] == NOT_DETERMINED
    st, ze = set_rat(p5, 0, 1)
    @test st == OK && isexact(ze)
    @test unop(:adf_lball_inv, ze)[1] == NOT_UNIT
    @test valuation(ze)[1] == OK && valuation(ze)[3] == 1
    @test decompose(ze)[1] == DOMAIN
    @test decompose(zb)[1] == NOT_DETERMINED

    # two primes and the archimedean place
    st, t3 = set_rat_ball(place(3), 1, 2, 5)
    @test st == OK
    @test binop(:adf_lball_add, x, t3)[1] == DOMAIN
    @test set_rat_ball(ARCH, 1, 2, 5)[1] == DOMAIN

    # a large prime, 2^64 - 59, and a limit
    big = place(18446744073709551557)
    st, b = set_rat_ball(big, 1, 3, 4)
    @test st == OK && prime(b) == 18446744073709551557 && mod(3 * num(b), BigInt(18446744073709551557)^4) == 1
    @test set_rat_ball(p5, -1, 1, 1 << 40)[1] == LIMIT
end
