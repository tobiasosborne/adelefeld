/* tests/test_text_fball.c: the value form of adf_fball (adf_fball_set_str, adf_fball_get_str).

   Contract: include/adelefeld/text.h; docs/conventions.md 8 (8.1 interface, 8.2 alphabet, 8.4
   limits, 8.5 order of checks), 9.2 (fball_v = "(" "*" ";" fin ")" | rat "mod" urat), 9.3 (zero
   denominator; centre reduced into [0, N); "mod 0" dropped; bare form), 9.4 (template (* ; F)),
   9.6 (round trips), 9.7 (no coercion), 11.3. Oracles: tests/golden/fball.tsv (every row) and
   tests/ref/vectors/m1-text/text_fball.jsonl (random texts through proto/text_grammar.py, written
   by lanes/m1-text/gen_text_vectors.py).

   Strings returned by adf_fball_get_str are freed with flint_free: adf_str_free is written by
   another lane (brief of lane m1-text); it is documented to call flint_free (common.h). */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "support/jsonl.h"
#include "test_runner.h"

/* ------------------------------------------------------------------ helpers */

static int
status_of_name(const char * name)
{
    int k;

    for (k = 0; k < ADF_STATUS_COUNT; k++)
        if (strcmp(adf_status_str(k), name) == 0)
            return k;
    return -1;
}

/* The sentinel 5 + 18 Zhat, which no failing parse may change. */
static void
set_sentinel(adf_fball_t x)
{
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 18);
    fmpz_set_si(x->d, 1);
}

static int
is_sentinel(const adf_fball_t x)
{
    return fmpz_cmp_si(x->A, 5) == 0 && fmpz_cmp_si(x->H, 18) == 0 && fmpz_is_one(x->d)
           && x->backend == ADF_GLOBAL && x->mctx == NULL && x->res == NULL;
}

static void
check_one(const char * s, size_t n, int want_status, const char * want_text, const char * where)
{
    adf_fball_t x;
    int st;

    adf_fball_init(x);
    set_sentinel(x);
    st = adf_fball_set_str(x, s, n, NULL);
    ADF_CHECK_MSG(st == want_status, "%s: status %d, expected %d", where, st, want_status);
    if (st != ADF_OK)
    {
        ADF_CHECK_MSG(is_sentinel(x), "%s: the output changed on status %d", where, st);
    }
    else
    {
        ADF_CHECK_MSG(adf_fball_is_canonical(x) && x->backend == ADF_GLOBAL, "%s: not canonical global",
                      where);
        if (want_text != NULL)
        {
            size_t len = 12345;
            char * out = adf_fball_get_str(&len, x);

            ADF_CHECK(out != NULL);
            ADF_CHECK_MSG(len == strlen(out) && strcmp(out, want_text) == 0,
                          "%s: printed \"%s\" (len %zu), expected \"%s\"", where, out, len, want_text);
            flint_free(out);
        }
    }
    adf_fball_clear(x);
}

static unsigned char *
hex_decode(const char * hex, size_t hlen, size_t * n)
{
    unsigned char * b = (unsigned char *) malloc(hlen / 2 + 1);
    size_t i;

    for (i = 0; i < hlen / 2; i++)
    {
        unsigned v = 0;
        int j;
        for (j = 0; j < 2; j++)
        {
            char c = hex[2 * i + j];
            v = 16 * v + (unsigned) (c <= '9' ? c - '0' : c - 'a' + 10);
        }
        b[i] = (unsigned char) v;
    }
    *n = hlen / 2;
    return b;
}

/* The text conventions 9.4 prescribes for the canonical triple (A, H, d). */
static char *
expected_text(const adf_fball_t x)
{
    fmpq_t a, N;
    char * as, * Ns, * out;

    fmpq_init(a);
    fmpq_init(N);
    fmpq_set_fmpz_frac(a, x->A, x->d);
    fmpq_set_fmpz_frac(N, x->H, x->d);
    as = fmpq_get_str(NULL, 10, a);
    Ns = fmpq_get_str(NULL, 10, N);
    out = (char *) flint_malloc(strlen(as) + strlen(Ns) + 16);
    if (fmpz_is_zero(x->H))
        sprintf(out, "(* ; %s)", as);
    else
        sprintf(out, "(* ; %s mod %s)", as, Ns);
    flint_free(as);
    flint_free(Ns);
    fmpq_clear(a);
    fmpq_clear(N);
    return out;
}

static void
round_trip(const adf_fball_t x, const char * where)
{
    size_t len, len2;
    char * t = adf_fball_get_str(&len, x);
    char * t2, * want = expected_text(x);
    adf_fball_t y;

    ADF_CHECK_MSG(len == strlen(want) && strcmp(t, want) == 0, "%s: printed \"%s\", expected \"%s\"", where,
                  t, want);
    adf_fball_init(y);
    ADF_CHECK_MSG(adf_fball_set_str(y, t, len, NULL) == ADF_OK, "%s: \"%s\" does not parse", where, t);
    ADF_CHECK_MSG(adf_fball_identical(x, y), "%s: \"%s\" parsed to another value", where, t);
    t2 = adf_fball_get_str(&len2, y);
    ADF_CHECK_MSG(len2 == len && strcmp(t, t2) == 0, "%s: \"%s\" printed again as \"%s\"", where, t, t2);
    /* the bare form "a mod N" (conventions 9.2, CV-32) reads the same set */
    if (!fmpz_is_zero(x->H))
    {
        adf_fball_t z;

        adf_fball_init(z);
        ADF_CHECK(adf_fball_set_str(z, t + 5, len - 6, NULL) == ADF_OK);
        ADF_CHECK_MSG(adf_fball_identical(x, z), "%s: the bare form of \"%s\" differs", where, t);
        adf_fball_clear(z);
    }
    flint_free(t);
    flint_free(t2);
    flint_free(want);
    adf_fball_clear(y);
}

static void
random_ball(adf_fball_t x, flint_rand_t st, ulong bits)
{
    fmpz_t A, H, d;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_randtest(A, st, bits);
    if (n_randint(st, 4) == 0)
        fmpz_zero(H);
    else
        fmpz_randtest_unsigned(H, st, bits);
    fmpz_randtest_not_zero(d, st, 1 + bits / 2);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

/* ------------------------------------------------------------------ golden and reference */

ADF_TEST(every_row_of_the_golden_file_fball)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/fball.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        char where[64];

        snprintf(where, sizeof(where), "fball.tsv line %lu", r->line);
        if (r->is_status)
            check_one(r->input, r->input_len, status_of_name(r->status), NULL, where);
        else
            check_one(r->input, r->input_len, ADF_OK, r->expected, where);
        rows++;
    }
    /* tests/golden/README.md: 68 vectors in fball.tsv */
    ADF_CHECK_MSG(rows == 68, "fball.tsv has %zu rows", rows);
    golden_close(f);
}

ADF_TEST(every_random_text_of_the_reference)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-text/text_fball.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    ADF_CHECK(jsonl_count(f) == 1500);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * h, * e;
        const char * hex, * exp;
        size_t hlen, elen, n;
        unsigned char * b;
        char where[64];

        ADF_CHECK(jsonl_field(rec, "hex", &h, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "expected", &e, &err) == 1);
        hex = jsonl_string(h, &hlen, &err);
        exp = jsonl_string(e, &elen, &err);
        b = hex_decode(hex, hlen, &n);
        snprintf(where, sizeof(where), "text_fball.jsonl record %zu", i + 1);
        if (exp[0] == '!')
            check_one((const char *) b, n, status_of_name(exp + 1), NULL, where);
        else
            check_one((const char *) b, n, ADF_OK, exp, where);
        free(b);
    }
    jsonl_close(f);
}

/* ------------------------------------------------------------------ printer and round trips */

ADF_TEST(the_printer_writes_the_template)
{
    adf_fball_t x;
    size_t len;
    char * s;

    adf_fball_init(x);
    s = adf_fball_get_str(&len, x);
    ADF_CHECK(len == 7 && strcmp(s, "(* ; 0)") == 0 && s[len] == '\0');
    flint_free(s);
    /* (A, H, d) = (5, 18, 3): 5/3 + 6 Zhat */
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 18);
    fmpz_set_si(x->d, 3);
    s = adf_fball_get_str(&len, x);
    ADF_CHECK(strcmp(s, "(* ; 5/3 mod 6)") == 0 && len == 15);
    flint_free(s);
    /* (1, 2, 12): 1/12 + 1/6 Zhat */
    fmpz_set_si(x->A, 1);
    fmpz_set_si(x->H, 2);
    fmpz_set_si(x->d, 12);
    s = adf_fball_get_str(&len, x);
    ADF_CHECK(strcmp(s, "(* ; 1/12 mod 1/6)") == 0);
    flint_free(s);
    /* (-7, 0, 3): the point -7/3 */
    fmpz_set_si(x->A, -7);
    fmpz_set_si(x->H, 0);
    fmpz_set_si(x->d, 3);
    s = adf_fball_get_str(&len, x);
    ADF_CHECK(strcmp(s, "(* ; -7/3)") == 0);
    flint_free(s);
    /* (0, 1, 1): Zhat */
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->H, 1);
    fmpz_set_si(x->d, 1);
    s = adf_fball_get_str(&len, x);
    ADF_CHECK(strcmp(s, "(* ; 0 mod 1)") == 0);
    flint_free(s);
    adf_fball_clear(x);
}

ADF_TEST(random_balls_round_trip)
{
    flint_rand_t st;
    adf_fball_t x;
    int k;

    flint_randinit(st);
    adf_fball_init(x);
    for (k = 0; k < 2000; k++)
    {
        random_ball(x, st, 1 + n_randint(st, 200));
        round_trip(x, "random ball");
    }
    for (k = 0; k < 20; k++)
    {
        random_ball(x, st, 2000 + 200 * (ulong) k);
        round_trip(x, "huge ball");
    }
    adf_fball_clear(x);
    flint_randclear(st);
}

ADF_TEST(the_parser_canonicalises)
{
    check_one("(* ; -1/2 mod 1/3)", 18, ADF_OK, "(* ; 1/6 mod 1/3)", "negative centre");
    check_one("(* ; 4/6 mod 0/7)", 17, ADF_OK, "(* ; 2/3)", "mod 0 over 7");
    check_one("(* ; -0 mod 00)", 15, ADF_OK, "(* ; 0)", "-0 mod 00");
    check_one("-0 mod 1", 8, ADF_OK, "(* ; 0 mod 1)", "bare -0 mod 1");
    check_one("7 mod 0", 7, ADF_OK, "(* ; 7)", "bare mod 0");
    check_one("12 mod 8/2", 10, ADF_OK, "(* ; 0 mod 4)", "bare 12 mod 4");
    check_one("(* ; 1/0 mod 6)", 15, ADF_DOMAIN, NULL, "zero denominator of the centre");
    check_one("(* ; 1 mod 6/0)", 15, ADF_DOMAIN, NULL, "zero denominator of the radius");
    check_one("1/0 mod 6", 9, ADF_DOMAIN, NULL, "bare zero denominator");
    check_one("1 mod 6/00", 10, ADF_DOMAIN, NULL, "bare zero denominator of the radius");
    check_one("7/3", 3, ADF_PARSE, NULL, "a rational is not a finite ball (9.7)");
    check_one("(* ; 1 mod 6 )x", 15, ADF_PARSE, NULL, "trailing letter");
    check_one("( * ; 1 mod 6", 13, ADF_PARSE, NULL, "open");
    check_one("(*;1mod6)", 9, ADF_OK, "(* ; 1 mod 6)", "no whitespace");
    check_one("(* ; 1 mod6/)", 13, ADF_PARSE, NULL, "slash without digits");
    check_one("(* ; 1 mod 1e5)", 15, ADF_PARSE, NULL, "a decimal radius");
    check_one("(* ; 1 mod -1)", 14, ADF_PARSE, NULL, "a negative radius");
    check_one("(* ; 1 mod)", 11, ADF_PARSE, NULL, "mod without radius");
    check_one("(* ; 1 mods 6)", 14, ADF_PARSE, NULL, "keyword run mods");
    check_one("(* * 1)", 7, ADF_PARSE, NULL, "star for semicolon");
    check_one("(+ ; 1)", 7, ADF_PARSE, NULL, "plus for star");
    check_one("(* ; 1 mod 2 mod 3)", 19, ADF_PARSE, NULL, "mod twice");
    check_one("1", 1, ADF_PARSE, NULL, "bare without mod");
    check_one("1 mod 2 x", 9, ADF_PARSE, NULL, "bare with a trailing letter");
}

ADF_TEST(the_output_may_hold_any_value_before)
{
    adf_fball_t x;
    size_t len;
    char * s;

    adf_fball_init(x);
    fmpz_set_si(x->A, 1);
    fmpz_set_si(x->H, 2);
    fmpz_set_si(x->d, 12);
    ADF_CHECK(adf_fball_set_str(x, "(* ; 7/3)", 9, NULL) == ADF_OK);
    ADF_CHECK(fmpz_cmp_si(x->A, 7) == 0 && fmpz_is_zero(x->H) && fmpz_cmp_si(x->d, 3) == 0);
    s = adf_fball_get_str(&len, x);
    ADF_CHECK(strcmp(s, "(* ; 7/3)") == 0);
    flint_free(s);
    adf_fball_clear(x);
}

/* ------------------------------------------------------------------ limits and hostile input */

ADF_TEST(max_len_at_the_limit_and_one_above)
{
    adf_text_limits_t lim;
    adf_fball_t x;

    adf_text_limits_default(&lim);
    lim.max_len = 7;
    adf_fball_init(x);
    set_sentinel(x);
    ADF_CHECK(adf_fball_set_str(x, "2 mod 6", 7, &lim) == ADF_OK);
    ADF_CHECK(fmpz_cmp_si(x->A, 2) == 0 && fmpz_cmp_si(x->H, 6) == 0);
    set_sentinel(x);
    ADF_CHECK(adf_fball_set_str(x, "2 mod 6 ", 8, &lim) == ADF_LIMIT);
    ADF_CHECK(is_sentinel(x));
    adf_text_limits_default(&lim);
    lim.max_exp10 = -1;
    lim.max_prec = 0;
    lim.max_items = 0;
    ADF_CHECK(adf_fball_set_str(x, "(* ; 1000 mod 10000)", 20, &lim) == ADF_OK);
    adf_fball_clear(x);
}

ADF_TEST(empty_null_nul_and_forbidden_bytes)
{
    const char * t = "(* ; 1 mod 6)";
    size_t n = strlen(t), pos;
    int b;

    check_one(NULL, 0, ADF_PARSE, NULL, "NULL");
    check_one("(* ; 1)", 0, ADF_PARSE, NULL, "len 0");
    for (pos = 0; pos <= n; pos++)
    {
        char buf[32];
        memcpy(buf, t, pos);
        buf[pos] = '\0';
        memcpy(buf + pos + 1, t + pos, n - pos);
        check_one(buf, n + 1, ADF_PARSE, NULL, "NUL inside");
    }
    for (b = 0; b < 256; b++)
    {
        char buf[32];
        int allowed = (b >= 0x20 && b <= 0x7e) || b == 9 || b == 10 || b == 13;

        if (allowed)
            continue;
        memcpy(buf, t, n);
        buf[5] = (char) b;           /* in place of the space after ';' */
        check_one(buf, n, ADF_PARSE, NULL, "forbidden byte");
    }
}

ADF_TEST(input_without_terminator_at_a_block_end_and_a_page_end)
{
    const char * texts[] = {"(* ; 1 mod 6)", "(* ; 1 mod 6", "2 mod 6", "2 mod", "2 mod 6/",
                            "1/0 mod 6", "(* ; 7/3)"};
    const int want[] = {ADF_OK, ADF_PARSE, ADF_OK, ADF_PARSE, ADF_PARSE, ADF_DOMAIN, ADF_OK};
    long page = sysconf(_SC_PAGESIZE);
    char * m;
    size_t k;

    m = (char *) mmap(NULL, 2 * (size_t) page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ADF_CHECK(m != MAP_FAILED);
    if (m == MAP_FAILED)
        return;
    ADF_CHECK(mprotect(m + page, (size_t) page, PROT_NONE) == 0);
    for (k = 0; k < sizeof(texts) / sizeof(texts[0]); k++)
    {
        size_t n = strlen(texts[k]);
        char * b = (char *) malloc(n);
        memcpy(b, texts[k], n);
        /* "2 mod 6/" is "2 mod 6" followed by "/": a parse error */
        check_one(b, n, want[k], NULL, texts[k]);
        free(b);
        memcpy(m + page - n, texts[k], n);
        check_one(m + page - n, n, want[k], NULL, texts[k]);
    }
    munmap(m, 2 * (size_t) page);
}

ADF_TEST(the_earlier_stage_decides_the_status)
{
    adf_text_limits_t lim;
    adf_fball_t x;

    adf_text_limits_default(&lim);
    lim.max_len = 8;
    adf_fball_init(x);
    set_sentinel(x);
    /* stage 1 against 2, 3 and 6 */
    ADF_CHECK(adf_fball_set_str(x, "(* ; \x80 1)", 9, &lim) == ADF_LIMIT);
    ADF_CHECK(adf_fball_set_str(x, "(* ; 1 mo", 9, &lim) == ADF_LIMIT);
    ADF_CHECK(adf_fball_set_str(x, "(* ; 1/0)", 9, &lim) == ADF_LIMIT);
    /* stage 2 against 3 and 6 */
    ADF_CHECK(adf_fball_set_str(x, "(* ; 1/0)\x80", 10, NULL) == ADF_PARSE);
    ADF_CHECK(adf_fball_set_str(x, "(* ; 1 mod\x7f", 11, NULL) == ADF_PARSE);
    /* stage 3 against 6 */
    ADF_CHECK(adf_fball_set_str(x, "(* ; 1/0 mod 6", 14, NULL) == ADF_PARSE);
    ADF_CHECK(adf_fball_set_str(x, "1/0 mod 6/0 x", 13, NULL) == ADF_PARSE);
    ADF_CHECK(is_sentinel(x));
    adf_fball_clear(x);
}
