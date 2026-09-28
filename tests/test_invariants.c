/* tests/test_invariants.c: the entry check of the debug build -DADF_CHECK_INVARIANTS
   (docs/conventions.md 4.4, row "Non-canonical value passed to a public function", line 288, and
   DECISION CV-09; docs/SPEC.md 15, row M1-D2; findings R3 of docs/reviews/m1/local/review.md and R7
   of docs/reviews/m1/text/review.md; the list lanes/m1-invariants/functions.tsv).

   For every function of functions.tsv whose column "inputs checked" is not "none":
   - for every checked argument and for every break of the type of that argument, a child process
     (fork) builds canonical arguments through the public interface, breaks the one argument, calls
     the function, and must end by SIGABRT with a line on stderr that names the function, the
     argument and the type;
   - a child with canonical arguments (global, and again with local finite balls) must return
     normally, must write nothing to stderr, and must be able to free its context after it has
     cleared everything, which also shows that the function leaves the borrow count right.
   The breaks are checked in the parent first: every broken value is really not canonical.

   The test reads lanes/m1-invariants/functions.tsv and requires that its table of cases and the
   list agree, in both directions. Without -DADF_CHECK_INVARIANTS the program says that it was
   skipped and counts as passed (tests/README.md). The test runs from the repository root. */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld.h>

#include "test_runner.h"

#ifndef ADF_CHECK_INVARIANTS

ADF_TEST(skipped_without_the_flag)
{
    printf("test_invariants: skipped: built without -DADF_CHECK_INVARIANTS\n");
    ADF_CHECK(1);
}

#else

#include <signal.h>
#include <sys/resource.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>

/* ------------------------------------------------------------------ the pack of arguments */

/* The order of the slots is the order of the argument names of a case: values first, rationals last. */
enum { SF1, SF2, SS1, SS2, SA1, SA2, SC1, SC2, SR1, SR2, NSLOT };

typedef struct
{
    adf_modctx_struct * ctx;
    adf_modctx_struct * ctx2;       /* the outputs of modes 3 and 4 start in this one */
    adf_rat_t r1, r2;
    adf_fball_t f1, f2;
    adf_scaled_t s1, s2;
    adf_adele_t a1, a2;
    adf_cadele_t c1, c2;
    adf_rat_t oq;
    adf_fball_t of;
    adf_scaled_t os;
    adf_adele_t oa;
    adf_cadele_t oc;
    arb_t rb, rb2;
    acb_t zb, zb2;
    fmpq_t fq;
    fmpz_t A, H, d;
    slong e;
    int lost;
    adf_place_t v;
} P;

/* Canonical values: r1 = 3/4, r2 = 5/6 (both positive and non-zero: caps, divisors, an interval
   lo < hi); f1 = 5 mod 6 and f2 = 1 mod 3, both local values of the context (4, 3), K = 12
   (K/R and c K/R are integers); s1, s2 the scaled conversions of f1, f2; a1, a2 = (3/2 +- 1/4 ;
   f1), (-1 ; f2); c1, c2 likewise. With local = 1 the finite balls are local values. */
static void
pack_init(P * p, int local, int local_out)
{
    ulong q[2] = {4, 3};
    fmpz_t A, H, d;
    fmpq_t t;

    ulong q2 = 5;

    if (adf_modctx_new_blocks(&p->ctx, q, 2) != ADF_OK || adf_modctx_new_blocks(&p->ctx2, &q2, 1) != ADF_OK)
        abort();
    adf_rat_init(p->r1);
    adf_rat_init(p->r2);
    adf_rat_init(p->oq);
    fmpz_init_set_si(A, 3);
    fmpz_init_set_si(d, 4);
    if (adf_rat_set_fmpz2(p->r1, A, d) != ADF_OK)
        abort();
    fmpz_set_si(A, 5);
    fmpz_set_si(d, 6);
    if (adf_rat_set_fmpz2(p->r2, A, d) != ADF_OK)
        abort();
    fmpz_init(H);
    adf_fball_init(p->f1);
    adf_fball_init(p->f2);
    adf_fball_init(p->of);
    fmpz_set_si(A, 5);
    fmpz_set_si(H, 6);
    fmpz_set_si(d, 1);
    if (adf_fball_set_fmpz3(p->f1, A, H, d) != ADF_OK)
        abort();
    fmpz_set_si(A, 1);
    fmpz_set_si(H, 3);
    if (adf_fball_set_fmpz3(p->f2, A, H, d) != ADF_OK)
        abort();
    adf_scaled_init(p->s1, p->ctx);
    adf_scaled_init(p->s2, p->ctx);
    adf_scaled_init(p->os, p->ctx);
    if (adf_scaled_set_fball(p->s1, NULL, p->f1, p->ctx) != ADF_OK ||
        adf_scaled_set_fball(p->s2, NULL, p->f2, p->ctx) != ADF_OK)
        abort();
    if (local)
    {
        if (adf_fball_set_local(p->f1, p->f1, p->ctx) != ADF_OK ||
            adf_fball_set_local(p->f2, p->f2, p->ctx) != ADF_OK)
            abort();
    }
    arb_init(p->rb);
    arb_init(p->rb2);
    acb_init(p->zb);
    acb_init(p->zb2);
    fmpq_init(t);
    fmpq_set_si(t, 3, 2);
    arb_set_fmpq(p->rb, t, 64);
    arb_add_error_2exp_si(p->rb, -2);
    fmpq_clear(t);
    adf_adele_init(p->a1);
    adf_adele_init(p->a2);
    adf_adele_init(p->oa);
    adf_cadele_init(p->c1);
    adf_cadele_init(p->c2);
    adf_cadele_init(p->oc);
    if (adf_adele_set_arb_fball(p->a1, p->rb, p->f1) != ADF_OK)
        abort();
    arb_set_si(p->rb2, -1);
    if (adf_adele_set_arb_fball(p->a2, p->rb2, p->f2) != ADF_OK)
        abort();
    acb_set_si_si(p->zb, 3, -2);
    acb_set_si_si(p->zb2, -1, 5);
    if (adf_cadele_set_acb_fball(p->c1, p->zb, p->f1) != ADF_OK ||
        adf_cadele_set_acb_fball(p->c2, p->zb2, p->f2) != ADF_OK)
        abort();
    if (local_out)
    {
        /* the outputs start as borrowers of another context (ctx2): a function that overwrites
           them must give that borrow back, and take one of ctx if the result is local */
        fmpz_t one, five;

        fmpz_init_set_si(one, 1);
        fmpz_init_set_si(five, 5);
        if (adf_fball_set_fmpz3(p->of, one, five, one) != ADF_OK ||
            adf_fball_set_local(p->of, p->of, p->ctx2) != ADF_OK)
            abort();
        adf_fball_set(&p->oa->fin, p->of);
        adf_fball_set(&p->oc->fin, p->of);
        adf_scaled_clear(p->os);
        adf_scaled_init(p->os, p->ctx2);
        fmpz_clear(one);
        fmpz_clear(five);
    }
    fmpq_init(p->fq);
    fmpz_init(p->A);
    fmpz_init(p->H);
    fmpz_init(p->d);
    p->e = 0;
    p->lost = 0;
    if (adf_place_prime(&p->v, 2) != ADF_OK)
        abort();
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

/* Clear every value and then free the context: the free aborts if any function left the count
   of borrowers wrong. */
static void
pack_clear_free(P * p)
{
    adf_rat_clear(p->r1);
    adf_rat_clear(p->r2);
    adf_rat_clear(p->oq);
    adf_fball_clear(p->f1);
    adf_fball_clear(p->f2);
    adf_fball_clear(p->of);
    adf_scaled_clear(p->s1);
    adf_scaled_clear(p->s2);
    adf_scaled_clear(p->os);
    adf_adele_clear(p->a1);
    adf_adele_clear(p->a2);
    adf_adele_clear(p->oa);
    adf_cadele_clear(p->c1);
    adf_cadele_clear(p->c2);
    adf_cadele_clear(p->oc);
    arb_clear(p->rb);
    arb_clear(p->rb2);
    acb_clear(p->zb);
    acb_clear(p->zb2);
    fmpq_clear(p->fq);
    fmpz_clear(p->A);
    fmpz_clear(p->H);
    fmpz_clear(p->d);
    adf_modctx_free(p->ctx2);
    adf_modctx_free(p->ctx);
}

/* ------------------------------------------------------------------ the breaks */

/* Each break turns a canonical value into one that violates one clause of its predicate, with
   every pointer field still valid (decision M1-D2). */

#define NB_RAT 5
static void
break_rat(adf_rat_struct * x, int kind)
{
    switch (kind)
    {
        case 0: fmpz_zero(fmpq_denref(x->q)); break;                                  /* den = 0 */
        case 1: fmpz_set_si(fmpq_numref(x->q), 3); fmpz_set_si(fmpq_denref(x->q), -4); break;
        case 2: fmpz_set_si(fmpq_numref(x->q), 6); fmpz_set_si(fmpq_denref(x->q), 4); break;
        case 3: fmpz_zero(fmpq_numref(x->q)); fmpz_set_si(fmpq_denref(x->q), 2); break; /* 0/2 */
        default: fmpz_set_si(fmpq_numref(x->q), 5); fmpz_set_si(fmpq_denref(x->q), 5); break;
    }
}

/* kinds 0 to 6 break a global value, 7 to 13 a local one (the value is first made local in ctx). */
#define NB_FBALL 14
static void
break_fball(adf_fball_struct * x, int kind, const adf_modctx_struct * ctx)
{
    if (kind >= 7)
    {
        if (adf_fball_set_local(x, x, ctx) != ADF_OK)
            abort();
    }
    switch (kind)
    {
        case 0: fmpz_zero(x->d); break;                                         /* d = 0 */
        case 1: fmpz_set_si(x->d, -1); break;                                   /* d < 0 */
        case 2: fmpz_set_si(x->H, -6); break;                                   /* H < 0 */
        case 3: fmpz_set_si(x->A, 6); fmpz_set_si(x->H, 6); break;              /* A = H */
        case 4: fmpz_set_si(x->A, 2); fmpz_set_si(x->H, 4); fmpz_set_si(x->d, 2); break; /* gcd 2 */
        case 5: fmpz_set_si(x->A, 2); fmpz_zero(x->H); fmpz_set_si(x->d, 4); break;      /* gcd(A,d) 2 */
        case 6: x->backend = 7; break;                                          /* tag */
        case 7: fmpz_zero(x->d); break;                                         /* local d = 0 */
        case 8: fmpz_add_ui(x->H, x->H, 1); break;                              /* H != K */
        case 9: fmpz_one(x->A); break;                                          /* A != 0 */
        case 10: x->res[0] = adf_modctx_block(ctx, 0); break;                   /* residue = block */
        case 11: x->mctx = NULL; break;                                         /* local, mctx NULL */
        case 12: x->res[1] = adf_modctx_block(ctx, 1) + 5; break;              /* residue above block */
        default: fmpz_set_si(x->d, -3); break;                                  /* local d < 0 */
    }
}

/* kinds 8 (mctx NULL) is a break only for a value of a context. */
#define NB_SCALED 9
static void
break_scaled(adf_scaled_struct * x, int kind)
{
    fmpz_t K;

    fmpz_init(K);
    adf_modctx_get_modulus(K, x->mctx);
    switch (kind)
    {
        case 0: fmpz_zero(fmpq_denref(x->s)); break;
        case 1: fmpz_set_si(fmpq_numref(x->s), 6); fmpz_set_si(fmpq_denref(x->s), 4); break;
        case 2: x->exact = 2; break;
        case 3: x->exact = -1; break;
        case 4: x->exact = 0; fmpz_set_si(fmpq_numref(x->s), -1); fmpz_set_si(fmpq_denref(x->s), 2); break;
        case 5: x->exact = 0; fmpz_set(x->u, K); break;                        /* u = K */
        case 6: x->exact = 0; fmpz_set_si(x->u, -1); break;                    /* u < 0 */
        case 7: x->exact = 1; fmpz_set_si(x->u, 1); break;                     /* exact, u != 0 */
        default: x->mctx = NULL; break;
    }
    fmpz_clear(K);
}

/* adele: kinds 0, 1 break the real ball (not finite); kinds 2 to 6 break the finite ball. */
#define NB_ADELE 7
static void
break_adele(adf_adele_struct * x, int kind, const adf_modctx_struct * ctx)
{
    switch (kind)
    {
        case 0: arb_pos_inf(x->inf); break;
        case 1: arb_indeterminate(x->inf); break;
        case 2: break_fball(&x->fin, 0, ctx); break;
        case 3: break_fball(&x->fin, 4, ctx); break;
        case 4: break_fball(&x->fin, 7, ctx); break;
        case 5: break_fball(&x->fin, 10, ctx); break;
        default: break_fball(&x->fin, 6, ctx); break;
    }
}

#define NB_CADELE 8
static void
break_cadele(adf_cadele_struct * x, int kind, const adf_modctx_struct * ctx)
{
    switch (kind)
    {
        case 0: arb_pos_inf(acb_realref(x->inf)); break;
        case 1: arb_indeterminate(acb_imagref(x->inf)); break;
        case 2: arb_neg_inf(acb_imagref(x->inf)); break;
        case 3: break_fball(&x->fin, 0, ctx); break;
        case 4: break_fball(&x->fin, 4, ctx); break;
        case 5: break_fball(&x->fin, 7, ctx); break;
        case 6: break_fball(&x->fin, 10, ctx); break;
        default: break_fball(&x->fin, 6, ctx); break;
    }
}

static int
nbreaks(int slot)
{
    switch (slot)
    {
        case SR1: case SR2: return NB_RAT;
        case SF1: case SF2: return NB_FBALL;
        case SS1: case SS2: return NB_SCALED;
        case SA1: case SA2: return NB_ADELE;
        default: return NB_CADELE;
    }
}

static void
break_slot(P * p, int slot, int kind)
{
    switch (slot)
    {
        case SR1: break_rat(p->r1, kind); break;
        case SR2: break_rat(p->r2, kind); break;
        case SF1: break_fball(p->f1, kind, p->ctx); break;
        case SF2: break_fball(p->f2, kind, p->ctx); break;
        case SS1: break_scaled(p->s1, kind); break;
        case SS2: break_scaled(p->s2, kind); break;
        case SA1: break_adele(p->a1, kind, p->ctx); break;
        case SA2: break_adele(p->a2, kind, p->ctx); break;
        case SC1: break_cadele(p->c1, kind, p->ctx); break;
        default: break_cadele(p->c2, kind, p->ctx); break;
    }
}

static int
slot_canonical(P * p, int slot)
{
    switch (slot)
    {
        case SR1: return adf_rat_is_canonical(p->r1);
        case SR2: return adf_rat_is_canonical(p->r2);
        case SF1: return adf_fball_is_canonical(p->f1);
        case SF2: return adf_fball_is_canonical(p->f2);
        case SS1: return adf_scaled_is_canonical(p->s1);
        case SS2: return adf_scaled_is_canonical(p->s2);
        case SA1: return adf_adele_is_canonical(p->a1);
        case SA2: return adf_adele_is_canonical(p->a2);
        case SC1: return adf_cadele_is_canonical(p->c1);
        default: return adf_cadele_is_canonical(p->c2);
    }
}

static const char *
slot_type(int slot)
{
    switch (slot)
    {
        case SR1: case SR2: return "adf_rat";
        case SF1: case SF2: return "adf_fball";
        case SS1: case SS2: return "adf_scaled";
        case SA1: case SA2: return "adf_adele";
        default: return "adf_cadele";
    }
}

/* ------------------------------------------------------------------ the cases */

typedef struct
{
    const char * name;
    void (*call)(P *);
    unsigned mask;        /* the checked slots */
    const char * args;    /* the names of the checked arguments, in the order of the slots */
    int local_only;       /* only the breaks of a local value abort (adf_fball_canonicalise) */
} kase;

#define B(s) (1u << (s))
#define Q1 (p->r1)
#define Q2 (p->r2)
#define F1 (p->f1)
#define F2 (p->f2)
#define S1 (p->s1)
#define S2 (p->s2)
#define A1 (p->a1)
#define A2 (p->a2)
#define C1 (p->c1)
#define C2 (p->c2)
#define OQ (p->oq)
#define OF (p->of)
#define OS (p->os)
#define OA (p->oa)
#define OC (p->oc)
#define CTX (p->ctx)

#define CASE(fn, mask, args, body)                                                       \
    static void call_##fn(P * p) { (void) p; body }
#define CASEL(fn, mask, args, body) CASE(fn, mask, args, body)

/* rat */
CASE(adf_rat_set, 0, "", adf_rat_set(OQ, Q1);)
CASE(adf_rat_swap, 0, "", adf_rat_swap(Q1, Q2);)
CASE(adf_rat_identical, 0, "", (void) adf_rat_identical(Q1, Q2);)
CASE(adf_rat_get_fmpq, 0, "", adf_rat_get_fmpq(p->fq, Q1);)
CASE(adf_rat_is_zero, 0, "", (void) adf_rat_is_zero(Q1);)
CASE(adf_rat_sgn, 0, "", (void) adf_rat_sgn(Q1);)
CASE(adf_rat_equal, 0, "", (void) adf_rat_equal(Q1, Q2);)
CASE(adf_rat_add, 0, "", adf_rat_add(OQ, Q1, Q2);)
CASE(adf_rat_sub, 0, "", adf_rat_sub(OQ, Q1, Q2);)
CASE(adf_rat_mul, 0, "", adf_rat_mul(OQ, Q1, Q2);)
CASE(adf_rat_div, 0, "", (void) adf_rat_div(OQ, Q1, Q2);)
CASE(adf_rat_neg, 0, "", adf_rat_neg(OQ, Q1);)
CASE(adf_rat_inv, 0, "", (void) adf_rat_inv(OQ, Q1);)
/* fball */
CASE(adf_fball_set, 0, "", adf_fball_set(OF, F1);)
CASE(adf_fball_swap, 0, "", adf_fball_swap(F1, F2);)
CASE(adf_fball_identical, 0, "", (void) adf_fball_identical(F1, F2);)
CASE(adf_fball_set_rat, 0, "", adf_fball_set_rat(OF, Q1);)
CASE(adf_fball_set_center_radius, 0, "", (void) adf_fball_set_center_radius(OF, Q1, Q2);)
CASE(adf_fball_canonicalise, 0, "", (void) adf_fball_canonicalise(F1);)
CASE(adf_fball_is_exact, 0, "", (void) adf_fball_is_exact(F1);)
CASE(adf_fball_get_center, 0, "", adf_fball_get_center(OQ, F1);)
CASE(adf_fball_get_radius, 0, "", adf_fball_get_radius(OQ, F1);)
CASE(adf_fball_get_den, 0, "", adf_fball_get_den(p->d, F1);)
CASE(adf_fball_haar_volume, 0, "", adf_fball_haar_volume(OQ, F1);)
CASE(adf_fball_get_fmpz3, 0, "", adf_fball_get_fmpz3(p->A, p->H, p->d, F1);)
CASE(adf_fball_prec_at, 0, "", (void) adf_fball_prec_at(&p->e, F1, p->v);)
CASE(adf_fball_add, 0, "", adf_fball_add(OF, F1, F2);)
CASE(adf_fball_sub, 0, "", adf_fball_sub(OF, F1, F2);)
CASE(adf_fball_mul, 0, "", adf_fball_mul(OF, F1, F2);)
CASE(adf_fball_neg, 0, "", adf_fball_neg(OF, F1);)
CASE(adf_fball_mul_rat, 0, "", adf_fball_mul_rat(OF, F1, Q1);)
CASE(adf_fball_div_rat, 0, "", (void) adf_fball_div_rat(OF, F1, Q1);)
CASE(adf_fball_equal_set, 0, "", (void) adf_fball_equal_set(F1, F2);)
CASE(adf_fball_overlaps, 0, "", (void) adf_fball_overlaps(F1, F2);)
CASE(adf_fball_contains, 0, "", (void) adf_fball_contains(F1, F2);)
CASE(adf_fball_compare, 0, "", (void) adf_fball_compare(F1, F2);)
CASE(adf_fball_contains_rat, 0, "", (void) adf_fball_contains_rat(F1, Q1);)
CASE(adf_fball_set_local, 0, "", (void) adf_fball_set_local(OF, F1, CTX);)
CASE(adf_fball_set_local_enclose, 0, "", (void) adf_fball_set_local_enclose(OF, &p->lost, F1, CTX);)
CASE(adf_fball_set_global, 0, "", adf_fball_set_global(OF, F1);)
CASE(adf_fball_is_local, 0, "", (void) adf_fball_is_local(F1);)
CASE(adf_fball_context, 0, "", (void) adf_fball_context(F1);)
/* scaled */
CASE(adf_scaled_set, 0, "", adf_scaled_set(OS, S1);)
CASE(adf_scaled_swap, 0, "", adf_scaled_swap(S1, S2);)
CASE(adf_scaled_identical, 0, "", (void) adf_scaled_identical(S1, S2);)
CASE(adf_scaled_context, 0, "", (void) adf_scaled_context(S1);)
CASE(adf_scaled_is_exact, 0, "", (void) adf_scaled_is_exact(S1);)
CASE(adf_scaled_set_rat, 0, "", adf_scaled_set_rat(OS, Q1, CTX);)
CASE(adf_scaled_set_fball, 0, "", (void) adf_scaled_set_fball(OS, &p->lost, F1, CTX);)
CASE(adf_scaled_set_context, 0, "", (void) adf_scaled_set_context(OS, &p->lost, S1, CTX);)
CASE(adf_scaled_get_fball, 0, "", adf_scaled_get_fball(OF, S1);)
CASE(adf_scaled_add, 0, "", (void) adf_scaled_add(OS, S1, S2);)
CASE(adf_scaled_sub, 0, "", (void) adf_scaled_sub(OS, S1, S2);)
CASE(adf_scaled_mul, 0, "", (void) adf_scaled_mul(OS, S1, S2);)
CASE(adf_scaled_mul_tight, 0, "", (void) adf_scaled_mul_tight(OS, S1, S2);)
CASE(adf_scaled_neg, 0, "", adf_scaled_neg(OS, S1);)
CASE(adf_scaled_mul_rat, 0, "", adf_scaled_mul_rat(OS, S1, Q1);)
CASE(adf_scaled_add_rat, 0, "", adf_scaled_add_rat(OS, &p->lost, S1, Q1);)
/* cap */
CASE(adf_fball_cap, 0, "", (void) adf_fball_cap(OF, F1, Q1);)
CASE(adf_fball_add_cap, 0, "", (void) adf_fball_add_cap(OF, F1, F2, Q1);)
CASE(adf_fball_sub_cap, 0, "", (void) adf_fball_sub_cap(OF, F1, F2, Q1);)
CASE(adf_fball_mul_cap, 0, "", (void) adf_fball_mul_cap(OF, F1, F2, Q1);)
CASE(adf_fball_mul_rat_cap, 0, "", (void) adf_fball_mul_rat_cap(OF, F1, Q1, Q2);)
/* recon */
CASE(adf_fball_reconstruct, 0, "", (void) adf_fball_reconstruct(OQ, F1, Q1, Q2);)
CASE(adf_adele_reconstruct, 0, "", (void) adf_adele_reconstruct(OQ, A1);)
/* adele */
CASE(adf_adele_set, 0, "", adf_adele_set(OA, A1);)
CASE(adf_adele_swap, 0, "", adf_adele_swap(A1, A2);)
CASE(adf_adele_identical, 0, "", (void) adf_adele_identical(A1, A2);)
CASE(adf_adele_set_rat, 0, "", adf_adele_set_rat(OA, Q1, 64);)
CASE(adf_adele_set_arb_fball, 0, "", (void) adf_adele_set_arb_fball(OA, p->rb, F1);)
CASE(adf_adele_get_real, 0, "", adf_adele_get_real(p->rb2, A1);)
CASE(adf_adele_get_arb_at, 0, "", (void) adf_adele_get_arb_at(p->rb2, A1, adf_place_inf());)
CASE(adf_adele_get_fin, 0, "", adf_adele_get_fin(OF, A1);)
CASE(adf_adele_add, 0, "", adf_adele_add(OA, A1, A2, 64);)
CASE(adf_adele_sub, 0, "", adf_adele_sub(OA, A1, A2, 64);)
CASE(adf_adele_mul, 0, "", adf_adele_mul(OA, A1, A2, 64);)
CASE(adf_adele_neg, 0, "", adf_adele_neg(OA, A1);)
CASE(adf_adele_add_rat, 0, "", adf_adele_add_rat(OA, A1, Q1, 64);)
CASE(adf_adele_mul_rat, 0, "", adf_adele_mul_rat(OA, A1, Q1, 64);)
CASE(adf_adele_div_rat, 0, "", (void) adf_adele_div_rat(OA, A1, Q1, 64);)
/* cadele */
CASE(adf_cadele_set, 0, "", adf_cadele_set(OC, C1);)
CASE(adf_cadele_swap, 0, "", adf_cadele_swap(C1, C2);)
CASE(adf_cadele_identical, 0, "", (void) adf_cadele_identical(C1, C2);)
CASE(adf_cadele_set_rat, 0, "", adf_cadele_set_rat(OC, Q1, 64);)
CASE(adf_cadele_set_adele, 0, "", adf_cadele_set_adele(OC, A1);)
CASE(adf_cadele_set_acb_fball, 0, "", (void) adf_cadele_set_acb_fball(OC, p->zb, F1);)
CASE(adf_cadele_get_complex, 0, "", adf_cadele_get_complex(p->zb2, C1);)
CASE(adf_cadele_get_fin, 0, "", adf_cadele_get_fin(OF, C1);)
CASE(adf_cadele_add, 0, "", adf_cadele_add(OC, C1, C2, 64);)
CASE(adf_cadele_sub, 0, "", adf_cadele_sub(OC, C1, C2, 64);)
CASE(adf_cadele_mul, 0, "", adf_cadele_mul(OC, C1, C2, 64);)
CASE(adf_cadele_neg, 0, "", adf_cadele_neg(OC, C1);)
CASE(adf_cadele_add_rat, 0, "", adf_cadele_add_rat(OC, C1, Q1, 64);)
CASE(adf_cadele_mul_rat, 0, "", adf_cadele_mul_rat(OC, C1, Q1, 64);)
CASE(adf_cadele_div_rat, 0, "", (void) adf_cadele_div_rat(OC, C1, Q1, 64);)
/* text and dump, value to string */
#define STRCASE(fn, expr) \
    CASE(fn, 0, "", { size_t n = 0; char * s = expr; adf_str_free(s); (void) n; })
STRCASE(adf_rat_get_str, adf_rat_get_str(&n, Q1))
STRCASE(adf_fball_get_str, adf_fball_get_str(&n, F1))
STRCASE(adf_adele_get_str, adf_adele_get_str(&n, A1, 20))
STRCASE(adf_cadele_get_str, adf_cadele_get_str(&n, C1, 20))
STRCASE(adf_rat_dump_str, adf_rat_dump_str(&n, Q1))
STRCASE(adf_fball_dump_str, adf_fball_dump_str(&n, F1))
STRCASE(adf_scaled_dump_str, adf_scaled_dump_str(&n, S1))
STRCASE(adf_scaled_get_str, adf_scaled_get_str(&n, S1))
STRCASE(adf_adele_dump_str, adf_adele_dump_str(&n, A1))
STRCASE(adf_cadele_dump_str, adf_cadele_dump_str(&n, C1))

#define K_(fn, mask, args) { #fn, call_##fn, mask, args, 0 }
#define KL(fn, mask, args) { #fn, call_##fn, mask, args, 1 }

static const kase cases[] = {
    K_(adf_rat_set, B(SR1), "x"),
    K_(adf_rat_swap, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_identical, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_get_fmpq, B(SR1), "x"),
    K_(adf_rat_is_zero, B(SR1), "x"),
    K_(adf_rat_sgn, B(SR1), "x"),
    K_(adf_rat_equal, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_add, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_sub, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_mul, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_div, B(SR1) | B(SR2), "x y"),
    K_(adf_rat_neg, B(SR1), "x"),
    K_(adf_rat_inv, B(SR1), "x"),
    K_(adf_fball_set, B(SF1), "x"),
    K_(adf_fball_swap, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_identical, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_set_rat, B(SR1), "q"),
    K_(adf_fball_set_center_radius, B(SR1) | B(SR2), "c N"),
    KL(adf_fball_canonicalise, B(SF1), "x"),
    K_(adf_fball_is_exact, B(SF1), "x"),
    K_(adf_fball_get_center, B(SF1), "x"),
    K_(adf_fball_get_radius, B(SF1), "x"),
    K_(adf_fball_get_den, B(SF1), "x"),
    K_(adf_fball_haar_volume, B(SF1), "x"),
    K_(adf_fball_get_fmpz3, B(SF1), "x"),
    K_(adf_fball_prec_at, B(SF1), "x"),
    K_(adf_fball_add, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_sub, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_mul, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_neg, B(SF1), "x"),
    K_(adf_fball_mul_rat, B(SF1) | B(SR1), "x q"),
    K_(adf_fball_div_rat, B(SF1) | B(SR1), "x q"),
    K_(adf_fball_equal_set, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_overlaps, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_contains, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_compare, B(SF1) | B(SF2), "x y"),
    K_(adf_fball_contains_rat, B(SF1) | B(SR1), "x q"),
    K_(adf_fball_set_local, B(SF1), "x"),
    K_(adf_fball_set_local_enclose, B(SF1), "x"),
    K_(adf_fball_set_global, B(SF1), "x"),
    K_(adf_fball_is_local, B(SF1), "x"),
    K_(adf_fball_context, B(SF1), "x"),
    K_(adf_scaled_set, B(SS1), "x"),
    K_(adf_scaled_swap, B(SS1) | B(SS2), "x y"),
    K_(adf_scaled_identical, B(SS1) | B(SS2), "x y"),
    K_(adf_scaled_context, B(SS1), "x"),
    K_(adf_scaled_is_exact, B(SS1), "x"),
    K_(adf_scaled_set_rat, B(SR1), "q"),
    K_(adf_scaled_set_fball, B(SF1), "x"),
    K_(adf_scaled_set_context, B(SS1), "x"),
    K_(adf_scaled_get_fball, B(SS1), "x"),
    K_(adf_scaled_add, B(SS1) | B(SS2), "x y"),
    K_(adf_scaled_sub, B(SS1) | B(SS2), "x y"),
    K_(adf_scaled_mul, B(SS1) | B(SS2), "x y"),
    K_(adf_scaled_mul_tight, B(SS1) | B(SS2), "x y"),
    K_(adf_scaled_neg, B(SS1), "x"),
    K_(adf_scaled_mul_rat, B(SS1) | B(SR1), "x q"),
    K_(adf_scaled_add_rat, B(SS1) | B(SR1), "x q"),
    K_(adf_fball_cap, B(SF1) | B(SR1), "x C"),
    K_(adf_fball_add_cap, B(SF1) | B(SF2) | B(SR1), "x y C"),
    K_(adf_fball_sub_cap, B(SF1) | B(SF2) | B(SR1), "x y C"),
    K_(adf_fball_mul_cap, B(SF1) | B(SF2) | B(SR1), "x y C"),
    K_(adf_fball_mul_rat_cap, B(SF1) | B(SR1) | B(SR2), "x q C"),
    K_(adf_fball_reconstruct, B(SF1) | B(SR1) | B(SR2), "x lo hi"),
    K_(adf_adele_reconstruct, B(SA1), "x"),
    K_(adf_adele_set, B(SA1), "x"),
    K_(adf_adele_swap, B(SA1) | B(SA2), "x y"),
    K_(adf_adele_identical, B(SA1) | B(SA2), "x y"),
    K_(adf_adele_set_rat, B(SR1), "q"),
    K_(adf_adele_set_arb_fball, B(SF1), "f"),
    K_(adf_adele_get_real, B(SA1), "x"),
    K_(adf_adele_get_arb_at, B(SA1), "x"),
    K_(adf_adele_get_fin, B(SA1), "x"),
    K_(adf_adele_add, B(SA1) | B(SA2), "x y"),
    K_(adf_adele_sub, B(SA1) | B(SA2), "x y"),
    K_(adf_adele_mul, B(SA1) | B(SA2), "x y"),
    K_(adf_adele_neg, B(SA1), "x"),
    K_(adf_adele_add_rat, B(SA1) | B(SR1), "x q"),
    K_(adf_adele_mul_rat, B(SA1) | B(SR1), "x q"),
    K_(adf_adele_div_rat, B(SA1) | B(SR1), "x q"),
    K_(adf_cadele_set, B(SC1), "x"),
    K_(adf_cadele_swap, B(SC1) | B(SC2), "x y"),
    K_(adf_cadele_identical, B(SC1) | B(SC2), "x y"),
    K_(adf_cadele_set_rat, B(SR1), "q"),
    K_(adf_cadele_set_adele, B(SA1), "x"),
    K_(adf_cadele_set_acb_fball, B(SF1), "f"),
    K_(adf_cadele_get_complex, B(SC1), "x"),
    K_(adf_cadele_get_fin, B(SC1), "x"),
    K_(adf_cadele_add, B(SC1) | B(SC2), "x y"),
    K_(adf_cadele_sub, B(SC1) | B(SC2), "x y"),
    K_(adf_cadele_mul, B(SC1) | B(SC2), "x y"),
    K_(adf_cadele_neg, B(SC1), "x"),
    K_(adf_cadele_add_rat, B(SC1) | B(SR1), "x q"),
    K_(adf_cadele_mul_rat, B(SC1) | B(SR1), "x q"),
    K_(adf_cadele_div_rat, B(SC1) | B(SR1), "x q"),
    K_(adf_rat_get_str, B(SR1), "x"),
    K_(adf_fball_get_str, B(SF1), "x"),
    K_(adf_adele_get_str, B(SA1), "x"),
    K_(adf_cadele_get_str, B(SC1), "x"),
    K_(adf_rat_dump_str, B(SR1), "x"),
    K_(adf_fball_dump_str, B(SF1), "x"),
    K_(adf_scaled_dump_str, B(SS1), "x"),
    K_(adf_scaled_get_str, B(SS1), "x"),
    K_(adf_adele_dump_str, B(SA1), "x"),
    K_(adf_cadele_dump_str, B(SC1), "x"),
};

#define NCASES (sizeof cases / sizeof cases[0])

/* ------------------------------------------------------------------ the child process */

typedef struct
{
    int exited;      /* WIFEXITED */
    int code;        /* exit code */
    int signal;      /* terminating signal, 0 if none */
    char err[4096];  /* what the child wrote to stderr */
} outcome;

/* mode 0: canonical inputs, global finite balls; 1: local finite balls; 2: break (slot, kind);
   3: as 0 with outputs that start as local values of the context; 4: as 1 with such outputs. */
static void
child_body(const kase * k, int mode, int slot, int kind)
{
    P p;

    pack_init(&p, mode == 1 || mode == 4, mode == 3 || mode == 4);
    if (mode == 2)
    {
        break_slot(&p, slot, kind);
    }
    k->call(&p);
    pack_clear_free(&p);
    flint_cleanup();
}

static void
run_child(const kase * k, int mode, int slot, int kind, outcome * o)
{
    int fd[2];
    pid_t pid;
    int st = 0;
    size_t n = 0;

    memset(o, 0, sizeof *o);
    fflush(stdout);
    fflush(stderr);
    if (pipe(fd) != 0)
        abort();
    pid = fork();
    if (pid < 0)
        abort();
    if (pid == 0)
    {
        struct rlimit nocore = {0, 0};

        /* An abort must not write a core file: with systemd-coredump each one costs 0.15 s, and
           RLIMIT_CORE does not stop it. A process that is not dumpable is not dumped. */
        setrlimit(RLIMIT_CORE, &nocore);
#ifdef __linux__
        prctl(PR_SET_DUMPABLE, 0);
#endif
        close(fd[0]);
        dup2(fd[1], 2);
        close(fd[1]);
        child_body(k, mode, slot, kind);
        _exit(0);
    }
    close(fd[1]);
    for (;;)
    {
        ssize_t r = read(fd[0], o->err + n, sizeof o->err - 1 - n);
        if (r <= 0)
            break;
        n += (size_t) r;
        if (n >= sizeof o->err - 1)
            break;
    }
    o->err[n] = 0;
    /* drain what does not fit */
    {
        char junk[256];
        while (read(fd[0], junk, sizeof junk) > 0)
            ;
    }
    close(fd[0]);
    if (waitpid(pid, &st, 0) != pid)
        abort();
    o->exited = WIFEXITED(st) ? 1 : 0;
    o->code = o->exited ? WEXITSTATUS(st) : -1;
    o->signal = WIFSIGNALED(st) ? WTERMSIG(st) : 0;
}

static const char *
nth_word(const char * s, int n, char * buf, size_t cap)
{
    int i = 0;

    while (*s)
    {
        size_t len = 0;
        while (*s == ' ')
            s++;
        while (s[len] && s[len] != ' ')
            len++;
        if (i == n)
        {
            if (len >= cap)
                len = cap - 1;
            memcpy(buf, s, len);
            buf[len] = 0;
            return buf;
        }
        s += len;
        i++;
    }
    buf[0] = 0;
    return buf;
}

/* ------------------------------------------------------------------ the tests */

/* The table of cases and the list agree in both directions. */
ADF_TEST(the_table_of_cases_is_the_list_of_checked_functions)
{
    FILE * f = fopen("lanes/m1-invariants/functions.tsv", "r");
    char line[4096];
    size_t nlist = 0, i;
    int found[NCASES];

    ADF_CHECK(f != NULL);
    if (f == NULL)
        return;
    memset(found, 0, sizeof found);
    ADF_CHECK(fgets(line, sizeof line, f) != NULL);          /* the header row */
    while (fgets(line, sizeof line, f) != NULL)
    {
        char * name = strtok(line, "\t");
        char * file = strtok(NULL, "\t");
        char * chk = strtok(NULL, "\t");
        if (name == NULL || file == NULL || chk == NULL)
            continue;
        if (strcmp(chk, "none") == 0)
            continue;
        nlist++;
        for (i = 0; i < NCASES; i++)
            if (strcmp(cases[i].name, name) == 0)
                found[i] = 1;
        {
            int hit = 0;
            for (i = 0; i < NCASES; i++)
                hit |= strcmp(cases[i].name, name) == 0;
            ADF_CHECK_MSG(hit, "%s is a checked function of the list and has no case", name);
        }
    }
    fclose(f);
    for (i = 0; i < NCASES; i++)
        ADF_CHECK_MSG(found[i], "the case %s is not a checked function of the list", cases[i].name);
    ADF_CHECK_MSG(nlist == NCASES, "the list has %zu checked functions, the table %zu", nlist,
                  (size_t) NCASES);
}

/* A canonical pack is canonical in every slot, so that an abort in a child is caused by the
   break; and every break really breaks its value (checked in a child, see below). */
ADF_TEST(a_canonical_pack_is_canonical_in_every_slot)
{
    int slot, mode;

    for (mode = 0; mode < 2; mode++)
    {
        P p;
        pack_init(&p, mode, 0);
        for (slot = 0; slot < NSLOT; slot++)
            ADF_CHECK_MSG(slot_canonical(&p, slot) == 1, "slot %d (mode %d) is not canonical", slot, mode);
        pack_clear_free(&p);
    }
}

ADF_TEST(every_checked_function_aborts_on_every_break_of_every_checked_argument)
{
    size_t ci;
    unsigned long runs = 0, aborted = 0;

    for (ci = 0; ci < NCASES; ci++)
    {
        const kase * k = &cases[ci];
        int slot, ord = 0;

        for (slot = 0; slot < NSLOT; slot++)
        {
            int kind;
            char arg[32];

            if (!(k->mask & (1u << slot)))
                continue;
            nth_word(k->args, ord++, arg, sizeof arg);
            for (kind = 0; kind < nbreaks(slot); kind++)
            {
                outcome o;
                int ok;
                char needle[128];

                if (k->local_only && !((slot == SF1 || slot == SF2) && kind >= 7))
                    continue;
                run_child(k, 2, slot, kind, &o);
                runs++;
                ok = o.signal == SIGABRT;
                snprintf(needle, sizeof needle, ": %s: argument %s is not a canonical %s\n", k->name, arg,
                         slot_type(slot));
                ADF_CHECK_MSG(ok, "%s: break %d of argument %s (%s) did not abort (exited %d code %d "
                                  "signal %d)", k->name, kind, arg, slot_type(slot), o.exited, o.code,
                              o.signal);
                aborted += ok ? 1u : 0u;
                ADF_CHECK_MSG(strstr(o.err, needle) != NULL,
                              "%s: break %d: stderr does not say '%s' (the function, the argument and "
                              "the type): [%s]", k->name, kind, needle, o.err);
                /* one line */
                ADF_CHECK_MSG(o.err[0] != 0 && strchr(o.err, '\n') == o.err + strlen(o.err) - 1,
                              "%s: break %d: stderr is not exactly one line: [%s]", k->name, kind,
                              o.err);
            }
        }
    }
    ADF_CHECK(runs > 0);
    printf("test_invariants: %lu break runs, %lu ended by SIGABRT\n", runs, aborted);
}

ADF_TEST(every_checked_function_returns_normally_on_canonical_inputs)
{
    size_t ci;
    int mode;
    static const char * const mode_name[5] = {"global finite balls", "local finite balls", "-",
                                              "global inputs, local outputs",
                                              "local inputs, local outputs"};

    for (ci = 0; ci < NCASES; ci++)
        for (mode = 0; mode < 5; mode++)
        {
            outcome o;

            if (mode == 2)
                continue;

            run_child(&cases[ci], mode, 0, 0, &o);
            ADF_CHECK_MSG(o.exited && o.code == 0,
                          "%s (%s): did not return normally (exited %d code %d signal %d): [%s]",
                          cases[ci].name, mode_name[mode], o.exited, o.code, o.signal, o.err);
            ADF_CHECK_MSG(o.err[0] == 0, "%s: wrote to stderr on canonical inputs: [%s]", cases[ci].name,
                          o.err);
        }
}

/* adf_fball_canonicalise: exempt for a global raw value (fball.h:138-141). */
static void
call_canonicalise_raw_global(P * p)
{
    adf_fball_t x;

    (void) p;
    adf_fball_init(x);
    fmpz_set_si(x->A, 2);
    fmpz_zero(x->H);
    fmpz_set_si(x->d, -4);
    if (adf_fball_canonicalise(x) != ADF_OK)
        abort();
    if (!adf_fball_is_canonical(x))
        abort();
    adf_fball_clear(x);
}

ADF_TEST(canonicalise_accepts_a_raw_global_value)
{
    kase k = {"adf_fball_canonicalise_raw", call_canonicalise_raw_global, 0, "", 0};
    outcome o;

    run_child(&k, 0, 0, 0, &o);
    ADF_CHECK_MSG(o.exited && o.code == 0 && o.err[0] == 0, "a raw global value was refused: [%s]", o.err);
}

/* The raw setters and the parsers take raw data: no check on their arguments. */
static void
call_raw_setters(P * p)
{
    fmpz_t n, dd;
    fmpq_t q;

    fmpz_init_set_si(n, 6);
    fmpz_init_set_si(dd, -4);
    fmpq_init(q);
    fmpz_set_si(fmpq_numref(q), 6);
    fmpz_set_si(fmpq_denref(q), 4);
    if (adf_rat_set_fmpz2(p->oq, n, dd) != ADF_OK || adf_rat_set_fmpq(p->oq, q) != ADF_OK)
        abort();
    fmpz_set_si(dd, 8);
    if (adf_fball_set_fmpz3(p->of, n, dd, dd) != ADF_OK)
        abort();
    fmpz_clear(n);
    fmpz_clear(dd);
    fmpq_clear(q);
}

ADF_TEST(the_raw_setters_take_non_canonical_data)
{
    kase k = {"raw_setters", call_raw_setters, 0, "", 0};
    outcome o;

    run_child(&k, 0, 0, 0, &o);
    ADF_CHECK_MSG(o.exited && o.code == 0 && o.err[0] == 0, "a raw setter was refused: [%s]", o.err);
}

/* The exempt functions do not abort on broken values: is_canonical answers 0; init and clear of
   the fields are not checked. */
static void
call_predicates_on_broken(P * p)
{
    int slot, kind;

    for (slot = 0; slot < NSLOT; slot++)
        for (kind = 0; kind < nbreaks(slot); kind++)
        {
            P q;
            pack_init(&q, 0, 0);
            break_slot(&q, slot, kind);
            if (slot_canonical(&q, slot) != 0)
            {
                fprintf(stderr, "slot %d kind %d is still canonical\n", slot, kind);
                abort();
            }
            /* q is leaked on purpose */
        }
    (void) p;
}

ADF_TEST(the_predicates_never_abort)
{
    kase k = {"predicates", call_predicates_on_broken, 0, "", 0};
    outcome o;

    run_child(&k, 0, 0, 0, &o);
    ADF_CHECK_MSG(o.exited && o.code == 0, "a predicate aborted: exited %d code %d signal %d [%s]",
                  o.exited, o.code, o.signal, o.err);
}

/* Finding R3 of docs/reviews/m1/local/review.md, reproducer checks/invariant_check.c: (1; 1) in the
   context (4), denominator set to 0, adf_fball_neg. */
static void
call_r3(P * p)
{
    adf_modctx_struct * ctx = NULL;
    ulong block = 4;
    adf_fball_t x, y;
    fmpz_t A, H, d;

    (void) p;
    adf_fball_init(x);
    adf_fball_init(y);
    fmpz_init_set_ui(A, 1);
    fmpz_init_set_ui(H, 4);
    fmpz_init_set_ui(d, 1);
    if (adf_modctx_new_blocks(&ctx, &block, 1) != ADF_OK || adf_fball_set_fmpz3(x, A, H, d) != ADF_OK ||
        adf_fball_set_local(x, x, ctx) != ADF_OK || !adf_fball_is_canonical(x))
        _exit(3);
    fmpz_zero(x->d);
    if (adf_fball_is_canonical(x))
        _exit(4);
    adf_fball_neg(y, x);
    _exit(0);
}

ADF_TEST(finding_R3_neg_of_a_local_ball_with_denominator_zero_aborts)
{
    kase k = {"adf_fball_neg", call_r3, 0, "", 0};
    outcome o;

    run_child(&k, 0, 0, 0, &o);
    ADF_CHECK_MSG(o.signal == SIGABRT, "neg returned (exited %d code %d signal %d)", o.exited, o.code,
                  o.signal);
    ADF_CHECK_MSG(strstr(o.err, "adf_fball_neg") != NULL, "stderr does not name adf_fball_neg: [%s]", o.err);
}

/* The Makefile and the flag agree: a program built for this test has the flag. */
ADF_TEST(the_flag_is_defined)
{
    ADF_CHECK(1);
}

#endif
