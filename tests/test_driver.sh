#!/bin/sh
# tests/test_driver.sh: the acceptance test of the command-line driver adf
# (docs/PLAN.md 6, row 1.5: "the tables of SPEC.md typed at the prompt").
#
#   sh tests/test_driver.sh          run from the repository root
#   SAN=1 sh tests/test_driver.sh   build and run the driver with the sanitizers
#
# Every case is a script of commands, tests/driver/NAME.cmd, with the expected lines in
# tests/driver/NAME.out.  A first line "#!exit N" of the script gives the expected exit
# status (0 when the line is absent).  The expected lines were written by hand from
# docs/SPEC.md, not from the output of the program; each script says in its first comment
# which part of the specification it is taken from.
#
# The hostile inputs are written by tests/driver/gen_hostile.sh into build/driver/; their
# expected outputs are the files of tests/driver/hostile/.
#
# The script exits with 0 when every case agrees, and with 1 at the first difference.

set -u

# byte-exact comparisons and a decimal point in the timing, whatever the locale is
LC_ALL=C
export LC_ALL

SAN=${SAN:-0}
# the sanitized driver has its own name, so that the two never mix
if [ "$SAN" = 1 ]; then ADF=build/adf-san; else ADF=build/adf; fi
D=tests/driver
H=$D/hostile
B=build/driver

cases=0
lines=0

mkdir -p "$B" || exit 1
: > "$B/empty"

if ! make -C tools/adf SAN="$SAN" > "$B/build.log" 2>&1; then
    echo "test_driver: the driver does not build; see $B/build.log"
    sed -n '1,20p' "$B/build.log"
    exit 1
fi
[ -x "$ADF" ] || { echo "test_driver: $ADF is missing"; exit 1; }

# want_status FILE: the exit status the case expects, from its "#!exit" line.
want_status()
{
    sed -n 's/^#!exit \([0-9][0-9]*\)$/\1/p' "$1" | head -n 1
}

# run_case NAME INPUT EXPECTED STATUS
run_case()
{
    cases=$((cases + 1))
    "$ADF" < "$2" > "$B/got.out" 2> "$B/got.err"
    got=$?
    if [ "$got" != "$4" ]; then
        echo "test_driver: $1: the exit status is $got, expected $4"
        sed -n '1,10p' "$B/got.err"
        exit 1
    fi
    if ! cmp -s "$3" "$B/got.out"; then
        echo "test_driver: $1: the output differs from $3"
        diff -u "$3" "$B/got.out" | sed -n '1,40p'
        exit 1
    fi
    lines=$((lines + $(wc -l < "$3")))
}

# ---- the cases of the specification and of the language of the driver

for cmd in "$D"/*.cmd; do
    name=$(basename "$cmd" .cmd)
    out="$D/$name.out"
    [ -f "$out" ] || { echo "test_driver: no expected output for $cmd"; exit 1; }
    status=$(want_status "$cmd")
    [ -n "$status" ] || status=0
    run_case "$name" "$cmd" "$out" "$status"
done

# ---- hostile input

if ! sh "$D/gen_hostile.sh" > "$B/gen.log" 2>&1; then
    echo "test_driver: the hostile inputs were not written; see $B/gen.log"
    exit 1
fi

run_case h1_nul              "$B/h1_nul.cmd"              "$H/h1_nul.out"              1
run_case h2_high             "$B/h2_high.cmd"             "$H/h2_high.out"             1
run_case h3_line_65536       "$B/h3_line_65536.cmd"       "$H/h3_line_65536.out"       0
run_case h4_line_65537       "$B/h4_line_65537.cmd"       "$H/h4_line_65537.out"       1
run_case h5_no_final_newline "$B/h5_no_final_newline.cmd" "$H/h5_no_final_newline.out" 0
run_case h6_empty            "$B/h6_empty.cmd"            "$H/h6_empty.out"            0

# h7: 100000 lines, timed.  The expected output is 100000 lines of "7/3".
cases=$((cases + 1))
start=$(date +%s%N 2>/dev/null || echo 0)
"$ADF" < "$B/h7_many_lines.cmd" > "$B/h7_many_lines.got" 2> "$B/got.err"
got=$?
end=$(date +%s%N 2>/dev/null || echo 0)
if [ "$got" != 0 ]; then
    echo "test_driver: h7_many_lines: the exit status is $got, expected 0"
    exit 1
fi
if ! cmp -s "$B/h7_many_lines.out" "$B/h7_many_lines.got"; then
    echo "test_driver: h7_many_lines: the output differs from the expected 100000 lines"
    diff -u "$B/h7_many_lines.out" "$B/h7_many_lines.got" | sed -n '1,10p'
    exit 1
fi
lines=$((lines + 100000))
if [ "$start" != 0 ] && [ "$end" != 0 ]; then
    echo "test_driver: h7_many_lines: 100000 lines in $(awk -v a="$start" -v b="$end" \
        'BEGIN { printf "%.3f", (b - a) / 1000000000 }') s"
fi

# h8: a file that is a directory.  fopen succeeds on a directory and the first read
# fails, so the driver reports a usage error: nothing on standard output, status 2.
cases=$((cases + 1))
"$ADF" "$D" > "$B/h8.out" 2> "$B/h8.err"
got=$?
if [ "$got" != 2 ]; then
    echo "test_driver: h8_directory: the exit status is $got, expected 2"
    exit 1
fi
if ! cmp -s "$B/empty" "$B/h8.out"; then
    echo "test_driver: h8_directory: standard output is not empty"
    sed -n '1,10p' "$B/h8.out"
    exit 1
fi
if [ ! -s "$B/h8.err" ]; then
    echo "test_driver: h8_directory: nothing was written to standard error"
    exit 1
fi

# ---- standard input, -v, and the usage errors

cases=$((cases + 1))
printf 'show 7/3\n' | "$ADF" > "$B/stdin.out" 2> "$B/stdin.err"
if [ $? != 0 ]; then
    echo "test_driver: standard input: the exit status is not 0"
    exit 1
fi
if ! printf '7/3\n' | cmp -s - "$B/stdin.out"; then
    echo "test_driver: standard input: the output differs"
    diff -u "$B/stdin.out" "$B/empty" | sed -n '1,10p'
    exit 1
fi
if [ -s "$B/stdin.err" ]; then
    echo "test_driver: standard input: something was written to standard error"
    exit 1
fi
lines=$((lines + 1))

cases=$((cases + 1))
printf 'show 7/3\nshow 1/0\n' | "$ADF" -v > "$B/verbose.out" 2> "$B/verbose.err"
got=$?
if [ "$got" != 1 ]; then
    echo "test_driver: -v: the exit status is $got, expected 1"
    exit 1
fi
if ! printf '7/3\nerror: DOMAIN\n' | cmp -s - "$B/verbose.out"; then
    echo "test_driver: -v: the output on standard output differs"
    exit 1
fi
if [ ! -s "$B/verbose.err" ]; then
    echo "test_driver: -v: nothing was written to standard error"
    exit 1
fi
lines=$((lines + 2))

usage_case()
{
    cases=$((cases + 1))
    "$@" > "$B/usage.out" 2> "$B/usage.err"
    got=$?
    if [ "$got" != 2 ]; then
        echo "test_driver: usage: the exit status of \"$*\" is $got, expected 2"
        exit 1
    fi
    if [ ! -s "$B/usage.err" ]; then
        echo "test_driver: usage: nothing was written to standard error for \"$*\""
        exit 1
    fi
    if [ -s "$B/usage.out" ]; then
        echo "test_driver: usage: standard output is not empty for \"$*\""
        exit 1
    fi
}

usage_case "$ADF" --help
usage_case "$ADF" build/driver/does_not_exist
usage_case "$ADF" "$D" "$D"

echo "test_driver: $cases cases, $lines expected lines, all equal (SAN=$SAN)"
exit 0
