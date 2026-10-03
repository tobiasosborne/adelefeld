/* f-review7: the _at forms. x has components at inf (arb 9), 2, 5, 7; s has components at 5 and 7 (and inf).
   Checks: the component at v equals the lball function's result on the component; y has the one place v;
   where untouched on OK, = v on failure; y untouched on failure; aliasing y = x, y = s, x = s. */
#include <stdio.h>
#include <adelefeld.h>
static int fails = 0, checks = 0;
#define CK(c) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static adf_place_t P(ulong p) { adf_place_t v; adf_place_prime(&v, p); return v; }
static void ex(adf_lball_t x, ulong p, slong a, slong b) { adf_rat_t r; adf_rat_init(r); { fmpq_t q; fmpq_init(q); fmpq_set_si(q, a, (ulong) b); adf_rat_set_fmpq(r, q); fmpq_clear(q); } adf_lball_set_rat(x, P(p), r); adf_rat_clear(r); }
static void bl(adf_lball_t x, ulong p, slong a, slong N) { adf_rat_t r; adf_rat_init(r); adf_rat_set_si(r, a); adf_lball_set_rat_ball(x, P(p), r, N); adf_rat_clear(r); }
int main(void)
{
    adf_lball_struct loc[3], sl[2]; adf_lball_t c, w; adf_sball_t x, s, y, y0, z; arb_t r; adf_place_t where, sentw;
    for (int i = 0; i < 3; i++) adf_lball_init(loc + i);
    for (int i = 0; i < 2; i++) adf_lball_init(sl + i);
    adf_lball_init(c); adf_lball_init(w); adf_sball_init(x); adf_sball_init(s); adf_sball_init(y); adf_sball_init(y0); adf_sball_init(z);
    arb_init(r); arb_set_si(r, 9);
    ex(loc + 0, 2, 9, 1); bl(loc + 1, 5, 9, 6); ex(loc + 2, 7, 2, 1);
    CK(adf_sball_set_arb_lballs(x, NULL, r, loc, 3) == ADF_OK);
    bl(sl + 0, 5, 2, 2); ex(sl + 1, 7, 1, 3);
    CK(adf_sball_set_arb_lballs(s, NULL, r, sl, 2) == ADF_OK);
    sentw = P(11);
    ulong ps[4] = { 2, 5, 7, 3 };
    for (int k = 0; k < 4; k++)
    {
        adf_place_t v = P(ps[k]);
        for (slong e = -3; e <= 3; e++) for (ulong n = 1; n <= 4; n++) for (ulong seed = 0; seed <= 7; seed++) for (slong N = -1; N <= 9; N += 5)
        {
            int st, st2; where = sentw;
            adf_sball_set(y, s);                         /* sentinel content */
            st = adf_sball_powrat_at(y, &where, x, v, e, n, seed, N);
            if (ps[k] == 3) { CK(st == ADF_DOMAIN && adf_place_equal(where, v) && adf_sball_identical(y, s)); continue; }
            CK(adf_sball_get_lball(c, x, v) == ADF_OK);
            st2 = adf_lball_powrat(w, c, e, n, seed, N);
            CK(st == st2);
            if (st == ADF_OK) {
                adf_lball_t g; adf_lball_init(g); adf_place_t v0;
                CK(adf_place_equal(where, sentw));
                CK(adf_sball_get_lball(g, y, v) == ADF_OK && adf_lball_identical(g, w));
                CK(adf_sball_arch(y) == 0 && adf_sball_get_place(&v0, y, 0) == ADF_OK && adf_place_equal(v0, v));
                CK(adf_sball_get_place(&v0, y, 1) != ADF_OK);
                adf_lball_clear(g);
                adf_sball_set(z, x); CK(adf_sball_powrat_at(z, NULL, z, v, e, n, seed, N) == ADF_OK && adf_sball_identical(z, y));
            } else {
                CK(adf_place_equal(where, v) && adf_sball_identical(y, s));
                adf_sball_set(z, x); CK(adf_sball_powrat_at(z, NULL, z, v, e, n, seed, N) == st && adf_sball_identical(z, x));
            }
        }
        for (slong N = -1; N <= 9; N += 2)
        {
            int st, st2; where = sentw;
            adf_lball_t cu, cs; adf_lball_init(cu); adf_lball_init(cs);
            adf_sball_set(y, x);
            st = adf_sball_powunit_at(y, &where, s, s, v, N);    /* u = s the same object */
            if (ps[k] == 2 || ps[k] == 3) { CK(st == ADF_DOMAIN && adf_place_equal(where, v) && adf_sball_identical(y, x)); }
            else {
                adf_sball_get_lball(cu, s, v);
                st2 = adf_lball_powunit(w, cu, cu, N);
                CK(st == st2);
                if (st == ADF_OK) { adf_lball_t g; adf_lball_init(g); CK(adf_sball_get_lball(g, y, v) == ADF_OK && adf_lball_identical(g, w)); adf_lball_clear(g); CK(adf_place_equal(where, sentw)); }
                else CK(adf_place_equal(where, v) && adf_sball_identical(y, x));
            }
            where = sentw; adf_sball_set(y, x);
            st = adf_sball_powunit_at(y, &where, x, s, v, N);
            if (ps[k] == 2 || ps[k] == 3) CK(st == ADF_DOMAIN && adf_place_equal(where, v));
            else {
                adf_sball_get_lball(cu, x, v); adf_sball_get_lball(cs, s, v);
                st2 = adf_lball_powunit(w, cu, cs, N);
                CK(st == st2);
                if (st == ADF_OK) {
                    adf_lball_t g; adf_lball_init(g); CK(adf_sball_get_lball(g, y, v) == ADF_OK && adf_lball_identical(g, w)); adf_lball_clear(g);
                    adf_sball_set(z, x); CK(adf_sball_powunit_at(z, NULL, z, s, v, N) == ADF_OK && adf_sball_identical(z, y));
                    adf_sball_set(z, s); CK(adf_sball_powunit_at(z, NULL, x, z, v, N) == ADF_OK && adf_sball_identical(z, y));
                } else CK(adf_sball_identical(y, x));
            }
            adf_lball_clear(cu); adf_lball_clear(cs);
        }
    }
    /* the real place: UNSUPPORTED, where = inf */
    where = sentw; adf_sball_set(y, s);
    CK(adf_sball_powrat_at(y, &where, x, adf_place_inf(), 3, 2, 1, 9) == ADF_UNSUPPORTED && adf_place_equal(where, adf_place_inf()) && adf_sball_identical(y, s));
    where = sentw;
    CK(adf_sball_powunit_at(y, &where, x, s, adf_place_inf(), 9) == ADF_UNSUPPORTED && adf_place_equal(where, adf_place_inf()));
    printf("checks %d failures %d\n", checks, fails);
    for (int i = 0; i < 3; i++) adf_lball_clear(loc + i);
    for (int i = 0; i < 2; i++) adf_lball_clear(sl + i);
    adf_lball_clear(c); adf_lball_clear(w); adf_sball_clear(x); adf_sball_clear(s); adf_sball_clear(y); adf_sball_clear(y0); adf_sball_clear(z); arb_clear(r);
    return fails != 0;
}
