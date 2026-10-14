#!/bin/sh
set -eu
R=lanes/m5-review1
export TMPDIR="$PWD/$R/compiler-tmp"
mkdir -p "$TMPDIR"
timeout 600 make -j2 BUILD="$R/build" "$R/build/libadelefeld.a"
timeout 600 make -j2 BUILD="$R/build-san" SAN=1 INV=1 "$R/build-san/libadelefeld.a"
timeout 60 cc -O2 -g -Iinclude "$R/runner.c" "$R/build/libadelefeld.a" -lflint -lgmp -lm -o "$R/runner"
timeout 60 cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DADF_CHECK_INVARIANTS -Iinclude "$R/runner.c" "$R/build-san/libadelefeld.a" \
  -lflint -lgmp -lm -o "$R/runner-san"
timeout 60 cc -O2 -g -Iinclude tools/adf/adf.c "$R/build/libadelefeld.a" -lflint -lgmp -lm -o "$R/adf"
for probe in certprobe taylorprobe; do
  timeout 60 cc -O2 -g -Iinclude -Isrc "$R/$probe.c" "$R/build/libadelefeld.a" \
    -lflint -lgmp -lm -o "$R/$probe"
done
timeout 170 make -j2 BUILD="$R/build" "$R/build/test_tate_local" "$R/build/test_tate_gamma" \
  "$R/build/test_tate" "$R/build/test_tate_continue" "$R/build/test_tate_fe"
rmdir "$TMPDIR"
