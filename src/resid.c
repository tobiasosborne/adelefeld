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

/* adf_resid_set(y, x), adf_resid_swap(x, y), adf_resid_identical(x, y): conventions 2.3, the life-cycle row
   of the table. A value is the pair (c, m) and nothing else, so representation identity is a comparison of
   the two fields; the set P(m, c) of Proposition 1.10 depends on c only through c modulo m (Lemma 1.2 and
   Definition 1.1), so two values with c and c + k m are the same set but not identical, and the test below
   says so. set and swap are the FLINT ones (conventions 1, the fmpz.h row of the table), which support
   aliasing. */
void
adf_resid_set(adf_resid_t y, const adf_resid_t x)
{
    fmpz_set(y->c, x->c);
    fmpz_set(y->m, x->m);
}

void
adf_resid_swap(adf_resid_t x, adf_resid_t y)
{
    fmpz_swap(x->c, y->c);
    fmpz_swap(x->m, y->m);
}

int
adf_resid_identical(const adf_resid_t x, const adf_resid_t y)
{
    return fmpz_equal(x->c, y->c) && fmpz_equal(x->m, y->m);
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

/* adf_recon_cert_is_canonical, _set, _swap, _identical: conventions 2.3. The storage invariant of
   docs/api-s.md section 1 is "kind is 0 or 1, and for kind = 0 all four integers are 0"; nothing else is
   required of a kind = 1 value, since a quadruple that satisfies (C1) to (C4) of Definition 1.3 is
   checked by adf_recon_cert_check, not by the predicate. The predicate reads only the integers and the
   tag, so it never aborts (decision M1-D2). */
int
adf_recon_cert_is_canonical(const adf_recon_cert_t cert)
{
    if (cert->kind != 0 && cert->kind != 1)
        return 0;
    if (cert->kind == 0)
        return fmpz_is_zero(cert->Rp) && fmpz_is_zero(cert->Tp) && fmpz_is_zero(cert->R)
               && fmpz_is_zero(cert->T);
    return 1;
}

void
adf_recon_cert_set(adf_recon_cert_t y, const adf_recon_cert_t x)
{
    fmpz_set(y->Rp, x->Rp);
    fmpz_set(y->Tp, x->Tp);
    fmpz_set(y->R, x->R);
    fmpz_set(y->T, x->T);
    y->kind = x->kind;
}

void
adf_recon_cert_swap(adf_recon_cert_t x, adf_recon_cert_t y)
{
    fmpz_swap(x->Rp, y->Rp);
    fmpz_swap(x->Tp, y->Tp);
    fmpz_swap(x->R, y->R);
    fmpz_swap(x->T, y->T);
    if (x->kind != y->kind)
    {
        int k = x->kind;
        x->kind = y->kind;
        y->kind = k;
    }
}

int
adf_recon_cert_identical(const adf_recon_cert_t x, const adf_recon_cert_t y)
{
    return x->kind == y->kind && fmpz_equal(x->Rp, y->Rp) && fmpz_equal(x->Tp, y->Tp)
           && fmpz_equal(x->R, y->R) && fmpz_equal(x->T, y->T);
}

/* adf_resid_set_rat(x, q, m) and adf_resid_contains_rat(x, q): P(m, c) is the set of Proposition 1.10
   (solvers.md:386), with the set statement of Lemma 1.2 (solvers.md:85). q = n/d is reduced with d > 0
   (conventions 5.1), so
     q is in P(m, c) exactly when gcd(d, m) = 1 and n = c d modulo m.
   "Only if": Lemma 1.2 (1), with n = c d modulo m, m | n - c d, so every common divisor of d and m
   divides n and d and is 1. "If": the definition of P(m, c). If gcd(d, m) = 1 then d is invertible
   modulo m and c = n d^(-1) is the unique c in [0, m) with c d = n modulo m, so the residue exists
   exactly when the gcd is 1 and the DOMAIN of the header is the case in which it does not.
   m = 1 is read separately: the residue is 0 and P(1, 0) = Q, so every reduced rational is in it and no
   modular inverse is asked for. The costs are one gcd and one modular inverse (fmpz.h:572) of the sizes
   of n, d, m, and one gcd and one divisibility test for the predicate. */

int
adf_resid_set_rat(adf_resid_t x, const adf_rat_t q, const fmpz_t m)
{
    fmpz_t g, c;

    if (fmpz_sgn(m) < 1)
        return ADF_DOMAIN;
    fmpz_init(g);
    fmpz_init(c);
    fmpz_gcd(g, fmpq_denref(q->q), m);
    if (!fmpz_is_one(g))
    {
        fmpz_clear(g);
        fmpz_clear(c);
        return ADF_DOMAIN;                    /* q is in no P(m, c) (Lemma 1.2 (1)); x untouched */
    }
    if (fmpz_is_one(m))
        fmpz_zero(c);                         /* P(1, 0) = Q */
    else
    {
        fmpz_invmod(c, fmpq_denref(q->q), m); /* d^(-1) modulo m */
        fmpz_mul(c, c, fmpq_numref(q->q));
        fmpz_mod(c, c, m);                    /* c = n d^(-1) in [0, m) */
    }
    fmpz_swap(x->c, c);
    fmpz_set(x->m, m);
    fmpz_clear(g);
    fmpz_clear(c);
    return ADF_OK;
}

int
adf_resid_contains_rat(const adf_resid_t x, const adf_rat_t q)
{
    fmpz_t g, t;
    int inside;

    if (fmpz_sgn(x->m) < 1)
        return 0;                             /* not a residue: P(m, c) is not defined */
    fmpz_init(g);
    fmpz_init(t);
    fmpz_gcd(g, fmpq_denref(q->q), x->m);
    if (!fmpz_is_one(g))
        inside = 0;
    else
    {
        fmpz_mul(t, x->c, fmpq_denref(q->q));  /* c d */
        fmpz_sub(t, t, fmpq_numref(q->q));     /* c d - n */
        inside = fmpz_divisible(t, x->m) ? 1 : 0;
    }
    fmpz_clear(g);
    fmpz_clear(t);
    return inside;
}

/* adf_resid_set_fball_forget(x, b): Proposition 1.10 (4) (solvers.md:386 to 424, the claim and its proof).
   The canonical triple (A, H, d) of b is read with adf_fball_get_fmpz3 (fball.h:161), which recombines a
   local value, so the two backends take the same path from here on. Then:
   - H = 0: the ball is the single rational A/d. It is DOMAIN: P1.10 (4) states the claim for H >= 1, and
     for H = 0 there is no residue modulus in the value at all (the exact value says nothing about any
     prime), which is the DOMAIN the header names.
   - gcd(d, H) > 1: DOMAIN. The proof of 1.10 (4) needs d invertible modulo H; with a common divisor the
     rationals (A + H k)/d are in no P(H, c), since a reduced rational of P(H, c) has its denominator
     prime to H (Lemma 1.2 (1)) and (A + H k)/d has denominator with that divisor unless d divides A and
     H in full, which the canonical triple excludes (predicate G, fball.h:54 to 58: gcd(A, H, d) = 1).
   - otherwise c = A d^(-1) in [0, H) exists and is unique, and x = P(H, c) contains every rational of b.
   H = 1 is the same case with c = 0 and P(1, 0) = Q, read without a modular inverse. */
int
adf_resid_set_fball_forget(adf_resid_t x, const adf_fball_t b)
{
    fmpz_t A, H, d, g, c;
    int st = ADF_DOMAIN;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(g);
    fmpz_init(c);
    adf_fball_get_fmpz3(A, H, d, b);
    if (fmpz_sgn(H) < 1)                    /* exact: no radius, no modulus */
        goto done;
    fmpz_gcd(g, d, H);
    if (!fmpz_is_one(g))
        goto done;                           /* d is not invertible modulo H */
    if (fmpz_is_one(H))
        fmpz_zero(c);                        /* P(1, 0) = Q */
    else
    {
        fmpz_invmod(c, d, H);                /* d^(-1) modulo H */
        fmpz_mul(c, c, A);
        fmpz_mod(c, c, H);                   /* c = A d^(-1) in [0, H) */
    }
    fmpz_swap(x->c, c);
    fmpz_set(x->m, H);
    st = ADF_OK;
done:
    fmpz_clear(c);
    fmpz_clear(g);
    fmpz_clear(d);
    fmpz_clear(H);
    fmpz_clear(A);
    return st;
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

/* resid_solve(fn, fd, nfound, cert, x, A, B, limit): Algorithm R (solvers.md:256 to 324) with the steps of
   the comment at the head of this file. It returns the status of Algorithm R and, through *nfound, whether
   it found a solution: when *nfound is 1, (fn, fd) is the FIRST point in the order of Algorithm R (x
   increasing, and for each x the y increasing), which is what adf_resid_reconstruct_first reports. The
   search resid_search already leaves its first reduced point in (fn, fd) on ADF_OK, ADF_NOT_UNIQUE and on a
   cut search that found one (it writes there at the first hit and only reads it afterwards), so no
   argument of resid_search had to be changed; the pair of Proposition 1.6 (a) is written here, since that
   step leaves the search at once.
   adf_resid_reconstruct ignores (fn, fd) and *nfound and returns the status unchanged; the two functions
   share this one search. */
static int
resid_solve(fmpz_t fn, fmpz_t fd, int * nfound, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A,
            const fmpz_t B, slong limit)
{
    fmpz_t r0, r1, t0, t1, quo, r2, w;
    int status;

    *nfound = 0;

    /* step 1: the box is empty (S-D5) */
    if (fmpz_sgn(A) < 0 || fmpz_sgn(B) < 1)
    {
        if (cert != NULL)
            cert_set_none(cert);
        return ADF_NO_SOLUTION;
    }

    /* step 2: A >= m, Proposition 1.6 (a); the first of the two solutions is (c mod m, 1) */
    if (fmpz_cmp(A, x->m) >= 0)
    {
        if (cert != NULL)
            cert_set_none(cert);
        fmpz_mod(fn, x->c, x->m);
        fmpz_one(fd);
        *nfound = 1;
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
            fmpz_set(fn, r1);
            fmpz_abs(fd, t1);
            if (fmpz_sgn(t1) < 0)
                fmpz_neg(fn, fn);
            *nfound = 1;
            status = ADF_OK;
        }
    }
    else                                            /* step 6: A < m <= 2 A B, |T| <= B */
    {
        fmpz_t X;
        int nf;

        fmpz_init(X);
        fmpz_abs(X, t1);
        fmpz_fdiv_q(X, B, X);                       /* X = floor(B/|T|) >= 1 */
        status = resid_search(fn, fd, r0, t0, r1, t1, A, B, X, limit > 0 ? limit : 0);
        nf = (!fmpz_is_zero(fd));                   /* the search writes (fn, fd) at the first hit */
        *nfound = nf;
        fmpz_clear(X);
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

/* adf_resid_reconstruct: Algorithm R through resid_solve; the point it found is written on ADF_OK and
   on no other status (conventions 4.3). What this function returns and writes is what it returned and
   wrote before the search was shared with adf_resid_reconstruct_first: the same status, the same q, the
   same cert. */
int
adf_resid_reconstruct(adf_rat_t q, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A,
                      const fmpz_t B, slong limit)
{
    fmpz_t n, d;
    int nfound, status;

    fmpz_init(n);
    fmpz_init(d);
    status = resid_solve(n, d, &nfound, cert, x, A, B, limit);
    if (status == ADF_OK)
    {
        fmpz_swap(fmpq_numref(q->q), n);
        fmpz_swap(fmpq_denref(q->q), d);
    }
    fmpz_clear(n);
    fmpz_clear(d);
    return status;
}

/* adf_resid_reconstruct_first: the same search, and the reading of the first point. The mapping from the
   status of Algorithm R to the status and the count of decision S-D4 is the one of recon_first of
   proto/solvers_checks.py (line 230), which follows from Proposition 1.7: a status with a solution to
   report becomes ADF_OK with the first point; a cut search that found one is ADF_OK with count 0 (the
   solution is verified, the uniqueness is not, which count = 0 says); a cut search that found none is
   ADF_NOT_DETERMINED, which says nothing about existence; an empty set is ADF_NO_SOLUTION; two solutions
   found, or proved by Proposition 1.6 (a), are ADF_OK with count 2. */
int
adf_resid_reconstruct_first(adf_rat_t q, int * count, adf_recon_cert_t cert, const adf_resid_t x,
                            const fmpz_t A, const fmpz_t B, slong limit)
{
    fmpz_t n, d;
    int nfound, status, st;

    fmpz_init(n);
    fmpz_init(d);
    status = resid_solve(n, d, &nfound, cert, x, A, B, limit);
    if (status == ADF_OK)
    {
        *count = 1;                                /* the search was complete and found one point */
        st = ADF_OK;
    }
    else if (status == ADF_NOT_UNIQUE)
    {
        *count = 2;
        st = ADF_OK;
    }
    else if (status == ADF_NOT_DETERMINED && nfound)
    {
        *count = 0;                                /* cut after one solution: a solution, not a proof */
        st = ADF_OK;
    }
    else
    {
        *count = 0;
        st = status;
    }
    if (st == ADF_OK)
    {
        fmpz_swap(fmpq_numref(q->q), n);
        fmpz_swap(fmpq_denref(q->q), d);
    }
    fmpz_clear(n);
    fmpz_clear(d);
    return st;
}

/* The certificate pair of Lemma 1.4 for 0 <= A < m, computed as Algorithm R step 4 does; (Rp, Tp, R, T)
   is written. Returns 0 if the loop of step 4 does not end, which cannot happen for A < m. */
static int
resid_pair(fmpz_t Rp, fmpz_t Tp, fmpz_t R, fmpz_t T, const adf_resid_t x, const fmpz_t A)
{
    fmpz_t r0, r1, t0, t1, quo, r2;
    int ok = 1;

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
        fmpz_submul(t0, quo, t1);
        fmpz_swap(t0, t1);
        fmpz_swap(r0, r1);
        fmpz_swap(r1, r2);
    }
    if (fmpz_cmp(r1, A) > 0)
        ok = 0;
    fmpz_swap(Rp, r0);
    fmpz_swap(Tp, t0);
    fmpz_swap(R, r1);
    fmpz_swap(T, t1);
    fmpz_clear(r0);
    fmpz_clear(r1);
    fmpz_clear(t0);
    fmpz_clear(t1);
    fmpz_clear(quo);
    fmpz_clear(r2);
    return ok;
}

/* is_solution(x, A, B, n, d): Definition 1.1 for one pair, tested directly (P1.11 (1)). Every
   condition of the definition is tested, the condition gcd(d, m) = 1 among them, although Lemma 1.2
   (1) says it follows from the others. */
static int
is_solution(const adf_resid_t x, const fmpz_t A, const fmpz_t B, const fmpz_t n, const fmpz_t d)
{
    fmpz_t g, t;
    int ok;

    if (fmpz_sgn(d) < 1 || fmpz_cmp(d, B) > 0 || fmpz_cmpabs(n, A) > 0)
        return 0;
    fmpz_init(g);
    fmpz_init(t);
    fmpz_gcd(g, n, d);
    ok = fmpz_is_one(g);
    if (ok)
    {
        fmpz_gcd(g, d, x->m);
        ok = fmpz_is_one(g);
    }
    if (ok)
    {
        fmpz_mul(t, x->c, d);
        fmpz_sub(t, t, n);                          /* c d - n */
        ok = fmpz_divisible(t, x->m) ? 1 : 0;
    }
    fmpz_clear(g);
    fmpz_clear(t);
    return ok;
}

/* enumerate_all(fn, fd, Rp, Tp, R, T, A, B, X, ell, status): the search resid_search, so the complete
   enumeration of Proposition 1.5 (4) is the case ell = WORD_MAX. Returns what resid_search returns, and
   0 before it when the enumeration cannot be run: a pair with T = 0, which (C4) forbids and which
   resid_search cannot use, or a complete enumeration whose round counter X is above a word, since then
   the rounds could not all be visited in one call. A cut search (ell < WORD_MAX) runs min(X, ell)
   rounds and is bounded by ell, so X above a word is no obstacle there. The point found is written to
   (fn, fd). */
static int
enumerate_all(fmpz_t fn, fmpz_t fd, const fmpz_t Rp, const fmpz_t Tp, const fmpz_t R, const fmpz_t T,
              const fmpz_t A, const fmpz_t B, const fmpz_t X, slong ell, int * status)
{
    if (fmpz_is_zero(T) || (ell == WORD_MAX && fmpz_cmp_si(X, WORD_MAX) > 0))
    {
        *status = -1;
        return 0;
    }
    *status = resid_search(fn, fd, Rp, Tp, R, T, A, B, X, ell);
    return 1;
}

/* adf_resid_verify_result: the checker of Proposition 1.11, in the order of the header comment. */
int
adf_resid_verify_result(const adf_resid_t x, const fmpz_t A, const fmpz_t B, slong limit, int status,
                        const adf_rat_t q, const adf_recon_cert_t cert)
{
    fmpz_t Rp, Tp, R, T, X, w, g, fn, fd;
    slong ell = limit > 0 ? limit : 0;
    int st, ok = 0;

    if (fmpz_sgn(x->m) < 1)
        return status == ADF_DOMAIN;               /* not a residue; P1.11 (1), the first line */
    if (fmpz_sgn(A) < 0 || fmpz_sgn(B) < 1)
        return status == ADF_NO_SOLUTION;          /* the box is empty (S-D5), the only answer */

    fmpz_init(Rp);
    fmpz_init(Tp);
    fmpz_init(R);
    fmpz_init(T);
    fmpz_init(X);
    fmpz_init(w);
    fmpz_init(g);
    fmpz_init(fn);
    fmpz_init(fd);

    if (status == ADF_NOT_UNIQUE)
    {
        /* P1.11 (1) with the complete enumeration (the second way of note 4 of api-s.md section 2):
           two solutions must be found. A >= m is proved by Proposition 1.6 (a) and needs no search.
           The certificate is not read: the claim is about the set, not about the pair. */
        if (fmpz_cmp(A, x->m) >= 0)
            ok = 1;
        else if (resid_pair(Rp, Tp, R, T, x, A) && !fmpz_is_zero(T) && fmpz_cmpabs(T, B) <= 0)
        {
            fmpz_mul(w, A, B);
            fmpz_mul_2exp(w, w, 1);                 /* w = 2 A B */
            fmpz_abs(X, T);
            fmpz_fdiv_q(X, B, X);                  /* X = floor(B/|T|) */
            if (fmpz_cmp(w, x->m) >= 0)            /* else 1.6 (c): at most one solution */
                ok = enumerate_all(fn, fd, Rp, Tp, R, T, A, B, X, WORD_MAX, &st) && st == ADF_NOT_UNIQUE;
        }
        goto done;
    }

    if (status != ADF_OK && status != ADF_NO_SOLUTION && status != ADF_NOT_DETERMINED)
        goto done;                                 /* any other status is not a claim of P1.11 */

    /* P1.11 (2): a quadruple that satisfies (C1) to (C4); the check also refuses A >= m and kind 0 */
    if (!adf_recon_cert_check(cert, x, A))
        goto done;
    fmpz_set(Rp, cert->Rp);
    fmpz_set(Tp, cert->Tp);
    fmpz_set(R, cert->R);
    fmpz_set(T, cert->T);                          /* (C4) gives T != 0, so X >= 1 below */
    fmpz_mul(w, A, B);
    fmpz_mul_2exp(w, w, 1);                         /* w = 2 A B */
    fmpz_abs(X, T);
    fmpz_fdiv_q(X, B, X);                           /* X = floor(B/|T|) */

    if (status == ADF_NO_SOLUTION)
    {
        if (fmpz_cmpabs(T, B) > 0)
            ok = 1;                                /* Proposition 1.6 (b) */
        else if (fmpz_cmp(w, x->m) < 0)             /* Proposition 1.6 (c) */
        {
            fmpz_gcd(g, R, T);
            ok = !fmpz_is_one(g);
        }
        else                                        /* Proposition 1.6 (d): the complete enumeration */
            ok = enumerate_all(fn, fd, Rp, Tp, R, T, A, B, X, WORD_MAX, &st) && st == ADF_NO_SOLUTION;
        goto done;
    }

    if (status == ADF_OK)
    {
        if (!is_solution(x, A, B, fmpq_numref(q->q), fmpq_denref(q->q)))
            goto done;                             /* q must be a solution of Definition 1.1 */
        if (fmpz_cmpabs(T, B) > 0)
            goto done;                             /* then there is no solution at all (1.6 (b)) */
        if (fmpz_cmp(w, x->m) < 0)                  /* Proposition 1.6 (c): the only solution is the row */
        {
            fmpz_set(g, R);
            if (fmpz_sgn(T) < 0)
                fmpz_neg(g, g);                     /* sigma R */
            fmpz_abs(fd, T);
            ok = fmpz_equal(fmpq_numref(q->q), g) && fmpz_equal(fmpq_denref(q->q), fd);
        }
        else                                        /* Proposition 1.6 (d) */
            ok = enumerate_all(fn, fd, Rp, Tp, R, T, A, B, X, WORD_MAX, &st) && st == ADF_OK
                 && fmpz_equal(fn, fmpq_numref(q->q)) && fmpz_equal(fd, fmpq_denref(q->q));
        goto done;
    }

    /* ADF_NOT_DETERMINED: the four conditions of Proposition 1.7 (3), the last one by the search */
    if (fmpz_cmp(w, x->m) < 0 || fmpz_cmpabs(T, B) > 0 || fmpz_cmp_si(X, ell) <= 0)
        goto done;
    ok = enumerate_all(fn, fd, Rp, Tp, R, T, A, B, X, ell, &st) && st == ADF_NOT_DETERMINED;

done:
    fmpz_clear(fd);
    fmpz_clear(fn);
    fmpz_clear(g);
    fmpz_clear(w);
    fmpz_clear(X);
    fmpz_clear(T);
    fmpz_clear(R);
    fmpz_clear(Tp);
    fmpz_clear(Rp);
    return ok;
}
