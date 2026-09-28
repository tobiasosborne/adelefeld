#!/bin/bash
# ground_truth.sh: the FLINT citations of src/modctx.c and src/modctx_internal.h against the
# files on disk. Run from the worktree root (refs/src must exist).
echo "== src/modctx.c:14-15 says the FLINT 3.0.1 documentation of multi mod / multi CRT is not on disk:"
sed -n 14,15p src/modctx.c
echo "== but refs/src/flint-3.0.1/fmpz.rst documents multi CRT (and not multi mod):"
grep -n "fmpz_multi_CRT_precomp\|fmpz_multi_mod_precomp" refs/src/flint-3.0.1/fmpz.rst
sed -n 1362,1365p refs/src/flint-3.0.1/fmpz.rst
echo "== the internal header promises [0, K) for recombine (modctx_internal.h:10-11), the call passes sign 0:"
sed -n 10,11p src/modctx_internal.h
grep -n "fmpz_multi_CRT_precomp(out" src/modctx.c
echo "== src/modctx.c:147-148 cites fmpz.h:656 and :691 for the precomp calls; those lines are:"
sed -n 148p src/modctx.c
sed -n 656p /usr/include/flint/fmpz.h
sed -n 691p /usr/include/flint/fmpz.h
echo "   the functions called are at:"
grep -n "^void fmpz_multi_CRT_precomp\|^void fmpz_multi_mod_precomp" /usr/include/flint/fmpz.h
echo "== src/modctx.c:19-23 speaks of fmpz_comb tables and fmpz_comb_temp_t; uses of fmpz_comb in the code:"
sed -n 19,23p src/modctx.c
grep -c "fmpz_comb_init\|fmpz_comb_temp_init\|fmpz_multi_mod_ui\|fmpz_multi_CRT_ui" src/modctx.c
