#!/bin/sh
# ux/notebook/julia.sh: start Julia for the notebook, from the repository root, offline, on one thread.
# The package is the project (julia/Adelefeld); Adelica is found by push!(LOAD_PATH, "ux/notebook/src")
# in the scripts. The libgmp preload is done by julia/Adelefeld/bin/adelefeld-julia.
set -eu
D=lanes/ux-notebook
mkdir -p "$PWD/$D/tmp"
exec timeout "${ADELICA_TIMEOUT:-300}" env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    JULIA_DEPOT_PATH="$PWD/$D/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$D/tmp" \
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld "$@"
