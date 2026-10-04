#!/bin/sh
set -eu
# The tool supplies SAN=1. Only symbol and its tests are rebuilt over a prebuilt matching archive.
timeout 60 cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -DADF_CHECK_INVARIANTS -Iinclude -Isrc -c src/symbol.c -o lanes/f-slice12/mutant.o
timeout 60 cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -DADF_CHECK_INVARIANTS -Iinclude -Itests tests/test_symbol.c tests/support/jsonl.c \
    lanes/f-slice12/mutant.o lanes/f-slice12/build-mutbase/libadelefeld.a \
    -lflint -lgmp -lm -pthread -o lanes/f-slice12/mutant
ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/f-slice12/mutant
