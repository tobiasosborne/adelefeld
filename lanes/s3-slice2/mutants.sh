#!/bin/sh
# Scratch-copy mutation of src/resid.c, one mutant at a time; run from the repository root.
# Usage: sh lanes/s3-slice2/mutants.sh   (prints, for each mutant, the failing tests of build/mut/)
# Each mutant is built in build/mut/ and both test programs are run; the first line of each failing check is
# shown, so that "which test fails" can be read off. A mutant that no test kills is printed as SURVIVED.
set -u
run() {
    name=$1
    shift
    rm -rf build/mut
    mkdir -p build/mut
    cp -r include src tests Makefile build/mut/
    python3 lanes/s3-slice2/mutate_one.py build/mut/src/resid.c "$@" || { echo "MUTANT $name: pattern not found"; return; }
    if ! (cd build/mut && make -s -j2 build/test_resid build/test_resid_full >/dev/null 2>&1); then
        echo "MUTANT $name: build failed"
        return
    fi
    killed=0
    for t in test_resid test_resid_full; do
        (cd build/mut && timeout 170 ./build/$t > out_$t.txt 2>&1)
        rc=$?
        nf=$(grep -c '^FAIL' build/mut/out_$t.txt)
        echo "MUTANT $name: $t exit status $rc; $nf failed checks"
        # the tests that failed, once each
        grep '^FAIL .*(tests' build/mut/out_$t.txt | sed 's/^FAIL \([a-z_0-9]*\) (tests.*/   failing test: \1/' | sort -u | head -6
        if [ "$rc" -ne 0 ]; then killed=1; fi
    done
    if [ "$killed" -eq 0 ]; then echo "MUTANT $name: SURVIVED"; fi
}
run "A >= m -> A > m" "if (fmpz_cmp(A, x->m) >= 0)" "if (fmpz_cmp(A, x->m) > 0)"
run "X > ell -> X >= ell (complete X < ell)" "int complete = fmpz_cmp_si(X, ell) <= 0;" "int complete = fmpz_cmp_si(X, ell) < 0;"
run "two found -> three found" "if (found == 0)" "if (found < 2)" "found = 1;" "found++;"
run "gcd test of a lattice point removed" "if (!fmpz_is_one(g))
                continue;" "if (0)
                continue;"
run "gcd(R, T) test of 1.6 (c) removed" "if (!fmpz_is_one(w))                        /* gcd > 1 */" "if (0)"
run "lower bound of y + 1" "fmpz_cdiv_q(ylo, w, Rp);                    /* ceil((x R - A)/R') */" "fmpz_cdiv_q(ylo, w, Rp); fmpz_add_ui(ylo, ylo, 1);"
run "lower bound of y - 1" "fmpz_cdiv_q(ylo, w, Rp);                    /* ceil((x R - A)/R') */" "fmpz_cdiv_q(ylo, w, Rp); fmpz_sub_ui(ylo, ylo, 1);"
run "upper bound of y + 1" "fmpz_fdiv_q(yhi, w, Rp);                    /* floor((x R + A)/R') */" "fmpz_fdiv_q(yhi, w, Rp); fmpz_add_ui(yhi, yhi, 1);"
run "upper bound of y - 1" "fmpz_fdiv_q(yhi, w, Rp);                    /* floor((x R + A)/R') */" "fmpz_fdiv_q(yhi, w, Rp); fmpz_sub_ui(yhi, yhi, 1);"
run "rounds run to ell + 1" "slong rounds = complete ? fmpz_get_si(X) : ell; /* min(X, ell) */" "slong rounds = complete ? fmpz_get_si(X) : ell + 1;"
run "d > B -> d >= B" "if (fmpz_cmp(d, B) > 0)
                break;" "if (fmpz_cmp(d, B) >= 0)
                break;"
run "sign sigma dropped in the search" "if (sigma < 0)
                fmpz_neg(n, n);" "if (sigma < -1)
                fmpz_neg(n, n);"
run "X compared as its low word (fmpz_get_ui) instead of as an fmpz" "int complete = fmpz_cmp_si(X, ell) <= 0;" "int complete = fmpz_get_ui(X) <= (ulong) ell;"
