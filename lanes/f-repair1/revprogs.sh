#!/bin/sh
# Builds the library (release and INV) into this lane and compiles the reviewer's programs against it.
# Usage: sh lanes/f-repair1/revprogs.sh   (from the repository root)
R=lanes/f-repair1
mkdir -p $R/rb $R/ri
timeout 600 make -j2 BUILD=$R/rb all >$R/rb.log 2>&1 || { echo build failed; exit 1; }
timeout 600 make -j2 BUILD=$R/ri INV=1 all >$R/ri.log 2>&1 || { echo invbuild failed; exit 1; }
F="-std=c11 -O2 -g -Wall -Wextra -Iinclude"
cc $F lanes/f-review1/boundaries.c $R/rb/libadelefeld.a -lflint -lgmp -lm -o $R/boundaries || exit 1
cc $F lanes/f-review1/test_assertion.c $R/rb/libadelefeld.a -lflint -lgmp -lm -o $R/test_assertion || exit 1
cc $F -DADF_CHECK_INVARIANTS lanes/f-review1/invariant_probe.c $R/ri/libadelefeld.a -lflint -lgmp -lm -pthread \
    -o $R/invariant_probe || exit 1
# the reviewer's script hardwires lanes/f-review1/invariant_probe: use a copy that points to this lane
sed 's#lanes/f-review1/invariant_probe#lanes/f-repair1/invariant_probe#' lanes/f-review1/check_invariants.py \
    > $R/check_invariants.py
echo programs built
