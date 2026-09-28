#!/bin/sh
# lanes/m1-rat/redgreen.sh: the red phase of the lane, repeated against the final tests.
#
#     sh lanes/m1-rat/redgreen.sh
#
# For each entry of the list below the script writes one wrong implementation into src/, builds
# the library and the test program that must notice, runs that program, and restores the file.
# The output is the red log of the lane: the summary line of the program and its first failing
# test. A test that passes where it should fail is a hole in the suite, and the script says so.
#
# The wrong implementations are the ones a careless implementation would have: a missing
# canonicalisation, a domain check that is made after the write or not at all, a primality test
# that always succeeds, an order that puts the archimedean place last, a predicate that compares
# one field too few. Run from the repository root. At most two build jobs at a time.

set -u
root=$(pwd)
fail=0
scratch=build/m1-rat
mkdir -p "$scratch"

restore() { cp "$scratch/backup.c" "$1"; }

# wrong <file> <test> <sed program> <what the mutant gets wrong>
# (the function is not called "break": that is a shell keyword)
wrong() {
    f=$1; t=$2; prog=$3; what=$4
    cp "$f" "$scratch/backup.c"
    sed -i "$prog" "$f"
    if ! make -s -j2 "build/$t" > "$scratch/build.log" 2>&1; then
        echo "RED $t (build error, not an assertion): $what"
        sed -n '1,3p' "$scratch/build.log" | sed 's/^/     /'
        restore "$f"
        return
    fi
    if ./"build/$t" > "$scratch/out.txt" 2>&1; then
        echo "NOT RED $t: the tests pass with this wrong implementation: $what"
        fail=1
    else
        echo "RED $t: $what"
        echo "     $(tail -1 "$scratch/out.txt")"
        echo "     first failing check: $(grep '^FAIL ' "$scratch/out.txt" | head -1 | cut -c1-140)"
    fi
    restore "$f"
}

make -s -j2 all > /dev/null 2>&1

echo "== the exact constructors and the invariant of 5.1"
wrong src/rat.c test_rat 's|^    fmpq_set(x->q, q);$|    /* no copy at all */|' \
    "adf_rat_set_fmpq does not copy the two fields"
wrong src/rat.c test_rat 's|^    fmpq_canonicalise(x->q);$|    /* no canonicalisation */|' \
    "adf_rat_set_fmpq stores a non-canonical fmpq"

echo "== the statuses and the state of the outputs after one"
wrong src/rat.c test_rat 's|^    if (fmpq_is_zero(y->q))$|    if (0)|' \
    "adf_rat_div does not test the divisor for 0"
wrong src/rat.c test_rat 's|^    if (fmpz_is_zero(den))$|    if (0)|' \
    "adf_rat_set_fmpz2 does not test the denominator for 0"
wrong src/place.c test_place 's|^    if (!n_is_prime(p))$|    if (0)|' \
    "adf_place_prime accepts every word as a prime"

echo "== the predicates and the canonical order of places"
wrong src/rat.c test_rat \
    's|&& fmpz_equal(fmpq_denref(x->q), fmpq_denref(y->q));|\&\& 1;|' \
    "adf_rat_identical does not compare the denominators"
wrong src/place.c test_place \
    's|(v.opaque > w.opaque) - (v.opaque < w.opaque)|(w.opaque > v.opaque) - (w.opaque < v.opaque)|' \
    "adf_place_cmp reverses the canonical order"
wrong src/place.c test_place 's|^    return v.opaque;$|    (void) v; return 0;|' \
    "adf_place_prime_get returns 0 for every place"

echo "== the header is read-only, so the status values are checked by the build"
wrong src/status.c test_status 's|_Static_assert(ADF_LIMIT == 10|_Static_assert(ADF_LIMIT == 11|' \
    "a status value of the header is changed"

echo "== the green run"
make -s -j2 all > /dev/null 2>&1
for t in test_status test_place test_rat; do
    make -s -j2 "build/$t" > /dev/null 2>&1
    if ./"build/$t" > "$scratch/out.txt" 2>&1; then
        echo "GREEN $t: $(tail -1 "$scratch/out.txt")"
    else
        echo "NOT GREEN $t"; fail=1
    fi
done

if [ $fail -eq 0 ]; then
    echo "redgreen: every wrong implementation was caught"
else
    echo "redgreen: FAILED"
fi
exit $fail
