# tests/julia/text_idele.jl: Julia ccalls of the value form of ideles (lane t-slice1, milestone 2;
# include/adelefeld/text.h, block "adf_ucoset, adf_idele, adf_idclass"): a user parses two ideles from text,
# multiplies them, and prints the product; the same for a unit coset and a class; the statuses of a bad text.
#
# Usage: julia --startup-file=no tests/julia/text_idele.jl build/libadelefeld.so
# (run by tests/test_julia.sh, with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a
# symbol; see the header of that script).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's own
# adf_sizeof_* functions. Statuses (include/adelefeld/status.h): OK 0, NOT_DETERMINED 1, DOMAIN 7, PARSE 9,
# LIMIT 10. The expected texts were computed by hand: (3/2)(-2/3) = -1, [5 mod 36][7 mod 12] = [11 mod 12] because
# 35 = 11 modulo gcd(36, 12) = 12.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia text_idele.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN, PARSE, LIMIT = 0, 1, 7, 9, 10

alloc(sizefn) = Libc.malloc(ccall(ad(sizefn), Csize_t, ()))

# a string returned by a printer of the library: copy it and free it with adf_str_free
function take_string(p::Ptr{UInt8}, len::Csize_t)
    p == C_NULL && return nothing
    s = unsafe_string(p, len)
    ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), p)
    return s
end

# ---- ideles ----
new_idele() = (x = alloc(:adf_sizeof_idele); ccall(ad(:adf_idele_init), Cvoid, (Ptr{Cvoid},), x); x)
free_idele(x) = (ccall(ad(:adf_idele_clear), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

# parse text into an idele (a status and the value); the length is passed, no NUL is needed
function parse_idele(text::AbstractString, prec::Integer)
    x = new_idele()
    st = ccall(ad(:adf_idele_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
               x, text, sizeof(text), prec, C_NULL)
    return st, x
end

function print_idele(x, digits::Integer)
    len = Ref{Csize_t}(0)
    p = ccall(ad(:adf_idele_get_str), Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}, Clong), len, x, digits)
    return take_string(p, len[])
end

mul_idele(z, x, y, prec) =
    ccall(ad(:adf_idele_mul), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, y, prec)

# ---- unit cosets ----
new_ucoset() = (x = alloc(:adf_sizeof_ucoset); ccall(ad(:adf_ucoset_init), Cvoid, (Ptr{Cvoid},), x); x)
free_ucoset(x) = (ccall(ad(:adf_ucoset_clear), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

function parse_ucoset(text::AbstractString)
    x = new_ucoset()
    st = ccall(ad(:adf_ucoset_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
               x, text, sizeof(text), C_NULL)
    return st, x
end

function print_ucoset(x)
    len = Ref{Csize_t}(0)
    p = ccall(ad(:adf_ucoset_get_str), Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len, x)
    return take_string(p, len[])
end

# ---- classes ----
new_idclass() = (x = alloc(:adf_sizeof_idclass); ccall(ad(:adf_idclass_init), Cvoid, (Ptr{Cvoid},), x); x)
free_idclass(x) = (ccall(ad(:adf_idclass_clear), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

function parse_idclass(text::AbstractString, prec::Integer)
    x = new_idclass()
    st = ccall(ad(:adf_idclass_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
               x, text, sizeof(text), prec, C_NULL)
    return st, x
end

function print_idclass(x, digits::Integer)
    len = Ref{Csize_t}(0)
    p = ccall(ad(:adf_idclass_get_str), Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}, Clong), len, x, digits)
    return take_string(p, len[])
end

# the kind of a text (conventions 9.7): the enum is an int (adf_sizeof_text_kind is 4)
function classify(text::AbstractString)
    kind = Ref{Cint}(-1)
    st = ccall(ad(:adf_text_classify), Cint, (Ptr{Cint}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
               kind, text, sizeof(text), C_NULL)
    return st, kind[]
end

@testset "the value form of ideles through ccall (milestone 2, lane t-slice1)" begin
    @test ccall(ad(:adf_sizeof_text_kind), Csize_t, ()) == 4

    # two ideles from text; the product; the printed product.  (3/2, in the real place -4 and content 3/2 with the
    # unit [5 mod 36]) times (2/3 at +9, unit [7 mod 12]): real -36, content 1, unit [11 mod 12]
    st1, x = parse_idele("(-4 ; 3/2 * [5 mod 36])", 64)
    st2, y = parse_idele("(9 ; 2/3 * [7 mod 12])", 64)
    @test st1 == OK && st2 == OK
    @test print_idele(x, 20) == "(-4 ; 3/2 * [5 mod 36])"
    z = new_idele()
    @test mul_idele(z, x, y, 64) == OK
    t = print_idele(z, 20)
    @test t == "(-36 ; 1 * [11 mod 12])"
    println("(-4 ; 3/2 * [5 mod 36]) * (9 ; 2/3 * [7 mod 12]) = ", t)

    # the printed text of a product is a text the reader accepts: the value is read back and printed again
    st3, w = parse_idele(t, 64)
    @test st3 == OK && print_idele(w, 20) == t

    # canonicalisation on input: the content reduced, the unit residue reduced, the modulus printed in normal form
    st, c = parse_idele("(2 +/- 0.125 ; 6/4 * [41 mod 6])", 64)
    @test st == OK && print_idele(c, 20) == "(2 +/- 0.13 ; 3/2 * [2 mod 3])"

    # statuses: a real part that contains 0, a zero content, a unit with a common factor, a bad exponent
    for (text, want) in (("(1 +/- 1 ; 1 * [1])", DOMAIN), ("(1 ; 0 * [1])", DOMAIN), ("(1 ; 1 * [2 mod 4])", DOMAIN),
                         ("(1e100001 ; 1 * [1])", LIMIT), ("(1 ; 1 [1])", PARSE), ("(1 ; 1 * [1]", PARSE))
        st, v = parse_idele(text, 64)
        @test st == want
        @test print_idele(v, 20) == "(1 ; 1 * [1])"          # a status leaves the value as it was: the idele 1
        free_idele(v)
    end
    # NOT_DETERMINED: the exact interval [1e-11, 2 - 1e-11] excludes 0 but no 30-bit ball certifies it; 64 bits do
    st, v = parse_idele("(1 +/- 0.99999999999 ; 1 * [1])", 30)
    @test st == NOT_DETERMINED
    free_idele(v)
    st, v = parse_idele("(1 +/- 0.99999999999 ; 1 * [1])", 64)
    @test st == OK
    free_idele(v)

    # the kind of a text
    @test classify("(1 ; 1 * [1])") == (OK, 5)
    @test classify("<1 ; [1]>") == (OK, 6)
    @test classify("[5 mod 36]") == (OK, 4)
    @test classify("(1 ; [1])") == (PARSE, -1)

    # a unit coset: the product of two, printed in normal form
    st1, u = parse_ucoset("[5 mod 6]")
    st2, v = parse_ucoset("[7 mod 10]")
    @test st1 == OK && st2 == OK
    @test print_ucoset(u) == "[2 mod 3]"                   # 5 mod 6 is 2 mod 3 (SPEC 5)
    @test print_ucoset(v) == "[2 mod 5]"                   # 7 mod 10 is 2 mod 5
    st, bad = parse_ucoset("[2 mod 4]")
    @test st == DOMAIN
    @test print_ucoset(bad) == "[1]"                       # untouched

    # a class: read, printed, and the unit in normal form
    st, k = parse_idclass("<0.5 +/- 0.25 ; [5 mod 6]>", 64)
    @test st == OK && print_idclass(k, 20) == "<0.5 +/- 0.25 ; [2 mod 3]>"
    st, k2 = parse_idclass("<-1 ; [1]>", 64)
    @test st == DOMAIN

    foreach(free_idele, (x, y, z, w, c))
    foreach(free_ucoset, (u, v, bad))
    foreach(free_idclass, (k, k2))
end
