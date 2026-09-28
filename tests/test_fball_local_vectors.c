/* tests/test_fball_local_vectors.c: the local backend of adf_fball against the JSON-lines vectors
   tests/ref/vectors/m1-local/local.jsonl, written by lanes/m1-local/gen_local_vectors.py from the
   Python reference tests/ref/adfref/local_ref.py (docs/proofs/policies.md section 4). Every
   record is run.

   Each record has one context (its "blocks"); "same_ctx": false puts the second operand into a
   second context with the same blocks (conventions 4.6: another pointer). A value result is
   compared field by field, backend included (a local result must be the local data of the
   reference, in the record's context; a global result the canonical triple), and its canonical
   triple (adf_fball_get_fmpz3) with "canonical". Every binary operation is also run with the
   output aliasing the first operand (conventions 4.1). */

#include <stdio.h>
#include <string.h>

#include <adelefeld.h>

#include "support/jsonl.h"
#include "test_runner.h"

static int
has_key(const jsonl_value * o, const char * key)
{
    size_t i;
    for (i = 0; i < jsonl_size(o); i++)
        if (strcmp(jsonl_key(o, i), key) == 0)
            return 1;
    return 0;
}

static int
json_fmpz(fmpz_t v, const jsonl_value * j, jsonl_error_t * err)
{
    const char * text = jsonl_int_text(j, err);
    if (text == NULL)
        return 0;
    return fmpz_set_str(v, text, 10) == 0;
}

static int
json_field_fmpz(fmpz_t v, const jsonl_value * o, const char * key, jsonl_error_t * err)
{
    const jsonl_value * j;
    return jsonl_field(o, key, &j, err) && json_fmpz(v, j, err);
}

static int
json_ulong(ulong * v, const jsonl_value * j, jsonl_error_t * err)
{
    fmpz_t t;
    int ok;
    fmpz_init(t);
    ok = json_fmpz(t, j, err) && fmpz_sgn(t) >= 0 && fmpz_abs_fits_ui(t);
    if (ok)
        *v = fmpz_get_ui(t);
    fmpz_clear(t);
    return ok;
}

static int
json_is_local(const jsonl_value * o, jsonl_error_t * err)
{
    const jsonl_value * b;
    size_t n;
    const char * s;
    if (!jsonl_field(o, "backend", &b, err))
        return -1;
    s = jsonl_string(b, &n, err);
    if (s == NULL)
        return -1;
    return strcmp(s, "local") == 0;
}

/* Build the value o: local data written into the fields (predicate L), in ctx; or a canonical
   global triple by adf_fball_set_fmpz3, which must leave it unchanged. */
static int
json_value(adf_fball_t x, const jsonl_value * o, const adf_modctx_struct * ctx, jsonl_error_t * err)
{
    int loc = json_is_local(o, err);
    int ok = 1;

    if (loc < 0)
        return 0;
    if (loc)
    {
        const jsonl_value * jr;
        slong i, k = adf_modctx_nblocks(ctx);
        adf_fball_t t;
        if (!jsonl_field(o, "res", &jr, err) || (slong) jsonl_size(jr) != k)
            return 0;
        adf_fball_init(t);
        t->res = (ulong *) flint_malloc(k * sizeof(ulong));
        for (i = 0; i < k && ok; i++)
            ok = json_ulong(&t->res[i], jsonl_at(jr, (size_t) i, err), err);
        ok = ok && json_field_fmpz(t->d, o, "d", err);
        adf_modctx_get_modulus(t->H, ctx);
        t->backend = ADF_LOCAL;
        t->mctx = ctx;
        ok = ok && adf_fball_is_canonical(t);
        adf_fball_swap(x, t);
        adf_fball_clear(t);
        return ok;
    }
    else
    {
        fmpz_t A, H, d;
        fmpz_init(A);
        fmpz_init(H);
        fmpz_init(d);
        ok = json_field_fmpz(A, o, "A", err) && json_field_fmpz(H, o, "H", err) &&
             json_field_fmpz(d, o, "d", err) && adf_fball_set_fmpz3(x, A, H, d) == ADF_OK &&
             fmpz_equal(x->A, A) && fmpz_equal(x->H, H) && fmpz_equal(x->d, d);
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
        return ok;
    }
}

/* x equals the value o exactly: backend, context (ctx for a local value), fields. */
static int
value_equals(const adf_fball_t x, const jsonl_value * o, const adf_modctx_struct * ctx,
             jsonl_error_t * err)
{
    adf_fball_t e;
    int ok;
    adf_fball_init(e);
    ok = json_value(e, o, ctx, err) && adf_fball_identical(x, e);
    adf_fball_clear(e);
    return ok;
}

/* The canonical triple of x equals the object o {A, H, d}. */
static int
canonical_equals(const adf_fball_t x, const jsonl_value * o, jsonl_error_t * err)
{
    fmpz_t A, H, d, a, h, dd;
    int ok;
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(dd);
    adf_fball_get_fmpz3(a, h, dd, x);
    ok = json_field_fmpz(A, o, "A", err) && json_field_fmpz(H, o, "H", err) &&
         json_field_fmpz(d, o, "d", err) && fmpz_equal(a, A) && fmpz_equal(h, H) &&
         fmpz_equal(dd, d);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
    return ok;
}

static int
json_rat(adf_rat_t q, const jsonl_value * o, jsonl_error_t * err)
{
    fmpz_t n, d;
    int ok;
    fmpz_init(n);
    fmpz_init(d);
    ok = json_field_fmpz(n, o, "num", err) && json_field_fmpz(d, o, "den", err) &&
         adf_rat_set_fmpz2(q, n, d) == ADF_OK;
    fmpz_clear(n);
    fmpz_clear(d);
    return ok;
}

static int
status_of(const char * s)
{
    int i;
    for (i = 0; i < ADF_STATUS_COUNT; i++)
        if (strcmp(adf_status_str(i), s) == 0)
            return i;
    return -1;
}

typedef void (*binop_t)(adf_fball_t, const adf_fball_t, const adf_fball_t);

ADF_TEST(every_record_of_local_jsonl)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;
    size_t nlocal = 0, nglobal = 0, nstatus = 0, npred = 0;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-local/local.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    ADF_CHECK(jsonl_count(f) >= 1900);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value *jop, *jblocks, *ja, *jb = NULL, *jres = NULL, *jst = NULL, *jq = NULL;
        const char * op;
        size_t oplen, j;
        ulong * q;
        slong k;
        adf_modctx_struct * c = NULL;
        adf_modctx_struct * c2 = NULL;
        const adf_modctx_struct * cb;
        adf_fball_t a, b, z, keep;
        adf_rat_t r;
        int ok = 1, same = 1;
        unsigned long line = jsonl_line_of(rec);

        ADF_CHECK(jsonl_field(rec, "op", &jop, &err) && jsonl_field(rec, "blocks", &jblocks, &err) &&
                  jsonl_field(rec, "a", &ja, &err));
        op = jsonl_string(jop, &oplen, &err);
        ADF_CHECK(op != NULL);
        if (op == NULL)
            continue;
        k = (slong) jsonl_size(jblocks);
        q = (ulong *) flint_malloc((k + 1) * sizeof(ulong));
        for (j = 0; j < (size_t) k; j++)
            ADF_CHECK(json_ulong(&q[j], jsonl_at(jblocks, j, &err), &err));
        ADF_CHECK(adf_modctx_new_blocks(&c, q, k) == ADF_OK);
        if (has_key(rec, "same_ctx"))
        {
            const jsonl_value * js;
            ADF_CHECK(jsonl_field(rec, "same_ctx", &js, &err) && jsonl_bool(js, &same, &err));
        }
        if (!same)
            ADF_CHECK(adf_modctx_new_blocks(&c2, q, k) == ADF_OK);
        cb = same ? c : c2;
        if (has_key(rec, "b"))
            ADF_CHECK(jsonl_field(rec, "b", &jb, &err));
        if (has_key(rec, "result"))
            ADF_CHECK(jsonl_field(rec, "result", &jres, &err));
        if (has_key(rec, "status"))
            ADF_CHECK(jsonl_field(rec, "status", &jst, &err));
        if (has_key(rec, "q"))
            ADF_CHECK(jsonl_field(rec, "q", &jq, &err));

        adf_fball_init(a);
        adf_fball_init(b);
        adf_fball_init(z);
        adf_fball_init(keep);
        adf_rat_init(r);
        ok = json_value(a, ja, c, &err);
        if (jb != NULL)
            ok = ok && json_value(b, jb, cb, &err);
        if (jq != NULL)
            ok = ok && json_rat(r, jq, &err);
        ADF_CHECK_MSG(ok, "line %lu: inputs: %s", line, jsonl_error_message(&err));
        {
            fmpz_t t1, t2, t3;
            fmpz_init_set_si(t1, 7);
            fmpz_init_set_si(t2, 5);
            fmpz_init_set_si(t3, 3);
            (void) adf_fball_set_fmpz3(keep, t1, t2, t3);
            adf_fball_set(z, keep);
            fmpz_clear(t1);
            fmpz_clear(t2);
            fmpz_clear(t3);
        }

        if (ok && (strcmp(op, "equal_set") == 0 || strcmp(op, "overlaps") == 0 ||
                   strcmp(op, "contains") == 0 || strcmp(op, "compare") == 0 ||
                   strcmp(op, "contains_rat") == 0))
        {
            int got = -1, want = -1;
            if (strcmp(op, "compare") == 0)
            {
                fmpz_t w;
                fmpz_init(w);
                ADF_CHECK(json_fmpz(w, jres, &err));
                want = (int) fmpz_get_si(w);
                fmpz_clear(w);
                got = adf_fball_compare(a, b);
            }
            else
            {
                ADF_CHECK(jsonl_bool(jres, &want, &err));
                if (strcmp(op, "equal_set") == 0)
                    got = adf_fball_equal_set(a, b);
                else if (strcmp(op, "overlaps") == 0)
                    got = adf_fball_overlaps(a, b);
                else if (strcmp(op, "contains") == 0)
                    got = adf_fball_contains(a, b);
                else
                    got = adf_fball_contains_rat(a, r);
            }
            ADF_CHECK_MSG(got == want, "line %lu: %s gave %d, expected %d", line, op, got, want);
            npred++;
        }
        else if (ok)
        {
            int st = ADF_OK, want_st = ADF_OK, lost = -1;
            binop_t bop = NULL;

            if (strcmp(op, "to_global") == 0)
                adf_fball_set_global(z, a);
            else if (strcmp(op, "set_local") == 0)
                st = adf_fball_set_local(z, a, c);
            else if (strcmp(op, "set_local_enclose") == 0)
                st = adf_fball_set_local_enclose(z, &lost, a, c);
            else if (strcmp(op, "neg") == 0)
                adf_fball_neg(z, a);
            else if (strcmp(op, "add") == 0)
                bop = adf_fball_add;
            else if (strcmp(op, "sub") == 0)
                bop = adf_fball_sub;
            else if (strcmp(op, "mul") == 0)
                bop = adf_fball_mul;
            else if (strcmp(op, "scale") == 0)
                adf_fball_mul_rat(z, a, r);
            else if (strcmp(op, "div_rat") == 0)
                st = adf_fball_div_rat(z, a, r);
            else
                ADF_CHECK_MSG(0, "line %lu: unknown op %s", line, op);
            if (bop != NULL)
                bop(z, a, b);

            if (jst != NULL)
            {
                size_t n;
                const char * s = jsonl_string(jst, &n, &err);
                want_st = s == NULL ? -1 : status_of(s);
                nstatus++;
            }
            ADF_CHECK_MSG(st == want_st, "line %lu: %s status %d, expected %d", line, op, st, want_st);
            if (jst != NULL)
                ADF_CHECK_MSG(adf_fball_identical(z, keep), "line %lu: output written on a status", line);
            else if (jres != NULL)
            {
                const jsonl_value * jc;
                ADF_CHECK_MSG(value_equals(z, jres, c, &err), "line %lu: %s result differs", line, op);
                ADF_CHECK(adf_fball_is_canonical(z));
                ADF_CHECK(jsonl_field(rec, "canonical", &jc, &err));
                ADF_CHECK_MSG(canonical_equals(z, jc, &err), "line %lu: canonical triple differs", line);
                if (adf_fball_is_local(z))
                    nlocal++;
                else
                    nglobal++;
                if (has_key(rec, "lost"))
                {
                    const jsonl_value * jl;
                    int wl = -1;
                    ADF_CHECK(jsonl_field(rec, "lost", &jl, &err) && jsonl_bool(jl, &wl, &err));
                    ADF_CHECK_MSG(lost == wl, "line %lu: lost %d, expected %d", line, lost, wl);
                }
                /* aliasing: the output is the first operand */
                if (bop != NULL || strcmp(op, "neg") == 0 || strcmp(op, "scale") == 0 ||
                    strcmp(op, "div_rat") == 0 || strcmp(op, "to_global") == 0 ||
                    strcmp(op, "set_local") == 0 || strcmp(op, "set_local_enclose") == 0)
                {
                    adf_fball_t w;
                    adf_fball_init(w);
                    adf_fball_set(w, a);
                    if (bop != NULL)
                        bop(w, w, b);
                    else if (strcmp(op, "neg") == 0)
                        adf_fball_neg(w, w);
                    else if (strcmp(op, "scale") == 0)
                        adf_fball_mul_rat(w, w, r);
                    else if (strcmp(op, "div_rat") == 0)
                        ADF_CHECK(adf_fball_div_rat(w, w, r) == ADF_OK);
                    else if (strcmp(op, "to_global") == 0)
                        adf_fball_set_global(w, w);
                    else if (strcmp(op, "set_local") == 0)
                        ADF_CHECK(adf_fball_set_local(w, w, c) == ADF_OK);
                    else
                        ADF_CHECK(adf_fball_set_local_enclose(w, NULL, w, c) == ADF_OK);
                    ADF_CHECK_MSG(adf_fball_identical(w, z), "line %lu: %s aliased differs", line, op);
                    adf_fball_clear(w);
                }
            }
            else
                ADF_CHECK_MSG(0, "line %lu: neither result nor status", line);
        }

        adf_fball_clear(a);
        adf_fball_clear(b);
        adf_fball_clear(z);
        adf_fball_clear(keep);
        adf_rat_clear(r);
        adf_modctx_free(c);
        adf_modctx_free(c2);
        flint_free(q);
    }
    jsonl_close(f);
    /* the file reaches every kind of expectation */
    ADF_CHECK(nlocal >= 900 && nglobal >= 300 && nstatus >= 150 && npred >= 150);
    printf("local.jsonl: %lu local results, %lu global results, %lu statuses, %lu predicates\n",
           (unsigned long) nlocal, (unsigned long) nglobal, (unsigned long) nstatus,
           (unsigned long) npred);
}
