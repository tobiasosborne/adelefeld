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

# ---- slice 2 of S.1 (lane s1-slice2): adf_linsolve_fball and adf_linsol_get_fball ----
# A finite ball is raw memory sized by adf_sizeof_fball, built by adf_fball_set_fmpz3(x, A, H, d): the set
# (A + H Zhat)/d. The array of balls that adf_linsolve_fball takes holds the structs themselves, so each
# ball is copied byte by byte into it (valid while the originals are alive: small values only here).
# Statuses: UNSUPPORTED 8 (an exact ball, H = 0).

const UNSUPPORTED = 8

fmpz_str(r::Ref{Clong}) = begin
    s = ccall(fl(:fmpz_get_str), Ptr{UInt8}, (Ptr{UInt8}, Cint, Ptr{Clong}), C_NULL, 10, r)
    v = parse(BigInt, unsafe_string(s))
    ccall(fl(:flint_free), Cvoid, (Ptr{Cvoid},), s)
    v
end

function fball_new(a, H, d)
    p = Libc.malloc(ccall(ad(:adf_sizeof_fball), Csize_t, ()))
    ccall(ad(:adf_fball_init), Cvoid, (Ptr{Cvoid},), p)
    fa, fH, fd = Fmpz(string(a)), Fmpz(string(H)), Fmpz(string(d))
    st = ccall(ad(:adf_fball_set_fmpz3), Cint, (Ptr{Cvoid}, Ptr{Clong}, Ptr{Clong}, Ptr{Clong}), p, fa.v, fH.v, fd.v)
    st == OK || error("adf_fball_set_fmpz3 failed: $st")
    return p
end

fball_free(p) = (ccall(ad(:adf_fball_clear), Cvoid, (Ptr{Cvoid},), p); Libc.free(p))

# solve A x in the balls (a_i + H_i Zhat)/d_i; returns (status, verified or -1, sol pointer, G, x0)
function solve_fball(A::AbstractMatrix, balls)
    r = size(A, 1)
    size_fb = Int(ccall(ad(:adf_sizeof_fball), Csize_t, ()))
    arr = Libc.malloc(max(r, 1) * size_fb)
    ps = [fball_new(a, H, d) for (a, H, d) in balls]
    for (i, p) in enumerate(ps)
        Base.unsafe_copyto!(Ptr{UInt8}(arr) + (i - 1) * size_fb, Ptr{UInt8}(p), size_fb)
    end
    sol = Libc.malloc(ccall(ad(:adf_sizeof_linsol), Csize_t, ()))
    ccall(ad(:adf_linsol_init), Cvoid, (Ptr{Cvoid},), sol)
    pA = mat_new(A)
    st = ccall(ad(:adf_linsolve_fball), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), sol, pA, arr, r)
    ver = ccall(ad(:adf_linsol_verify_fball), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), sol, pA, arr, r)
    out = mat_new(zeros(Int, 0, 0))
    ccall(ad(:adf_linsol_get_kernel), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), out, sol)
    G = mat_get(out)
    x0 = ccall(ad(:adf_linsol_get_particular), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), out, sol) == 1 ? vec(mat_get(out)) : nothing
    mat_free(out)
    mat_free(pA)
    foreach(fball_free, ps)
    Libc.free(arr)
    return st, ver, sol, G, x0
end

function free_sol(sol)
    ccall(ad(:adf_linsol_clear), Cvoid, (Ptr{Cvoid},), sol)
    Libc.free(sol)
end

# the ball of coordinate j (0-based) of a solution: (status, A, H, d)
function coord_ball(sol, j)
    x = Libc.malloc(ccall(ad(:adf_sizeof_fball), Csize_t, ()))
    ccall(ad(:adf_fball_init), Cvoid, (Ptr{Cvoid},), x)
    st = ccall(ad(:adf_linsol_get_fball), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), x, sol, j)
    a, H, d = Fmpz("0"), Fmpz("0"), Fmpz("1")
    st == OK && ccall(ad(:adf_fball_get_fmpz3), Cvoid, (Ptr{Clong}, Ptr{Clong}, Ptr{Clong}, Ptr{Cvoid}), a.v, H.v, d.v, x)
    ccall(ad(:adf_fball_clear), Cvoid, (Ptr{Cvoid},), x)
    Libc.free(x)
    return st, fmpz_str(a.v), fmpz_str(H.v), fmpz_str(d.v)
end

@testset "adf_linsolve_fball and adf_linsol_get_fball through ccall" begin
    # x1 + x2 in (3 + 4 Zhat) and x1 - x2 in (1 + 2 Zhat): N = lcm(4, 2) = 4
    # (A' = [1 1; 2 -2], b' = [3, 2] by solvers P2.9); the set of solutions modulo 4 is found by enumeration
    st, ver, sol, G, x0 = solve_fball([1 1; 1 -1], [(3, 4, 1), (1, 2, 1)])
    @test st == OK && ver == 1
    N = 4
    member(x) = mod(x[1] + x[2] - 3, 4) == 0 && mod(x[1] - x[2] - 1, 2) == 0
    ref = Set((u, v) for u in 0:N-1, v in 0:N-1 if member((u, v)))
    @test !isempty(ref)
    # x0 + S(G): all combinations of the rows of G with coefficients in Z/4
    span = Set([Tuple(mod.(x0, N))])
    for i in 1:size(G, 1)
        span = Set(Tuple(mod.(collect(p) .+ t .* G[i, :], N)) for p in span, t in 0:N-1)
    end
    @test span == ref
    # the ball of each coordinate: its residues modulo 4 are the j-th coordinates of the solutions modulo 4
    for j in 1:2
        stj, a, H, d = coord_ball(sol, j - 1)
        @test stj == OK && d == 1 && H > 0 && 4 % H == 0
        @test Set(t for t in 0:N-1 if mod(t - a, H) == 0) == Set(p[j] for p in ref)
    end
    @test coord_ball(sol, 2)[1] == DOMAIN
    @test coord_ball(sol, -1)[1] == DOMAIN
    free_sol(sol)
    # an exact ball (H = 0): UNSUPPORTED (8)
    st, ver, sol, G, x0 = solve_fball([1 1], [(3, 0, 1)])
    @test st == UNSUPPORTED && ver == 0
    free_sol(sol)
    # a ball with a denominator: x1 + 2 x2 in (2 + 6 Zhat)/2 = 1 + 3 Zhat, canonical; N = 3
    st, ver, sol, G, x0 = solve_fball([1 2], [(2, 6, 2)])
    @test st == OK && ver == 1
    @test mod(x0[1] + 2 * x0[2] - 1, 3) == 0
    free_sol(sol)
    # no solution: x in 1 + 2 Zhat and x in 0 + 2 Zhat
    st, ver, sol, G, x0 = solve_fball(reshape([1, 1], 2, 1), [(1, 2, 1), (0, 2, 1)])
    @test st == NO_SOLUTION && ver == 1 && x0 === nothing
    @test coord_ball(sol, 0)[1] == NO_SOLUTION
    free_sol(sol)
end
