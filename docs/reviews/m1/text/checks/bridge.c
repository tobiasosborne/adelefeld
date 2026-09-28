/* Review adapter. No parser or arithmetic oracle is taken from src/text.c. */
#include <stdlib.h>
#include <string.h>
#include "adelefeld.h"

static char *dyadic(const arb_t x)
{
    fmpz_t m, e, r, re;
    arf_t a;
    char *ms, *es, *rs, *res, *out;
    fmpz_init(m); fmpz_init(e); fmpz_init(r); fmpz_init(re); arf_init(a);
    arf_get_fmpz_2exp(m, e, arb_midref(x));
    arf_set_mag(a, arb_radref(x));
    arf_get_fmpz_2exp(r, re, a);
    ms = fmpz_get_str(NULL, 10, m); es = fmpz_get_str(NULL, 10, e);
    rs = fmpz_get_str(NULL, 10, r); res = fmpz_get_str(NULL, 10, re);
    out = flint_malloc(strlen(ms) + strlen(es) + strlen(rs) + strlen(res) + 4);
    sprintf(out, "%s %s %s %s", ms, es, rs, res);
    flint_free(ms); flint_free(es); flint_free(rs); flint_free(res);
    fmpz_clear(m); fmpz_clear(e); fmpz_clear(r); fmpz_clear(re); arf_clear(a);
    return out;
}

int review_parse(const char *s, size_t len, slong p, const adf_text_limits_t *lim,
                 int part, char **out)
{
    int st;
    if (part < 0) {
        adf_adele_t x;
        adf_adele_init(x);
        st = adf_adele_set_str(x, s, len, p, lim);
        if (st == ADF_OK) *out = dyadic(x->inf);
        adf_adele_clear(x);
    } else {
        adf_cadele_t x;
        adf_cadele_init(x);
        st = adf_cadele_set_str(x, s, len, p, lim);
        if (st == ADF_OK) *out = dyadic(part ? acb_imagref(x->inf) : acb_realref(x->inf));
        adf_cadele_clear(x);
    }
    return st;
}

/* Negative return means a failed parse changed the initialized sentinel. */
int review_status(int kind, const char *s, size_t len, const adf_text_limits_t *lim)
{
    int st, changed;
#define TRY(T, init, set, parse, clear, extra) do { \
    T x, old; memset(x, 0, sizeof(T)); init(x); set; memcpy(old, x, sizeof(T)); \
    st = parse(x, s, len, extra lim); changed = memcmp(old, x, sizeof(T)); clear(x); \
} while (0)
#define PREC 17,
    switch (kind) {
    case 0: TRY(adf_rat_t, adf_rat_init, adf_rat_set_si(x, 71), adf_rat_set_str, adf_rat_clear, ); break;
    case 1: TRY(adf_fball_t, adf_fball_init, adf_fball_set_si(x, 71), adf_fball_set_str,
                adf_fball_clear, ); break;
    case 2: TRY(adf_adele_t, adf_adele_init, arb_set_si(x->inf, 71), adf_adele_set_str,
                adf_adele_clear, PREC); break;
    default: TRY(adf_cadele_t, adf_cadele_init, acb_set_si(x->inf, 71), adf_cadele_set_str,
                 adf_cadele_clear, PREC); break;
    }
    return st != ADF_OK && changed ? -100 - st : st;
}

char *review_print(const char *man, slong e, ulong r, slong re, slong digits)
{
    adf_adele_t x;
    fmpz_t m, ex;
    size_t len;
    char *out;
    adf_adele_init(x); fmpz_init(m); fmpz_init_set_si(ex, e);
    fmpz_set_str(m, man, 10);
    arf_set_fmpz_2exp(arb_midref(x->inf), m, ex);
    mag_set_ui_2exp_si(arb_radref(x->inf), r, re);
    out = adf_adele_get_str(&len, x, digits);
    if (strlen(out) != len) abort();
    fmpz_clear(m); fmpz_clear(ex); adf_adele_clear(x);
    return out;
}
