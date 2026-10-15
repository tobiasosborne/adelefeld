# ux/notebook/src/replay_main.jl: replay a records file through the driver (run by replay.sh).
# The last line is "replay: N same, M differs, K unchecked". Exit status 1 when M > 0, else 0.
push!(LOAD_PATH, @__DIR__)
using Adelica

length(ARGS) == 1 || (println(stderr, "usage: replay.sh <records.jsonl>"); exit(2))
s = Adelica.replay_summary(ARGS[1], Adelica.driver_path())
println(Adelica.replay_sentence(s))
exit(s.differs == 0 ? 0 : 1)
