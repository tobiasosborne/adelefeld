# tests/julia/idmap.jl: Julia ccalls of the third slice of milestone 2 (lane i-slice3; docs/api-2.md section 3;
# include/adelefeld/idpow.h, include/adelefeld/idmap.h): a user makes the idele of -6/35, squares it, maps it to an
# adele, divides the adele of 1/2 by it, and reads the results; then the tight power of a unit coset, the smallest
# hull of a coset, the way back from an adele to an idele and its refusals, and the limits.
#
# Usage: julia --startup-file=no tests/julia/idmap.jl build/libadelefeld.so
# (run by tests/test_julia.sh, with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a
# symbol; see the header of that script).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's own
# adf_sizeof_* functions; FLINT's fmpz, fmpq and arb are made and read through libflint. Statuses
# (include/adelefeld/status.h): OK 0, UNIT_NOT_CERTIFIED 2, NOT_UNIT 6, LIMIT 10.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia idmap.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, UNIT_NOT_CERTIFIED, NOT_UNIT, LIMIT = 0, 2, 6, 10

alloc(sizefn) = Libc.malloc(ccall(ad(sizefn), Csize_t, ()))

function flint_str(fn, p::Ptr)
    s = ccall(fl(fn), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end
fmpz_str(p) = flint_str(:fmpz_get_str, p)
fmpq_str(p) = flint_str(:fmpq_get_str, p)          # an adf_rat holds one fmpq at offset 0

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

function arb_has(a, n, d)
    f = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), f)
    ccall(fl(:fmpq_set_si), Cvoid, (Ptr{Cvoid}, Clong, Culong), f, n, d)
    r = ccall(fl(:arb_contains_fmpq), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), a, f) != 0
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), f)
    Libc.free(f)
    return r
end
arb_pos(a) = ccall(fl(:arb_is_positive), Cint, (Ptr{Cvoid},), a) != 0
function arb_str(a)
    s = ccall(fl(:arb_get_str), Ptr{UInt8}, (Ptr{Cvoid}, Clong, Culong), a, 15, 0)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

function new_value(prefix)
    x = alloc(Symbol("adf_sizeof_", prefix))
    ccall(ad(Symbol("adf_", prefix, "_init")), Cvoid, (Ptr{Cvoid},), x)
    return x
end
free_value(prefix, x) = (ccall(ad(Symbol("adf_", prefix, "_clear")), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

# the stored pair (c, N) of a unit coset, as strings
function uc_pair(u)
    a, N = Libc.malloc(8), Libc.malloc(8)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), a)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), N)
    ccall(ad(:adf_ucoset_get_fmpz2), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), a, N, u)
    out = (fmpz_str(a), fmpz_str(N))
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), a)
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), N)
    Libc.free(a); Libc.free(N)
    return out
end

function new_uc(c::Integer, N::Integer)
    u = new_value(:ucoset)
    a, M = Libc.malloc(8), Libc.malloc(8)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), a)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), M)
    ccall(fl(:fmpz_set_si), Cvoid, (Ptr{Cvoid}, Clong), a, c)
    ccall(fl(:fmpz_set_si), Cvoid, (Ptr{Cvoid}, Clong), M, N)
    @test ccall(ad(:adf_ucoset_set_fmpz2), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), u, a, M) == OK
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), a)
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), M)
    Libc.free(a); Libc.free(M)
    return u
end

# the content and the unit of an idele
function idele_finite(x)
    r = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), r)
    ccall(ad(:adf_idele_content), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), r, x)
    rs = fmpq_str(r)
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), r)
    Libc.free(r)
    u = new_value(:ucoset)
    ccall(ad(:adf_idele_get_unit), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), u, x)
    us = uc_pair(u)
    free_value(:ucoset, u)
    return rs, us
end

# the finite part of an adele as its value text, e.g. "(* ; 36/1225)" or "(* ; 5 mod 6)"
function adele_fin_text(a)
    f = new_value(:fball)
    ccall(ad(:adf_adele_get_fin), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), f, a)
    len = Ref{Csize_t}(0)
    s = ccall(ad(:adf_fball_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}), len, f)
    str = unsafe_string(s, len[])
    ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
    free_value(:fball, f)
    return str
end

function adele_real(a)
    t = new_arb()
    ccall(ad(:adf_adele_get_real), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), t, a)
    return t
end

@testset "powers, hulls and division through ccall (milestone 2, slice 3)" begin
    # the idele of -6/35 at 64 bits, and its square: content 36/1225, unit [1] exactly, real ball around 36/1225
    q = new_rat(-6, 35)
    x = new_value(:idele)
    @test ccall(ad(:adf_idele_set_rat), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), x, q, 64) == OK
    x2 = new_value(:idele)
    @test ccall(ad(:adf_idele_pow), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), x2, x, 2, 64) == OK
    @test idele_finite(x2) == ("36/1225", ("1", "0"))
    t = new_arb()
    ccall(ad(:adf_idele_get_real), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), t, x2)
    @test arb_has(t, 36, 1225) && arb_pos(t)
    println("(-6/35)^2 = (", arb_str(t), " ; content ", idele_finite(x2)[1], ", unit [", idele_finite(x2)[2][1],
            " mod ", idele_finite(x2)[2][2], "])")
    # x^-3: content -(35/6)^3 has the sign on the unit: content 42875/216, unit [-1]
    x3 = new_value(:idele)
    @test ccall(ad(:adf_idele_pow_tight), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), x3, x, -3, 64) == OK
    @test idele_finite(x3) == ("42875/216", ("-1", "0"))

    # the map to an adele: the unit is exact, so the finite part is the exact rational 36/1225 (SPEC 5)
    a = new_value(:adele)
    ccall(ad(:adf_adele_set_idele), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), a, x2)
    @test adele_fin_text(a) == "(* ; 36/1225)"
    ar = adele_real(a)
    @test arb_has(ar, 36, 1225)
    println("as an adele: finite part ", adele_fin_text(a), ", real part ", arb_str(ar))

    # the adele of 1/2 divided by the square: (1/2) / (36/1225) = 1225/72, exact in the finite part
    half = new_rat(1, 2)
    h = new_value(:adele)
    ccall(ad(:adf_adele_set_rat), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), h, half, 64)
    w = new_value(:adele)
    @test ccall(ad(:adf_adele_div_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), w, h, x2, 64) == OK
    @test adele_fin_text(w) == "(* ; 1225/72)"
    wr = adele_real(w)
    @test arb_has(wr, 1225, 72)
    println("(1/2) / (-6/35)^2 = finite ", adele_fin_text(w), ", real ", arb_str(wr))

    # back from the adele to an idele: the same content and unit (the round trip of an exact unit)
    y = new_value(:idele)
    @test ccall(ad(:adf_idele_set_adele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), y, a) == OK
    @test idele_finite(y) == ("36/1225", ("1", "0"))

    # a unit coset: [2 mod 5]^2 is [4 mod 5] by default and [49 mod 120] tight; [5 mod 6]^1 is [2 mod 3] (M_1 = 3)
    u = new_uc(2, 5)
    v = new_value(:ucoset)
    ccall(ad(:adf_ucoset_pow), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), v, u, 2)
    @test uc_pair(v) == ("4", "5")
    ccall(ad(:adf_ucoset_pow_tight), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), v, u, 2)
    @test uc_pair(v) == ("49", "120")
    u6 = new_uc(5, 6)
    ccall(ad(:adf_ucoset_pow_tight), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), v, u6, 1)
    @test uc_pair(v) == ("2", "3")
    one = new_uc(1, 1)
    ccall(ad(:adf_ucoset_pow_tight), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), v, one, 2)
    @test uc_pair(v) == ("1", "24")                       # every square of a unit lies in U(24)
    println("U(1)^2: tight [", join(uc_pair(v), " mod "), "]; [2 mod 5]^2: default [4 mod 5], tight [49 mod 120]")

    # the hulls of (3 ; 1 [2 mod 3]): smallest 5 + 6 Zhat, simple 2 + 3 Zhat (ideles.md P16)
    r1 = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), r1)
    ccall(fl(:fmpq_one), Cvoid, (Ptr{Cvoid},), r1)
    three = new_arb()
    ccall(fl(:arb_set_si), Cvoid, (Ptr{Cvoid}, Clong), three, 3)
    u3 = new_uc(2, 3)
    z = new_value(:idele)
    @test ccall(ad(:adf_idele_set_parts), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}),
                z, three, r1, u3) == OK
    ccall(ad(:adf_adele_set_idele), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), a, z)
    @test adele_fin_text(a) == "(* ; 5 mod 6)"
    ccall(ad(:adf_adele_set_idele_simple), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), a, z)
    @test adele_fin_text(a) == "(* ; 2 mod 3)"
    # the simple hull cannot come back: its finite part has a radius
    @test ccall(ad(:adf_idele_set_adele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), y, a) == UNIT_NOT_CERTIFIED
    @test idele_finite(y) == ("36/1225", ("1", "0"))      # untouched
    # the adele 0: NOT_UNIT
    zero = new_value(:adele)
    @test ccall(ad(:adf_idele_set_adele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), y, zero) == NOT_UNIT
    # division by a coset: (1 ; 1 + 4 Zhat) / (3 ; [1 mod 1]) has the finite part 1 + 2 Zhat (ideles.md P19)
    @test ccall(ad(:adf_idele_set_parts), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}),
                z, three, r1, one) == OK
    b = Vector{UInt8}("(1 ; 1 mod 4)")
    @test ccall(ad(:adf_adele_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                a, b, Csize_t(length(b)), 64, C_NULL) == OK
    @test ccall(ad(:adf_adele_div_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), w, a, z, 64) == OK
    @test adele_fin_text(w) == "(* ; 1 mod 2)"
    println("(1 ; 1 mod 4) / (3 ; [1 mod 1]) = finite ", adele_fin_text(w))

    # the limits: prec above 2^21, and the content (-6/35)^(2^40) of more than 2^26 bits; outputs untouched
    @test ccall(ad(:adf_adele_div_idele), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), w, a, z,
                typemax(Clong)) == LIMIT
    @test adele_fin_text(w) == "(* ; 1 mod 2)"
    @test ccall(ad(:adf_idele_pow), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong),
                x2, x, Clong(1) << 40, 64) == LIMIT
    @test idele_finite(x2) == ("36/1225", ("1", "0"))

    foreach(free_arb, (t, ar, wr, three))
    foreach(v_ -> free_value(:idele, v_), (x, x2, x3, y, z))
    foreach(v_ -> free_value(:adele, v_), (a, h, w, zero))
    foreach(v_ -> free_value(:ucoset, v_), (u, v, u6, one, u3))
    free_rat(q); free_rat(half)
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), r1); Libc.free(r1)
end
