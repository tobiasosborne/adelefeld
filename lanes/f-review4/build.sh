#!/bin/sh
# build.sh (lane f-review4): builds the probe with four copies of lfunc.c into $OUT (a scratch directory).
#   old: src/lfunc.c at commit 1cf0e42 (git show: a read, not a change of state)
#   sum: the worktree's src/lfunc.c with the route forced to log_sum (F8)
#   bal: the worktree's src/lfunc.c with the route forced to log_balanced (F9)
# Usage: sh lanes/f-review4/build.sh OUTDIR [ARCHIVE]  (run from the worktree root; the library archive
#        defaults to lanes/f-review4/build/libadelefeld.a, from `make -j2 BUILD=lanes/f-review4/build`;
#        faults.py passes an archive with a faulty lfunc.o).
set -e
OUT="$1"
LIB="${2:-lanes/f-review4/build/libadelefeld.a}"
mkdir -p "$OUT"
git show 1cf0e42:src/lfunc.c > "$OUT/lfunc_old.c"
# The forced routes: replace the three-way selection by one call.
python3 - "$OUT" <<'EOF'
import sys, re
out = sys.argv[1]
src = open('src/lfunc.c').read()
sel = """    if (fmpz_bits(P) <= FLINT_BITS - 2)
        log_sum_word(S, p, zr, T, P, PK);
    else if (K <= 64)
        log_sum(S, p, zr, vz, K, T, PK);
    else
        log_balanced(S, p, zr, vz, K, PK);
"""
assert src.count(sel) == 1
open(out + '/lfunc_sum.c', 'w').write(src.replace(sel, "    log_sum(S, p, zr, vz, K, T, PK);\n"))
open(out + '/lfunc_bal.c', 'w').write(src.replace(sel, "    log_balanced(S, p, zr, vz, K, PK);\n"))
EOF
CF="-std=c11 -O2 -g -Wall -Wextra -Wno-unused-function -Iinclude -Isrc"
for v in old sum bal; do
  cc $CF -Dadf_lball_log=${v}_lball_log -Dadf_lball_Log=${v}_lball_Log -Dadf_lball_exp=${v}_lball_exp \
     -c "$OUT/lfunc_$v.c" -o "$OUT/lfunc_$v.o"
done
cc $CF -Werror -c lanes/f-review4/probe.c -o "$OUT/probe.o"
cc -o "$OUT/probe" "$OUT/probe.o" "$OUT/lfunc_old.o" "$OUT/lfunc_sum.o" "$OUT/lfunc_bal.o" \
   "$LIB" -lflint -lgmp -lm
echo "built $OUT/probe"
