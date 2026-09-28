#!/bin/sh
# lanes/m1-repair-recon/run_limit.sh: the tests of the limit of decision M1-D3 under a memory
# limit, so that a regression that tries to allocate the exact end points fails fast instead of
# swapping (the review's reproducer, checks/recon_big_alloc.out: "Cannot reallocate memory
# (old_size=16 new_size=8589934600)", exit 134). Run from the root of the worktree after
# `make -j2 build/test_recon_limit`:
#
#   sh lanes/m1-repair-recon/run_limit.sh
#
# The limit is 2000000 KiB of address space, the one of the review.
set -e
prog=${1:-build/test_recon_limit}
( ulimit -v 2000000; time "$prog" )
echo "exit=$?"
