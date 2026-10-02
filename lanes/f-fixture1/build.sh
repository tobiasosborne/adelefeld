#!/bin/sh
# Same old-code extraction and symbol renaming as lanes/f-review4/build.sh.
set -eu
OUT=lanes/f-fixture1/build
mkdir -p "$OUT"
git show 1cf0e42:src/lfunc.c > "$OUT/lfunc_old.c"
CF='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Isrc'
timeout 60 cc $CF -Dadf_lball_log=old_lball_log -Dadf_lball_Log=old_lball_Log \
    -Dadf_lball_exp=old_lball_exp -c "$OUT/lfunc_old.c" -o "$OUT/lfunc_old.o"
timeout 60 cc $CF lanes/f-fixture1/stored_probe.c "$OUT/lfunc_old.o" \
    "$OUT/libadelefeld.a" -lflint -lgmp -lm -o "$OUT/stored_probe"
sha256sum "$OUT/lfunc_old.c"
