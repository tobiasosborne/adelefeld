#!/bin/sh
# Builds and runs test_poisson and test_tensor plain, under SAN=1 (leak detection), INV=1 and CC=clang, 2 jobs.
cd "$(dirname "$0")/../.." || exit 1
L=lanes/f4-slice6
for v in plain san inv clang; do
    case $v in
        plain) args="" ;;
        san) args="SAN=1" ;;
        inv) args="INV=1" ;;
        clang) args="CC=clang" ;;
    esac
    if ! timeout 900 make -s -j2 BUILD=$L/b-$v $args $L/b-$v/test_poisson $L/b-$v/test_tensor \
            > $L/b-$v.log 2>&1; then
        echo "$v: BUILD FAILED"; tail -5 $L/b-$v.log; continue
    fi
    for t in test_poisson test_tensor; do
        ASAN_OPTIONS=detect_leaks=1 timeout 600 ./$L/b-$v/$t > $L/b-$v-$t.out 2>&1
        echo "$v $t: exit $? $(tail -1 $L/b-$v-$t.out)"
    done
done
