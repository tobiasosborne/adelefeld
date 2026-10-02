# Local roots via ccall; exact references 3^2=(-3)^2=9.
# Proposition 13, docs/proofs/functions.md:410: 3 mod 8 fails at 2; cubes mod 7 are 0,1,6.
# Proposition 15:463: 1+4 Z_2 includes the nonsquare 5, so the strong guard fails.
using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia lroot.jl <path-to-libadelefeld.so>"); exit(2))

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

function root(x::LB, n::Integer, seed::Integer, N::Integer)
    y = LB()
    st = ccall(ad(:adf_lball_root_seed), Cint,
               (Ptr{UInt8}, Ptr{UInt8}, Culong, Culong, Clong), y.ptr, x.ptr, n, seed, N)
    return st, y
end

@testset "local roots with explicit branches" begin
    x = exact(5, 9)
    count = Ref{Culong}(99)
    @test ccall(ad(:adf_lball_root_count), Cint,
                (Ref{Culong}, Ptr{UInt8}, Culong), count, x.ptr, 2) == OK
    @test count[] == 2
    for (seed, expected) in ((2, -3), (3, 3))
        st, y = root(x, 2, seed, 20)
        @test st == OK && isexact(y) && val(y) == 0 && num(y) == expected && den(y) == 1
        println("sqrt(9) at 5 [", seed, "]: ", num(y), " (exact)")
    end
    @test root(exact(2, 3), 2, 1, 20)[1] == DOMAIN
    println("sqrt(3) at 2: DOMAIN")
    @test root(exact(7, 2), 3, 1, 20)[1] == DOMAIN
    println("root(2,3) at 7: DOMAIN")
    @test root(ball(2, 1, 1, 2), 2, 1, 20)[1] == NOT_DETERMINED
    println("sqrt(1 + 4 Z_2): NOT_DETERMINED")
    st, y = root(ball(2, 1, 1, 4), 2, 3, 20)
    @test st == OK && !isexact(y) && prec(y) == 3 && num(y) == 7
end
