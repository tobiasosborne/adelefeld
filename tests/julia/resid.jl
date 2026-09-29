# tests/julia/resid.jl: a Julia ccall of adf_resid_reconstruct (slices 1 and 2 of milestone S).
#
# The driver `adf` takes at most three operands per command (tools/adf/adf.c, the line grammar and
# tools/adf/README.md), so `adf resid <c> <m> <A> <B>` would change its grammar; the brief of the
# lane says not to do that and to call the function from Julia instead.
#
# Usage: julia --startup-file=no tests/julia/resid.jl build/libadelefeld.so
# (with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a symbol;
# see the header of tests/test_julia.sh).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's
# own adf_sizeof_* functions. Statuses: OK 0, NOT_DETERMINED 1, NOT_UNIQUE 4, NO_SOLUTION 5 (include/adelefeld/status.h).

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia resid.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NOT_DETERMINED, NOT_UNIQUE, NO_SOLUTION = 0, 1, 4, 5

mutable struct Fmpz
    v::Ref{Clong}
    Fmpz(s::AbstractString) = begin
        r = Ref{Clong}(0)
        ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), r)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), r, s, 10) == 0 || error("bad integer $s")
        new(r)
    end
end

function fmpz_str(p::Ptr)
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, p)
    str = unsafe_string(s)
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    return str
end

# reconstruct c, m, A, B (decimal strings) and the search limit; returns (status, "n/d" or "")
function reconstruct(c, m, A, B, limit=0)
    fc, fm, fA, fB = Fmpz(c), Fmpz(m), Fmpz(A), Fmpz(B)
    x = Libc.malloc(ccall(ad(:adf_sizeof_resid), Csize_t, ()))
    q = Libc.malloc(ccall(ad(:adf_sizeof_rat), Csize_t, ()))
    cert = Libc.malloc(ccall(ad(:adf_sizeof_recon_cert), Csize_t, ()))
    ccall(ad(:adf_resid_init), Cvoid, (Ptr{Cvoid},), x)
    ccall(ad(:adf_rat_init), Cvoid, (Ptr{Cvoid},), q)
    ccall(ad(:adf_recon_cert_init), Cvoid, (Ptr{Cvoid},), cert)
    @test ccall(ad(:adf_resid_set_fmpz2), Cint, (Ptr{Cvoid}, Ptr{Clong}, Ptr{Clong}), x, fc.v, fm.v) == 0
    st = ccall(ad(:adf_resid_reconstruct), Cint,
               (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Clong}, Ptr{Clong}, Clong), q, cert, x, fA.v, fB.v, limit)
    out = st == OK ? fmpz_str(q) * "/" * fmpz_str(q + 8) : ""
    ccall(ad(:adf_resid_clear), Cvoid, (Ptr{Cvoid},), x)
    ccall(ad(:adf_rat_clear), Cvoid, (Ptr{Cvoid},), q)
    ccall(ad(:adf_recon_cert_clear), Cvoid, (Ptr{Cvoid},), cert)
    Libc.free(x); Libc.free(q); Libc.free(cert)
    return st, out
end

@testset "adf_resid_reconstruct through ccall" begin
    # 1/3 = 34 modulo 101 (3 * 34 = 102 = 1): a fraction
    @test reconstruct("34", "101", "1", "3") == (OK, "1/3")
    # negative numerator, c outside [0, m): -1/1 = 100 = -1 modulo 101
    @test reconstruct("-1", "101", "1", "3") == (OK, "-1/1")
    # gcd(R, T) = 2 (docs/proofs/solvers.md Remark 1 of 1.6): m = 12, c = 6, A = 1, B = 5, no solution
    @test reconstruct("6", "12", "1", "5")[1] == NO_SOLUTION
    # 2 A B >= m, the search (docs/api-s.md section 2, note 1): 1/5 = 5 modulo 6, A = 1, B = 5 has -1/1 and 1/5
    @test reconstruct("5", "6", "1", "5", 1000)[1] == NOT_UNIQUE
    @test reconstruct("5", "6", "1", "4", 1000) == (OK, "-1/1")
    # limit 0 does not decide: m = 2, c = 1, A = B = 1 (1/1 and -1/1), 2 A B = m: uniqueness not certified
    @test reconstruct("1", "2", "1", "1", 0)[1] == NOT_DETERMINED
    @test reconstruct("1", "2", "1", "1", 1)[1] == NOT_UNIQUE
    @test reconstruct("1", "2", "1", "1", -3)[1] == NOT_DETERMINED
    # A >= m: NOT_UNIQUE whatever the limit (note 5: m = 2, c = 1, A = 2, B = 1, limit 0)
    @test reconstruct("1", "2", "2", "1", 0)[1] == NOT_UNIQUE
    # empty box
    @test reconstruct("6", "12", "-1", "6")[1] == NO_SOLUTION
    # a planted fraction of 200 digits: n = -(10^100 + 7), d = 10^100 + 9 (coprime: they differ by 16 and
    # d is odd), m = 2 A B + 1 with A = 10^100 + 7, B = 10^100 + 9; c = n d^-1 mod m is computed with BigInt
    A = big(10)^100 + 7
    B = big(10)^100 + 9
    m = 2 * A * B + 1
    n = -A
    d = B
    c = mod(n * invmod(d, m), m)
    @test reconstruct(string(c), string(m), string(A), string(B)) == (OK, string(n) * "/" * string(d))
    # m - 1 = 2 A B: the search (values of recon_partial): |T| = B, X = 1; limit 0 does not decide, limit 1 does
    @test reconstruct(string(c), string(m - 1), string(A), string(B), 0)[1] == NOT_DETERMINED
    @test reconstruct(string(c), string(m - 1), string(A), string(B), 1)[1] == NO_SOLUTION
    # B / |T| above 2^64 (c = 1, T = 1, B = 10^30) and a large limit that is never run: limit 0 returns at once
    @test reconstruct("1", string(big(10)^60 + 7), string(big(10)^40), string(big(10)^30), 0)[1] == NOT_DETERMINED
end
