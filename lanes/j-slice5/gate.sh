#!/bin/sh
# J-b.1 gate; api-ja.md finding 10 requires the trailing depot colon. Run from the repository root.
set -eu
L=lanes/j-slice5
PKGDIR=${PKGDIR:-julia/Adelefeld}
mkdir -p "$PWD/$L/tmp"
exec timeout 600 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project="$PKGDIR" -e 'using Pkg; Pkg.test()'
