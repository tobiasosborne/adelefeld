#!/bin/sh
# lanes/i-slice1/run_faults.sh: do the tests bite? (brief item 5). Four faults, each in its own scratch copy of
# the tree under build/faults/<F>/ (Makefile, include, src, tests); the two test programs of the slice are
# built and run there. Run from the repository root:  sh lanes/i-slice1/run_faults.sh
#
#   F1  src/idele.c, E1: the lower end l = |m| - rho rounded UP (ARF_RND_CEIL) instead of down
#   F2  src/idele.c, B1: NOT_DETERMINED only when e(hi) - e(lo) > p + 1 instead of > p
#   F3  src/ucoset.c, normal form: the modulus is not halved when N = 2 mod 4
#   F4  src/idele.c, adf_idele_mul: the rejected design, arb_mul followed by a test of arb_is_nonzero
set -u
root=$(pwd)
[ -d include/adelefeld ] || { echo "run from the repository root" >&2; exit 2; }
for f in F1 F2 F3 F4; do
    d=build/faults/$f
    rm -rf "$d"
    mkdir -p "$d"
    cp -r Makefile include src tests "$d/"
done
sed -i 's/arf_sub(l, a, r, p, ARF_RND_FLOOR);/arf_sub(l, a, r, p, ARF_RND_CEIL);/' build/faults/F1/src/idele.c
sed -i 's/b1 = fmpz_cmp_si(gap, p) > 0;/b1 = fmpz_cmp_si(gap, p + 1) > 0;/' build/faults/F2/src/idele.c
sed -i 's/        fmpz_fdiv_q_2exp(N, N, 1);/        fmpz_fdiv_q_2exp(N, N, 0);/' build/faults/F3/src/ucoset.c
f4='    (void) sx; (void) sy; arb_mul(t, x->inf, y->inf, p); st = arb_is_nonzero(t) ? ADF_OK : ADF_NOT_DETERMINED;'
sed -i "s/    st = ball_from_ends(t, lo, hi, sx \\* sy, p);/$f4/" build/faults/F4/src/idele.c
for f in F1 F2 F3 F4; do
    d=build/faults/$f
    n=$(diff -r src "$d/src" | grep -c '^>')
    echo "== $f: $n changed line(s)"
    diff -r src "$d/src" | grep '^[<>]'
    if ! (cd "$d" && make -s -j2 build/test_ucoset build/test_idele > build.log 2>&1); then
        echo "   build failed"
        cat "$d/build.log"
        continue
    fi
    for t in test_ucoset test_idele; do
        (cd "$d" && timeout 120 "./build/$t" > "$t.out" 2>&1)
        rc=$?
        echo "   $t: exit $rc; $(tail -1 "$d/$t.out")"
        grep '^FAIL [A-Za-z_0-9]* (' "$d/$t.out" | sed 's/^/      /'
    done
done
cd "$root"
