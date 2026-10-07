# Slice 3.1-d: exported layout, preserved backing arrays, owned strings and values.
using Test
using Libdl
length(ARGS) == 1 || error("usage: qclass_text.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
ad(name) = Libdl.dlsym(LIB, name)
function storage()
    size = ccall(ad(:adf_sizeof_qclass), Csize_t, ())
    align = ccall(ad(:adf_alignof_qclass), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = Ptr{Cvoid}((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end
@testset "qclass union text and dump" begin
    xb, x = storage()
    yb, y = storage()
    text = "union((0.75 ; 1 mod 3), (0.25 ; -1 mod 2), (0.25 ; 1 mod 2)) + Q"
    expected = "union((0.25 ; 1 mod 2), (0.75 ; 1 mod 3)) + Q"
    dump_expected = "adf1 Q qclass pieces 2 1 1 -2 0 0 g 1 2 1 1 3 -2 0 0 g 1 3 1"
    GC.@preserve xb yb text expected dump_expected begin
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), y)
        try
            @test ccall(ad(:adf_qclass_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), x, text, sizeof(text), 128, C_NULL) == 0
            @test ccall(ad(:adf_qclass_length), Clong, (Ptr{Cvoid},), x) == 2
            n = Ref{Csize_t}(0)
            value = ccall(ad(:adf_qclass_get_str), Ptr{UInt8},
                (Ref{Csize_t}, Ptr{Cvoid}, Clong), n, x, 20)
            @test value != C_NULL
            if value != C_NULL
                try
                    @test unsafe_string(value, n[]) == expected
                    @test ccall(ad(:adf_qclass_set_str), Cint,
                        (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
                        y, value, n[], 128, C_NULL) == 0
                finally
                    ccall(ad(:adf_str_free), Cvoid, (Ptr{Cvoid},), value)
                end
            end
            dump = ccall(ad(:adf_qclass_dump_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}), n, x)
            @test dump != C_NULL
            if dump != C_NULL
                try
                    @test unsafe_string(dump, n[]) == dump_expected
                    @test ccall(ad(:adf_qclass_load_str), Cint,
                        (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}, Ptr{Cvoid}),
                        y, dump, n[], C_NULL, C_NULL) == 0
                    @test ccall(ad(:adf_qclass_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, y) == 1
                finally
                    ccall(ad(:adf_str_free), Cvoid, (Ptr{Cvoid},), dump)
                end
            end
        finally
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), y)
        end
    end
end
