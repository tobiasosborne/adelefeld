# Rational powers and principal-unit powers at a prime via ccall (lane f-slice9, SPEC 9.3.4 items 2 and 3).
# include/adelefeld/lpow.h; docs/api-1f6.md. Expected values, by hand or by integer enumeration:
#   9^(3/2) at 5: the square roots of 9 are 3 (identifier 3) and -3 (identifier 2); cubed: 27 and -27, exact.
#   (1 + 5)^(1/2) at 5 to 5^10: the x = 1 mod 5 with x^2 = 6 mod 5^10 is 6520516 (enumeration of all x).
#   (1 + 5)^s, s = 1/2 + 5^3 Z_5 (centre 63 = 1/2 mod 125): alpha = v(6 - 1) = 1, R = B + alpha = 4
#   (Proposition 18, docs/proofs/functions.md:616): 6520516 mod 5^4 = 516, exponent 4.
#   2^(1/2) at 2: the valuation 1 is odd, no root: DOMAIN. (1 + 4 Z_2)^(1/2): outside the guard of SPEC 9.3.3
#   (5 = 1 mod 4 is no square): NOT_DETERMINED.
using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia lpow.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN = 0, 1, 7
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
    ccall(ad(:adf_lball_set_rat), Cint, (Ptr{UInt8}, Place, Ptr{UInt8}), x.ptr, place(p), r) == OK || error("set")
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

function powrat(x::LB, e::Integer, n::Integer, seed::Integer, N::Integer)
    y = LB()
    st = ccall(ad(:adf_lball_powrat), Cint, (Ptr{UInt8}, Ptr{UInt8}, Clong, Culong, Culong, Clong),
               y.ptr, x.ptr, e, n, seed, N)
    return st, y
end
function powunit(u::LB, s::LB, N::Integer)
    y = LB()
    st = ccall(ad(:adf_lball_powunit), Cint, (Ptr{UInt8}, Ptr{UInt8}, Ptr{UInt8}, Clong), y.ptr, u.ptr, s.ptr, N)
    return st, y
end

@testset "rational powers and principal-unit powers" begin
    nine = exact(5, 9)
    for (seed, expected) in ((3, 27), (2, -27))
        st, y = powrat(nine, 3, 2, seed, 20)
        @test st == OK && isexact(y) && val(y) == 0 && num(y) == expected && den(y) == 1
        println("9^(3/2) at 5 on the branch [", seed, "]: ", num(y), " (exact)")
    end
    six = exact(5, 6)
    st, y = powunit(six, exact(5, 1, 2), 10)
    @test st == OK && !isexact(y) && val(y) == 0 && prec(y) == 10 && num(y) == 6520516
    println("(1 + 5)^(1/2) at 5: ", num(y), " + O(5^", prec(y), ")")
    s = ball(5, 1, 2, 3)
    @test num(s) == 63 && prec(s) == 3
    st, y = powunit(six, s, 10)
    @test st == OK && !isexact(y) && prec(y) == 4 && num(y) == 516
    println("(1 + 5)^(1/2 + O(5^3)) at 5: ", num(y), " + O(5^", prec(y), ")")
    @test powrat(exact(2, 2), 1, 2, 1, 20)[1] == DOMAIN
    println("2^(1/2) at 2: DOMAIN")
    @test powrat(ball(2, 1, 1, 2), 1, 2, 1, 20)[1] == NOT_DETERMINED
    println("(1 + 4 Z_2)^(1/2): NOT_DETERMINED")
end
