# Slice 4d (lane f4-slice2): docs/api-4.md 9 item 4, ccall((:adf_rfun_translate_rat,lib),Cint,(P,P,P,Clong),
# out,r,q,128). Storage from the exported layout queries; every string freed with adf_str_free.
using Test
using Libdl
length(ARGS) == 1 || error("usage: rfun.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
ad(name) = Libdl.dlsym(LIB, name)
const P = Ptr{Cvoid}
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end
function text(f)
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_rfun_get_str), Ptr{UInt8}, (Ref{Csize_t}, P, Clong), n, f, 20)
    s == C_NULL && return nothing
    try
        return unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (P,), s)
    end
end
@testset "rfun translate by 1/3" begin
    @test ccall(ad(:adf_sizeof_rterm), Csize_t, ()) > 0
    rb, r = storage("rfun")
    ob, out = storage("rfun")
    qb, q = storage("rat")
    gauss = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    third = "1/3"
    shifted = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(2.0943951023931954923 +/- 8.5e-21) + (0)*i, " *
              "C=(-0.34906585039886591538 +/- 4.8e-21) + (0)*i))"
    GC.@preserve rb ob qb gauss third begin
        ccall(ad(:adf_rfun_init), Cvoid, (P,), r)
        ccall(ad(:adf_rfun_init), Cvoid, (P,), out)
        ccall(ad(:adf_rat_init), Cvoid, (P,), q)
        try
            @test ccall(ad(:adf_rfun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        r, gauss, sizeof(gauss), 128, C_NULL) == 0
            @test ccall(ad(:adf_rat_set_str), Cint, (P, Ptr{UInt8}, Csize_t, P),
                        q, third, sizeof(third), C_NULL) == 0
            @test ccall(ad(:adf_rfun_translate_rat), Cint, (P, P, P, Clong), out, r, q, 128) == 0
            @test ccall(ad(:adf_rfun_is_canonical), Cint, (P,), out) == 1
            @test text(out) == shifted
            @test text(r) == gauss
            # prec above ADF_REAL_PREC_MAX: LIMIT (10), out untouched
            @test ccall(ad(:adf_rfun_translate_rat), Cint, (P, P, P, Clong), out, r, q, 2097153) == 10
            @test text(out) == shifted
        finally
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), r)
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), out)
            ccall(ad(:adf_rat_clear), Cvoid, (P,), q)
        end
    end
end
