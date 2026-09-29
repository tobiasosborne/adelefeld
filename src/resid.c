/* resid.c: adf_resid and partial rational reconstruction (slices 1 and 2 of milestone S).

   The contract is the comment block of include/adelefeld/resid.h: docs/api-s.md sections 1 and 2,
   docs/proofs/solvers.md Definition 1.1 (line 75), Definition 1.3 (line 117), Lemma 1.4 (line 133),
   Propositions 1.5 (line 162), 1.6 (line 216), Algorithm R and Proposition 1.7 (line 256);
   docs/conventions.md 4.3 (outputs untouched on a status other than ADF_OK). The oracle of the
   reference is proto/solvers_checks.py, recon_partial (line 193); the tests are tests/test_resid.c
   and tests/test_resid_full.c.

   The algorithm of adf_resid_reconstruct is Algorithm R (solvers.md:258 to 271), with the proof steps:
   1. A < 0 or B < 1: the box of Definition 1.1 is empty, so there is no solution (decision S-D5).
      No certificate exists: kind = 0.
   2. A >= m: ADF_NOT_UNIQUE (Proposition 1.6 (a)). No certificate (the pair needs A < m): kind = 0.
   3. Otherwise 0 <= A < m. Let b = c mod m, and (r_i, t_i) the sequence of (EEA) (solvers.md:26) for
      (m, b): r_0 = m, t_0 = 0, r_1 = b, t_1 = 1, r_(i+1) = r_(i-1) - floor(r_(i-1)/r_i) r_i,
      t_(i+1) = t_(i-1) - floor(r_(i-1)/r_i) t_i. The loop stops at the first j with r_j <= A;
      it exists (Lemma 1.4 (1): the remainders decrease to 0 <= A) and (R', T', R, T) =
      (r_(j-1), t_(j-1), r_j, t_j) is a certificate pair (Lemma 1.4 (2)). Only the column t is
      kept; the column s is not needed by any statement used here.
   4. |T| > B: no solution (Proposition 1.6 (b)).
   5. 2 A B < m: Proposition 1.6 (c) applies: gcd(R, T) = 1 gives the only solution (sigma R, |T|),
      sigma the sign of T, and gcd(R, T) > 1 gives no solution. The row is NOT divided by the gcd
      (Remark 1 of 1.6): the reduced point would not lie in the lattice.
   6. Otherwise the search, resid_search below (Algorithm R steps 7 and 8, Proposition 1.7). */

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

/* adf_recon_cert_check(cert, x, A): (C1) to (C4) of Definition 1.3 (docs/proofs/solvers.md:117) with the four
   multiplications c T, c T', T R', T' R and two reductions (here two divisibility tests, which need no reduced
   copy). Nothing else is assumed of the quadruple; an x with m < 1 (not canonical) is refused. */
int
adf_recon_cert_check(const adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A)
{
    fmpz_t t, u;
    int ok = 1;

    if (cert->kind != 1 || fmpz_sgn(x->m) < 1)
        return 0;
    fmpz_init(t);
    fmpz_init(u);
    /* C1: R = c T and R' = c T' modulo m */
    fmpz_mul(t, x->c, cert->T);
    fmpz_sub(t, cert->R, t);
    ok = ok && fmpz_divisible(t, x->m);
    fmpz_mul(t, x->c, cert->Tp);
    fmpz_sub(t, cert->Rp, t);
    ok = ok && fmpz_divisible(t, x->m);
    /* C2: |T R' - T' R| = m */
    fmpz_mul(t, cert->T, cert->Rp);
    fmpz_mul(u, cert->Tp, cert->R);
    fmpz_sub(t, t, u);
    ok = ok && fmpz_cmpabs(t, x->m) == 0;
    /* C3: 0 <= R <= A < R' */
    ok = ok && fmpz_sgn(cert->R) >= 0 && fmpz_cmp(cert->R, A) <= 0 && fmpz_cmp(A, cert->Rp) < 0;
    /* C4: T is not 0 and T T' <= 0 */
    ok = ok && !fmpz_is_zero(cert->T);
    fmpz_mul(t, cert->T, cert->Tp);
    ok = ok && fmpz_sgn(t) <= 0;
    fmpz_clear(t);
    fmpz_clear(u);
    return ok;
}

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

/* resid_search(fn, fd, ...): Algorithm R steps 7 and 8 (solvers.md:266 to 271) for A < m <= 2 A B,
   |T| <= B, so X = floor(B/|T|) >= 1. Rounds x = 1, ..., min(X, ell); in the round x the points of
   Proposition 1.5 (1) are phi(x, y) with y an integer in [max(0, ceil((x R - A)/R')), floor((x R + A)/R')]
   (Proposition 1.5 (3); R' > A >= 0 so R' >= 1 and the division is defined), found by two divisions,
   not by a loop over y; d = x |T| + y |T'| grows with y, so the loop over y stops at the first d > B.
   A point is counted if gcd(n, d) = 1 (Proposition 1.6 (d)). Two counted points: ADF_NOT_UNIQUE, at once
   (also when the search would have been cut: two solutions are proved, 1.5 (1) makes the points
   distinct). After the rounds: X > ell gives ADF_NOT_DETERMINED (1.7 (3)); else ADF_OK with the point
   found (in fn, fd), or ADF_NO_SOLUTION. X is an fmpz and is compared with ell as an fmpz; it is
   converted to a word only when X <= ell, and then it fits. The counter of the rounds is a word i
   (at most ell <= WORD_MAX); x |T| and x R are kept as fmpz by addition. */
static int
resid_search(fmpz_t fn, fmpz_t fd, const fmpz_t Rp, const fmpz_t Tp, const fmpz_t R, const fmpz_t T,
             const fmpz_t A, const fmpz_t B, const fmpz_t X, slong ell)
{
    int complete = fmpz_cmp_si(X, ell) <= 0;        /* X <= ell: the loop visits every round */
    slong rounds = complete ? fmpz_get_si(X) : ell; /* min(X, ell) */
    int sigma = fmpz_sgn(T) < 0 ? -1 : 1;
    int found = 0, status = -1;
    slong i = 0;
    fmpz_t aT, aTp, xT, xR, w, ylo, yhi, y, d, n, g;

    fmpz_init(aT);
    fmpz_init(aTp);
    fmpz_init(xT);
    fmpz_init(xR);
    fmpz_init(w);
    fmpz_init(ylo);
    fmpz_init(yhi);
    fmpz_init(y);
    fmpz_init(d);
    fmpz_init(n);
    fmpz_init(g);
    fmpz_abs(aT, T);
    fmpz_abs(aTp, Tp);
    while (i < rounds && status < 0)
    {
        i++;
        fmpz_add(xT, xT, aT);
        fmpz_add(xR, xR, R);
        fmpz_sub(w, xR, A);
        fmpz_cdiv_q(ylo, w, Rp);                    /* ceil((x R - A)/R') */
        if (fmpz_sgn(ylo) < 0)
            fmpz_zero(ylo);
        fmpz_add(w, xR, A);
        fmpz_fdiv_q(yhi, w, Rp);                    /* floor((x R + A)/R') */
        for (fmpz_set(y, ylo); fmpz_cmp(y, yhi) <= 0; fmpz_add_ui(y, y, 1))
        {
            fmpz_mul(d, y, aTp);
            fmpz_add(d, d, xT);                     /* d = x |T| + y |T'| */
            if (fmpz_cmp(d, B) > 0)
                break;
            fmpz_mul(n, y, Rp);
            fmpz_sub(n, xR, n);                     /* x R - y R' */
            if (sigma < 0)
                fmpz_neg(n, n);
            fmpz_gcd(g, n, d);
            if (!fmpz_is_one(g))
                continue;
            if (found == 0)
            {
                fmpz_set(fn, n);
                fmpz_set(fd, d);
                found = 1;
            }
            else
            {
                status = ADF_NOT_UNIQUE;
                break;
            }
        }
    }
    if (status < 0)
    {
        if (!complete)
            status = ADF_NOT_DETERMINED;
        else
            status = found ? ADF_OK : ADF_NO_SOLUTION;
    }
    fmpz_clear(aT);
    fmpz_clear(aTp);
    fmpz_clear(xT);
    fmpz_clear(xR);
    fmpz_clear(w);
    fmpz_clear(ylo);
    fmpz_clear(yhi);
    fmpz_clear(y);
    fmpz_clear(d);
    fmpz_clear(n);
    fmpz_clear(g);
    return status;
}

int
adf_resid_reconstruct(adf_rat_t q, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A,
                      const fmpz_t B, slong limit)
{
    fmpz_t r0, r1, t0, t1, quo, r2, w;
    int status;

    /* step 1: the box is empty (S-D5) */
    if (fmpz_sgn(A) < 0 || fmpz_sgn(B) < 1)
    {
        if (cert != NULL)
            cert_set_none(cert);
        return ADF_NO_SOLUTION;
    }

    /* step 2: A >= m, Proposition 1.6 (a) */
    if (fmpz_cmp(A, x->m) >= 0)
    {
        if (cert != NULL)
            cert_set_none(cert);
        return ADF_NOT_UNIQUE;
    }

    /* step 3: the Euclidean algorithm on (m, c mod m), Lemma 1.4; 0 <= A < m here */
    fmpz_init_set(r0, x->m);
    fmpz_init(r1);
    fmpz_mod(r1, x->c, x->m);
    fmpz_init(t0);
    fmpz_init_set_ui(t1, 1);
    fmpz_init(quo);
    fmpz_init(r2);
    fmpz_init(w);
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

    fmpz_mul(w, A, B);                              /* 2 A B is an fmpz, never a word */
    fmpz_mul_2exp(w, w, 1);
    if (fmpz_cmpabs(t1, B) > 0)                     /* step 4: 1.6 (b) */
        status = ADF_NO_SOLUTION;
    else if (fmpz_cmp(w, x->m) < 0)                 /* step 5: 1.6 (c) */
    {
        fmpz_gcd(w, r1, t1);                        /* gcd(0, T) = |T| */
        if (!fmpz_is_one(w))                        /* gcd > 1 */
            status = ADF_NO_SOLUTION;
        else                                        /* gcd = 1: sigma R / |T| */
        {
            fmpz_set(fmpq_numref(q->q), r1);
            fmpz_abs(fmpq_denref(q->q), t1);
            if (fmpz_sgn(t1) < 0)
                fmpz_neg(fmpq_numref(q->q), fmpq_numref(q->q));
            status = ADF_OK;
        }
    }
    else                                            /* step 6: A < m <= 2 A B, |T| <= B */
    {
        fmpz_t X, sn, sd;

        fmpz_init(X);
        fmpz_init(sn);
        fmpz_init(sd);
        fmpz_abs(X, t1);
        fmpz_fdiv_q(X, B, X);                       /* X = floor(B/|T|) >= 1 */
        status = resid_search(sn, sd, r0, t0, r1, t1, A, B, X, limit > 0 ? limit : 0);
        if (status == ADF_OK)
        {
            fmpz_swap(fmpq_numref(q->q), sn);
            fmpz_swap(fmpq_denref(q->q), sd);
        }
        fmpz_clear(X);
        fmpz_clear(sn);
        fmpz_clear(sd);
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
