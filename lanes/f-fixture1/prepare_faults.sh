#!/bin/sh
# The review tool hardcodes lanes/f-review4/build. Supply that path in a lane-owned
# scratch root, without creating or changing anything in the real f-review4 lane.
set -eu
ROOT=$(pwd)
HARNESS="$ROOT/lanes/f-fixture1/build/harness"
mkdir -p "$HARNESS/lanes/f-review4"
for name in src include tests; do
    ln -s "$ROOT/$name" "$HARNESS/$name"
done
ln -s "$ROOT/lanes/f-fixture1/build" "$HARNESS/lanes/f-review4/build"
for name in faults.py build.sh probe.c oracle.py; do
    ln -s "$ROOT/lanes/f-review4/$name" "$HARNESS/lanes/f-review4/$name"
done
