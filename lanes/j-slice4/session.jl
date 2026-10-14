using Adelefeld
# SPEC 9.2's exact progression gives one k=0 on [100,200], and none on [0,50].
println(reconstruct(parse(FBall, "146 mod 1000"), Rat(100), Rat(200)))
println("SYMBOLS=", length(Adelefeld.REQUIRED), " LAYOUT_ROWS=", length(Adelefeld.LAYOUT_ROWS))
