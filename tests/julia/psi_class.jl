# Slices 3.2-b and 3.2-c (lane q-slice5): the class character and the local character through ccall,
# with the signatures of docs/api-3.md 7. Storage by exported size/alignment, GC.@preserve, cleanup in finally.
# E3 = (0 ; 0 mod 1/2): the lift fails strict (D3-3); its two-piece reduction passes; both enclose +1 and -1.
# psi_2(1/6) = E(fp_2(1/6)) = E(1/2) = -1 exactly (conventions 6.1:844; fault 3 of api-3.md 5).
using Test, Libdl
length(ARGS) == 1 || error("usage: psi_class.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
ad(n) = Libdl.dlsym(LIB, n)
fl(n) = Libdl.dlsym(FLINT, n)
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    ptr = Ptr{Cvoid}((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    bytes, ptr
end
@testset "Tate psi on classes and local balls" begin
    xb, x = storage("adele")
    qb, q = storage("qclass")
    yb, y = storage("qclass")
    lb, l = storage("lball")
    zb, z = storage("cadele")      # an acb is the first member of a cadele: ample aligned space
    wb, w = storage("cadele")
    rb, r = storage("rat")
    e3, local16, minus1 = "(0 ; 0 mod 1/2)", "[p=2: 1/6]", "-1"
    GC.@preserve xb qb yb lb zb wb rb e3 local16 minus1 begin
        ccall(ad(:adf_adele_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), q)
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), y)
        ccall(ad(:adf_lball_init), Cvoid, (Ptr{Cvoid},), l)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), z)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), w)
        ccall(ad(:adf_rat_init), Cvoid, (Ptr{Cvoid},), r)
        try
            @test ccall(ad(:adf_adele_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                        x, e3, sizeof(e3), 128, C_NULL) == 0
            ccall(ad(:adf_qclass_set_adele), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), q, x)
            @test ccall(ad(:adf_qclass_reduce), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, 2, 53) == 0
            @test ccall(ad(:adf_qclass_length), Clong, (Ptr{Cvoid},), y) == 2
            for (cls, strict_status) in ((q, 1), (y, 0))
                @test ccall(ad(:adf_qclass_psi_tate), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, cls, 128) == 0
                ccall(fl(:acb_set_si), Cvoid, (Ptr{Cvoid}, Clong), w, 1)
                @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, w) == 1
                ccall(fl(:acb_set_si), Cvoid, (Ptr{Cvoid}, Clong), w, -1)
                @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, w) == 1
                @test ccall(ad(:adf_qclass_psi_tate_strict), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong),
                            w, cls, 128) == strict_status
                @test ccall(ad(:adf_qclass_psi_tate_phase), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), r, cls) == 1
            end
            @test ccall(ad(:adf_lball_set_str), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}),
                        l, local16, sizeof(local16), C_NULL) == 0
            @test ccall(ad(:adf_lball_psi_tate), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, l, 128) == 0
            ccall(fl(:acb_set_si), Cvoid, (Ptr{Cvoid}, Clong), w, -1)
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, w) == 1
            @test ccall(ad(:adf_lball_psi_tate_phase), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), r, l) == 0
            len = Ref{Csize_t}(0)
            s = ccall(ad(:adf_rat_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}), len, r)
            @test unsafe_string(s, len[]) == "1/2"
            ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
        finally
            ccall(ad(:adf_adele_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), q)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), y)
            ccall(ad(:adf_lball_clear), Cvoid, (Ptr{Cvoid},), l)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), z)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), w)
            ccall(ad(:adf_rat_clear), Cvoid, (Ptr{Cvoid},), r)
        end
    end
end
