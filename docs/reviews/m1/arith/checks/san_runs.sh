#!/bin/sh
# The four fuzzers of this directory, built by build_san.sh (library sources under ASan + UBSan),
# each piped into its oracle; sanitizer output goes to stderr and its size is printed.
# Run from the root of the worktree. Output: san_runs.out.
d=docs/reviews/m1/arith/checks
t=${TMPDIR:-/tmp}/arith_san.$$
echo "== fball_fuzz.san 5000 5 60"
$d/fball_fuzz.san 5000 5 60 2> $t | python3 $d/fball_fuzz_check.py
echo "sanitizer stderr bytes: $(wc -c < $t)"
echo "== adele_prec.san 3000 7"
$d/adele_prec.san 3000 7 2> $t | python3 $d/adele_prec_check.py | tail -1
echo "sanitizer stderr bytes: $(wc -c < $t)"
echo "== recon_fuzz.san 5000 11 20"
$d/recon_fuzz.san 5000 11 20 2> $t | python3 $d/recon_fuzz_check.py
echo "sanitizer stderr bytes: $(wc -c < $t)"
echo "== rat_fuzz.san 5000 11 64"
$d/rat_fuzz.san 5000 11 64 2> $t | python3 $d/rat_fuzz_check.py
echo "sanitizer stderr bytes: $(wc -c < $t)"
rm -f $t
