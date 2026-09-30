/* src/text_idele.c: the value form of adf_ucoset, adf_idele and adf_idclass (lane t-slice1, milestone 2):
   adf_ucoset_set_str, adf_ucoset_get_str, adf_idele_set_str, adf_idele_get_str, adf_idclass_set_str,
   adf_idclass_get_str.

   Contract: include/adelefeld/text.h (the block "adf_ucoset, adf_idele, adf_idclass"); docs/conventions.md 5.6,
   5.7, 8.1 to 8.5, 9.2 to 9.7. The reader and the printer proper are in src/text.c, which owns the cursor, the
   literals, the exact decimals and the printer of 9.5, and reaches this file through two hidden functions,
   adf_tx_read_unit_form (stages 1 to 7 of 8.5 for the three forms; it writes nothing on a status other than
   ADF_OK) and adf_tx_write_unit_form (the templates of 9.4 from primitive values). This file does what
   needs the types of the library: it builds the values with the constructors of ucoset.h, idele.h and
   idclass.h, so that the output is committed only on ADF_OK (conventions 4.3), and it prints the unit in its
   normal form (conventions 5.6; adf_ucoset_normalise). The declarations below repeat those of src/text.c,
   which a header would hold; the two must agree (tests/test_text_idele.c calls every one of them). */

#include <adelefeld.h>

#include "invariants.h"

#ifndef ADF_TX_HIDDEN
#if defined(__GNUC__) || defined(__clang__)
#define ADF_TX_HIDDEN __attribute__((visibility("hidden")))
#else
#define ADF_TX_HIDDEN
#endif
#endif

#define ADF_TX_FORM_UCOSET 0
#define ADF_TX_FORM_IDELE 1
#define ADF_TX_FORM_IDCLASS 2

ADF_TX_HIDDEN int adf_tx_read_unit_form(int form, arb_t inf, fmpq_t r, fmpz_t c, fmpz_t N, const char * s,
                                        size_t len, slong prec, const adf_text_limits_t * lim);
ADF_TX_HIDDEN char * adf_tx_write_unit_form(size_t * len, int form, const arb_t inf, const fmpq_t r,
                                            const fmpz_t c, const fmpz_t N, slong digits);

/* Entry checks of the debug build (conventions 4.4, CV-09; src/invariants.h). */
#ifdef ADF_CHECK_INVARIANTS
#define TI_INV_UC(x)                                                                                   \
    do { if (!adf_ucoset_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#define TI_INV_ID(x)                                                                                   \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#define TI_INV_CL(x)                                                                                   \
    do { if (!adf_idclass_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idclass"); } while (0)
#else
#define TI_INV_UC(x) ((void) 0)
#define TI_INV_ID(x) ((void) 0)
#define TI_INV_CL(x) ((void) 0)
#endif

/* The unit of a value, in its normal form (5.6; the printer of 9.4 writes it), as the integers c and N. */
static void
ti_normal_unit(fmpz_t c, fmpz_t N, const adf_ucoset_t u)
{
    adf_ucoset_t v;

    adf_ucoset_init(v);
    adf_ucoset_normalise(v, u);
    adf_ucoset_get_fmpz2(c, N, v);
    adf_ucoset_clear(v);
}

/* ---- adf_ucoset: ucoset_v = "[" int ["mod" uint] "]" (conventions 9.2); template 9.4 ---- */

int
adf_ucoset_set_str(adf_ucoset_t x, const char * s, size_t len, const adf_text_limits_t * lim)
{
    fmpz_t c, N;
    adf_ucoset_t t;
    int st;

    fmpz_init(c);
    fmpz_init(N);
    adf_ucoset_init(t);
    st = adf_tx_read_unit_form(ADF_TX_FORM_UCOSET, NULL, NULL, c, N, s, len, 0, lim);
    if (st == ADF_OK)
    {
        /* (c, N) satisfies the predicate of 5.6 and N is as written (CV-17): the constructor keeps both */
        (void) adf_ucoset_set_fmpz2(t, c, N);
        adf_ucoset_swap(x, t);
    }
    fmpz_clear(c);
    fmpz_clear(N);
    adf_ucoset_clear(t);
    return st;
}

char *
adf_ucoset_get_str(size_t * len, const adf_ucoset_t x)
{
    fmpz_t c, N;
    char * out;

    TI_INV_UC(x);
    fmpz_init(c);
    fmpz_init(N);
    ti_normal_unit(c, N, x);
    out = adf_tx_write_unit_form(len, ADF_TX_FORM_UCOSET, NULL, NULL, c, N, 0);
    fmpz_clear(c);
    fmpz_clear(N);
    return out;
}

/* ---- adf_idele: idele_v = "(" real ";" urat "*" ucoset ")"; template (r(x_inf) ; q(r) * U) ---- */

int
adf_idele_set_str(adf_idele_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim)
{
    fmpz_t c, N;
    fmpq_t r;
    arb_t inf;
    adf_ucoset_t u;
    adf_idele_t t;
    int st;

    fmpz_init(c);
    fmpz_init(N);
    fmpq_init(r);
    arb_init(inf);
    st = adf_tx_read_unit_form(ADF_TX_FORM_IDELE, inf, r, c, N, s, len, prec, lim);
    if (st == ADF_OK)
    {
        adf_ucoset_init(u);
        adf_idele_init(t);
        (void) adf_ucoset_set_fmpz2(u, c, N);
        /* the reader has checked the predicate of 5.7: a finite ball that excludes 0, r > 0 */
        st = adf_idele_set_parts(t, inf, r, u);
        if (st == ADF_OK)
            adf_idele_swap(x, t);
        adf_ucoset_clear(u);
        adf_idele_clear(t);
    }
    fmpz_clear(c);
    fmpz_clear(N);
    fmpq_clear(r);
    arb_clear(inf);
    return st;
}

char *
adf_idele_get_str(size_t * len, const adf_idele_t x, slong digits)
{
    fmpz_t c, N;
    char * out;

    TI_INV_ID(x);
    fmpz_init(c);
    fmpz_init(N);
    ti_normal_unit(c, N, &x->u);
    out = adf_tx_write_unit_form(len, ADF_TX_FORM_IDELE, x->inf, x->r, c, N, digits);
    fmpz_clear(c);
    fmpz_clear(N);
    return out;
}

/* ---- adf_idclass: idclass_v = "<" real ";" ucoset ">"; template <r(t) ; U> ---- */

int
adf_idclass_set_str(adf_idclass_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim)
{
    fmpz_t c, N;
    arb_t inf;
    adf_ucoset_t u;
    adf_idclass_t t;
    int st;

    fmpz_init(c);
    fmpz_init(N);
    arb_init(inf);
    st = adf_tx_read_unit_form(ADF_TX_FORM_IDCLASS, inf, NULL, c, N, s, len, prec, lim);
    if (st == ADF_OK)
    {
        adf_ucoset_init(u);
        adf_idclass_init(t);
        (void) adf_ucoset_set_fmpz2(u, c, N);
        st = adf_idclass_set_parts(t, inf, u);
        if (st == ADF_OK)
            adf_idclass_swap(x, t);
        adf_ucoset_clear(u);
        adf_idclass_clear(t);
    }
    fmpz_clear(c);
    fmpz_clear(N);
    arb_clear(inf);
    return st;
}

char *
adf_idclass_get_str(size_t * len, const adf_idclass_t x, slong digits)
{
    fmpz_t c, N;
    char * out;

    TI_INV_CL(x);
    fmpz_init(c);
    fmpz_init(N);
    ti_normal_unit(c, N, &x->u);
    out = adf_tx_write_unit_form(len, ADF_TX_FORM_IDCLASS, x->t, NULL, c, N, digits);
    fmpz_clear(c);
    fmpz_clear(N);
    return out;
}
