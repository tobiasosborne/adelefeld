#!/bin/sh
# lanes/j-slice2/gate.sh: the gate command of docs/api-j.md section 9 (lines 409-410), with the lane's
# writable depot (trailing colon: Julia's bundled depots are appended, docs/api-ja.md finding 10) and
# temporary directory. Run from the repository root. PKGDIR selects the package (default julia/Adelefeld);
# the fault runs pass a scratch copy and ADELEFELD_LIB.
L=lanes/j-slice2
PKGDIR=${PKGDIR:-julia/Adelefeld}
mkdir -p "$PWD/$L/tmp"
exec timeout 180 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
    JULIA_DEPOT_PATH="$PWD/$L/depot:$HOME/.julia:" JULIA_PKG_OFFLINE=true TMPDIR="$PWD/$L/tmp" \
    julia --startup-file=no --project="$PKGDIR" -e 'using Pkg; Pkg.test()'
