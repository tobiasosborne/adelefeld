using REPL
println("Julia ", VERSION)
for ex in (:(@enum), :IOContext, :get, :show, :unsafe_string, :(GC.@preserve))
    println("\n--- ", ex)
    show(stdout, MIME"text/plain"(), eval(:(@doc $ex)))
    println()
end
