# tests/julia/ideles.jl: Julia ccalls of the first slice of milestone 2 (lane i-slice1; docs/api-2.md
# section 1; include/adelefeld/ucoset.h, include/adelefeld/idele.h): a user makes ideles from rationals,
# multiplies and inverts them, and reads the content, the unit coset and the real ball of the result.
#
# Usage: julia --startup-file=no tests/julia/ideles.jl build/libadelefeld.so
# (run by tests/test_julia.sh, with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp
# lacks a symbol; see the header of that script).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's own
# adf_sizeof_* functions; FLINT's fmpz, fmpq and arb are made and read through libflint. Statuses
# (include/adelefeld/status.h): OK 0, NOT_DETERMINED 1, NOT_UNIT 6.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia ideles.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, NOT_UNIT = 0, 1, 6

# ---- raw values ----

alloc(sizefn) = Libc.malloc(ccall(ad(sizefn), Csize_t, ()))

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

function fmpq_str(p::Ptr)
    s = ccall(fl(:fmpq_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# an adf_rat n/d
function rat(n::Integer, d::Integer)
    q = alloc(:adf_sizeof_rat)
    ccall(ad(:adf_rat_init), Cvoid, (Ptr{Cvoid},), q)
    fn, fd = Libc.malloc(8), Libc.malloc(8)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), fn)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), fd)
    ccall(fl(:fmpz_set_si), Cvoid, (Ptr{Cvoid}, Clong), fn, n)
    ccall(fl(:fmpz_set_si), Cvoid, (Ptr{Cvoid}, Clong), fd, d)
    @test ccall(ad(:adf_rat_set_fmpz2), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), q, fn, fd) == OK
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), fn)
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), fd)
    Libc.free(fn); Libc.free(fd)
    return q
end

function free_rat(q)
    ccall(ad(:adf_rat_clear), Cvoid, (Ptr{Cvoid},), q)
    Libc.free(q)
end

function new_idele()
    x = alloc(:adf_sizeof_idele)
    ccall(ad(:adf_idele_init), Cvoid, (Ptr{Cvoid},), x)
    return x
end

function free_idele(x)
    ccall(ad(:adf_idele_clear), Cvoid, (Ptr{Cvoid},), x)
    Libc.free(x)
end

# the idele of the rational n/d at prec bits: (status, idele)
function idele_of(n, d, prec)
    q = rat(n, d)
    x = new_idele()
    st = ccall(ad(:adf_idele_set_rat), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), x, q, prec)
    free_rat(q)
    return st, x
end

# read a result: (content "n/d", unit (c, N) as strings, real ball as a string, sign, contains the integer k?,
# contains the rational q = (n, d)?)
function read_idele(x; k = nothing, q = nothing)
    r = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), r)
    ccall(ad(:adf_idele_content), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), r, x)
    content = fmpq_str(r)
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), r)
    Libc.free(r)

    u = alloc(:adf_sizeof_ucoset)
    ccall(ad(:adf_ucoset_init), Cvoid, (Ptr{Cvoid},), u)
    ccall(ad(:adf_idele_get_unit), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), u, x)
    c, N = Libc.malloc(8), Libc.malloc(8)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), c)
    ccall(fl(:fmpz_init), Cvoid, (Ptr{Cvoid},), N)
    ccall(ad(:adf_ucoset_get_fmpz2), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), c, N, u)
    unit = (fmpz_str(c), fmpz_str(N))
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), c)
    ccall(fl(:fmpz_clear), Cvoid, (Ptr{Cvoid},), N)
    Libc.free(c); Libc.free(N)
    ccall(ad(:adf_ucoset_clear), Cvoid, (Ptr{Cvoid},), u)
    Libc.free(u)

    a = Libc.malloc(48)
    ccall(fl(:arb_init), Cvoid, (Ptr{Cvoid},), a)
    ccall(ad(:adf_idele_get_real), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), a, x)
    s = ccall(fl(:arb_get_str), Ptr{UInt8}, (Ptr{Cvoid}, Clong, Culong), a, 15, 0)
    real = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    sign = ccall(fl(:arb_is_positive), Cint, (Ptr{Cvoid},), a) != 0 ? 1 :
           ccall(fl(:arb_is_negative), Cint, (Ptr{Cvoid},), a) != 0 ? -1 : 0
    has_k = k === nothing ? nothing : ccall(fl(:arb_contains_si), Cint, (Ptr{Cvoid}, Clong), a, k) != 0
    has_q = nothing
    if q !== nothing
        f = Libc.malloc(16)
        ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), f)
        ccall(fl(:fmpq_set_si), Cvoid, (Ptr{Cvoid}, Clong, Culong), f, q[1], q[2])
        has_q = ccall(fl(:arb_contains_fmpq), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), a, f) != 0
        ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), f)
        Libc.free(f)
    end
    ccall(fl(:arb_clear), Cvoid, (Ptr{Cvoid},), a)
    Libc.free(a)
    return (content = content, unit = unit, real = real, sign = sign, has_k = has_k, has_q = has_q)
end

mul(z, x, y, prec) = ccall(ad(:adf_idele_mul), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, y, prec)
inv(y, x, prec) = ccall(ad(:adf_idele_inv), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), y, x, prec)

@testset "ideles through ccall (milestone 2, slice 1)" begin
    @test ccall(ad(:adf_sizeof_ucoset), Csize_t, ()) == 16
    @test ccall(ad(:adf_sizeof_idele), Csize_t, ()) == 80
    @test ccall(ad(:adf_alignof_idele), Csize_t, ()) == 8

    # (-3/2) * (-2/3) = 1: content 1, the exact unit [1], a positive real ball around 1
    st1, x = idele_of(-3, 2, 64)
    st2, y = idele_of(-2, 3, 64)
    @test st1 == OK && st2 == OK
    r = read_idele(x)
    @test r.content == "3/2" && r.unit == ("-1", "0") && r.sign == -1 && r.real == "-1.50000000000000"
    z = new_idele()
    @test mul(z, x, y, 64) == OK
    r = read_idele(z; k = 1)
    @test r.content == "1" && r.unit == ("1", "0") && r.sign == 1 && r.has_k
    println("(-3/2) * (-2/3): content ", r.content, ", unit ", r.unit, ", real ", r.real)

    # 6 * (-5/7) = -30/7: content 30/7, unit [-1], a negative real ball
    st1, a = idele_of(6, 1, 64)
    st2, b = idele_of(-5, 7, 64)
    @test mul(z, a, b, 64) == OK
    r = read_idele(z; q = (-30, 7))
    @test r.content == "30/7" && r.unit == ("-1", "0") && r.sign == -1 && r.has_q
    println("6 * (-5/7): content ", r.content, ", unit ", r.unit, ", real ", r.real)

    # the inverse of -5/7 is -7/5; aliasing: the output is the input
    @test inv(b, b, 64) == OK
    r = read_idele(b; q = (-7, 5))
    @test r.content == "7/5" && r.unit == ("-1", "0") && r.sign == -1 && r.has_q

    # 0 has no idele: NOT_UNIT
    st, w = idele_of(0, 1, 64)
    @test st == NOT_UNIT
    r = read_idele(w; k = 1)
    @test r.content == "1" && r.unit == ("1", "0") && r.has_k          # untouched: still the idele 1

    # SPEC 5: x = 1 +/- (1 - 2^-30) as an idele; x * x at prec 128 is certified positive, at prec 60 it is not
    inf = Libc.malloc(48)
    ccall(fl(:arb_init), Cvoid, (Ptr{Cvoid},), inf)
    ccall(fl(:arb_one), Cvoid, (Ptr{Cvoid},), inf)
    ccall(fl(:mag_set_ui_2exp_si), Cvoid, (Ptr{Cvoid}, Culong, Clong), inf + 32, (1 << 30) - 1, -30)
    one = Libc.malloc(16)
    ccall(fl(:fmpq_init), Cvoid, (Ptr{Cvoid},), one)
    ccall(fl(:fmpq_one), Cvoid, (Ptr{Cvoid},), one)
    u = alloc(:adf_sizeof_ucoset)
    ccall(ad(:adf_ucoset_init), Cvoid, (Ptr{Cvoid},), u)
    s5 = new_idele()
    @test ccall(ad(:adf_idele_set_parts), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}),
                s5, inf, one, u) == OK
    t = Libc.malloc(48)
    ccall(fl(:arb_init), Cvoid, (Ptr{Cvoid},), t)
    ccall(fl(:arb_mul), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), t, inf, inf, 128)
    @test ccall(fl(:arb_contains_zero), Cint, (Ptr{Cvoid},), t) != 0          # arb_mul loses the sign
    @test mul(z, s5, s5, 128) == OK
    @test read_idele(z).sign == 1
    @test mul(z, s5, s5, 60) == NOT_DETERMINED
    # arb_get_str prints this wide positive ball as "[+/- 4.01]", which hides the sign; the test reads the sign
    println("SPEC 5 example: x * x at prec 128 is certified positive (arb_mul contains 0); ",
            "at prec 60 NOT_DETERMINED")

    ccall(fl(:arb_clear), Cvoid, (Ptr{Cvoid},), t); Libc.free(t)
    ccall(fl(:arb_clear), Cvoid, (Ptr{Cvoid},), inf); Libc.free(inf)
    ccall(fl(:fmpq_clear), Cvoid, (Ptr{Cvoid},), one); Libc.free(one)
    ccall(ad(:adf_ucoset_clear), Cvoid, (Ptr{Cvoid},), u); Libc.free(u)
    foreach(free_idele, (x, y, z, a, b, w, s5))
end
