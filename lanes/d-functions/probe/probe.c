/* Probe of the installed FLINT 3.0.1 for the design docs/api-1f.md (lane d-functions).
   Not part of the library and not a test. Build and run, from the root of the repository:
     gcc -O1 -o /tmp/probe_1f lanes/d-functions/probe/probe.c -lflint -lm && timeout 60 /tmp/probe_1f
   The output that the design quotes is in lanes/d-functions/probe/probe.out. */
#include <stdio.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/padic.h>
#include <flint/arb.h>
#include <flint/acb.h>

static void show(const char *tag, int r, const padic_t y)
{
    printf("%s: ret=%d u=", tag, r); fmpz_print(padic_unit(y));
    printf(" v=%ld N=%ld\n", (long) padic_val(y), (long) padic_prec(y));
}

static void pexp(ulong pp, slong xv, slong Nin, slong Nout)
{
    fmpz_t p; padic_ctx_t ctx; padic_t x, y; char tag[100]; int r;
    fmpz_init_set_ui(p, pp); padic_ctx_init(ctx, p, 0, 8, PADIC_TERSE);
    padic_init2(x, Nin); padic_init2(y, Nout);
    padic_set_si(y, 777, ctx);
    padic_set_si(x, xv, ctx);
    r = padic_exp(y, x, ctx);
    sprintf(tag, "exp p=%lu x=%ld Nin=%ld Nout=%ld", (unsigned long) pp, (long) xv, (long) Nin, (long) Nout);
    show(tag, r, y);
    padic_clear(x); padic_clear(y); padic_ctx_clear(ctx); fmpz_clear(p);
}

static void plog(ulong pp, slong xv, slong Nin, slong Nout)
{
    fmpz_t p; padic_ctx_t ctx; padic_t x, y; char tag[100]; int r;
    fmpz_init_set_ui(p, pp); padic_ctx_init(ctx, p, 0, 8, PADIC_TERSE);
    padic_init2(x, Nin); padic_init2(y, Nout);
    padic_set_si(y, 777, ctx);
    padic_set_si(x, xv, ctx);
    r = padic_log(y, x, ctx);
    sprintf(tag, "log p=%lu x=%ld Nin=%ld Nout=%ld", (unsigned long) pp, (long) xv, (long) Nin, (long) Nout);
    show(tag, r, y);
    padic_clear(x); padic_clear(y); padic_ctx_clear(ctx); fmpz_clear(p);
}

static void pteich(ulong pp, slong xv, slong N)
{
    fmpz_t p; padic_ctx_t ctx; padic_t x, y; char tag[100];
    fmpz_init_set_ui(p, pp); padic_ctx_init(ctx, p, 0, 8, PADIC_TERSE);
    padic_init2(x, N); padic_init2(y, N);
    padic_set_si(x, xv, ctx);
    padic_teichmuller(y, x, ctx);
    sprintf(tag, "teich p=%lu x=%ld N=%ld", (unsigned long) pp, (long) xv, (long) N);
    show(tag, -1, y);
    padic_clear(x); padic_clear(y); padic_ctx_clear(ctx); fmpz_clear(p);
}

static void sa(const char *tag, const arb_t y)
{
    printf("%s: finite=%d ", tag, arb_is_finite(y)); arb_printd(y, 15); printf("\n");
}

static void sc(const char *tag, const acb_t y)
{
    printf("%s: finite=%d ", tag, acb_is_finite(y)); acb_printd(y, 15); printf("\n");
}

int main(void)
{
    /* the regression of review N4 */
    pexp(3, 3, 8, 8); pexp(3, 12, 8, 8); pexp(3, 3, 2, 2); pexp(3, 12, 2, 2);
    pexp(3, 3, 2, 8);           /* input known modulo 9, output variable of precision 8 */
    pexp(3, 0, 8, 8); pexp(3, 1, 8, 8); pexp(2, 2, 8, 8); pexp(2, 4, 8, 8); pexp(2, 6, 8, 8);
    pexp(3, 3, 8, 1); pexp(3, 3, 8, 0); pexp(3, 3, 8, -2); pexp(3, 81, 8, 3); pexp(3, 1, 8, 0);
    pexp(3, 1, 8, -1); pexp(2, 2, 8, 1); pexp(2, 2, 8, 0);
    plog(2, -1, 8, 8); plog(2, 3, 8, 8); plog(2, 5, 8, 8); plog(2, 1, 8, 8); plog(3, 4, 8, 8);
    plog(3, 2, 8, 8); plog(3, 3, 8, 8); plog(3, 4, 8, 1); plog(3, 4, 8, 0); plog(3, 4, 8, -3);
    plog(3, 2, 8, 0); plog(2, 3, 8, 1); plog(3, 1, 8, 8); plog(2, -3, 8, 8); plog(3, 4, 2, 8);
    pteich(5, 2, 8); pteich(101, 2, 5); pteich(2, 3, 8); pteich(5, 10, 8); pteich(5, 2, 0);
    {
        arb_t x, y; acb_t z, w; slong prec = 53;
        arb_init(x); arb_init(y); acb_init(z); acb_init(w);
        arb_zero(x); arb_add_error_2exp_si(x, -10); arb_log(y, x, prec); sa("arb_log [-e,e]", y);
        arb_zero(x); arb_log(y, x, prec); sa("arb_log exact 0", y);
        arb_set_si(x, -1); arb_log(y, x, prec); sa("arb_log -1", y);
        arb_zero(x); arb_add_error_2exp_si(x, -10); arb_sqrt(y, x, prec); sa("arb_sqrt [-e,e]", y);
        arb_set_si(x, -1); arb_sqrt(y, x, prec); sa("arb_sqrt -1", y);
        arb_zero(x); arb_sqrt(y, x, prec); sa("arb_sqrt exact 0", y);
        arb_set_d(x, 0.0009765625); arb_add_error_2exp_si(x, -10); arb_sqrt(y, x, prec);
        sa("arb_sqrt [0,2e]", y);
        arb_zero(x); arb_add_error_2exp_si(x, -10); arb_sqrtpos(y, x, prec); sa("arb_sqrtpos [-e,e]", y);
        arb_zero(x); arb_add_error_2exp_si(x, -10); arb_root_ui(y, x, 3, prec);
        sa("arb_root_ui 3 [-e,e]", y);
        arb_set_si(x, -8); arb_root_ui(y, x, 3, prec); sa("arb_root_ui 3 of -8", y);
        arb_set_si(x, 8); arb_root_ui(y, x, 3, prec); sa("arb_root_ui 3 of 8", y);
        arb_zero(x); arb_root_ui(y, x, 3, prec); sa("arb_root_ui 3 of exact 0", y);
        arb_zero(x); arb_root_ui(y, x, 2, prec); sa("arb_root_ui 2 of exact 0", y);
        arb_zero(x); arb_gamma(y, x, prec); sa("arb_gamma 0", y);
        arb_zero(x); arb_add_error_2exp_si(x, -10); arb_gamma(y, x, prec); sa("arb_gamma [-e,e]", y);
        arb_set_si(x, -3); arb_gamma(y, x, prec); sa("arb_gamma -3", y);
        arb_set_d(x, 1.5707963267948966); arb_add_error_2exp_si(x, -10); arb_tan(y, x, prec);
        sa("arb_tan near pi/2", y);
        arb_set_si(x, 1); arb_zeta(y, x, prec); sa("arb_zeta 1", y);
        arb_set_si(x, 1); arb_mul_2exp_si(x, x, 80); arb_exp(y, x, prec); sa("arb_exp 2^80", y);
        arb_set_si(x, 1); arb_exp(y, x, 1); sa("arb_exp prec 1", y);
        arb_set_si(x, 1); arb_exp(y, x, 2); sa("arb_exp prec 2", y);
        acb_set_si(z, -1); acb_log(w, z, prec); sc("acb_log -1", w);
        acb_set_si(z, -1); arb_add_error_2exp_si(acb_imagref(z), -10); acb_log(w, z, prec);
        sc("acb_log -1 +- e i", w);
        acb_zero(z); acb_log(w, z, prec); sc("acb_log 0", w);
        acb_zero(z); arb_add_error_2exp_si(acb_imagref(z), -10);
        arb_add_error_2exp_si(acb_realref(z), -10); acb_log(w, z, prec); sc("acb_log ball at 0", w);
        acb_set_si(z, -1); arb_add_error_2exp_si(acb_imagref(z), -10); acb_sqrt(w, z, prec);
        sc("acb_sqrt -1 +- e i", w);
        acb_set_si(z, -1); acb_sqrt(w, z, prec); sc("acb_sqrt -1", w);
        arb_clear(x); arb_clear(y); acb_clear(z); acb_clear(w);
    }
    return 0;
}
