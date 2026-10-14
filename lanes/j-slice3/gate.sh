#!/bin/sh
# Step J-a.3 gate. See docs/api-j.md:409-410 and api-ja.md finding 10 for the trailing colon.
L=lanes/j-slice3
PKGDIR=${PKGDIR:-julia/Adelefeld}
mkdir -p "$PWD/$L/tmp"
exec timeout 180 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia --startup-file=no --project="$PKGDIR" -e 'using Pkg; Pkg.test()'
