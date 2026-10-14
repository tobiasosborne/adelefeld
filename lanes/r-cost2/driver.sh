#!/bin/sh
# Targeted form of tests/test_driver.sh: same fixtures, statuses and byte comparisons.
set -eu
B=lanes/r-cost2/build-after
timeout 170 cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude \
    tools/adf/adf.c "$B/libadelefeld.a" -lflint -lgmp -lm -o "$B/adf"
cases=0
lines=0
for name in s-realroots s-hostile; do
    input="tests/driver/$name.cmd"
    expected="tests/driver/$name.out"
    status=$(sed -n 's/^#!exit \([0-9][0-9]*\)$/\1/p' "$input" | head -n 1)
    status=${status:-0}
    got=0
    timeout 170 taskset -c 2 "$B/adf" < "$input" > "$B/$name.got" 2> "$B/$name.err" || got=$?
    test "$got" = "$status"
    cmp "$expected" "$B/$name.got"
    n=$(wc -l < "$expected")
    printf '%s: exit %s, expected %s, %s equal lines\n' "$name" "$got" "$status" "$n"
    cases=$((cases + 1))
    lines=$((lines + n))
done
printf 'driver: %s cases, %s expected lines, 0 differences\n' "$cases" "$lines"
