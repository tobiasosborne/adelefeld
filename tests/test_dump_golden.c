/* tests/test_dump_golden.c: every row of tests/golden/dump.tsv (docs/conventions.md 11.3 item 5:
   "load with one binding per context occurrence (10.2), then dump; the text must be identical;
   loading an invalid dump must leave the output untouched and return the status").

   Each row is given to adf_modctx_new_from_dump at every occurrence index (every body). A row
   whose body is one of the five types of dump.h is also given to that type's inspector and
   loader: a valid row is inspected, one context per occurrence is made with
   adf_modctx_new_from_dump and checked against the descriptor, the row is loaded with those
   bindings, dumped, and must come back byte for byte; an invalid row must give its status with
   the output untouched, from the loader and from the inspector. */

#include <string.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "test_runner.h"

/* The body keyword of a text: its third space-separated token. */
static int
body_is(const char * t, size_t len, const char * kw)
{
    size_t i = 0, sp = 0, n = strlen(kw);

    while (i < len && sp < 2)
        if (t[i++] == ' ')
            sp++;
    return sp == 2 && i + n <= len && memcmp(t + i, kw, n) == 0 && (i + n == len || t[i + n] == ' ');
}

/* The number of tokens "l" (a local finite ball: the only context occurrences of the bodies
   fball, adele, cadele, qclass), plus 1 for scaled and modctx. Used for the rows whose body has
   no inspector here (qclass, modctx). */
static size_t
count_occurrences(const char * t, size_t len)
{
    size_t i, n = 0;

    if (body_is(t, len, "scaled") || body_is(t, len, "modctx"))
        return 1;
    for (i = 0; i + 1 <= len; i++)
        if (t[i] == 'l' && (i == 0 || t[i - 1] == ' ') && (i + 1 == len || t[i + 1] == ' '))
            n++;
    return n;
}

static int
status_of_name(const char * name)
{
    int s;
    for (s = 0; s < ADF_STATUS_COUNT; s++)
        if (strcmp(adf_status_str(s), name) == 0)
            return s;
    return -1;
}

/* A typed row: inspect, load with bindings, dump; or the status with the output untouched. */
static void
typed_row(const golden_record * r, int want, size_t * nvalid)
{
    const char * t = r->input;
    size_t len = r->input_len, n, i, dl = 0;
    adf_ctx_desc_t d[4];
    adf_modctx_struct * ctxs[4] = {NULL, NULL, NULL, NULL};
    const adf_modctx_struct * binds[4] = {NULL, NULL, NULL, NULL};
    char * again = NULL;
    int ist, lst;

    for (i = 0; i < 4; i++)
        adf_ctx_desc_init(&d[i]);
    n = 4;
    if (body_is(t, len, "rat"))
        ist = adf_rat_dump_inspect(&n, d, t, len, NULL);
    else if (body_is(t, len, "fball"))
        ist = adf_fball_dump_inspect(&n, d, t, len, NULL);
    else if (body_is(t, len, "scaled"))
        ist = adf_scaled_dump_inspect(&n, d, t, len, NULL);
    else if (body_is(t, len, "adele"))
        ist = adf_adele_dump_inspect(&n, d, t, len, NULL);
    else
        ist = adf_cadele_dump_inspect(&n, d, t, len, NULL);
    ADF_CHECK_MSG(ist == want, "line %lu \"%s\": inspect %s, expected %s", r->line, t, adf_status_str(ist),
                  adf_status_str(want));
    if (ist == ADF_OK)
    {
        ADF_CHECK(n <= 1);
        for (i = 0; i < n && i < 4; i++)
        {
            ADF_CHECK(adf_modctx_new_from_dump(&ctxs[i], t, len, i, NULL) == ADF_OK);
            ADF_CHECK(ctxs[i] != NULL && adf_modctx_matches_desc(ctxs[i], &d[i]));
            binds[i] = ctxs[i];
        }
    }
    else
    {
        ADF_CHECK(n == 4 && fmpz_is_one(d[0].K) && d[0].q == NULL);
        n = 0;
    }
    if (body_is(t, len, "rat"))
    {
        adf_rat_t x;
        adf_rat_struct copy;
        memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
        adf_rat_init(x);
        fmpq_set_si(x->q, 9, 4);
        memcpy(&copy, x, sizeof(copy));
        lst = adf_rat_load_str_binds(x, t, len, binds, n, NULL);
        if (lst == ADF_OK)
            again = adf_rat_dump_str(&dl, x);
        else
            ADF_CHECK(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(fmpq_numref(x->q), 9));
        adf_rat_clear(x);
    }
    else if (body_is(t, len, "fball"))
    {
        adf_fball_t x;
        adf_fball_struct copy;
        memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
        adf_fball_init(x);
        fmpz_set_si(x->A, 3);
        fmpz_set_si(x->H, 7);
        memcpy(&copy, x, sizeof(copy));
        lst = adf_fball_load_str_binds(x, t, len, binds, n, NULL);
        if (lst == ADF_OK)
            again = adf_fball_dump_str(&dl, x);
        else
            ADF_CHECK(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(x->A, 3) && fmpz_equal_si(x->H, 7));
        adf_fball_clear(x);
    }
    else if (body_is(t, len, "scaled"))
    {
        adf_scaled_t x;
        adf_scaled_struct copy;
        memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
        fmpq_init(x->s);
        fmpz_init(x->u);
        x->mctx = (const adf_modctx_struct *) (void *) 0x40;
        x->exact = 1;
        fmpq_set_si(x->s, -5, 2);
        memcpy(&copy, x, sizeof(copy));
        lst = adf_scaled_load_str_binds(x, t, len, binds, n, NULL);
        if (lst == ADF_OK)
            again = adf_scaled_dump_str(&dl, x);
        else
            ADF_CHECK(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(fmpq_numref(x->s), -5));
        fmpq_clear(x->s);
        fmpz_clear(x->u);
    }
    else if (body_is(t, len, "adele"))
    {
        adf_adele_t x;
        adf_adele_struct copy;
        memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
        adf_adele_init(x);
        arb_set_si(x->inf, 11);
        memcpy(&copy, x, sizeof(copy));
        lst = adf_adele_load_str_binds(x, t, len, binds, n, NULL);
        if (lst == ADF_OK)
            again = adf_adele_dump_str(&dl, x);
        else
            ADF_CHECK(memcmp(&copy, x, sizeof(copy)) == 0 && arf_equal_si(arb_midref(x->inf), 11));
        adf_adele_clear(x);
    }
    else
    {
        adf_cadele_t x;
        adf_cadele_struct copy;
        memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
        adf_cadele_init(x);
        acb_set_si(x->inf, 13);
        memcpy(&copy, x, sizeof(copy));
        lst = adf_cadele_load_str_binds(x, t, len, binds, n, NULL);
        if (lst == ADF_OK)
            again = adf_cadele_dump_str(&dl, x);
        else
            ADF_CHECK(memcmp(&copy, x, sizeof(copy)) == 0 && arf_equal_si(arb_midref(acb_realref(x->inf)), 13));
        adf_cadele_clear(x);
    }
    ADF_CHECK_MSG(lst == want, "line %lu \"%s\": load %s, expected %s", r->line, t, adf_status_str(lst),
                  adf_status_str(want));
    if (lst == ADF_OK)
    {
        ADF_CHECK_MSG(again != NULL && dl == r->expected_len && memcmp(again, r->expected, dl) == 0,
                      "line %lu: dumped \"%s\", expected \"%s\"", r->line, again, r->expected);
        (*nvalid)++;
    }
    adf_str_free(again);
    for (i = 0; i < 4; i++)
    {
        adf_ctx_desc_clear(&d[i]);
        adf_modctx_free(ctxs[i]);
    }
}

ADF_TEST(golden_dump_every_row)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, nrows = 0, ntyped = 0, nvalid = 0, nstatus = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/dump.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        const char * t = r->input;
        size_t len = r->input_len, occ, nocc;
        int want = r->is_status ? status_of_name(r->status) : ADF_OK;

        ADF_CHECK(want >= 0);
        nrows++;
        if (r->is_status)
            nstatus++;
        else
            ADF_CHECK(len == r->expected_len && memcmp(t, r->expected, len) == 0);   /* 10.2: its own text */
        /* adf_modctx_new_from_dump, every body */
        if (r->is_status)
        {
            adf_modctx_struct * sentinel = (adf_modctx_struct *) (void *) 0x50, * c = sentinel;
            int st = adf_modctx_new_from_dump(&c, t, len, 0, NULL);
            ADF_CHECK_MSG(st == want && c == sentinel, "line %lu \"%s\": from_dump %s, expected %s", r->line, t,
                          adf_status_str(st), r->status);
        }
        else
        {
            nocc = count_occurrences(t, len);
            for (occ = 0; occ <= nocc; occ++)
            {
                adf_modctx_struct * c = NULL;
                int st = adf_modctx_new_from_dump(&c, t, len, occ, NULL);
                ADF_CHECK_MSG(st == (occ < nocc ? ADF_OK : ADF_DOMAIN), "line %lu \"%s\" occurrence %zu: %s",
                              r->line, t, occ, adf_status_str(st));
                if (st == ADF_OK && body_is(t, len, "modctx"))
                {
                    size_t dl;
                    char * d = adf_modctx_dump_str(&dl, c);
                    ADF_CHECK(dl == len && memcmp(d, t, len) == 0);
                    adf_str_free(d);
                }
                adf_modctx_free(c);
            }
        }
        if (body_is(t, len, "rat") || body_is(t, len, "fball") || body_is(t, len, "scaled")
            || body_is(t, len, "adele") || body_is(t, len, "cadele"))
        {
            typed_row(r, want, &nvalid);
            ntyped++;
        }
    }
    /* tests/golden/README.md: 175 vectors in dump.tsv (165 before the rows of lane m1-repair-dump,
       which add the whitespace of the body, the exponent bound of M1-D9 and the block cap of
       M1-D5; tests/golden/README.md still says 165 and is not this lane's to change). */
    ADF_CHECK_MSG(nrows == 175, "%zu rows", nrows);
    /* 77 rows with a body of dump.h (the @gen row counted, the row with a leading space not: its
       third token is "Q"), 23 of them valid; counted in Python from the file */
    ADF_CHECK_MSG(ntyped == 77 && nvalid == 23 && nstatus == 115, "%zu typed rows, %zu valid, %zu status", ntyped,
                  nvalid, nstatus);
    golden_close(f);
}
