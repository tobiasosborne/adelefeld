#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void arf_q(fmpq_t q, const arf_t a)
{
    fmpz_t z, e;
    fmpz_init(z); fmpz_init(e);
    arf_get_fmpz_2exp(z, e, a);
    slong k = fmpz_get_si(e);
    fmpq_set_fmpz(q, z);
    if (k >= 0) fmpz_mul_2exp(fmpq_numref(q), fmpq_numref(q), k);
    else fmpz_mul_2exp(fmpq_denref(q), fmpq_denref(q), -k);
    fmpq_canonicalise(q);
    fmpz_clear(z); fmpz_clear(e);
}
static void ends(const arb_t a)
{
    fmpq_t mid, rad, lo, hi;
    arf_t r;
    fmpq_init(mid); fmpq_init(rad); fmpq_init(lo); fmpq_init(hi); arf_init(r);
    arf_q(mid, arb_midref(a)); arf_set_mag(r, arb_radref(a)); arf_q(rad, r);
    fmpq_sub(lo, mid, rad); fmpq_add(hi, mid, rad);
    fmpq_print(lo); putchar(' '); fmpq_print(hi);
    fmpq_clear(mid); fmpq_clear(rad); fmpq_clear(lo); fmpq_clear(hi); arf_clear(r);
}
static int digit(char c)
{
    return c <= '9' ? c-'0' : c-'a'+10;
}
int main(void)
{
    char *line = NULL; size_t cap = 0;
    adf_ucoset_t u, u2;
    adf_idele_t x, x2;
    adf_idclass_t c, c2;
    adf_ucoset_init(u); adf_ucoset_init(u2); adf_idele_init(x); adf_idele_init(x2);
    adf_idclass_init(c); adf_idclass_init(c2);
    while (getline(&line, &cap, stdin) > 0)
    {
        char form; long prec, digits; int used;
        if (sscanf(line, "%c %ld %ld %n", &form, &prec, &digits, &used) != 3) return 2;
        char *h = line+used;
        size_t n = strcspn(h, "\r\n")/2;
        char *s = malloc(n+1);
        for (size_t i=0; i<n; i++) s[i] = (char)(16*digit(h[2*i])+digit(h[2*i+1]));
        s[n] = 0;
        adf_ucoset_set(u2, u); adf_idele_set(x2, x); adf_idclass_set(c2, c);
        adf_text_kind kind = ADF_TEXT_CHAR;
        int ks = adf_text_classify(&kind, s, n, NULL), st;
        if (form == 'U') st = adf_ucoset_set_str(u, s, n, NULL);
        else if (form == 'I') st = adf_idele_set_str(x, s, n, prec, NULL);
        else st = adf_idclass_set_str(c, s, n, prec, NULL);
        printf("%d %d %d", st, ks, kind);
        if (st != ADF_OK) printf(" unchanged=%d", form == 'U' ? adf_ucoset_identical(u, u2)
            : form == 'I' ? adf_idele_identical(x, x2) : adf_idclass_identical(c, c2));
        if (st == ADF_OK)
        {
            adf_ucoset_srcptr unit = form == 'U' ? u : form == 'I' ? &x->u : &c->u;
            putchar(' '); fmpz_print(unit->c); putchar(' '); fmpz_print(unit->N);
            if (form != 'U') { putchar(' '); ends(form == 'I' ? x->inf : c->t); }
            if (form == 'I') { putchar(' '); fmpq_print(x->r); }
            size_t len;
            char *text = form == 'U' ? adf_ucoset_get_str(&len, u) : form == 'I'
                ? adf_idele_get_str(&len, x, digits) : adf_idclass_get_str(&len, c, digits);
            if (text)
            {
                int rt = form == 'U' ? adf_ucoset_set_str(u2, text, len, NULL) : form == 'I'
                    ? adf_idele_set_str(x2, text, len, 512, NULL) : adf_idclass_set_str(c2, text, len, 512, NULL);
                printf(" %d", rt);
                if (form != 'U' && rt == ADF_OK) { putchar(' '); ends(form == 'I' ? x2->inf : c2->t); }
                printf(" TEXT "); fwrite(text, 1, len, stdout);
                adf_str_free(text);
            }
            else printf(" NULL");
        }
        putchar('\n'); fflush(stdout); free(s);
    }
    free(line); adf_ucoset_clear(u); adf_ucoset_clear(u2); adf_idele_clear(x); adf_idele_clear(x2);
    adf_idclass_clear(c); adf_idclass_clear(c2); flint_cleanup(); return 0;
}
