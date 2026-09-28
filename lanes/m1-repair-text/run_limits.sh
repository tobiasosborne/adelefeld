#!/bin/sh
# Run tests/test_text_limits under the resource limits of finding R2/adf-b8l: 2 GiB of address
# space, 1 CPU second per printer call is not separable, so 20 seconds for the whole binary.
# The printer must return or refuse quickly; it must not expand an over-bound value.
set -e
ulimit -v 2000000
ulimit -t 20
ulimit -c 0
exec ./build/test_text_limits
