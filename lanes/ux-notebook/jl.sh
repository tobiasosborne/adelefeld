#!/bin/sh
# Start Julia the way lanes/j-slice4/run-julia.sh does, through the package launcher, for this lane.
set -eu
L=lanes/ux-notebook
mkdir -p "$PWD/$L/tmp"
exec timeout "${JL_TIMEOUT:-300}" env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld "$@"
