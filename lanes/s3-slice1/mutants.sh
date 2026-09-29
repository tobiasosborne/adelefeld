#!/bin/sh
# Scratch-copy mutation of src/resid.c, one mutant at a time; run from the repository root.
# Usage: sh lanes/s3-slice1/mutants.sh   (prints, for each mutant, the failing tests of build/mut/)
set -u
run() {
    name=$1; from=$2; to=$3
    rm -rf build/mut
    mkdir -p build/mut
    cp -r include src tests Makefile build/mut/
    python3 lanes/s3-slice1/mutate_one.py build/mut/src/resid.c "$from" "$to" || { echo "MUTANT $name: pattern not found"; return; }
    if ! (cd build/mut && make -s -j2 build/test_resid >/dev/null 2>&1); then
        echo "MUTANT $name: build failed"
        return
    fi
    (cd build/mut && timeout 170 ./build/test_resid > out.txt 2>&1; rc=$?
     echo "MUTANT $name: exit status $rc; $(grep -c '^FAIL' out.txt) failed checks"
     grep '^FAIL .*(tests' out.txt | sed 's/^/   /' | head -8
     tail -2 out.txt | sed 's/^/   /')
}
run "r <= A -> r < A" "while (fmpz_cmp(r1, A) > 0)" "while (fmpz_cmp(r1, A) >= 0)"
run "abs(T) > B -> abs(T) >= B" "if (fmpz_cmpabs(t1, B) > 0)" "if (fmpz_cmpabs(t1, B) >= 0)"
run "2AB < m -> 2AB <= m" "if (fmpz_cmp(w, x->m) >= 0)" "if (fmpz_cmp(w, x->m) > 0)"
run "gcd test removed" "if (!fmpz_is_one(w))" "if (0)"
run "sigma dropped" "if (fmpz_sgn(t1) < 0)" "if (0)"
