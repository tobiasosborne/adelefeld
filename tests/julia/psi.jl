# Slice 3.2-a. psi(0 ; 1/3)=E(1/3) has positive imaginary part; CV-59 strict
# rejects fractional finite radius. All storage uses exported size/alignment queries.
# An acb is the first member of cadele storage (adele.h); that carrier supplies ample space.
using Test, Libdl
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
@testset "Tate psi" begin
    xb, x = storage("adele")
    zb, z = storage("cadele")
    wb, w = storage("cadele")
    cb, c = storage("cadele")
    rb, r = storage("rat")
    text = "(0 ; 1/3)"
    expected = "((-0.5) + (0.86602540378443864676372317075293618347140262690519031402790349 +/- 1e-62)*i ; 0)"
    fractional = "(0 ; 0 mod 1/2)"
    GC.@preserve xb zb wb cb rb text expected fractional begin
        ccall(ad(:adf_adele_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), z)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), w)
        ccall(ad(:adf_cadele_init), Cvoid, (Ptr{Cvoid},), c)
        ccall(ad(:adf_rat_init), Cvoid, (Ptr{Cvoid},), r)
        try
            @test ccall(ad(:adf_adele_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), x, text, sizeof(text), 128, C_NULL) == 0
            status = ccall(ad(:adf_adele_psi_tate), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, 128)
            @test status == 0
            @test ccall(ad(:adf_cadele_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                c, expected, sizeof(expected), 256, C_NULL) == 0
            ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), w, c)
            @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, w) == 1
            @test ccall(ad(:adf_adele_psi_tate_phase), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), r, x) == 0
            @test ccall(ad(:adf_phase_get_acb), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), w, r, 128) == 0
            @test ccall(fl(:acb_overlaps), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, w) == 1
            @test ccall(ad(:adf_adele_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                x, fractional, sizeof(fractional), 128, C_NULL) == 0
            ccall(fl(:acb_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), w, z)
            @test ccall(ad(:adf_adele_psi_tate_strict), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, 128) == 1
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, w) == 1
            @test ccall(ad(:adf_adele_psi_tate), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, 128) == 0
        finally
            ccall(ad(:adf_adele_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), z)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), w)
            ccall(ad(:adf_cadele_clear), Cvoid, (Ptr{Cvoid},), c)
            ccall(ad(:adf_rat_clear), Cvoid, (Ptr{Cvoid},), r)
        end
    end
end
