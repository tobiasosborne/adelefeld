#!/bin/sh
# Build one reproducer of this directory against build/libadelefeld.a.
#   docs/reviews/m1/arith/checks/build.sh <name> [san]
# Run from the root of the worktree after `make -j2`. With a second argument `san` the program is
# built with the address and undefined-behaviour sanitizers (the library as built by make, so the
# library itself is instrumented only if it was built with `make SAN=1`).
set -e
name=$1
dir=docs/reviews/m1/arith/checks
flags="-std=c11 -O1 -g -Wall -Wextra -Iinclude"
out=$dir/$name
if [ "$2" = san ]; then
    flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"
    out=$dir/$name.san
fi
cc $flags $dir/$name.c build/libadelefeld.a -lflint -lgmp -lm -o $out
echo "built $out"
