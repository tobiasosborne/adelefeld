#!/bin/sh
# Restrict each mutant to the two finite-function tests, under bounded processes and two make jobs.
set -eu
export ASAN_OPTIONS=detect_leaks=0
export UBSAN_OPTIONS=halt_on_error=1
# Use the check target with the selected TEST_BIN override; the outer timeout bounds its test loop.
timeout 120 make -s -j2 check INV=1 CC=clang \
    TEST_BIN='build/test_ffun build/test_ffun_algebra'
