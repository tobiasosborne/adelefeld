#!/bin/bash
# run_limits.sh: the official run of tests/test_modctx_limits.c under the 4 GB
# address-space limit of the brief.  ADF_MODCTX_LIMITS_FULL=1 turns on the three admitted
# k = 65536 cases, which are slow (about two minutes in all); without it the same code
# paths run at k = 2000.
set -u
cd "$(dirname "$0")/../.."
ulimit -v 4000000
ADF_MODCTX_LIMITS_FULL=1 exec ./build/test_modctx_limits "$@"
