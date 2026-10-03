# tests/julia/text_local.jl: Julia ccalls of the value form of adf_lball and adf_sball (lane t-slice2;
# include/adelefeld/text.h, the block "adf_lball, adf_sball"): a user reads a local ball and a partial ball
# from text, prints them again, reads a complex archimedean entry, and meets the statuses of bad texts.
#
# Usage: julia --startup-file=no tests/julia/text_local.jl build/libadelefeld.so
# (run by tests/test_julia.sh, with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a
# symbol; see the header of that script).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's own
# adf_sizeof_* functions. Statuses (include/adelefeld/status.h): OK 0, NOT_DETERMINED 1, DOMAIN 7,
# UNSUPPORTED 8, PARSE 9, LIMIT 10.
#
# The expected texts (docs/conventions.md 9.4):
#   "[p=5: 3 + O(5^4)]" is canonical: 3 is the centre in [0, 5^4) and N = 4 is printed;
#   "[p=5: 1/3 + O(5^4)]" has the centre 417, because 3 * 417 = 1251 = 2 * 625 + 1, that is 417 = 1/3 in
#     5 Z_5 up to 5^4; the partial ball entries are sorted into the canonical order of places.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia text_local.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, DOMAIN, UNSUPPORTED, PARSE, LIMIT = 0, 1, 7, 8, 9, 10

alloc(sizefn) = Libc.malloc(ccall(ad(sizefn), Csize_t, ()))

# a string returned by a printer of the library: copy it and free it with adf_str_free
function take_string(p::Ptr{UInt8}, len::Csize_t)
    p == C_NULL && return nothing
    s = unsafe_string(p, len)
    ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), p)
    return s
end

# ---- local balls ----
new_lball() = (x = alloc(:adf_sizeof_lball); ccall(ad(:adf_lball_init), Cvoid, (Ptr{Cvoid},), x); x)
free_lball(x) = (ccall(ad(:adf_lball_clear), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

function parse_lball(text::AbstractString)
    x = new_lball()
    st = ccall(ad(:adf_lball_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
               x, text, sizeof(text), C_NULL)
    return st, x
end

function print_lball(x)
    len = Ref{Csize_t}(0)
    p = ccall(ad(:adf_lball_get_str), Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}), len, x)
    return take_string(p, len[])
end

# ---- partial balls ----
new_sball() = (x = alloc(:adf_sizeof_sball); ccall(ad(:adf_sball_init), Cvoid, (Ptr{Cvoid},), x); x)
free_sball(x) = (ccall(ad(:adf_sball_clear), Cvoid, (Ptr{Cvoid},), x); Libc.free(x))

function parse_sball(text::AbstractString, prec::Integer)
    x = new_sball()
    st = ccall(ad(:adf_sball_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
               x, text, sizeof(text), prec, C_NULL)
    return st, x
end

function print_sball(x, digits::Integer)
    len = Ref{Csize_t}(0)
    p = ccall(ad(:adf_sball_get_str), Ptr{UInt8}, (Ptr{Csize_t}, Ptr{Cvoid}, Clong), len, x, digits)
    return take_string(p, len[])
end

# the kind of a text (conventions 9.7): the enum is an int (adf_sizeof_text_kind is 4)
function classify(text::AbstractString)
    kind = Ref{Cint}(-1)
    st = ccall(ad(:adf_text_classify), Cint, (Ptr{Cint}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
               kind, text, sizeof(text), C_NULL)
    return st, kind[]
end

@testset "the value form of local balls and partial balls through ccall (lane t-slice2)" begin
    # a local ball from text, printed again: the text of a local ball is canonical
    st, x = parse_lball("[p=5: 3 + O(5^4)]")
    @test st == OK
    @test print_lball(x) == "[p=5: 3 + O(5^4)]"
    println("[p=5: 3 + O(5^4)] -> ", print_lball(x))

    # the canonical centre: the only element of Z[1/5] in [0, 5^4) that lies in 1/3 + 5^4 Z_5 is 417
    st, y = parse_lball("[p=5: 1/3 + O(5^4)]")
    @test st == OK && print_lball(y) == "[p=5: 417 + O(5^4)]"
    println("[p=5: 1/3 + O(5^4)] -> ", print_lball(y))

    # an exact value is the rational p^v u, and the printed text reads back to the same value
    st, z = parse_lball("[p=5: 25/3]")
    @test st == OK && print_lball(z) == "[p=5: 25/3]"
    st, z2 = parse_lball(print_lball(z))
    @test st == OK && print_lball(z2) == "[p=5: 25/3]"

    # statuses: a prime that is not prime, a base that is not the prime, a zero denominator, a prime above
    # 2^64, an exponent above the limit, and a syntax fault. The value is untouched: it stays "[p=2: 0]".
    for (text, want) in (("[p=4: 1]", DOMAIN), ("[p=5: 3 + O(7^4)]", DOMAIN), ("[p=5: 1/0]", DOMAIN),
                         ("[p=18446744073709551616: 1]", UNSUPPORTED), ("[p=5: 3 + O(5^100001)]", LIMIT),
                         ("[p=5: 3 + O(5^2)", PARSE), ("[p=5 3]", PARSE), ("[p=5: 1.5]", PARSE))
        st, v = parse_lball(text)
        @test st == want
        @test print_lball(v) == "[p=2: 0]"          # a status leaves the value as it was: the exact 0 at 2
        free_lball(v)
    end

    # a partial ball from text: the entries are sorted into the canonical order of places
    st, s = parse_sball("{p=5: 3 + O(5^4); inf: 1.5; p=2: 1 + O(2^3)}", 64)
    @test st == OK
    t = print_sball(s, 20)
    @test t == "{inf: 1.5; p=2: 1 + O(2^3); p=5: 3 + O(5^4)}"
    println("{p=5: 3 + O(5^4); inf: 1.5; p=2: 1 + O(2^3)} -> ", t)

    # the printed text is a text the reader accepts, and it prints the same text again
    st, s2 = parse_sball(t, 64)
    @test st == OK && print_sball(s2, 20) == t

    # a partial ball with no place
    st, e = parse_sball("{}", 64)
    @test st == OK && print_sball(e, 20) == "{}"

    # a complex archimedean entry: the tag ADF_ARCH_COMPLEX is 2 (include/adelefeld/sball.h)
    st, c = parse_sball("{inf: (1) + (2)*i}", 64)
    @test st == OK && print_sball(c, 20) == "{inf: (1) + (2)*i}"
    tag = ccall(ad(:adf_sball_arch), Cint, (Ptr{Cvoid},), c)
    @test tag == 2                                # ADF_ARCH_COMPLEX (include/adelefeld/sball.h)
    println("{inf: (1) + (2)*i} -> ", print_sball(c, 20))

    # a real ball with a decimal radius is rounded outwards at the working precision, and the printed text
    # is read back at a prec that admits it (conventions 9.6)
    st, r = parse_sball("{inf: 1.5 +/- 1e-9}", 64)
    @test st == OK
    t2 = print_sball(r, 20)
    @test startswith(t2, "{inf: 1.5 +/- 1.0") || startswith(t2, "{inf: 1.5 +/- 1.1")
    st, r2 = parse_sball(t2, 64)
    @test st == OK
    println("{inf: 1.5 +/- 1e-9} -> ", t2, " -> ", print_sball(r2, 20))

    # statuses of a partial ball: a prime that is not prime, two inf entries, the same prime twice, a
    # trailing ';', an item limit, and a prec above the bound
    for (text, want) in (("{p=4: 1}", DOMAIN), ("{inf: 1; inf: 2}", DOMAIN), ("{p=5: 1; p=5: 2}", DOMAIN),
                         ("{p=5: 1;}", PARSE), ("{;}", PARSE), ("{inf: 1e100001}", LIMIT))
        st, v = parse_sball(text, 64)
        @test st == want
        @test print_sball(v, 20) == "{}"       # a status leaves the value as it was: no place at all
        free_sball(v)
    end
    st, v = parse_sball("{p=5: 1}", 2097153)
    @test st == LIMIT
    free_sball(v)

    # the kind of a text
    @test classify("[p=5: 3 + O(5^4)]") == (OK, 7)
    @test classify("{p=5: 3 + O(5^4)}") == (OK, 8)
    @test classify("7/3") == (OK, 0)
    @test classify("(7/3)") == (PARSE, -1)

    foreach(free_lball, (x, y, z, z2))
    foreach(free_sball, (s, s2, e, c, r, r2))
end