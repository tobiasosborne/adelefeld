#!/bin/sh
# lanes/m1-testgaps/redgreen.sh: apply one mutation by hand in a scratch copy of the tree under
# build/redgreen/, build one test program there and run it. Usage:
#
#   lanes/m1-testgaps/redgreen.sh <file> <line> <delete|replace> <replacement> <test> <label>
#
# <file> is relative to src/, <line> is the line number in the current src/<file>, delete removes
# the line, replace writes <replacement> in its place. The working tree is never written to.

set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
FILE=$1
LINE=$2
MODE=$3
REPL=$4
TEST=$5
LABEL=$6
SCRATCH=$ROOT/build/redgreen

rm -rf "$SCRATCH"
mkdir -p "$SCRATCH/src" "$SCRATCH/tests/support" "$SCRATCH/include"
cp "$ROOT/Makefile" "$SCRATCH/"
cp -r "$ROOT/include/." "$SCRATCH/include/"
cp "$ROOT"/src/*.c "$SCRATCH/src/"
cp "$ROOT"/tests/support/*.c "$ROOT"/tests/support/*.h "$SCRATCH/tests/support/"
cp "$ROOT/tests/test_runner.h" "$SCRATCH/tests/"
cp "$ROOT/tests/$TEST.c" "$SCRATCH/tests/"

python3 - "$SCRATCH/src/$FILE" "$LINE" "$MODE" "$REPL" <<'PY'
import sys
path, line, mode, repl = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
lines = open(path).readlines()
assert lines[line - 1].strip() != "", "line %d is empty" % line
if mode == "delete":
    del lines[line - 1]
else:
    lines[line - 1] = repl + "\n"
open(path, "w").writelines(lines)
PY

echo "=== $LABEL"
echo "--- src/$FILE:$LINE $MODE"
if ! ( cd "$SCRATCH" && make -s -j2 "build/$TEST" > "$SCRATCH/log.txt" 2>&1 ); then
    echo "not compiled: the mutant does not build (mutate.py counts this as \"not compiled\","
    echo "              so it is not a survivor); the reason:"
    grep -m 2 "error" "$SCRATCH/log.txt" | sed "s/^/              /"
    exit 2
fi
if ( cd "$ROOT" && "$SCRATCH/build/$TEST" > "$SCRATCH/out.txt" 2>&1 ); then
    echo "SURVIVED: the test passes against the mutant"
    tail -3 "$SCRATCH/out.txt"
    exit 1
else
    echo "red: the test fails, as it must"
    grep -m 4 "^FAIL" "$SCRATCH/out.txt" || tail -3 "$SCRATCH/out.txt"
fi
