/* Instrument the external FLINT calls, without changing production code.
   A deliberately widened [-1,1] trig enclosure is still valid. It forces the
   documented retry path. Link with --wrap for the four functions below.
   The exact image and certificate are docs/api-3.md 3.3 and Q4. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "test_retry.c:%d: %s\n", __LINE__, #c); exit(1); } } while (0)
static slong user_p, previous;
static int mode, calls, limits_active;

void __real_arb_sin_cos_pi_fmpq(arb_t s, arb_t c, const fmpq_t q, slong p);
void __real_arb_cos_pi_fmpq(arb_t c, const fmpq_t q, slong p);
void __real_fmpq_mul_fmpz(fmpq_t z, const fmpq_t x, const fmpz_t n);
void __real_fmpq_div_fmpz(fmpq_t z, const fmpq_t x, const fmpz_t n);

static void before(slong p)
{
    calls++;
    if (calls == 1) CHECK(p == FLINT_MIN(user_p+32, ADF_REAL_PREC_MAX));
    if (mode == 1 && calls == 2) CHECK(p == FLINT_MIN(2*previous, ADF_REAL_PREC_MAX));
    previous = p;
}
static void widen(arb_t c)
{
    if (mode == 1 && calls == 1) { arb_zero(c); mag_one(arb_radref(c)); }
    if (mode == 2 && calls == 1) {
        /* Preserve the computed midpoint; replace a smaller radius by epsilon/2. */
        CHECK(mag_cmp_2exp_si(arb_radref(c), -user_p-1) <= 0);
        mag_set_ui_2exp_si(arb_radref(c), 1, -user_p-1);
    }
}
void __wrap_arb_sin_cos_pi_fmpq(arb_t s, arb_t c, const fmpq_t q, slong p)
{
    before(p); __real_arb_sin_cos_pi_fmpq(s, c, q, p); widen(s); widen(c);
}
void __wrap_arb_cos_pi_fmpq(arb_t c, const fmpq_t q, slong p)
{
    before(p); __real_arb_cos_pi_fmpq(c, q, p); widen(c);
}
void __wrap_fmpq_mul_fmpz(fmpq_t z, const fmpq_t x, const fmpz_t n)
{
    if (limits_active && !fmpq_is_zero(x) && !fmpz_is_one(n))
        CHECK(fmpz_bits(fmpq_numref(x))+fmpz_bits(n) <= ADF_QCLASS_BITS_MAX);
    __real_fmpq_mul_fmpz(z, x, n);
}
void __wrap_fmpq_div_fmpz(fmpq_t z, const fmpq_t x, const fmpz_t n)
{
    if (limits_active && !fmpq_is_zero(x) && !fmpz_is_one(n))
        CHECK(fmpz_bits(fmpq_denref(x))+fmpz_bits(n) <= ADF_QCLASS_BITS_MAX);
    __real_fmpq_div_fmpz(z, x, n);
}

int main(void)
{
    adf_adele_t x; adf_rat_t a, N; fmpq_t theta; acb_t z, saved;
    adf_adele_init(x); adf_rat_init(a); adf_rat_init(N); fmpq_init(theta);
    acb_init(z); acb_init(saved);
    fmpq_set_si(a->q, 1, 3); adf_fball_set_rat(&x->fin, a); fmpq_set(theta, a->q);
    user_p = 20; mode = 1; calls = 0;
    CHECK(adf_phase_get_acb(z, theta, user_p) == ADF_OK); CHECK(calls == 2);
    calls = 0;
    CHECK(adf_adele_psi_tate(z, x, user_p) == ADF_OK); CHECK(calls == 5);
    mode = 2; calls = 0;
    CHECK(adf_phase_get_acb(z, theta, user_p) == ADF_OK); CHECK(calls == 1);
    calls = 0;
    CHECK(adf_adele_psi_tate(z, x, user_p) == ADF_OK); CHECK(calls == 4);
    user_p = ADF_REAL_PREC_MAX-16; mode = 1; calls = 0;
    acb_set_si(z, 17); acb_set(saved, z);
    CHECK(adf_phase_get_acb(z, theta, user_p) == ADF_NOT_DETERMINED);
    CHECK(acb_equal(z, saved)); CHECK(calls == 1);
    calls = 0;
    CHECK(adf_adele_psi_tate(z, x, user_p) == ADF_NOT_DETERMINED);
    CHECK(acb_equal(z, saved)); CHECK(calls == 1);

    /* Radius denominator fits, but Q4's projected division does not. */
    arb_one(x->inf); arb_mul_2exp_si(x->inf, x->inf, -ADF_QCLASS_EXP_MAX-1);
    fmpq_zero(a->q); fmpq_one(N->q);
    fmpz_mul_2exp(fmpq_denref(N->q), fmpq_denref(N->q), ADF_QCLASS_EXP_MAX);
    fmpz_add_ui(fmpq_denref(N->q), fmpq_denref(N->q), 1);
    CHECK(adf_fball_set_center_radius(&x->fin, a, N) == ADF_OK);
    limits_active = 1;
    CHECK(adf_adele_psi_tate(z, x, 53) == ADF_LIMIT); CHECK(acb_equal(z, saved));
    CHECK(adf_adele_psi_tate_strict(z, x, 53) == ADF_LIMIT); CHECK(acb_equal(z, saved));
    limits_active = 0;

    /* Radius denominator fits, but Q4's projected multiplication does not. */
    fmpq_set_si(theta, 85, 256); arb_set_fmpq(x->inf, theta, 128);
    fmpq_one(N->q);
    fmpz_mul_2exp(fmpq_denref(N->q), fmpq_denref(N->q), ADF_QCLASS_BITS_MAX-4);
    fmpz_add_ui(fmpq_denref(N->q), fmpq_denref(N->q), 1);
    CHECK(adf_fball_set_center_radius(&x->fin, a, N) == ADF_OK);
    limits_active = 1;
    CHECK(adf_adele_psi_tate(z, x, 53) == ADF_LIMIT); CHECK(acb_equal(z, saved));
    CHECK(adf_adele_psi_tate_strict(z, x, 53) == ADF_LIMIT); CHECK(acb_equal(z, saved));

    adf_adele_clear(x); adf_rat_clear(a); adf_rat_clear(N); fmpq_clear(theta);
    acb_clear(z); acb_clear(saved); flint_cleanup();
    printf("psi retry/preflight: %u checks\n", checks); return 0;
}
