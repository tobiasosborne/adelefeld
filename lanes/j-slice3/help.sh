#!/bin/sh
L=lanes/j-slice3
mkdir -p "$PWD/$L/tmp"
exec timeout 30 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia --startup-file=no --project=julia/Adelefeld -e 'using REPL, Adelefeld; println("Julia ", VERSION);
for ex in (:(GC.@preserve), :finalizer, :setprecision, :BigFloat, :parse, :(^))
    println("\n--- ", ex); show(stdout, MIME"text/plain"(), eval(:(@doc $ex))); println()
end'
