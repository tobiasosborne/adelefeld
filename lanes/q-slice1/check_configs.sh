#!/bin/sh
# Separate build trees; every executable has its own timeout. At most two compiler jobs.
set -u
failed=0
texts=$(printf '%s\n' tests/test_text*.c | sed 's|tests/||; s|\.c$||')
if [ "$#" -eq 0 ]; then set -- san inv clang; fi
for config in "$@"; do
    build="lanes/q-slice1/$config"
    targets="$build/test_qclass"
    for test in $texts; do targets="$targets $build/$test"; done
    case "$config" in
        san) flags='SAN=1' ;;
        inv) flags='INV=1' ;;
        clang) flags='CC=clang' ;;
    esac
    # flags and targets are words made above, with no shell input from text files.
    timeout 180 make -j2 BUILD="$build" $flags $targets > "lanes/q-slice1/$config-build.log" 2>&1
    code=$?
    printf '%s build exit %s\n' "$config" "$code"
    if [ "$code" -ne 0 ]; then failed=1; continue; fi
    : > "lanes/q-slice1/$config-tests.log"
    for test in $targets; do
        printf '== %s\n' "$test" >> "lanes/q-slice1/$config-tests.log"
        ASAN_OPTIONS=detect_leaks=0 timeout 120 "$test" >> "lanes/q-slice1/$config-tests.log" 2>&1
        code=$?
        if [ "$code" -ne 0 ]; then failed=1; fi
        printf '%s exit %s\n' "$test" "$code" | tee -a "lanes/q-slice1/$config-tests.log"
    done
done
exit "$failed"
