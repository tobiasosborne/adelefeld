#!/bin/sh
for p in 2 3 5; do
  for n in 2000 10000 100000; do
    for f in exp log Log; do
      timeout 180 lanes/f-slice5/build/bench-before "$f" "$p" "$n" 3 \
        > "lanes/f-slice5/before-$f-$p-$n.log" 2>&1
      echo "$f $p $n exit=$?"
    done
  done
done
for f in exp log Log; do
  timeout 120 lanes/f-slice5/build/bench-before "$f" 18446744073709551557 10000 1 \
    > "lanes/f-slice5/before-$f-big-10000.log" 2>&1
  echo "$f big 10000 exit=$?"
done
