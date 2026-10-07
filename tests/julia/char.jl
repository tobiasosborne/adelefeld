# Slice a, api-3c 7: allocate through exported layout queries; no guessed character offsets.
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
@testset "primitive character slice a" begin
    xb, x = storage("char")
    yb, y = storage("char")
    tb, tau = storage("cadele") # acb is the first member of cadele (adele.h).
    wb, W = storage("cadele")
    eb, expected = storage("cadele")
    text = "char(q=8, n=7, s=(9) + (3)*i)"
    canonical = "char(q=4, n=3, s=(9) + (3)*i)"
    wanted = "((0) + (2)*i ; 0)"
    len = Ref{Csize_t}(0)
    GC.@preserve xb yb tb wb eb text canonical wanted len begin
        ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), y)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), tau)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), W)
        ccall(ad(:adf_cadele_init), Cvoid, (Ptr{Cvoid},), expected)
        try
            @test ccall(ad(:adf_char_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), x, text, sizeof(text), 128, C_NULL) == 0
            @test ccall(ad(:adf_char_is_canonical), Cint, (Ptr{Cvoid},), x) == 1
            @test ccall(ad(:adf_char_get_conductor), Culong, (Ptr{Cvoid},), x) == 4
            @test ccall(ad(:adf_char_get_label), Culong, (Ptr{Cvoid},), x) == 3
            @test ccall(ad(:adf_char_get_parity), Cint, (Ptr{Cvoid},), x) == 1
            str = ccall(ad(:adf_char_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}, Clong), len, x, 20)
            try
                @test str != C_NULL
                @test unsafe_string(str, len[]) == canonical
                @test ccall(ad(:adf_char_set_str), Cint,
                    (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), y, str, len[], 128, C_NULL) == 0
                @test ccall(ad(:adf_char_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, y) == 1
            finally
                ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), str)
            end
            # The declaration and call are exactly the slice-a ABI from the design.
            @test ccall(ad(:adf_char_gauss_sum), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), tau, x, 128) == 0
            @test ccall(ad(:adf_cadele_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                expected, wanted, sizeof(wanted), 128, C_NULL) == 0
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), tau, expected) == 1
            @test ccall(ad(:adf_char_root_number), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), W, x, 128) == 0
            @test ccall(fl(:acb_is_one), Cint, (Ptr{Cvoid},), W) == 1
            @test ccall(ad(:adf_char_gauss_sum), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), tau, x, 2097153) == 10
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), tau, expected) == 1
        finally
            ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), y)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), tau)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), W)
            ccall(ad(:adf_cadele_clear), Cvoid, (Ptr{Cvoid},), expected)
        end
    end
end
