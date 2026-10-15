#!/bin/sh
# ux/notebook/run.sh: build what is missing, then regenerate both scenario pages and their records.
# Run from the repository root: sh ux/notebook/run.sh
set -eu
[ -d ux/notebook ] || { echo "run.sh: run it from the repository root" >&2; exit 2; }
[ -f build/libadelefeld.a ] || timeout 600 make -j2
[ -x build/adf ] || timeout 600 make -C tools/adf
[ -f build/libadelefeld.so ] || timeout 600 sh tests/test_exports.sh
exec sh ux/notebook/julia.sh ux/notebook/src/make_pages.jl
