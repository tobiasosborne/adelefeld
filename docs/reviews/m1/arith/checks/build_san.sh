#!/bin/sh
# Build one reproducer together with the library sources src/*.c, all under the address and
# undefined-behaviour sanitizers, into this directory (build/ is not touched).
#   docs/reviews/m1/arith/checks/build_san.sh <name>
set -e
name=$1
dir=docs/reviews/m1/arith/checks
cc -std=c11 -O1 -g -Iinclude -fsanitize=address,undefined -fno-sanitize-recover=undefined \
   -fno-omit-frame-pointer $dir/$name.c src/*.c -lflint -lgmp -lm -o $dir/$name.san
echo "built $dir/$name.san"
