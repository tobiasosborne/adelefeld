#!/bin/sh
# lanes/m1-testgaps/run_redgreen.sh: every mutation this lane's two test files are written for,
# one at a time, in a scratch copy of the tree under build/redgreen/. The output is
# lanes/m1-testgaps/redgreen.log.

set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
RG=$ROOT/lanes/m1-testgaps/redgreen.sh
LOG=$ROOT/lanes/m1-testgaps/redgreen.log
T1=test_fball_overwrite
T2=test_adele_prec

: > "$LOG"
{
  echo "# lanes/m1-testgaps/redgreen.log: each mutation applied by hand in build/redgreen/, the"
  echo "# one new test program built there and run from the repository root. Red means the test"
  echo "# fails, which is what the mutation must achieve. Date: $(date -u +%Y-%m-%d)."
  echo
} >> "$LOG"

# redgreen.sh exits 0 on a red run, 1 if the mutant survived, 2 if it does not build; the log
# says which, and the battery carries on.
run() { "$RG" "$@" >> "$LOG" 2>&1 || true; echo "" >> "$LOG"; }

echo "## tests/test_fball_overwrite.c against drop_call mutants of src/fball.c" >> "$LOG"
# adf_fball_zero, line 256
run fball.c 259 delete "" $T1 "fball: drop fmpz_zero(x->A) in adf_fball_zero (line 259)"
run fball.c 260 delete "" $T1 "fball: drop fmpz_zero(x->H) in adf_fball_zero (line 260)"
run fball.c 261 delete "" $T1 "fball: drop fmpz_one(x->d) in adf_fball_zero (line 261)"
# adf_fball_one, line 268
run fball.c 271 delete "" $T1 "fball: drop fmpz_one(x->A) in adf_fball_one (line 271)"
run fball.c 272 delete "" $T1 "fball: drop fmpz_zero(x->H) in adf_fball_one (line 272)"
run fball.c 273 delete "" $T1 "fball: drop fmpz_one(x->d) in adf_fball_one (line 273)"
# adf_fball_set_si, line 280
run fball.c 283 delete "" $T1 "fball: drop fmpz_set_si(x->A, n) in adf_fball_set_si (line 283)"
run fball.c 284 delete "" $T1 "fball: drop fmpz_zero(x->H) in adf_fball_set_si (line 284)"
run fball.c 285 delete "" $T1 "fball: drop fmpz_one(x->d) in adf_fball_set_si (line 285)"
# adf_fball_set_fmpz, line 292
run fball.c 295 delete "" $T1 "fball: drop fmpz_set(x->A, n) in adf_fball_set_fmpz (line 295)"
run fball.c 296 delete "" $T1 "fball: drop fmpz_zero(x->H) in adf_fball_set_fmpz (line 296)"
run fball.c 297 delete "" $T1 "fball: drop fmpz_one(x->d) in adf_fball_set_fmpz (line 297)"
# adf_fball_set_rat, line 304
run fball.c 307 delete "" $T1 "fball: drop fmpz_set(x->A, num) in adf_fball_set_rat (line 307)"
run fball.c 308 delete "" $T1 "fball: drop fmpz_zero(x->H) in adf_fball_set_rat (line 308)"
run fball.c 309 delete "" $T1 "fball: drop fmpz_set(x->d, den) in adf_fball_set_rat (line 309)"
# adf_fball_set, line 163
run fball.c 170 delete "" $T1 "fball: drop fmpz_set(y->A, x->A) in adf_fball_set (line 170)"
run fball.c 171 delete "" $T1 "fball: drop fmpz_set(y->H, x->H) in adf_fball_set (line 171)"
run fball.c 172 delete "" $T1 "fball: drop fmpz_set(y->d, x->d) in adf_fball_set (line 172)"
# fb_store, line 72, reached by set_fmpz3, set_center_radius and canonicalise
run fball.c 77 delete "" $T1 "fball: drop fmpz_set(x->A, A) in fb_store (line 77)"
run fball.c 78 delete "" $T1 "fball: drop fmpz_set(x->H, H) in fb_store (line 78)"
run fball.c 79 delete "" $T1 "fball: drop fmpz_set(x->d, d) in fb_store (line 79)"

echo >> "$LOG"
echo "## tests/test_adele_prec.c against prec mutants of src/adele.c" >> "$LOG"
run adele.c 256 replace "    arb_set_fmpq(t, tq, 2);" $T2 "adele: prec -> 2 in adf_adele_add_rat (line 256)"
run adele.c 259 replace "    arb_add(z->inf, x->inf, t, 2);" $T2 "adele: prec -> 2 in adf_adele_add_rat (line 259)"
run adele.c 283 replace "    arb_set_fmpq(t, tq, 2);" $T2 "adele: prec -> 2 in adf_adele_mul_rat (line 283)"
run adele.c 285 replace "    arb_mul(z->inf, x->inf, t, 2);" $T2 "adele: prec -> 2 in adf_adele_mul_rat (line 285)"
run adele.c 318 replace "        arb_set_fmpq(t, tq, 2);" $T2 "adele: prec -> 2 in adf_adele_div_rat (line 318)"
run adele.c 319 replace "        arb_div(z->inf, x->inf, t, 2);" $T2 "adele: prec -> 2 in adf_adele_div_rat (line 319)"
run adele.c 489 replace "    acb_set_fmpq(t, tq, 2);" $T2 "adele: prec -> 2 in adf_cadele_add_rat (line 489)"
run adele.c 492 replace "    acb_add(z->inf, x->inf, t, 2);" $T2 "adele: prec -> 2 in adf_cadele_add_rat (line 492)"
run adele.c 512 replace "    acb_set_fmpq(t, tq, 2);" $T2 "adele: prec -> 2 in adf_cadele_mul_rat (line 512)"
run adele.c 514 replace "    acb_mul(z->inf, x->inf, t, 2);" $T2 "adele: prec -> 2 in adf_cadele_mul_rat (line 514)"
run adele.c 543 replace "        acb_set_fmpq(t, tq, 2);" $T2 "adele: prec -> 2 in adf_cadele_div_rat (line 543)"
run adele.c 544 replace "        acb_div(z->inf, x->inf, t, 2);" $T2 "adele: prec -> 2 in adf_cadele_div_rat (line 544)"
run adele.c 123 replace "    arb_set_fmpq(y->inf, t, prec); arb_set_fmpq(y->inf, t, 2);" $T2 "adele: prec -> 2 in adf_adele_set_rat (line 123)"
run adele.c 392 replace "    acb_set_fmpq(y->inf, t, prec); acb_set_fmpq(y->inf, t, 2);" $T2 "adele: prec -> 2 in adf_cadele_set_rat (line 392)"

echo >> "$LOG"
echo "## tests/test_adele_prec.c against the drop_call and status mutants of src/adele.c" >> "$LOG"
run adele.c 82 delete "" $T2 "adele: drop adf_fball_swap in adf_adele_swap (line 82)"
run adele.c 360 delete "" $T2 "adele: drop adf_fball_swap in adf_cadele_swap (line 360)"
run adele.c 179 replace "    return ADF_DOMAIN;" $T2 "adele: ADF_OK -> ADF_DOMAIN in adf_adele_get_arb_at (line 179)"

echo >> "$LOG"
echo "## green: the unmutated tree" >> "$LOG"
make -s -C "$ROOT" "build/$T1" "build/$T2" >> "$LOG" 2>&1
( cd "$ROOT" && ./build/$T1 >> "$LOG" 2>&1 && echo "green: $T1 passes" >> "$LOG" )
( cd "$ROOT" && ./build/$T2 >> "$LOG" 2>&1 && echo "green: $T2 passes" >> "$LOG" )
rm -rf "$ROOT/build/redgreen"
echo "written: $LOG"
