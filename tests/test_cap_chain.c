/* tests/test_cap_chain.c: the cap along a chain, the capped chain against the tight chain, and
   the cap on small radii by enumeration (the three chain tests of lane m1-cap-chain).

   Ground truth, all read on disk:
   - docs/proofs/policies.md section 3: Definition 13 (line 253: after a tight operation with
     result c + R Zhat, R > 0, the radius is replaced by gcd(R, C)), Proposition 14 (line 258:
     1 the old ball is inside the new one, 2 an exact result keeps its tag, 3 gcd(R, C) is the
     finest radius dividing both), Proposition 15 (line 283: 1 every capped radius R > 0 has
     R | C, 2 the cap is idempotent, 3 the sum of two capped values needs no capping,
     4 products and products with exact scalars can produce radii not dividing C).
   - docs/SPEC.md 4.4 item 3 (line 277): the radius R is replaced by gcd(R, C); the cap never
     touches an exact value.
   - docs/conventions.md 5.4, paragraph "The absolute cap" (line 520): R divides C, the sum of
     two capped values needs no cap.
   - tests/ref/adfref/policies.py:150 absolute_cap (the reference cap: exact kept, otherwise
     from_center_radius(centre, qgcd(radius, C))).
   - tests/ref/adfref/rat.py:32 qgcd (the non-negative generator of the subgroup of Q generated
     by its arguments: the common denominator L, the gcd of the numerators over L). The test
     recomputes that gcd with fmpz alone, so it does not simply repeat the fmpq_gcd call of
     src/cap.c.
   - Enclosure: docs/proofs/policies.md Theorem 3 (line 75) with Lemma 1 of the same file
     (line 43): the tight result is the smallest ball containing the result set, so every member
     of a result set lies in it, and every result of fball.h contains the result set of its
     inputs.

   The three tests:
   1. chain_invariant: 50 random capped operations (add, sub, mul, mul_rat, the operation of
      each step drawn at random, so the chain is mixed), three seeds and the four caps
      1, 1/6, 12 and 2^120 * 3^50 (a 200-bit cap). After every step: the status, predicate G,
      the invariant of Proposition 15.1 (the radius divides C, checked as C/R an integer),
      the idempotence of Proposition 15.2, Proposition 15.3 (a sum or difference of two capped
      values is already capped: the result is identical to the tight one), and a count of the
      steps of Proposition 15.4 in which the cap really acted.
   2. chain_contains_tight: the same random expression of 50 steps, evaluated once with the
      tight operations of fball.h and once with the capped operations; after every step
      adf_fball_contains(tight, capped), and the radius of the capped value is the rational gcd
      of the radius of the tight result of the capped inputs with C (0 when that tight result
      is exact, and then the capped value is that exact result). The order of the arguments of
      the verb is the one of docs/SPEC.md 4.2 (line 223), "first inside second": the cap makes
      the ball larger (policies.md Proposition 14.1), so it is the tight value that lies inside
      the capped one, not the other way round.
   3. enumeration_small_radii: for small balls, all members of the inputs inside a window, and
      every sum, every product and every product with a fixed exact scalar of two members lies
      in the corresponding capped result. Done for the bare inputs and for the capped inputs.

   Nothing here weakens a check: a wrong implementation of the cap must fail an assertion. */

#include <stdio.h>

#include <adelefeld/scaled.h>

#include "test_runner.h"

#define CHAIN_STEPS 50
#define SEEDS 3
#define NCAPS 4

/* The four operations of a step, drawn at random. */
enum { OP_ADD = 0, OP_SUB = 1, OP_MUL = 2, OP_MULRAT = 3, OP_COUNT = 4 };

static const char *const op_name[OP_COUNT] = { "add", "sub", "mul", "mul_rat" };

/* --------------------------------------------------------------- helpers */

/* The tight operation of fball.h for one step. For OP_MULRAT the second operand is ignored and
   the exact scalar q is used. */
static void
apply_tight(int op, adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t q)
{
    switch (op)
    {
    case OP_ADD:
        adf_fball_add(z, x, y);
        break;
    case OP_SUB:
        adf_fball_sub(z, x, y);
        break;
    case OP_MUL:
        adf_fball_mul(z, x, y);
        break;
    default:
        adf_fball_mul_rat(z, x, q);
        break;
    }
}

/* The capped operation of scaled.h for one step. Returns the status of the call. */
static int
apply_cap(int op, adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t q,
          const adf_rat_t C)
{
    switch (op)
    {
    case OP_ADD:
        return adf_fball_add_cap(z, x, y, C);
    case OP_SUB:
        return adf_fball_sub_cap(z, x, y, C);
    case OP_MUL:
        return adf_fball_mul_cap(z, x, y, C);
    default:
        return adf_fball_mul_rat_cap(z, x, q, C);
    }
}

static void
mkrat_si(adf_rat_t q, slong num, slong den)
{
    fmpq_set_si(q->q, num, den);
}

static void
mkball_si(adf_fball_t x, slong A, slong H, slong d)
{
    fmpz_t a, h, dd;

    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(dd, d);
    ADF_CHECK(adf_fball_set_fmpz3(x, a, h, dd) == ADF_OK);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
}

/* A printable copy of a rational, with four rotating buffers, for failure messages. */
static const char *
rs(const adf_rat_t q)
{
    static char *buf[4] = { NULL, NULL, NULL, NULL };
    static int next = 0;
    char *s;

    next = (next + 1) % 4;
    if (buf[next] != NULL)
        flint_free(buf[next]);
    s = fmpq_get_str(NULL, 10, q->q);
    buf[next] = s;
    return s;
}

/* 1 if the rational b divides a, that is a/b is a positive integer. This is the reading of
   R | C of Proposition 15.1 (R divides C: C/R is an integer) with a in place of C. */
static int
rat_divides(const adf_rat_t a, const adf_rat_t b)
{
    fmpq_t q;
    int r;

    fmpq_init(q);
    fmpq_div(q, a->q, b->q);
    r = (fmpq_sgn(q) > 0) && fmpz_is_one(fmpq_denref(q));
    fmpq_clear(q);
    return r;
}

/* The rational gcd of two positive rationals, computed as in the reference
   tests/ref/adfref/rat.py:32 qgcd: L = lcm of the two denominators, the gcd of the two
   numerators over L, then reduced. Deliberately not fmpq_gcd, so that a wrong call in
   src/cap.c cannot be masked by the same wrong call here. */
static void
qgcd2(adf_rat_t g, const adf_rat_t a, const adf_rat_t b)
{
    fmpz_t L, t, u, num, den;

    fmpz_init(L);
    fmpz_init(t);
    fmpz_init(u);
    fmpz_init(num);
    fmpz_init(den);

    fmpz_lcm(L, fmpq_denref(a->q), fmpq_denref(b->q));
    fmpz_divexact(t, L, fmpq_denref(a->q));
    fmpz_mul(t, t, fmpq_numref(a->q));
    fmpz_divexact(u, L, fmpq_denref(b->q));
    fmpz_mul(u, u, fmpq_numref(b->q));
    fmpz_gcd(num, t, u);
    fmpz_set(den, L);
    fmpq_set_fmpz_frac(g->q, num, den);

    fmpz_clear(L);
    fmpz_clear(t);
    fmpz_clear(u);
    fmpz_clear(num);
    fmpz_clear(den);
}

/* A random ball: centre num/den with num in -8..8 and den in 1..4, radius rnum/rden with rnum in
   0..12 and rden in 1..3 (a radius 0 gives an exact value). If wide_bits > 0, with probability
   1/4 the radius is 2^e, e in 0..wide_bits, so that a cap of about 200 bits has something to
   act on. The value is built with adf_fball_set_center_radius and is canonical by
   construction. */
static void
rand_ball(adf_fball_t x, flint_rand_t st, ulong wide_bits)
{
    adf_rat_t c, N;
    slong num, den, rnum, rden;

    fmpq_init(c->q);
    fmpq_init(N->q);

    den = 1 + (slong) n_randint(st, 4);
    num = (slong) n_randint(st, 17) - 8;
    fmpq_set_si(c->q, num, den);

    if (wide_bits > 0 && n_randint(st, 4) == 0)
    {
        fmpz_t r;

        fmpz_init(r);
        fmpz_one(r);
        fmpz_mul_2exp(r, r, n_randint(st, wide_bits + 1));
        fmpq_set_fmpz(N->q, r);
        fmpz_clear(r);
    }
    else
    {
        rden = 1 + (slong) n_randint(st, 3);
        rnum = (slong) n_randint(st, 13);
        fmpq_set_si(N->q, rnum, rden);
    }

    ADF_CHECK(adf_fball_set_center_radius(x, c, N) == ADF_OK);
    fmpq_clear(c->q);
    fmpq_clear(N->q);
}

/* A random exact scalar in -6..6 over 1..4, with 0 and small fractions among the values. */
static void
rand_rat(adf_rat_t q, flint_rand_t st)
{
    slong num, den;

    den = 1 + (slong) n_randint(st, 4);
    num = (slong) n_randint(st, 13) - 6;
    fmpq_set_si(q->q, num, den);
}

/* The four caps: 1, 1/6, 12, and 2^120 * 3^50, a cap of 200 bits (120 + 50 log2 3 = 199.25). */
static void
make_caps(adf_rat_t caps[NCAPS])
{
    fmpz_t p;
    int i;

    for (i = 0; i < NCAPS; i++)
        fmpq_init(caps[i]->q);

    fmpq_one(caps[0]->q);
    fmpq_set_si(caps[1]->q, 1, 6);
    fmpq_set_si(caps[2]->q, 12, 1);

    fmpz_init(p);
    fmpz_one(p);
    fmpz_mul_2exp(p, p, 120);
    for (i = 0; i < 50; i++)
        fmpz_mul_ui(p, p, 3);
    fmpq_set_fmpz(caps[3]->q, p);
    fmpz_clear(p);
}

static void
clear_caps(adf_rat_t caps[NCAPS])
{
    int i;

    for (i = 0; i < NCAPS; i++)
        fmpq_clear(caps[i]->q);
}

/* ------------------------------------------- 1. the invariant along a chain of 50 steps */

/* Proposition 15 along a chain of 50 random capped operations. All the values of the chain are
   capped values: the start and every fresh operand are passed through adf_fball_cap first, so
   that item 3 (the sum of two capped values needs no cap) can be checked. */
ADF_TEST(chain_invariant)
{
    adf_rat_t caps[NCAPS];
    long total_acted = 0;
    int ci;

    make_caps(caps);

    for (ci = 0; ci < NCAPS; ci++)
    {
        int si;
        long acted_of_cap = 0;

        for (si = 0; si < SEEDS; si++)
        {
            flint_rand_t st;
            adf_fball_t v, w, wc, t, t2, z2;
            adf_rat_t q, R;
            long acted = 0, steps_of_kind[OP_COUNT] = { 0, 0, 0, 0 };
            int step;

            flint_randinit(st);
            flint_randseed(st, 1000 + (ulong) si, 17 + (ulong) ci);

            adf_fball_init(v);
            adf_fball_init(w);
            adf_fball_init(wc);
            adf_fball_init(t);
            adf_fball_init(t2);
            adf_fball_init(z2);
            fmpq_init(q->q);
            fmpq_init(R->q);

            /* The start of the chain is a capped value. */
            rand_ball(v, st, 0);
            ADF_CHECK(adf_fball_cap(v, v, caps[ci]) == ADF_OK);

            for (step = 0; step < CHAIN_STEPS; step++)
            {
                int op = (int) n_randint(st, OP_COUNT);
                int status;

                rand_ball(w, st, 0);
                ADF_CHECK(adf_fball_cap(wc, w, caps[ci]) == ADF_OK);
                rand_rat(q, st);
                steps_of_kind[op]++;

                status = apply_cap(op, t, v, wc, q, caps[ci]);
                ADF_CHECK_MSG(status == ADF_OK, "cap %d seed %d step %d op %s: status %d", ci, si,
                              step, op_name[op], status);
                ADF_CHECK_MSG(adf_fball_is_canonical(t), "cap %d seed %d step %d op %s: not canonical",
                              ci, si, step, op_name[op]);

                /* Proposition 15.1: a capped radius R > 0 divides C. */
                if (!adf_fball_is_exact(t))
                {
                    adf_fball_get_radius(R, t);
                    ADF_CHECK_MSG(rat_divides(caps[ci], R),
                                  "cap %d seed %d step %d op %s: radius %s does not divide C %s",
                                  ci, si, step, op_name[op], rs(R), rs(caps[ci]));
                }

                /* Proposition 15.2: the cap is idempotent on a capped value. */
                ADF_CHECK(adf_fball_cap(z2, t, caps[ci]) == ADF_OK);
                ADF_CHECK_MSG(adf_fball_identical(z2, t), "cap %d seed %d step %d op %s: not idempotent",
                              ci, si, step, op_name[op]);

                /* The tight result of the same step, for items 3 and 4. */
                apply_tight(op, t2, v, wc, q);
                ADF_CHECK(adf_fball_is_canonical(t2));

                if (op == OP_ADD || op == OP_SUB)
                {
                    /* Proposition 15.3: gcd(R1, R2) divides C, so the cap changes nothing. */
                    ADF_CHECK_MSG(adf_fball_identical(t2, t),
                                  "cap %d seed %d step %d op %s: the sum of two capped values was changed",
                                  ci, si, step, op_name[op]);
                }
                else
                {
                    /* Proposition 15.4: a product (or a product with an exact scalar) can have a
                       radius that does not divide C; there the cap acts. */
                    if (!adf_fball_is_exact(t2))
                    {
                        adf_fball_get_radius(R, t2);
                        if (!rat_divides(caps[ci], R))
                        {
                            acted++;
                            ADF_CHECK_MSG(!adf_fball_identical(t2, t),
                                          "cap %d seed %d step %d op %s: the cap did not act on radius %s",
                                          ci, si, step, op_name[op], rs(R));
                        }
                    }
                }

                adf_fball_set(v, t);
            }

            /* The four operations must really have occurred in a chain of this length. The number
               of steps in which the cap acted on a product is a property of the random draws, not
               of the specification, so it is summed over the seeds of a cap and only required to
               be positive: it says that item 4 of Proposition 15 was met. */
            {
                int i;

                for (i = 0; i < OP_COUNT; i++)
                    ADF_CHECK_MSG(steps_of_kind[i] > 0, "cap %d seed %d: no %s step in %d steps", ci,
                                  si, op_name[i], CHAIN_STEPS);
            }
            acted_of_cap += acted;
            total_acted += acted;

            adf_fball_clear(v);
            adf_fball_clear(w);
            adf_fball_clear(wc);
            adf_fball_clear(t);
            adf_fball_clear(t2);
            adf_fball_clear(z2);
            fmpq_clear(q->q);
            fmpq_clear(R->q);
            flint_randclear(st);
        }

        ADF_CHECK_MSG(acted_of_cap > 0, "cap %d: the cap never acted on a product in %d chains", ci,
                      SEEDS);
    }

    ADF_CHECK_MSG(total_acted > 0, "the cap never acted on a product in any chain");
    clear_caps(caps);
}

/* ------------------------------ 2. the capped chain against the tight chain, step by step */

/* The same random expression of 50 steps, once all tight (fball.h) and once all capped. The
   fresh operand w of a step is the same ball in both chains, so the two chains are the same
   expression read at two precisions. After every step: adf_fball_contains(capped, tight), and
   the radius of the capped value is gcd(radius of the tight result of the capped inputs, C). */
ADF_TEST(chain_contains_tight)
{
    adf_rat_t caps[NCAPS];
    long total_acted = 0;
    int ci;

    make_caps(caps);

    for (ci = 0; ci < NCAPS; ci++)
    {
        int si;

        for (si = 0; si < SEEDS; si++)
        {
            flint_rand_t st;
            adf_fball_t vc, vt, w, t, tc, t2;
            adf_rat_t q, R, Rc, g;
            long acted = 0;
            int step;

            flint_randinit(st);
            flint_randseed(st, 1000 + (ulong) si, 17 + (ulong) ci);

            adf_fball_init(vc);
            adf_fball_init(vt);
            adf_fball_init(w);
            adf_fball_init(t);
            adf_fball_init(tc);
            adf_fball_init(t2);
            fmpq_init(q->q);
            fmpq_init(R->q);
            fmpq_init(Rc->q);
            fmpq_init(g->q);

            /* Both chains start from the same capped value. */
            rand_ball(vc, st, 0);
            ADF_CHECK(adf_fball_cap(vc, vc, caps[ci]) == ADF_OK);
            adf_fball_set(vt, vc);

            for (step = 0; step < CHAIN_STEPS; step++)
            {
                int op = (int) n_randint(st, OP_COUNT);
                int status;

                rand_ball(w, st, (ci == 3) ? 200 : 60);
                rand_rat(q, st);

                /* t = the tight result of the same operation on the capped inputs. */
                apply_tight(op, t, vc, w, q);
                status = apply_cap(op, tc, vc, w, q, caps[ci]);
                ADF_CHECK_MSG(status == ADF_OK, "cap %d seed %d step %d op %s: status %d", ci, si,
                              step, op_name[op], status);
                ADF_CHECK(adf_fball_is_canonical(tc));

                /* The radius of the capped value is gcd(radius(t), C), 0 for an exact t. */
                if (adf_fball_is_exact(t))
                {
                    adf_fball_get_center(R, t);
                    ADF_CHECK_MSG(adf_fball_is_exact(tc),
                                  "cap %d seed %d step %d op %s: the exact value %s was widened",
                                  ci, si, step, op_name[op], rs(R));
                    ADF_CHECK_MSG(adf_fball_identical(tc, t),
                                  "cap %d seed %d step %d op %s: the exact value %s was changed",
                                  ci, si, step, op_name[op], rs(R));
                }
                else
                {
                    adf_fball_get_radius(R, t);
                    qgcd2(g, R, caps[ci]);
                    adf_fball_get_radius(Rc, tc);
                    ADF_CHECK_MSG(fmpq_equal(Rc->q, g->q),
                                  "cap %d seed %d step %d op %s: tight radius %s, gcd(%s, C) = %s, "
                                  "capped radius %s", ci, si, step, op_name[op], rs(R), rs(R), rs(g),
                                  rs(Rc));
                    acted++;
                }

                /* Theorem 3 and Proposition 14.1 on the capped inputs: the tight result c + R Zhat
                   is inside the capped result c + gcd(R, C) Zhat. The verb of the library is
                   "first inside second" (docs/SPEC.md 4.2, line 223), so the order is
                   contains(tight, capped). */
                ADF_CHECK_MSG(adf_fball_contains(t, tc),
                              "cap %d seed %d step %d op %s: the tight result is not inside the "
                              "capped value", ci, si, step, op_name[op]);

                /* The same expression, evaluated all tight: its value is inside the capped one. */
                apply_tight(op, t2, vt, w, q);
                ADF_CHECK(adf_fball_is_canonical(t2));
                ADF_CHECK_MSG(adf_fball_contains(t2, tc),
                              "cap %d seed %d step %d op %s: the all-tight chain left the capped chain",
                              ci, si, step, op_name[op]);

                adf_fball_set(vc, tc);
                adf_fball_set(vt, t2);
            }

            ADF_CHECK_MSG(acted > 0, "cap %d seed %d: every step of the chain was exact", ci, si);
            total_acted += acted;

            adf_fball_clear(vc);
            adf_fball_clear(vt);
            adf_fball_clear(w);
            adf_fball_clear(t);
            adf_fball_clear(tc);
            adf_fball_clear(t2);
            fmpq_clear(q->q);
            fmpq_clear(R->q);
            fmpq_clear(Rc->q);
            fmpq_clear(g->q);
            flint_randclear(st);
        }
    }

    ADF_CHECK_MSG(total_acted > 0, "no step of any chain had a positive radius");
    clear_caps(caps);
}

/* ------------------------------------- 3. enumeration of members, small radii only */

/* Every member of a set a + N Zhat in a window of N K is one of a, a + N, ..., a + (K-1) N, up
   to a shift of the window: the set is N-periodic, so K consecutive members lie in every
   window of K periods. The test uses the stored centre a, which is itself a member. */
#define NMEM 8

static void
members_of(const adf_fball_t x, adf_rat_t mem[NMEM])
{
    adf_rat_t a, N, k, t;
    int i;

    fmpq_init(a->q);
    fmpq_init(N->q);
    fmpq_init(k->q);
    fmpq_init(t->q);
    for (i = 0; i < NMEM; i++)
        fmpq_init(mem[i]->q);

    adf_fball_get_center(a, x);
    adf_fball_get_radius(N, x);

    for (i = 0; i < NMEM; i++)
    {
        fmpq_set_si(k->q, i, 1);
        fmpq_mul(t->q, N->q, k->q);
        fmpq_add(mem[i]->q, a->q, t->q);
    }

    fmpq_clear(a->q);
    fmpq_clear(N->q);
    fmpq_clear(k->q);
    fmpq_clear(t->q);
}

/* All members of x and of y in a window, and their sums, products and scalar products, must lie
   in the capped results. Every sum of a member of x with a member of y is a member of the
   result set of x + y, which the tight result of fball.h contains (docs/proofs/policies.md
   Theorem 3, line 75, with Lemma 1, line 43, of the same file), and the cap only makes the ball
   larger (Proposition 14.1). */
ADF_TEST(enumeration_small_radii)
{
    /* Balls (A, H, d): radius H/d between 0 and 4, centre A/d with d in 1..3. */
    static const slong vals[][3] = {
        { 0, 1, 1 }, { 1, 2, 1 }, { 2, 6, 1 }, { 1, 3, 1 }, { 3, 4, 1 },
        { 5, 12, 1 }, { 0, 5, 1 }, { 1, 1, 1 }, { 2, 1, 1 }, { 1, 5, 2 },
        { 3, 2, 2 }, { 7, 4, 3 }, { 2, 0, 3 }, { 0, 0, 1 }, { 5, 0, 2 }
    };
    const size_t nvals = sizeof(vals) / sizeof(vals[0]);
    static const slong scalars[][2] = { { 3, 2 }, { -5, 4 }, { 0, 1 }, { 7, 1 } };
    const size_t nscal = sizeof(scalars) / sizeof(scalars[0]);
    adf_rat_t caps[NCAPS];
    size_t i, j, k, l;
    int ci;

    make_caps(caps);

    for (ci = 0; ci < NCAPS; ci++)
    {
        adf_fball_t x, y, cx, cy, zadd, zmul, zrat;
        adf_rat_t mx[NMEM], my[NMEM], s, p, q;
        int variant;

        adf_fball_init(x);
        adf_fball_init(y);
        adf_fball_init(cx);
        adf_fball_init(cy);
        adf_fball_init(zadd);
        adf_fball_init(zmul);
        adf_fball_init(zrat);
        for (k = 0; k < NMEM; k++)
        {
            fmpq_init(mx[k]->q);
            fmpq_init(my[k]->q);
        }
        fmpq_init(s->q);
        fmpq_init(p->q);
        fmpq_init(q->q);

        for (i = 0; i < nvals; i++)
        {
            mkball_si(x, vals[i][0], vals[i][1], vals[i][2]);
            ADF_CHECK(adf_fball_cap(cx, x, caps[ci]) == ADF_OK);

            for (j = 0; j < nvals; j++)
            {
                mkball_si(y, vals[j][0], vals[j][1], vals[j][2]);
                ADF_CHECK(adf_fball_cap(cy, y, caps[ci]) == ADF_OK);

                /* Variant 0: the bare inputs. Variant 1: the capped inputs, whose radii divide C
                   and are the members that the cap produces. */
                for (variant = 0; variant < 2; variant++)
                {
                    const adf_fball_srcptr inx = variant ? cx : x;
                    const adf_fball_srcptr iny = variant ? cy : y;

                    members_of(inx, mx);
                    members_of(iny, my);

                    ADF_CHECK(adf_fball_add_cap(zadd, inx, iny, caps[ci]) == ADF_OK);
                    ADF_CHECK(adf_fball_mul_cap(zmul, inx, iny, caps[ci]) == ADF_OK);
                    ADF_CHECK(adf_fball_is_canonical(zadd));
                    ADF_CHECK(adf_fball_is_canonical(zmul));

                    for (k = 0; k < nscal; k++)
                    {
                        mkrat_si(q, scalars[k][0], scalars[k][1]);
                        ADF_CHECK(adf_fball_mul_rat_cap(zrat, inx, q, caps[ci]) == ADF_OK);
                        ADF_CHECK(adf_fball_is_canonical(zrat));

                        for (l = 0; l < NMEM; l++)
                        {
                            fmpq_mul(p->q, mx[l]->q, q->q);
                            ADF_CHECK_MSG(adf_fball_contains_rat(zrat, p),
                                          "cap %d v%d x=(%ld,%ld,%ld) q=%s member %s: "
                                          "the scalar product is not in the capped result",
                                          ci, variant, (long) vals[i][0], (long) vals[i][1],
                                          (long) vals[i][2], rs(q), rs(p));
                        }
                    }

                    for (k = 0; k < NMEM; k++)
                    {
                        for (l = 0; l < NMEM; l++)
                        {
                            fmpq_add(s->q, mx[k]->q, my[l]->q);
                            ADF_CHECK_MSG(adf_fball_contains_rat(zadd, s),
                                          "cap %d v%d x=(%ld,%ld,%ld) y=(%ld,%ld,%ld): "
                                          "the sum %s is not in the capped result", ci, variant,
                                          (long) vals[i][0], (long) vals[i][1], (long) vals[i][2],
                                          (long) vals[j][0], (long) vals[j][1], (long) vals[j][2], rs(s));

                            fmpq_mul(p->q, mx[k]->q, my[l]->q);
                            ADF_CHECK_MSG(adf_fball_contains_rat(zmul, p),
                                          "cap %d v%d x=(%ld,%ld,%ld) y=(%ld,%ld,%ld): "
                                          "the product %s is not in the capped result", ci, variant,
                                          (long) vals[i][0], (long) vals[i][1], (long) vals[i][2],
                                          (long) vals[j][0], (long) vals[j][1], (long) vals[j][2], rs(p));
                        }
                    }
                }
            }
        }

        adf_fball_clear(x);
        adf_fball_clear(y);
        adf_fball_clear(cx);
        adf_fball_clear(cy);
        adf_fball_clear(zadd);
        adf_fball_clear(zmul);
        adf_fball_clear(zrat);
        for (k = 0; k < NMEM; k++)
        {
            fmpq_clear(mx[k]->q);
            fmpq_clear(my[k]->q);
        }
        fmpq_clear(s->q);
        fmpq_clear(p->q);
        fmpq_clear(q->q);
    }

    clear_caps(caps);
}
