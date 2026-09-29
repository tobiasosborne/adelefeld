#!/bin/sh
# lanes/s3-slice3/mutants.sh: hand mutants of the code this lane added to src/resid.c, one at a time.
# The tree is restored after every mutant; the build directory is reused, so each mutant costs one
# compile of src/resid.c, one link of the tests and one run of build/test_resid_rest.
#
# Usage:  sh lanes/s3-slice3/mutants.sh            all the mutants below
#         sh lanes/s3-slice3/mutants.sh <n> ...   only the numbered ones
#
# The report is lanes/s3-slice3/mutants.out.

set -u
cd "$(dirname "$0")/../.." || exit 2

cp src/resid.c lanes/s3-slice3/resid.c.orig

run_one() {
    n=$1
    what=$2
    from=$3
    to=$4
    cp lanes/s3-slice3/resid.c.orig src/resid.c
    if ! python3 - "$from" "$to" <<'PY'
import sys
p = "src/resid.c"
s = open(p).read()
frm, to = sys.argv[1], sys.argv[2]
if s.count(frm) != 1:
    print("PATTERN NOT UNIQUE (%d): %s" % (s.count(frm), frm))
    sys.exit(1)
open(p, "w").write(s.replace(frm, to))
PY
    then
        echo "mutant $n: PATTERN ERROR, skipped: $what"
        return
    fi
    if ! make -s -j2 build/test_resid_rest > /dev/null 2>&1; then
        echo "mutant $n: $what -- does not build"
        return
    fi
    if timeout 600 ./build/test_resid_rest > "lanes/s3-slice3/mut-$n.out" 2>&1; then
        echo "mutant $n: $what -- SURVIVED (no test failed)"
    else
        echo "mutant $n: $what -- killed: $(grep -c '^FAIL' "lanes/s3-slice3/mut-$n.out") failing lines, $(tail -1 "lanes/s3-slice3/mut-$n.out")"
    fi
}

mut() {
    case " ${SELECT:-} " in
        *" $1 "*) run_one "$@" ;;
        *) echo "mutant $1: skipped by SELECT" ;;
    esac
}

mut 1 "adf_resid_set_rat: the test m < 1 removed" \
    "    fmpz_t g, c;

    if (fmpz_sgn(m) < 1)
        return ADF_DOMAIN;" "    fmpz_t g, c;

    if (0)
        return ADF_DOMAIN;"
mut 2 "adf_resid_set_rat: the test gcd(d, m) = 1 removed" \
    "    fmpz_gcd(g, fmpq_denref(q->q), m);
    if (!fmpz_is_one(g))" "    fmpz_gcd(g, fmpq_denref(q->q), m);
    if (0)"
mut 3 "adf_resid_set_rat: the numerator n forgotten in c = n d^-1" \
    "        fmpz_invmod(c, fmpq_denref(q->q), m); /* d^(-1) modulo m */
        fmpz_mul(c, c, fmpq_numref(q->q));" \
    "        fmpz_invmod(c, fmpq_denref(q->q), m); /* d^(-1) modulo m */"
mut 4 "adf_resid_contains_rat: the test gcd(d, m) = 1 removed" \
    "    fmpz_gcd(g, fmpq_denref(q->q), x->m);
    if (!fmpz_is_one(g))" "    fmpz_gcd(g, fmpq_denref(q->q), x->m);
    if (0)"
mut 5 "adf_resid_contains_rat: the divisibility test m | c d - n replaced by 1" \
    "        inside = fmpz_divisible(t, x->m) ? 1 : 0;" "        inside = 1;"
mut 6 "adf_resid_set_fball_forget: the test H = 0 removed" \
    "    if (fmpz_sgn(H) < 1)                    /* exact: no radius, no modulus */
        goto done;" "    if (0)                                 /* exact: no radius, no modulus */
        goto done;"
mut 7 "adf_resid_set_fball_forget: the test gcd(d, H) = 1 removed" \
    "    fmpz_gcd(g, d, H);
    if (!fmpz_is_one(g))
        goto done;                           /* d is not invertible modulo H */" \
    "    fmpz_gcd(g, d, H);
    if (0)
        goto done;                           /* d is not invertible modulo H */"
mut 8 "adf_resid_set_fball_forget: the centre A forgotten in c = A d^-1" \
    "        fmpz_invmod(c, d, H);                /* d^(-1) modulo H */
        fmpz_mul(c, c, A);" \
    "        fmpz_invmod(c, d, H);                /* d^(-1) modulo H */"
mut 9 "adf_recon_cert_is_canonical: the test on the kind removed" \
    "    if (cert->kind != 0 && cert->kind != 1)
        return 0;" "    if (0)
        return 0;"
mut 10 "adf_recon_cert_identical: the comparison of T removed" \
    "    return x->kind == y->kind && fmpz_equal(x->Rp, y->Rp) && fmpz_equal(x->Tp, y->Tp)
           && fmpz_equal(x->R, y->R) && fmpz_equal(x->T, y->T);" \
    "    return x->kind == y->kind && fmpz_equal(x->Rp, y->Rp) && fmpz_equal(x->Tp, y->Tp)
           && fmpz_equal(x->R, y->R);"
mut 11 "resid_solve: the pair (c mod m, 1) of Proposition 1.6 (a) not reported as found" \
    "        fmpz_one(fd);
        *nfound = 1;
        return ADF_NOT_UNIQUE;" "        fmpz_one(fd);
        *nfound = 0;
        return ADF_NOT_UNIQUE;"
mut 12 "adf_resid_reconstruct_first: the count 2 of a NOT_UNIQUE replaced by 1" \
    "    else if (status == ADF_NOT_UNIQUE)
    {
        *count = 2;" "    else if (status == ADF_NOT_UNIQUE)
    {
        *count = 1;"
mut 13 "adf_resid_reconstruct_first: a cut search with one solution reported as cut without it" \
    "    else if (status == ADF_NOT_DETERMINED && nfound)" \
    "    else if (status == ADF_NOT_DETERMINED)"
mut 14 "adf_resid_verify_result: the test that q is a solution removed" \
    "        if (!is_solution(x, A, B, fmpq_numref(q->q), fmpq_denref(q->q)))
            goto done;                             /* q must be a solution of Definition 1.1 */" \
    "        if (0 && !is_solution(x, A, B, fmpq_numref(q->q), fmpq_denref(q->q)))
            goto done;"
mut 15 "adf_resid_verify_result: the condition X > ell of Proposition 1.7 (3) removed" \
    "    if (fmpz_cmp(w, x->m) < 0 || fmpz_cmpabs(T, B) > 0 || fmpz_cmp_si(X, ell) <= 0)
        goto done;" \
    "    if (fmpz_cmp(w, x->m) < 0 || fmpz_cmpabs(T, B) > 0)
        goto done;"
mut 16 "adf_resid_verify_result: the certificate check of Proposition 1.11 (2) skipped" \
    "    if (!adf_recon_cert_check(cert, x, A))
        goto done;" "    if (0)
        goto done;"
mut 17 "adf_resid_verify_result: a NOT_UNIQUE claim accepted without a search" \
    "        if (fmpz_cmp(A, x->m) >= 0)
            ok = 1;" "        if (1)
            ok = 1;"
mut 18 "adf_resid_verify_result: the empty box answered NO_SOLUTION whatever the claim" \
    "    if (fmpz_sgn(A) < 0 || fmpz_sgn(B) < 1)
        return status == ADF_NO_SOLUTION;          /* the box is empty (S-D5), the only answer */" \
    "    if (fmpz_sgn(A) < 0 || fmpz_sgn(B) < 1)
        return 1;"

cp lanes/s3-slice3/resid.c.orig src/resid.c
make -s -j2 build/test_resid_rest > /dev/null 2>&1
echo "the tree is restored"
