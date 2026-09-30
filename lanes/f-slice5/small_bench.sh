#!/bin/sh
set -e
for p in 2 3 5 18446744073709551557; do
  for n in 8 32 64 128; do
    for f in exp log Log; do
      for stage in before after; do
        timeout 60 "lanes/f-slice5/build/bench-small-$stage" "$f" "$p" "$n" 31 \
          > "lanes/f-slice5/small-$stage-$f-$p-$n.log" 2>&1
      done
    done
  done
done
