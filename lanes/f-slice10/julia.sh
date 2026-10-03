#!/bin/sh
# lanes/f-slice10/julia.sh [red]: build the shared object of src/ into lanes/f-slice10/build and run
# tests/julia/gfunc.jl against it ("red": without src/gfunc.c). Run from the repository root.
set -u
B=lanes/f-slice10/build
mkdir -p "$B"
if [ "${1:-}" = red ]; then
    srcs=$(ls src/*.c | grep -v gfunc.c); so=$B/libred.so
else
    srcs=$(ls src/*.c); so=$B/libgfunc.so
fi
# shellcheck disable=SC2086
cc -shared -fPIC -Iinclude -std=c11 -O1 $srcs -lflint -lgmp -lm -o "$so" || exit 1
timeout 120 julia --startup-file=no tests/julia/gfunc.jl "$PWD/$so" && exit 0
sys_gmp=$(ldconfig -p 2>/dev/null | sed -n 's/.*libgmp.so.10 .*=> \(.*\)$/\1/p' | head -n 1)
[ -n "$sys_gmp" ] && LD_PRELOAD="$sys_gmp" timeout 120 julia --startup-file=no tests/julia/gfunc.jl "$PWD/$so"
