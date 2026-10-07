# Slice 4c: lossless dumps, layout queries, bounded input and owned strings.
using Test, Libdl
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB, n)
const P = Ptr{Cvoid}
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    @test size > 0 && ispow2(align)
    bytes = Vector{UInt8}(undef, size + align - 1)
    ptr = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    bytes, ptr
end
@testset "slice 4c dumps" begin
    for (kind, text, expected) in (
        ("ffun", "ffun(D=1, M=1; (0.5) + (-0.75)*i)", "adf1 Q ffun 1 1 1 -1 0 0 -3 -2 0 0"),
        ("rfun", "rfun(term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))",
         "adf1 Q rfun 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"))
        fb, f = storage(kind); gb, g = storage(kind)
        init = ad(Symbol("adf_", kind, "_init"))
        clear = ad(Symbol("adf_", kind, "_clear"))
        dump = ad(Symbol("adf_", kind, "_dump_str"))
        load = ad(Symbol("adf_", kind, "_load_str"))
        identical = ad(Symbol("adf_", kind, "_identical"))
        GC.@preserve fb gb text expected begin
            ccall(init, Cvoid, (P,), f); ccall(init, Cvoid, (P,), g)
            try
                @test ccall(ad(Symbol("adf_", kind, "_set_str")), Cint,
                            (P, Ptr{UInt8}, Csize_t, Clong, P), f, text, sizeof(text), 128, C_NULL) == 0
                n = Ref{Csize_t}(0)
                s = ccall(dump, Ptr{UInt8}, (Ref{Csize_t}, P), n, f)
                @test s != C_NULL
                try
                    @test unsafe_string(s, n[]) == expected
                    @test ccall(load, Cint, (P, Ptr{UInt8}, Csize_t, P, P),
                                g, s, n[], C_NULL, C_NULL) == 0
                    @test ccall(identical, Cint, (P, P), f, g) == 1
                    @test ccall(load, Cint, (P, Ptr{UInt8}, Csize_t, P, P),
                                g, s, n[] - 1, C_NULL, C_NULL) == 9
                    @test ccall(identical, Cint, (P, P), f, g) == 1
                finally
                    ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
                end
            finally
                ccall(clear, Cvoid, (P,), g); ccall(clear, Cvoid, (P,), f)
            end
        end
    end
end
