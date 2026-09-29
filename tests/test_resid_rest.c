/* tests/test_resid_rest.c: the rest of include/adelefeld/resid.h (slice 3 of milestone S, lane
   s3-slice3; docs/api-s.md section 2; docs/proofs/solvers.md Lemma 1.2, Proposition 1.10,
   Proposition 1.11, Algorithm R).

   What is tested here, and the oracle of each part (the order of independence):

   1. the life cycle (conventions 2.3): set, swap, identical, is_canonical of adf_resid and
      adf_recon_cert, against the init values and against values written by hand into the
      fmpz members (the fields are public, so a non-canonical value can be made);
   2. adf_resid_set_rat and adf_resid_contains_rat (solvers L1.2, P1.10(1)): for every m <= 40 and
      every reduced n/d with |n|, d <= 12 the status, the residue and the predicate against the
      definition of P(m, c) of P1.10(1) (gcd(d, m) = 1 and n = c d modulo m);
   3. adf_resid_set_fball_forget (solvers P1.10(4)): every rational (A + H k)/d of the ball with
      -20 <= k <= 20 is in the result by the definition of P(m, c); DOMAIN for H = 0 and for
      gcd(d, H) > 1 with x untouched; the example of note 1 of docs/api-s.md section 2 (5 + 6 Zhat);
      the same ball global and local;
   4. adf_resid_reconstruct_first (S-D4): the grid m <= 24, c in [0, m), A from 0 to 2 m, B in
      {1, 2, 3, 5, m, 2 m}, the limits -1, 0, 1, 2, 5 and a large one: status, q and count against
      the enumeration of Definition 1.1 written in this file, and against
      tests/ref/vectors/s3-slice3/recon_first.jsonl (written by lanes/s3-slice3/gen_vectors.py from
      recon_first of proto/solvers_checks.py, which the script imports);
   5. adf_resid_verify_result (solvers P1.11): every result of adf_resid_reconstruct is accepted, and
      the changed claims (another status, q changed in numerator, in denominator, in sign, a
      certificate with one entry changed, the kind flipped) are refused or are true by the
      enumeration;
   6. the layout of the two types (pinned here, since docs/api-s.md section 1 gives them). */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <flint/flint.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld/fball.h>
#include <adelefeld/modctx.h>
#include <adelefeld/rat.h>
#include <adelefeld/recon.h>
#include <adelefeld/resid.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- helpers ---- */

/* A helper of this file that no test of the moment uses yet is not a warning (as in
   tests/test_fball_local.c). */

#if defined(__GNUC__) || defined(__clang__)
#define REST_UNUSED __attribute__((unused))
#else
#define REST_UNUSED
#endif

static slong REST_UNUSED
gcd_sl(slong a, slong b)
{
    if (a < 0)
        a = -a;
    if (b < 0)
        b = -b;
    while (b != 0)
    {
        slong t = a % b;
        a = b;
        b = t;
    }
    return a;
}

/* x = P(m, c) with c reduced, the long way (fmpz_mod). Returns the status. */
static int REST_UNUSED
mkresid(adf_resid_t x, slong c, slong m)
{
    fmpz_t fc, fm;
    int st;

    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fm, m);
    st = adf_resid_set_fmpz2(x, fc, fm);
    fmpz_clear(fc);
    fmpz_clear(fm);
    return st;
}

/* The pair of x as machine integers (the values of this file are small). */
static void REST_UNUSED
resid_pair(const adf_resid_t x, slong * c, slong * m)
{
    fmpz_t fc, fm;

    fmpz_init(fc);
    fmpz_init(fm);
    adf_resid_get_fmpz2(fc, fm, x);
    *c = fmpz_get_si(fc);
    *m = fmpz_get_si(fm);
    fmpz_clear(fc);
    fmpz_clear(fm);
}

/* The numerator and the denominator of q as machine integers. */
static void REST_UNUSED
rat_pair(const adf_rat_t q, slong * n, slong * d)
{
    fmpq_t t;

    fmpq_init(t);
    adf_rat_get_fmpq(t, q);
    *n = fmpz_get_si(fmpq_numref(t));
    *d = fmpz_get_si(fmpq_denref(t));
    fmpq_clear(t);
}

/* cert = the quadruple (Rp, Tp, R, T) and kind, as machine integers (kind is -1 for a kind that
   is neither 0 nor 1, which no function of the library produces but a caller may write). */
static void REST_UNUSED
cert_fields(const adf_recon_cert_t cert, slong * Rp, slong * Tp, slong * R, slong * T, slong * kind)
{
    *Rp = fmpz_get_si(cert->Rp);
    *Tp = fmpz_get_si(cert->Tp);
    *R = fmpz_get_si(cert->R);
    *T = fmpz_get_si(cert->T);
    *kind = cert->kind;
}

static void REST_UNUSED
cert_set_si(adf_recon_cert_t cert, slong Rp, slong Tp, slong R, slong T, slong kind)
{
    fmpz_set_si(cert->Rp, Rp);
    fmpz_set_si(cert->Tp, Tp);
    fmpz_set_si(cert->R, R);
    fmpz_set_si(cert->T, T);
    cert->kind = (int) kind;
}

/* ---- 1. the life cycle ---- */

ADF_TEST(life_cycle_init_values)
{
    adf_resid_t x;
    adf_recon_cert_t cert;
    slong c, m, Rp, Tp, R, T, kind;

    adf_resid_init(x);
    adf_recon_cert_init(cert);

    /* the init value of adf_resid is (0, 1): every rational (P(m, c) with m = 1 is Q, P1.10(1)) */
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 0);
    ADF_CHECK(m == 1);
    ADF_CHECK(adf_resid_is_canonical(x) == 1);

    /* the init value of adf_recon_cert is kind = 0 with the four integers 0 */
    cert_fields(cert, &Rp, &Tp, &R, &T, &kind);
    ADF_CHECK(Rp == 0);
    ADF_CHECK(Tp == 0);
    ADF_CHECK(R == 0);
    ADF_CHECK(T == 0);
    ADF_CHECK(kind == 0);
    ADF_CHECK(adf_recon_cert_is_canonical(cert) == 1);

    adf_recon_cert_clear(cert);
    adf_resid_clear(x);
}

ADF_TEST(life_cycle_resid_set_swap_identical)
{
    adf_resid_t x, y;
    slong c, m, c2, m2;

    adf_resid_init(x);
    adf_resid_init(y);

    ADF_CHECK(mkresid(x, 3, 7) == ADF_OK);
    ADF_CHECK(mkresid(y, 5, 7) == ADF_OK);
    resid_pair(x, &c, &m);
    resid_pair(y, &c2, &m2);
    ADF_CHECK(c == 3 && m == 7);
    ADF_CHECK(c2 == 5 && m2 == 7);
    ADF_CHECK(adf_resid_identical(x, y) == 0);

    /* set: y becomes x */
    adf_resid_set(y, x);
    ADF_CHECK(adf_resid_identical(x, y) == 1);
    resid_pair(y, &c2, &m2);
    ADF_CHECK(c2 == 3 && m2 == 7);
    ADF_CHECK(adf_resid_is_canonical(y) == 1);

    /* y may be x */
    adf_resid_set(x, x);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 3 && m == 7);
    ADF_CHECK(adf_resid_identical(x, y) == 1);

    /* swap: the contents travel, nothing is allocated */
    ADF_CHECK(mkresid(x, 1, 2) == ADF_OK);
    ADF_CHECK(adf_resid_identical(x, y) == 0);
    adf_resid_swap(x, y);
    ADF_CHECK(adf_resid_identical(x, y) == 0);
    resid_pair(x, &c, &m);
    resid_pair(y, &c2, &m2);
    ADF_CHECK(c == 3 && m == 7);
    ADF_CHECK(c2 == 1 && m2 == 2);
    adf_resid_swap(x, y);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 1 && m == 2);
    ADF_CHECK(adf_resid_is_canonical(x) == 1);
    ADF_CHECK(adf_resid_is_canonical(y) == 1);

    /* identical compares the fields, not the sets: c = 3 and c = -3 with m = 7 are the same set
       (P1.10(1): the set depends on c only through c modulo m) but not the same value. The second
       value is written by hand into the members, since adf_resid_set_fmpz2 reduces c into [0, m) and
       never leaves a non-canonical residue. */
    ADF_CHECK(mkresid(x, 3, 7) == ADF_OK);
    ADF_CHECK(mkresid(y, -3, 7) == ADF_OK);
    resid_pair(y, &c2, &m2);
    ADF_CHECK(c2 == 4 && m2 == 7);           /* -3 reduced into [0, 7) */
    fmpz_set_si(y->c, -3);
    ADF_CHECK(adf_resid_identical(x, y) == 0);
    ADF_CHECK(adf_resid_is_canonical(y) == 0);
    /* a different modulus with a non-canonical c is a different value */
    fmpz_set_si(y->c, 3);
    fmpz_set_si(y->m, 8);
    ADF_CHECK(adf_resid_identical(x, y) == 0);
    /* m = 0: not a residue, not canonical */
    fmpz_set_si(y->m, 0);
    ADF_CHECK(adf_resid_is_canonical(y) == 0);
    /* m < 0 */
    fmpz_set_si(y->c, 0);
    fmpz_set_si(y->m, -5);
    ADF_CHECK(adf_resid_is_canonical(y) == 0);

    adf_resid_clear(y);
    adf_resid_clear(x);
}

ADF_TEST(life_cycle_recon_cert_set_swap_identical)
{
    adf_recon_cert_t cert, cert2;
    slong Rp, Tp, R, T, kind, Rp2, Tp2, R2, T2, kind2;

    adf_recon_cert_init(cert);
    adf_recon_cert_init(cert2);

    cert_set_si(cert, 7, -2, 3, 1, 1);
    cert_set_si(cert2, 7, -2, 3, 1, 1);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 1);

    /* one field changed */
    cert_set_si(cert2, 7, -2, 3, -1, 1);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 0);
    cert_set_si(cert2, 8, -2, 3, 1, 1);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 0);
    cert_set_si(cert2, 7, 2, 3, 1, 1);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 0);
    cert_set_si(cert2, 7, -2, 4, 1, 1);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 0);
    /* the kind alone */
    cert_set_si(cert2, 7, -2, 3, 1, 0);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 0);

    /* set and self-set */
    cert_set_si(cert2, 0, 0, 0, 0, 0);
    adf_recon_cert_set(cert2, cert);
    ADF_CHECK(adf_recon_cert_identical(cert, cert2) == 1);
    adf_recon_cert_set(cert, cert);
    cert_fields(cert, &Rp, &Tp, &R, &T, &kind);
    ADF_CHECK(Rp == 7 && Tp == -2 && R == 3 && T == 1 && kind == 1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert) == 1);

    /* swap */
    cert_set_si(cert2, 0, 0, 0, 0, 0);
    adf_recon_cert_swap(cert, cert2);
    cert_fields(cert, &Rp, &Tp, &R, &T, &kind);
    cert_fields(cert2, &Rp2, &Tp2, &R2, &T2, &kind2);
    ADF_CHECK(Rp == 0 && Tp == 0 && R == 0 && T == 0 && kind == 0);
    ADF_CHECK(Rp2 == 7 && Tp2 == -2 && R2 == 3 && T2 == 1 && kind2 == 1);
    adf_recon_cert_swap(cert, cert2);
    cert_fields(cert, &Rp, &Tp, &R, &T, &kind);
    ADF_CHECK(Rp == 7 && Tp == -2 && R == 3 && T == 1 && kind == 1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert) == 1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 1);

    /* is_canonical: kind 0 with a non-zero field, and a kind outside {0, 1} */
    cert_set_si(cert2, 0, 0, 0, 0, 0);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 1);
    fmpz_set_si(cert2->T, 5);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 0);
    fmpz_set_si(cert2->T, 0);
    fmpz_set_si(cert2->R, -1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 0);
    fmpz_set_si(cert2->R, 0);
    fmpz_set_si(cert2->Rp, 1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 0);
    fmpz_set_si(cert2->Rp, 0);
    fmpz_set_si(cert2->Tp, 1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 0);
    fmpz_set_si(cert2->Tp, 0);
    cert2->kind = 2;
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 0);
    cert2->kind = -1;
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 0);
    cert2->kind = 1;
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 1);
    /* a quadruple that does not satisfy (C1) to (C4) is canonical data; soundness is the check */
    cert_set_si(cert2, 3, 3, 3, 3, 1);
    ADF_CHECK(adf_recon_cert_is_canonical(cert2) == 1);

    adf_recon_cert_clear(cert2);
    adf_recon_cert_clear(cert);
}

ADF_TEST(layouts_are_pinned)
{
    /* docs/api-s.md section 1 (64 bit): adf_resid 16 bytes, c at 0, m at 8; adf_recon_cert 40 bytes,
       Rp at 0, Tp at 8, R at 16, T at 24, kind at 32. */
    adf_resid_t x;
    adf_recon_cert_t cert;

    ADF_CHECK(adf_sizeof_resid() == 16);
    ADF_CHECK(adf_alignof_resid() == 8);
    ADF_CHECK(adf_sizeof_recon_cert() == 40);
    ADF_CHECK(adf_alignof_recon_cert() == 8);
    ADF_CHECK(offsetof(adf_resid_struct, c) == 0);
    ADF_CHECK(offsetof(adf_resid_struct, m) == 8);
    ADF_CHECK(offsetof(adf_recon_cert_struct, Rp) == 0);
    ADF_CHECK(offsetof(adf_recon_cert_struct, Tp) == 8);
    ADF_CHECK(offsetof(adf_recon_cert_struct, R) == 16);
    ADF_CHECK(offsetof(adf_recon_cert_struct, T) == 24);
    ADF_CHECK(offsetof(adf_recon_cert_struct, kind) == 32);
    ADF_CHECK(sizeof(adf_resid_struct) == adf_sizeof_resid());
    ADF_CHECK(sizeof(adf_recon_cert_struct) == adf_sizeof_recon_cert());
    adf_recon_cert_init(cert);
    adf_resid_init(x);
    adf_recon_cert_clear(cert);
    adf_resid_clear(x);
}

/* ---- 2. adf_resid_set_rat and adf_resid_contains_rat ---- */

/* q = n/d as an adf_rat, reduced by adf_rat_set_fmpz2. Returns the status of the constructor. */
static int REST_UNUSED
mkrat(adf_rat_t q, slong n, slong d)
{
    fmpz_t fn, fd;
    int st;

    fmpz_init_set_si(fn, n);
    fmpz_init_set_si(fd, d);
    st = adf_rat_set_fmpz2(q, fn, fd);
    fmpz_clear(fn);
    fmpz_clear(fd);
    return st;
}

ADF_TEST(set_rat_and_contains_rat_against_the_definition)
{
    adf_rat_t q;
    adf_resid_t x, y;
    fmpz_t fm;
    slong m, d, n, c, cc, cm, cases, dom, ok, cont, notcont;
    int st;

    adf_rat_init(q);
    adf_resid_init(x);
    adf_resid_init(y);
    fmpz_init(fm);
    cases = dom = ok = cont = notcont = 0;

    for (m = 1; m <= 40; m++)
    {
        fmpz_set_si(fm, m);
        for (d = 1; d <= 12; d++)
        {
            for (n = -12; n <= 12; n++)
            {
                int coprime, expect_ok;

                if (gcd_sl(n, d) != 1)
                    continue;
                cases++;
                coprime = (gcd_sl(d, m) == 1);
                expect_ok = coprime;
                if (mkrat(q, n, d) != ADF_OK)
                {
                    ADF_CHECK_MSG(0, "adf_rat_set_fmpz2 refused %ld/%ld", n, d);
                    continue;
                }
                /* a known value in x, to see that a DOMAIN leaves it untouched */
                ADF_CHECK(mkresid(x, 4, 9) == ADF_OK);
                st = adf_resid_set_rat(x, q, fm);
                ADF_CHECK_MSG(st == (expect_ok ? ADF_OK : ADF_DOMAIN),
                              "set_rat(%ld/%ld, m = %ld) gave %d, expected %s", n, d, m, st,
                              expect_ok ? "ADF_OK" : "ADF_DOMAIN");
                if (st == ADF_DOMAIN)
                {
                    dom++;
                    resid_pair(x, &c, &cm);
                    ADF_CHECK_MSG(c == 4 && cm == 9, "set_rat wrote x on DOMAIN (%ld/%ld, m = %ld)", n, d, m);
                }
                else
                {
                    ok++;
                    ADF_CHECK(adf_resid_is_canonical(x) == 1);
                    resid_pair(x, &c, &cm);
                    ADF_CHECK_MSG(cm == m, "set_rat(%ld/%ld, m = %ld) gave the modulus %ld", n, d, m, cm);
                    /* c d = n modulo m: the definition of the residue the function claims to build */
                    ADF_CHECK_MSG(((c * d - n) % m) == 0,
                                  "set_rat(%ld/%ld, m = %ld) gave c = %ld, and c d - n = %ld", n, d, m, c,
                                  c * d - n);
                    /* and q is in the set it names */
                    ADF_CHECK_MSG(adf_resid_contains_rat(x, q) == 1,
                                  "set_rat(%ld/%ld, m = %ld) gave c = %ld, which does not contain q", n, d, m,
                                  c);
                }
                /* the predicate, for every c in [0, m): q in P(m, c) exactly when gcd(d, m) = 1 and
                   n = c d modulo m (P1.10 (1)) */
                for (cc = 0; cc < m; cc++)
                {
                    int inside = coprime && ((cc * d - n) % m == 0);
                    int got;

                    ADF_CHECK(mkresid(y, cc, m) == ADF_OK);
                    got = adf_resid_contains_rat(y, q);
                    ADF_CHECK_MSG(got == inside, "contains_rat(P(%ld, %ld), %ld/%ld) = %d, expected %d", m, cc,
                                  n, d, got, inside);
                    if (got)
                        cont++;
                    else
                        notcont++;
                }
            }
        }
    }
    printf("   set_rat and contains_rat: %ld reduced fractions with |n|, d <= 12 over every m <= 40; "
           "set_rat OK %ld, DOMAIN %ld; contains_rat inside %ld, outside %ld\n", cases, ok, dom, cont,
           notcont);
    /* the grid is not empty on either side */
    ADF_CHECK(cases > 5000);
    ADF_CHECK(ok > 1000);
    ADF_CHECK(dom > 100);
    ADF_CHECK(cont > 1000);
    ADF_CHECK(notcont > 1000);
    /* the number of c in [0, m) that contain a fixed q is the number of the solutions of c d = n
       modulo m, that is 1 when gcd(d, m) = 1 and 0 otherwise: 4/3 with m = 10, 3 c = 4 gives c = 8 */
    {
        int hits = 0, cc, hit_c = -1;

        ADF_CHECK(mkrat(q, 4, 3) == ADF_OK);
        for (cc = 0; cc < 10; cc++)
        {
            int in2;

            ADF_CHECK(mkresid(y, cc, 10) == ADF_OK);
            in2 = adf_resid_contains_rat(y, q);
            ADF_CHECK_MSG(in2 == (cc == 8), "contains_rat(P(10, %d), 4/3) = %d, expected %d", cc, in2,
                          cc == 8);
            hits += in2;
            if (in2)
                hit_c = cc;
        }
        ADF_CHECK(hits == 1);
        ADF_CHECK(hit_c == 8);
        /* a rational whose denominator is not prime to m is in no P(m, c) at all */
        ADF_CHECK(mkrat(q, 1, 2) == ADF_OK);
        for (cc = 0; cc < 10; cc++)
        {
            ADF_CHECK(mkresid(y, cc, 10) == ADF_OK);
            ADF_CHECK(adf_resid_contains_rat(y, q) == 0);
        }
    }

    fmpz_clear(fm);
    adf_resid_clear(y);
    adf_resid_clear(x);
    adf_rat_clear(q);
}

ADF_TEST(set_rat_statuses_and_shapes)
{
    adf_rat_t q;
    adf_resid_t x;
    fmpz_t fm;
    slong c, m, j;

    adf_rat_init(q);
    adf_resid_init(x);
    fmpz_init(fm);

    /* m = 0 and m < 0: DOMAIN, x untouched. The rational has denominator 1 here, so that the case is
       not caught by gcd(d, m) > 1 and the function really has to test m itself. */
    ADF_CHECK(mkrat(q, 1, 2) == ADF_OK);
    ADF_CHECK(mkresid(x, 1, 3) == ADF_OK);
    fmpz_zero(fm);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_DOMAIN);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 1 && m == 3);
    ADF_CHECK(mkrat(q, 3, 1) == ADF_OK);
    fmpz_zero(fm);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_DOMAIN);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 1 && m == 3);
    fmpz_one(fm);
    fmpz_neg(fm, fm);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_DOMAIN);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 1 && m == 3);
    fmpz_set_si(fm, -4);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_DOMAIN);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 1 && m == 3);

    /* m = 1: P(1, 0) = Q, every rational is in it (P1.10 (1)) */
    fmpz_one(fm);
    for (j = -3; j <= 3; j++)
    {
        ADF_CHECK(mkrat(q, j, 5) == ADF_OK);
        ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_OK);
        resid_pair(x, &c, &m);
        ADF_CHECK(c == 0 && m == 1);
        ADF_CHECK(adf_resid_contains_rat(x, q) == 1);
    }

    /* gcd(d, m) > 1: DOMAIN although the numerator is 0 and the fraction reduced (0/1 has d = 1, so
       it is never refused; 1/2 with m = 4 is) */
    fmpz_set_si(fm, 4);
    ADF_CHECK(mkrat(q, 0, 1) == ADF_OK);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_OK);
    ADF_CHECK(adf_resid_contains_rat(x, q) == 1);
    ADF_CHECK(mkrat(q, 1, 2) == ADF_OK);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_DOMAIN);
    /* 1/2 = 2/4 is not reduced and is not an adf_rat; 2/4 would be c = 1 of P(4, 1) after reduction */
    ADF_CHECK(mkrat(q, 1, 4) == ADF_OK);
    ADF_CHECK(adf_resid_set_rat(x, q, fm) == ADF_DOMAIN);
    resid_pair(x, &c, &m);
    ADF_CHECK_MSG(c == 0 && m == 4, "after two DOMAIN calls x is P(%ld, %ld), expected P(4, 0)", m, c);
    /* a large modulus and a large numerator, above a word */
    {
        fmpz_t big, bn;

        fmpz_init(big);
        fmpz_init(bn);
        fmpz_set_str(big, "123456789012345678901234567890123456789012345678901234567890", 10);
        fmpz_mul_2exp(big, big, 200);
        fmpz_add_ui(big, big, 1);            /* odd, so invertible modulo the large modulus below */
        fmpz_set_str(bn, "987654321098765432109876543210987654321098765432109876543211", 10);
        if (mkrat(q, 0, 1) == ADF_OK)
        {
            ADF_CHECK(adf_resid_set_rat(x, q, big) == ADF_OK);
            resid_pair(x, &c, &m);
            ADF_CHECK(c == 0);
        }
        /* the same with a real fraction: q = bn / big is in P(big, c) for the c it computes */
        fmpq_t f;
        adf_rat_t qq;
        fmpz_t t;

        fmpq_init(f);
        adf_rat_init(qq);
        fmpz_init(t);
        fmpz_set(fmpq_numref(f), bn);
        fmpz_set(fmpq_denref(f), big);
        if (adf_rat_set_fmpq(qq, f) == ADF_OK)
        {
            int st = adf_resid_set_rat(x, qq, big);

            ADF_CHECK(st == ADF_OK || st == ADF_DOMAIN);
            if (st == ADF_OK)
            {
                fmpz_t cc, mm;
                fmpz_init(cc);
                fmpz_init(mm);
                adf_resid_get_fmpz2(cc, mm, x);
                ADF_CHECK(fmpz_equal(mm, big));
                /* c d = n modulo m with d the reduced denominator of q */
                fmpz_mul(t, cc, fmpq_denref(f));
                fmpz_sub(t, t, fmpq_numref(f));
                ADF_CHECK_MSG(fmpz_divisible(t, big), "c d - n = %s is not divisible by m", "the large m");
                ADF_CHECK(adf_resid_contains_rat(x, qq) == 1);
                fmpz_clear(cc);
                fmpz_clear(mm);
            }
        }
        fmpz_clear(t);
        adf_rat_clear(qq);
        fmpq_clear(f);
        fmpz_clear(bn);
        fmpz_clear(big);
    }

    fmpz_clear(fm);
    adf_resid_clear(x);
    adf_rat_clear(q);
}

/* ---- 3. adf_resid_set_fball_forget ---- */

/* The local value of ctx of the set (A + K Zhat)/d, made through the public interface (as
   tests/test_fball_local.c does): the global ball with the triple (A, K, d) is converted with
   adf_fball_set_local, which gives the residue array of the set. */
static void REST_UNUSED
mklocal(adf_fball_t y, const adf_modctx_struct * ctx, slong A, slong K, slong d)
{
    adf_fball_t g;
    fmpz_t fa, fk, fd;
    int st;

    adf_fball_init(g);
    fmpz_init_set_si(fa, A);
    fmpz_init_set_si(fk, K);
    fmpz_init_set_si(fd, d);
    st = adf_fball_set_fmpz3(g, fa, fk, fd);
    if (st == ADF_OK)
        st = adf_fball_set_local(y, g, ctx);
    if (st != ADF_OK)
    {
        fprintf(stderr, "mklocal: status %s\n", adf_status_str(st));
        abort();
    }
    fmpz_clear(fa);
    fmpz_clear(fk);
    fmpz_clear(fd);
    adf_fball_clear(g);
}

ADF_TEST(forget_contains_every_rational_of_the_ball)
{
    adf_fball_t b;
    adf_resid_t x;
    adf_rat_t q;
    fmpz_t fa, fh, fd, a0, h0, d0;
    slong H, d, A, k, n, den, c, m, a, h, dd, balls, rats, kept, refused, exact;

    adf_fball_init(b);
    adf_resid_init(x);
    adf_rat_init(q);
    fmpz_init(fa);
    fmpz_init(fh);
    fmpz_init(fd);
    fmpz_init(a0);
    fmpz_init(h0);
    fmpz_init(d0);
    balls = rats = kept = refused = exact = 0;

    /* the claim of P1.10 (4) is about the canonical triple of the ball, so the test reads it with
       adf_fball_get_fmpz3 and uses those three integers: (A + H Zhat)/d with a common factor of A,
       H and d is the set of the reduced triple. */
    for (H = 1; H <= 12; H++)
    {
        for (d = 1; d <= 8; d++)
        {
            for (A = 0; A < H; A++)
            {
                fmpz_set_si(fa, A);
                fmpz_set_si(fh, H);
                fmpz_set_si(fd, d);
                if (adf_fball_set_fmpz3(b, fa, fh, fd) != ADF_OK)
                {
                    ADF_CHECK_MSG(0, "adf_fball_set_fmpz3 refused (%ld, %ld, %ld)", A, H, d);
                    continue;
                }
                balls++;
                adf_fball_get_fmpz3(a0, h0, d0, b);
                a = fmpz_get_si(a0);
                h = fmpz_get_si(h0);
                dd = fmpz_get_si(d0);
                ADF_CHECK_MSG(h >= 1, "the triple of (%ld + %ld Zhat)/%ld has H = %ld", A, H, d, h);
                ADF_CHECK_MSG(0 <= a && a < h, "the triple of (%ld + %ld Zhat)/%ld is (%ld, %ld, %ld)", A, H,
                              d, a, h, dd);
                ADF_CHECK(mkresid(x, 4, 9) == ADF_OK);   /* a value to see that DOMAIN does not write */
                if (gcd_sl(dd, h) == 1)
                {
                    kept++;
                    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
                    ADF_CHECK(adf_resid_is_canonical(x) == 1);
                    resid_pair(x, &c, &m);
                    ADF_CHECK_MSG(m == h, "forget of (%ld + %ld Zhat)/%ld = (%ld + %ld Zhat)/%ld gave the "
                                  "modulus %ld, expected %ld", A, H, d, a, h, dd, m, h);
                    ADF_CHECK_MSG((c * dd - a) % h == 0, "forget of (%ld + %ld Zhat)/%ld = (%ld + %ld Zhat)/"
                                  "%ld gave c = %ld, and c d - a = %ld", A, H, d, a, h, dd, c, c * dd - a);
                    /* every rational (a + h k)/d of the ball is in P(h, c) (P1.10 (4)) */
                    for (k = -20; k <= 20; k++)
                    {
                        slong g;

                        n = a + h * k;
                        den = dd;
                        g = gcd_sl(n, den);
                        n /= g;
                        den /= g;
                        if (mkrat(q, n, den) != ADF_OK)
                        {
                            ADF_CHECK_MSG(0, "mkrat(%ld, %ld) failed", n, den);
                            continue;
                        }
                        rats++;
                        ADF_CHECK_MSG(adf_resid_contains_rat(x, q) == 1,
                                      "forget of (%ld + %ld Zhat)/%ld = (%ld + %ld Zhat)/%ld: %ld/%ld is not "
                                      "in P(%ld, %ld)", A, H, d, a, h, dd, n, den, m, c);
                    }
                }
                else
                {
                    refused++;
                    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_DOMAIN);
                    resid_pair(x, &c, &m);
                    ADF_CHECK_MSG(c == 4 && m == 9, "forget wrote x on DOMAIN for (%ld, %ld, %ld)", A, H, d);
                }
            }
        }
    }
    /* the count line is printed after the exact balls, a few lines below */
    ADF_CHECK_MSG(balls == 624, "the grid has %ld balls, expected 624", balls);
    ADF_CHECK(kept > 100);
    ADF_CHECK(refused > 100);
    ADF_CHECK(kept + refused == balls);
    ADF_CHECK(rats == kept * 41);
    ADF_CHECK(rats > 10000);

    /* an exact ball (H = 0) is DOMAIN: a single rational is not a residue (P1.10 (4) needs H >= 1) */
    for (d = 1; d <= 8; d++)
    {
        ADF_CHECK(mkrat(q, 3, d) == ADF_OK);
        adf_fball_set_rat(b, q);
        if (adf_fball_is_exact(b) == 1)
        {
            ADF_CHECK(adf_fball_is_canonical(b) == 1);
            exact++;
            ADF_CHECK(mkresid(x, 4, 9) == ADF_OK);
            ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_DOMAIN);
            resid_pair(x, &c, &m);
            ADF_CHECK(c == 4 && m == 9);
        }
    }
    ADF_CHECK(exact == 8);
    printf("   forget: %ld balls of the grid (H <= 12, d <= 8, a in [0, H), read on the canonical "
           "triple), %ld kept, %ld refused, %ld rationals of the balls, %ld exact balls refused\n",
           balls, kept, refused, rats, exact);

    /* the exact zero and an exact integer are refused as well */
    adf_fball_zero(b);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_DOMAIN);
    adf_fball_one(b);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_DOMAIN);

    fmpz_clear(d0);
    fmpz_clear(h0);
    fmpz_clear(a0);
    fmpz_clear(fd);
    fmpz_clear(fh);
    fmpz_clear(fa);
    adf_rat_clear(q);
    adf_resid_clear(x);
    adf_fball_clear(b);
}

ADF_TEST(forget_uses_the_canonical_triple)
{
    adf_fball_t b;
    adf_resid_t x;
    adf_rat_t q;
    fmpz_t fa, fh, fd, a, h, dd;
    slong c, m, c2, m2;

    adf_fball_init(b);
    adf_resid_init(x);
    adf_rat_init(q);
    fmpz_init(fa);
    fmpz_init(fh);
    fmpz_init(fd);
    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(dd);

    /* a raw triple with a common factor and a negative d: (6 + 12 Zhat)/2 = 3 + 6 Zhat, so the
       residue is P(6, 3), not P(12, 6) and not P(6, 6) */
    fmpz_set_si(fa, 6);
    fmpz_set_si(fh, 12);
    fmpz_set_si(fd, 2);
    ADF_CHECK(adf_fball_set_fmpz3(b, fa, fh, fd) == ADF_OK);
    adf_fball_get_fmpz3(a, h, dd, b);
    ADF_CHECK(fmpz_get_si(a) == 3 && fmpz_get_si(h) == 6 && fmpz_get_si(dd) == 1);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 3 && m == 6);

    /* the same set written with a negative denominator: (3 + 6 Zhat)/(-1) = (-3 + 6 Zhat)/1 */
    fmpz_set_si(fa, 3);
    fmpz_set_si(fh, 6);
    fmpz_set_si(fd, -1);
    ADF_CHECK(adf_fball_set_fmpz3(b, fa, fh, fd) == ADF_OK);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
    resid_pair(x, &c2, &m2);
    ADF_CHECK(c2 == 3 && m2 == 6);

    /* a centre above H is reduced: (7 + 6 Zhat)/1 = 1 + 6 Zhat */
    fmpz_set_si(fa, 7);
    fmpz_set_si(fh, 6);
    fmpz_one(fd);
    ADF_CHECK(adf_fball_set_fmpz3(b, fa, fh, fd) == ADF_OK);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 1 && m == 6);

    /* H = 1: the ball is Zhat/1 plus a centre, every rational is in P(1, 0) = Q */
    fmpz_set_si(fa, 5);
    fmpz_one(fh);
    fmpz_set_si(fd, 3);
    ADF_CHECK(adf_fball_set_fmpz3(b, fa, fh, fd) == ADF_OK);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
    resid_pair(x, &c, &m);
    ADF_CHECK(c == 0 && m == 1);

    /* the inclusion is proper (P1.10 (4)): a rational of P(6, 3) outside 3 + 6 Zhat is c + H/e with
       e >= 2 prime to H d, here 3 + 6/5 = 21/5 */
    fmpz_set_si(fa, 3);
    fmpz_set_si(fh, 6);
    fmpz_one(fd);
    ADF_CHECK(adf_fball_set_fmpz3(b, fa, fh, fd) == ADF_OK);
    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
    ADF_CHECK(mkrat(q, 21, 5) == ADF_OK);
    ADF_CHECK(adf_resid_contains_rat(x, q) == 1);
    /* 21/5 is not of the ball: q is in (a + H Zhat)/d exactly when (q d - a)/H is an integer
       (docs/proofs/precision.md Lemma 1), and (21/5 - 3)/6 = 1/5 is not (here d = 1) */
    ADF_CHECK((21 - 3 * 5) % (6 * 5) != 0);

    fmpz_clear(dd);
    fmpz_clear(h);
    fmpz_clear(a);
    fmpz_clear(fd);
    fmpz_clear(fh);
    fmpz_clear(fa);
    adf_rat_clear(q);
    adf_resid_clear(x);
    adf_fball_clear(b);
}

ADF_TEST(forget_the_example_of_note_1)
{
    /* docs/api-s.md section 2, note 1: for 5 + 6 Zhat, adf_fball_reconstruct on [0, 1] gives
       NO_SOLUTION; after forget, adf_resid_reconstruct with A = 1, B = 5 gives NOT_UNIQUE (the
       solutions -1/1 and 1/5) and with B = 4 gives OK and -1/1. */
    adf_fball_t b;
    adf_resid_t x;
    adf_rat_t q, lo, hi;
    fmpz_t fa, fh, fd, fA, fB;
    adf_recon_cert_t cert;
    slong c, m, n, d;

    adf_fball_init(b);
    adf_resid_init(x);
    adf_rat_init(q);
    adf_rat_init(lo);
    adf_rat_init(hi);
    adf_recon_cert_init(cert);
    fmpz_init(fa);
    fmpz_init(fh);
    fmpz_init(fd);
    fmpz_init(fA);
    fmpz_init(fB);

    fmpz_set_si(fa, 5);
    fmpz_set_si(fh, 6);
    fmpz_one(fd);
    ADF_CHECK(adf_fball_set_fmpz3(b, fa, fh, fd) == ADF_OK);

    adf_rat_zero(lo);
    adf_rat_one(hi);
    ADF_CHECK(adf_fball_reconstruct(q, b, lo, hi) == ADF_NO_SOLUTION);

    ADF_CHECK(adf_resid_set_fball_forget(x, b) == ADF_OK);
    resid_pair(x, &c, &m);
    ADF_CHECK_MSG(c == 5 && m == 6, "forget of 5 + 6 Zhat gave P(%ld, %ld)", m, c);

    fmpz_one(fA);
    fmpz_set_si(fB, 5);
    ADF_CHECK(adf_resid_reconstruct(q, cert, x, fA, fB, 1000) == ADF_NOT_UNIQUE);
    ADF_CHECK(adf_recon_cert_check(cert, x, fA) == 1);

    fmpz_set_si(fB, 4);
    ADF_CHECK(adf_resid_reconstruct(q, cert, x, fA, fB, 1000) == ADF_OK);
    rat_pair(q, &n, &d);
    ADF_CHECK_MSG(n == -1 && d == 1, "B = 4 gave %ld/%ld, expected -1/1", n, d);

    fmpz_clear(fB);
    fmpz_clear(fA);
    fmpz_clear(fd);
    fmpz_clear(fh);
    fmpz_clear(fa);
    adf_recon_cert_clear(cert);
    adf_rat_clear(hi);
    adf_rat_clear(lo);
    adf_rat_clear(q);
    adf_resid_clear(x);
    adf_fball_clear(b);
}

ADF_TEST(forget_local_and_global_give_the_same_residue)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t g, l;
    adf_resid_t xg, xl;
    adf_rat_t q;
    fmpz_t fa, fh, fd, a1, h1, d1, a2, h2, d2;
    slong H, d, A, c, m, c2, m2, k, cases;

    /* two blocks: 6 and 35, the modulus 210, so that a local value carries more than one prime */
    {
        ulong blocks[2];

        blocks[0] = 6;
        blocks[1] = 35;
        ADF_CHECK(adf_modctx_new_blocks(&ctx, blocks, 2) == ADF_OK);
    }

    adf_fball_init(g);
    adf_fball_init(l);
    adf_resid_init(xg);
    adf_resid_init(xl);
    adf_rat_init(q);
    fmpz_init(fa);
    fmpz_init(fh);
    fmpz_init(fd);
    fmpz_init(a1);
    fmpz_init(h1);
    fmpz_init(d1);
    fmpz_init(a2);
    fmpz_init(h2);
    fmpz_init(d2);
    cases = 0;

    for (H = 1; H <= 12; H++)
    {
        for (d = 1; d <= 6; d++)
        {
            if (gcd_sl(d, 210) != 1)
                continue;                   /* the local value needs d prime to the modulus */
            for (A = 0; A < 210; A += 17)
            {
                fmpz_set_si(fa, A);
                fmpz_set_si(fh, 210);
                fmpz_set_si(fd, d);
                if (adf_fball_set_fmpz3(g, fa, fh, fd) != ADF_OK)
                    continue;
                mklocal(l, ctx, A, 210, d);
                ADF_CHECK(adf_fball_is_local(l) == 1);
                ADF_CHECK(adf_fball_is_canonical(l) == 1);
                /* the two values are the same set, so the same canonical triple */
                adf_fball_get_fmpz3(a1, h1, d1, g);
                adf_fball_get_fmpz3(a2, h2, d2, l);
                ADF_CHECK(fmpz_equal(a1, a2) && fmpz_equal(h1, h2) && fmpz_equal(d1, d2));
                cases++;
                /* and the same residue */
                ADF_CHECK(adf_resid_set_fball_forget(xg, g) == ADF_OK);
                ADF_CHECK(adf_resid_set_fball_forget(xl, l) == ADF_OK);
                resid_pair(xg, &c, &m);
                resid_pair(xl, &c2, &m2);
                ADF_CHECK_MSG(c == c2 && m == m2, "the local value of (%ld + 210 Zhat)/%ld gave P(%ld, "
                              "%ld), the global one P(%ld, %ld)", A, d, m2, c2, m, c);
                /* every rational of the ball, reduced, is in the residue (P1.10 (4)); the rational is
                   built from the global canonical triple, which says nothing about the backend */
                for (k = -3; k <= 3; k++)
                {
                    slong n2, d2v, gg;

                    n2 = fmpz_get_si(a1) + fmpz_get_si(h1) * k;
                    d2v = fmpz_get_si(d1);
                    gg = gcd_sl(n2, d2v);
                    n2 /= gg;
                    d2v /= gg;
                    if (mkrat(q, n2, d2v) == ADF_OK)
                        ADF_CHECK_MSG(adf_resid_contains_rat(xl, q) == 1,
                                      "the local value of (%ld + 210 Zhat)/%ld: %ld/%ld is not in P(%ld, "
                                      "%ld)", A, d, n2, d2v, m2, c2);
                }
            }
        }
    }
    ADF_CHECK(cases > 100);

    adf_fball_clear(l);
    adf_fball_clear(g);
    adf_resid_clear(xl);
    adf_resid_clear(xg);
    adf_rat_clear(q);
    fmpz_clear(d2);
    fmpz_clear(h2);
    fmpz_clear(a2);
    fmpz_clear(d1);
    fmpz_clear(h1);
    fmpz_clear(a1);
    fmpz_clear(fd);
    fmpz_clear(fh);
    fmpz_clear(fa);
    adf_modctx_free(ctx);
}

/* ---- 4. adf_resid_reconstruct_first ---- */

/* The certificate pair of Lemma 1.4 for 0 <= A < m, by an own extended Euclidean algorithm in machine
   integers: (Rp, Tp, R, T) = (r_(j-1), t_(j-1), r_j, t_j) with r_j <= A < r_(j-1). */
static void REST_UNUSED
eea_pair_sl(slong m, slong c, slong A, slong * Rp, slong * Tp, slong * R, slong * T)
{
    slong r0 = m, t0 = 0, r1 = c % m, t1 = 1, q, r2;

    if (r1 < 0)
        r1 += m;
    while (r1 > A)
    {
        q = r0 / r1;
        r2 = r0 - q * r1;
        r0 = r1;
        r1 = r2;
        q = t0 - q * t1;
        t0 = t1;
        t1 = q;
    }
    *Rp = r0;
    *Tp = t0;
    *R = r1;
    *T = t1;
}

#define MAXP 512

/* One solution of Definition 1.1 with the round (x, y) of Proposition 1.5 that produces it, so that
   the list is in the order of Algorithm R. x and y are recovered from (n, d) by Cramer's rule:
   sigma R d - n |T| = sigma y m (Proposition 1.5 (2)) and d = x |T| + y |T'|. */
typedef struct
{
    slong n, d, x, y;
} solpt;

static int REST_UNUSED
cmp_solpt(const void *va, const void *vb)
{
    const solpt *a = va, *b = vb;

    if (a->x != b->x)
        return a->x < b->x ? -1 : 1;
    if (a->y != b->y)
        return a->y < b->y ? -1 : 1;
    return 0;
}

/* All the solutions of Definition 1.1, in the order of Algorithm R. For each d prime to m only the
   integers n of the class c d modulo m are tried, so that A >= m is affordable; the literal double loop
   over (n, d) is cross-checked against it in the test for m <= 8. Returns the number of solutions. */
static int REST_UNUSED
enum_sol(slong m, slong c, slong A, slong B, solpt * out)
{
    slong d, n, k = 0, r, lo;

    if (A < 0 || B < 1)
        return 0;
    c = c % m;
    if (c < 0)
        c += m;
    for (d = 1; d <= B; d++)
    {
        if (gcd_sl(d, m) != 1)
            continue;
        r = (c * d) % m;
        if (r < 0)
            r += m;
        lo = -A + ((r - (-A)) % m + m) % m;    /* the least n >= -A with n = r modulo m */
        for (n = lo; n <= A; n += m)
        {
            if (gcd_sl(n, d) == 1)
            {
                out[k].n = n;
                out[k].d = d;
                k++;
                if (k >= MAXP)
                    return k;
            }
        }
    }
    return k;
}

/* The round (x, y) of every point of the list, from its own pair (own Euclidean algorithm, machine
   integers). Needs 0 <= A < m and |T| <= B, that is the case in which the search runs. */
static void REST_UNUSED
rounds_of(solpt * L, int k, slong m, slong c, slong A)
{
    slong Rp, Tp, R, T, aT, aTp, sg, i, y, x;

    eea_pair_sl(m, c, A, &Rp, &Tp, &R, &T);
    aT = T < 0 ? -T : T;
    aTp = Tp < 0 ? -Tp : Tp;
    sg = T < 0 ? -1 : 1;
    for (i = 0; i < k; i++)
    {
        y = (R * L[i].d - sg * L[i].n * aT) / m;   /* sigma y m = sigma R d - n |T| (1.5 (2)) */
        x = (L[i].d - y * aTp) / aT;
        L[i].x = x;
        L[i].y = y;
    }
    qsort(L, (size_t) k, sizeof(solpt), cmp_solpt);
}

/* What adf_resid_reconstruct_first must return, from the enumeration of Definition 1.1 and
   Proposition 1.7 (claim 3), computed here and not read from the library. -1 in n and d means that q
   must be untouched. */
static void REST_UNUSED
expected_first(slong m, slong c, slong A, slong B, slong limit, int * st, int * cnt, slong * n, slong * d)
{
    solpt L[MAXP];
    slong aT, ell = limit > 0 ? limit : 0, X;
    int k;

    *n = -1;
    *d = -1;
    *cnt = 0;
    if (A < 0 || B < 1)
    {
        *st = ADF_NO_SOLUTION;                /* the box is empty (S-D5) */
        return;
    }
    if (A >= m)
    {
        *st = ADF_OK;                         /* Proposition 1.6 (a): the first solution is c/1 */
        *cnt = 2;
        *n = c % m;
        *d = 1;
        return;
    }
    {
        slong Rp, Tp, R, T;

        eea_pair_sl(m, c, A, &Rp, &Tp, &R, &T);
        aT = T < 0 ? -T : T;
    }
    k = enum_sol(m, c, A, B, L);
    if (aT > B)
    {
        *st = ADF_NO_SOLUTION;                /* Proposition 1.6 (b): the box has no point of the lattice */
        return;
    }
    if (2 * A * B < m)
    {
        /* Proposition 1.6 (c): the search never runs, the answer does not depend on the limit */
        if (k == 1)
        {
            *st = ADF_OK;
            *cnt = 1;
            *n = L[0].n;
            *d = L[0].d;
        }
        else
            *st = ADF_NO_SOLUTION;
        return;
    }
    X = B / aT;
    rounds_of(L, k, m, c, A);
    /* the search visits the rounds x = 1, ..., min(X, ell), so it finds exactly the solutions with
       x <= ell, which are the first ones of the list (it is sorted by x) */
    while (k > 0 && L[k - 1].x > ell)
        k--;
    if (k >= 2)
    {
        *st = ADF_OK;                         /* two solutions found: NOT_UNIQUE of Algorithm R, count 2 */
        *cnt = 2;
        *n = L[0].n;
        *d = L[0].d;
        return;
    }
    if (k == 1)
    {
        *st = ADF_OK;
        *cnt = (X > ell) ? 0 : 1;             /* a cut search after one solution does not prove uniqueness */
        *n = L[0].n;
        *d = L[0].d;
        return;
    }
    *st = (X > ell) ? ADF_NOT_DETERMINED : ADF_NO_SOLUTION;
}

/* The distinct values of B of the grid. */
static int REST_UNUSED
bvalues(slong m, slong * out)
{
    slong v[6] = {1, 2, 3, 5, m, 2 * m};
    int i, j, k = 0;

    for (i = 0; i < 6; i++)
    {
        if (v[i] < 1)
            continue;
        for (j = 0; j < k; j++)
            if (out[j] == v[i])
                break;
        if (j == k)
            out[k++] = v[i];
    }
    return k;
}

ADF_TEST(reconstruct_first_against_the_enumeration)
{
    adf_resid_t x;
    adf_rat_t q, qs;
    adf_recon_cert_t cert;
    fmpz_t fA, fB;
    static const slong limits[6] = {-1, 0, 1, 2, 5, 1000000000};
    slong m, c, A, B, bv[6], nb, nbv, li, n, d, sn, sd, Rp, Tp, R, T, kind, calls;
    int st, cnt, est, ecnt, count_status[3], count_cnt[3], ok_q, untouched, cut0;
    solpt L[MAXP], L2[MAXP];

    adf_resid_init(x);
    adf_rat_init(q);
    adf_rat_init(qs);
    adf_recon_cert_init(cert);
    fmpz_init(fA);
    fmpz_init(fB);
    calls = 0;
    count_status[0] = count_status[1] = count_status[2] = 0;
    count_cnt[0] = count_cnt[1] = count_cnt[2] = 0;
    ok_q = untouched = cut0 = 0;

    for (m = 1; m <= 24; m++)
    {
        for (c = 0; c < m; c++)
        {
            ADF_CHECK(mkresid(x, c, m) == ADF_OK);
            for (A = 0; A <= 2 * m; A++)
            {
                fmpz_set_si(fA, A);
                nbv = bvalues(m, bv);
                for (nb = 0; nb < nbv; nb++)
                {
                    B = bv[nb];
                    fmpz_set_si(fB, B);
                    for (li = 0; li < 6; li++)
                    {
                        expected_first(m, c, A, B, limits[li], &est, &ecnt, &sn, &sd);
                        /* a sentinel in q: it must survive every status but ADF_OK */
                        adf_rat_set_si(qs, -12345);
                        adf_rat_set(q, qs);
                        st = adf_resid_reconstruct_first(q, &cnt, cert, x, fA, fB, limits[li]);
                        calls++;
                        ADF_CHECK_MSG(st == est, "m = %ld, c = %ld, A = %ld, B = %ld, limit = %ld: status "
                                      "%d, expected %d", m, c, A, B, limits[li], st, est);
                        ADF_CHECK_MSG(cnt == ecnt, "m = %ld, c = %ld, A = %ld, B = %ld, limit = %ld: count "
                                      "%d, expected %d", m, c, A, B, limits[li], cnt, ecnt);
                        if (st == ADF_OK)
                        {
                            rat_pair(q, &n, &d);
                            ADF_CHECK_MSG(n == sn && d == sd, "m = %ld, c = %ld, A = %ld, B = %ld, limit = "
                                          "%ld: q = %ld/%ld, expected %ld/%ld", m, c, A, B, limits[li], n,
                                          d, sn, sd);
                            ok_q++;
                        }
                        if (st != ADF_OK)
                        {
                            rat_pair(q, &n, &d);
                            ADF_CHECK_MSG(n == -12345 && d == 1, "q was written on status %d (m = %ld, "
                                          "c = %ld, A = %ld, B = %ld, limit = %ld): %ld/%ld", st, m, c, A, B,
                                          limits[li], n, d);
                            untouched++;
                        }
                        /* the certificate, as adf_resid_reconstruct writes it */
                        ADF_CHECK_MSG(adf_recon_cert_is_canonical(cert) == 1, "m = %ld, c = %ld, A = %ld, "
                                      "B = %ld, limit = %ld: the certificate written is not canonical", m, c,
                                      A, B, limits[li]);
                        cert_fields(cert, &Rp, &Tp, &R, &T, &kind);
                        if (A < 0 || B < 1 || A >= m)
                            ADF_CHECK_MSG(kind == 0 && Rp == 0 && Tp == 0 && R == 0 && T == 0,
                                          "cert is (%ld, %ld, %ld, %ld) with kind %ld for A = %ld, B = %ld, "
                                          "m = %ld", Rp, Tp, R, T, kind, A, B, m);
                        else
                        {
                            slong eRp, eTp, eR, eT;

                            ADF_CHECK_MSG(kind == 1, "cert has kind %ld for A = %ld < m = %ld", kind, A, m);
                            ADF_CHECK(adf_recon_cert_check(cert, x, fA) == 1);
                            eea_pair_sl(m, c, A, &eRp, &eTp, &eR, &eT);
                            ADF_CHECK_MSG(Rp == eRp && Tp == eTp && R == eR && T == eT,
                                          "cert = (%ld, %ld, %ld, %ld), own pair = (%ld, %ld, %ld, %ld) for "
                                          "m = %ld, c = %ld, A = %ld", Rp, Tp, R, T, eRp, eTp, eR, eT, m,
                                          c, A);
                        }
                        /* the claims of the brief: count 1 only with one element, 2 only with two or
                           more, 0 with OK only after a cut search */
                        if (ecnt == 1 || ecnt == 2)
                        {
                            int nel = enum_sol(m, c, A, B, L);

                            if (ecnt == 1)
                                ADF_CHECK_MSG(nel == 1, "count 1 with %d solutions (m = %ld, c = %ld, A = %ld, "
                                                  "B = %ld, limit = %ld)", nel, m, c, A, B, limits[li]);
                            else
                                ADF_CHECK_MSG(nel >= 2, "count 2 with %d solutions (m = %ld, c = %ld, A = %ld, "
                                                  "B = %ld, limit = %ld)", nel, m, c, A, B, limits[li]);
                        }
                        else if (est == ADF_OK)
                        {
                            slong aT, ell = limits[li] > 0 ? limits[li] : 0, X;

                            eea_pair_sl(m, c, A, &Rp, &Tp, &R, &T);
                            aT = T < 0 ? -T : T;
                            X = B / aT;
                            ADF_CHECK_MSG(X > ell && 2 * A * B >= m && A < m && aT <= B,
                                          "count 0 with OK but the search was not cut (m = %ld, c = %ld, "
                                          "A = %ld, B = %ld, limit = %ld)", m, c, A, B, limits[li]);
                            cut0++;
                        }
                        if (st == ADF_OK)
                            count_status[0]++;
                        else if (st == ADF_NOT_DETERMINED)
                            count_status[1]++;
                        else
                            count_status[2]++;
                        if (cnt >= 0 && cnt <= 2)
                            count_cnt[cnt]++;
                    }
                }
            }
            /* the enumeration used above against the literal double loop of Definition 1.1 */
            if (m <= 8)
            {
                for (A = 0; A <= 2 * m; A++)
                {
                    for (nb = 0; nb < bvalues(m, bv); nb++)
                    {
                        int k1, k2 = 0;
                        slong dd, nn;

                        B = bv[nb];
                        k1 = enum_sol(m, c, A, B, L);
                        for (dd = 1; dd <= B; dd++)
                            for (nn = -A; nn <= A; nn++)
                                if (gcd_sl(nn, dd) == 1 && gcd_sl(dd, m) == 1 && (nn - c * dd) % m == 0)
                                    k2++;
                        ADF_CHECK_MSG(k1 == k2, "the enumeration of m = %ld, c = %ld, A = %ld, B = %ld has "
                                      "%d points, the double loop %d", m, c, A, B, k1, k2);
                        (void) L2;
                    }
                }
            }
        }
    }
    printf("   reconstruct_first: %ld calls, OK %d, NOT_DETERMINED %d, NO_SOLUTION %d; count 0/1/2 "
           "%d/%d/%d; %ld q written, %ld left untouched, %ld cut answers\n", calls,
           (int) count_status[0], (int) count_status[1], (int) count_status[2], (int) count_cnt[0],
           (int) count_cnt[1],
           (int) count_cnt[2], (long) ok_q, (long) untouched, (long) cut0);
    /* every status and every count occurs, and the two are not empty on either side */
    ADF_CHECK(calls > 300000);
    ADF_CHECK(count_status[0] > 1000);
    ADF_CHECK(count_status[1] > 1000);
    ADF_CHECK(count_status[2] > 1000);
    ADF_CHECK(count_cnt[0] > 100);
    ADF_CHECK(count_cnt[1] > 100);
    ADF_CHECK(count_cnt[2] > 1000);
    ADF_CHECK(cut0 > 100);

    fmpz_clear(fB);
    fmpz_clear(fA);
    adf_recon_cert_clear(cert);
    adf_rat_clear(qs);
    adf_rat_clear(q);
    adf_resid_clear(x);
}

/* ---- 4b. the vectors of recon_first (lanes/s3-slice3/gen_vectors.py, from proto/solvers_checks.py) ---- */

/* The status of a vector file, as a code of status.h. */
static int REST_UNUSED
status_of(const jsonl_value * v, const char * what, jsonl_error_t * err)
{
    const char *s;
    size_t len;

    s = jsonl_string(v, &len, err);
    if (s == NULL)
        return -1;
    if (strcmp(s, "OK") == 0)
        return ADF_OK;
    if (strcmp(s, "NO_SOLUTION") == 0)
        return ADF_NO_SOLUTION;
    if (strcmp(s, "NOT_DETERMINED") == 0)
        return ADF_NOT_DETERMINED;
    if (strcmp(s, "NOT_UNIQUE") == 0)
        return ADF_NOT_UNIQUE;
    if (strcmp(s, "DOMAIN") == 0)
        return ADF_DOMAIN;
    ADF_CHECK_MSG(0, "%s: unknown status \"%s\"", what, s);
    return -1;
}

ADF_TEST(reconstruct_first_vectors)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    adf_resid_t x;
    adf_rat_t q, qs;
    adf_recon_cert_t cert;
    fmpz_t fm, fc, fA, fB;
    size_t i, ok_cnt = 0, nd_cnt = 0, ns_cnt = 0, c0 = 0, c1 = 0, c2 = 0, longlines = 0;
    int st, cnt, est;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s3-slice3/recon_first.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    adf_resid_init(x);
    adf_rat_init(q);
    adf_rat_init(qs);
    adf_recon_cert_init(cert);
    fmpz_init(fm);
    fmpz_init(fc);
    fmpz_init(fA);
    fmpz_init(fB);
    adf_rat_set_si(qs, -12345);

    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        const jsonl_value *v;
        slong m, A = 0, B = 0, limit, n, d;

        ADF_CHECK(jsonl_field(rec, "m", &v, &err) == 1 && fmpz_set_str(fm, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "c", &v, &err) == 1 && fmpz_set_str(fc, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "A", &v, &err) == 1 && fmpz_set_str(fA, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "B", &v, &err) == 1 && fmpz_set_str(fB, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "limit", &v, &err) == 1);
        limit = strtol(jsonl_int_text(v, &err), NULL, 10);
        ADF_CHECK(jsonl_field(rec, "status", &v, &err) == 1);
        est = status_of(v, "recon_first.jsonl", &err);
        ADF_CHECK(jsonl_field(rec, "count", &v, &err) == 1);
        cnt = (int) strtol(jsonl_int_text(v, &err), NULL, 10);
        m = fmpz_get_si(fm);
        A = fmpz_get_si(fA);
        B = fmpz_get_si(fB);
        if (m > 1000000 || A > 1000000)
            longlines++;
        ADF_CHECK(adf_resid_set_fmpz2(x, fc, fm) == ADF_OK);
        adf_rat_set(q, qs);
        st = adf_resid_reconstruct_first(q, &cnt, cert, x, fA, fB, limit);
        /* the count of the file, read again: the call above overwrote the variable */
        ADF_CHECK(jsonl_field(rec, "count", &v, &err) == 1);
        {
            int want = (int) strtol(jsonl_int_text(v, &err), NULL, 10);

            ADF_CHECK_MSG(cnt == want, "line %lu: m = %s, c = %s, A = %s, B = %s, limit = %ld: count %d, "
                          "the reference says %d", (unsigned long) i + 1, "see the file", "see the file",
                          "see the file", "see the file", limit, cnt, want);
        }
        ADF_CHECK_MSG(st == est, "line %lu (m = %ld, A = %ld, B = %ld, limit = %ld): status %d, the "
                      "reference says %d", (unsigned long) i + 1, m, A, B, limit, st, est);
        if (st == ADF_OK)
        {
            fmpq_t got, wantq;
            fmpz_t fn, fd;

            ok_cnt++;
            ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
            fmpz_init(fn);
            ADF_CHECK(fmpz_set_str(fn, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "d", &v, &err) == 1);
            fmpz_init(fd);
            fmpz_set_str(fd, jsonl_int_text(v, &err), 10);
            fmpq_init(got);
            fmpq_init(wantq);
            adf_rat_get_fmpq(got, q);
            fmpz_set(fmpq_numref(wantq), fn);
            fmpz_set(fmpq_denref(wantq), fd);
            fmpq_canonicalise(wantq);
            if (!fmpq_equal(got, wantq))
            {
                char *gs = fmpq_get_str(NULL, 10, got);
                char *ns = fmpz_get_str(NULL, 10, fn);
                char *ds = fmpz_get_str(NULL, 10, fd);

                ADF_CHECK_MSG(0, "line %lu (m = %ld, A = %ld, B = %ld, limit = %ld): q is %s, the file says "
                              "%s/%s", (unsigned long) i + 1, m, A, B, limit, gs, ns, ds);
                flint_free(gs);
                flint_free(ns);
                flint_free(ds);
            }
            fmpq_clear(wantq);
            fmpq_clear(got);
            fmpz_clear(fd);
            fmpz_clear(fn);
        }
        else
        {
            rat_pair(q, &n, &d);
            ADF_CHECK_MSG(n == -12345 && d == 1, "line %lu: q was written on status %d", (unsigned long) i + 1,
                          st);
            ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
            ADF_CHECK(jsonl_is_null(v, &err) == 1);
        }
        /* the certificate of the file */
        ADF_CHECK_MSG(adf_recon_cert_is_canonical(cert) == 1, "line %lu: the certificate is not canonical",
                      (unsigned long) i + 1);
        ADF_CHECK(jsonl_field(rec, "cert", &v, &err) == 1);
        if (jsonl_is_null(v, &err) == 1)
            ADF_CHECK_MSG(cert->kind == 0, "line %lu: the file has no certificate, the call gave kind %d",
                          (unsigned long) i + 1, cert->kind);
        else
        {
            ADF_CHECK_MSG(cert->kind == 1, "line %lu: the file has a certificate, the call gave kind %d",
                          (unsigned long) i + 1, cert->kind);
            if (A >= 0 && fmpz_cmp(fA, fm) < 0 && fmpz_sgn(fB) >= 1)
                ADF_CHECK_MSG(adf_recon_cert_check(cert, x, fA) == 1,
                              "line %lu: adf_recon_cert_check refuses the certificate of the call",
                              (unsigned long) i + 1);
            if (jsonl_size(v) == 4)
            {
                ADF_CHECK(jsonl_at(v, 0, &err) != NULL);
                ADF_CHECK(fmpz_set_str(cert->Rp, jsonl_int_text(jsonl_at(v, 0, &err), &err), 10) == 0);
                ADF_CHECK(fmpz_set_str(cert->Tp, jsonl_int_text(jsonl_at(v, 1, &err), &err), 10) == 0);
                ADF_CHECK(fmpz_set_str(cert->R, jsonl_int_text(jsonl_at(v, 2, &err), &err), 10) == 0);
                ADF_CHECK(fmpz_set_str(cert->T, jsonl_int_text(jsonl_at(v, 3, &err), &err), 10) == 0);
                ADF_CHECK_MSG(adf_recon_cert_check(cert, x, fA) == 1,
                              "line %lu: the certificate of the file does not satisfy (C1) to (C4)",
                              (unsigned long) i + 1);
            }
        }
        if (cnt == 0)
            c0++;
        else if (cnt == 1)
            c1++;
        else
            c2++;
        if (st == ADF_NOT_DETERMINED)
            nd_cnt++;
        else if (st == ADF_NO_SOLUTION)
            ns_cnt++;
    }
    printf("   recon_first vectors: %lu lines, all run; OK %lu, NOT_DETERMINED %lu, NO_SOLUTION %lu; "
           "count 0/1/2 %lu/%lu/%lu; %lu lines with m or A above a million\n", (unsigned long) jsonl_count(f),
           (unsigned long) ok_cnt, (unsigned long) nd_cnt, (unsigned long) ns_cnt, (unsigned long) c0,
           (unsigned long) c1, (unsigned long) c2, (unsigned long) longlines);
    ADF_CHECK(jsonl_count(f) > 10000);
    ADF_CHECK(ok_cnt > 1000);
    ADF_CHECK(nd_cnt > 100);
    ADF_CHECK(ns_cnt > 100);
    ADF_CHECK(c0 > 100 && c1 > 100 && c2 > 100);
    ADF_CHECK(longlines > 100);

    fmpz_clear(fB);
    fmpz_clear(fA);
    fmpz_clear(fc);
    fmpz_clear(fm);
    adf_recon_cert_clear(cert);
    adf_rat_clear(qs);
    adf_rat_clear(q);
    adf_resid_clear(x);
    jsonl_close(f);
}

/* ---- 2b and 3b. the vectors of set_rat, contains_rat and set_fball_forget ---- */

ADF_TEST(set_rat_and_forget_vectors)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    adf_resid_t x, y;
    adf_rat_t q;
    adf_fball_t b;
    fmpz_t fm, fn, fd, fh, fA;
    size_t i, rat_lines = 0, forget_lines = 0, rat_ok = 0, rat_dom = 0, forget_ok = 0, forget_dom = 0;
    size_t inside_true = 0, inside_false = 0;
    int st;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s3-slice3/sets.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    adf_resid_init(x);
    adf_resid_init(y);
    adf_rat_init(q);
    adf_fball_init(b);
    fmpz_init(fm);
    fmpz_init(fn);
    fmpz_init(fd);
    fmpz_init(fh);
    fmpz_init(fA);

    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        const jsonl_value *v, *kind, *inside;
        size_t len;
        const char *ks;

        ADF_CHECK(jsonl_field(rec, "kind", &kind, &err) == 1);
        ks = jsonl_string(kind, &len, &err);
        ADF_CHECK(ks != NULL);
        if (ks == NULL)
            continue;
        if (strcmp(ks, "set_rat") == 0)
        {
            int want, cc, hits = 0;

            rat_lines++;
            ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1 && fmpz_set_str(fn, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "d", &v, &err) == 1 && fmpz_set_str(fd, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "m", &v, &err) == 1 && fmpz_set_str(fm, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "status", &v, &err) == 1);
            want = (status_of(v, "sets.jsonl", &err) == ADF_OK) ? ADF_OK : ADF_DOMAIN;
            ADF_CHECK(jsonl_field(rec, "inside", &inside, &err) == 1);
            ADF_CHECK(mkrat(q, fmpz_get_si(fn), fmpz_get_si(fd)) == ADF_OK);
            ADF_CHECK(mkresid(x, 4, 9) == ADF_OK);
            st = adf_resid_set_rat(x, q, fm);
            ADF_CHECK_MSG(st == want, "line %lu: set_rat(%s, %s) gave %d, the file says %s",
                          (unsigned long) i + 1, "n/d", "m", st, want == ADF_OK ? "OK" : "DOMAIN");
            if (st == ADF_OK)
            {
                fmpz_t cc2, mm;

                rat_ok++;
                fmpz_init(cc2);
                fmpz_init(mm);
                adf_resid_get_fmpz2(cc2, mm, x);
                ADF_CHECK_MSG(fmpz_equal(mm, fm), "line %lu: set_rat gave another modulus", (unsigned long) i + 1);
                ADF_CHECK(jsonl_field(rec, "c", &v, &err) == 1);
                ADF_CHECK(fmpz_set_str(cc2, jsonl_int_text(v, &err), 10) == 0);
                {
                    adf_resid_t z;
                    slong want_c = fmpz_get_si(cc2);

                    adf_resid_init(z);
                    ADF_CHECK(mkresid(z, want_c, fmpz_get_si(fm)) == ADF_OK);
                    ADF_CHECK_MSG(adf_resid_identical(x, z) == 1, "line %lu: set_rat gave another c than the "
                                  "file", (unsigned long) i + 1);
                    adf_resid_clear(z);
                }
                fmpz_clear(mm);
                fmpz_clear(cc2);
            }
            else
            {
                slong c, m;

                rat_dom++;
                resid_pair(x, &c, &m);
                ADF_CHECK_MSG(c == 4 && m == 9, "line %lu: set_rat wrote x on DOMAIN", (unsigned long) i + 1);
            }
            /* the predicate, for every c in [0, m) */
            ADF_CHECK(jsonl_size(inside) == (size_t) fmpz_get_si(fm));
            for (cc = 0; cc < fmpz_get_si(fm); cc++)
            {
                int want_in, got;

                ADF_CHECK(jsonl_at(inside, (size_t) cc, &err) != NULL);
                want_in = (int) strtol(jsonl_int_text(jsonl_at(inside, (size_t) cc, &err), &err), NULL, 10);
                ADF_CHECK(mkresid(y, cc, fmpz_get_si(fm)) == ADF_OK);
                got = adf_resid_contains_rat(y, q);
                ADF_CHECK_MSG(got == want_in, "line %lu: contains_rat(P(%ld, %d)) = %d, the file says %d",
                              (unsigned long) i + 1, fmpz_get_si(fm), cc, got, want_in);
                hits += got;
                if (got)
                    inside_true++;
                else
                    inside_false++;
            }
            /* exactly one c contains the rational when gcd(d, m) = 1, and none otherwise */
            ADF_CHECK_MSG(hits == (st == ADF_OK ? 1 : 0), "line %lu: %d residues contain the rational",
                          (unsigned long) i + 1, hits);
        }
        else if (strcmp(ks, "forget") == 0)
        {
            slong k;

            forget_lines++;
            ADF_CHECK(jsonl_field(rec, "A", &v, &err) == 1 && fmpz_set_str(fA, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "H", &v, &err) == 1 && fmpz_set_str(fh, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "d", &v, &err) == 1 && fmpz_set_str(fd, jsonl_int_text(v, &err), 10) == 0);
            ADF_CHECK(jsonl_field(rec, "k", &v, &err) == 1);
            k = strtol(jsonl_int_text(v, &err), NULL, 10);
            ADF_CHECK(jsonl_field(rec, "status", &v, &err) == 1);
            {
                int want = (status_of(v, "sets.jsonl", &err) == ADF_OK) ? ADF_OK : ADF_DOMAIN;

                ADF_CHECK(adf_fball_set_fmpz3(b, fA, fh, fd) == ADF_OK);
                ADF_CHECK(mkresid(x, 4, 9) == ADF_OK);
                st = adf_resid_set_fball_forget(x, b);
                ADF_CHECK_MSG(st == want, "line %lu: forget(%s, %s, %s) gave %d, the file says %d",
                              (unsigned long) i + 1, "A", "H", "d", st, want);
                if (st == ADF_OK)
                {
                    slong c, m;

                    forget_ok++;
                    resid_pair(x, &c, &m);
                    ADF_CHECK_MSG(m == fmpz_get_si(fh), "line %lu: forget gave the modulus %ld, the file says "
                                  "%ld", (unsigned long) i + 1, m, fmpz_get_si(fh));
                    ADF_CHECK(jsonl_field(rec, "c", &v, &err) == 1);
                    ADF_CHECK_MSG(c == strtol(jsonl_int_text(v, &err), NULL, 10),
                                  "line %lu: forget gave c = %ld, the file says %s", (unsigned long) i + 1, c,
                                  jsonl_int_text(v, &err));
                    ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
                    ADF_CHECK(fmpz_set_str(fn, jsonl_int_text(v, &err), 10) == 0);
                    ADF_CHECK(jsonl_field(rec, "dn", &v, &err) == 1);
                    ADF_CHECK(fmpz_set_str(fd, jsonl_int_text(v, &err), 10) == 0);
                    ADF_CHECK(mkrat(q, fmpz_get_si(fn), fmpz_get_si(fd)) == ADF_OK);
                    ADF_CHECK(jsonl_field(rec, "inside", &inside, &err) == 1);
                    ADF_CHECK_MSG(adf_resid_contains_rat(x, q) == (int) strtol(jsonl_int_text(inside, &err),
                                                  NULL, 10), "line %lu: the rational %s/%s of the ball is not "
                                  "in the residue", (unsigned long) i + 1, fmpz_get_str(NULL, 10, fn),
                                  fmpz_get_str(NULL, 10, fd));
                    (void) k;
                }
                else
                {
                    slong c, m;

                    forget_dom++;
                    resid_pair(x, &c, &m);
                    ADF_CHECK_MSG(c == 4 && m == 9, "line %lu: forget wrote x on DOMAIN",
                                  (unsigned long) i + 1);
                }
            }
        }
        else
            ADF_CHECK_MSG(0, "line %lu: unknown kind \"%s\"", (unsigned long) i + 1, ks);
    }
    printf("   sets vectors: %lu lines, all run; set_rat %lu (OK %lu, DOMAIN %lu), forget %lu (OK %lu, "
           "DOMAIN %lu); contains_rat inside %lu, outside %lu\n", (unsigned long) jsonl_count(f),
           (unsigned long) rat_lines, (unsigned long) rat_ok, (unsigned long) rat_dom,
           (unsigned long) forget_lines, (unsigned long) forget_ok, (unsigned long) forget_dom,
           (unsigned long) inside_true, (unsigned long) inside_false);
    ADF_CHECK(rat_lines > 1000);
    ADF_CHECK(forget_lines > 1000);
    ADF_CHECK(rat_ok > 100 && rat_dom > 100);
    ADF_CHECK(forget_ok > 100 && forget_dom > 100);
    ADF_CHECK(inside_true > 100 && inside_false > 1000);

    fmpz_clear(fA);
    fmpz_clear(fh);
    fmpz_clear(fd);
    fmpz_clear(fn);
    fmpz_clear(fm);
    adf_fball_clear(b);
    adf_rat_clear(q);
    adf_resid_clear(y);
    adf_resid_clear(x);
    jsonl_close(f);
}

/* ---- 5. adf_resid_verify_result ---- */

/* The status of Algorithm R for (m, c, A, B, limit) and the first two solutions in the order of
   Algorithm R, computed from the enumeration of Definition 1.1 and from Proposition 1.7 (3). The first
   two are written when they exist; second is written when the set has at least two elements. This is
   the truth of a claim, decided without the library: the claim (status, q) is true exactly when
   expected_r gives that status, and, for ADF_OK, when q is the first point. */
static void REST_UNUSED
expected_r(slong m, slong c, slong A, slong B, slong limit, int * st, slong * n1, slong * d1, slong * n2,
           slong * d2, slong * nel)
{
    solpt L[MAXP];
    slong Rp, Tp, R, T, aT, ell = limit > 0 ? limit : 0, X;
    int k;

    *n1 = *d1 = *n2 = *d2 = -1;
    *nel = 0;
    if (A < 0 || B < 1)
    {
        *st = ADF_NO_SOLUTION;
        return;
    }
    if (A >= m)
    {
        solpt L2[MAXP];

        *nel = enum_sol(m, c, A, B, L2);
        *st = ADF_NOT_UNIQUE;                     /* Proposition 1.6 (a) */
        *n1 = c % m;
        *d1 = 1;
        *n2 = c % m - m;
        *d2 = 1;
        return;
    }
    eea_pair_sl(m, c, A, &Rp, &Tp, &R, &T);
    aT = T < 0 ? -T : T;
    k = enum_sol(m, c, A, B, L);
    *nel = k;
    if (aT > B)
    {
        *st = ADF_NO_SOLUTION;                    /* Proposition 1.6 (b) */
        return;
    }
    if (2 * A * B < m)
    {
        /* Proposition 1.6 (c): no search, the answer does not depend on the limit */
        if (k == 1)
        {
            *st = ADF_OK;
            *n1 = L[0].n;
            *d1 = L[0].d;
        }
        else
            *st = ADF_NO_SOLUTION;
        return;
    }
    X = B / aT;
    rounds_of(L, k, m, c, A);
    {
        int found = k;

        while (found > 0 && L[found - 1].x > ell)
            found--;
        if (found >= 2)
            *st = ADF_NOT_UNIQUE;
        else if (found == 1)
            *st = (X > ell) ? ADF_NOT_DETERMINED : ADF_OK;
        else
            *st = (X > ell) ? ADF_NOT_DETERMINED : ADF_NO_SOLUTION;
    }
    if (k >= 1)
    {
        *n1 = L[0].n;
        *d1 = L[0].d;
    }
    if (k >= 2)
    {
        *n2 = L[1].n;
        *d2 = L[1].d;
    }
}

/* Is the claim (status, q) true? Decided by expected_r alone. */
static int REST_UNUSED
claim_true(slong m, slong c, slong A, slong B, slong limit, int status, slong qn, slong qd)
{
    int st;
    slong n1, d1, n2, d2, nel;

    expected_r(m, c, A, B, limit, &st, &n1, &d1, &n2, &d2, &nel);
    /* Repair of review s13: the verifier accepts the claim "adf_resid_reconstruct returned status for
       this limit (and q on OK)", so the claim is true exactly when the status of expected_r is the
       claimed one and, on OK, q is its first point. */
    if (status == ADF_OK)
        return st == ADF_OK && qn == n1 && qd == d1;
    if (status == ADF_NO_SOLUTION || status == ADF_NOT_UNIQUE || status == ADF_NOT_DETERMINED)
        return st == status;
    return 0;
}

/* One changed claim, checked: refused (0 from the library) or accepted and true by the enumeration. */
static void REST_UNUSED
check_changed(const adf_resid_t x, const fmpz_t fA, const fmpz_t fB, slong limit, int status,
              const adf_rat_t q, const adf_recon_cert_t cert, slong m, slong c, slong A, slong B,
              slong * refused, slong * accepted_true, const char * what)
{
    int got = adf_resid_verify_result(x, fA, fB, limit, status, q, cert);
    slong qn = fmpz_get_si(fmpq_numref(q->q));
    slong qd = fmpz_get_si(fmpq_denref(q->q));

    if (got)
    {
        ADF_CHECK_MSG(claim_true(m, c, A, B, limit, status, qn, qd),
                      "m = %ld, c = %ld, A = %ld, B = %ld, limit = %ld: the changed claim (%s, %ld/%ld) was "
                      "accepted and is false", m, c, A, B, limit, what, qn, qd);
        (*accepted_true)++;
    }
    else
        (*refused)++;
}

ADF_TEST(verify_result_accepts_every_result_and_refuses_changed_claims)
{
    adf_resid_t x;
    adf_rat_t q, q2;
    adf_recon_cert_t cert, cert2;
    fmpz_t fA, fB, t;
    static const slong limits[6] = {-1, 0, 1, 2, 5, 1000000000};
    static const int statuses[4] = {ADF_OK, ADF_NO_SOLUTION, ADF_NOT_UNIQUE, ADF_NOT_DETERMINED};
    slong m, c, A, B, bv[6], nbv, nb, li, n, d, n1, d1, n2, d2, nel, results, refused[6], acc_true[6];
    int st, rst, i, j;

    adf_resid_init(x);
    adf_rat_init(q);
    adf_rat_init(q2);
    adf_recon_cert_init(cert);
    adf_recon_cert_init(cert2);
    fmpz_init(fA);
    fmpz_init(fB);
    fmpz_init(t);
    results = 0;
    for (i = 0; i < 6; i++)
        refused[i] = acc_true[i] = 0;

    for (m = 1; m <= 24; m++)
    {
        for (c = 0; c < m; c++)
        {
            ADF_CHECK(mkresid(x, c, m) == ADF_OK);
            for (A = 0; A <= 2 * m; A++)
            {
                fmpz_set_si(fA, A);
                nbv = bvalues(m, bv);
                for (nb = 0; nb < nbv; nb++)
                {
                    B = bv[nb];
                    fmpz_set_si(fB, B);
                    for (li = 0; li < 6; li++)
                    {
                        st = adf_resid_reconstruct(q, cert, x, fA, fB, limits[li]);
                        ADF_CHECK(adf_recon_cert_is_canonical(cert) == 1);
                        expected_r(m, c, A, B, limits[li], &rst, &n1, &d1, &n2, &d2, &nel);
                        ADF_CHECK_MSG(st == rst, "m = %ld, c = %ld, A = %ld, B = %ld, limit = %ld: "
                                      "reconstruct gave %d, expected %d", m, c, A, B, limits[li], st, rst);
                        results++;
                        /* 1. the result of the function is accepted */
                        ADF_CHECK_MSG(adf_resid_verify_result(x, fA, fB, limits[li], st, q, cert) == 1,
                                      "m = %ld, c = %ld, A = %ld, B = %ld, limit = %ld: the result (%d) was "
                                      "not accepted", m, c, A, B, limits[li], st);
                        /* 2. the other three statuses */
                        for (i = 0; i < 4; i++)
                        {
                            if (statuses[i] == st)
                                continue;
                            check_changed(x, fA, fB, limits[li], statuses[i], q, cert, m, c, A, B,
                                          &refused[0], &acc_true[0], "another status");
                        }
                        /* 3. q with the numerator, the denominator or the sign changed */
                        rat_pair(q, &n, &d);
                        if (mkrat(q2, n + 1, d) == ADF_OK)
                            check_changed(x, fA, fB, limits[li], st, q2, cert, m, c, A, B, &refused[1],
                                          &acc_true[1], "numerator + 1");
                        if (mkrat(q2, n, d + 1) == ADF_OK)
                            check_changed(x, fA, fB, limits[li], st, q2, cert, m, c, A, B, &refused[2],
                                          &acc_true[2], "denominator + 1");
                        if (mkrat(q2, -n, d) == ADF_OK)
                            check_changed(x, fA, fB, limits[li], st, q2, cert, m, c, A, B, &refused[3],
                                          &acc_true[3], "sign");
                        /* 4. a certificate with one entry changed, and the kind flipped */
                        adf_recon_cert_set(cert2, cert);
                        if (cert2->kind == 1)
                        {
                            fmpz_set_si(t, m);
                            fmpz_add(cert2->Rp, cert2->Rp, t);   /* (C2) fails: |T Rp - Tp R| is no longer m */
                            check_changed(x, fA, fB, limits[li], st, q, cert2, m, c, A, B, &refused[4],
                                          &acc_true[4], "Rp + m");
                            adf_recon_cert_set(cert2, cert);
                            fmpz_neg(cert2->T, cert2->T);
                            check_changed(x, fA, fB, limits[li], st, q, cert2, m, c, A, B, &refused[4],
                                          &acc_true[4], "T negated");
                            adf_recon_cert_set(cert2, cert);
                            fmpz_add_ui(cert2->R, cert2->R, 1);
                            check_changed(x, fA, fB, limits[li], st, q, cert2, m, c, A, B, &refused[4],
                                          &acc_true[4], "R + 1");
                            adf_recon_cert_set(cert2, cert);
                            fmpz_neg(cert2->Tp, cert2->Tp);
                            check_changed(x, fA, fB, limits[li], st, q, cert2, m, c, A, B, &refused[4],
                                          &acc_true[4], "Tp negated");
                            adf_recon_cert_set(cert2, cert);
                            cert_set_si(cert2, 0, 0, 0, 0, 0);
                            check_changed(x, fA, fB, limits[li], st, q, cert2, m, c, A, B, &refused[5],
                                          &acc_true[5], "kind flipped to 0");
                        }
                        else
                        {
                            cert_set_si(cert2, 0, 0, 0, 0, 0);
                            check_changed(x, fA, fB, limits[li], st, q, cert2, m, c, A, B, &refused[5],
                                          &acc_true[5], "kind flipped to 0");
                        }
                        /* 5. a status outside the class of the function, and the empty box */
                        ADF_CHECK(adf_resid_verify_result(x, fA, fB, limits[li], ADF_LIMIT, q, cert) == 0);
                        ADF_CHECK(adf_resid_verify_result(x, fA, fB, limits[li], ADF_DOMAIN, q, cert) == 0);
                        ADF_CHECK(adf_resid_verify_result(x, fA, fB, limits[li], 12345, q, cert) == 0);
                        (void) j;
                        (void) t;
                    }
                }
            }
        }
    }
    printf("   verify_result: %ld results, all accepted; changed claims refused/accepted-and-true: "
           "status %ld/%ld, numerator %ld/%ld, denominator %ld/%ld, sign %ld/%ld, certificate %ld/%ld, "
           "kind %ld/%ld\n", results, (long) refused[0], (long) acc_true[0], (long) refused[1],
           (long) acc_true[1], (long) refused[2], (long) acc_true[2], (long) refused[3], (long) acc_true[3],
           (long) refused[4], (long) acc_true[4], (long) refused[5], (long) acc_true[5]);
    /* every kind of change must have been refused at least once */
    for (i = 0; i < 6; i++)
        ADF_CHECK_MSG(refused[i] > 0, "no changed claim of kind %d was refused", i);

    fmpz_clear(t);
    fmpz_clear(fB);
    fmpz_clear(fA);
    adf_recon_cert_clear(cert2);
    adf_recon_cert_clear(cert);
    adf_rat_clear(q2);
    adf_rat_clear(q);
    adf_resid_clear(x);
}

/* ---- 5b. verify_result on the vectors, on the large operands ---- */

ADF_TEST(verify_result_on_the_vectors)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    adf_resid_t x;
    adf_rat_t q, qs;
    adf_recon_cert_t cert;
    fmpz_t fm, fc, fA, fB, X, aT;
    size_t i, accepted = 0, undecided = 0, bigX = 0, changed_refused = 0, changed_true = 0;
    int st, est;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s3-slice3/recon_first.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    adf_resid_init(x);
    adf_rat_init(q);
    adf_rat_init(qs);
    adf_recon_cert_init(cert);
    fmpz_init(fm);
    fmpz_init(fc);
    fmpz_init(fA);
    fmpz_init(fB);
    fmpz_init(X);
    fmpz_init(aT);
    adf_rat_set_si(qs, -12345);

    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        const jsonl_value *v;
        slong limit;
        int got, xbig;

        ADF_CHECK(jsonl_field(rec, "m", &v, &err) == 1 && fmpz_set_str(fm, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "c", &v, &err) == 1 && fmpz_set_str(fc, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "A", &v, &err) == 1 && fmpz_set_str(fA, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "B", &v, &err) == 1 && fmpz_set_str(fB, jsonl_int_text(v, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "limit", &v, &err) == 1);
        limit = strtol(jsonl_int_text(v, &err), NULL, 10);
        ADF_CHECK(adf_resid_set_fmpz2(x, fc, fm) == ADF_OK);
        adf_rat_set(q, qs);
        st = adf_resid_reconstruct(q, cert, x, fA, fB, limit);
        /* the status of adf_resid_reconstruct, which is what the checker is asked about */
        if (st == ADF_NOT_UNIQUE)
        {
            /* review s13, finding 4: this branch asserts what a NOT_UNIQUE line says. q is untouched,
               the certificate is the one of the vector, the verifier accepts the claim and refuses OK
               with the first solution of the vector (the set has at least two elements). */
            const jsonl_value *cv, *nv, *dv;
            adf_rat_t first;
            fmpz_t fn2, fd2;

            ADF_CHECK_MSG(fmpz_equal_si(fmpq_numref(q->q), -12345) && fmpz_is_one(fmpq_denref(q->q)),
                          "line %lu: q was written on NOT_UNIQUE", (unsigned long) i + 1);
            ADF_CHECK(jsonl_field(rec, "cert", &cv, &err) == 1);
            if (jsonl_is_null(cv, &err) == 1)
                ADF_CHECK_MSG(cert->kind == 0, "line %lu: the file has no certificate, the call gave kind %d",
                              (unsigned long) i + 1, cert->kind);
            else
            {
                ADF_CHECK_MSG(cert->kind == 1 && jsonl_size(cv) == 4, "line %lu: certificate kind %d",
                              (unsigned long) i + 1, cert->kind);
                if (cert->kind == 1 && jsonl_size(cv) == 4)
                {
                    fmpz_t e;

                    fmpz_init(e);
                    ADF_CHECK(fmpz_set_str(e, jsonl_int_text(jsonl_at(cv, 0, &err), &err), 10) == 0);
                    ADF_CHECK_MSG(fmpz_equal(e, cert->Rp), "line %lu: Rp differs", (unsigned long) i + 1);
                    ADF_CHECK(fmpz_set_str(e, jsonl_int_text(jsonl_at(cv, 1, &err), &err), 10) == 0);
                    ADF_CHECK_MSG(fmpz_equal(e, cert->Tp), "line %lu: Tp differs", (unsigned long) i + 1);
                    ADF_CHECK(fmpz_set_str(e, jsonl_int_text(jsonl_at(cv, 2, &err), &err), 10) == 0);
                    ADF_CHECK_MSG(fmpz_equal(e, cert->R), "line %lu: R differs", (unsigned long) i + 1);
                    ADF_CHECK(fmpz_set_str(e, jsonl_int_text(jsonl_at(cv, 3, &err), &err), 10) == 0);
                    ADF_CHECK_MSG(fmpz_equal(e, cert->T), "line %lu: T differs", (unsigned long) i + 1);
                    fmpz_clear(e);
                }
            }
            ADF_CHECK_MSG(adf_resid_verify_result(x, fA, fB, limit, ADF_NOT_UNIQUE, q, cert) == 1,
                          "line %lu: the verifier refuses the NOT_UNIQUE result", (unsigned long) i + 1);
            ADF_CHECK(jsonl_field(rec, "n", &nv, &err) == 1);
            ADF_CHECK(jsonl_field(rec, "d", &dv, &err) == 1);
            adf_rat_init(first);
            fmpz_init(fn2);
            fmpz_init(fd2);
            ADF_CHECK_MSG(jsonl_is_null(nv, &err) == 0, "line %lu: no first solution in the file",
                          (unsigned long) i + 1);
            if (jsonl_is_null(nv, &err) == 0)
            {
                ADF_CHECK(fmpz_set_str(fn2, jsonl_int_text(nv, &err), 10) == 0);
                ADF_CHECK(fmpz_set_str(fd2, jsonl_int_text(dv, &err), 10) == 0);
                fmpz_swap(fmpq_numref(first->q), fn2);
                fmpz_swap(fmpq_denref(first->q), fd2);
                ADF_CHECK_MSG(adf_resid_verify_result(x, fA, fB, limit, ADF_OK, first, cert) == 0,
                              "line %lu: the verifier accepts OK with the first solution although a "
                              "second exists", (unsigned long) i + 1);
            }
            fmpz_clear(fn2);
            fmpz_clear(fd2);
            adf_rat_clear(first);
        }
        ADF_CHECK(jsonl_field(rec, "status", &v, &err) == 1);
        est = status_of(v, "recon_first.jsonl", &err);
        /* the status of the file is the status of recon_first; Algorithm R gives NOT_UNIQUE where
           recon_first gives OK with count 2, so the comparison is with the same function: the file
           says OK with a count of 2 exactly where Algorithm R says NOT_UNIQUE. */
        if (est == ADF_OK)
        {
            /* recon_first reports OK with a count; Algorithm R, which is what adf_resid_reconstruct
               returns and what the checker is asked about, returns NOT_UNIQUE for count 2,
               NOT_DETERMINED for count 0 (the search was cut after one solution) and OK for count 1 */
            long cnt;

            ADF_CHECK(jsonl_field(rec, "count", &v, &err) == 1);
            cnt = strtol(jsonl_int_text(v, &err), NULL, 10);
            est = (cnt == 2) ? ADF_NOT_UNIQUE : ((cnt == 0) ? ADF_NOT_DETERMINED : ADF_OK);
        }
        ADF_CHECK_MSG(st == est, "line %lu: reconstruct gave %d, the file says %d", (unsigned long) i + 1, st,
                      est);
        got = adf_resid_verify_result(x, fA, fB, limit, st, q, cert);
        /* X = floor(B/|T|) from the certificate, to know when the complete enumeration is impossible */
        xbig = 0;
        if (cert->kind == 1)
        {
            fmpz_abs(aT, cert->T);
            if (fmpz_sgn(aT) > 0)
            {
                fmpz_fdiv_q(X, fB, aT);
                xbig = (fmpz_cmp_si(X, WORD_MAX) > 0);
            }
        }
        if (xbig)
            bigX++;
        if (got)
            accepted++;
        else
        {
            undecided++;
            /* the only reason for a true claim not to be decided here is a complete enumeration whose
               round counter is above a word; a cut claim (NOT_DETERMINED) is always decided */
            ADF_CHECK_MSG(0, "line %lu (status %d): a result of adf_resid_reconstruct was not accepted",
                          (unsigned long) i + 1, st);
        }
        /* a changed claim: the status of the file where it differs, and the q of the file */
        if (st == ADF_NOT_UNIQUE)
        {
            int other = (st == ADF_OK) ? ADF_NOT_UNIQUE : ADF_OK;

            if (adf_resid_verify_result(x, fA, fB, limit, other, q, cert) == 0)
                changed_refused++;
            else
                changed_true++;
        }
    }
    printf("   verify_result on the vectors: %lu lines; accepted %lu, not decided %lu (of which %lu have "
           "X above a word); a status flipped: refused %lu, accepted and true %lu\n",
           (unsigned long) jsonl_count(f), (unsigned long) accepted, (unsigned long) undecided,
           (unsigned long) bigX, (unsigned long) changed_refused, (unsigned long) changed_true);
    ADF_CHECK(jsonl_count(f) > 10000);
    ADF_CHECK(accepted + undecided == jsonl_count(f));
    ADF_CHECK(accepted > 10000);
    ADF_CHECK(undecided == 0);             /* every result is accepted, X above a word or not */
    ADF_CHECK(bigX > 100);                 /* the large operands of the file really have X above a word */
    ADF_CHECK(changed_refused > 0);
    ADF_CHECK(changed_true == 0);          /* a NOT_UNIQUE claim cannot be an OK claim as well */

    fmpz_clear(aT);
    fmpz_clear(X);
    fmpz_clear(fB);
    fmpz_clear(fA);
    fmpz_clear(fc);
    fmpz_clear(fm);
    adf_recon_cert_clear(cert);
    adf_rat_clear(qs);
    adf_rat_clear(q);
    adf_resid_clear(x);
    jsonl_close(f);
}

/* ---- 5c. the case X above a word: what the checker decides and what it cannot ---- */

ADF_TEST(verify_result_when_X_is_above_a_word)
{
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
    fmpz_t fA, fB, X, aT;
    int st;

    /* m = 5, c = 3, A = 2: the pair of Lemma 1.4 is (3, 1, 2, -1), and the two points of the round
       x = 1 are (-2, 1) (y = 0) and 1/2 (y = 1), both solutions of Definition 1.1. |T| = 1, so
       X = floor(B/|T|) = B, which is taken above a word: the complete enumeration of Proposition 1.5
       (4) cannot be run in one call, since its round counter is a word. */
    adf_resid_init(x);
    adf_rat_init(q);
    adf_recon_cert_init(cert);
    fmpz_init(fA);
    fmpz_init(fB);
    fmpz_init(X);
    fmpz_init(aT);
    ADF_CHECK(mkresid(x, 3, 5) == ADF_OK);
    fmpz_set_si(fA, 2);
    fmpz_set_str(fB, "1180591620717411303424", 10);        /* 2^70 */
    fmpz_set_si(X, 1);
    fmpz_mul_2exp(X, X, 70);
    ADF_CHECK(fmpz_cmp_si(X, WORD_MAX) > 0);
    ADF_CHECK(fmpz_cmp(fB, X) == 0);

    /* limit 1: the second solution is in the first round, so Algorithm R returns NOT_UNIQUE at once
       and X never matters */
    st = adf_resid_reconstruct(q, cert, x, fA, fB, 1);
    ADF_CHECK(st == ADF_NOT_UNIQUE);
    ADF_CHECK(cert->kind == 1);
    fmpz_abs(aT, cert->T);
    fmpz_fdiv_q(X, fB, aT);
    ADF_CHECK_MSG(fmpz_cmp_si(X, WORD_MAX) > 0, "X = %s fits in a word", fmpz_get_str(NULL, 10, X));
    /* the claim is what adf_resid_reconstruct returned; the cut search of at most min(1, X) = 1 round
       decides it (repair of review s13, finding 2), X above a word is no obstacle */
    ADF_CHECK_MSG(adf_resid_verify_result(x, fA, fB, 1, ADF_NOT_UNIQUE, q, cert) == 1,
                  "the claim NOT_UNIQUE with X above a word and limit 1 was not accepted");
    /* the two solutions, tested directly as P1.11 (1) does */
    ADF_CHECK(mkrat(q, -2, 1) == ADF_OK);
    ADF_CHECK(adf_resid_contains_rat(x, q) == 1);
    ADF_CHECK(mkrat(q, 1, 2) == ADF_OK);
    ADF_CHECK(adf_resid_contains_rat(x, q) == 1);

    /* limit 0: the search is cut before any round, so Algorithm R returns NOT_DETERMINED, and the
       four conditions of Proposition 1.7 (3) are decided without any enumeration: X > ell is a
       comparison of integers of any size, and the cut search runs no round */
    st = adf_resid_reconstruct(q, cert, x, fA, fB, 0);
    ADF_CHECK(st == ADF_NOT_DETERMINED);
    ADF_CHECK_MSG(adf_resid_verify_result(x, fA, fB, 0, ADF_NOT_DETERMINED, q, cert) == 1,
                  "the claim NOT_DETERMINED with X above a word and limit 0 was not accepted");
    /* a NO_SOLUTION claim on the same problem is false and is refused (the two solutions are found) */
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 0, ADF_NO_SOLUTION, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 1, ADF_OK, q, cert) == 0);

    /* with B small enough for X to fit in a word, both claims are decided */
    fmpz_set_si(fB, 2);
    st = adf_resid_reconstruct(q, cert, x, fA, fB, 1000000);
    ADF_CHECK(st == ADF_NOT_UNIQUE);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 1000000, ADF_NOT_UNIQUE, q, cert) == 1);
    st = adf_resid_reconstruct(q, cert, x, fA, fB, 0);
    ADF_CHECK(st == ADF_NOT_DETERMINED);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 0, ADF_NOT_DETERMINED, q, cert) == 1);
    st = adf_resid_reconstruct(q, cert, x, fA, fB, 1);
    ADF_CHECK(st == ADF_NOT_UNIQUE);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 1, ADF_NOT_UNIQUE, q, cert) == 1);

    fmpz_clear(aT);
    fmpz_clear(X);
    fmpz_clear(fB);
    fmpz_clear(fA);
    adf_recon_cert_clear(cert);
    adf_rat_clear(q);
    adf_resid_clear(x);
}

ADF_TEST(verify_result_outside_the_problem)
{
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
    fmpz_t fA, fB;

    /* an empty box: the only claim is NO_SOLUTION, whatever the residue and the certificate */
    adf_resid_init(x);
    adf_rat_init(q);
    adf_recon_cert_init(cert);
    fmpz_init(fA);
    fmpz_init(fB);
    ADF_CHECK(mkresid(x, 3, 5) == ADF_OK);
    fmpz_set_si(fA, -1);
    fmpz_set_si(fB, 5);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NO_SOLUTION, q, cert) == 1);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_OK, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NOT_UNIQUE, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NOT_DETERMINED, q, cert) == 0);
    fmpz_set_si(fA, 2);
    fmpz_set_si(fB, 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NO_SOLUTION, q, cert) == 1);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NOT_UNIQUE, q, cert) == 0);

    /* m < 1 is not a residue: the type excludes it, and the only claim is DOMAIN */
    fmpz_set_si(fB, 5);
    fmpz_zero(x->m);
    ADF_CHECK(adf_resid_is_canonical(x) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_DOMAIN, q, cert) == 1);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NO_SOLUTION, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_OK, q, cert) == 0);
    fmpz_set_si(x->m, -3);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_DOMAIN, q, cert) == 1);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 10, ADF_NOT_UNIQUE, q, cert) == 0);
    /* and a residue that is canonical is answered normally again */
    fmpz_one(x->m);

    fmpz_clear(fB);
    fmpz_clear(fA);
    adf_recon_cert_clear(cert);
    adf_rat_clear(q);
    adf_resid_clear(x);
}
