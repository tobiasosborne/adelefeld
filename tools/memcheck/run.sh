#!/usr/bin/env bash
#
# run.sh -- check tests and library for reads of uninitialised FLINT / adelefeld values.
#
# What is run:
#   valgrind present        build every tests/test_*.c at -O1 -g into build-memcheck/ and run
#                           each binary under
#                             valgrind --error-exitcode=9 --track-origins=yes --leak-check=full --errors-for-leak-kinds=definite,indirect -q
#                           at most 2 at a time, each with a 120 s time limit.  One line per
#                           program: PASS, ERROR (with the first error) or TIMEOUT.
#   valgrind absent         MemorySanitizer is probed.  Even when the probe works, FLINT and GMP
#                           are not instrumented, so every value that comes back from FLINT looks
#                           uninitialised: a useful run is not possible, and none is faked.
#   neither usable          the static checker tools/memcheck/check_uninit.py is run over src/ and
#                           tests/, which is what this host does.
#
# Usage:
#   tools/memcheck/run.sh             auto: valgrind if present, else the checker
#   tools/memcheck/run.sh --checker   force the static checker
#
# Exit status: 0 when no problem was found, 1 when a finding or a valgrind error was reported.

set -u

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
cd "$root"

JOBS=2
TIME_LIMIT=120
BUILD_DIR=build-memcheck

run_checker() {
    python3 "$here/check_uninit.py" src/*.c tests/*.c
}

msan_probe() {
    # Returns 0 when clang MemorySanitizer runs and reports a plain uninitialised read.
    command -v clang > /dev/null 2>&1 || return 1
    local src out bin rc
    src="$(mktemp /tmp/adf-msan-XXXXXX.c)"
    bin="$(mktemp /tmp/adf-msan-XXXXXX)"
    out="$(mktemp /tmp/adf-msan-XXXXXX.out)"
    cat > "$src" <<'EOF'
#include <stdio.h>
int main(void)
{
    int x;
    if (x == 0x5A5A) printf("unreachable\n");
    return 0;
}
EOF
    if ! clang -fsanitize=memory -fno-omit-frame-pointer -g -O1 -o "$bin" "$src" \
            > "$out" 2>&1; then
        rm -f "$src" "$bin" "$out"
        return 1
    fi
    "$bin" > "$out" 2>&1
    rc=$?
    grep -q "use-of-uninitialized-value" "$out"
    local found=$?
    rm -f "$src" "$bin" "$out"
    [ "$rc" -ne 0 ] && [ "$found" -eq 0 ]
}

run_one_valgrind() {
    local bin="$1" out rc line
    out="$(mktemp /tmp/adf-valgrind-XXXXXX)"
    timeout "$TIME_LIMIT" valgrind --error-exitcode=9 --track-origins=yes \
        --leak-check=full --errors-for-leak-kinds=definite,indirect -q "$bin" > "$out" 2>&1
    rc=$?
    if [ "$rc" -eq 0 ]; then
        echo "PASS    $bin"
    elif [ "$rc" -eq 124 ]; then
        echo "TIMEOUT $bin (limit ${TIME_LIMIT}s)"
    else
        line="$(grep -m1 -E 'uninitialised|Invalid|definitely lost|ERROR SUMMARY' "$out")"
        echo "ERROR   $bin: ${line:-exit $rc}"
    fi
    rm -f "$out"
}

if [ "${1:-}" = "--checker" ]; then
    run_checker
    exit $?
fi

if command -v valgrind > /dev/null 2>&1; then
    echo "valgrind $(valgrind --version), building at -O1 -g into $BUILD_DIR"
    mapfile -t sources < <(ls tests/test_*.c 2> /dev/null)
    bins=()
    for src in "${sources[@]}"; do
        bins+=("$BUILD_DIR/$(basename "$src" .c)")
    done
    if [ "${#bins[@]}" -eq 0 ]; then
        echo "no tests/test_*.c found"
        exit 1
    fi
    if ! make BUILD="$BUILD_DIR" \
            CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror' -j2 "${bins[@]}"; then
        echo "build failed"
        exit 1
    fi
    results="$(mktemp /tmp/adf-valgrind-results-XXXXXX)"
    for bin in "${bins[@]}"; do
        run_one_valgrind "$bin" >> "$results" &
        while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do
            wait -n
        done
    done
    wait
    sort "$results"
    if grep -qE '^(ERROR|TIMEOUT)' "$results"; then
        rm -f "$results"
        exit 1
    fi
    rm -f "$results"
    exit 0
fi

echo "valgrind is not installed"
if command -v clang > /dev/null 2>&1; then
    if msan_probe; then
        echo "MemorySanitizer runs, but FLINT and GMP are not instrumented: every value that"
        echo "comes back from FLINT would look uninitialised, so a useful run is not possible."
    else
        echo "MemorySanitizer does not produce a usable instrumented run on this host."
    fi
else
    echo "clang is not installed either."
fi
echo "falling back to the static checker (reads the C source):"
run_checker
