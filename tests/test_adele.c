/* tests/test_adele.c: adf_adele against its rules and against the m1-adele vector oracle.

   The contract is the comment block of include/adelefeld/adele.h; the set statements are
   docs/SPEC.md 4.1, 4.3, 4.5 and docs/conventions.md 5.5. The vectors under
   tests/ref/vectors/m1-adele/ are the oracle of lanes/m1-adele/gen_adele_vectors.py, which uses
   tests/ref/adfref/adele_ref.py; this file does not read the Python reference.

   The real coordinate of a vector is an exact closed interval [lo, hi] with rational ends. The
   test builds an arb that contains it with arb_set_fmpq at the vector's prec and arb_add_error
   of the exact difference (arb.rst:167 and :394); the claim is then that the C output contains
   the exact image of every rational point the vector names. */

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
json_adele(adf_adele_t x, const jsonl_value * o, slong prec, jsonl_error_t * err)
{
    const jsonl_value * jre, * jfin;

    if (!jsonl_field(o, "re", &jre, err) || !jsonl_field(o, "fin", &jfin, err))
        return 0;
    return json_interval(x->inf, jre, prec, err) && json_ball(&x->fin, jfin, err);
}

/* json_fmpz but writing nothing: read the JSON integer and compare with v. */
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

/* One record of adele_ops.jsonl. */
static void
run_adele_record(const jsonl_value * rec, jsonl_error_t * err)
{
    const jsonl_value * jop, * jprec, * ja, * jb, * jq, * jres, * jrre, * jw;
    const char * op;
    size_t oplen, i;
    slong prec;
    adf_adele_t x, y, z, before;
    adf_fball_t ref, before_fin;
    adf_rat_t q;
    fmpq_t rq;
    fmpz_t Jprec;
    int has_b, has_q, has_rre, has_w;

    ADF_CHECK(jsonl_field(rec, "op", &jop, err) == 1);
    ADF_CHECK(jsonl_field(rec, "prec", &jprec, err) == 1);
    ADF_CHECK(jsonl_field(rec, "a", &ja, err) == 1);
    has_b = jsonl_field(rec, "b", &jb, err) == 1;
    has_q = jsonl_field(rec, "q", &jq, err) == 1;
    has_w = jsonl_field(rec, "witness", &jw, err) == 1;
    has_rre = jsonl_field(rec, "result_re", &jrre, err) == 1;
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

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    adf_adele_init(before);
    adf_fball_init(ref);
    adf_fball_init(before_fin);
    adf_rat_init(q);
    fmpq_init(rq);

    ADF_CHECK(json_adele(x, ja, prec, err));
    if (has_b)
        ADF_CHECK(json_adele(y, jb, prec, err));
    if (has_q)
        ADF_CHECK(json_fmpq(rq, jq, err) && adf_rat_set_fmpq(q, rq) == ADF_OK);

    adf_adele_set(before, x);
    adf_fball_set(before_fin, &x->fin);

    if (strcmp(op, "add") == 0)
    {
        adf_adele_add(z, x, y, prec);
        adf_fball_add(ref, &x->fin, &y->fin);
    }
    else if (strcmp(op, "sub") == 0)
    {
        adf_adele_sub(z, x, y, prec);
        adf_fball_sub(ref, &x->fin, &y->fin);
    }
    else if (strcmp(op, "mul") == 0)
    {
        adf_adele_mul(z, x, y, prec);
        adf_fball_mul(ref, &x->fin, &y->fin);
    }
    else if (strcmp(op, "neg") == 0)
    {
        adf_adele_neg(z, x);
        adf_fball_neg(ref, &x->fin);
    }
    else if (strcmp(op, "add_rat") == 0)
    {
        adf_fball_t fq;
        adf_fball_init(fq);
        adf_fball_set_rat(fq, q);
        adf_adele_add_rat(z, x, q, prec);
        adf_fball_add(ref, &x->fin, fq);
        adf_fball_clear(fq);
    }
    else if (strcmp(op, "mul_rat") == 0)
    {
        adf_adele_mul_rat(z, x, q, prec);
        adf_fball_mul_rat(ref, &x->fin, q);
    }
    else if (strcmp(op, "div_rat") == 0)
    {
        ADF_CHECK(adf_adele_div_rat(z, x, q, prec) == ADF_OK);
        adf_fball_div_rat(ref, &x->fin, q);
    }
    else
    {
        ADF_CHECK_MSG(0, "unknown op \"%s\"", op);
        goto done;
    }

    /* The finite coordinate is the result of the fball.h function alone and satisfies the
       canonical predicate; it also equals the exact set recorded in the vector. */
    ADF_CHECK(adf_adele_is_canonical(z));
    ADF_CHECK_MSG(adf_fball_identical(&z->fin, ref), "%s line %lu", op,
                  jsonl_line_of(rec));
    ADF_CHECK_MSG(ball_equals_json(&z->fin, jres, err), "%s line %lu", op,
                  jsonl_line_of(rec));

    /* The real coordinate contains the interval hull of the true result: since the output is an
       interval, it suffices that it contains both exact ends. The operation is binary with
       independent inputs, so the image is the interval below. */
    if (has_rre)
    {
        const jsonl_value * jlo, * jhi;
        fmpq_t rlo, rhi;
        ADF_CHECK(jsonl_field(jrre, "lo", &jlo, err) == 1);
        ADF_CHECK(jsonl_field(jrre, "hi", &jhi, err) == 1);
        fmpq_init(rlo);
        fmpq_init(rhi);
        ADF_CHECK(json_fmpq(rlo, jlo, err));
        ADF_CHECK(json_fmpq(rhi, jhi, err));
        ADF_CHECK_MSG(arb_contains_fmpq(z->inf, rlo), "%s line %lu: result_re lo not contained",
                      op, jsonl_line_of(rec));
        ADF_CHECK_MSG(arb_contains_fmpq(z->inf, rhi), "%s line %lu: result_re hi not contained",
                      op, jsonl_line_of(rec));
        fmpq_clear(rlo);
        fmpq_clear(rhi);
    }

    /* Every witness: the named input points lie in the C inputs, and their exact image lies
       in the C output, in each coordinate. */
    if (has_w)
    {
        size_t nw = jsonl_size(jw);
        for (i = 0; i < nw; i++)
        {
            const jsonl_value * w = jsonl_at(jw, i, err);
            fmpq_t are, bre, aimg, faimg, fbre, fbimg;
            const jsonl_value * v;

            ADF_CHECK(w != NULL);
            if (w == NULL)
                continue;
            fmpq_init(are);
            fmpq_init(bre);
            fmpq_init(aimg);
            fmpq_init(faimg);
            fmpq_init(fbre);
            fmpq_init(fbimg);

            ADF_CHECK(jsonl_field(w, "a_re", &v, err) == 1 && json_fmpq(are, v, err));
            ADF_CHECK(jsonl_field(w, "re_image", &v, err) == 1 && json_fmpq(aimg, v, err));
            ADF_CHECK(jsonl_field(w, "a_fin", &v, err) == 1 && json_fmpq(faimg, v, err));
            ADF_CHECK(jsonl_field(w, "fin_image", &v, err) == 1 && json_fmpq(fbimg, v, err));

            ADF_CHECK_MSG(arb_contains_fmpq(x->inf, are), "%s line %lu: a_re not in input",
                          op, jsonl_line_of(rec));
            ADF_CHECK_MSG(fball_contains_fmpq(&x->fin, faimg), "%s line %lu: a_fin not in "
                          "input", op, jsonl_line_of(rec));
            if (has_b)
            {
                ADF_CHECK(jsonl_field(w, "b_re", &v, err) == 1 && json_fmpq(bre, v, err));
                ADF_CHECK(jsonl_field(w, "b_fin", &v, err) == 1 && json_fmpq(fbre, v, err));
                ADF_CHECK_MSG(arb_contains_fmpq(y->inf, bre), "%s line %lu: b_re not in input",
                              op, jsonl_line_of(rec));
                ADF_CHECK_MSG(fball_contains_fmpq(&y->fin, fbre), "%s line %lu: b_fin not in "
                              "input", op, jsonl_line_of(rec));
            }
            ADF_CHECK_MSG(arb_contains_fmpq(z->inf, aimg), "%s line %lu: real image not in "
                          "output", op, jsonl_line_of(rec));
            ADF_CHECK_MSG(fball_contains_fmpq(&z->fin, fbimg), "%s line %lu: finite image not "
                          "in output", op, jsonl_line_of(rec));
            ADF_CHECK_MSG(adf_adele_identical(z, z), "%s line %lu", op, jsonl_line_of(rec));

            fmpq_clear(are);
            fmpq_clear(bre);
            fmpq_clear(aimg);
            fmpq_clear(faimg);
            fmpq_clear(fbre);
            fmpq_clear(fbimg);
        }
    }

done:
    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
    adf_adele_clear(before);
    adf_fball_clear(ref);
    adf_fball_clear(before_fin);
    adf_rat_clear(q);
    fmpq_clear(rq);
}

ADF_TEST(adele_ops_vectors)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-adele/adele_ops.jsonl", &f, &err) == 1,
                  "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
        run_adele_record(jsonl_record(f, i), &err);
    jsonl_close(f);
}

ADF_TEST(adele_set_rat_vectors)
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
        adf_adele_t x;

        ADF_CHECK(jsonl_field(rec, "q", &jq, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "prec", &jp, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "exact", &jex, &err) == 1);
        fmpq_init(q);
        fmpz_init(zp);
        adf_rat_init(r);
        adf_adele_init(x);
        ADF_CHECK(json_fmpq(q, jq, &err));
        ADF_CHECK(json_fmpz(zp, jp, &err));
        ADF_CHECK(jsonl_bool(jex, &exact, &err) == 1);
        prec = fmpz_get_si(zp);
        ADF_CHECK(adf_rat_set_fmpq(r, q) == ADF_OK);
        adf_adele_set_rat(x, r, prec);
        ADF_CHECK_MSG(adf_adele_is_canonical(x), "set_rat line %lu", jsonl_line_of(rec));
        ADF_CHECK_MSG(arb_contains_fmpq(x->inf, q), "set_rat line %lu: q not in the real ball",
                      jsonl_line_of(rec));
        ADF_CHECK_MSG(adf_fball_contains_rat(&x->fin, r), "set_rat line %lu: q not in the finite "
                      "ball", jsonl_line_of(rec));
        ADF_CHECK(adf_fball_is_exact(&x->fin));
        got = arb_is_exact(x->inf);
        ADF_CHECK_MSG(got == exact, "set_rat line %lu: exactness %d, expected %d",
                      jsonl_line_of(rec), got, exact);
        adf_rat_clear(r);
        adf_adele_clear(x);
        fmpq_clear(q);
        fmpz_clear(zp);
    }
    jsonl_close(f);
}

/* -------------------------------------------------------------- direct tests */

ADF_TEST(init_is_the_exact_zero)
{
    adf_adele_t x;

    adf_adele_init(x);
    ADF_CHECK(adf_adele_is_canonical(x));
    ADF_CHECK(arb_is_zero(x->inf));
    ADF_CHECK(arb_is_exact(x->inf));
    ADF_CHECK(adf_fball_is_exact(&x->fin));
    ADF_CHECK(fmpz_is_zero(x->fin.H));
    adf_adele_clear(x);
}

ADF_TEST(set_si_is_exact_in_both_coordinates)
{
    adf_adele_t x;

    adf_adele_init(x);
    adf_adele_set_si(x, -7);
    ADF_CHECK(adf_adele_is_canonical(x));
    ADF_CHECK(arb_is_exact(x->inf));
    ADF_CHECK(arb_contains_si(x->inf, -7));
    ADF_CHECK(adf_fball_is_exact(&x->fin));
    ADF_CHECK(fmpz_equal_si(x->fin.A, -7) && fmpz_is_zero(x->fin.H));
    adf_adele_clear(x);
}

ADF_TEST(set_and_swap_copy_both_coordinates)
{
    adf_adele_t x, y;
    adf_fball_t f;
    arb_t r;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_fball_init(f);
    arb_init(r);
    adf_adele_set_si(x, 5);
    adf_fball_set_si(f, 9);
    arb_set_si(r, 3);
    ADF_CHECK(adf_adele_set_arb_fball(y, r, f) == ADF_OK);
    adf_adele_set(x, y);
    ADF_CHECK(adf_adele_identical(x, y));
    adf_adele_set_si(y, 11);
    ADF_CHECK(!adf_adele_identical(x, y));
    adf_adele_swap(x, y);
    ADF_CHECK(adf_adele_identical(x, y) == 0);
    ADF_CHECK(arb_contains_si(x->inf, 11) && arb_contains_si(y->inf, 3));

    /* y may be x for set and swap. */
    adf_adele_set(x, x);
    ADF_CHECK(arb_contains_si(x->inf, 11));
    adf_adele_swap(x, x);
    ADF_CHECK(arb_contains_si(x->inf, 11));
    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_fball_clear(f);
    arb_clear(r);
}

ADF_TEST(set_arb_fball_rejects_a_non_finite_real_ball)
{
    adf_adele_t y, before;
    adf_fball_t f;
    arb_t r;

    adf_adele_init(y);
    adf_adele_init(before);
    adf_fball_init(f);
    arb_init(r);
    adf_adele_set_si(y, 4);
    adf_adele_set(before, y);
    adf_fball_set_si(f, 9);
    arb_pos_inf(r);
    ADF_CHECK(adf_adele_set_arb_fball(y, r, f) == ADF_DOMAIN);
    ADF_CHECK_MSG(adf_adele_identical(y, before), "the output changed on ADF_DOMAIN");
    arb_indeterminate(r);
    ADF_CHECK(adf_adele_set_arb_fball(y, r, f) == ADF_DOMAIN);
    ADF_CHECK(adf_adele_identical(y, before));
    arb_zero(r);
    mag_inf(arb_radref(r));
    ADF_CHECK(adf_adele_set_arb_fball(y, r, f) == ADF_DOMAIN);
    ADF_CHECK(adf_adele_identical(y, before));
    adf_adele_clear(y);
    adf_adele_clear(before);
    adf_fball_clear(f);
    arb_clear(r);
}

ADF_TEST(get_real_get_fin_and_get_arb_at)
{
    adf_adele_t x;
    adf_fball_t f;
    arb_t r, r0;
    adf_place_t v;

    adf_adele_init(x);
    adf_fball_init(f);
    arb_init(r);
    arb_init(r0);
    adf_adele_set_si(x, 12);

    adf_adele_get_real(r, x);
    ADF_CHECK(arb_contains_si(r, 12) && arb_is_exact(r));
    adf_adele_get_fin(f, x);
    ADF_CHECK(adf_fball_identical(f, &x->fin));

    adf_adele_get_real(r0, x);
    adf_adele_get_arb_at(r, x, adf_place_inf());
    ADF_CHECK(arb_equal(r, r0));

    ADF_CHECK(adf_place_prime(&v, 5) == ADF_OK);
    arb_set_si(r, -99);
    ADF_CHECK(adf_adele_get_arb_at(r, x, v) == ADF_DOMAIN);
    ADF_CHECK_MSG(arb_contains_si(r, -99) && arb_is_exact(r), "r changed on ADF_DOMAIN");

    adf_adele_clear(x);
    adf_fball_clear(f);
    arb_clear(r);
    arb_clear(r0);
}

ADF_TEST(div_rat_by_zero_is_not_unit_and_leaves_z)
{
    adf_adele_t x, z, before;
    adf_rat_t q;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_adele_init(before);
    adf_rat_init(q);
    adf_adele_set_si(x, 5);
    adf_adele_set_si(z, 7);
    adf_adele_set(before, z);
    ADF_CHECK(adf_adele_div_rat(z, x, q, 53) == ADF_NOT_UNIT);
    ADF_CHECK_MSG(adf_adele_identical(z, before), "z changed on ADF_NOT_UNIT");
    adf_adele_clear(x);
    adf_adele_clear(z);
    adf_adele_clear(before);
    adf_rat_clear(q);
}

/* Every permitted aliasing of a binary operation gives the same result as the disjoint call. */
static void
check_binary_aliasing(void (*op)(adf_adele_t, const adf_adele_t, const adf_adele_t, slong))
{
    adf_adele_t x, y, z, expected;
    adf_fball_t f;
    arb_t r;
    fmpz_t A, H, d;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    adf_adele_init(expected);
    adf_fball_init(f);
    arb_init(r);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);

    arb_set_si(r, 6);
    fmpz_set_si(A, 3);
    fmpz_set_si(H, 12);
    fmpz_set_si(d, 1);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_adele_set_arb_fball(x, r, f) == ADF_OK);

    arb_set_si(r, -4);
    fmpz_set_si(A, 5);
    fmpz_set_si(H, 18);
    ADF_CHECK(adf_fball_set_fmpz3(f, A, H, d) == ADF_OK);
    ADF_CHECK(adf_adele_set_arb_fball(y, r, f) == ADF_OK);

    op(expected, x, y, 53);

    /* Output aliases the first input. */
    adf_adele_set(z, x);
    op(z, z, y, 53);
    ADF_CHECK_MSG(adf_adele_identical(z, expected), "output aliasing the first input");

    /* Output aliases the second input. */
    adf_adele_set(z, y);
    op(z, x, z, 53);
    ADF_CHECK_MSG(adf_adele_identical(z, expected), "output aliasing the second input");

    /* Output aliases both inputs: one object is x, y and z. The expected value is computed
       with the same object as both inputs. */
    adf_adele_set(z, x);
    op(expected, z, z, 53);
    op(z, z, z, 53);
    ADF_CHECK_MSG(adf_adele_identical(z, expected), "output aliasing both inputs");

    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
    adf_adele_clear(expected);
    adf_fball_clear(f);
    arb_clear(r);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

ADF_TEST(aliasing_of_add_sub_mul)
{
    check_binary_aliasing(adf_adele_add);
    check_binary_aliasing(adf_adele_sub);
    check_binary_aliasing(adf_adele_mul);
}

ADF_TEST(aliasing_of_neg_and_scale)
{
    adf_adele_t x, y, expected;
    adf_rat_t q;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(expected);
    adf_rat_init(q);
    adf_adele_set_si(x, 3);
    fmpq_set_si(q->q, 5, 2);

    adf_adele_neg(expected, x);
    adf_adele_set(y, x);
    adf_adele_neg(y, y);
    ADF_CHECK(adf_adele_identical(y, expected));

    adf_adele_add_rat(expected, x, q, 53);
    adf_adele_set(y, x);
    adf_adele_add_rat(y, y, q, 53);
    ADF_CHECK(adf_adele_identical(y, expected));

    adf_adele_mul_rat(expected, x, q, 53);
    adf_adele_set(y, x);
    adf_adele_mul_rat(y, y, q, 53);
    ADF_CHECK(adf_adele_identical(y, expected));

    ADF_CHECK(adf_adele_div_rat(expected, x, q, 53) == ADF_OK);
    adf_adele_set(y, x);
    ADF_CHECK(adf_adele_div_rat(y, y, q, 53) == ADF_OK);
    ADF_CHECK(adf_adele_identical(y, expected));

    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(expected);
    adf_rat_clear(q);
}

ADF_TEST(identical_separates_midpoint_radius_and_finite_part)
{
    adf_adele_t x, y;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_set_si(x, 5);
    adf_adele_set_si(y, 5);
    ADF_CHECK(adf_adele_identical(x, y));

    adf_adele_set_si(y, 6);
    ADF_CHECK(adf_adele_identical(x, y) == 0);
    adf_adele_set_si(y, 5);
    arb_add_error_2exp_si(y->inf, -10);
    ADF_CHECK(adf_adele_identical(x, y) == 0);

    adf_adele_set_si(y, 5);
    ADF_CHECK(adf_adele_identical(x, y));
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
        ADF_CHECK(adf_adele_set_arb_fball(y, y->inf, f) == ADF_OK);
        ADF_CHECK(adf_adele_identical(x, y) == 0);
        adf_fball_clear(f);
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
    }

    adf_adele_clear(x);
    adf_adele_clear(y);
}

ADF_TEST(is_canonical_sees_a_non_finite_real_ball)
{
    adf_adele_t x;

    adf_adele_init(x);
    adf_adele_set_si(x, 1);
    ADF_CHECK(adf_adele_is_canonical(x));
    arb_pos_inf(x->inf);
    ADF_CHECK(adf_adele_is_canonical(x) == 0);
    arb_zero(x->inf);
    ADF_CHECK(adf_adele_is_canonical(x));
    adf_adele_clear(x);
}

ADF_TEST(the_finite_coordinate_does_not_depend_on_prec)
{
    adf_adele_t x, z2, z4096;
    adf_rat_t q;

    adf_adele_init(x);
    adf_adele_init(z2);
    adf_adele_init(z4096);
    adf_rat_init(q);
    adf_adele_set_si(x, 3);
    fmpq_set_si(q->q, 1, 3);
    adf_adele_add_rat(z2, x, q, 2);
    adf_adele_add_rat(z4096, x, q, 4096);
    ADF_CHECK(adf_fball_identical(&z2->fin, &z4096->fin));
    adf_adele_mul_rat(z2, x, q, 2);
    adf_adele_mul_rat(z4096, x, q, 4096);
    ADF_CHECK(adf_fball_identical(&z2->fin, &z4096->fin));
    ADF_CHECK(adf_adele_div_rat(z2, x, q, 2) == ADF_OK);
    ADF_CHECK(adf_adele_div_rat(z4096, x, q, 4096) == ADF_OK);
    ADF_CHECK(adf_fball_identical(&z2->fin, &z4096->fin));
    adf_adele_clear(x);
    adf_adele_clear(z2);
    adf_adele_clear(z4096);
    adf_rat_clear(q);
}
