# Slice b, api-3c 7. Layout queries, preserved owners, exact unit values and strict snapshots.
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
@testset "character unit coset slice b" begin
    xb, x = storage("char")
    yb, y = storage("char")
    ub, u = storage("ucoset")
    zb, z = storage("cadele") # acb is the first member (adele.h); initialize only acb here.
    pb, point = storage("cadele")
    text = "char(q=5, n=2, s=(7) + (3)*i)"
    coset = "[1 mod 1]"
    len = Ref{Csize_t}(0)
    GC.@preserve xb yb ub zb pb text coset len begin
        ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), y)
        ccall(ad(:adf_ucoset_init), Cvoid, (Ptr{Cvoid},), u)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), z)
        ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), point)
        try
            @test ccall(ad(:adf_char_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), x, text, sizeof(text), 128, C_NULL) == 0
            @test ccall(ad(:adf_ucoset_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}), u, coset, sizeof(coset), C_NULL) == 0
            # The exact ABI requested by design section 7.
            @test ccall(ad(:adf_char_eval_ucoset), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, u, 128) == 0
            for cardinal in (1, -1)
                ccall(fl(:acb_set_si), Cvoid, (Ptr{Cvoid}, Clong), point, cardinal)
                @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
                ccall(fl(:acb_mul_onei), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), point, point)
                @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
            end
            ccall(fl(:acb_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), point, z)
            @test ccall(ad(:adf_char_eval_ucoset_strict), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, u, 128) == 1
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
            ccall(ad(:adf_char_conj), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), y, x)
            @test ccall(ad(:adf_char_get_label), Culong, (Ptr{Cvoid},), y) == 3
            ccall(ad(:adf_char_conj), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), y, y)
            @test ccall(ad(:adf_char_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, y) == 1
            dump = ccall(ad(:adf_char_dump_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}), len, x)
            try
                @test dump != C_NULL
                @test ccall(ad(:adf_char_load_str), Cint,
                    (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}, Ptr{Cvoid}), y, dump, len[], C_NULL, C_NULL) == 0
                @test ccall(ad(:adf_char_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, y) == 1
            finally
                ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), dump)
            end
            @test ccall(ad(:adf_char_eval_ucoset), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, u, 2097153) == 10
            @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, point) == 1
        finally
            ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), y)
            ccall(ad(:adf_ucoset_clear), Cvoid, (Ptr{Cvoid},), u)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), z)
            ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), point)
        end
    end
end
