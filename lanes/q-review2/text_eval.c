/* lanes/q-review2/text_eval.c: drive adf_qclass_set_str and adf_qclass_get_str.

   Input commands on stdin, one per line:
     SET <midman> <midexp> <radman> <radexp> <A> <H> <d>   a reference class, LIFT, global finite
     READ <prec> <max_len> <max_exp10> <max_prec> <max_items> <hexbytes>
          lim from the five numbers; max_len < 0 means lim = NULL. The bytes are hex.
   Output per READ:
     S <status> <changed> <canonical> <hex printed or -> <midnum> <midden> <radnum> <radden>

   "changed" is 1 unless adf_qclass_identical says the destination equals a copy taken before the
   call. The two fractions are the exact dyadic midpoint and radius of the stored real ball. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char * statusname(int s)
{
    switch (s) {
    case ADF_OK: return "OK";
    case ADF_PARSE: return "PARSE";
    case ADF_LIMIT: return "LIMIT";
    case ADF_DOMAIN: return "DOMAIN";
    case ADF_UNSUPPORTED: return "UNSUPPORTED";
    case ADF_NOT_DETERMINED: return "NOT_DETERMINED";
    default: return "OTHER";
    }
}

static void set_pow2(arf_t r, const char * man, const char * exp)
{
    fmpz_t m, e;
    fmpz_init(m); fmpz_init(e);
    fmpz_set_str(m, man, 10);
    fmpz_set_str(e, exp, 10);
    arf_set_fmpz_2exp(r, m, e);
    fmpz_clear(m); fmpz_clear(e);
}

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static void print_hex(const char * s, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) printf("%02x", (unsigned char) s[i]);
}

/* print num/den of an exact dyadic arf, as two decimal integers */
static void print_dyadic(arf_srcptr v)
{
    fmpz_t man, ex, num, den;
    char * s;
    fmpz_init(man); fmpz_init(ex); fmpz_init(num); fmpz_init(den);
    arf_get_fmpz_2exp(man, ex, v);
    fmpz_set(num, man);
    fmpz_one(den);
    if (fmpz_sgn(ex) >= 0) fmpz_mul_2exp(num, num, fmpz_get_ui(ex));
    else fmpz_mul_2exp(den, den, (-fmpz_get_si(ex)));
    s = fmpz_get_str(NULL, 10, num); printf(" %s", s); flint_free(s);
    s = fmpz_get_str(NULL, 10, den); printf(" %s", s); flint_free(s);
    fmpz_clear(man); fmpz_clear(ex); fmpz_clear(num); fmpz_clear(den);
}

static void print_mag(mag_srcptr v)
{
    arf_t t;
    arf_init(t);
    arf_set_mag(t, v);
    print_dyadic(t);
    arf_clear(t);
}

int main(void)
{
    static char line[8000000];
    adf_qclass_t x, before;
    adf_qclass_init(x);
    adf_qclass_init(before);
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "SET ", 4) == 0) {
            char mm[64], me[64], rm[64], re[64], A[128], H[128], d[128];
            adf_adele_t a;
            if (sscanf(line, "SET %63s %63s %63s %63s %127s %127s %127s",
                       mm, me, rm, re, A, H, d) != 7) { printf("BAD SET\n"); return 2; }
            adf_adele_init(a);
            set_pow2(arb_midref(a->inf), mm, me);
            mag_set_ui_2exp_si(arb_radref(a->inf), strtoull(rm, NULL, 10), strtol(re, NULL, 10));
            {
                fmpz_t t;
                fmpz_init(t);
                fmpz_set_str(t, A, 10); fmpz_set(a->fin.A, t);
                fmpz_set_str(t, H, 10); fmpz_set(a->fin.H, t);
                fmpz_set_str(t, d, 10); fmpz_set(a->fin.d, t);
                fmpz_clear(t);
            }
            adf_qclass_set_adele(x, a);
            adf_adele_clear(a);
            printf("K %d\n", adf_qclass_is_canonical(x));
            fflush(stdout);
            continue;
        }
        if (strncmp(line, "PIECES", 6) == 0) {
            long n = strtol(line + 7, NULL, 10), j;
            adf_qclass_clear(x);
            x->form = ADF_QCLASS_PIECES;
            x->len = n;
            x->piece = flint_calloc((size_t) (n > 0 ? n : 1), sizeof(*x->piece));
            for (j = 0; j < n; j++) {
                adf_adele_init(&x->piece[j]);
                char mm[8];
                snprintf(mm, sizeof(mm), "%ld", 4 + j);
                set_pow2(arb_midref(x->piece[j].inf), mm, "-3");
                mag_zero(arb_radref(x->piece[j].inf));
            }
            printf("B %d %d\n", adf_qclass_is_canonical(x), (int) adf_qclass_length(x));
            fflush(stdout);
            continue;
        }
        if (strncmp(line, "GETSTR ", 7) == 0) {
            long dg = strtol(line + 7, NULL, 10);
            char * out;
            size_t olen = 12345;
            out = adf_qclass_get_str(&olen, x, dg);
            printf("G %d %ld ", out == NULL ? 1 : 0, out == NULL ? -1L : (long) olen);
            if (out) { print_hex(out, olen); adf_str_free(out); }
            printf("\n");
            fflush(stdout);
            continue;
        }
        if (strncmp(line, "READ ", 5) == 0) {
            char * p = line + 5;
            long prec, ml, me10, mp, mi;
            adf_text_limits_t lim, store;
            char * buf, * hex;
            size_t n, i, hl;
            int st, changed, canon;
            adf_qclass_set(before, x);
            prec = strtol(p, &p, 10);
            ml = strtol(p, &p, 10);
            me10 = strtol(p, &p, 10);
            mp = strtol(p, &p, 10);
            mi = strtol(p, &p, 10);
            while (*p == ' ') p++;
            hex = p;
            hl = strlen(hex);
            while (hl > 0 && (hex[hl - 1] == '\n' || hex[hl - 1] == '\r')) hl--;
            n = hl / 2;
            buf = flint_malloc(n + 2);
            for (i = 0; i < n; i++)
                buf[i] = (char) (16 * hexval(hex[2 * i]) + hexval(hex[2 * i + 1]));
            buf[n] = 0;
            if (ml == -999) st = adf_qclass_set_str(x, buf, n, prec, NULL);
            else {
                adf_text_limits_default(&store);
                lim.max_len = (size_t) ml;
                lim.max_exp10 = me10;
                lim.max_prec = mp;
                lim.max_items = mi;
                st = adf_qclass_set_str(x, buf, n, prec, &lim);
            }
            changed = adf_qclass_identical(x, before) ? 0 : 1;
            canon = adf_qclass_is_canonical(x) ? 1 : 0;
            printf("S %s %d %d ", statusname(st), changed, st == ADF_OK ? canon : -1);
            if (st == ADF_OK) {
                char * out;
                size_t olen = 0;
                out = adf_qclass_get_str(&olen, x, 20);
                if (out == NULL) printf("-");
                else { print_hex(out, olen); adf_str_free(out); }
                print_dyadic(arb_midref(x->piece[0].inf));
                print_mag(arb_radref(x->piece[0].inf));
            }
            else printf("-");
            printf("\n");
            fflush(stdout);
            flint_free(buf);
            continue;
        }
    }
    adf_qclass_clear(x);
    adf_qclass_clear(before);
    return 0;
}
