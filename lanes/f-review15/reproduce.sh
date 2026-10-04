#!/bin/sh
# Recreate the three coverage witnesses after the scratch build has been removed.
set -eu
cd "$(dirname "$0")/../.."
L=lanes/f-review15
timeout 600 make -j2 BUILD="$L/build" "$L/build/libadelefeld.a"
timeout 10 python3 "$L/setup.py"
timeout 30 cc -std=c11 -O2 -g -Iinclude -Isrc -DREVIEW_TRACE "$L/bridge.c" \
    "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/build/bridge"
timeout 60 make -j2 BUILD="$L/build" "$L/build/test_localfactor"
timeout 30 "$L/build/test_localfactor"
timeout 170 python3 "$L/mutations.py"
timeout 170 python3 "$L/witnesses.py"
timeout 170 python3 "$L/minimize.py"
