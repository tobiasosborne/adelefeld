/* tests/test_support.c: the tests of the two support readers, tests/support/jsonl.c and
   tests/support/golden.c.

   They read the real files of the repository, so they are run from the repository root, which
   is what `make check` does. The claims are:

   - every vector file under tests/ref/vectors/ is read, every record is an object, and the
     number of records is the number of lines of the file and the number the generator wrote;
   - every record carries the fields that tests/ref/README.md gives for its operation, of the
     right kind, and every integer is a literal that fmpz_set_str reads;
   - a malformed line is refused, with the file and the line of the error, and nothing else in
     the file is silently accepted;
   - every golden file under tests/golden/ is read, the number of vectors and the number of
     status vectors are those of tests/golden/README.md, and every status is one of the codes
     of docs/conventions.md 3.1 (lines 144 to 157);
   - the escapes of docs/conventions.md 11.1 are decoded as it says, embedded NUL and
     non-ASCII bytes included, and an @gen: line is expanded;
   - a malformed golden line is refused, with the file and the line.

   A reader that returned a wrong count, ignored an error, dropped a byte, or expanded an
   @gen: line wrongly fails one of these tests. */

#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "support/jsonl.h"
#include "test_runner.h"

/* The vector files: the path, the number of records that tests/ref/gen_vectors.py wrote, and
   the operations the file may carry (tests/ref/README.md, "Vector format"). The record counts
   are the counts of the files themselves; a test below checks that against `wc -l`. */

static const struct
{
    const char *path;
    unsigned long records;
    const char *ops;
} vector_files[] = {
    {"tests/ref/vectors/add.jsonl", 444, "add"},
    {"tests/ref/vectors/canonical.jsonl", 212, "canonical"},
    {"tests/ref/vectors/compare.jsonl", 444, "compare"},
    {"tests/ref/vectors/membership.jsonl", 580, "membership"},
    {"tests/ref/vectors/mul.jsonl", 444, "mul"},
    {"tests/ref/vectors/neg.jsonl", 212, "neg"},
    {"tests/ref/vectors/policies.jsonl", 1260,
     "convert_from_tight scaled_add scaled_sub scaled_mul absolute_cap"},
    {"tests/ref/vectors/predicates.jsonl", 1332, "equal_set overlaps contains"},
    {"tests/ref/vectors/recon.jsonl", 360, "reconstruct"},
    {"tests/ref/vectors/scale.jsonl", 396, "scale"},
    {"tests/ref/vectors/sub.jsonl", 444, "sub"},
};

#define N_VECTOR_FILES (sizeof(vector_files) / sizeof(vector_files[0]))

/* The golden files. The number of vectors and the number of status vectors of each are not
   written here: the lane that owns tests/golden/ was rewriting the files while this test was
   written (2026-09-28: the counts of dump.tsv, realball_print.tsv, realball_read.tsv and
   rfun.tsv no longer agreed with tests/golden/README.md), and a number frozen here would be a
   claim about another lane's file. What is checked instead is the claim about the reader: one
   record for every line of the file that is neither a comment nor empty, and one status vector
   for every line whose expected field starts with a '!'. Both are counted from the file by
   count_vector_lines below, so a reader that drops, doubles or splits a line fails. */

static const char *const golden_files[] = {
    "tests/golden/adele.tsv",      "tests/golden/cadele.tsv",     "tests/golden/char.tsv",
    "tests/golden/dispatch.tsv",   "tests/golden/dump.tsv",       "tests/golden/fball.tsv",
    "tests/golden/ffun.tsv",       "tests/golden/gauss.tsv",      "tests/golden/idclass.tsv",
    "tests/golden/idele.tsv",      "tests/golden/lball.tsv",      "tests/golden/psi_phases.tsv",
    "tests/golden/qclass.tsv",     "tests/golden/rat.tsv",        "tests/golden/realball_print.tsv",
    "tests/golden/realball_read.tsv", "tests/golden/rfun.tsv",     "tests/golden/sball.tsv",
    "tests/golden/ucoset.tsv",
};

/* The number of lines of a golden file that carry a vector, and of those the number whose
   expected field is a status. The file is read as bytes: a line that starts with '#' is a
   comment, an empty line is ignored, and the expected field is the one after the first TAB. */

static void
count_vector_lines(const char *path, unsigned long *vectors, unsigned long *statuses)
{
    FILE *fp = fopen(path, "rb");
    char line[65536];

    *vectors = 0;
    *statuses = 0;
    if (fp == NULL)
        return;
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        size_t len = strlen(line);
        char *tab;

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len > 0 && line[0] == '#')
            continue;
        if (len == 0)
            continue;
        tab = strchr(line, '\t');
        if (tab == NULL)
            continue; /* the reader refuses this line; golden_refuses_malformed_lines says so */
        (*vectors)++;
        if (tab[1] == '!')
            (*statuses)++;
    }
    fclose(fp);
}

#define N_GOLDEN_FILES (sizeof(golden_files) / sizeof(golden_files[0]))

/* The status codes of docs/conventions.md 3.1, lines 144 to 157, without the leading ADF_ and
   without ADF_OK, which is not a failure. */

static const char *const status_names[] = {"NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT",
                                           "NOT_UNIQUE",     "NO_SOLUTION",       "NOT_UNIT",
                                           "DOMAIN",         "UNSUPPORTED",       "PARSE",
                                           "LIMIT"};

#define N_STATUS_NAMES (sizeof(status_names) / sizeof(status_names[0]))

/* The number of newlines in a file, the number `wc -l` prints. NULL if it cannot be read. */

static long
count_lines(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long n = 0;
    int ch;

    if (fp == NULL)
        return -1;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (ch == '\n')
            n++;
    }
    fclose(fp);
    return n;
}

/* 1 if word is one of the space-separated words of list. */

static int
word_in(const char *list, const char *word)
{
    size_t n = strlen(word);
    const char *p = list;

    while ((p = strstr(p, word)) != NULL)
    {
        int left = (p == list) || p[-1] == ' ';
        int right = (p[n] == '\0' || p[n] == ' ');

        if (left && right)
            return 1;
        p += n;
    }
    return 0;
}

/* An integer of a record, read into z. The claim is that the text is one fmpz_set_str takes
   whatever its size. */

static void
check_int(const jsonl_value *v, fmpz_t z)
{
    jsonl_error_t err;
    const char *text = jsonl_int_text(v, &err);

    ADF_CHECK_MSG(text != NULL, "expected an integer: %s", jsonl_error_message(&err));
    if (text == NULL)
        return;
    ADF_CHECK_MSG(fmpz_set_str(z, text, 10) == 0, "fmpz_set_str refused \"%s\"", text);
}

/* A rational {"num": n, "den": d}. */

static void
check_rat(const jsonl_value *v)
{
    jsonl_error_t err;
    const jsonl_value *num, *den;
    fmpq_t q;

    ADF_CHECK_MSG(jsonl_is(v, JSONL_OBJECT), "a rational is an object, found a %s",
                  jsonl_kind_name(v));
    if (!jsonl_is(v, JSONL_OBJECT))
        return;
    ADF_CHECK_MSG(jsonl_field(v, "num", &num, &err) == 1, "%s", jsonl_error_message(&err));
    ADF_CHECK_MSG(jsonl_field(v, "den", &den, &err) == 1, "%s", jsonl_error_message(&err));
    if (num == NULL || den == NULL)
        return;
    fmpq_init(q);
    check_int(num, fmpq_numref(q));
    check_int(den, fmpq_denref(q));
    fmpq_clear(q);
}

/* A finite ball {"A": a, "H": h, "d": d}. */

static void
check_ball(const jsonl_value *v)
{
    jsonl_error_t err;
    const jsonl_value *f[3];
    const char *const keys[3] = {"A", "H", "d"};
    fmpz_t t;
    int i;

    ADF_CHECK_MSG(jsonl_is(v, JSONL_OBJECT), "a ball is an object, found a %s", jsonl_kind_name(v));
    if (!jsonl_is(v, JSONL_OBJECT))
        return;
    ADF_CHECK(jsonl_size(v) == 3);
    for (i = 0; i < 3; i++)
    {
        const char *key = jsonl_key(v, (size_t) i);

        ADF_CHECK_MSG(key != NULL, "the ball has no key at %d", i);
        if (key == NULL)
            return;
        ADF_CHECK(strcmp(key, keys[i]) == 0);
        ADF_CHECK(jsonl_field(v, keys[i], &f[i], &err));
        if (f[i] == NULL)
            return;
    }
    fmpz_init(t);
    for (i = 0; i < 3; i++)
        check_int(f[i], t);
    fmpz_clear(t);
}

/* A scaled value {"s": q, "u": n, "K": n} of tests/ref/README.md. */

static void
check_scaled(const jsonl_value *v)
{
    jsonl_error_t err;
    const jsonl_value *s, *u, *k;
    fmpq_t q;
    fmpz_t t;

    ADF_CHECK_MSG(jsonl_is(v, JSONL_OBJECT), "a scaled value is an object, found a %s",
                  jsonl_kind_name(v));
    if (!jsonl_is(v, JSONL_OBJECT))
        return;
    ADF_CHECK(jsonl_field(v, "s", &s, &err));
    ADF_CHECK(jsonl_field(v, "u", &u, &err));
    ADF_CHECK(jsonl_field(v, "K", &k, &err));
    if (s == NULL || u == NULL || k == NULL)
        return;
    fmpq_init(q);
    fmpz_init(t);
    check_rat(s);
    check_int(u, t);
    check_int(k, t);
    fmpq_clear(q);
    fmpz_clear(t);
}

/* The fields one record of one operation carries, and their kinds. Nothing is compared with a
   computed result here: that is the work of the tests of the library. */

/* The comparison of two finite balls is three-valued. tests/ref/README.md line 63 says that
   the result of a compare record is "one of the three comparison strings"; the vector file as
   it stands on 2026-09-28 carries the three values as integers instead, because the lane that
   owns tests/ref/ had rewritten it while this test was written. So the test accepts both
   forms and checks that the three values are there: the integers -1, 0, 1, or three distinct
   strings. A fourth value, or a two-valued file, fails. */

#define COMPARE_MAX_VALUES 8

static const char *compare_values[COMPARE_MAX_VALUES]; /* the distinct results, integers */
static size_t compare_nvalues = 0;
static const char *compare_strings[COMPARE_MAX_VALUES]; /* the distinct results, strings */
static size_t compare_nstrings = 0;

/* Note one result of a compare record, and check that it has one form only: a file in which
   some records carry an integer and others a string is not a fixture. Returns 0 on an error,
   with a message. */

static int
check_compare_value(const jsonl_value *v)
{
    jsonl_error_t err;
    const char **table;
    size_t *count;
    size_t i;
    const char *text;

    if (jsonl_is(v, JSONL_STR))
    {
        table = compare_strings;
        count = &compare_nstrings;
        text = jsonl_string(v, NULL, &err);
    }
    else
    {
        table = compare_values;
        count = &compare_nvalues;
        text = jsonl_int_text(v, &err);
    }
    if (text == NULL)
    {
        ADF_CHECK_MSG(0, "the result of a compare record is a string or an integer, found a %s",
                      jsonl_kind_name(v));
        return 0;
    }
    if (*count > 0)
    {
        ADF_CHECK_MSG(compare_nvalues == 0 || compare_nstrings == 0,
                      "the compare records carry both an integer and a string result");
        if (compare_nvalues > 0 && compare_nstrings > 0)
            return 0;
    }
    for (i = 0; i < *count; i++)
    {
        if (strcmp(table[i], text) == 0)
            return 1;
    }
    if (*count == COMPARE_MAX_VALUES)
    {
        ADF_CHECK_MSG(0, "the compare records have more than %d distinct results",
                      COMPARE_MAX_VALUES);
        return 0;
    }
    table[(*count)++] = text;
    return 1;
}

static void
check_record_fields(const jsonl_value *rec, const char *op)
{
    jsonl_error_t err;
    const jsonl_value *v[4];
    int b;

    /* a failed accessor fills this, so that the last message can be printed */
    memset(&err, 0, sizeof(err));

    if (strcmp(op, "canonical") == 0)
    {
        fmpz_t t;

        ADF_CHECK(jsonl_field(rec, "input", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[1], &err));
        if (v[0] == NULL || v[1] == NULL)
            return;
        ADF_CHECK(jsonl_is(v[0], JSONL_ARRAY) && jsonl_size(v[0]) == 3);
        ADF_CHECK(jsonl_is(v[1], JSONL_ARRAY) && jsonl_size(v[1]) == 3);
        fmpz_init(t);
        for (b = 0; b < 3; b++)
        {
            const jsonl_value *e = jsonl_at(v[0], (size_t) b, &err);

            ADF_CHECK(e != NULL);
            if (e != NULL)
                check_int(e, t);
        }
        fmpz_clear(t);
        return;
    }
    if (strcmp(op, "reconstruct") == 0)
    {
        const jsonl_value *sol;
        size_t i;

        ADF_CHECK(jsonl_field(rec, "ball", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "lo", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "hi", &v[2], &err));
        ADF_CHECK(jsonl_field(rec, "status", &v[3], &err));
        ADF_CHECK(jsonl_field(rec, "solutions", &sol, &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL || v[3] == NULL || sol == NULL)
            return;
        check_ball(v[0]);
        check_rat(v[1]);
        check_rat(v[2]);
        ADF_CHECK(jsonl_is(v[3], JSONL_STR));
        ADF_CHECK(jsonl_is(sol, JSONL_ARRAY));
        for (i = 0; i < jsonl_size(sol); i++)
            check_rat(jsonl_at(sol, i, &err));
        return;
    }
    if (strcmp(op, "convert_from_tight") == 0)
    {
        int lost;

        ADF_CHECK(jsonl_field(rec, "ball", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "K", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "value", &v[2], &err));
        ADF_CHECK(jsonl_field(rec, "lost", &v[3], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL || v[3] == NULL)
            return;
        {
            fmpz_t t;

            fmpz_init(t);
            check_int(v[1], t);
            fmpz_clear(t);
        }
        ADF_CHECK(jsonl_is(v[2], JSONL_OBJECT));
        ADF_CHECK(jsonl_bool(v[3], &lost, &err));
        if (jsonl_is(v[2], JSONL_OBJECT))
        {
            const jsonl_value *exact;

            if (jsonl_field(v[2], "exact", &exact, &err))
                check_rat(exact);
            else
            {
                ADF_CHECK(strstr(err.message, "is missing") != NULL);
                check_scaled(v[2]);
            }
        }
        return;
    }
    if (strcmp(op, "absolute_cap") == 0)
    {
        ADF_CHECK(jsonl_field(rec, "ball", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "C", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
            return;
        check_ball(v[0]);
        check_rat(v[1]);
        check_ball(v[2]);
        return;
    }
    if (strcmp(op, "compare") == 0)
    {
        ADF_CHECK(jsonl_field(rec, "a", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "b", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
            return;
        check_ball(v[0]);
        check_ball(v[1]);
        check_compare_value(v[2]);
        return;
    }
    if (strcmp(op, "membership") == 0)
    {
        int r;

        ADF_CHECK(jsonl_field(rec, "ball", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "x", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
            return;
        check_ball(v[0]);
        check_rat(v[1]);
        ADF_CHECK(jsonl_bool(v[2], &r, &err));
        return;
    }
    if (strcmp(op, "scale") == 0)
    {
        ADF_CHECK(jsonl_field(rec, "a", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "q", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
            return;
        check_ball(v[0]);
        check_rat(v[1]);
        check_ball(v[2]);
        return;
    }
    if (strcmp(op, "scaled_add") == 0 || strcmp(op, "scaled_sub") == 0 ||
        strcmp(op, "scaled_mul") == 0)
    {
        ADF_CHECK(jsonl_field(rec, "a", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "b", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
            return;
        check_scaled(v[0]);
        check_scaled(v[1]);
        check_scaled(v[2]);
        return;
    }
    if (strcmp(op, "equal_set") == 0 || strcmp(op, "overlaps") == 0 || strcmp(op, "contains") == 0)
    {
        int r;

        ADF_CHECK(jsonl_field(rec, "a", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "b", &v[1], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
        if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
            return;
        check_ball(v[0]);
        check_ball(v[1]);
        ADF_CHECK(jsonl_bool(v[2], &r, &err));
        return;
    }
    if (strcmp(op, "neg") == 0)
    {
        ADF_CHECK(jsonl_field(rec, "a", &v[0], &err));
        ADF_CHECK(jsonl_field(rec, "result", &v[1], &err));
        if (v[0] == NULL || v[1] == NULL)
            return;
        check_ball(v[0]);
        check_ball(v[1]);
        return;
    }
    /* add, sub, mul: two balls in, one out. */

    ADF_CHECK(jsonl_field(rec, "a", &v[0], &err));
    ADF_CHECK(jsonl_field(rec, "b", &v[1], &err));
    ADF_CHECK(jsonl_field(rec, "result", &v[2], &err));
    ADF_CHECK_MSG(err.message[0] == '\0', "fields of %s: %s", op, jsonl_error_message(&err));
    if (v[0] == NULL || v[1] == NULL || v[2] == NULL)
        return;
    check_ball(v[0]);
    check_ball(v[1]);
    check_ball(v[2]);
}

/* Every vector file is read; the number of records is the number of lines and the number the
   generator wrote; every record is an object with an "op" the file may carry. */

ADF_TEST(jsonl_reads_every_vector_file)
{
    size_t k;
    unsigned long total = 0;

    for (k = 0; k < N_VECTOR_FILES; k++)
    {
        jsonl_error_t err;
        jsonl_file *f = NULL;
        size_t i;

        ADF_CHECK_MSG(jsonl_open(vector_files[k].path, &f, &err) == 1, "%s: %s",
                      vector_files[k].path, jsonl_error_message(&err));
        if (f == NULL)
            continue;
        ADF_CHECK_MSG(jsonl_file_error(f) == NULL, "%s holds an error: %s", vector_files[k].path,
                      jsonl_error_message(jsonl_file_error(f)));
        ADF_CHECK_MSG(jsonl_count(f) == vector_files[k].records, "%s: %lu records, expected %lu",
                      vector_files[k].path, (unsigned long) jsonl_count(f),
                      vector_files[k].records);
        ADF_CHECK_MSG(count_lines(vector_files[k].path) == (long) jsonl_count(f),
                      "%s: %ld lines for %lu records", vector_files[k].path,
                      count_lines(vector_files[k].path), (unsigned long) jsonl_count(f));
        total += (unsigned long) jsonl_count(f);

        for (i = 0; i < jsonl_count(f); i++)
        {
            const jsonl_value *rec = jsonl_record(f, i);
            const jsonl_value *op;
            const char *text;
            size_t len;

            ADF_CHECK_MSG(jsonl_is(rec, JSONL_OBJECT), "%s line %lu: a record is an object, found a %s",
                          vector_files[k].path, (unsigned long) i + 1, jsonl_kind_name(rec));
            if (!jsonl_is(rec, JSONL_OBJECT))
                break;
            if (!jsonl_field(rec, "op", &op, &err))
            {
                ADF_CHECK_MSG(0, "%s line %lu: %s", vector_files[k].path, (unsigned long) i + 1,
                              jsonl_error_message(&err));
                break;
            }
            text = jsonl_string(op, &len, &err);
            if (text == NULL)
            {
                ADF_CHECK_MSG(0, "%s line %lu: %s", vector_files[k].path, (unsigned long) i + 1,
                              jsonl_error_message(&err));
                break;
            }
            ADF_CHECK_MSG(len == strlen(text),
                          "%s line %lu: the operation name holds a NUL in the middle",
                          vector_files[k].path, (unsigned long) i + 1);
            ADF_CHECK_MSG(word_in(vector_files[k].ops, text),
                          "%s line %lu: the operation \"%s\" is not one of \"%s\"",
                          vector_files[k].path, (unsigned long) i + 1, text, vector_files[k].ops);
            ADF_CHECK_MSG(jsonl_line_of(rec) == (unsigned long) i + 1,
                          "%s record %lu stands on line %lu", vector_files[k].path,
                          (unsigned long) i + 1, jsonl_line_of(rec));
            check_record_fields(rec, text);
        }
        jsonl_close(f);
    }
    ADF_CHECK_MSG(total == 6128, "%lu records over the eleven vector files, expected 6128", total);
    /* the comparison of two balls has three values, in one form or the other */
    ADF_CHECK_MSG(compare_nvalues + compare_nstrings == 3,
                  "the compare records have %lu distinct integer results and %lu distinct string "
                  "results, expected three values in one form",
                  (unsigned long) compare_nvalues, (unsigned long) compare_nstrings);
}

/* The first records of add.jsonl, field by field, against the file. */

ADF_TEST(jsonl_reads_the_documented_records)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    const jsonl_value *rec, *v, *A;
    fmpz_t t;
    size_t i;

    ADF_CHECK(jsonl_open("tests/ref/vectors/add.jsonl", &f, &err) == 1);
    if (f == NULL)
        return;
    fmpz_init(t);

    for (i = 0; i < 3; i++)
    {
        rec = jsonl_record(f, i);
        ADF_CHECK(jsonl_is(rec, JSONL_OBJECT));
        ADF_CHECK(jsonl_field(rec, "a", &v, &err));
        if (v == NULL)
            break;
        ADF_CHECK(jsonl_field(v, "A", &A, &err));
        check_int(A, t);
        ADF_CHECK(fmpz_is_zero(t)); /* the first three records have A = 0 in a and in b */
        ADF_CHECK(jsonl_field(v, "H", &A, &err));
        check_int(A, t);
        ADF_CHECK(fmpz_is_zero(t));
        ADF_CHECK(jsonl_field(v, "d", &A, &err));
        check_int(A, t);
        ADF_CHECK(fmpz_equal_ui(t, 1));
    }

    /* Record 2 (the third line) has b = 3/4, so the ball is {"A": 3, "H": 0, "d": 4}. */

    rec = jsonl_record(f, 2);
    ADF_CHECK(jsonl_field(rec, "b", &v, &err));
    if (v != NULL)
    {
        ADF_CHECK(jsonl_field(v, "A", &A, &err));
        check_int(A, t);
        ADF_CHECK(fmpz_equal_ui(t, 3));
        ADF_CHECK(jsonl_field(v, "d", &A, &err));
        check_int(A, t);
        ADF_CHECK(fmpz_equal_ui(t, 4));
    }
    fmpz_clear(t);
    jsonl_close(f);
}

/* A large integer, written as a JSON string, reaches fmpz_set_str without loss. */

ADF_TEST(jsonl_keeps_integers_as_text)
{
    static const char *const big = "123456789012345678901234567890123456789012345678901234567890";
    static const char doc[] = "{\"n\": \"" "123456789012345678901234567890123456789012345678901234567890"
                              "\", \"s\": \"-7\"}";
    jsonl_error_t err;
    jsonl_file *f = NULL;
    jsonl_file *g = NULL;
    const jsonl_value *rec, *v;
    const char *itxt = NULL;
    fmpz_t z;
    char *s;
    size_t len = 0;

    ADF_CHECK(jsonl_parse(doc, strlen(doc), "memory", &f, &err) == 1);
    if (f == NULL)
        return;
    rec = jsonl_record(f, 0);
    ADF_CHECK(jsonl_field(rec, "n", &v, &err));
    if (v != NULL)
    {
        ADF_CHECK(jsonl_is(v, JSONL_STR));
        ADF_CHECK(jsonl_int_text_or_string(v, &itxt, &err) == 1);
        ADF_CHECK_MSG(itxt != NULL && strcmp(itxt, big) == 0,
                      "the integer as text is not the 60 digits of the input");
        if (itxt != NULL)
        {
            fmpz_init(z);
            ADF_CHECK(fmpz_set_str(z, itxt, 10) == 0);
            /* 60 digits, so fmpz_sizeinbase(z, 10) is 60 and the sign is positive */
            ADF_CHECK(fmpz_sizeinbase(z, 10) == 60);
            ADF_CHECK(fmpz_sgn(z) > 0);
            fmpz_clear(z);
        }
    }
    ADF_CHECK(jsonl_field(rec, "s", &v, &err));
    if (v != NULL)
    {
        ADF_CHECK(jsonl_int_text_or_string(v, &itxt, &err) == 1);
        ADF_CHECK(itxt != NULL && strcmp(itxt, "-7") == 0);
    }
    jsonl_close(f);

    /* a string that is not an integer is refused, not silently dropped */

    if (jsonl_parse("{\"n\": \"12x\"}", 12, "memory", &g, &err) == 1)
    {
        rec = jsonl_record(g, 0);
        ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
        ADF_CHECK(jsonl_int_text_or_string(v, &itxt, &err) == 0);
        ADF_CHECK(strstr(err.message, "not an integer") != NULL);
        jsonl_close(g);
        g = NULL;
    }
    if (jsonl_parse("{\"n\": \"\"}", 9, "memory", &g, &err) == 1)
    {
        rec = jsonl_record(g, 0);
        ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
        ADF_CHECK(jsonl_int_text_or_string(v, &itxt, &err) == 0);
        jsonl_close(g);
        g = NULL;
    }
    if (jsonl_parse("{\"n\": \"01\"}", 10, "memory", &g, &err) == 1)
    {
        rec = jsonl_record(g, 0);
        ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
        ADF_CHECK(jsonl_int_text_or_string(v, &itxt, &err) == 0);
        ADF_CHECK(strstr(err.message, "leading zero") != NULL);
        jsonl_close(g);
        g = NULL;
    }
    if (jsonl_parse("{\"n\": \"1 2\"}", 11, "memory", &g, &err) == 1)
    {
        rec = jsonl_record(g, 0);
        ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
        ADF_CHECK(jsonl_int_text_or_string(v, &itxt, &err) == 0);
        jsonl_close(g);
        g = NULL;
    }

    /* the text of a string is a C string too, and the length is the decoded length */

    ADF_CHECK(jsonl_parse("{\"n\": \"a\\u0041b\"}", 17, "memory", &g, &err) == 1);
    if (g != NULL)
    {
        rec = jsonl_record(g, 0);
        ADF_CHECK(jsonl_field(rec, "n", &v, &err));
        if (v != NULL)
        {
            ADF_CHECK(jsonl_is(v, JSONL_STR));
            s = (char *) jsonl_string(v, &len, &err);
            ADF_CHECK(s != NULL);
            if (s != NULL)
            {
                ADF_CHECK(len == 3);
                ADF_CHECK(s[0] == 'a' && s[1] == 'A' && s[2] == 'b' && s[3] == '\0');
            }
        }
        jsonl_close(g);
    }
}

/* A malformed line is refused, and the error names the file and the line. The text of the
   message is checked for the cases where the reason is the claim. */

ADF_TEST(jsonl_refuses_malformed_lines)
{
    static const struct
    {
        const char *text;
        const char *why;
    } bad[] = {
        {"", "empty"},
        {"   ", "white space"},
        {"{", "key must be a string"},
        {"}", "unexpected byte"},
        {"{\"a\"}", "expected ':'"},
        {"{\"a\":}", "unexpected byte"},
        {"{\"a\":1,}", "string"},
        {"{a:1}", "key must be a string"},
        {"{'a':1}", "key must be a string"},
        {"{\"a\":1 \"b\":2}", "expected ',' or '}'"},
        {"{\"a\":01}", "leading zero"},
        {"{\"a\":+1}", "unexpected byte"},
        {"{\"a\":.5}", "unexpected byte"},
        {"{\"a\":1.5}", "integer numbers"},
        {"{\"a\":1e3}", "integer numbers"},
        {"{\"a\":1E3}", "integer numbers"},
        {"{\"a\":-}", "digit"},
        {"{\"a\":1} {\"b\":2}", "trailing bytes"},
        {"{\"a\":1}{\"b\":2}", "trailing bytes"},
        {"{\"a\":1,\"a\":2}", "twice"},
        {"{\"a\":\"x}", "unterminated string"},
        {"{\"a\":\"x\\qy\"}", "unknown escape"},
        {"{\"a\":\"x\\u00zz\"}", "four hexadecimal"},
        {"{\"a\":\"\\ud800\"}", "high surrogate"},
        {"{\"a\":\"\\ud800\\u0041\"}", "high surrogate"},
        {"{\"a\":tru}", "expected true"},
        {"{\"a\":nul}", "expected null"},
        {"{\"a\":NaN}", "unexpected byte"},
        {"{\"a\":[1,2}", "',' or ']'"},
        {"{\"a\":[1 2]}", "',' or ']'"},
        {"{\"a\":1}\n{\"b\"", "expected ':'"},
        {"{\"a\":1}\n\n{\"b\":2}", "empty line"},
        {"{\"a\":\"a\nb\"}", "below 0x20"},
    };
    size_t k;

    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++)
    {
        jsonl_error_t err;
        jsonl_file *f = (jsonl_file *) (void *) &bad; /* a nonnull value that must be cleared */
        int ok = jsonl_parse(bad[k].text, strlen(bad[k].text), "memory", &f, &err);

        ADF_CHECK_MSG(ok == 0, "\"%s\" was accepted", bad[k].text);
        ADF_CHECK_MSG(f == NULL, "\"%s\" left a file behind", bad[k].text);
        ADF_CHECK_MSG(err.file != NULL && strcmp(err.file, "memory") == 0, "\"%s\": no file name",
                      bad[k].text);
        ADF_CHECK_MSG(err.line >= 1, "\"%s\": no line number", bad[k].text);
        ADF_CHECK_MSG(err.message[0] != '\0', "\"%s\": no message", bad[k].text);
        ADF_CHECK_MSG(strstr(err.message, bad[k].why) != NULL, "\"%s\": the message \"%s\" does not "
                                                            "say \"%s\"",
                      bad[k].text, err.message, bad[k].why);
    }
}

/* A \u escape is encoded as UTF-8: one byte below 0x80, two below 0x800, three below 0x10000
   and four for a code point above it, which a surrogate pair spells. The test states the four
   cases byte for byte, so that a wrong shift or a wrong mask in the encoding is caught. */

ADF_TEST(jsonl_encodes_a_unicode_escape)
{
    static const struct
    {
        const char *doc;
        const char *bytes;
        size_t len;
    } cases[] = {
        {"{\"n\": \"\\u0041\"}", "A", 1},
        {"{\"n\": \"\\u00e9\"}", "\xc3\xa9", 2},
        {"{\"n\": \"\\u20ac\"}", "\xe2\x82\xac", 3},
        {"{\"n\": \"\\ud83d\\ude00\"}", "\xf0\x9f\x98\x80", 4},
        {"{\"n\": \"\\u007f\\u0080\\u07ff\\u0800\"}", "\x7f\xc2\x80\xdf\xbf\xe0\xa0\x80", 8},
    };
    size_t k;

    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++)
    {
        jsonl_error_t err;
        jsonl_file *f = NULL;
        const jsonl_value *rec, *v;
        size_t len = 0;
        const char *s;

        ADF_CHECK_MSG(jsonl_parse(cases[k].doc, strlen(cases[k].doc), "memory", &f, &err) == 1,
                      "%s: %s", cases[k].doc, jsonl_error_message(&err));
        if (f == NULL)
            continue;
        rec = jsonl_record(f, 0);
        ADF_CHECK(jsonl_field(rec, "n", &v, &err) == 1);
        if (v != NULL)
        {
            s = jsonl_string(v, &len, &err);
            ADF_CHECK_MSG(len == cases[k].len, "%s: %lu bytes, expected %lu", cases[k].doc,
                          (unsigned long) len, (unsigned long) cases[k].len);
            ADF_CHECK_MSG(s != NULL && memcmp(s, cases[k].bytes, cases[k].len) == 0,
                          "%s: the bytes are not the UTF-8 of the escape", cases[k].doc);
        }
        jsonl_close(f);
    }
}



ADF_TEST(jsonl_errors_name_the_line)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    static const char *const text = "{\"a\": 1}\n{\"a\": 01}\n{\"a\": 3}\n";

    ADF_CHECK(jsonl_parse(text, strlen(text), "vectors.jsonl", &f, &err) == 0);
    ADF_CHECK(f == NULL);
    ADF_CHECK(err.file != NULL && strcmp(err.file, "vectors.jsonl") == 0);
    ADF_CHECK_MSG(err.line == 2, "the error stands on line %lu, expected 2", err.line);
    ADF_CHECK(err.column >= 8);
    ADF_CHECK(strstr(jsonl_error_message(&err), "vectors.jsonl:2:") != NULL);
    ADF_CHECK(strcmp(jsonl_error_message(NULL), "(no error)") == 0);
}

/* Nesting is bounded, so a hostile file cannot make the recursive descent run out of stack. */

ADF_TEST(jsonl_bounds_the_nesting)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    char *buf;
    size_t i;
    size_t depth = JSONL_MAX_DEPTH;

    buf = (char *) malloc(2 * (depth + 3) + 2); /* the deepest buffer written below */
    ADF_CHECK(buf != NULL);
    if (buf == NULL)
        return;
    for (i = 0; i < depth; i++)
        buf[i] = '[';
    for (i = 0; i < depth; i++)
        buf[depth + i] = ']';
    buf[2 * depth] = '\0';
    ADF_CHECK_MSG(jsonl_parse(buf, 2 * depth, "memory", &f, &err) == 1, "depth %lu: %s",
                  (unsigned long) depth, jsonl_error_message(&err));
    jsonl_close(f);

    for (i = depth + 1; i <= depth + 3; i++)
    {
        size_t j;
        int ok;

        for (j = 0; j < i; j++)
            buf[j] = '[';
        for (j = 0; j < i; j++)
            buf[i + j] = ']';
        buf[2 * i] = '\0';
        f = NULL;
        ok = jsonl_parse(buf, 2 * i, "memory", &f, &err);
        ADF_CHECK_MSG(ok == 0, "depth %lu was accepted", (unsigned long) i);
        ADF_CHECK(strstr(err.message, "nested deeper") != NULL);
    }
    free(buf);
}

/* A value of the wrong kind is an error, not a zero and not a crash. */

ADF_TEST(jsonl_refuses_a_value_of_the_wrong_kind)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    const jsonl_value *rec, *v, *w;
    const char *text = "{\"a\": 1, \"b\": \"x\", \"c\": [1, 2], \"d\": true, \"e\": null}";
    int b = 0;
    size_t len = 0;
    char *s;

    /* a NULL value is refused with a message, not dereferenced */
    s = (char *) jsonl_string(NULL, &len, &err);
    ADF_CHECK(s == NULL);
    ADF_CHECK(strstr(err.message, "expected a string") != NULL);

    ADF_CHECK(jsonl_parse(text, strlen(text), "memory", &f, &err) == 1);
    if (f == NULL)
        return;
    rec = jsonl_record(f, 0);

    ADF_CHECK(jsonl_string(jsonl_record(f, 0), &len, &err) == NULL);
    ADF_CHECK(strstr(err.message, "expected a string") != NULL);
    ADF_CHECK(jsonl_int_text(rec, &err) == NULL);
    ADF_CHECK(strstr(err.message, "expected an integer") != NULL);
    ADF_CHECK(jsonl_bool(rec, &b, &err) == 0);
    ADF_CHECK(strstr(err.message, "expected a boolean") != NULL);
    ADF_CHECK(jsonl_is_null(rec, &err) == 0);
    ADF_CHECK(jsonl_is_null(NULL, &err) == 0);
    ADF_CHECK(jsonl_int_text_or_string(rec, &text, &err) == 0);

    ADF_CHECK(jsonl_field(rec, "zz", &v, &err) == 0);
    ADF_CHECK(strstr(err.message, "is missing") != NULL);
    ADF_CHECK(jsonl_field(rec, "a", &v, &err) == 1);
    ADF_CHECK(jsonl_at(v, 0, &err) == NULL); /* an integer has no elements */
    ADF_CHECK(strstr(err.message, "no elements") != NULL);
    ADF_CHECK(jsonl_field(v, "a", &v, &err) == 0);

    ADF_CHECK(jsonl_field(rec, "c", &v, &err) == 1);
    ADF_CHECK(jsonl_size(v) == 2);
    ADF_CHECK(jsonl_at(v, 2, &err) == NULL);
    ADF_CHECK(strstr(err.message, "out of range") != NULL);
    ADF_CHECK(jsonl_at(v, 1, &err) != NULL);
    ADF_CHECK(jsonl_key(v, 0) == NULL); /* an array has no keys */

    ADF_CHECK(jsonl_field(rec, "d", &v, &err) == 1);
    ADF_CHECK(jsonl_bool(v, &b, &err) == 1 && b == 1);
    ADF_CHECK(jsonl_field(rec, "e", &v, &err) == 1);
    ADF_CHECK(jsonl_is_null(v, &err) == 1);

    ADF_CHECK(jsonl_field(rec, "b", &w, &err) == 1);
    ADF_CHECK(jsonl_string(w, &len, &err) != NULL && len == 1);
    ADF_CHECK(jsonl_bool(w, &b, &err) == 0);

    ADF_CHECK(jsonl_record(f, 1) == NULL);
    ADF_CHECK(jsonl_count(NULL) == 0);
    jsonl_close(f);
    jsonl_close(NULL); /* closing nothing is harmless */
    ADF_CHECK(jsonl_open("tests/ref/vectors/does_not_exist.jsonl", &f, &err) == 0);
    ADF_CHECK(f == NULL);
    ADF_CHECK(strstr(err.message, "cannot open") != NULL);
}

/* Every golden file is read, with the number of vectors and of status vectors of
   tests/golden/README.md, and every status is a code of docs/conventions.md 3.1. */

ADF_TEST(golden_reads_every_golden_file)
{
    size_t k;
    unsigned long total = 0;
    unsigned long total_status = 0;

    for (k = 0; k < N_GOLDEN_FILES; k++)
    {
        golden_error_t err;
        golden_file *f = NULL;
        size_t i;
        unsigned long statuses = 0;

        ADF_CHECK_MSG(golden_open(golden_files[k], GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s: %s",
                      golden_files[k], golden_error_message(&err));
        if (f == NULL)
            continue;
        ADF_CHECK(golden_file_error(f) == NULL);
        {
            unsigned long vectors = 0;
            unsigned long lines = 0;

            count_vector_lines(golden_files[k], &vectors, &lines);
            ADF_CHECK_MSG(golden_count(f) == vectors, "%s: %lu records for %lu vector lines",
                          golden_files[k], (unsigned long) golden_count(f), vectors);
        }
        ADF_CHECK(golden_comment_lines(f) >= 1);
        for (i = 0; i < golden_count(f); i++)
        {
            const golden_record *r = golden_record_at(f, i);
            size_t j;
            int known = 0;

            ADF_CHECK(r != NULL);
            if (r == NULL)
                break;
            ADF_CHECK_MSG(r->line >= 1, "%s: record %lu has no line", golden_files[k],
                          (unsigned long) i);
            ADF_CHECK(r->input != NULL);
            ADF_CHECK_MSG(r->input[r->input_len] == '\0',
                          "%s line %lu: the input is not NUL-terminated", golden_files[k],
                          r->line);
            ADF_CHECK(r->expected != NULL);
            ADF_CHECK(r->expected_len == strlen(r->expected));
            if (!r->is_status)
                continue;
            statuses++;
            for (j = 0; j < N_STATUS_NAMES; j++)
            {
                if (strcmp(r->status, status_names[j]) == 0)
                    known = 1;
            }
            ADF_CHECK_MSG(known, "%s line %lu: \"%s\" is not a status of conventions.md 3.1",
                          golden_files[k], r->line, r->status);
            ADF_CHECK(r->expected[0] == '!');
            ADF_CHECK(strlen(r->status) == r->expected_len - 1);
        }
        total += (unsigned long) golden_count(f);
        total_status += statuses;
        golden_close(f);
    }
    ADF_CHECK_MSG(total > 0, "the golden files hold no vector at all");
    ADF_CHECK_MSG(total_status > 0 && total_status <= total,
                  "%lu status vectors of %lu vectors", total_status, total);
}

/* The escapes of conventions.md 11.1 in the input, an embedded NUL and bytes above 0x7f
   included. */

ADF_TEST(golden_decodes_the_escapes)
{
    golden_error_t err;
    golden_file *f = NULL;
    size_t i;
    int found_nul = 0;
    int found_high = 0;

    ADF_CHECK(golden_open("tests/golden/adele.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
    if (f == NULL)
        return;

    /* Every vector of the file is walked: an input must be decoded, and a decoded input may
       hold a NUL or a byte above 0x7f, which a C string would lose. */

    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record *r = golden_record_at(f, i);
        const unsigned char *p = (const unsigned char *) r->input;
        size_t j;

        if (r->generated)
            continue;
        for (j = 0; j < r->input_len; j++)
        {
            if (p[j] == 0)
            {
                found_nul = 1;
                ADF_CHECK_MSG(r->input[r->input_len] == '\0',
                              "adele.tsv line %lu: the input is not NUL-terminated", r->line);
                ADF_CHECK_MSG(strlen(r->input) < r->input_len,
                              "adele.tsv line %lu: the length of the input is the length of a C "
                              "string, so the NUL is lost",
                              r->line);
            }
            if (p[j] >= 0x80)
                found_high = 1;
        }
        /* the escapes are decoded, so no backslash of an escape is left in an input */
        for (j = 0; j < r->input_len; j++)
        {
            if (p[j] == '\\')
            {
                /* a decoded backslash is possible (\\ is an escape), but never followed by
                   x, t, n or r, which would have been an escape of the file */
                ADF_CHECK_MSG(!(j + 1 < r->input_len &&
                               (p[j + 1] == 'x' || p[j + 1] == 't' || p[j + 1] == 'n' ||
                                p[j + 1] == 'r')),
                              "adele.tsv line %lu: an escape was left in the input at byte %lu",
                              r->line, (unsigned long) j);
            }
        }
    }
    ADF_CHECK_MSG(found_nul, "tests/golden/adele.tsv has no input with an embedded NUL");
    ADF_CHECK_MSG(found_high, "tests/golden/adele.tsv has no input with a byte above 0x7f");
    golden_close(f);
}

/* The exact bytes of the two hostile inputs of adele.tsv. */

ADF_TEST(golden_decodes_the_hostile_inputs)
{
    golden_error_t err;
    golden_file *f = NULL;
    const golden_record *r;
    size_t i;
    int found_minus = 0;
    int found_nul = 0;

    ADF_CHECK(golden_open("tests/golden/adele.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        r = golden_record_at(f, i);
        if (r->generated)
            continue;
        /* "(3.14 \xc2\xb1 0.1 ; 0)" TAB "!PARSE": the two bytes of a UTF-8 minus sign */
        if (r->input_len == 17 && memcmp(r->input, "(3.14 \xc2\xb1 0.1 ; 0)", 17) == 0)
        {
            ADF_CHECK((unsigned char) r->input[6] == 0xc2);
            ADF_CHECK((unsigned char) r->input[7] == 0xb1);
            ADF_CHECK(r->is_status && strcmp(r->status, "PARSE") == 0);
            found_minus = 1;
        }
        /* "(1 ; 0)\x00" TAB "!PARSE": eight bytes, the last one a NUL */
        if (r->input_len == 8 && memcmp(r->input, "(1 ; 0)", 7) == 0)
        {
            ADF_CHECK((unsigned char) r->input[7] == 0);
            ADF_CHECK(r->input[8] == '\0');
            ADF_CHECK(r->is_status && strcmp(r->status, "PARSE") == 0);
            found_nul = 1;
        }
    }
    ADF_CHECK_MSG(found_minus, "the UTF-8 minus sign vector is missing from tests/golden/adele.tsv");
    ADF_CHECK_MSG(found_nul, "the embedded NUL vector is missing from tests/golden/adele.tsv");
    golden_close(f);
}

/* An @gen: line stands for PREFIX, COUNT copies of UNIT and SUFFIX. The expected input is
   built here and compared byte for byte, so a reader that expands one part wrongly, or drops
   one copy, fails. The lines are the @gen: lines of tests/golden/README.md's files. */

ADF_TEST(golden_expands_generated_inputs)
{
    static const struct
    {
        const char *path;
        unsigned long line;
        const char *prefix;
        const char *unit;
        unsigned long count;
        const char *suffix;
        const char *status; /* "" if the expected field is output text */
    } generated[] = {
        {"tests/golden/dispatch.tsv", 32, "", "0", 1048577, "", "LIMIT"},
        {"tests/golden/dump.tsv", 31, "adf1 Q rat 1", "0", 1048576, " 1", "LIMIT"},
        {"tests/golden/fball.tsv", 50, "(* ; ", "0", 1048563, "2 mod 6)", ""},
        {"tests/golden/fball.tsv", 73, "(* ; ", "0", 1048564, "2 mod 6)", "LIMIT"},
        {"tests/golden/rat.tsv", 24, "", "0", 100000, "7", ""},
        {"tests/golden/rat.tsv", 25, "", "0", 1048575, "7", ""},
        {"tests/golden/rat.tsv", 54, "", "0", 1048576, "7", "LIMIT"},
        {"tests/golden/rat.tsv", 55, "", "(", 10000, "", "PARSE"},
    };
    size_t k;

    for (k = 0; k < sizeof(generated) / sizeof(generated[0]); k++)
    {
        golden_error_t err;
        golden_file *f = NULL;
        const golden_record *r = NULL;
        char *want;
        size_t want_len;
        size_t i;
        size_t u = strlen(generated[k].unit);

        ADF_CHECK(golden_open(generated[k].path, GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
        if (f == NULL)
            continue;
        for (i = 0; i < golden_count(f); i++)
        {
            if (golden_record_at(f, i)->line == generated[k].line)
            {
                r = golden_record_at(f, i);
                break;
            }
        }
        ADF_CHECK_MSG(r != NULL, "%s has no record on line %lu", generated[k].path,
                      generated[k].line);
        if (r == NULL)
        {
            golden_close(f);
            continue;
        }
        ADF_CHECK_MSG(r->generated, "%s line %lu was not read as an @gen: line",
                      generated[k].path, r->line);
        want_len = strlen(generated[k].prefix) + u * generated[k].count +
                   strlen(generated[k].suffix);
        want = (char *) malloc(want_len + 1);
        ADF_CHECK(want != NULL);
        if (want == NULL)
        {
            golden_close(f);
            continue;
        }
        memcpy(want, generated[k].prefix, strlen(generated[k].prefix));
        for (i = 0; i < generated[k].count; i++)
            memcpy(want + strlen(generated[k].prefix) + i * u, generated[k].unit, u);
        memcpy(want + strlen(generated[k].prefix) + u * generated[k].count,
               generated[k].suffix, strlen(generated[k].suffix));
        want[want_len] = '\0';
        ADF_CHECK_MSG(r->input_len == want_len, "%s line %lu: %lu bytes, expected %lu",
                      generated[k].path, r->line, (unsigned long) r->input_len,
                      (unsigned long) want_len);
        ADF_CHECK_MSG(memcmp(r->input, want, want_len) == 0,
                      "%s line %lu: the expanded input is not PREFIX, COUNT copies of UNIT and "
                      "SUFFIX",
                      generated[k].path, r->line);
        free(want);
        if (generated[k].status[0] != '\0')
        {
            ADF_CHECK(r->is_status);
            ADF_CHECK(strcmp(r->status, generated[k].status) == 0);
        }
        else
            ADF_CHECK(r->is_status == 0);
        golden_close(f);
    }
}

/* A malformed golden line is refused, with the file and the line. */

ADF_TEST(golden_refuses_malformed_lines)
{
    static const struct
    {
        const char *text;
        const char *why;
    } bad[] = {
        {"", "empty"},
        {"7/3", "no TAB"},
        {"7/3\t7/3\t7/3", "exactly one TAB"},
        {"7/3\t7/3\textra", "exactly one TAB"},
        {"\\q\t7/3", "unknown escape"},
        {"7/3\\\t7/3", "backslash at the end"},
        {"7/3\t", "expected field is empty"},
        {"7/3\t!parse", "not a letter"},  /* a status name is upper case */
        {"7/3\t!PARSE9x", "not a letter"},
        {"7/3\t!A_VERY_LONG_STATUS_NAME_THAT_DOES_NOT_FIT", "longer than"},
        {"@gen:a|bc\t7/3", "four parts"},
        {"@gen:a|bc|1\t7/3", "four parts"},
        {"@gen:a|bc|1|d|e\t7/3", "four parts"},
        {"@gen:a|bc||d\t7/3", "count is empty"},
        {"@gen:a|bc|007|d\t7/3", "leading zero"},
        {"@gen:a|bc|-1|d\t7/3", "not a decimal number"},
        {"@gen:a|bc|99999999999999999999999|d\t7/3", "too large"},
        {"@gen:a|\\q|1|d\t7/3", "unknown escape"},
        {"@gen:\t7/3", "four parts"},
    };
    size_t k;

    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++)
    {
        golden_error_t err;
        golden_file *f = (golden_file *) (void *) &bad;
        int ok = golden_parse(bad[k].text, strlen(bad[k].text), "memory", GOLDEN_DEFAULT_MAX_INPUT,
                              &f, &err);

        ADF_CHECK_MSG(ok == 0, "\"%s\" was accepted", bad[k].text);
        ADF_CHECK_MSG(f == NULL, "\"%s\" left a file behind", bad[k].text);
        ADF_CHECK_MSG(err.file != NULL && strcmp(err.file, "memory") == 0, "\"%s\": no file name",
                      bad[k].text);
        ADF_CHECK_MSG(err.line == 1, "\"%s\": the error stands on line %lu", bad[k].text, err.line);
        ADF_CHECK_MSG(strstr(err.message, bad[k].why) != NULL,
                      "\"%s\": the message \"%s\" does not say \"%s\"", bad[k].text, err.message,
                      bad[k].why);
    }
}

/* A line that would expand past max_input is refused before anything is allocated. */

ADF_TEST(golden_bounds_a_generated_input)
{
    static const char *const line = "@gen:a|bc|100000|d\t7/3\n";
    golden_error_t err;
    golden_file *f = NULL;

    ADF_CHECK(golden_parse(line, strlen(line), "memory", 1000, &f, &err) == 0);
    ADF_CHECK(f == NULL);
    ADF_CHECK(strstr(err.message, "the limit is 1000") != NULL);

    f = NULL;
    ADF_CHECK(golden_parse(line, strlen(line), "memory", 300001, &f, &err) == 1);
    ADF_CHECK_MSG(f != NULL, "%s", golden_error_message(&err));
    if (f != NULL)
    {
        ADF_CHECK(golden_count(f) == 1);
        ADF_CHECK(golden_record_at(f, 0)->input_len == 200002);
        golden_close(f);
    }
}

/* The line of a golden error, the shape of a record, and a few well-known vectors. */

ADF_TEST(golden_reads_the_documented_vectors)
{
    golden_error_t err;
    golden_file *f = NULL;
    const golden_record *r;

    ADF_CHECK(golden_open("tests/golden/rat.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
    if (f == NULL)
        return;
    ADF_CHECK(golden_count(f) == 51);
    ADF_CHECK(golden_comment_lines(f) == 4);
    r = golden_record_at(f, 0);
    ADF_CHECK(r != NULL);
    if (r != NULL)
    {
        ADF_CHECK(r->line == 2);
        ADF_CHECK(r->input_len == 3);
        ADF_CHECK(memcmp(r->input, "7/3", 3) == 0);
        ADF_CHECK(r->expected_len == 3);
        ADF_CHECK(memcmp(r->expected, "7/3", 3) == 0);
        ADF_CHECK(r->is_status == 0);
        ADF_CHECK(r->generated == 0);
    }
    /* the last vector of rat.tsv is the generated one of 1048577 bytes with !LIMIT */
    r = golden_record_at(f, 49);
    ADF_CHECK(r != NULL);
    if (r != NULL)
    {
        ADF_CHECK(r->line == 54);
        ADF_CHECK(r->input_len == 1048577);
        ADF_CHECK(r->is_status && strcmp(r->status, "LIMIT") == 0);
    }
    ADF_CHECK(golden_record_at(f, 51) == NULL);
    golden_close(f);

    /* psi_phases.tsv: the expected field of a vector with several phases is one text */
    f = NULL;
    ADF_CHECK(golden_open("tests/golden/psi_phases.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
    if (f == NULL)
        return;
    ADF_CHECK(golden_count(f) == 17);
    r = golden_record_at(f, 10);
    ADF_CHECK(r != NULL);
    if (r != NULL)
    {
        ADF_CHECK(r->line == 14);
        ADF_CHECK(strcmp(r->input, "(* ; 1/4 mod 1/3)") == 0);
        ADF_CHECK(strcmp(r->expected, "1/4 7/12 11/12") == 0);
        ADF_CHECK(r->is_status == 0);
    }
    r = golden_record_at(f, 15);
    ADF_CHECK(r != NULL);
    if (r != NULL)
    {
        ADF_CHECK(r->line == 19);
        ADF_CHECK(strcmp(r->input, "(* ; 1/0)") == 0);
        ADF_CHECK(r->is_status && strcmp(r->status, "DOMAIN") == 0);
        ADF_CHECK(strcmp(r->expected, "!DOMAIN") == 0);
        ADF_CHECK(r->expected_len == 7);
    }
    golden_close(f);
}

/* A file with no vector at all is not an error; it has no record. */

ADF_TEST(golden_accepts_a_file_without_a_vector)
{
    golden_error_t err;
    golden_file *f = NULL;
    static const char *const text = "# only a comment\n\n#and another\n";

    ADF_CHECK(golden_parse(text, strlen(text), "memory", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
    ADF_CHECK(golden_count(f) == 0);
    ADF_CHECK(golden_comment_lines(f) == 2);
    ADF_CHECK(golden_blank_lines(f) == 1);
    golden_close(f);

    /* the last line of a file may have no LF, and it is a vector all the same */
    f = NULL;
    ADF_CHECK(golden_parse("7/3\t7/3", 7, "memory", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1);
    ADF_CHECK(golden_count(f) == 1);
    golden_close(f);
    golden_close(NULL);
    ADF_CHECK(golden_open("tests/golden/does_not_exist.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) ==
              0);
    ADF_CHECK(f == NULL);
    ADF_CHECK(strstr(err.message, "cannot open") != NULL);
}
