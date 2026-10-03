# Functions at all places via ccall (lane f-slice10, WP 1F.8; include/adelefeld/gfunc.h; docs/api-1f8.md).
# Expected values, by hand (SPEC 9.3.3 lines 636-646, PLAN row 1F.8):
#   the root of an exact rational: 8 has the cube root 2; 2 has no square root (DOMAIN, no place named: the first
#   failing prime is not searched); 1 has the rational square roots 1 (sign +1) and -1 (sign -1); -4 has no square
#   root (DOMAIN at the real place).
#   the idele (4, 4, [1]) (exact unit) has the square root (2, 2, [1]); the idele (4, 4, [1 mod 8]) of finite
#   precision is NOT_DETERMINED for n = 2 (some unit outside the modulus is not a square, Proposition 16).
using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia gfunc.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN = 0, 1, 7

struct Place
    opaque::UInt64
end

isreal(w::Place) = ccall(ad(:adf_place_is_archimedean), Cint, (Place,), w) == 1
function sentinel()
    v = Ref(Place(0))
    ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, 97) == OK || error("97")
    return v
end
issentinel(w::Ref{Place}) = ccall(ad(:adf_place_prime_get), Culong, (Place,), w[]) == 97

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# An object of the library, allocated by its sizeof query and released by its clear.
mutable struct Obj
    ptr::Ptr{UInt8}
    function Obj(kind::Symbol)
        p = Ptr{UInt8}(Libc.malloc(ccall(ad(Symbol(:adf_sizeof_, kind)), Csize_t, ())))
        ccall(ad(Symbol(:adf_, kind, :_init)), Cvoid, (Ptr{UInt8},), p)
        x = new(p)
        clear = ad(Symbol(:adf_, kind, :_clear))
        finalizer(x -> (ccall(clear, Cvoid, (Ptr{UInt8},), x.ptr); Libc.free(x.ptr)), x)
        return x
    end
end

# adf_rat: one fmpq, the numerator at offset 0 and the denominator at 8 (rat.h layout).
function rat(n::Integer, d::Integer = 1)
    r = Obj(:rat)
    fn, fd = Ref{Clong}(0), Ref{Clong}(0)
    for (f, x) in ((fn, n), (fd, d))
        ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), f)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), f, string(x), 10) == 0 || error("bad integer")
    end
    ccall(ad(:adf_rat_set_fmpz2), Cint, (Ptr{UInt8}, Ptr{Clong}, Ptr{Clong}), r.ptr, fn, fd) == OK || error("rat")
    for f in (fn, fd)
        ccall(fl(:fmpz_clear), Cvoid, (Ptr{Clong},), f)
    end
    return r
end
ratval(r::Obj) = parse(BigInt, fmpz_str(r.ptr)) // parse(BigInt, fmpz_str(r.ptr + 8))

function rat_root(a::Obj, n::Integer, sign::Integer)
    y, w = rat(0), sentinel()
    st = ccall(ad(:adf_rat_root), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Culong, Cint),
               y.ptr, w, a.ptr, n, sign)
    return st, y, w
end

# The value text of an idele, through the library's printer (text.h).
function idele_text(x::Obj)
    len = Ref{Csize_t}(0)
    s = ccall(ad(:adf_idele_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{UInt8}, Clong), len, x.ptr, 20)
    s == C_NULL && error("printer")
    t = unsafe_string(s, len[])
    ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
    return t
end
function idele_parse(text::String)
    x = Obj(:idele)
    st = ccall(ad(:adf_idele_set_str), Cint, (Ptr{UInt8}, Cstring, Csize_t, Clong, Ptr{Cvoid}),
               x.ptr, text, sizeof(text), 64, C_NULL)
    st == OK || error("parse $text: $st")
    return x
end
function idele_root(x::Obj, n::Integer, sign::Integer)
    y, w = Obj(:idele), sentinel()
    st = ccall(ad(:adf_idele_root), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Culong, Cint, Clong),
               y.ptr, w, x.ptr, n, sign, 64)
    return st, y, w
end

# Slice B (SPEC 9.3.2 lines 588-596): exp of (1/2 ; 0) is (e^(1/2) ; 1), e^(1/2) = 1.6487212707001281...; cos of
# (0 ; 0) is (1 ; 1) exactly; exp of the rational 1, the adele (1 ; 1), is DOMAIN at the prime 2 (v_2(1) = 0 < 2).
function adele_parse(text::String)
    x = Obj(:adele)
    st = ccall(ad(:adf_adele_set_str), Cint, (Ptr{UInt8}, Cstring, Csize_t, Clong, Ptr{Cvoid}),
               x.ptr, text, sizeof(text), 64, C_NULL)
    st == OK || error("parse $text: $st")
    return x
end
function adele_text(x::Obj, digits::Integer)
    len = Ref{Csize_t}(0)
    s = ccall(ad(:adf_adele_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{UInt8}, Clong), len, x.ptr, digits)
    s == C_NULL && error("printer")
    t = unsafe_string(s, len[])
    ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
    return t
end
function series(name::Symbol, x::Obj)
    y, w = Obj(:adele), sentinel()
    st = ccall(ad(Symbol(:adf_adele_, name)), Cint, (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Clong),
               y.ptr, w, x.ptr, 64)
    return st, y, w
end

@testset "the five series at all places" begin
    st, y, w = series(:exp, adele_parse("(0.5 ; 0)"))
    t = adele_text(y, 10)
    @test st == OK && endswith(t, "; 1)")
    m, r = parse(BigFloat, split(t[2:end])[1]), parse(BigFloat, split(t)[3])
    @test abs(m - exp(big(0.5))) <= r
    println("exp (1/2 ; 0): ", t)
    st, y, w = series(:cos, adele_parse("(0 ; 0)"))
    @test st == OK && adele_text(y, 10) == "(1 ; 1)"
    println("cos (0 ; 0): ", adele_text(y, 10))
    st, y, w = series(:exp, adele_parse("(1 ; 1)"))
    @test st == DOMAIN && !isreal(w[]) && ccall(ad(:adf_place_prime_get), Culong, (Place,), w[]) == 2
    println("exp 1 = exp (1 ; 1): DOMAIN at the prime 2")
end

@testset "the root at all places" begin
    st, y, w = rat_root(rat(8), 3, 1)
    @test st == OK && ratval(y) == 2 && issentinel(w)
    println("root 8 with 3: ", ratval(y))
    st, y, w = rat_root(rat(2), 2, 1)
    @test st == DOMAIN && issentinel(w)
    println("root 2 with 2: DOMAIN (no place named)")
    for (sign, expected) in ((1, 1), (-1, -1))
        st, y, w = rat_root(rat(1), 2, sign)
        @test st == OK && ratval(y) == expected
        println("root 1 with 2 with ", sign, ": ", ratval(y))
    end
    st, y, w = rat_root(rat(-4), 2, 1)
    @test st == DOMAIN && isreal(w[])
    println("root -4 with 2: DOMAIN at the real place")
    st, y, w = idele_root(idele_parse("(4 ; 4 * [1])"), 2, 1)
    @test st == OK && idele_text(y) == "(2 ; 2 * [1])"
    println("root (4 ; 4 * [1]) with 2: ", idele_text(y))
    st, y, w = idele_root(idele_parse("(4 ; 4 * [1 mod 8])"), 2, 1)
    @test st == NOT_DETERMINED && issentinel(w)
    println("root (4 ; 4 * [1 mod 8]) with 2: NOT_DETERMINED")
end
