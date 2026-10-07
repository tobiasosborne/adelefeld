#!/bin/sh
# Build mutation-base once with SAN=1 INV=1 before starting the sweep.
# test_qclass_sets includes src/qclass_sets.c, so each mutant compiles that source here.
# The archive supplies unchanged dependencies; its qclass_sets.o is not pulled by the linker.
set -eu
qsets_base=lanes/q-slice6/mutation-base
qsets_binary="$qsets_base/test_mutant"
timeout 30 cc -Iinclude -Itests -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
    -DADF_CHECK_INVARIANTS -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/test_qclass_sets.c "$qsets_base/support/jsonl.o" "$qsets_base/support/golden.o" \
    "$qsets_base/libadelefeld.a" -lflint -lgmp -lm -pthread -o "$qsets_binary"
ASAN_OPTIONS=detect_leaks=0 timeout 15 "$qsets_binary"
