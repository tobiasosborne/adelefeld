#!/bin/sh
set -eu
L=lanes/j-slice4
mkdir -p "$PWD/$L/tmp"
exec timeout 120 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia --startup-file=no --project="${PKGDIR:-julia/Adelefeld}" "$@"
