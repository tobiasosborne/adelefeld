#!/bin/sh
# lanes/i-slice3/run_faults.sh: do the tests bite? (brief item 5). Six faults, each in its own scratch copy of the
# tree under build/faults3/<F>/ (Makefile, include, src, tests); the two test programs of the slice are built and
# run there. Run from the repository root:  sh lanes/i-slice3/run_faults.sh
#
#   H1  src/idpow.c, the tight modulus: b_p = e_p instead of 1 + e_p for an odd p with (p - 1) | k
#   H2  src/idpow.c, the sign of a power: sign(X) also for even k
#   H3  src/idpow.c, the real kernel for k < 0: lo = RD_p(1/lo) (the wrong end) instead of RD_p(1/hi)
#   H4  src/idmap.c, the smallest hull without the factor 2 of lcm(N, 2) for odd N
#   H5  src/idmap.c, the division multiplies by the hull of the coset instead of its inverse
#   H6  src/idpow.c, the limit of the content off by one (>= instead of >)
set -u
root=$(pwd)
[ -d include/adelefeld ] || { echo "run from the repository root" >&2; exit 2; }
for f in H1 H2 H3 H4 H5 H6; do
    d=build/faults3/$f
    rm -rf "$d"
    mkdir -p "$d"
    cp -r Makefile include src tests "$d/"
done
sed -i 's/            ulong e = 1;/            ulong e = 0;/' build/faults3/H1/src/idpow.c
sed -i 's/ball_from_ends(z, lo, hi, k % 2 != 0 ? s : 1, p);/ball_from_ends(z, lo, hi, s, p);/' \
    build/faults3/H2/src/idpow.c
sed -i 's/        arf_ui_div(t, 1, hi, p, ARF_RND_FLOOR);/        arf_ui_div(t, 1, lo, p, ARF_RND_FLOOR);/' \
    build/faults3/H3/src/idpow.c
sed -i 's/            fmpz_mul_2exp(H, H, 1);  /            fmpz_mul_2exp(H, H, 0);  /' build/faults3/H4/src/idmap.c
sed -i 's/    adf_ucoset_inv(w, &y->u); /    adf_ucoset_set(w, \&y->u); /' build/faults3/H5/src/idmap.c
sed -i 's|    return abs_k(k) > (ulong) ADF_IDELE_POW_BITS_MAX / b;|    return abs_k(k) >= (ulong) ADF_IDELE_POW_BITS_MAX / b;|' \
    build/faults3/H6/src/idpow.c
for f in H1 H2 H3 H4 H5 H6; do
    d=build/faults3/$f
    n=$(diff -r src "$d/src" | grep -c '^>')
    echo "== $f: $n changed line(s)"
    diff -r src "$d/src" | grep '^[<>]'
    if ! (cd "$d" && make -s -j2 build/test_idpow build/test_idmap > build.log 2>&1); then
        echo "   build failed"
        cat "$d/build.log"
        continue
    fi
    for t in test_idpow test_idmap; do
        (cd "$d" && timeout 300 "./build/$t" > "$t.out" 2>&1)
        rc=$?
        echo "   $t: exit $rc; $(tail -1 "$d/$t.out")"
        grep '^FAIL [A-Za-z_0-9]* (' "$d/$t.out" | sed 's/^/      /'
    done
done
cd "$root"
