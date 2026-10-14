#!/bin/sh
set -eu
B=lanes/r-cost2/build-mutcache
names='test_roots_padic test_roots_bigp test_roots_seed test_roots_forged'
names="$names test_roots_real test_roots_real_isolate"
set --
for name in $names; do set -- "$@" "$B/$name"; done
timeout 170 make -s -j2 BUILD="$B" SAN=1 INV=1 "$@"
for program in "$@"; do timeout 170 "$program"; done
