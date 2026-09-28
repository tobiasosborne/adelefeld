#!/bin/sh
set -eu
d=docs/reviews/m1/text/checks
make -j2 BUILD="$d/build" CFLAGS='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fPIC' all
cc -std=c11 -O2 -g -Wall -Wextra -Werror -fPIC -shared -Iinclude "$d/bridge.c" \
    -Wl,--whole-archive "$d/build/libadelefeld.a" -Wl,--no-whole-archive \
    -lflint -lgmp -lm -o "$d/bridge.so"
