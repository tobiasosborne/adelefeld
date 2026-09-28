/* tests/test_text_rat.c: the value form of adf_rat (adf_rat_set_str, adf_rat_get_str).

   Contract: include/adelefeld/text.h; docs/conventions.md 8 (interface 8.1, alphabet 8.2, limits
   8.4, order of checks 8.5), 9.1 (rat), 9.3 (zero denominator), 9.4 (template q(x)), 9.6 (round
   trips), 11.3 (use of the golden files). Oracles: tests/golden/rat.tsv (every row) and
   tests/ref/vectors/m1-text/text_rat.jsonl (random texts through proto/text_grammar.py, written by
   lanes/m1-text/gen_text_vectors.py).

   Strings returned by adf_rat_get_str are freed with flint_free here: adf_str_free is written by
   another lane (brief of lane m1-text), and it is documented to call flint_free (common.h). */

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

/* A sentinel value that no failing parse may change: 12345/7. */
static void
set_sentinel(adf_rat_t x)
{
    fmpz_set_si(fmpq_numref(x->q), 12345);
    fmpz_set_si(fmpq_denref(x->q), 7);
}

static int
is_sentinel(const adf_rat_t x)
{
    return fmpz_cmp_si(fmpq_numref(x->q), 12345) == 0 && fmpz_cmp_si(fmpq_denref(x->q), 7) == 0;
}

/* Parse (s, n) and compare with an expected canonical text or status (NULL text: status). */
static void
check_one(const char * s, size_t n, int want_status, const char * want_text, const char * where)
{
    adf_rat_t x;
    int st;

    adf_rat_init(x);
    set_sentinel(x);
    st = adf_rat_set_str(x, s, n, NULL);
    ADF_CHECK_MSG(st == want_status, "%s: status %d, expected %d", where, st, want_status);
    if (st != ADF_OK)
    {
        ADF_CHECK_MSG(is_sentinel(x), "%s: the output changed on status %d", where, st);
    }
    else if (want_text != NULL)
    {
        size_t len = 12345;
        char * out;

        ADF_CHECK_MSG(adf_rat_is_canonical(x), "%s: not canonical", where);
        out = adf_rat_get_str(&len, x);
        ADF_CHECK(out != NULL);
        ADF_CHECK_MSG(len == strlen(out) && strcmp(out, want_text) == 0,
                      "%s: printed \"%s\" (len %zu), expected \"%s\"", where, out, len, want_text);
        flint_free(out);
    }
    adf_rat_clear(x);
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

/* Print x, parse the text back, and require the identical value and the same text. */
static void
round_trip(const adf_rat_t x, const char * where)
{
    size_t len, len2;
    char * t = adf_rat_get_str(&len, x);
    char * t2;
    adf_rat_t y;

    adf_rat_init(y);
    ADF_CHECK_MSG(adf_rat_set_str(y, t, len, NULL) == ADF_OK, "%s: \"%s\" does not parse", where, t);
    ADF_CHECK_MSG(adf_rat_identical(x, y), "%s: \"%s\" parsed to another value", where, t);
    t2 = adf_rat_get_str(&len2, y);
    ADF_CHECK_MSG(len2 == len && strcmp(t, t2) == 0, "%s: \"%s\" printed again as \"%s\"", where, t, t2);
    flint_free(t);
    flint_free(t2);
    adf_rat_clear(y);
}

/* ------------------------------------------------------------------ golden */

ADF_TEST(every_row_of_the_golden_file_rat)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/rat.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        char where[64];

        snprintf(where, sizeof(where), "rat.tsv line %lu", r->line);
        if (r->is_status)
            check_one(r->input, r->input_len, status_of_name(r->status), NULL, where);
        else
            check_one(r->input, r->input_len, ADF_OK, r->expected, where);
        rows++;
    }
    /* tests/golden/README.md: 51 vectors in rat.tsv */
    ADF_CHECK_MSG(rows == 51, "rat.tsv has %zu rows", rows);
    golden_close(f);
}

ADF_TEST(every_random_text_of_the_reference)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-text/text_rat.jsonl", &f, &err) == 1, "%s",
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
        snprintf(where, sizeof(where), "text_rat.jsonl record %zu", i + 1);
        if (exp[0] == '!')
            check_one((const char *) b, n, status_of_name(exp + 1), NULL, where);
        else
            check_one((const char *) b, n, ADF_OK, exp, where);
        free(b);
    }
    jsonl_close(f);
}

/* ------------------------------------------------------------------ printer */

ADF_TEST(the_printer_writes_the_template_q)
{
    adf_rat_t x;
    size_t len;
    char * s;

    adf_rat_init(x);
    s = adf_rat_get_str(&len, x);
    ADF_CHECK(len == 1 && strcmp(s, "0") == 0 && s[len] == '\0');
    flint_free(s);

    adf_rat_set_si(x, -7);
    s = adf_rat_get_str(&len, x);
    ADF_CHECK(len == 2 && strcmp(s, "-7") == 0);
    flint_free(s);

    fmpz_set_si(fmpq_numref(x->q), -7);
    fmpz_set_si(fmpq_denref(x->q), 3);
    s = adf_rat_get_str(&len, x);
    ADF_CHECK(len == 4 && strcmp(s, "-7/3") == 0);
    flint_free(s);

    fmpz_set_si(fmpq_numref(x->q), 1);
    fmpz_set_si(fmpq_denref(x->q), 2);
    s = adf_rat_get_str(&len, x);
    ADF_CHECK(len == 3 && strcmp(s, "1/2") == 0);
    flint_free(s);
    adf_rat_clear(x);
}

ADF_TEST(huge_operands_print_their_digits_and_come_back)
{
    flint_rand_t st;
    adf_rat_t x;
    int k;

    flint_randinit(st);
    adf_rat_init(x);
    for (k = 0; k < 40; k++)
    {
        char * s, * ns, * ds, * want;
        size_t len;

        fmpz_randtest(fmpq_numref(x->q), st, 3000 + 100 * k);
        fmpz_randtest_not_zero(fmpq_denref(x->q), st, 2000 + 50 * k);
        fmpz_abs(fmpq_denref(x->q), fmpq_denref(x->q));
        fmpq_canonicalise(x->q);
        s = adf_rat_get_str(&len, x);
        ns = fmpz_get_str(NULL, 10, fmpq_numref(x->q));
        ds = fmpz_get_str(NULL, 10, fmpq_denref(x->q));
        want = (char *) flint_malloc(strlen(ns) + strlen(ds) + 2);
        if (fmpz_is_one(fmpq_denref(x->q)))
            strcpy(want, ns);
        else
        {
            strcpy(want, ns);
            strcat(want, "/");
            strcat(want, ds);
        }
        ADF_CHECK(len == strlen(want) && strcmp(s, want) == 0);
        flint_free(s);
        flint_free(ns);
        flint_free(ds);
        flint_free(want);
        round_trip(x, "huge operand");
    }
    adf_rat_clear(x);
    flint_randclear(st);
}

ADF_TEST(random_values_round_trip)
{
    flint_rand_t st;
    adf_rat_t x;
    int k;

    flint_randinit(st);
    adf_rat_init(x);
    for (k = 0; k < 3000; k++)
    {
        fmpq_randtest(x->q, st, 1 + n_randint(st, 300));
        round_trip(x, "random value");
    }
    adf_rat_clear(x);
    flint_randclear(st);
}

/* ------------------------------------------------------------------ limits and hostile input */

ADF_TEST(max_len_at_the_limit_and_one_above)
{
    adf_text_limits_t lim;
    adf_rat_t x;

    adf_text_limits_default(&lim);
    lim.max_len = 5;
    adf_rat_init(x);
    set_sentinel(x);
    ADF_CHECK(adf_rat_set_str(x, "12/35", 5, &lim) == ADF_OK);
    ADF_CHECK(fmpz_cmp_si(fmpq_numref(x->q), 12) == 0 && fmpz_cmp_si(fmpq_denref(x->q), 35) == 0);
    set_sentinel(x);
    ADF_CHECK(adf_rat_set_str(x, "12/35 ", 6, &lim) == ADF_LIMIT);
    ADF_CHECK(is_sentinel(x));
    lim.max_len = 0;
    ADF_CHECK(adf_rat_set_str(x, "7", 1, &lim) == ADF_LIMIT);
    ADF_CHECK(adf_rat_set_str(x, "", 0, &lim) == ADF_PARSE);
    ADF_CHECK(is_sentinel(x));
    /* the other limits do not concern a rational: they change nothing */
    adf_text_limits_default(&lim);
    lim.max_exp10 = 0;
    lim.max_prec = 0;
    lim.max_items = 0;
    ADF_CHECK(adf_rat_set_str(x, "1000000/3", 9, &lim) == ADF_OK);
    adf_rat_clear(x);
}

ADF_TEST(empty_and_null_input)
{
    adf_rat_t x;

    adf_rat_init(x);
    set_sentinel(x);
    ADF_CHECK(adf_rat_set_str(x, NULL, 0, NULL) == ADF_PARSE);
    ADF_CHECK(adf_rat_set_str(x, "7", 0, NULL) == ADF_PARSE);
    ADF_CHECK(adf_rat_set_str(x, " \t\r\n", 4, NULL) == ADF_PARSE);
    ADF_CHECK(is_sentinel(x));
    adf_rat_clear(x);
}

ADF_TEST(a_nul_byte_anywhere_is_a_parse_error)
{
    const char * t = "-12/35";
    size_t n = strlen(t), pos;

    for (pos = 0; pos <= n; pos++)
    {
        char buf[16];
        memcpy(buf, t, pos);
        buf[pos] = '\0';
        memcpy(buf + pos + 1, t + pos, n - pos);
        check_one(buf, n + 1, ADF_PARSE, NULL, "NUL inside");
    }
    /* the length decides, not a terminator: "7\0" read as one byte is 7 */
    check_one("7\0" "3", 1, ADF_OK, "7", "length 1");
}

ADF_TEST(every_byte_outside_the_alphabet_is_a_parse_error)
{
    int b;

    for (b = 0; b < 256; b++)
    {
        char buf[3];
        int allowed = (b >= 0x20 && b <= 0x7e) || b == 9 || b == 10 || b == 13;

        buf[0] = '7';
        buf[1] = (char) b;
        if (!allowed)
        {
            check_one(buf, 2, ADF_PARSE, NULL, "a forbidden byte after 7");
            buf[0] = (char) b;
            buf[1] = '7';
            check_one(buf, 2, ADF_PARSE, NULL, "a forbidden byte before 7");
        }
        else if (b == 9 || b == 10 || b == 13 || b == ' ')
        {
            check_one(buf, 2, ADF_OK, "7", "whitespace after 7");
        }
    }
}

ADF_TEST(input_at_the_end_of_a_heap_block_of_exact_size)
{
    const char * texts[] = {"7/3", "7/", "7", "-", "007/0021", "1/0", "7/3 "};
    const int want[] = {ADF_OK, ADF_PARSE, ADF_OK, ADF_PARSE, ADF_OK, ADF_DOMAIN, ADF_OK};
    size_t k;

    for (k = 0; k < sizeof(texts) / sizeof(texts[0]); k++)
    {
        size_t n = strlen(texts[k]);
        char * b = (char *) malloc(n);   /* no terminator: the sanitizer sees a read past n */
        memcpy(b, texts[k], n);
        check_one(b, n, want[k], NULL, texts[k]);
        free(b);
    }
}

ADF_TEST(input_at_the_end_of_a_page)
{
    long page = sysconf(_SC_PAGESIZE);
    const char * texts[] = {"7/3", "7/", "-12", "1/0", " 5"};
    const int want[] = {ADF_OK, ADF_PARSE, ADF_OK, ADF_DOMAIN, ADF_OK};
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
        char * s = m + page - n;         /* the byte after the text is on a page that faults */
        memcpy(s, texts[k], n);
        check_one(s, n, want[k], NULL, texts[k]);
    }
    munmap(m, 2 * (size_t) page);
}

/* ------------------------------------------------------------------ order of checks (8.5) */

ADF_TEST(the_earlier_stage_decides_the_status)
{
    adf_text_limits_t lim;
    adf_rat_t x;

    adf_text_limits_default(&lim);
    lim.max_len = 3;
    adf_rat_init(x);
    set_sentinel(x);
    /* stage 1 against stages 2, 3, 6 */
    ADF_CHECK(adf_rat_set_str(x, "7\x80/3", 4, &lim) == ADF_LIMIT);
    ADF_CHECK(adf_rat_set_str(x, "7//3", 4, &lim) == ADF_LIMIT);
    ADF_CHECK(adf_rat_set_str(x, "10/0", 4, &lim) == ADF_LIMIT);
    ADF_CHECK(adf_rat_set_str(x, "1/0", 3, &lim) == ADF_DOMAIN);
    /* stage 2 against stages 3 and 6, the forbidden byte after the other fault */
    ADF_CHECK(adf_rat_set_str(x, "7//3\x80", 5, NULL) == ADF_PARSE);
    ADF_CHECK(adf_rat_set_str(x, "1/0\xff", 4, NULL) == ADF_PARSE);
    ADF_CHECK(adf_rat_set_str(x, "1/0\x01", 4, NULL) == ADF_PARSE);
    /* stage 3 against stage 6, the grammar fault after the zero denominator */
    ADF_CHECK(adf_rat_set_str(x, "1/0 x", 5, NULL) == ADF_PARSE);
    ADF_CHECK(adf_rat_set_str(x, "1/0/", 4, NULL) == ADF_PARSE);
    ADF_CHECK(adf_rat_set_str(x, "1/0 mod 6", 9, NULL) == ADF_PARSE);
    ADF_CHECK(is_sentinel(x));
    adf_rat_clear(x);
}

ADF_TEST(canonicalisation_on_input)
{
    check_one("-0/000", 6, ADF_DOMAIN, NULL, "-0/0");
    check_one("-0/5", 4, ADF_OK, "0", "-0/5");
    check_one("-000", 4, ADF_OK, "0", "-000");
    check_one("-6/4", 4, ADF_OK, "-3/2", "-6/4");
    check_one("6/0004", 6, ADF_OK, "3/2", "6/0004");
    check_one("0010/0015", 9, ADF_OK, "2/3", "0010/0015");
    check_one("\r\n-1/1\t", 7, ADF_OK, "-1", "-1/1");
    check_one("1/1/1", 5, ADF_PARSE, NULL, "1/1/1");
    check_one("1/-1", 4, ADF_PARSE, NULL, "1/-1");
    check_one("1 /1", 4, ADF_PARSE, NULL, "1 /1");
    check_one("1/ 1", 4, ADF_PARSE, NULL, "1/ 1");
    check_one("- 1", 3, ADF_PARSE, NULL, "- 1");
    check_one("1e5", 3, ADF_PARSE, NULL, "1e5");
    check_one("1 2", 3, ADF_PARSE, NULL, "1 2");
    check_one("x", 1, ADF_PARSE, NULL, "x");
    check_one("-", 1, ADF_PARSE, NULL, "-");
    check_one("/", 1, ADF_PARSE, NULL, "/");
}
