/* 1F.7: exact Fraction fixtures from proto/lfunc_trig_checks.py, whole-ball enumeration,
   and exp identities at stated precision. Fails on a wrong residue, exponent, exact flag,
   status, changed output on failure, alias result, or missing smallest-hull witness. */
#include <limits.h>
#include <string.h>
#include <adelefeld.h>
#include "support/jsonl.h"
#include "test_runner.h"

#define EMAX ADF_LBALL_EXP_MAX

typedef int (*fn_t)(adf_lball_t, const adf_lball_t, slong);
static fn_t fs[] = {adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh};
static const char *names[] = {"sin", "cos", "sinh", "cosh"};

static const jsonl_value *field(const jsonl_value *r, const char *key)
{
    const jsonl_value *v = NULL;
    jsonl_error_t e;
    ADF_CHECK_MSG(jsonl_field(r, key, &v, &e), "%s", jsonl_error_message(&e));
    return v;
}
static const char *integer(const jsonl_value *r, const char *key)
{
    const char *s = jsonl_int_text(field(r, key), NULL);
    ADF_CHECK(s != NULL);
    return s ? s : "0";
}
static slong number(const jsonl_value *r, const char *key) { return strtol(integer(r, key), NULL, 10); }
static ulong word(const jsonl_value *r, const char *key) { return strtoul(integer(r, key), NULL, 10); }
static const char *string(const jsonl_value *r, const char *key)
{
    const char *s = jsonl_string(field(r, key), NULL, NULL);
    ADF_CHECK(s != NULL);
    return s ? s : "";
}
static jsonl_file *vectors(const char *name, size_t count)
{
    char path[256];
    jsonl_file *f = NULL;
    jsonl_error_t e;
    snprintf(path, sizeof path, "tests/ref/vectors/f-slice7/%s.jsonl", name);
    ADF_CHECK_MSG(jsonl_open(path, &f, &e), "%s", jsonl_error_message(&e));
    if (f) ADF_CHECK(jsonl_count(f) == count);
    return f;
}
static void read_lb(adf_lball_t x, const jsonl_value *r)
{
    x->p = word(r, "p"); x->exact = (int) number(r, "exact");
    x->v = number(r, "v"); x->N = number(r, "N");
    ADF_CHECK(fmpz_set_str(fmpq_numref(x->u), integer(r, "un"), 10) == 0);
    ADF_CHECK(fmpz_set_str(fmpq_denref(x->u), integer(r, "ud"), 10) == 0);
    ADF_CHECK(adf_lball_is_canonical(x));
}
static void setq(adf_lball_t x, ulong p, const fmpq_t q, int exact, slong M)
{
    adf_rat_t r;
    adf_place_t place;
    adf_rat_init(r); fmpq_set(r->q, q);
    ADF_CHECK(adf_place_prime(&place, p) == ADF_OK);
    ADF_CHECK((exact ? adf_lball_set_rat(x, place, r) : adf_lball_set_rat_ball(x, place, r, M)) == ADF_OK);
    adf_rat_clear(r);
}
static void setsi(adf_lball_t x, ulong p, slong a, int exact, slong M)
{
    fmpq_t q;
    fmpq_init(q); fmpq_set_si(q, a, 1); setq(x, p, q, exact, M); fmpq_clear(q);
}
static void raw(adf_lball_t x, ulong p, slong unit, slong v, slong M, int exact)
{
    x->p = p; x->v = v; x->N = M; x->exact = exact; fmpq_set_si(x->u, unit, 1);
    ADF_CHECK(adf_lball_is_canonical(x));
}
static ulong pow_word(ulong p, slong n)
{
    ulong r = 1;
    while (n-- > 0) r *= p;
    return r;
}

ADF_TEST(exact_reference_all_rows_and_aliases)
{
    jsonl_file *f = vectors("cases", 4064);
    adf_lball_t x, y, z, want, saved;
    if (!f) return;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z);
    adf_lball_init(want); adf_lball_init(saved);
    setsi(saved, 11, 17, 1, 0);
    for (size_t j = 0; j < jsonl_count(f); j++)
    {
        const jsonl_value *r = jsonl_record(f, j);
        int which = 0, expected = ADF_OK, st, ast;
        const char *name = string(r, "f"), *status = string(r, "status");
        while (which < 3 && strcmp(name, names[which])) which++;
        ADF_CHECK(strcmp(name, names[which]) == 0);
        if (!strcmp(status, "DOMAIN")) expected = ADF_DOMAIN;
        if (!strcmp(status, "NOT_DETERMINED")) expected = ADF_NOT_DETERMINED;
        read_lb(x, field(r, "x"));
        adf_lball_set(y, saved); adf_lball_set(z, x);
        st = fs[which](y, x, number(r, "N")); ast = fs[which](z, z, number(r, "N"));
        ADF_CHECK_MSG(st == expected && ast == expected, "row %zu %s statuses %d %d expected %d",
                      j+1, name, st, ast, expected);
        if (expected == ADF_OK)
        {
            read_lb(want, field(r, "y"));
            ADF_CHECK_MSG(adf_lball_identical(y, want), "row %zu %s fields", j+1, name);
            ADF_CHECK_MSG(adf_lball_identical(z, want), "row %zu %s alias", j+1, name);
            ADF_CHECK(adf_lball_is_canonical(y));
        }
        else
            ADF_CHECK(adf_lball_identical(y, saved) && adf_lball_identical(z, x));
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z);
    adf_lball_clear(want); adf_lball_clear(saved); jsonl_close(f);
}

/* All domain balls c<=M<=c+2; every point modulo p^(c+3), output oracle precision H>=E+1.
   sin/sinh and centred cos/cosh must have witnesses separated at exactly E. Noncentred
   cos/cosh must keep the promised safe M, even though their true hull can be smaller. */
ADF_TEST(every_ball_and_every_grid_point)
{
    jsonl_file *f = vectors("points", 503);
    const ulong primes[] = {2, 3, 5, 7};
    adf_lball_t x, y, pt;
    ulong calls = 0, points = 0, witnesses = 0;
    if (!f) return;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(pt);
    for (int ip = 0; ip < 4; ip++)
    {
        ulong p = primes[ip];
        slong c = p == 2 ? 2 : 1;
        for (slong M = c; M <= c+2; M++)
        for (ulong a = 0; a < pow_word(p, M); a += pow_word(p, c))
        for (int which = 0; which < 4; which++)
        {
            slong E = which % 2 && a == 0 ? 2*M-(p == 2) : M;
            slong ns[] = {M-1, E, E+2};
            setsi(x, p, (slong) a, 0, M);
            for (int j = 0; j < 3; j++)
            {
                ulong first = 0, seen = 0;
                int different = 0;
                slong K = ns[j] < E ? ns[j] : E;
                int st = fs[which](y, x, ns[j]);
                ADF_CHECK(st == ADF_OK);
                if (st != ADF_OK) continue;
                ADF_CHECK(!y->exact && y->N == K && y->p == p);
                calls++;
                for (size_t row = 0; row < jsonl_count(f); row++)
                {
                    const jsonl_value *r = jsonl_record(f, row), *v;
                    ulong t, value;
                    if (word(r, "p") != p) continue;
                    t = word(r, "t");
                    if (t % pow_word(p, M) != a) continue;
                    ADF_CHECK(number(r, "H") >= E+1);
                    v = jsonl_at(field(r, "values"), (size_t) which, NULL);
                    value = strtoul(jsonl_int_text(v, NULL), NULL, 10);
                    setsi(pt, p, (slong) value, 1, 0);
                    ADF_CHECK_MSG(adf_lball_contains(pt, y), "%s p=%lu a=%lu M=%ld N=%ld point=%lu",
                                  names[which], p, a, M, ns[j], t);
                    value %= pow_word(p, E+1);
                    if (seen && value != first) different = 1;
                    if (!seen) first = value;
                    seen++; points++;
                }
                ADF_CHECK(seen == pow_word(p, c+3-M));
                if (ns[j] >= E && (which % 2 == 0 || a == 0))
                {
                    ADF_CHECK(different); witnesses++;
                }
            }
        }
    }
    printf("enumeration: %lu balls/requests, %lu point enclosures, %lu hull witnesses\n",
           calls, points, witnesses);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(pt); jsonl_close(f);
}

ADF_TEST(independent_small_truncations)
{
    adf_lball_t x, y, want;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(want);
    setsi(x, 2, 4, 1, 0); setsi(want, 2, 9, 0, 4);
    ADF_CHECK(adf_lball_cos(y, x, 4) == ADF_OK && adf_lball_identical(y, want));
    setsi(x, 3, 3, 1, 0); setsi(want, 3, 3, 0, 2);
    ADF_CHECK(adf_lball_sin(y, x, 2) == ADF_OK && adf_lball_identical(y, want));
    ADF_CHECK(adf_lball_cos(y, x, 5) == ADF_OK);
    setsi(want, 3, 1, 1, 0);
    ADF_CHECK(adf_lball_sub(y, y, want) == ADF_OK && y->v == 2 && !fmpq_is_zero(y->u));
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(want);
}

/* Every status is checked both distinct and aliased, with an unchanged output on failure.
   The W-only limit case has K bits(p)<=BITS_MAX but (K+D) bits(p)>BITS_MAX: no huge allocation. */
ADF_TEST(statuses_limits_and_no_power_shortcuts)
{
    adf_lball_t x, y, z, saved;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_lball_init(saved);
    setsi(saved, 11, 17, 1, 0);
    for (int f = 0; f < 4; f++)
    {
        for (int j = 0; j < 12; j++)
        {
            slong N = 8;
            int want = ADF_LIMIT;
            switch (j)
            {
                case 0: setsi(x, 2, 2, 1, 0); want = ADF_DOMAIN; break;
                case 1: setsi(x, 2, 0, 0, 1); want = ADF_NOT_DETERMINED; break;
                case 2: setsi(x, 3, 1, 0, 2); want = ADF_DOMAIN; break;
                case 3: raw(x, 3, 1, -1, 0, 1); want = ADF_DOMAIN; break;
                case 4: raw(x, 3, 1, EMAX+1, 0, 1); break;
                case 5: raw(x, 2, 0, 0, -EMAX-1, 0); break;
                case 6: setsi(x, 3, 3, 1, 0); N = LONG_MAX; break;
                case 7: setsi(x, 3, 3, 1, 0); N = LONG_MIN; break;
                case 8: setsi(x, 3, 3, 1, 0); N = ADF_LBALL_BITS_MAX/2; break;
                case 9: setsi(x, 2, 4, 1, 0); N = ADF_LBALL_BITS_MAX/2; break;
                case 10: raw(x, 3, 0, 0, EMAX+1, 0); break;
                default: raw(x, 3, 1, 0, EMAX+1, 0); break;
            }
            adf_lball_set(y, saved); adf_lball_set(z, x);
            ADF_CHECK_MSG(fs[f](y, x, N) == want && adf_lball_identical(y, saved),
                          "%s status case %d", names[f], j);
            ADF_CHECK(fs[f](z, z, N) == want && adf_lball_identical(z, x));
        }
        setsi(x, 2, 0, 1, 0);
        ADF_CHECK(fs[f](y, x, LONG_MIN) == ADF_OK && y->exact && y->N == 0);
        ADF_CHECK(fmpq_equal_si(y->u, f % 2));
        ADF_CHECK(fs[f](y, x, LONG_MAX) == ADF_OK && y->exact);
        raw(x, 2, 0, 0, EMAX, 0);
        ADF_CHECK(fs[f](y, x, EMAX) == ADF_OK && !y->exact && y->N == EMAX);
        if (f % 2)
        {
            adf_lball_set(y, saved);
            ADF_CHECK(fs[f](y, x, LONG_MAX) == ADF_LIMIT && adf_lball_identical(y, saved));
            raw(x, 2, 0, 0, (EMAX+1)/2, 0);
            ADF_CHECK(fs[f](y, x, LONG_MAX) == ADF_OK && y->N == EMAX-1 && !y->exact);
        }
        raw(x, 3, 1, EMAX, 0, 1);
        ADF_CHECK(fs[f](y, x, EMAX) == ADF_OK && y->N == EMAX && !y->exact);
        raw(x, 3, 0, 0, 4, 0);
        ADF_CHECK(fs[f](y, x, LONG_MAX) == ADF_OK && !y->exact && y->N == (f % 2 ? 8 : 4));
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_lball_clear(saved);
}

/* exp at N+v_p(2) makes division by 2 safe at requested N. The RHS ball must lie inside
   the returned ball; overlap alone would miss a wrong exponent. Includes p=2^64-59. */
ADF_TEST(hyperbolic_exp_containment)
{
    ulong primes[] = {2, 3, 5, 7, 13, UWORD(18446744073709551557)};
    adf_lball_t x, neg, e, en, rhs, two, y;
    adf_lball_init(x); adf_lball_init(neg); adf_lball_init(e); adf_lball_init(en);
    adf_lball_init(rhs); adf_lball_init(two); adf_lball_init(y);
    for (int ip = 0; ip < 6; ip++)
    for (slong N = 2; N <= 32; N += 6)
    for (int ball = 0; ball < 2; ball++)
    {
        ulong p = primes[ip];
        raw(x, p, p == 3 ? -1 : -3, p == 2 ? 2 : 1, 0, 1);
        if (ball) setsi(x, p, 0, 0, p == 2 ? 3 : 2);
        setsi(two, p, 2, 1, 0);
        ADF_CHECK(adf_lball_neg(neg, x) == ADF_OK);
        ADF_CHECK(adf_lball_exp(e, x, N+(p == 2)) == ADF_OK);
        ADF_CHECK(adf_lball_exp(en, neg, N+(p == 2)) == ADF_OK);
        for (int f = 2; f < 4; f++)
        {
            ADF_CHECK((f == 2 ? adf_lball_sub(rhs, e, en) : adf_lball_add(rhs, e, en)) == ADF_OK);
            ADF_CHECK(adf_lball_div(rhs, rhs, two) == ADF_OK);
            ADF_CHECK(fs[f](y, x, N) == ADF_OK);
            if (!ball)
                ADF_CHECK(y->N == N && adf_lball_contains(rhs, y));
            else
                /* Independent interval arithmetic forgets dependence of exp(x) and exp(-x),
                   so the new result can be narrower (centred cosh, and division at 2). */
                ADF_CHECK(adf_lball_contains(y, rhs));
        }
    }
    adf_lball_clear(x); adf_lball_clear(neg); adf_lball_clear(e); adf_lball_clear(en);
    adf_lball_clear(rhs); adf_lball_clear(two); adf_lball_clear(y);
}

ADF_TEST(circular_exp_with_hensel_i)
{
    jsonl_file *f = vectors("roots", 8);
    adf_lball_t x, ix, im, e, en, rhs, div, y;
    fmpq_t q, iq;
    if (!f) return;
    adf_lball_init(x); adf_lball_init(ix); adf_lball_init(im); adf_lball_init(e);
    adf_lball_init(en); adf_lball_init(rhs); adf_lball_init(div); adf_lball_init(y);
    fmpq_init(q); fmpq_init(iq);
    for (size_t row = 0; row < jsonl_count(f); row++)
    {
        const jsonl_value *r = jsonl_record(f, row);
        ulong p = word(r, "p");
        slong N = number(r, "N");
        ADF_CHECK(number(r, "H") == N+2);
        ADF_CHECK(fmpz_set_str(fmpq_numref(iq), integer(r, "i"), 10) == 0);
        for (slong t = -3; t <= 3; t++)
        {
            fmpq_set_si(q, (slong) p*t, 2);
            setq(x, p, q, 1, 0);
            fmpq_mul(q, q, iq); setq(ix, p, q, 1, 0);
            ADF_CHECK(adf_lball_neg(im, ix) == ADF_OK);
            ADF_CHECK(adf_lball_exp(e, ix, N) == ADF_OK && adf_lball_exp(en, im, N) == ADF_OK);
            for (int k = 0; k < 2; k++)
            {
                ADF_CHECK((k == 0 ? adf_lball_sub(rhs, e, en) : adf_lball_add(rhs, e, en)) == ADF_OK);
                if (k == 0) fmpq_mul_ui(q, iq, 2); else fmpq_set_si(q, 2, 1);
                setq(div, p, q, 1, 0);
                ADF_CHECK(adf_lball_div(rhs, rhs, div) == ADF_OK);
                ADF_CHECK(fs[k](y, x, N) == ADF_OK && adf_lball_contains(rhs, y));
                ADF_CHECK(t == 0 ? y->exact : (!y->exact && y->N == N));
            }
        }
    }
    adf_lball_clear(x); adf_lball_clear(ix); adf_lball_clear(im); adf_lball_clear(e);
    adf_lball_clear(en); adf_lball_clear(rhs); adf_lball_clear(div); adf_lball_clear(y);
    fmpq_clear(q); fmpq_clear(iq); jsonl_close(f);
}

/* f-repair3: residues of the parity Horner sum pinned at odd (sin, sinh) and even (cos, cosh) top degree L,
   so that a change of the summation (lane f-repair3 paired its steps) is caught by the suite. The values were
   computed by src/lfunc.c before the paired loop (lanes/f-repair3/gen_pins.c) and agree with the Fraction
   oracle proto/lfunc_trig_checks.py point() (lanes/f-repair3/check_pins.py, 20 of 20). C is F10's count,
   L the top degree; both parities of C occur. Each residue r is p^v u of the result, 0 <= r < p^N, at K = N. */
ADF_TEST(pinned_residues_odd_and_even_top_degree)
{
    static const struct
    {
        ulong p; slong num, den, N; const char *r[4];
    } cs[] = {
        /* p=3, x=3/2, w=1, N=20: C=39 (odd); L=37 for sin/sinh, L=38 for cos/cosh */
        {3, 3, 2, 20, {"3168081969", "2838301057", "1926456855", "2169655354"}},
        /* p=3, x=9/2, w=2, N=21: C=14 (even); L=13, L=12 */
        {3, 9, 2, 21, {"7077663567", "6849623575", "3032135415", "5248672345"}},
        /* p=5, x=35/3, w=1, N=12: C=16 (even); L=15, L=14 */
        {5, 35, 3, 12, {"105647845", "85990801", "211082595", "93232951"}},
        /* p=2, x=12/5, w=2, N=30: C=29 (odd); L=27, L=28 */
        {2, 12, 5, 30, {"534315644", "1010450137", "852560316", "915693545"}},
        /* p=2^64-59, x=3p, w=1, N=4: C=5 (odd); L=3, L=4 */
        {UWORD(18446744073709551557), 3, 0, 4,
         {"57896044618658096942840529919475564181354450997768997699819720662034468736053",
          "57896044618658096971087487728715627346049327498531788214734208108064742847881",
          "57896044618658096999334445537955690513806745301583024881339305279143644387290",
          "57896044618658096971087487728715627349111868800820234366314137368671112966122"}},
    };
    adf_lball_t x, y;
    fmpq_t q;
    fmpz_t r, want;
    adf_lball_init(x); adf_lball_init(y); fmpq_init(q); fmpz_init(r); fmpz_init(want);
    for (size_t i = 0; i < sizeof cs / sizeof cs[0]; i++)
    for (int f = 0; f < 4; f++)
    {
        ulong p = cs[i].p;
        if (cs[i].den)
            fmpq_set_si(q, cs[i].num, cs[i].den);
        else
        {
            fmpz_set_ui(fmpq_numref(q), p); fmpz_mul_si(fmpq_numref(q), fmpq_numref(q), cs[i].num);
            fmpz_one(fmpq_denref(q));
        }
        setq(x, p, q, 1, 0);
        ADF_CHECK(fs[f](y, x, cs[i].N) == ADF_OK && !y->exact && y->N == cs[i].N && y->p == p);
        ADF_CHECK(fmpz_is_one(fmpq_denref(y->u)));
        fmpz_set_ui(r, p); fmpz_pow_ui(r, r, (ulong) y->v); fmpz_mul(r, r, fmpq_numref(y->u));
        ADF_CHECK(fmpz_set_str(want, cs[i].r[f], 10) == 0);
        ADF_CHECK_MSG(fmpz_equal(r, want), "%s p=%lu case %zu residue", names[f], p, i);
    }
    adf_lball_clear(x); adf_lball_clear(y); fmpq_clear(q); fmpz_clear(r); fmpz_clear(want);
}
