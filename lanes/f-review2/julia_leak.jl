# Execute the actual helper definitions; keep the author's test body out of this focused leak probe.
source = read("tests/julia/sball.jl", String)
prefix = first(split(source, "@testset \"adf_sball and the real functions through ccall\" begin"))
include_string(Main, prefix, "tests/julia/sball.jl")
a = adele_of(2, 3)
st, s, _ = project(a, [ARCH])
@assert st == OK
for i in 1:20
    @assert realtext(s)[1] == OK
end
ccall(ad(:adf_adele_clear), Cvoid, (Ptr{UInt8},), a)
Libc.free(a)
finalize(s)
GC.gc()
println("realtext_calls=20 caller_owned_adele_released=1")
