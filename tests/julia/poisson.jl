# Slice 4g (lane f4-slice6): docs/api-4.md 9 item 7,
# ccall((:adf_tensor_poisson,lib),Cint,(P,P,P,P,P,P,Clong,Clong),l,r,nl,nr,phi,f,80,144).
# Storage from the exported layout queries; an acb is the first member of cadele storage (as tests/julia/tensor.jl).
# Expected values derived by hand (docs/api-4c.md "Slice 4g"; tests/driver/poisson.cmd): theta = sum exp(-pi n^2)
# = 1.0864348112133080, at bits 80 the cutoffs 4 and 4 and the imaginary radius E(4) = 1.5546e-34.
using Test
using Libdl
length(ARGS) == 1 || error("usage: poisson.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
ad(name) = Libdl.dlsym(LIB, name)
fl(name) = Libdl.dlsym(FLINT, name)
const P = Ptr{Cvoid}
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end
# z printed as the real and imaginary parts of a complex adele with the finite part 0, digits 5
function text(c, z, zero)
    @assert ccall(ad(:adf_cadele_set_acb_fball), Cint, (P, P, P), c, z, zero) == 0
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_cadele_get_str), Ptr{UInt8}, (Ref{Csize_t}, P, Clong), n, c, 5)
    try
        return unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (P,), s)
    end
end
@testset "tensor poisson" begin
    rb, r = storage("rfun")
    wb, w = storage("rfun")
    fb, f = storage("ffun")
    lb, l = storage("cadele")
    qb, q = storage("cadele")
    cb, c = storage("cadele")
    bb, zero = storage("fball")
    cuts = Culong[777, 778]
    gauss = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    witness = "rfun(term(P=[(1.5 +/- 0.5) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    one = "ffun(D=1, M=1; (1) + (0)*i)"
    GC.@preserve rb wb fb lb qb cb bb cuts gauss witness one begin
        nl = P(pointer(cuts, 1))
        nr = P(pointer(cuts, 2))
        ccall(ad(:adf_rfun_init), Cvoid, (P,), r)
        ccall(ad(:adf_rfun_init), Cvoid, (P,), w)
        ccall(ad(:adf_ffun_init), Cvoid, (P,), f)
        ccall(fl(:acb_init), Cvoid, (P,), l)
        ccall(fl(:acb_init), Cvoid, (P,), q)
        ccall(ad(:adf_cadele_init), Cvoid, (P,), c)
        ccall(ad(:adf_fball_init), Cvoid, (P,), zero)
        try
            @test ccall(ad(:adf_rfun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        r, gauss, sizeof(gauss), 128, C_NULL) == 0
            @test ccall(ad(:adf_rfun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        w, witness, sizeof(witness), 128, C_NULL) == 0
            @test ccall(ad(:adf_ffun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        f, one, sizeof(one), 128, C_NULL) == 0
            # the witness c in [1, 2]: NOT_DETERMINED (1), the cutoffs untouched
            @test ccall(ad(:adf_tensor_poisson), Cint, (P, P, P, P, P, P, Clong, Clong),
                        l, q, nl, nr, w, f, 80, 144) == 1
            @test cuts == Culong[777, 778]
            @test ccall(fl(:acb_is_zero), Cint, (P,), l) == 1
            # bits -1: DOMAIN (7); prec above ADF_REAL_PREC_MAX: LIMIT (10)
            @test ccall(ad(:adf_tensor_poisson), Cint, (P, P, P, P, P, P, Clong, Clong),
                        l, q, nl, nr, r, f, -1, 144) == 7
            @test ccall(ad(:adf_tensor_poisson), Cint, (P, P, P, P, P, P, Clong, Clong),
                        l, q, nl, nr, r, f, 80, 2097153) == 10
            @test cuts == Culong[777, 778]
            # the design's call: theta at bits 80, prec 144
            @test ccall(ad(:adf_tensor_poisson), Cint, (P, P, P, P, P, P, Clong, Clong),
                        l, q, nl, nr, r, f, 80, 144) == 0
            @test cuts == Culong[4, 4]
            @test text(c, l, zero) == "((1.0864 +/- 3.5e-5) + (0 +/- 1.6e-34)*i ; 0)"
            @test text(c, q, zero) == "((1.0864 +/- 3.5e-5) + (0 +/- 1.6e-34)*i ; 0)"
        finally
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), r)
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), w)
            ccall(ad(:adf_ffun_clear), Cvoid, (P,), f)
            ccall(fl(:acb_clear), Cvoid, (P,), l)
            ccall(fl(:acb_clear), Cvoid, (P,), q)
            ccall(ad(:adf_cadele_clear), Cvoid, (P,), c)
            ccall(ad(:adf_fball_clear), Cvoid, (P,), zero)
        end
    end
end
