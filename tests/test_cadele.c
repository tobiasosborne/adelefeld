/* tests/test_cadele.c: adf_cadele against its rules and against the m1-adele vector oracle.

   The contract is the comment block of include/adelefeld/adele.h; the set statements are
   docs/SPEC.md 4.1 (C x A_f, "(i ; 0) squares to (-1 ; 0)"), 4.3 and 4.5, and docs/conventions.md
   5.5. The complex coordinate is an acb ball: this test builds it from a real and an imaginary
   rational interval with arb_set_fmpq at the vector's prec and arb_add_error of the exact
   difference (arb.rst:167, :394), then acb_set_arb_arb (acb.rst:124). */

#include <stdio.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld/adele.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* --------------------------------------------------------------- JSON helpers */

static int
json_fmpz(fmpz_t v, const jsonl_value * j, jsonl_error_t * err)
{
    const char * text = jsonl_int_text(j, err);

    if (text == NULL)
        return 0;
    return fmpz_set_str(v, text, 10) == 0;
}

static int
json_fmpq(fmpq_t q, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value * jn, * jd;
    fmpz_t n, d;
    int ok;

    if (!jsonl_field(o, "num", &jn, err) || !jsonl_field(o, "den", &jd, err))
        return 0;
    fmpz_init(n);
    fmpz_init(d);
    ok = json_fmpz(n, jn, err) && json_fmpz(d, jd, err);
    if (ok)
        fmpq_set_fmpz_frac(q, n, d);
    fmpz_clear(n);
    fmpz_clear(d);
    return ok;
}

static int
json_ball(adf_fball_t b, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value * jA, * jH, * jd;
    fmpz_t A, H, d;
    int ok;

    if (!jsonl_field(o, "A", &jA, err) || !jsonl_field(o, "H", &jH, err) ||
        !jsonl_field(o, "d", &jd, err))
        return 0;
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    ok = json_fmpz(A, jA, err) && json_fmpz(H, jH, err) && json_fmpz(d, jd, err) &&
         adf_fball_set_fmpz3(b, A, H, d) == ADF_OK;
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    return ok;
}

/* An arb that contains the exact closed interval [lo, hi] (arb.rst:167, :394). */
static int
json_interval(arb_t x, const jsonl_value * o, slong prec, jsonl_error_t * err)
{
    const jsonl_value * jlo, * jhi;
    fmpq_t lo, hi, diff;
    arb_t e;
    int ok;

    if (!jsonl_field(o, "lo", &jlo, err) || !jsonl_field(o, "hi", &jhi, err))
        return 0;
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(diff);
    arb_init(e);
    ok = json_fmpq(lo, jlo, err) && json_fmpq(hi, jhi, err);
    if (ok)
    {
        arb_set_fmpq(x, lo, prec);
        fmpq_sub(diff, hi, lo);
        arb_set_fmpq(e, diff, prec);
        arb_add_error(x, e);
    }
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(diff);
    arb_clear(e);
    return ok;
}

static int
json_cadele(adf_cadele_t x, const jsonl_value * o, slong prec, jsonl_error_t * err)
{
    const jsonl_value * jre, * jim, * jfin;
    arb_t re, im;
    int ok;

    if (!jsonl_field(o, "re", &jre, err) || !jsonl_field(o, "im", &jim, err) ||
        !jsonl_field(o, "fin", &jfin, err))
        return 0;
    arb_init(re);
    arb_init(im);
    ok = json_interval(re, jre, prec, err) && json_interval(im, jim, prec, err);
    if (ok)
    {
        acb_set_arb_arb(x->inf, re, im);
        ok = json_ball(&x->fin, jfin, err);
    }
    arb_clear(re);
    arb_clear(im);
    return ok;
}

static int
json_fmpz2(const fmpz_t v, const jsonl_value * j, jsonl_error_t * err)
{
    fmpz_t w;
    int ok;

    fmpz_init(w);
    ok = json_fmpz(w, j, err) && fmpz_equal(v, w);
    fmpz_clear(w);
    return ok;
}

/* Compare the canonical triple of b with the JSON ball {A,H,d}. */
static int
ball_equals_json(const adf_fball_t b, const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value * jA, * jH, * jd;
    fmpz_t A, H, d;
    int ok;

    if (!jsonl_field(o, "A", &jA, err) || !jsonl_field(o, "H", &jH, err) ||
        !jsonl_field(o, "d", &jd, err))
        return 0;
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    adf_fball_get_fmpz3(A, H, d, b);
    ok = json_fmpz2(A, jA, err) && json_fmpz2(H, jH, err) && json_fmpz2(d, jd, err);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    return ok;
}

/* Membership of the rational q in the finite ball b, through the adf_rat constructor. */
static int
fball_contains_fmpq(const adf_fball_t b, const fmpq_t q)
{
    adf_rat_t r;
    int res;

    adf_rat_init(r);
    if (adf_rat_set_fmpq(r, q) != ADF_OK)
        res = 0;
    else
        res = adf_fball_contains_rat(b, r);
    adf_rat_clear(r);
    return res;
}

/* ------------------------------------------------------------ vector tests */

static void
run_cadele_record(const jsonl_value * rec, jsonl_error_t * err)
{
    const jsonl_value * jop, * jprec, * ja, * jb, * jq, * jres, * jw;
    const char * op;
    size_t oplen, i;
    slong prec;
    adf_cadele_t x, y, z;
    adf_fball_t ref;
    adf_rat_t q;
    fmpq_t rq;
    fmpz_t Jprec;
    int has_b, has_q, has_w;

    ADF_CHECK(jsonl_field(rec, "op", &jop, err) == 1);
    ADF_CHECK(jsonl_field(rec, "prec", &jprec, err) == 1);
    ADF_CHECK(jsonl_field(rec, "a", &ja, err) == 1);
    has_b = jsonl_field(rec, "b", &jb, err) == 1;
    has_q = jsonl_field(rec, "q", &jq, err) == 1;
    has_w = jsonl_field(rec, "witness", &jw, err) == 1;
    ADF_CHECK(jsonl_field(rec, "result_fin", &jres, err) == 1);
    if (!jop || !jprec || !ja || !jres)
        return;

    op = jsonl_string(jop, &oplen, err);
    ADF_CHECK(op != NULL);
    if (op == NULL)
        return;

    fmpz_init(Jprec);
    ADF_CHECK(json_fmpz(Jprec, jprec, err));
    prec = fmpz_get_si(Jprec);
    fmpz_clear(Jprec);

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    adf_fball_init(ref);
    adf_rat_init(q);
    fmpq_init(rq);

    ADF_CHECK(json_cadele(x, ja, prec, err));
    if (has_b)
        ADF_CHECK(json_cadele(y, jb, prec, err));
    if (has_q)
        ADF_CHECK(json_fmpq(rq, jq, err) && adf_rat_set_fmpq(q, rq) == ADF_OK);

    if (strcmp(op, "add") == 0)
    {
        adf_cadele_add(z, x, y, prec);
        adf_fball_add(ref, &x->fin, &y->fin);
    }
    else if (strcmp(op, "sub") == 0)
    {
        adf_cadele_sub(z, x, y, prec);
        adf_fball_sub(ref, &x->fin, &y->fin);
    }
    else if (strcmp(op, "mul") == 0)
    {
        adf_cadele_mul(z, x, y, prec);
        adf_fball_mul(ref, &x->fin, &y->fin);
    }
    else if (strcmp(op, "neg") == 0)
    {
        adf_cadele_neg(z, x);
        adf_fball_neg(ref, &x->fin);
    }
    else if (strcmp(op, "add_rat") == 0)
    {
        adf_fball_t fq;
        adf_fball_init(fq);
        adf_fball_set_rat(fq, q);
        adf_cadele_add_rat(z, x, q, prec);
        adf_fball_add(ref, &x->fin, fq);
        adf_fball_clear(fq);
    }
    else if (strcmp(op, "mul_rat") == 0)
    {
        adf_cadele_mul_rat(z, x, q, prec);
        adf_fball_mul_rat(ref, &x->fin, q);
    }
    else if (strcmp(op, "div_rat") == 0)
    {
        ADF_CHECK(adf_cadele_div_rat(z, x, q, prec) == ADF_OK);
        adf_fball_div_rat(ref, &x->fin, q);
    }
    else
    {
        ADF_CHECK_MSG(0, "unknown op \"%s\"", op);
        goto done;
    }

    ADF_CHECK(adf_cadele_is_canonical(z));
    ADF_CHECK_MSG(adf_fball_identical(&z->fin, ref), "%s line %lu", op, jsonl_line_of(rec));
    ADF_CHECK_MSG(ball_equals_json(&z->fin, jres, err), "%s line %lu", op, jsonl_line_of(rec));

    if (has_w)
    {
        size_t nw = jsonl_size(jw);
        for (i = 0; i < nw; i++)
        {
            const jsonl_value * w = jsonl_at(jw, i, err);
            fmpq_t are, aim, bre, bim, aimg, biimg, faimg, fbre, fbimg;
            const jsonl_value * v;

            if (w == NULL)
            {
                ADF_CHECK(w != NULL);
                continue;
            }
            fmpq_init(are);
            fmpq_init(aim);
            fmpq_init(bre);
            fmpq_init(bim);
            fmpq_init(aimg);
            fmpq_init(biimg);
            fmpq_init(faimg);
            fmpq_init(fbre);
            fmpq_init(fbimg);

            ADF_CHECK(jsonl_field(w, "a_re", &v, err) == 1 && json_fmpq(are, v, err));
            ADF_CHECK(jsonl_field(w, "a_im", &v, err) == 1 && json_fmpq(aim, v, err));
            ADF_CHECK(jsonl_field(w, "re_image", &v, err) == 1 && json_fmpq(aimg, v, err));
            ADF_CHECK(jsonl_field(w, "im_image", &v, err) == 1 && json_fmpq(biimg, v, err));
            ADF_CHECK(jsonl_field(w, "a_fin", &v, err) == 1 && json_fmpq(faimg, v, err));
            ADF_CHECK(jsonl_field(w, "fin_image", &v, err) == 1 && json_fmpq(fbimg, v, err));

            ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(x->inf), are),
                          "%s line %lu: a_re not in input", op, jsonl_line_of(rec));
            ADF_CHECK_MSG(arb_contains_fmpq(acb_imagref(x->inf), aim),
                          "%s line %lu: a_im not in input", op, jsonl_line_of(rec));
            ADF_CHECK_MSG(fball_contains_fmpq(&x->fin, faimg),
                          "%s line %lu: a_fin not in input", op, jsonl_line_of(rec));
            if (has_b)
            {
                ADF_CHECK(jsonl_field(w, "b_re", &v, err) == 1 && json_fmpq(bre, v, err));
                ADF_CHECK(jsonl_field(w, "b_im", &v, err) == 1 && json_fmpq(bim, v, err));
                ADF_CHECK(jsonl_field(w, "b_fin", &v, err) == 1 && json_fmpq(fbre, v, err));
                ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(y->inf), bre),
                              "%s line %lu: b_re not in input", op, jsonl_line_of(rec));
                ADF_CHECK_MSG(arb_contains_fmpq(acb_imagref(y->inf), bim),
                              "%s line %lu: b_im not in input", op, jsonl_line_of(rec));
                ADF_CHECK_MSG(fball_contains_fmpq(&y->fin, fbre),
                              "%s line %lu: b_fin not in input", op, jsonl_line_of(rec));
            }
            ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(z->inf), aimg),
                          "%s line %lu: real image not in output", op, jsonl_line_of(rec));
            ADF_CHECK_MSG(arb_contains_fmpq(acb_imagref(z->inf), biimg),
                          "%s line %lu: imaginary image not in output", op,
                          jsonl_line_of(rec));
            ADF_CHECK_MSG(fball_contains_fmpq(&z->fin, fbimg),
                          "%s line %lu: finite image not in output", op, jsonl_line_of(rec));

            fmpq_clear(are);
            fmpq_clear(aim);
            fmpq_clear(bre);
            fmpq_clear(bim);
            fmpq_clear(aimg);
            fmpq_clear(biimg);
            fmpq_clear(faimg);
            fmpq_clear(fbre);
            fmpq_clear(fbimg);
        }
    }

done:
    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(z);
    adf_fball_clear(ref);
    adf_rat_clear(q);
    fmpq_clear(rq);
}

ADF_TEST(cadele_ops_vectors)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-adele/cadele_ops.jsonl", &f, &err) == 1,
                  "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
        run_cadele_record(jsonl_record(f, i), &err);
    jsonl_close(f);
}

ADF_TEST(cadele_set_rat_vectors)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-adele/set_rat.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * jq, * jp, * jex;
        fmpq_t q;
        fmpz_t zp;
        slong prec;
        int exact, got;
        adf_rat_t r;
        adf_cadele_t x;

        ADF_CHECK(jsonl_field(rec, "q", &jq, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "prec", &jp, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "exact", &jex, &err) == 1);
        fmpq_init(q);
        fmpz_init(zp);
        adf_rat_init(r);
        adf_cadele_init(x);
        ADF_CHECK(json_fmpq(q, jq, &err));
        ADF_CHECK(json_fmpz(zp, jp, &err));
        ADF_CHECK(jsonl_bool(jex, &exact, &err) == 1);
        prec = fmpz_get_si(zp);
        ADF_CHECK(adf_rat_set_fmpq(r, q) == ADF_OK);
        adf_cadele_set_rat(x, r, prec);
        ADF_CHECK(adf_cadele_is_canonical(x));
        ADF_CHECK_MSG(acb_contains_fmpq(x->inf, q), "cadele set_rat line %lu: q not in the ball",
                      jsonl_line_of(rec));
        ADF_CHECK_MSG(arb_is_zero(acb_imagref(x->inf)), "cadele set_rat line %lu: imaginary part "
                      "not exactly zero", jsonl_line_of(rec));
        ADF_CHECK(fball_contains_fmpq(&x->fin, q));
        ADF_CHECK(adf_fball_is_exact(&x->fin));
        got = arb_is_exact(acb_realref(x->inf));
        ADF_CHECK_MSG(got == exact, "cadele set_rat line %lu: exactness %d, expected %d",
                      jsonl_line_of(rec), got, exact);
        adf_rat_clear(r);
        adf_cadele_clear(x);
        fmpq_clear(q);
        fmpz_clear(zp);
    }
    jsonl_close(f);
}

/* -------------------------------------------------------------- direct tests */

ADF_TEST(cadele_init_is_the_exact_zero)
{
    adf_cadele_t x;

    adf_cadele_init(x);
    ADF_CHECK(adf_cadele_is_canonical(x));
    ADF_CHECK(acb_is_zero(x->inf));
    ADF_CHECK(acb_is_exact(x->inf));
    ADF_CHECK(adf_fball_is_exact(&x->fin));
    adf_cadele_clear(x);
}

ADF_TEST(cadele_set_and_swap)
{
    adf_cadele_t x, y;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_rat_init(q);
    adf_rat_set_si(q, 5);
    adf_cadele_set_rat(x, q, 53);
    adf_cadele_set(y, x);
    ADF_CHECK(adf_cadele_identical(x, y));
    ADF_CHECK(adf_cadele_is_canonical(y));

    adf_rat_set_si(q, -3);
    adf_cadele_set_rat(y, q, 53);
    ADF_CHECK(adf_cadele_identical(x, y) == 0);
    adf_cadele_swap(x, y);
    ADF_CHECK(arb_contains_si(acb_realref(x->inf), -3));
    ADF_CHECK(arb_contains_si(acb_realref(y->inf), 5));

    /* y may be x for set and swap. */
    adf_cadele_set(x, x);
    ADF_CHECK(arb_contains_si(acb_realref(x->inf), -3));
    adf_cadele_swap(x, x);
    ADF_CHECK(arb_contains_si(acb_realref(x->inf), -3));

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_rat_clear(q);
}

ADF_TEST(cadele_set_rat_has_exact_zero_imaginary_part)
{
    adf_cadele_t x;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_rat_init(q);
    adf_rat_set_si(q, -3);
    adf_cadele_set_rat(x, q, 53);
    ADF_CHECK(adf_cadele_is_canonical(x));
    ADF_CHECK(arb_is_zero(acb_imagref(x->inf)));
    ADF_CHECK(arb_contains_si(acb_realref(x->inf), -3));
    ADF_CHECK(adf_fball_is_exact(&x->fin));
    ADF_CHECK(fmpz_equal_si(x->fin.A, -3));
    adf_cadele_clear(x);
    adf_rat_clear(q);
}

ADF_TEST(cadele_set_adele_is_exact)
{
    adf_adele_t a;
    adf_cadele_t y;

    adf_adele_init(a);
    adf_cadele_init(y);
    adf_adele_set_si(a, 7);
    adf_cadele_set_adele(y, a);
    ADF_CHECK(arb_equal(acb_realref(y->inf), a->inf));
    ADF_CHECK(arb_is_zero(acb_imagref(y->inf)));
    ADF_CHECK(adf_fball_identical(&y->fin, &a->fin));
    ADF_CHECK(arb_contains_si(acb_realref(y->inf), 7));
    adf_adele_clear(a);
    adf_cadele_clear(y);
}

ADF_TEST(i_squared_is_minus_one)
{
    adf_cadele_t i, z, expected, ring_minus_one;
    adf_adele_t minus_one;

    adf_cadele_init(i);
    adf_cadele_init(z);
    adf_cadele_init(expected);
    adf_cadele_init(ring_minus_one);
    adf_adele_init(minus_one);

    acb_onei(i->inf);
    ADF_CHECK(adf_fball_is_exact(&i->fin));
    adf_cadele_mul(z, i, i, 53);

    /* (-1 ; 0): the complex coordinate is -1, the finite coordinate stays 0. */
    acb_set_si(expected->inf, -1);
    ADF_CHECK_MSG(adf_cadele_identical(z, expected), "(i ; 0)^2 is not (-1 ; 0)");
    ADF_CHECK(arb_is_zero(acb_imagref(z->inf)));
    ADF_CHECK(adf_fball_is_exact(&z->fin));

    /* It is not the -1 of the ring R x A_f, which also has finite coordinate -1. */
    adf_adele_set_si(minus_one, -1);
    adf_cadele_set_adele(ring_minus_one, minus_one);
    ADF_CHECK(adf_cadele_identical(z, ring_minus_one) == 0);

    adf_cadele_clear(i);
    adf_cadele_clear(z);
    adf_cadele_clear(expected);
    adf_cadele_clear(ring_minus_one);
    adf_adele_clear(minus_one);
}

ADF_TEST(cadele_set_acb_fball_rejects_a_non_finite_complex_ball)
{
    adf_cadele_t y, before;
    adf_fball_t f;
    acb_t z;

    adf_cadele_init(y);
    adf_cadele_init(before);
    adf_fball_init(f);
    acb_init(z);

    adf_fball_set_si(f, 9);
    acb_set_si(z, 4);
    ADF_CHECK(adf_cadele_set_acb_fball(y, z, f) == ADF_OK);
    ADF_CHECK(adf_cadele_is_canonical(y));
    ADF_CHECK(arb_contains_si(acb_realref(y->inf), 4));
    adf_cadele_set(before, y);

    acb_indeterminate(z);
    ADF_CHECK(adf_cadele_set_acb_fball(y, z, f) == ADF_DOMAIN);
    ADF_CHECK_MSG(adf_cadele_identical(y, before), "the output changed on ADF_DOMAIN");

    acb_set_si(z, 0);
    arb_pos_inf(acb_realref(z));
    ADF_CHECK(adf_cadele_set_acb_fball(y, z, f) == ADF_DOMAIN);
    ADF_CHECK(adf_cadele_identical(y, before));

    acb_zero(z);
    mag_inf(arb_radref(acb_realref(z)));
    ADF_CHECK(adf_cadele_set_acb_fball(y, z, f) == ADF_DOMAIN);
    ADF_CHECK(adf_cadele_identical(y, before));

    adf_cadele_clear(y);
    adf_cadele_clear(before);
    adf_fball_clear(f);
    acb_clear(z);
}

ADF_TEST(cadele_div_rat_by_zero_is_not_unit_and_leaves_z)
{
    adf_cadele_t x, z, before;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_cadele_init(z);
    adf_cadele_init(before);
    adf_rat_init(q);
    adf_rat_set_si(q, 5);
    adf_cadele_set_rat(x, q, 53);
    adf_cadele_set_rat(z, q, 53);
    adf_cadele_set(before, z);
    adf_rat_zero(q);
    ADF_CHECK(adf_cadele_div_rat(z, x, q, 53) == ADF_NOT_UNIT);
    ADF_CHECK_MSG(adf_cadele_identical(z, before), "z changed on ADF_NOT_UNIT");
    adf_cadele_clear(x);
    adf_cadele_clear(z);
    adf_cadele_clear(before);
    adf_rat_clear(q);
}

static void
check_cadele_binary_aliasing(void (*op)(adf_cadele_t, const adf_cadele_t, const adf_cadele_t,
                                         slong))
{
    adf_cadele_t x, y, z, expected;

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    adf_cadele_init(expected);
    {
        adf_rat_t q;
        adf_rat_init(q);
        adf_rat_set_si(q, 6);
        adf_cadele_set_rat(x, q, 53);
        adf_rat_set_si(q, -4);
        adf_cadele_set_rat(y, q, 53);
        adf_rat_clear(q);
    }

    op(expected, x, y, 53);
    adf_cadele_set(z, x);
    op(z, z, y, 53);
    ADF_CHECK_MSG(adf_cadele_identical(z, expected), "output aliasing the first input");
    adf_cadele_set(z, y);
    op(z, x, z, 53);
    ADF_CHECK_MSG(adf_cadele_identical(z, expected), "output aliasing the second input");
    adf_cadele_set(z, x);
    op(expected, z, z, 53);
    op(z, z, z, 53);
    ADF_CHECK_MSG(adf_cadele_identical(z, expected), "output aliasing both inputs");

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(z);
    adf_cadele_clear(expected);
}

ADF_TEST(cadele_aliasing_of_add_sub_mul)
{
    check_cadele_binary_aliasing(adf_cadele_add);
    check_cadele_binary_aliasing(adf_cadele_sub);
    check_cadele_binary_aliasing(adf_cadele_mul);
}

ADF_TEST(cadele_aliasing_of_neg_and_scale)
{
    adf_cadele_t x, y, expected;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(expected);
    adf_rat_init(q);
    adf_rat_set_si(q, 3);
    adf_cadele_set_rat(x, q, 53);
    fmpq_set_si(q->q, 5, 2);

    adf_cadele_neg(expected, x);
    adf_cadele_set(y, x);
    adf_cadele_neg(y, y);
    ADF_CHECK(adf_cadele_identical(y, expected));

    adf_cadele_add_rat(expected, x, q, 53);
    adf_cadele_set(y, x);
    adf_cadele_add_rat(y, y, q, 53);
    ADF_CHECK(adf_cadele_identical(y, expected));

    adf_cadele_mul_rat(expected, x, q, 53);
    adf_cadele_set(y, x);
    adf_cadele_mul_rat(y, y, q, 53);
    ADF_CHECK(adf_cadele_identical(y, expected));

    ADF_CHECK(adf_cadele_div_rat(expected, x, q, 53) == ADF_OK);
    adf_cadele_set(y, x);
    ADF_CHECK(adf_cadele_div_rat(y, y, q, 53) == ADF_OK);
    ADF_CHECK(adf_cadele_identical(y, expected));

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(expected);
    adf_rat_clear(q);
}

ADF_TEST(cadele_is_canonical_needs_both_coordinates)
{
    adf_cadele_t x;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_rat_init(q);
    adf_rat_set_si(q, 1);
    adf_cadele_set_rat(x, q, 53);
    ADF_CHECK(adf_cadele_is_canonical(x));

    /* The complex coordinate is finite, the finite part is not canonical. */
    fmpz_set_si(x->fin.H, -1);
    ADF_CHECK(adf_cadele_is_canonical(x) == 0);

    /* The finite part is canonical again, the complex coordinate is not finite. */
    adf_fball_set_si(&x->fin, 0);
    acb_indeterminate(x->inf);
    ADF_CHECK(adf_cadele_is_canonical(x) == 0);

    adf_cadele_clear(x);
    adf_rat_clear(q);
}

ADF_TEST(cadele_identical_separates_the_parts)
{
    adf_cadele_t x, y;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_rat_init(q);
    adf_rat_set_si(q, 5);
    adf_cadele_set_rat(x, q, 53);
    adf_cadele_set_rat(y, q, 53);
    ADF_CHECK(adf_cadele_identical(x, y));

    /* Differing real midpoint. */
    adf_rat_set_si(q, 6);
    adf_cadele_set_rat(y, q, 53);
    ADF_CHECK(adf_cadele_identical(x, y) == 0);

    /* Differing imaginary part. */
    adf_rat_set_si(q, 5);
    adf_cadele_set_rat(y, q, 53);
    ADF_CHECK(adf_cadele_identical(x, y));
    arb_add_error_2exp_si(acb_imagref(y->inf), -10);
    ADF_CHECK(adf_cadele_identical(x, y) == 0);

    /* Differing radius of the real part. */
    adf_cadele_set_rat(y, q, 53);
    arb_add_error_2exp_si(acb_realref(y->inf), -10);
    ADF_CHECK(adf_cadele_identical(x, y) == 0);

    /* Differing finite part. */
    adf_cadele_set_rat(y, q, 53);
    ADF_CHECK(adf_cadele_identical(x, y));
    {
        fmpz_t A, H, d;
        adf_fball_t f;
        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        adf_fball_init(f);
        fmpz_set_si(A, 1);
        fmpz_set_si(H, 4);
        fmpz_set_si(d, 1);
        ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
        ADF_CHECK(adf_cadele_set_acb_fball(y, y->inf, f) == ADF_OK);
        ADF_CHECK(adf_cadele_identical(x, y) == 0);
        adf_fball_clear(f);
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_rat_clear(q);
}

ADF_TEST(cadele_finite_coordinate_does_not_depend_on_prec)
{
    adf_cadele_t x, z2, z4096;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_cadele_init(z2);
    adf_cadele_init(z4096);
    adf_rat_init(q);
    adf_rat_set_si(q, 3);
    adf_cadele_set_rat(x, q, 53);
    fmpq_set_si(q->q, 1, 3);
    adf_cadele_add_rat(z2, x, q, 2);
    adf_cadele_add_rat(z4096, x, q, 4096);
    ADF_CHECK(adf_fball_identical(&z2->fin, &z4096->fin));
    adf_cadele_mul_rat(z2, x, q, 2);
    adf_cadele_mul_rat(z4096, x, q, 4096);
    ADF_CHECK(adf_fball_identical(&z2->fin, &z4096->fin));
    ADF_CHECK(adf_cadele_div_rat(z2, x, q, 2) == ADF_OK);
    ADF_CHECK(adf_cadele_div_rat(z4096, x, q, 4096) == ADF_OK);
    ADF_CHECK(adf_fball_identical(&z2->fin, &z4096->fin));
    adf_cadele_clear(x);
    adf_cadele_clear(z2);
    adf_cadele_clear(z4096);
    adf_rat_clear(q);
}

ADF_TEST(cadele_get_complex_and_get_fin)
{
    adf_cadele_t x;
    adf_fball_t f;
    acb_t z;
    adf_rat_t q;

    adf_cadele_init(x);
    adf_fball_init(f);
    acb_init(z);
    adf_rat_init(q);
    adf_rat_set_si(q, 9);
    adf_cadele_set_rat(x, q, 53);

    adf_cadele_get_complex(z, x);
    ADF_CHECK(acb_equal(z, x->inf));
    adf_cadele_get_fin(f, x);
    ADF_CHECK(adf_fball_identical(f, &x->fin));

    adf_cadele_clear(x);
    adf_fball_clear(f);
    acb_clear(z);
    adf_rat_clear(q);
}
