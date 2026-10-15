# ux/notebook/src/make_pages.jl: regenerate the pages and records of the two scenarios (run by run.sh).
push!(LOAD_PATH, @__DIR__)
using Adelefeld
using Adelica

for name in ("a_roots", "b_which_rational")
    src = joinpath(Adelica.REPO, "ux", "notebook", "scenarios", name * ".jl")
    out = joinpath(Adelica.REPO, "ux", "notebook", "out", name)
    t = @elapsed outs = Adelica.run_file(src, out)
    println(rpad(name, 18), length(outs), " cells, ", round(t; digits = 2), " s: ux/notebook/out/", name, ".html")
end
