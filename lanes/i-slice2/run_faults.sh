#!/bin/sh
# lanes/i-slice2/run_faults.sh: do the tests bite? (brief item 5). Four faults, each in its own scratch copy of
# the tree under build/faults2/<F>/ (Makefile, include, src, tests); the two test programs of the slice are
# built and run there. Run from the repository root:  sh lanes/i-slice2/run_faults.sh
#
#   G1  src/idclass.c, the class map without the sign on the unit (u' = u; the map Psi of ideles.md P15.3)
#   G2  src/idele.c, the norm multiplies by r instead of dividing (a and b of Statement F exchanged)
#   G3  src/idele.c, the valuation ignores the denominator of r (v = v_p(n))
#   G4  src/idele.c, Statement F: the lower end RD_p(l a / b) rounded UP (ARF_RND_CEIL)
set -u
root=$(pwd)
[ -d include/adelefeld ] || { echo "run from the repository root" >&2; exit 2; }
for f in G1 G2 G3 G4; do
    d=build/faults2/$f
    rm -rf "$d"
    mkdir -p "$d"
    cp -r Makefile include src tests "$d/"
done
sed -i 's/if (arf_sgn(arb_midref(x->inf)) < 0) /if (arf_sgn(arb_midref(x->inf)) < 0 \&\& 0) /' \
    build/faults2/G1/src/idclass.c
g2a='lx, hx, fmpq_denref(x->r), fmpq_numref(x->r), p);'
g2b='lx, hx, fmpq_numref(x->r), fmpq_denref(x->r), p);'
sed -i "s/$g2a/$g2b/" build/faults2/G2/src/idele.c
sed -i 's/    \*v = a - b; /    *v = a - 0 * b; /' build/faults2/G3/src/idele.c
sed -i 's/arf_div_fmpz(lo, t, b, p, ARF_RND_FLOOR);/arf_div_fmpz(lo, t, b, p, ARF_RND_CEIL);/' \
    build/faults2/G4/src/idele.c
for f in G1 G2 G3 G4; do
    d=build/faults2/$f
    n=$(diff -r src "$d/src" | grep -c '^>')
    echo "== $f: $n changed line(s)"
    diff -r src "$d/src" | grep '^[<>]'
    if ! (cd "$d" && make -s -j2 build/test_idclass build/test_idele_maps > build.log 2>&1); then
        echo "   build failed"
        cat "$d/build.log"
        continue
    fi
    for t in test_idclass test_idele_maps; do
        (cd "$d" && timeout 300 "./build/$t" > "$t.out" 2>&1)
        rc=$?
        echo "   $t: exit $rc; $(tail -1 "$d/$t.out")"
        grep '^FAIL [A-Za-z_0-9]* (' "$d/$t.out" | sed 's/^/      /'
    done
done
cd "$root"
