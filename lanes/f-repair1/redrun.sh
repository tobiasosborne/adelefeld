#!/bin/sh
# Builds tests/test_lball.c against the library in lanes/f-repair1/rb (release) and ri (INV) and runs it.
# Usage: sh lanes/f-repair1/redrun.sh   (from the repository root)
R=lanes/f-repair1
timeout 600 make -j2 BUILD=$R/rb $R/rb/test_lball >$R/rb-test-build.log 2>&1 || { tail -30 $R/rb-test-build.log; echo "release test build failed"; }
echo "== release: test_lball"
timeout 300 $R/rb/test_lball 2>&1 | grep -v "^ok " | head -${LINES_MAX:-60}
echo "exit=$?"
timeout 600 make -j2 BUILD=$R/ri INV=1 $R/ri/test_lball >$R/ri-test-build.log 2>&1 || { tail -30 $R/ri-test-build.log; echo "INV test build failed"; }
echo "== INV: test_lball"
timeout 300 $R/ri/test_lball 2>&1 | grep -v "^ok " | head -${LINES_MAX:-60}
