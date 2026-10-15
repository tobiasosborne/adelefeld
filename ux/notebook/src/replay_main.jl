# ux/notebook/src/replay_main.jl: replay a records file through the driver (run by replay.sh).
# Exit status 0 when every record with a driver line is the same, 1 on a difference.
push!(LOAD_PATH, @__DIR__)
using Adelica

length(ARGS) == 1 || (println(stderr, "usage: replay.sh <records.jsonl>"); exit(2))
n = Adelica.replay(ARGS[1], Adelica.driver_path())
println(n == 0 ? "replay: every record is the same" : "replay: $(n) records differ")
exit(n == 0 ? 0 : 1)
