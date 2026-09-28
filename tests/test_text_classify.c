/* tests/test_text_classify.c: adf_text_classify, the type of a text (docs/conventions.md 9.7).

   Contract: include/adelefeld/text.h (stages 1 to 3 of conventions 8.5 only; *kind written only on
   ADF_OK); conventions 9.2 (the thirteen start symbols, pairwise disjoint), 8.2, 8.4 (max_len).
   Oracles: tests/golden/dispatch.tsv (every row); the golden files of the thirteen types (a text
   that a typed parser accepts, or refuses at a stage after the grammar, is of that type; its
   canonical output too); tests/ref/vectors/m1-text/classify.jsonl (golden inputs, their mutations and
   random texts, classified by proto/text_grammar.py; lanes/m1-text/gen_text_vectors.py). */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "support/jsonl.h"
#include "test_runner.h"

static const char * type_names[13] = {"rat", "fball", "adele", "cadele", "ucoset", "idele", "idclass",
                                      "lball", "sball", "qclass", "ffun", "rfun", "char"};

static int
status_of_name(const char * name)
{
    int k;

    for (k = 0; k < ADF_STATUS_COUNT; k++)
        if (strcmp(adf_status_str(k), name) == 0)
            return k;
    return -1;
}

static int
kind_of_name(const char * name)
{
    int k;

    for (k = 0; k < 13; k++)
        if (strcmp(type_names[k], name) == 0)
            return k;
    return -1;
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

/* Classify (s, n) and compare with an expected kind (0..12) or, for want_kind < 0, a status. The
   kind must be untouched on a status. */
static void
check_one(const char * s, size_t n, const adf_text_limits_t * lim, int want_kind, int want_status,
          const char * where)
{
    adf_text_kind k;
    int sentinel = 77, st;

    memcpy(&k, &sentinel, sizeof(k));
    st = adf_text_classify(&k, s, n, lim);
    if (want_kind >= 0)
    {
        ADF_CHECK_MSG(st == ADF_OK && (int) k == want_kind, "%s: status %d kind %d, expected kind %s", where, st,
                      (int) k, type_names[want_kind]);
    }
    else
    {
        ADF_CHECK_MSG(st == want_status, "%s: status %d, expected %d", where, st, want_status);
        ADF_CHECK_MSG(memcmp(&k, &sentinel, sizeof(k)) == 0, "%s: the kind was written on status %d", where, st);
    }
}

ADF_TEST(the_kinds_are_numbered_as_in_9_7)
{
    ADF_CHECK(ADF_TEXT_RAT == 0 && ADF_TEXT_CHAR == 12 && ADF_TEXT_QCLASS == 9);
}

ADF_TEST(every_row_of_the_golden_file_dispatch)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/dispatch.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        char where[64];

        snprintf(where, sizeof(where), "dispatch.tsv line %lu", r->line);
        if (r->is_status)
            check_one(r->input, r->input_len, NULL, -1, status_of_name(r->status), where);
        else
        {
            ADF_CHECK_MSG(kind_of_name(r->expected) >= 0, "%s: unknown type %s", where, r->expected);
            check_one(r->input, r->input_len, NULL, kind_of_name(r->expected), 0, where);
        }
        rows++;
    }
    /* tests/golden/README.md: 29 vectors in dispatch.tsv */
    ADF_CHECK_MSG(rows == 29, "dispatch.tsv has %zu rows", rows);
    golden_close(f);
}

/* Every golden file of a type: an accepted input and its canonical output are texts of that type;
   an input refused at a stage after the grammar (DOMAIN, UNSUPPORTED, NOT_DETERMINED, LIMIT of a
   literal) is of that type too; an input over max_len is LIMIT; a PARSE input is not of that type. */
ADF_TEST(the_texts_of_every_type_file_are_of_that_type)
{
    int t;

    for (t = 0; t < 13; t++)
    {
        golden_error_t err;
        golden_file * f = NULL;
        char path[64];
        size_t i;

        snprintf(path, sizeof(path), "tests/golden/%s.tsv", type_names[t]);
        ADF_CHECK_MSG(golden_open(path, GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s", golden_error_message(&err));
        if (f == NULL)
            continue;
        for (i = 0; i < golden_count(f); i++)
        {
            const golden_record * r = golden_record_at(f, i);
            char where[80];

            snprintf(where, sizeof(where), "%s line %lu", path, r->line);
            if (!r->is_status)
            {
                check_one(r->input, r->input_len, NULL, t, 0, where);
                check_one(r->expected, r->expected_len, NULL, t, 0, where);
            }
            else if (strcmp(r->status, "PARSE") == 0)
            {
                adf_text_kind k;
                int st = adf_text_classify(&k, r->input, r->input_len, NULL);
                ADF_CHECK_MSG(st != ADF_OK || (int) k != t, "%s: a PARSE input of the type is of the type", where);
            }
            else if (strcmp(r->status, "LIMIT") == 0 && r->input_len > ADF_TEXT_MAX_LEN_DEFAULT)
                check_one(r->input, r->input_len, NULL, -1, ADF_LIMIT, where);
            else
                check_one(r->input, r->input_len, NULL, t, 0, where);
        }
        golden_close(f);
    }
}

ADF_TEST(every_text_of_the_reference)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-text/classify.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    ADF_CHECK(jsonl_count(f) > 5000);
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
        snprintf(where, sizeof(where), "classify.jsonl record %zu", i + 1);
        if (exp[0] == '!')
            check_one((const char *) b, n, NULL, -1, status_of_name(exp + 1), where);
        else
            check_one((const char *) b, n, NULL, kind_of_name(exp), 0, where);
        free(b);
    }
    jsonl_close(f);
}

ADF_TEST(one_text_of_each_kind)
{
    const char * texts[13] = {"-7/3", "2 mod 6", "(1 ; 0)", "((1) + (0)*i ; 0)", "[-1]", "(1 ; 1 * [1])",
                              "<1 ; [1 mod 1]>", "[p=5: 1/5 + O(5^-2)]", "{inf: (0) + (1)*i; p=2: 1}",
                              "union((0.5 ; 0), (1 ; 1 mod 2)) + Q", "ffun(D=1, M=2; (0) + (0)*i, (1) + (0)*i)",
                              "rfun(term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))",
                              "char(q=5, n=2, s=(0.5) + (14.1)*i)"};
    int t;

    for (t = 0; t < 13; t++)
        check_one(texts[t], strlen(texts[t]), NULL, t, 0, texts[t]);
    check_one("{}", 2, NULL, ADF_TEXT_SBALL, 0, "{}");
    check_one("rfun()", 6, NULL, ADF_TEXT_RFUN, 0, "rfun()");
    check_one("{inf: 1 +/- 1; p=3: 2 + O(3); p=5: 0}", 37, NULL, ADF_TEXT_SBALL, 0, "sball");
    check_one("[p=5: 3 + O(5^4)]", 17, NULL, ADF_TEXT_LBALL, 0, "lball");
    check_one("[p=5: 3 +/- O(5^4)]", 19, NULL, -1, ADF_PARSE, "+/- in an lball");
    check_one("rfun(term(P=[(1) + (0)*i, (2) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), "
              "term(P=[], A=(2) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))", 142, NULL, ADF_TEXT_RFUN, 0, "rfun 2");
    check_one("(1 ; 0) + Q", 11, NULL, ADF_TEXT_QCLASS, 0, "qclass lift");
    check_one("(1 ; 0) +/- Q", 13, NULL, -1, ADF_PARSE, "+/- Q");
    check_one("(1 ; 0) + q", 11, NULL, -1, ADF_PARSE, "lower-case q");
    check_one("union() + Q", 11, NULL, -1, ADF_PARSE, "empty union");
    check_one("ffun(D=1, M=1)", 14, NULL, -1, ADF_PARSE, "ffun without values");
    check_one("char(q=5, n=2)", 14, NULL, -1, ADF_PARSE, "char without s");
    check_one("(1 ; -1 * [1])", 14, NULL, -1, ADF_PARSE, "negative scale");
    check_one("(1 ; 1/2 * [1 mod 3])", 21, NULL, ADF_TEXT_IDELE, 0, "idele with fraction scale");
    check_one("<1 ; [1] >", 10, NULL, ADF_TEXT_IDCLASS, 0, "idclass");
    check_one("[1 mod -3]", 10, NULL, -1, ADF_PARSE, "negative ucoset modulus");
    check_one("[p=5: 1 + O(5^+2)]", 18, NULL, -1, ADF_PARSE, "plus exponent");
    check_one("{inf: 1; }", 10, NULL, -1, ADF_PARSE, "trailing semicolon");
    check_one("{p=5: 1 + O(3)}", 15, NULL, ADF_TEXT_SBALL, 0, "base differs: syntax only");
    check_one("7/3 mod 6", 9, NULL, ADF_TEXT_FBALL, 0, "bare fball");
    check_one("7/3", 3, NULL, ADF_TEXT_RAT, 0, "rat");
    check_one("7.5", 3, NULL, -1, ADF_PARSE, "a bare decimal is no type");
    check_one("hello", 5, NULL, -1, ADF_PARSE, "hello");
}

/* The integer literals of the other types: which may carry a sign or a denominator (conventions
   9.1, 9.2: uint, int, sint, urat). Expected kinds from proto/text_grammar.py classify. Added after
   the mutation run of src/text.c (survivor at the "p=" uint). */
ADF_TEST(signs_and_denominators_of_the_other_literals)
{
    const char * parse[] = {"[p=5/3: 1]", "[p=-5: 1]", "{p=5/3: 1}", "[1 mod 6/2]", "[1/2 mod 6]",
                            "[p=5: 1 + O(5/1)]", "[p=5: 1 + O(-5)]", "[p=5: 1 + O(5^2/3)]",
                            "ffun(D=1/1, M=1; (0) + (0)*i)", "ffun(D=1, M=-1; (0) + (0)*i)",
                            "char(q=5/1, n=2, s=(0) + (0)*i)", "char(q=5, n=-2, s=(0) + (0)*i)",
                            "(1 ; -1/2 * [1])", "(1 ; 1/2 * [1/1])", "<1 ; [1 mod 2/1]>", "[p=Z: 1]",
                            "(1 ; 0) + QZ"};
    size_t k;

    for (k = 0; k < sizeof(parse) / sizeof(parse[0]); k++)
        check_one(parse[k], strlen(parse[k]), NULL, -1, ADF_PARSE, parse[k]);
    check_one("[-1 mod 3]", 10, NULL, ADF_TEXT_UCOSET, 0, "negative unit residue");
    check_one("[p=5: 1 + O(5^-2)]", 18, NULL, ADF_TEXT_LBALL, 0, "negative exponent");
    check_one("[p=5: -1/3 + O(5^2)]", 20, NULL, ADF_TEXT_LBALL, 0, "negative fraction centre");
    check_one("(1 ; 1/2 * [1])", 15, NULL, ADF_TEXT_IDELE, 0, "fraction scale");
}

ADF_TEST(limits_hostile_bytes_and_the_order_of_checks)
{
    adf_text_limits_t lim;
    int b;
    size_t pos;
    const char * t = "(1 ; 2 mod 6)";
    size_t n = strlen(t);

    adf_text_limits_default(&lim);
    lim.max_len = 13;
    check_one(t, n, &lim, ADF_TEXT_ADELE, 0, "at max_len");
    check_one("(1 ; 2 mod 6) ", n + 1, &lim, -1, ADF_LIMIT, "max_len + 1");
    check_one("(1 ; 2 mod 6)\x80", n + 1, &lim, -1, ADF_LIMIT, "stage 1 before 2");
    check_one("(1 ; 2 mod 6 x", n + 1, &lim, -1, ADF_LIMIT, "stage 1 before 3");
    check_one("(1 ; 2 mod 6\x80", n, NULL, -1, ADF_PARSE, "stage 2");
    /* classification is syntax only: stage 4 and 6 faults leave the kind */
    check_one("(1e100001 ; 1/0)", 16, NULL, ADF_TEXT_ADELE, 0, "stages 4 and 6 are not checked");
    lim.max_exp10 = 0;
    lim.max_prec = 0;
    lim.max_items = 0;
    lim.max_len = 100;
    check_one("(1e5 ; 0)", 9, &lim, ADF_TEXT_ADELE, 0, "max_exp10 is not checked");
    check_one("ffun(D=2, M=1; (0) + (0)*i, (0) + (0)*i)", 40, &lim, ADF_TEXT_FFUN, 0, "max_items is not checked");
    check_one(NULL, 0, NULL, -1, ADF_PARSE, "NULL");
    check_one(t, 0, NULL, -1, ADF_PARSE, "len 0");
    for (pos = 0; pos <= n; pos++)
    {
        char buf[32];
        memcpy(buf, t, pos);
        buf[pos] = '\0';
        memcpy(buf + pos + 1, t + pos, n - pos);
        check_one(buf, n + 1, NULL, -1, ADF_PARSE, "NUL inside");
    }
    for (b = 0; b < 256; b++)
    {
        char buf[32];
        if ((b >= 0x20 && b <= 0x7e) || b == 9 || b == 10 || b == 13)
            continue;
        memcpy(buf, t, n);
        buf[4] = (char) b;
        check_one(buf, n, NULL, -1, ADF_PARSE, "forbidden byte");
    }
}

ADF_TEST(input_without_terminator_at_a_block_end_and_a_page_end)
{
    const char * texts[] = {"[p=5: 1 + O(5^2)]", "[p=5: 1 + O(5^", "ffun(D=1, M=1; (0) + (0)*i)", "rfun(term(",
                            "char(q=5, n=2, s=(0.5) + (14.1)*i)", "{inf: 1", "union((0.5 ; 0)) + Q",
                            "union((0.5 ; 0)) +",
                            "7/3", "7/", "(1 +/- 1e", "<1 ; [1]>", "<1 ; [1]"};
    const int want[] = {ADF_TEXT_LBALL, -1, ADF_TEXT_FFUN, -1, ADF_TEXT_CHAR, -1, ADF_TEXT_QCLASS, -1, ADF_TEXT_RAT,
                        -1, -1, ADF_TEXT_IDCLASS, -1};
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
        char * h = (char *) malloc(n);
        memcpy(h, texts[k], n);
        memcpy(m + page - n, texts[k], n);
        check_one(h, n, NULL, want[k], ADF_PARSE, texts[k]);
        check_one(m + page - n, n, NULL, want[k], ADF_PARSE, texts[k]);
        free(h);
    }
    munmap(m, 2 * (size_t) page);
}
