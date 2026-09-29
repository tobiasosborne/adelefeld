/* resid.c: adf_resid and partial rational reconstruction in the range 2 A B < m (slice 1).

   The contract is the comment block of include/adelefeld/resid.h: docs/api-s.md sections 1 and 2,
   docs/proofs/solvers.md Definition 1.1 (line 76), Definition 1.3 (line 121), Lemma 1.4
   (line 133), Proposition 1.6 (line 210); docs/conventions.md 4.3 (outputs untouched on a status
   other than ADF_OK). The oracle of the reference is proto/solvers_checks.py, recon_partial
   (line 193); the tests are tests/test_resid.c.

   The algorithm of adf_resid_reconstruct, with its proof steps:
   1. A < 0 or B < 1: the box of Definition 1.1 is empty, so there is no solution (decision
      S-D5). No certificate exists: kind = 0.
   2. 2 A B >= m is the range of slice 2: ADF_UNSUPPORTED, nothing written. 2 A B is an fmpz.
   3. Otherwise 2 A B < m with B >= 1, so A <= A B < m: the hypothesis 0 <= A < m of Lemma 1.4.
      Let b = c mod m, and (r_i, t_i) the sequence of (EEA) (docs/proofs/solvers.md:26) for
      (m, b): r_0 = m, t_0 = 0, r_1 = b, t_1 = 1, r_(i+1) = r_(i-1) - floor(r_(i-1)/r_i) r_i,
      t_(i+1) = t_(i-1) - floor(r_(i-1)/r_i) t_i. The loop stops at the first j with r_j <= A;
      it exists (Lemma 1.4 (1): the remainders decrease to 0 <= A) and (R', T', R, T) =
      (r_(j-1), t_(j-1), r_j, t_j) is a certificate pair (Lemma 1.4 (2)). Only the column t is
      kept; the column s is not needed by any statement used here.
   4. |T| > B: no solution (Proposition 1.6 (b)). Else, since 2 A B < m, Proposition 1.6 (c)
      applies: gcd(R, T) = 1 gives the only solution (sigma R, |T|), sigma the sign of T, and
      gcd(R, T) > 1 gives no solution. The row is NOT divided by the gcd (Remark 1 of 1.6): the
      reduced point would not lie in the lattice. */

#include <adelefeld.h>

/* ---- life cycle (conventions 2.3) ---- */

void
adf_resid_init(adf_resid_t x)
{
    fmpz_init(x->c);
    fmpz_init_set_ui(x->m, 1);
}

void
adf_resid_clear(adf_resid_t x)
{
    fmpz_clear(x->c);
    fmpz_clear(x->m);
}

/* adf_resid_set_fmpz2(x, c, m): c reduced into [0, m) by a floor modulus (fmpz_mod, a result in
   [0, m) for m > 0). The result goes through a temporary and is swapped in, so that c and m may
   be the members of x. */
int
adf_resid_set_fmpz2(adf_resid_t x, const fmpz_t c, const fmpz_t m)
{
    fmpz_t t, mm;

    if (fmpz_sgn(m) < 1)
        return ADF_DOMAIN;
    fmpz_init(t);
    fmpz_init_set(mm, m);
    fmpz_mod(t, c, mm);
    fmpz_swap(x->c, t);
    fmpz_swap(x->m, mm);
    fmpz_clear(t);
    fmpz_clear(mm);
    return ADF_OK;
}

/* adf_resid_get_fmpz2(c, m, x): through temporaries, so that c and m may be members of x. */
void
adf_resid_get_fmpz2(fmpz_t c, fmpz_t m, const adf_resid_t x)
{
    fmpz_t tc, tm;

    fmpz_init_set(tc, x->c);
    fmpz_init_set(tm, x->m);
    fmpz_swap(c, tc);
    fmpz_swap(m, tm);
    fmpz_clear(tc);
    fmpz_clear(tm);
}

int
adf_resid_is_canonical(const adf_resid_t x)
{
    return fmpz_sgn(x->m) >= 1 && fmpz_sgn(x->c) >= 0 && fmpz_cmp(x->c, x->m) < 0;
}

void
adf_recon_cert_init(adf_recon_cert_t cert)
{
    fmpz_init(cert->Rp);
    fmpz_init(cert->Tp);
    fmpz_init(cert->R);
    fmpz_init(cert->T);
    cert->kind = 0;
}

void
adf_recon_cert_clear(adf_recon_cert_t cert)
{
    fmpz_clear(cert->Rp);
    fmpz_clear(cert->Tp);
    fmpz_clear(cert->R);
    fmpz_clear(cert->T);
}

/* ---- reconstruction ---- */

/* Write "no certificate": kind = 0 and the four integers 0 (the predicate of adf_recon_cert). */
static void
cert_set_none(adf_recon_cert_t cert)
{
    fmpz_zero(cert->Rp);
    fmpz_zero(cert->Tp);
    fmpz_zero(cert->R);
    fmpz_zero(cert->T);
    cert->kind = 0;
}

int
adf_resid_reconstruct(adf_rat_t q, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A,
                      const fmpz_t B, slong limit)
{
    fmpz_t r0, r1, t0, t1, quo, r2, w;
    int status;

    (void) limit;                                   /* slice 1: no search, see the header */

    /* step 1: the box is empty (S-D5) */
    if (fmpz_sgn(A) < 0 || fmpz_sgn(B) < 1)
    {
        if (cert != NULL)
            cert_set_none(cert);
        return ADF_NO_SOLUTION;
    }

    /* step 2: 2 A B >= m is the range of slice 2 */
    fmpz_init(w);
    fmpz_mul(w, A, B);
    fmpz_mul_2exp(w, w, 1);
    if (fmpz_cmp(w, x->m) >= 0)
    {
        fmpz_clear(w);
        return ADF_UNSUPPORTED;
    }

    /* step 3: the Euclidean algorithm on (m, c mod m), Lemma 1.4; A < m here */
    fmpz_init_set(r0, x->m);
    fmpz_init(r1);
    fmpz_mod(r1, x->c, x->m);
    fmpz_init(t0);
    fmpz_init_set_ui(t1, 1);
    fmpz_init(quo);
    fmpz_init(r2);
    while (fmpz_cmp(r1, A) > 0)                     /* r1 > A >= 0, so r1 > 0 */
    {
        fmpz_fdiv_qr(quo, r2, r0, r1);
        fmpz_submul(t0, quo, t1);                   /* t0 - quo t1 */
        fmpz_swap(t0, t1);                          /* (t0, t1) = (t1, t0 - quo t1) */
        fmpz_swap(r0, r1);                          /* (r0, r1) = (r1, r2) */
        fmpz_swap(r1, r2);
    }
    /* now (R', T', R, T) = (r0, t0, r1, t1) */

    if (cert != NULL)
    {
        fmpz_set(cert->Rp, r0);
        fmpz_set(cert->Tp, t0);
        fmpz_set(cert->R, r1);
        fmpz_set(cert->T, t1);
        cert->kind = 1;
    }

    if (fmpz_cmpabs(t1, B) > 0)                     /* 1.6 (b) */
        status = ADF_NO_SOLUTION;
    else
    {
        fmpz_gcd(w, r1, t1);                        /* gcd(0, T) = |T| */
        if (!fmpz_is_one(w))                        /* 1.6 (c), gcd > 1 */
            status = ADF_NO_SOLUTION;
        else                                        /* 1.6 (c), gcd = 1: sigma R / |T| */
        {
            fmpz_set(fmpq_numref(q->q), r1);
            fmpz_abs(fmpq_denref(q->q), t1);
            if (fmpz_sgn(t1) < 0)
                fmpz_neg(fmpq_numref(q->q), fmpq_numref(q->q));
            status = ADF_OK;
        }
    }

    fmpz_clear(r0);
    fmpz_clear(r1);
    fmpz_clear(t0);
    fmpz_clear(t1);
    fmpz_clear(quo);
    fmpz_clear(r2);
    fmpz_clear(w);
    return status;
}
