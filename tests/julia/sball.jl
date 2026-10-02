# tests/julia/sball.jl: Julia ccalls of adf_sball and the real functions (lane f-slice2; include/adelefeld/sball.h,
# include/adelefeld/rfunc.h; docs/api-1f.md statements S1 to S7).
#
# What a user does: makes the adele of 2/3 at 200 bits, projects it to the places {2, real, 5} (any order), reads the
# three components, takes log at the real place, and meets the statuses and the reported places. Every number is read
# back through the library's accessors and FLINT's arb_get_str / fmpz_get_str, and compared with values Julia
# computes itself (BigInt and BigFloat), not with a table of results.
#
# Usage: julia --startup-file=no tests/julia/sball.jl build/libadelefeld.so
# (with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a symbol; tests/test_julia.sh does
# that.) Only Libdl and Test of the standard library. Values are raw memory sized by the library's own
# adf_sizeof_* queries. The layout of adf_lball_struct (conventions 5.8): p at 0, u = fmpq at 8 (num at 8, den at 16),
# v at 24, N at 32, exact (int) at 40; size 48. The layout of adf_sball_struct (conventions 5.9, sball.h): arch (int)
# at 0, inf (acb) at 8, len (slong) at 104, loc (pointer) at 112; size 120.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia sball.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

# statuses (include/adelefeld/status.h)
const OK, NOT_DETERMINED, UNIT_NOT_CERTIFIED, NOT_UNIT, DOMAIN, UNSUPPORTED, LIMIT = 0, 1, 2, 6, 7, 8, 10
const ARCH_NONE, ARCH_REAL, ARCH_COMPLEX = 0, 1, 2

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
isarch(v::Place) = ccall(ad(:adf_place_is_archimedean), Cint, (Place,), v) != 0
primeof(v::Place) = ccall(ad(:adf_place_prime_get), Culong, (Place,), v)

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# an adf_lball in raw memory, as in lball.jl
const OFF_NUM, OFF_DEN, OFF_V, OFF_N, OFF_EXACT = 8, 16, 24, 32, 40
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
function getplace(s::SB, i)
    v = Ref(Place(0))
    st = ccall(ad(:adf_sball_get_place), Cint, (Ref{Place}, Ptr{UInt8}, Clong), v, s.ptr, i)
    return st, v[]
end
function getlball(s::SB, v::Place)
    c = LB()
    st = ccall(ad(:adf_sball_get_lball), Cint, (Ptr{UInt8}, Ptr{UInt8}, Place), c.ptr, s.ptr, v)
    return st, c
end

# an arb, as text through FLINT
function newarb()
    p = Ptr{UInt8}(Libc.calloc(1, 48))
    ccall(fl(:arb_init), Cvoid, (Ptr{UInt8},), p)
    return p
end
# clears and frees what newarb made (finding R6: realtext used to leave its temporary arb behind)
function freearb(p::Ptr{UInt8})
    ccall(fl(:arb_clear), Cvoid, (Ptr{UInt8},), p)
    Libc.free(p)
end
function arbtext(p::Ptr, digits = 70)
    s = ccall(fl(:arb_get_str), Ptr{UInt8}, (Ptr{UInt8}, Clong, Culong), p, digits, 0)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end
setprecision(BigFloat, 300)
# "[0.666... +/- 1.2e-61]" (arb_get_str) as (midpoint, radius) in BigFloat; the midpoint is printed to 70 digits
function arbval(txt)
    m = match(r"^\[?([-0-9.e+]+)(?: \+/- ([-0-9.e+]+))?\]?$", txt)
    m === nothing && error("cannot read an arb text: $txt")
    return parse(BigFloat, m.captures[1]), m.captures[2] === nothing ? BigFloat(0) : parse(BigFloat, m.captures[2])
end
# the text describes a ball that contains `want` (up to the 70 printed digits) and is narrower than 2^-190
function ball_around(txt, want)
    mid, rad = arbval(txt)
    return abs(mid - want) <= rad + BigFloat(10)^-66 && rad < BigFloat(2)^-190
end
function realtext(s::SB, digits = 70)
    r = newarb()
    st = ccall(ad(:adf_sball_get_arb), Cint, (Ptr{UInt8}, Ptr{UInt8}, Place), r, s.ptr, ARCH)
    txt = st == OK ? arbtext(r, digits) : ""
    freearb(r)
    return st, txt
end

# an adf_rat holding n/d, and an adele holding it at `prec` bits. Every adele made is listed in ADELES and cleared and
# freed at the end of the test set (finding R6).
const ADELES = Ptr{UInt8}[]
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
    push!(ADELES, a)
    return a
end

function project(a, places::Vector{Place})
    s, where = SB(), Ref(Place(0))
    st = ccall(ad(:adf_sball_project), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Ptr{Place}, Clong),
               s.ptr, where, a, places, length(places))
    return st, s, where[]
end

function binop(name::Symbol, x::SB, y::SB, prec = 200)
    z, where = SB(), Ref(Place(0))
    st = ccall(ad(name), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Ptr{UInt8}, Clong), z.ptr, where, x.ptr, y.ptr, prec)
    return st, z, where[]
end
function unary_at(name::Symbol, x::SB, v::Place, prec = 200)
    z, where = SB(), Ref(Place(0))
    st = ccall(ad(name), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Place, Clong), z.ptr, where, x.ptr, v, prec)
    return st, z, where[]
end
function root_at(x::SB, v::Place, n, prec = 200)
    z, where = SB(), Ref(Place(0))
    st = ccall(ad(:adf_sball_root_at), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Place, Culong, Clong),
               z.ptr, where, x.ptr, v, n, prec)
    return st, z, where[]
end
pred(name::Symbol, x::SB, y::SB) = ccall(ad(name), Cint, (Ptr{UInt8}, Ptr{UInt8}), x.ptr, y.ptr) != 0

@testset "adf_sball and the real functions through ccall" begin
    @test ccall(ad(:adf_sizeof_sball), Csize_t, ()) == 120
    @test ccall(ad(:adf_alignof_sball), Csize_t, ()) == 8

    p2, p5, p3 = place(2), place(5), place(3)
    third2 = adele_of(2, 3)

    # the projection of 2/3 to {2, real, 5}, given in this order: the result lists the real place first, then 2, 5
    st, s, _ = project(third2, [p2, ARCH, p5])
    @test st == OK
    @test nplaces(s) == 3 && arch(s) == ARCH_REAL
    @test isarch(getplace(s, 0)[2]) && primeof(getplace(s, 1)[2]) == 2 && primeof(getplace(s, 2)[2]) == 5
    @test getplace(s, 3)[1] == DOMAIN && getplace(s, -1)[1] == DOMAIN

    # the components: at 2 the exact 2/3 = 2^1 (1/3); at 5 the exact 2/3, a unit; the real one a ball around 2/3
    st, c2 = getlball(s, p2)
    @test st == OK && isexact(c2) && prime(c2) == 2 && val(c2) == 1 && num(c2) == 1 && den(c2) == 3
    st, c5 = getlball(s, p5)
    @test st == OK && isexact(c5) && prime(c5) == 5 && val(c5) == 0 && num(c5) == 2 && den(c5) == 3
    @test getlball(s, p3)[1] == DOMAIN && getlball(s, ARCH)[1] == DOMAIN
    st, txt = realtext(s)
    @test st == OK && ball_around(txt, BigFloat(2) / 3)

    # log at the real place: a partial ball over the one place; the components of the other places are not in it
    st, l, where = unary_at(:adf_sball_log_at, s, ARCH)
    @test st == OK && nplaces(l) == 1 && arch(l) == ARCH_REAL && isarch(getplace(l, 0)[2])
    st, txt = realtext(l)
    @test st == OK && ball_around(txt, log(BigFloat(2) / 3))   # -0.405465108108164381978013115464349137...

    # statuses and the reported place: a prime that is not in the set; a prime that is (functions at primes: later)
    st, _, where = unary_at(:adf_sball_log_at, s, p3)
    @test st == DOMAIN && primeof(where) == 3
    st, _, where = unary_at(:adf_sball_exp_at, s, p2)
    @test st == DOMAIN && primeof(where) == 2   # lane f-slice6: exp exists at a prime; 2/3 has v_2 = 1 < 2: DOMAIN
    st, _, where = unary_at(:adf_sball_sin_at, s, p2)
    @test st == DOMAIN && primeof(where) == 2   # v_2(2/3)=1 is outside 4 Z_2
    # log of a negative real ball: DOMAIN at the real place; the odd root and the exp are fine
    neg = adele_of(-2, 3)
    st, sn, _ = project(neg, [ARCH, p2])
    @test st == OK
    st, _, where = unary_at(:adf_sball_log_at, sn, ARCH)
    @test st == DOMAIN && isarch(where)
    st, _, where = unary_at(:adf_sball_sqrt_at, sn, ARCH)
    @test st == DOMAIN && isarch(where)
    st, r3, _ = root_at(sn, ARCH, 3)
    @test st == OK
    @test ball_around(realtext(r3)[2], -cbrt(BigFloat(2) / 3))   # -0.8735804647362989...
    st, _, where = unary_at(:adf_sball_log_abs_at, sn, ARCH)
    @test st == OK
    st, e, _ = unary_at(:adf_sball_exp_at, sn, ARCH)
    @test st == OK && ball_around(realtext(e)[2], exp(BigFloat(-2) / 3))   # 0.513417119032592...
    # a root of degree 0: DOMAIN
    @test root_at(sn, ARCH, 0)[1] == DOMAIN

    # componentwise sum and product over the same places
    st, s2, _ = project(third2, [p2, ARCH, p5])
    st, sum, _ = binop(:adf_sball_add, s, s2)
    @test st == OK && nplaces(sum) == 3
    c = getlball(sum, p2)[2]
    @test isexact(c) && val(c) == 2 && num(c) == 1 && den(c) == 3    # 4/3 = 2^2 (1/3)
    @test ball_around(realtext(sum)[2], BigFloat(4) / 3)
    st, prod, _ = binop(:adf_sball_mul, s, s2)
    @test st == OK
    c = getlball(prod, p5)[2]
    @test isexact(c) && val(c) == 0 && num(c) == 4 && den(c) == 9    # 4/9
    @test ball_around(realtext(prod)[2], BigFloat(4) / 9)
    # the product is inside itself, equal to itself, and overlaps the sum's real ball? (different places' values)
    @test pred(:adf_sball_equal_set, prod, prod) && pred(:adf_sball_contains, prod, prod)
    @test !pred(:adf_sball_equal_set, prod, sum) && !pred(:adf_sball_overlaps, prod, sum)

    # a different set of places: DOMAIN, the place named is the first that is in one operand only
    st, s25, _ = project(third2, [p2, p5, ARCH])
    st, s2i, _ = project(third2, [p2, ARCH])
    st, _, where = binop(:adf_sball_add, s25, s2i)
    @test st == DOMAIN && primeof(where) == 5
    st, s5, _ = project(third2, [p2, p5])
    st, _, where = binop(:adf_sball_mul, s5, s25)
    @test st == DOMAIN && isarch(where)
    @test !pred(:adf_sball_equal_set, s25, s2i)

    # a repeated place in the list: DOMAIN with that place, nothing written
    st, _, where = project(third2, [p2, p5, p2])
    @test st == DOMAIN && primeof(where) == 2

    # the adele of the exact 1: the components at 2, 3, 5 are the exact 1
    b = adele_of(1, 1)
    st, sb, _ = project(b, [p2, p3, p5, ARCH])
    @test st == OK && isexact(getlball(sb, p3)[2]) && num(getlball(sb, p3)[2]) == 1

    # what the test made is cleared: the adeles (the arb temporaries are cleared in realtext, the fmpz in adele_of)
    for a in ADELES
        ccall(ad(:adf_adele_clear), Cvoid, (Ptr{UInt8},), a)
        Libc.free(a)
    end
    empty!(ADELES)
end
