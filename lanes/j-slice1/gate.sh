#!/bin/sh
# lanes/j-slice1/gate.sh: the gate command of docs/api-j.md section 9, with the lane's writable depot and
# temporary directory (the brief of lane j-slice1). Run from the repository root.
L=lanes/j-slice1
exec timeout 180 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia --startup-file=no --project=julia/Adelefeld -e 'using Pkg; Pkg.test()'
