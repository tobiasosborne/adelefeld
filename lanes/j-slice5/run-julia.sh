#!/bin/sh
# Run a Julia script with the package, as the gate starts Julia. Run from the repository root.
set -eu
L=lanes/j-slice5
mkdir -p "$PWD/$L/tmp"
exec timeout ${T:-180} env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project="${PKGDIR:-julia/Adelefeld}" "$@"
