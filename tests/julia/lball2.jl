# tests/julia/lball2.jl: Julia ccalls of the rest of work package 1F.3 (lane f-slice3; include/adelefeld/lball.h, section
# "slice 1F.3-b"; docs/api-1f.md L9 to L13).
#
# What a user does: decomposes 50/3 at p = 5 and -12 at p = 2 into p^m w u (w the Teichmueller factor, or the sign at 2, u
# a principal unit), reads m, the residue, w and u; takes the fractional part of 7/25 at 5; raises 1/3 to the power -7 at 5;
# meets the statuses. Every number is read back through fmpz_get_str and checked in Julia with BigInt arithmetic against
# the definitions (3 u + 2 = 0, w^(p-1) = 1 modulo p^n, w u = the unit part modulo p^k), not against a table of results.
#
# Usage: julia --startup-file=no tests/julia/lball2.jl build/libadelefeld.so
# (with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a symbol; tests/test_julia.sh does that.)
# Layout of adf_lball_struct (conventions 5.8, lball.h): p at 0, u = fmpq at 8 (num at 8, den at 16), v at 24, N at 32,
# exact (int) at 40; size 48. An adf_rat is one fmpq: numerator at 0, denominator at 8.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia lball2.jl <path-to-libadelefeld.so>"); exit(2))

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

# the value of a canonical lball as a rational (exact, or the centre of a ball): u p^v
value(x::LB) = (num(x) // den(x)) * (BigInt(prime(x)) // 1)^val(x)

# adf_lball_decompose_teich(m, w, index, u, x, prec)
function split(x::LB, n)
    m, idx, w, u = Ref{Clong}(0), Ref{Culong}(0), LB(), LB()
    st = ccall(ad(:adf_lball_decompose_teich), Cint,
               (Ptr{Clong}, Ptr{UInt8}, Ptr{Culong}, Ptr{UInt8}, Ptr{UInt8}, Clong), m, w.ptr, idx, u.ptr, x.ptr, n)
    return st, m[], idx[], w, u
end

# adf_lball_frac(r, x): the rational as (num, den)
function frac(x::LB)
    r = rat(999, 1)
    st = ccall(ad(:adf_lball_frac), Cint, (Ptr{UInt8}, Ptr{UInt8}), r, x.ptr)
    q = (parse(BigInt, fmpz_str(r)), parse(BigInt, fmpz_str(r + 8)))
    freerat(r)
    return st, q
end

function pow(x::LB, k)
    y = LB()
    st = ccall(ad(:adf_lball_pow_si), Cint, (Ptr{UInt8}, Ptr{UInt8}, Clong), y.ptr, x.ptr, k)
    return st, y
end

function teich(pl::Place, r, n)
    w = LB()
    st = ccall(ad(:adf_lball_teichmuller), Cint, (Ptr{UInt8}, Place, Culong, Clong), w.ptr, pl, r, n)
    return st, w
end

# a modular inverse in BigInt
inv_mod(a, m) = invmod(mod(a, m), m)

@testset "adf_lball slice 1F.3-b through ccall" begin
    p5, p2 = place(5), place(2)

    # ---- 50/3 at p = 5: 5^2 (2/3), unit 2/3 = 4 modulo 5 = p - 1, so w = -1 (exact) and u = -2/3 (exact)
    st, x = set_rat(p5, 50, 3)
    @test st == OK
    st, m, idx, w, u = split(x, 8)
    @test st == OK && m == 2 && idx == 4
    @test isexact(w) && num(w) == -1 && den(w) == 1
    @test isexact(u) && 3 * (num(u) // den(u)) == -2         # u = -2/3
    @test value(w) * value(u) * 25 == 50 // 3                # x = 5^2 w u

    # the same as a ball 50/3 + O(5^7): relative precision 5, unit ball -(2/3) + O(5^5)
    st, xb = set_rat_ball(p5, 50, 3, 7)
    st, m, idx, w, u = split(xb, 8)
    @test st == OK && m == 2 && idx == 4 && isexact(w) && num(w) == -1
    @test !isexact(u) && val(u) == 0 && prec(u) == 5 && 0 < num(u) < BigInt(5)^5
    @test mod(3 * num(u) + 2, BigInt(5)^5) == 0              # 3 u = -2 modulo 5^5

    # a unit whose Teichmueller factor is irrational: 7 at 5, residue 2; precision 10
    st, x7 = set_rat_ball(p5, 7, 1, 12)
    st, m, idx, w, u = split(x7, 10)
    @test st == OK && m == 0 && idx == 2 && !isexact(w) && prec(w) == 10 && val(w) == 0
    @test powermod(num(w), 4, BigInt(5)^10) == 1             # w^(p-1) = 1 modulo 5^10
    @test mod(num(w), 5) == 2                                # w = 2 modulo 5
    @test prec(u) == 12 && mod(num(u) * num(w), BigInt(5)^10) == 7 % BigInt(5)^10     # w u = 7 modulo 5^10
    @test mod(num(u), 5) == 1                                # a principal unit

    # ---- -12 at p = 2: 2^2 (-3), -3 = 1 modulo 4, so w = +1, u = -3 (exact); the ball -12 + O(2^6): u = -3 + O(2^4)
    st, x = set_rat(p2, -12, 1)
    st, m, idx, w, u = split(x, 5)
    @test st == OK && m == 2 && idx == 1 && isexact(w) && num(w) == 1 && isexact(u) && num(u) == -3
    st, xb = set_rat_ball(p2, -12, 1, 6)
    st, m, idx, w, u = split(xb, 5)
    @test st == OK && m == 2 && idx == 1 && isexact(w) && num(w) == 1 && prec(u) == 4 && num(u) == 13   # -3 mod 16
    # 12 = 4 * 3: 3 = 3 modulo 4, so the sign is -1 and u = -3 = 1 modulo 4
    st, x12 = set_rat_ball(p2, 12, 1, 6)
    st, m, idx, w, u = split(x12, 5)
    @test st == OK && idx == 3 && isexact(w) && num(w) == -1 && num(u) == 13
    # relative precision 1 at p = 2: the sign is not determined
    st, x3 = set_rat_ball(p2, 3, 1, 1)
    @test split(x3, 5)[1] == NOT_DETERMINED
    # the exact 0 and a ball around 0
    st, z = set_rat(p5, 0, 1)
    @test split(z, 5)[1] == DOMAIN
    st, zb = set_rat_ball(p5, 0, 1, 4)
    @test split(zb, 5)[1] == NOT_DETERMINED

    # ---- the Teichmueller representative of 2 at 5, and the exact ones
    st, w = teich(p5, 2, 6)
    @test st == OK && !isexact(w) && powermod(num(w), 4, BigInt(5)^6) == 1 && mod(num(w), 5) == 2
    st, w = teich(p5, 9, 6)                                  # 9 = 4 = -1
    @test st == OK && isexact(w) && num(w) == -1
    @test teich(p5, 10, 6)[1] == DOMAIN && teich(ARCH, 2, 6)[1] == DOMAIN

    # ---- the fractional part: 7/25 at 5 is 7/25; 1/3 at 2 is integral, so 0; -1/4 at 2 is 3/4
    st, y = set_rat(p5, 7, 25)
    @test frac(y) == (OK, (7, 25))
    st, y = set_rat(p2, 1, 3)
    @test frac(y) == (OK, (0, 1))
    st, y = set_rat(p2, -1, 4)
    @test frac(y) == (OK, (3, 4))
    # a ball 7/25 + O(5^0) is determined, 7/25 + O(5^-1) is not
    st, y = set_rat_ball(p5, 7, 25, 0)
    @test frac(y) == (OK, (7, 25))
    st, y = set_rat_ball(p5, 7, 25, -1)
    @test frac(y)[1] == NOT_DETERMINED

    # ---- (1/3)^-7 at 5: exact 3^7 = 2187; as a ball 1/3 + O(5^10): 2187 + O(5^10)
    st, t = set_rat(p5, 1, 3)
    st, y = pow(t, -7)
    @test st == OK && isexact(y) && num(y) == 2187 && den(y) == 1
    st, tb = set_rat_ball(p5, 1, 3, 10)
    st, y = pow(tb, -7)
    @test st == OK && !isexact(y) && prec(y) == 10 && num(y) == 2187
    @test mod(num(y) * num(tb)^7, BigInt(5)^10) == 1         # y (1/3)^7 = 1 modulo 5^10: the centres are inverse
    # the factor v_5(k): (2 + O(5^2))^5 = 32 + O(5^3), and the inverse power
    st, b2 = set_rat_ball(p5, 2, 1, 2)
    st, y = pow(b2, 5)
    @test st == OK && prec(y) == 3 && num(y) == 32
    st, y = pow(b2, 0)
    @test st == OK && isexact(y) && num(y) == 1
    # statuses of pow: the exact 0 and a ball with 0 to a negative power
    @test pow(z, -1)[1] == NOT_UNIT && pow(zb, -1)[1] == UNIT_NOT_CERTIFIED
    @test pow(z, 3)[1] == OK && pow(t, 1 << 62)[1] == LIMIT
end
