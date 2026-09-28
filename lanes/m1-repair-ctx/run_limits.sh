#!/bin/bash
# run_limits.sh: the official run of tests/test_modctx_limits.c under the 4 GB
# address-space limit of the brief.  ADF_MODCTX_LIMITS_FULL=1 (the default here) turns on the
# admitted k = 65536 cases; without it the same code paths run at k = 2000.  The full run
# measured 321 s of user time on the lane machine (the k = 65536 context dominates), so set
# ADF_MODCTX_LIMITS_FULL=0 for a quick run of the refusal paths alone.
set -u
cd "$(dirname "$0")/../.."
ulimit -v 4000000
ADF_MODCTX_LIMITS_FULL="${ADF_MODCTX_LIMITS_FULL:-1}" exec ./build/test_modctx_limits "$@"
