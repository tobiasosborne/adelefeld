#!/bin/sh
set -e
timeout 10 lanes/f-slice5/build/clockprobe 2 > lanes/f-slice5/clock-before.log
for p in 2 3 5; do
  for n in 2000 10000 100000; do
    for f in exp log Log; do
      timeout 180 lanes/f-slice5/build/bench-after "$f" "$p" "$n" 3 \
        > "lanes/f-slice5/after-$f-$p-$n.log" 2>&1
      echo "$f $p $n exit=$?"
    done
  done
done
for f in exp log Log; do
  timeout 60 lanes/f-slice5/build/bench-after "$f" 18446744073709551557 10000 5 \
    > "lanes/f-slice5/after-$f-big-10000.log" 2>&1
  echo "$f big 10000 exit=$?"
done
timeout 10 lanes/f-slice5/build/clockprobe 2 > lanes/f-slice5/clock-after.log
