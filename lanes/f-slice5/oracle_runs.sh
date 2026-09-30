#!/bin/sh
for mode in precision small big limits random large powered_large; do
    timeout 180 python3 -B lanes/f-slice5/oracle.py "$mode" > "lanes/f-slice5/oracle-$mode.log" 2>&1
    echo "$mode exit=$?"
    tail -1 "lanes/f-slice5/oracle-$mode.log"
done
timeout 180 lanes/f-slice5/build/probe < lanes/f-slice5/bench.in > lanes/f-slice5/bench-new.out
head -7 lanes/f-slice5/bench-new.out > lanes/f-slice5/bench.out
timeout 180 python3 -B lanes/f-slice5/oracle.py bench_validate > lanes/f-slice5/oracle-bench_validate.log 2>&1
echo "bench_validate exit=$?"
tail -1 lanes/f-slice5/oracle-bench_validate.log
