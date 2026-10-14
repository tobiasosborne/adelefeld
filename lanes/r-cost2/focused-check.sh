#!/bin/sh
set -eu
B=lanes/r-cost2/build-mutcache
timeout 170 make -s -j2 BUILD="$B" SAN=1 INV=1 "$B/test_roots_real"
timeout 170 "$B/test_roots_real"
