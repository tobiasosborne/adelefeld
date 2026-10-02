#!/bin/sh
# lanes/f-slice9/drv.sh: build the driver into lanes/f-slice9/build and run the pow-*.cmd cases (lane work only;
# the final check is tests/test_driver.sh in build/).
B=lanes/f-slice9/build
make -s -j2 BUILD=$B $B/libadelefeld.a || exit 1
cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror tools/adf/adf.c $B/libadelefeld.a -lflint -lgmp -lm \
   -o $B/adf || exit 1
fail=0
for cmd in tests/driver/pow-*.cmd ${EXTRA:-}; do
    out=${cmd%.cmd}.out
    want=$(sed -n 's/^#!exit \([0-9][0-9]*\)$/\1/p' "$cmd" | head -n 1); [ -n "$want" ] || want=0
    timeout 60 $B/adf < "$cmd" > $B/got.out 2> $B/got.err; got=$?
    if [ "$got" != "$want" ] || ! cmp -s "$out" $B/got.out; then
        echo "FAIL $cmd (exit $got, want $want)"; diff "$out" $B/got.out | head -40; fail=1
    else echo "ok   $cmd ($(wc -l < "$out") lines)"; fi
done
exit $fail
