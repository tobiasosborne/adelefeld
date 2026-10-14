#!/bin/sh
L=lanes/j-slice3
mkdir -p "$PWD/$L/tmp"
exec timeout 30 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia --startup-file=no --project=julia/Adelefeld -e 'using Adelefeld;
display(Adele(Rat(1, 3); prec = 64) + Adele(parse(FBall, "5 mod 18"); prec = 64));
println("symbols=", length(Adelefeld.REQUIRED)); println("rows=", length(Adelefeld.LAYOUT_ROWS))'
