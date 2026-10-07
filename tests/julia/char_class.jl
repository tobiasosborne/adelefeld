# Slice c, api-3c 7. Allocate by exported layout; retain owners across every ccall.
using Test, Libdl
const LIB = Libdl.dlopen(abspath(ARGS[1]))
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
@testset "character class slice c" begin
    cb, chi = storage("char")
    xb, x = storage("idele")
    ib, ic = storage("idclass")
    zb, z = storage("cadele") # acb is the first member; initialize only this member.
    pb, point = storage("cadele")
    character = "char(q=3, n=2, s=(1) + (0)*i)"
    idele = "(-2 ; 1 * [1])"
    ambiguous = "<2 ; [1 mod 1]>"
    GC.@preserve cb xb ib zb pb character idele ambiguous begin
        ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), chi)
        ccall(ad(:adf_idele_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_idclass_init), Cvoid, (Ptr{Cvoid},), ic)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), z)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), point)
        try
            @test ccall(ad(:adf_char_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                chi, character, sizeof(character), 128, C_NULL) == 0
            @test ccall(ad(:adf_idele_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                x, idele, sizeof(idele), 128, C_NULL) == 0
            # Design section 7's ABI. By hand: t=2, u'=-1, chi_3(2)(-1)=-1, value=-2.
            @test ccall(ad(:adf_char_eval_idele), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, x, 128) == 0
            ccall(fl(:acb_set_si), Cvoid, (Ptr{Cvoid}, Clong), point, -2)
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
            @test ccall(ad(:adf_char_eval_idele_strict), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, x, 128) == 0
            @test ccall(ad(:adf_idclass_set_idele), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), ic, x, 128) == 0
            @test ccall(ad(:adf_char_eval_idclass), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, ic, 128) == 0
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
            @test ccall(ad(:adf_char_eval_idclass_strict), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, ic, 128) == 0
            @test ccall(ad(:adf_idclass_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                ic, ambiguous, sizeof(ambiguous), 128, C_NULL) == 0
            @test ccall(ad(:adf_char_eval_idclass_strict), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, ic, 128) == 1
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
            @test ccall(ad(:adf_char_eval_idclass), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, ic, 128) == 0
            for integer in (-2, 2)
                ccall(fl(:acb_set_si), Cvoid, (Ptr{Cvoid}, Clong), point, integer)
                @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
            end
            ccall(fl(:acb_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), point, z)
            @test ccall(ad(:adf_char_eval_idele), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, chi, x, 2097153) == 10
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
        finally
            ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), chi)
            ccall(ad(:adf_idele_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_idclass_clear), Cvoid, (Ptr{Cvoid},), ic)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), z)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), point)
        end
    end
end
