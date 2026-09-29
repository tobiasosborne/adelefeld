# tests/julia/idclass.jl: Julia ccalls of the second slice of milestone 2 (lane i-slice2; docs/api-2.md
# section 2; include/adelefeld/idele.h, include/adelefeld/idclass.h): a user makes the idele of -6/35, reads
# its valuations at 2, 3, 5, 7, 11, its absolute values, its norm and its class, and checks the product
# formula and the class of q times the idele.
#
# Usage: julia --startup-file=no tests/julia/idclass.jl build/libadelefeld.so
# (run by tests/test_julia.sh, with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp
# lacks a symbol; see the header of that script).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's own
# adf_sizeof_* functions; FLINT's fmpz, fmpq and arb are made and read through libflint. Statuses
# (include/adelefeld/status.h): OK 0, NOT_DETERMINED 1, DOMAIN 7, LIMIT 10.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia idclass.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN, LIMIT = 0, 1, 7, 10

# a place is an 8-byte struct passed by value (conventions 7, 12.5)
struct Place
    opaque::UInt64
end

function place(p::Integer)
    v = Ref(Place(0))
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, p) == OK
    return v[]
end

const INF = ccall(ad(:adf_place_inf), Place, ())

alloc(sizefn) = Libc.malloc(ccall(ad(sizefn), Csize_t, ()))

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# an adf_rat holds one fmpq at offset 0: read it as "n/d" (fmpq_get_str)
function rat_str(q::Ptr)
    s = ccall(fl(:fmpq_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, q)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

function new_rat(n::Integer, d::Integer)
    q = alloc(:adf_sizeof_rat)
    ccall(ad(:adf_rat_init), Cvoid, (Ptr{Cvoid},), q)
    ccall(fl(:fmpq_set_si), Cvoid, (Ptr{Cvoid}, Clong, Culong), q, n, d)
    return q
end

free_rat(q) = (ccall(ad(:adf_rat_clear), Cvoid, (Ptr{Cvoid},), q); Libc.free(q))

function new_arb()
    a = Libc.malloc(48)
    ccall(fl(:arb_init), Cvoid, (Ptr{Cvoid},), a)
    return a
end

free_arb(a) = (ccall(fl(:arb_clear), Cvoid, (Ptr{Cvoid},), a); Libc.free(a))

arb_has(a, n, d) = begin
    f = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), f)
    ccall(fl(:fmpq_set_si), Cvoid, (Ptr{Cvoid}, Clong, Culong), f, n, d)
    r = ccall(fl(:arb_contains_fmpq), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), a, f) != 0
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), f)
    Libc.free(f)
    r
end
arb_pos(a) = ccall(fl(:arb_is_positive), Cint, (Ptr{Cvoid},), a) != 0
arb_neg(a) = ccall(fl(:arb_is_negative), Cint, (Ptr{Cvoid},), a) != 0
arb_one(a) = ccall(fl(:arb_is_one), Cint, (Ptr{Cvoid},), a) != 0
arb_overlap(a, b) = ccall(fl(:arb_overlaps), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), a, b) != 0
function arb_str(a)
    s = ccall(fl(:arb_get_str), Ptr{UInt8}, (Ptr{Cvoid}, Clong, Culong), a, 15, 0)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

function new_idele()
    x = alloc(:adf_sizeof_idele)
    ccall(ad(:adf_idele_init), Cvoid, (Ptr{Cvoid},), x)
    return x
end

free_idele(x) = (ccall(ad(:adf_idele_clear), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

function new_class()
    c = alloc(:adf_sizeof_idclass)
    ccall(ad(:adf_idclass_init), Cvoid, (Ptr{Cvoid},), c)
    return c
end

free_class(c) = (ccall(ad(:adf_idclass_clear), Cvoid, (Ptr{Cvoid},), c); Libc.free(c))

# the unit coset (c, N) of a class, as strings
function class_unit(c)
    u = alloc(:adf_sizeof_ucoset)
    ccall(ad(:adf_ucoset_init), Cvoid, (Ptr{Cvoid},), u)
    ccall(ad(:adf_idclass_get_unit), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), u, c)
    a, N = Libc.malloc(8), Libc.malloc(8)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), a)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), N)
    ccall(ad(:adf_ucoset_get_fmpz2), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), a, N, u)
    out = (fmpz_str(a), fmpz_str(N))
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), a)
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), N)
    Libc.free(a); Libc.free(N)
    ccall(ad(:adf_ucoset_clear), Cvoid, (Ptr{Cvoid},), u)
    Libc.free(u)
    return out
end

valuation(x, w) = begin
    v = Ref{Clong}(-777)
    st = ccall(ad(:adf_idele_valuation_at), Cint, (Ref{Clong}, Ptr{Cvoid}, Place), v, x, w)
    (st, v[])
end

function abs_at(x, w)
    a = new_rat(0, 1)
    st = ccall(ad(:adf_idele_abs_at), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Place), a, x, w)
    s = rat_str(a)
    free_rat(a)
    return st, s
end

@testset "idele classes, valuations and the norm through ccall (milestone 2, slice 2)" begin
    @test ccall(ad(:adf_sizeof_idclass), Csize_t, ()) == 64
    @test ccall(ad(:adf_alignof_idclass), Csize_t, ()) == 8

    # the idele of -6/35 at 64 bits
    q = new_rat(-6, 35)
    x = new_idele()
    @test ccall(ad(:adf_idele_set_rat), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), x, q, 64) == OK

    # valuations and absolute values at 2, 3, 5, 7, 11: v_p(6/35) and p^(-v)
    want_v = Dict(2 => 1, 3 => 1, 5 => -1, 7 => -1, 11 => 0)
    want_a = Dict(2 => "1/2", 3 => "1/3", 5 => "5", 7 => "7", 11 => "1")
    prod = 1 // 1
    for p in (2, 3, 5, 7, 11)
        st, v = valuation(x, place(p))
        @test st == OK && v == want_v[p]
        st, a = abs_at(x, place(p))
        @test st == OK && a == want_a[p]
        parts = split(a, "/")
        prod *= parse(Int, parts[1]) // (length(parts) == 2 ? parse(Int, parts[2]) : 1)
        println("v_$p = $v, |x|_$p = $a")
    end
    @test prod == 35 // 6                                   # the product of |x|_p is 1/r (P14.2)
    @test valuation(x, INF)[1] == DOMAIN && valuation(x, INF)[2] == -777
    @test abs_at(x, INF)[1] == DOMAIN

    # |x|_inf and the norm: |x_inf| contains 6/35; the norm contains 1 (product formula) and is positive
    a = new_arb()
    ccall(ad(:adf_idele_abs_inf), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), a, x)
    @test arb_has(a, 6, 35) && arb_pos(a)
    println("|x|_inf = ", arb_str(a))
    t = new_arb()
    @test ccall(ad(:adf_idele_norm), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), t, x, 64) == OK
    @test arb_has(t, 1, 1) && arb_pos(t) && !arb_one(t)      # 6/35 is not dyadic: a ball around 1
    println("norm = ", arb_str(t))

    # its class: <t ; [1]>, t around 1; the class of a rational is the class of 1
    c = new_class()
    @test ccall(ad(:adf_idclass_set_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), c, x, 64) == OK
    ct = new_arb()
    ccall(ad(:adf_idclass_get_t), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), ct, c)
    @test class_unit(c) == ("1", "0") && arb_has(ct, 1, 1) && arb_pos(ct)
    println("class = <", arb_str(ct), " ; [", class_unit(c)[1], " mod ", class_unit(c)[2], "]>")

    # the idele of -3/4 is exact at 64 bits: its norm and its class t are the exact 1
    q2 = new_rat(-3, 4)
    y = new_idele()
    @test ccall(ad(:adf_idele_set_rat), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), y, q2, 64) == OK
    @test ccall(ad(:adf_idele_norm), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), t, y, 64) == OK && arb_one(t)

    # q times the idele has the same class: (-3/4) * x, the unit [1] again, t overlapping
    z = new_idele()
    @test ccall(ad(:adf_idele_mul_rat), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, q2, 64) == OK
    cz = new_class()
    @test ccall(ad(:adf_idclass_set_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), cz, z, 64) == OK
    czt = new_arb()
    ccall(ad(:adf_idclass_get_t), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), czt, cz)
    @test class_unit(cz) == class_unit(c) && arb_overlap(czt, ct)
    @test valuation(z, place(2)) == (OK, -1)                # v_2(6/35 * 3/4) = 1 - 2

    # the class of (1 ; 1 [-1]) is <1 ; [-1]>, not the class of a rational: the sign is on the unit (P15.3)
    inf1 = new_arb()
    ccall(fl(:arb_one), Cvoid, (Ptr{Cvoid},), inf1)
    one = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), one)
    ccall(fl(:fmpq_one), Cvoid, (Ptr{Cvoid},), one)
    um = alloc(:adf_sizeof_ucoset)
    ccall(ad(:adf_ucoset_init), Cvoid, (Ptr{Cvoid},), um)
    ccall(ad(:adf_ucoset_minus_one), Cvoid, (Ptr{Cvoid},), um)
    w = new_idele()
    @test ccall(ad(:adf_idele_set_parts), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}),
                w, inf1, one, um) == OK
    cw = new_class()
    @test ccall(ad(:adf_idclass_set_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), cw, w, 64) == OK
    cwt = new_arb()
    ccall(ad(:adf_idclass_norm), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), cwt, cw)
    @test class_unit(cw) == ("-1", "0") && arb_one(cwt)
    println("the class of (1 ; 1 [-1]) is <1 ; [-1 mod 0]>")

    # the precision limit: LIMIT above ADF_IDELE_PREC_MAX = 2^21, the class untouched
    @test ccall(ad(:adf_idclass_set_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), cw, x, typemax(Clong)) == LIMIT
    @test class_unit(cw) == ("-1", "0")

    foreach(free_arb, (a, t, ct, czt, inf1, cwt))
    foreach(free_class, (c, cz, cw))
    foreach(free_idele, (x, y, z, w))
    free_rat(q); free_rat(q2)
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), one); Libc.free(one)
    ccall(ad(:adf_ucoset_clear), Cvoid, (Ptr{Cvoid},), um); Libc.free(um)
end
