#!/bin/sh
# Runs the reviewer's programs (built by revprogs.sh). Prints the outputs and exit codes.
R=lanes/f-repair1
echo "== boundaries"; timeout 20 $R/boundaries; echo exit=$?
echo "== check_invariants.py"; timeout 65 python3 -B $R/check_invariants.py; echo exit=$?
echo "== test_assertion (F6, the reviewer's program: it does not run tests/test_lball.c)"
timeout 10 $R/test_assertion; echo exit=$?
echo "== proof_probe.py (F5, checks the equality as the reviewer wrote it)"
timeout 10 python3 -B lanes/f-review1/proof_probe.py 2>&1 | tail -3; echo exit=$?
