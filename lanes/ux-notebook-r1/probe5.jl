push!(LOAD_PATH, "ux/notebook/src"); using Adelefeld, Adelica
const A = Adelica
out = joinpath(mktempdir(), "b")
A.run_file("ux/notebook/scenarios/b_which_rational.jl", out)
A.replay_summary(out * ".records.jsonl", A.driver_path())
lit = "(0.99 +/- 0.0099 ; 0 mod 1)"
out2 = joinpath(mktempdir(), "d2")
A.run(["x = adf\"$(lit)\"", "y = x + Rat(0)", "project(y, :real)", "reconstruct(y)",
       "z = y * y", "w = adf\"(3.14 +/- 0.01 ; 9 mod 625)\" + Rat(0)", "roots(w, 2; at = 5)",
       "project(w, :real, 5)"], out2)
println(A.replay_sentence(A.replay_summary(out2 * ".records.jsonl", A.driver_path())))
for l in eachline(out2 * ".records.jsonl"); println(first(l, 400)); end
