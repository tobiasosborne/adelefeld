#!/bin/sh
# One mutant of src/tate.c (lane t5-slice4), run from the root of the mutant copy: src/tate.c compiled with SAN and
# INV, linked before the prebuilt SAN+INV archive of the rest of the library (W, built by
# make SAN=1 INV=1 BUILD=lanes/t5-slice4/build-mut), then tests/test_tate_continue.c run under LeakSanitizer.
W=/home/user/adelefeld-wt/t5-slice4/lanes/t5-slice4/build-mut
F="-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS -fsanitize=address,undefined"
F="$F -fno-omit-frame-pointer"
cc -Iinclude -Isrc $F -c src/tate.c -o tate_m.o || exit 1
cc -Iinclude -Itests $F tests/test_tate_continue.c $W/support/golden.o $W/support/jsonl.o tate_m.o \
    $W/libadelefeld.a -lflint -lgmp -lm -pthread -o t_m || exit 1
ASAN_OPTIONS=detect_leaks=1 timeout 240 ./t_m > /dev/null 2>&1
