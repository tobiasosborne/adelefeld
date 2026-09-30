#!/bin/sh
# bench.sh (lane f-review4): builds bench.c against the new archive and the old lfunc.o of build.sh's OUTDIR,
# then runs each line of the input file in its own process (so that the peak memory belongs to one call).
# Usage: sh lanes/f-review4/bench.sh OUTDIR INPUT   (OUTDIR as given to build.sh; run from the worktree root)
set -e
OUT="$1"; IN="$2"
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude -c lanes/f-review4/bench.c -o "$OUT/bench.o"
cc -o "$OUT/bench" "$OUT/bench.o" "$OUT/lfunc_old.o" lanes/f-review4/build/libadelefeld.a -lflint -lgmp -lm
n=$(wc -l < "$IN")
i=1
while [ "$i" -le "$n" ]; do
  sed -n "${i}p" "$IN" | timeout 300 "$OUT/bench" | awk '{printf "%s %s %s %s %s %s\n", $1, substr($2,1,12), $3, $4, $9, $11}'
  i=$((i + 1))
done
