# tests/julia/linsolve.jl: a Julia ccall of adf_linsolve_mod (slice 1 of S.1, lane s1-slice1).
#
# Usage: julia --startup-file=no tests/julia/linsolve.jl build/libadelefeld.so
# (with the LD_PRELOAD of the system libgmp.so.10 if Julia's bundled libgmp lacks a symbol;
# see the header of tests/test_julia.sh).
#
# Only Libdl and Test of the standard library are used. Values are raw memory sized by the library's
# own adf_sizeof_linsol; a FLINT fmpz_mat_struct is 32 bytes (FLINT 3.0.1, fmpz_types.h). The results are
# read through the accessors adf_linsol_kind, adf_linsol_kernel_rows, adf_linsol_get_kernel,
# adf_linsol_get_particular and adf_linsol_get_dual, and checked with adf_linsol_verify.
# Statuses: OK 0, NO_SOLUTION 5, DOMAIN 7 (include/adelefeld/status.h); kinds COSET 0, EMPTY 1.

using Test
using Libdl

length(ARGS) == 1 || (println(stderr, "usage: julia linsolve.jl <path-to-libadelefeld.so>"); exit(2))

const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
fl(name) = Libdl.dlsym(FLINT, name)
ad(name) = Libdl.dlsym(LIB, name)

const OK, NO_SOLUTION, DOMAIN = 0, 5, 7
const COSET, EMPTY = 0, 1
const SIZEOF_FMPZ_MAT = 32

# a new fmpz_mat with r rows and c columns, entries from a Julia matrix of integers (any size)
function mat_new(M::AbstractMatrix)
    p = Libc.malloc(SIZEOF_FMPZ_MAT)
    r, c = size(M)
    ccall(fl(:fmpz_mat_init), Cvoid, (Ptr{Cvoid}, Clong, Clong), p, r, c)
    for i in 1:r, j in 1:c
        e = ccall(fl(:fmpz_mat_entry), Ptr{Cvoid}, (Ptr{Cvoid}, Clong, Clong), p, i - 1, j - 1)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Cvoid}, Cstring, Cint), e, string(M[i, j]), 10) == 0 || error("bad entry")
    end
    return p
end

function mat_free(p)
    ccall(fl(:fmpz_mat_clear), Cvoid, (Ptr{Cvoid},), p)
    Libc.free(p)
end

# the entries of an fmpz_mat as a Julia matrix of BigInt (r and c are the slongs at offsets 8 and 16)
function mat_get(p)
    r = unsafe_load(Ptr{Clong}(p + 8))
    c = unsafe_load(Ptr{Clong}(p + 16))
    M = zeros(BigInt, r, c)
    for i in 1:r, j in 1:c
        e = ccall(fl(:fmpz_mat_entry), Ptr{Cvoid}, (Ptr{Cvoid}, Clong, Clong), p, i - 1, j - 1)
        s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Cvoid}), C_NULL, 10, e)
        M[i, j] = parse(BigInt, unsafe_string(s))
        ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    end
    return M
end

mutable struct Fmpz
    v::Ref{Clong}
    Fmpz(s::AbstractString) = begin
        r = Ref{Clong}(0)
        ccall(fl(:fmpz_init), Cvoid, (Ptr{Clong},), r)
        ccall(fl(:fmpz_set_str), Cint, (Ptr{Clong}, Cstring, Cint), r, s, 10) == 0 || error("bad integer $s")
        new(r)
    end
end

# solve A x = b modulo N; returns (status, kind, G, x0 or nothing, y or nothing, verified)
function solve(A::AbstractMatrix, b::AbstractVector, N)
    sol = Libc.malloc(ccall(ad(:adf_sizeof_linsol), Csize_t, ()))
    ccall(ad(:adf_linsol_init), Cvoid, (Ptr{Cvoid},), sol)
    pA = mat_new(A)
    pb = mat_new(reshape(b, length(b), 1))
    fN = Fmpz(string(N))
    st = ccall(ad(:adf_linsolve_mod), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Clong}), sol, pA, pb, fN.v)
    kind = ccall(ad(:adf_linsol_kind), Cint, (Ptr{Cvoid},), sol)
    k = ccall(ad(:adf_linsol_kernel_rows), Clong, (Ptr{Cvoid},), sol)
    out = mat_new(zeros(Int, 0, 0))
    ccall(ad(:adf_linsol_get_kernel), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), out, sol)
    G = mat_get(out)
    @test size(G, 1) == k
    x0 = ccall(ad(:adf_linsol_get_particular), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), out, sol) == 1 ? vec(mat_get(out)) : nothing
    y = ccall(ad(:adf_linsol_get_dual), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), out, sol) == 1 ? vec(mat_get(out)) : nothing
    ver = ccall(ad(:adf_linsol_verify), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Clong}), sol, pA, pb, fN.v)
    mat_free(out)
    mat_free(pA)
    mat_free(pb)
    ccall(ad(:adf_linsol_clear), Cvoid, (Ptr{Cvoid},), sol)
    Libc.free(sol)
    return st, kind, G, x0, y, ver
end

@testset "adf_linsolve_mod through ccall" begin
    # 2 x + 4 y = 6 modulo 8: the kernel is x + 2 y = 0 modulo 4, Howell form [[2, 1], [0, 2]]
    # (check_s1_edge, proto/solvers_checks.py:1334); x0 is determined by Algorithm L
    st, kind, G, x0, y, ver = solve([2 4], [6], 8)
    @test st == OK && kind == COSET && ver == 1
    @test G == [2 1; 0 2]
    @test y === nothing
    @test x0 !== nothing && mod(2 * x0[1] + 4 * x0[2], 8) == 6
    # every solution modulo 8 is x0 + S(G): count them by enumeration here
    sols = Set((u, v) for u in 0:7, v in 0:7 if mod(2u + 4v, 8) == 6)
    span = Set(mod.((x0[1] + a * G[1, 1] + c * G[2, 1], x0[2] + a * G[1, 2] + c * G[2, 2]), 8) for a in 0:7, c in 0:7)
    @test span == sols
    # 2 x + 4 y = 1 modulo 8: no solution; y = 4 (y A = 0, y b = 4 modulo 8)
    st, kind, G, x0, y, ver = solve([2 4], [1], 8)
    @test st == NO_SOLUTION && kind == EMPTY && ver == 1
    @test x0 === nothing && y == [4]
    @test G == [2 1; 0 2]
    # a modulus of 200 bits with a planted solution and entries of any sign
    N = big(2)^120 * big(3)^50
    A = [big(3)^70 -(big(2)^130 + 1); 6 big(10)^40]
    xp = [big(123456789), -big(987654321)]
    b = A * xp
    st, kind, G, x0, y, ver = solve(A, b, N)
    @test st == OK && kind == COSET && ver == 1
    @test all(mod.(A * x0 - b, N) .== 0)
    # N = 0 is refused
    st, kind, G, x0, y, ver = solve([1 1], [1], 0)
    @test st == DOMAIN
end
