/* tests/test_dump_units.c: the dump form (docs/conventions.md 10) of adf_ucoset, adf_idele and
   adf_idclass, the three types of the unit coset, the idele and the idele class (include/adelefeld/
   dump.h, bodies "ucoset", "idele", "idclass" of conventions 10.1; predicates of conventions 5.6 and
   5.7; rules of 10.2: strict, real balls as arb_dump_str writes them, CV-52, no context occurrence;
   order of checks 8.5; limits 8.4).

   What is checked:

   1. Every row of tests/golden/dump.tsv whose body is "ucoset", "idele" or "idclass" (24 rows: 7
      valid, 17 with a status; counted in Python from the file with grep -c). A valid row is loaded
      and dumped back byte for byte; a row with a status gives that status from the loader and from
      the inspector, leaves the output value untouched and does not write *nctx.
   2. Round trips on 2000 random canonical values of each type, built through the public
      constructors (adf_ucoset_set_fmpz2, adf_idele_set_parts, adf_idclass_set_parts) with integers
      of up to 2000 bits: load(dump(v)) is identical to v, and dump(load(t)) is t byte for byte.
   3. Strictness: for every field a well formed text that is not canonical is ADF_DOMAIN; the
      grammatical defects (a missing or an extra token, an upper-case hexadecimal digit, a leading
      zero, two spaces, a trailing space, a NUL byte) are ADF_PARSE; the version "adf2" and the
      field "K" are ADF_UNSUPPORTED; a text longer than max_len is ADF_LIMIT; and the order of
      8.5 is pinned with texts that break two rules at once.
   4. adf_x_dump_inspect returns the status of the loader for every text above and writes *nctx = 0
      on ADF_OK only.

   The expected texts and statuses are written from the grammar of conventions 10.1 and the
   predicates of conventions 5.6 and 5.7, not from the output of this library. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "test_runner.h"

/* ---- helpers ---- */

/* One strictness case: a text and the status the loader must give it. */
typedef struct
{
    const char * text;
    int status;
} tcase;

/* The body keyword of a text: its third space-separated token ("adf1 Q ucoset ..."). 0 for
   another body. */
static int
body_is(const char * t, size_t len, const char * kw)
{
    size_t i = 0, sp = 0, n = strlen(kw);

    while (i < len && sp < 2)
        if (t[i++] == ' ')
            sp++;
    return sp == 2 && i + n <= len && memcmp(t + i, kw, n) == 0 && (i + n == len || t[i + n] == ' ');
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

/* 1 if the text is made of bytes 0x20 to 0x7e with single spaces and no leading or trailing space
   (conventions 8.2 and 10.1). */
static int
dump_well_formed(const char * t, size_t len)
{
    size_t i;

    if (len == 0 || t[0] == ' ' || t[len - 1] == ' ')
        return 0;
    for (i = 0; i < len; i++)
    {
        if ((unsigned char) t[i] < 0x20 || (unsigned char) t[i] > 0x7e)
            return 0;
        if (t[i] == ' ' && i + 1 < len && t[i + 1] == ' ')
            return 0;
    }
    return 1;
}

/* The random values of section 2. */

/* A random canonical unit coset (conventions 5.6): with probability 1 in 4 an exact unit, else a
   modulus of up to `bits` bits and a residue in 1..N prime to it. adf_ucoset_set_fmpz2 stores the
   modulus as supplied (CV-17), so the round trip tests that form too. */
static void
random_ucoset(adf_ucoset_t x, flint_rand_t st, flint_bitcnt_t bits)
{
    fmpz_t c, N, g;
    int r;

    fmpz_init(c);
    fmpz_init(N);
    fmpz_init(g);
    if (n_randint(st, 4) == 0)
    {
        fmpz_set_si(N, 0);
        fmpz_set_si(c, (n_randint(st, 2) ? 1 : -1));
    }
    else
    {
        do
        {
            fmpz_randtest_not_zero(N, st, bits);
            fmpz_abs(N, N);
        } while (fmpz_is_zero(N));
        fmpz_randtest_unsigned(c, st, bits);
        fmpz_fdiv_r(c, c, N);   /* c in 0..N-1 */
        fmpz_add_ui(c, c, 1);          /* c in 1..N */
        fmpz_gcd(g, c, N);
        while (!fmpz_is_one(g))        /* a unit exists in 1..N for every N >= 1 */
        {
            fmpz_randtest_unsigned(c, st, bits);
            fmpz_fdiv_r(c, c, N);   /* c in 0..N-1 */
            fmpz_add_ui(c, c, 1);
            fmpz_gcd(g, c, N);
        }
    }
    r = adf_ucoset_set_fmpz2(x, c, N);
    ADF_CHECK_MSG(r == ADF_OK && adf_ucoset_is_canonical(x), "set_fmpz2 %s on c=%s N=%s",
                  adf_status_str(r), "random", "random");
    fmpz_clear(c);
    fmpz_clear(N);
    fmpz_clear(g);
}

/* A random finite real ball (conventions 10.2: an odd midpoint mantissa, an odd radius mantissa
   below 2^30, either sign of exponent; the special values are not canonical and are not built). */
static void
random_arb(arb_t x, flint_rand_t st, flint_bitcnt_t bits)
{
    static const ulong rads[] = {0, 1, 3, (UWORD(1) << 29) + 1, (UWORD(1) << 30) - 1, 12345};
    fmpz_t m, e;

    fmpz_init(m);
    fmpz_init(e);
    fmpz_randtest_not_zero(m, st, bits);
    fmpz_randtest(e, st, 90);
    arf_set_fmpz_2exp(arb_midref(x), m, e);
    {
        ulong rm = rads[n_randint(st, sizeof(rads) / sizeof(rads[0]))];
        if (rm == 0)
            mag_zero(arb_radref(x));
        else
        {
            fmpz_randtest(e, st, 80);
            mag_set_ui_2exp_si(arb_radref(x), rm, 0);
            mag_mul_2exp_fmpz(arb_radref(x), arb_radref(x), e);
        }
    }
    fmpz_clear(m);
    fmpz_clear(e);
}

/* A random real ball that excludes 0 (the predicate of 5.7 for the inf of an idele) and a random
   one every point of which is positive (the predicate of 5.7 for the t of a class). */
static void
random_arb_nonzero(arb_t x, flint_rand_t st, flint_bitcnt_t bits, int positive)
{
    do
    {
        random_arb(x, st, bits);
    } while (!arb_is_finite(x) || (positive ? !arb_is_positive(x) : !arb_is_nonzero(x)));
}

/* ---- the sentinel values: a fixed value of each type, and the identity of the type ---- */

static void
ucoset_sentinel(adf_ucoset_t x)
{
    fmpz_t c, N;

    fmpz_init_set_ui(c, 5);
    fmpz_init_set_ui(N, 6);
    ADF_CHECK(adf_ucoset_set_fmpz2(x, c, N) == ADF_OK);
    fmpz_clear(c);
    fmpz_clear(N);
}

static void
idele_sentinel(adf_idele_t x)
{
    adf_ucoset_t u;
    arb_t a;
    fmpq_t r;

    adf_ucoset_init(u);
    arb_init(a);
    fmpq_init(r);
    arb_set_si(a, -3);
    fmpq_set_si(r, 3, 7);
    ucoset_sentinel(u);
    ADF_CHECK(adf_idele_set_parts(x, a, r, u) == ADF_OK);
    adf_ucoset_clear(u);
    arb_clear(a);
    fmpq_clear(r);
}

static void
idclass_sentinel(adf_idclass_t x)
{
    adf_ucoset_t u;
    arb_t a;

    adf_ucoset_init(u);
    arb_init(a);
    arb_set_si(a, 5);
    ucoset_sentinel(u);
    ADF_CHECK(adf_idclass_set_parts(x, a, u) == ADF_OK);
    adf_ucoset_clear(u);
    arb_clear(a);
}

/* Every case of one table: the loader gives the status, the value is untouched unless the status is
   ADF_OK, the inspector gives the same status and writes *nctx only on ADF_OK, and on ADF_OK the
   dump of the loaded value is the text of the case. */
#define TYPED_RUN(TY, IDENT, INIT, SET, CLEAR, LOAD, INSP, DMP, tc, n)                                  \
    do                                                                                                  \
    {                                                                                                   \
        size_t _i;                                                                                      \
        for (_i = 0; _i < (n); _i++)                                                                     \
        {                                                                                               \
            TY x, keep;                                                                                 \
            size_t len = strlen((tc)[_i].text), nc = 7, blen = 0;                                        \
            int st, ist;                                                                                \
            char * back = NULL;                                                                          \
            INIT(x);                                                                                     \
            INIT(keep);                                                                                  \
            SET(x);                                                                                      \
            SET(keep);                                                                                   \
            st = LOAD(x, (tc)[_i].text, len, NULL, NULL);                                                \
            ADF_CHECK_MSG(st == (tc)[_i].status, "load \"%s\": %s, expected %s", (tc)[_i].text,         \
                          adf_status_str(st), adf_status_str((tc)[_i].status));                          \
            if (st != ADF_OK)                                                                            \
                ADF_CHECK_MSG(IDENT(x, keep), "load \"%s\" touched the output", (tc)[_i].text);           \
            ist = INSP(&nc, NULL, (tc)[_i].text, len, NULL);                                             \
            ADF_CHECK_MSG(ist == st, "inspect \"%s\": %s, loader %s", (tc)[_i].text, adf_status_str(ist), \
                          adf_status_str(st));                                                          \
            if (ist == ADF_OK)                                                                           \
                ADF_CHECK_MSG(nc == 0, "inspect \"%s\" wrote nctx = %zu", (tc)[_i].text, nc);             \
            else                                                                                         \
                ADF_CHECK_MSG(nc == 7, "inspect \"%s\" wrote nctx on %s", (tc)[_i].text,                  \
                              adf_status_str(ist));                                                      \
            if (st == ADF_OK)                                                                            \
            {                                                                                           \
                back = DMP(&blen, x);                                                                    \
                ADF_CHECK_MSG(back != NULL && blen == len && memcmp(back, (tc)[_i].text, len) == 0,       \
                              "dump of \"%s\" is \"%s\"", (tc)[_i].text, back);                          \
                adf_str_free(back);                                                                      \
            }                                                                                           \
            CLEAR(x);                                                                                   \
            CLEAR(keep);                                                                                \
        }                                                                                               \
    }                                                                                                   \
    while (0)

/* ------------------------------------------------------------------ the rows of tests/golden */

/* One row of tests/golden/dump.tsv with a body of this file. */
static void
golden_row_ucoset(const golden_record * r, int want, size_t * nvalid)
{
    adf_ucoset_t x, keep;
    size_t len = r->input_len, nc = 7;
    int st, ist;

    adf_ucoset_init(x);
    adf_ucoset_init(keep);
    ucoset_sentinel(x);
    ucoset_sentinel(keep);
    st = adf_ucoset_load_str(x, r->input, len, NULL, NULL);
    ADF_CHECK_MSG(st == want, "line %lu \"%s\": load %s, expected %s", r->line, r->input, adf_status_str(st),
                  adf_status_str(want));
    if (st != ADF_OK)
        ADF_CHECK(adf_ucoset_identical(x, keep));
    else
    {
        size_t dl = 0;
        char * back = adf_ucoset_dump_str(&dl, x);
        ADF_CHECK_MSG(back != NULL && dl == r->expected_len && memcmp(back, r->expected, dl) == 0,
                      "line %lu: dumped \"%s\", expected \"%s\"", r->line, back, r->expected);
        adf_str_free(back);
        (*nvalid)++;
    }
    ist = adf_ucoset_dump_inspect(&nc, NULL, r->input, len, NULL);
    ADF_CHECK(ist == want && (want == ADF_OK ? nc == 0 : nc == 7));
    adf_ucoset_clear(x);
    adf_ucoset_clear(keep);
}

static void
golden_row_idele(const golden_record * r, int want, size_t * nvalid)
{
    adf_idele_t x, keep;
    size_t len = r->input_len, nc = 7;
    int st, ist;

    adf_idele_init(x);
    adf_idele_init(keep);
    idele_sentinel(x);
    idele_sentinel(keep);
    st = adf_idele_load_str(x, r->input, len, NULL, NULL);
    ADF_CHECK_MSG(st == want, "line %lu \"%s\": load %s, expected %s", r->line, r->input, adf_status_str(st),
                  adf_status_str(want));
    if (st != ADF_OK)
        ADF_CHECK(adf_idele_identical(x, keep));
    else
    {
        size_t dl = 0;
        char * back = adf_idele_dump_str(&dl, x);
        ADF_CHECK_MSG(back != NULL && dl == r->expected_len && memcmp(back, r->expected, dl) == 0,
                      "line %lu: dumped \"%s\", expected \"%s\"", r->line, back, r->expected);
        adf_str_free(back);
        (*nvalid)++;
    }
    ist = adf_idele_dump_inspect(&nc, NULL, r->input, len, NULL);
    ADF_CHECK(ist == want && (want == ADF_OK ? nc == 0 : nc == 7));
    adf_idele_clear(x);
    adf_idele_clear(keep);
}

static void
golden_row_idclass(const golden_record * r, int want, size_t * nvalid)
{
    adf_idclass_t x, keep;
    size_t len = r->input_len, nc = 7;
    int st, ist;

    adf_idclass_init(x);
    adf_idclass_init(keep);
    idclass_sentinel(x);
    idclass_sentinel(keep);
    st = adf_idclass_load_str(x, r->input, len, NULL, NULL);
    ADF_CHECK_MSG(st == want, "line %lu \"%s\": load %s, expected %s", r->line, r->input, adf_status_str(st),
                  adf_status_str(want));
    if (st != ADF_OK)
        ADF_CHECK(adf_idclass_identical(x, keep));
    else
    {
        size_t dl = 0;
        char * back = adf_idclass_dump_str(&dl, x);
        ADF_CHECK_MSG(back != NULL && dl == r->expected_len && memcmp(back, r->expected, dl) == 0,
                      "line %lu: dumped \"%s\", expected \"%s\"", r->line, back, r->expected);
        adf_str_free(back);
        (*nvalid)++;
    }
    ist = adf_idclass_dump_inspect(&nc, NULL, r->input, len, NULL);
    ADF_CHECK(ist == want && (want == ADF_OK ? nc == 0 : nc == 7));
    adf_idclass_clear(x);
    adf_idclass_clear(keep);
}

ADF_TEST(golden_rows_of_dump_tsv)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, nrows = 0, nvalid = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/dump.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        int want = r->is_status ? status_of_name(r->status) : ADF_OK;

        ADF_CHECK(want >= 0);
        if (!r->is_status)
            ADF_CHECK_MSG(r->input_len == r->expected_len && memcmp(r->input, r->expected, r->input_len) == 0,
                          "line %lu is not its own expected output", r->line);
        if (body_is(r->input, r->input_len, "ucoset"))
        {
            golden_row_ucoset(r, want, &nvalid);
            nrows++;
        }
        else if (body_is(r->input, r->input_len, "idele"))
        {
            golden_row_idele(r, want, &nvalid);
            nrows++;
        }
        else if (body_is(r->input, r->input_len, "idclass"))
        {
            golden_row_idclass(r, want, &nvalid);
            nrows++;
        }
    }
    ADF_CHECK_MSG(nrows == 20 && nvalid == 7, "%zu rows, %zu valid", nrows, nvalid);
    golden_close(f);
}

/* ------------------------------------------------------------------ round trips */

ADF_TEST(ucoset_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 2000; i++)
    {
        adf_ucoset_t x, y;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 2000 : 1 + n_randint(st, 120);

        adf_ucoset_init(x);
        adf_ucoset_init(y);
        random_ucoset(x, st, bits);
        t = adf_ucoset_dump_str(&len, x);
        ADF_CHECK(t != NULL && dump_well_formed(t, len));
        ADF_CHECK(adf_ucoset_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK_MSG(adf_ucoset_identical(x, y), "load(dump(v)) is not v");
        t2 = adf_ucoset_dump_str(&len2, y);
        ADF_CHECK_MSG(len2 == len && memcmp(t2, t, len) == 0, "dump(load(t)) is not t");
        adf_str_free(t);
        adf_str_free(t2);
        adf_ucoset_clear(x);
        adf_ucoset_clear(y);
    }
    flint_randclear(st);
}

ADF_TEST(idele_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 2000; i++)
    {
        adf_idele_t x, y;
        adf_ucoset_t u;
        arb_t a;
        fmpq_t r;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 2000 : 1 + n_randint(st, 120);

        adf_idele_init(x);
        adf_idele_init(y);
        adf_ucoset_init(u);
        arb_init(a);
        fmpq_init(r);
        random_arb_nonzero(a, st, bits, 0);
        fmpq_randtest_not_zero(r, st, bits);
        fmpq_abs(r, r);                  /* the content is positive (5.7) */
        random_ucoset(u, st, bits);
        ADF_CHECK(adf_idele_set_parts(x, a, r, u) == ADF_OK && adf_idele_is_canonical(x));
        t = adf_idele_dump_str(&len, x);
        ADF_CHECK(t != NULL && dump_well_formed(t, len));
        ADF_CHECK(adf_idele_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK_MSG(adf_idele_identical(x, y), "load(dump(v)) is not v");
        t2 = adf_idele_dump_str(&len2, y);
        ADF_CHECK_MSG(len2 == len && memcmp(t2, t, len) == 0, "dump(load(t)) is not t");
        adf_str_free(t);
        adf_str_free(t2);
        adf_idele_clear(x);
        adf_idele_clear(y);
        adf_ucoset_clear(u);
        arb_clear(a);
        fmpq_clear(r);
    }
    flint_randclear(st);
}

ADF_TEST(idclass_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 2000; i++)
    {
        adf_idclass_t x, y;
        adf_ucoset_t u;
        arb_t a;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 2000 : 1 + n_randint(st, 120);

        adf_idclass_init(x);
        adf_idclass_init(y);
        adf_ucoset_init(u);
        arb_init(a);
        random_arb_nonzero(a, st, bits, 1);
        random_ucoset(u, st, bits);
        ADF_CHECK(adf_idclass_set_parts(x, a, u) == ADF_OK && adf_idclass_is_canonical(x));
        t = adf_idclass_dump_str(&len, x);
        ADF_CHECK(t != NULL && dump_well_formed(t, len));
        ADF_CHECK(adf_idclass_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK_MSG(adf_idclass_identical(x, y), "load(dump(v)) is not v");
        t2 = adf_idclass_dump_str(&len2, y);
        ADF_CHECK_MSG(len2 == len && memcmp(t2, t, len) == 0, "dump(load(t)) is not t");
        adf_str_free(t);
        adf_str_free(t2);
        adf_idclass_clear(x);
        adf_idclass_clear(y);
        adf_ucoset_clear(u);
        arb_clear(a);
    }
    flint_randclear(st);
}

/* ------------------------------------------------------------------ strictness */

/* Body "ucoset c N". Each text is derived from the predicate of conventions 5.6:
   (N >= 1 and 1 <= c <= N and gcd(c, N) = 1) or (N = 0 and c in {1, -1}). */
static const tcase ucoset_cases[] = {
    /* canonical */
    {"adf1 Q ucoset 1 0", ADF_OK},
    {"adf1 Q ucoset -1 0", ADF_OK},
    {"adf1 Q ucoset 1 1", ADF_OK},
    {"adf1 Q ucoset 5 6", ADF_OK},                  /* the modulus as stored (CV-17) */
    {"adf1 Q ucoset 1 2", ADF_OK},
    {"adf1 Q ucoset 3 4", ADF_OK},
    {"adf1 Q ucoset ffffffffffffffffff 3", ADF_DOMAIN},      /* c = 2^64 - 1 is not below N = 3 */
    {"adf1 Q ucoset 3b ffffffffffffffff", ADF_OK},            /* 59 = 3 * 20 - 1 is prime to 2^64 - 1 */
    {"adf1 Q ucoset 1 ffffffffffffffffff", ADF_OK},   /* the whole unit group modulo a big prime */
    /* not canonical: the predicate of 5.6 */
    {"adf1 Q ucoset 0 1", ADF_DOMAIN},              /* c < 1 with N = 1 */
    {"adf1 Q ucoset 2 2", ADF_DOMAIN},              /* gcd(c, N) = 2 */
    {"adf1 Q ucoset 2 4", ADF_DOMAIN},              /* gcd = 2 */
    {"adf1 Q ucoset 6 4", ADF_DOMAIN},              /* c > N */
    {"adf1 Q ucoset 5 4", ADF_DOMAIN},              /* c > N */
    {"adf1 Q ucoset -1 6", ADF_DOMAIN},             /* c < 1 with N >= 1 */
    {"adf1 Q ucoset 1 -3", ADF_DOMAIN},             /* N < 0 */
    {"adf1 Q ucoset 2 0", ADF_DOMAIN},              /* an exact unit is 1 or -1 */
    {"adf1 Q ucoset 0 0", ADF_DOMAIN},
    {"adf1 Q ucoset ffffffffffffffffff 3", ADF_DOMAIN},       /* c > N */
    /* the grammar of 10.1 */
    {"adf1 Q ucoset 1", ADF_PARSE},                 /* a missing token */
    {"adf1 Q ucoset", ADF_PARSE},
    {"adf1 Q ucoset 1 6 1", ADF_PARSE},             /* an extra token */
    {"adf1 Q ucoset 1  6", ADF_PARSE},              /* two spaces */
    {"adf1 Q ucoset 1 6 ", ADF_PARSE},              /* a trailing space */
    {" adf1 Q ucoset 1 6", ADF_PARSE},              /* a leading space */
    {"adf1 Q ucoset 1 06", ADF_PARSE},              /* a leading zero */
    {"adf1 Q ucoset 01 6", ADF_PARSE},
    {"adf1 Q ucoset 1 0x6", ADF_PARSE},
    {"adf1 Q ucoset 1 A", ADF_PARSE},               /* an upper-case hexadecimal digit */
    {"adf1 Q ucoset 1 -0", ADF_PARSE},
    {"adf1 Q ucoset 1 6\n", ADF_PARSE},
    {"adf1 Q ucoset 1\t6", ADF_PARSE},
    {"adf1 Q ucoset 1 6 x", ADF_PARSE},
    /* the version and the field (10.2, "Version and field") */
    {"adf2 Q ucoset 1 6", ADF_UNSUPPORTED},
    {"adf1 K ucoset 1 6", ADF_UNSUPPORTED},
    {"adf1 A ucoset 1 6", ADF_UNSUPPORTED},        /* "A" is a field of the grammar (10.1) */
    {"adf1 @ ucoset 1 6", ADF_PARSE},              /* a field must start with an upper-case letter */
    {"adf0 Q ucoset 1 6", ADF_UNSUPPORTED},
    {"adf01 Q ucoset 1 6", ADF_PARSE}               /* a leading zero in the version is a grammar fault */
};

ADF_TEST(ucoset_strictness)
{
    TYPED_RUN(adf_ucoset_t, adf_ucoset_identical, adf_ucoset_init, ucoset_sentinel, adf_ucoset_clear,
              adf_ucoset_load_str, adf_ucoset_dump_inspect, adf_ucoset_dump_str, ucoset_cases,
              sizeof(ucoset_cases) / sizeof(ucoset_cases[0]));
}

/* Body "idele 1 <arb> num(r) den(r) c N". The arb is written as arb_dump_str writes it: the odd
   midpoint mantissa and exponent, then the radius mantissa and exponent. The predicate of 5.7 is
   the ball finite and excluding 0, r canonical and positive, and u as in 5.6. */
static const tcase idele_cases[] = {
    /* canonical */
    {"adf1 Q idele 1 1 0 0 0 3 2 5 6", ADF_OK},   /* 1 exact, r = 3/2, u = [5 mod 6] */
    {"adf1 Q idele 1 -1 0 0 0 3 2 5 6", ADF_OK},  /* the real ball excludes 0 on either sign */
    {"adf1 Q idele 1 1 -1 0 0 1 1 1 0", ADF_OK},  /* 1/2 exact */
    {"adf1 Q idele 1 3 -1 1 -2 3 1 1 0", ADF_OK}, /* [1.5 +- 0.5] excludes 0, r = 3 */
    {"adf1 Q idele 1 5 -1 1 -1e 3 2 5 24", ADF_OK},
    /* the archimedean count of Q (10.1) */
    {"adf1 Q idele 0 1 1 1 0", ADF_DOMAIN},                  /* no archimedean component */
    {"adf1 Q idele 2 1 0 0 0 1 0 0 0 1 1 1 0", ADF_DOMAIN},
    /* the real ball (10.2, 5.7) */
    {"adf1 Q idele 1 0 0 0 0 1 1 1 0", ADF_DOMAIN},   /* the ball 0 contains 0 */
    {"adf1 Q idele 1 1 0 1 0 1 1 1 0", ADF_DOMAIN},   /* [0 +- 1] contains 0 */
    {"adf1 Q idele 1 2 0 0 0 1 1 1 0", ADF_DOMAIN},   /* an even midpoint mantissa */
    {"adf1 Q idele 1 1 0 0 1 1 1 1 0", ADF_DOMAIN},   /* an even radius mantissa */
    {"adf1 Q idele 1 1 0 7fffffff 0 1 1 1 0", ADF_DOMAIN},   /* a radius mantissa of 31 bits */
    {"adf1 Q idele 1 1 0 40000001 0 1 1 1 0", ADF_DOMAIN},   /* 2^30 + 1 is not below 2^30 */
    {"adf1 Q idele 1 0 -1 0 0 1 1 1 0", ADF_DOMAIN},  /* +inf: not finite */
    {"adf1 Q idele 1 0 5 0 0 1 1 1 0", ADF_DOMAIN},   /* a zero mantissa with another exponent */
    {"adf1 Q idele 1 1 0 0 0 0 1 1 0", ADF_DOMAIN},   /* a zero radius with an exponent */
    /* the content r */
    {"adf1 Q idele 1 1 0 0 0 0 1 1 0", ADF_DOMAIN},   /* r = 0 */
    {"adf1 Q idele 1 1 0 0 0 -1 1 1 0", ADF_DOMAIN},  /* a negative denominator */
    {"adf1 Q idele 1 1 0 0 0 0 -1 1 0", ADF_DOMAIN},  /* r < 0 */
    {"adf1 Q idele 1 1 0 0 0 6 4 1 0", ADF_DOMAIN},   /* 6/4 is not in lowest terms */
    {"adf1 Q idele 1 1 0 0 0 4 2 1 0", ADF_DOMAIN},
    /* the unit coset (5.6) */
    {"adf1 Q idele 1 1 0 0 0 1 1 2 2", ADF_DOMAIN},
    {"adf1 Q idele 1 1 0 0 0 1 1 2 1", ADF_DOMAIN},   /* the unit [2 mod 1]: c > N */
    {"adf1 Q idele 1 1 0 0 0 1 2 0 0", ADF_DOMAIN},
    /* the grammar of 10.1 */
    {"adf1 Q idele 1 1 0 0 0 3 2 5", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0 0 3 2 5 6 0", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0 0 3 2 5 A", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0 0 3 2 05 6", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0  0 3 2 5 6", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0 0 3 2 5 6 ", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0 0 3 2 5 0x6", ADF_PARSE},
    {"adf1 Q idele 1 1 0 0 0 3 2 5 6\n", ADF_PARSE},
    /* the version and the field */
    {"adf2 Q idele 1 1 0 0 0 3 2 5 6", ADF_UNSUPPORTED},
    {"adf1 K idele 1 1 0 0 0 3 2 5 6", ADF_UNSUPPORTED}
};

ADF_TEST(idele_strictness)
{
    TYPED_RUN(adf_idele_t, adf_idele_identical, adf_idele_init, idele_sentinel, adf_idele_clear,
              adf_idele_load_str, adf_idele_dump_inspect, adf_idele_dump_str, idele_cases,
              sizeof(idele_cases) / sizeof(idele_cases[0]));
}

/* Body "idclass <arb> c N": the type is special to Q, so its dump has no count (10.1). The
   predicate of 5.7 is the ball finite and positive and the unit as in 5.6. */
static const tcase idclass_cases[] = {
    /* canonical */
    {"adf1 Q idclass 1 0 0 0 1 0", ADF_OK},
    {"adf1 Q idclass 5 -2 0 0 5 24", ADF_OK},
    {"adf1 Q idclass 3 -1 1 -2 1 0", ADF_OK},        /* [1.5 +- 0.5] is positive */
    {"adf1 Q idclass 1 0 1 -1 1 0", ADF_OK},        /* [1 +- 0.5] is positive */
    /* the real ball */
    {"adf1 Q idclass 1 0 1 0 1 0", ADF_DOMAIN},      /* [0 +- 1] contains 0 */
    {"adf1 Q idclass -1 0 0 0 1 0", ADF_DOMAIN},
    {"adf1 Q idclass 0 0 0 0 1 0", ADF_DOMAIN},
    {"adf1 Q idclass 2 0 0 0 1 0", ADF_DOMAIN},
    {"adf1 Q idclass 1 1 0 -1 1 0", ADF_DOMAIN},    /* [-1 +- 2] contains negative numbers */
    {"adf1 Q idclass 4 0 1 0 1 0", ADF_DOMAIN},      /* an even midpoint mantissa */
    {"adf1 Q idclass 0 -1 0 0 1 0", ADF_DOMAIN},     /* +inf: not finite */
    /* the unit coset (5.6) */
    {"adf1 Q idclass 1 0 0 0 2 4", ADF_DOMAIN},
    {"adf1 Q idclass 1 0 0 0 3 2", ADF_DOMAIN},     /* c > N */
    {"adf1 Q idclass 1 0 0 0 5 0", ADF_DOMAIN},     /* an exact unit that is not +-1 */
    /* the grammar of 10.1 */
    {"adf1 Q idclass 1 0 0 0 1", ADF_PARSE},
    {"adf1 Q idclass 1 0 0 0 1 0 0", ADF_PARSE},
    {"adf1 Q idclass 1 0 0 0 1 A", ADF_PARSE},
    {"adf1 Q idclass 1 0 0 0 1 00", ADF_PARSE},
    {"adf1 Q idclass 1  0 0 0 1 0", ADF_PARSE},
    {"adf1 Q idclass 1 0 0 0 1 0 ", ADF_PARSE},
    /* the version and the field */
    {"adf2 Q idclass 1 0 0 0 1 0", ADF_UNSUPPORTED},
    {"adf1 K idclass 1 0 0 0 1 0", ADF_UNSUPPORTED}
};

ADF_TEST(idclass_strictness)
{
    TYPED_RUN(adf_idclass_t, adf_idclass_identical, adf_idclass_init, idclass_sentinel, adf_idclass_clear,
              adf_idclass_load_str, adf_idclass_dump_inspect, adf_idclass_dump_str, idclass_cases,
              sizeof(idclass_cases) / sizeof(idclass_cases[0]));
}

/* ------------------------------------------------------------------ the limits and the order */

ADF_TEST(limits_and_order_of_checks)
{
    /* max_len is a limit of stage 1 (8.5 item 1), checked before any byte is read: it comes before
       the version, the grammar and the predicate. The texts below break two rules at once. */
    static const tcase short_lim[] = {
        {"adf2 Q ucoset 1 6", ADF_LIMIT},          /* UNSUPPORTED and LIMIT */
        {"adf1 Q ucoset 0 1", ADF_LIMIT},          /* DOMAIN and LIMIT */
        {"adf1 Q ucoset 0 01", ADF_LIMIT},         /* PARSE and LIMIT */
        {"adf1 Q ucoset", ADF_LIMIT}               /* an empty body and LIMIT */
    };
    adf_text_limits_t lim;
    size_t i;

    adf_text_limits_default(&lim);
    lim.max_len = 8;
    ADF_CHECK(lim.max_len == 8);
    for (i = 0; i < sizeof(short_lim) / sizeof(short_lim[0]); i++)
    {
        adf_ucoset_t x, keep;
        size_t len = strlen(short_lim[i].text), nc = 7;

        adf_ucoset_init(x);
        adf_ucoset_init(keep);
        ucoset_sentinel(x);
        ucoset_sentinel(keep);
        ADF_CHECK_MSG(adf_ucoset_load_str(x, short_lim[i].text, len, NULL, &lim) == short_lim[i].status,
                      "\"%s\" with max_len 8", short_lim[i].text);
        ADF_CHECK_MSG(adf_ucoset_dump_inspect(&nc, NULL, short_lim[i].text, len, &lim) == short_lim[i].status
                          && nc == 7,
                      "inspect \"%s\" with max_len 8", short_lim[i].text);
        ADF_CHECK(adf_ucoset_identical(x, keep));
        adf_ucoset_clear(x);
        adf_ucoset_clear(keep);
    }
    /* The idele and the class obey the same stage 1: the length is decided before the body. */
    {
        adf_idele_t e, e0;
        adf_idclass_t k, k0;

        adf_idele_init(e);
        adf_idele_init(e0);
        adf_idclass_init(k);
        adf_idclass_init(k0);
        idele_sentinel(e);
        idele_sentinel(e0);
        idclass_sentinel(k);
        idclass_sentinel(k0);
        ADF_CHECK(adf_idele_load_str(e, "adf2 Q idele 1 0 0 0 0 1 1 1 0",
                                     strlen("adf2 Q idele 1 0 0 0 0 1 1 1 0"), NULL, &lim) == ADF_LIMIT);
        ADF_CHECK(adf_idele_load_str(e, "adf1 Q idele 1 0 0 0 0 1 1 1 0",
                                     strlen("adf1 Q idele 1 0 0 0 0 1 1 1 0"), NULL, NULL) == ADF_DOMAIN);
        ADF_CHECK(adf_idele_identical(e, e0));
        ADF_CHECK(adf_idclass_load_str(k, "adf1 K idclass 1 0 0 0 1 0",
                                       strlen("adf1 K idclass 1 0 0 0 1 0"), NULL, &lim) == ADF_LIMIT);
        ADF_CHECK(adf_idclass_load_str(k, "adf1 Q idclass -1 0 0 0 1 0",
                                       strlen("adf1 Q idclass -1 0 0 0 1 0"), NULL, NULL) == ADF_DOMAIN);
        ADF_CHECK(adf_idclass_identical(k, k0));
        adf_idele_clear(e);
        adf_idele_clear(e0);
        adf_idclass_clear(k);
        adf_idclass_clear(k0);
    }

    /* The header is read before the body: an unknown field or version is ADF_UNSUPPORTED whatever
       the body holds (8.5 item 3, conventions 10.2 "Version and field"). */
    {
        adf_ucoset_t x, keep;

        adf_ucoset_init(x);
        adf_ucoset_init(keep);
        ucoset_sentinel(x);
        ucoset_sentinel(keep);
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 K ucoset 1 zz", strlen("adf1 K ucoset 1 zz"), NULL, NULL)
                  == ADF_UNSUPPORTED);
        ADF_CHECK(adf_ucoset_load_str(x, "adf3 Q ucoset 0 1", strlen("adf3 Q ucoset 0 1"), NULL, NULL)
                  == ADF_UNSUPPORTED);
        /* the grammar of the body (stage 3) comes before its predicate (stage 6) */
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q ucoset 0 01", strlen("adf1 Q ucoset 0 01"), NULL, NULL)
                  == ADF_PARSE);
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q ucoset 0 1", strlen("adf1 Q ucoset 0 1"), NULL, NULL)
                  == ADF_DOMAIN);
        /* a dump of another type is not a sentence of the grammar of this loader (9.7) */
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q idclass 1 0 0 0 1 0", strlen("adf1 Q idclass 1 0 0 0 1 0"),
                                      NULL, NULL)
                  == ADF_PARSE);
        ADF_CHECK(adf_ucoset_identical(x, keep));
        adf_ucoset_clear(x);
        adf_ucoset_clear(keep);
    }
    /* A NUL byte inside the text (8.1) and a byte above 0x7e (8.2) are ADF_PARSE. */
    {
        static const char nul_text[] = "adf1 Q ucoset 1\0 0";
        static const char high_text[] = "adf1 Q ucoset 1 \xc2\xb1";
        adf_ucoset_t x, keep;

        adf_ucoset_init(x);
        adf_ucoset_init(keep);
        ucoset_sentinel(x);
        ucoset_sentinel(keep);
        ADF_CHECK(adf_ucoset_load_str(x, nul_text, sizeof(nul_text) - 1, NULL, NULL) == ADF_PARSE);
        ADF_CHECK(adf_ucoset_load_str(x, high_text, sizeof(high_text) - 1, NULL, NULL) == ADF_PARSE);
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q ucoset", strlen("adf1 Q ucoset"), NULL, NULL) == ADF_PARSE);
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q ucoset 1 0", 0, NULL, NULL) == ADF_PARSE);
        ADF_CHECK(adf_ucoset_identical(x, keep));
        adf_ucoset_clear(x);
        adf_ucoset_clear(keep);
    }
    /* The bindings of a body with no occurrence: the count must be 0 (10.2, G3). */
    {
        adf_ucoset_t x, y;

        adf_ucoset_init(x);
        adf_ucoset_init(y);
        ADF_CHECK(adf_ucoset_load_str_binds(x, "adf1 Q ucoset 1 0", strlen("adf1 Q ucoset 1 0"), NULL, 0,
                                            NULL)
                  == ADF_OK);
        ADF_CHECK(adf_ucoset_identical(x, y));
        ADF_CHECK(adf_ucoset_load_str_binds(x, "adf1 Q ucoset 1 0", strlen("adf1 Q ucoset 1 0"), NULL, 1,
                                            NULL)
                  == ADF_DOMAIN);
        ADF_CHECK(adf_ucoset_identical(x, y));
        ADF_CHECK(adf_ucoset_load_str_binds(x, "adf1 Q ucoset 5 6", strlen("adf1 Q ucoset 5 6"), NULL, 0,
                                            NULL)
                  == ADF_OK);
        ADF_CHECK(!adf_ucoset_identical(x, y));
        adf_ucoset_clear(x);
        adf_ucoset_clear(y);
    }
    /* Any context is accepted by the one-context form when the body has no occurrence. */
    {
        adf_ucoset_t x;
        adf_modctx_struct * ctx = NULL;
        fmpz_t K;

        fmpz_init_set_ui(K, 6);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK && ctx != NULL);
        adf_ucoset_init(x);
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q ucoset 5 6", strlen("adf1 Q ucoset 5 6"), ctx, NULL) == ADF_OK);
        ADF_CHECK(adf_ucoset_load_str(x, "adf1 Q ucoset 5 6", strlen("adf1 Q ucoset 5 6"), NULL, NULL)
                  == ADF_OK);
        adf_ucoset_clear(x);
        adf_modctx_free(ctx);
        fmpz_clear(K);
    }
}

/* The dumped text of the sentinels, so that a change of the dump form is noticed. The mantissa of
   a dyadic midpoint is the odd one that arf_get_fmpz_2exp returns: -3 is -3 * 2^0 and 5 is 5 * 2^0
   (conventions 10.2, real balls as arb_dump_str writes them). */
static void
check_text(const char * what, char * t, size_t len, const char * want)
{
    ADF_CHECK_MSG(t != NULL && len == strlen(want) && memcmp(t, want, len) == 0,
                  "%s: dumped \"%s\", expected \"%s\"", what, t != NULL ? t : "(null)", want);
    adf_str_free(t);
}

ADF_TEST(dump_texts_of_hand_values)
{
    adf_ucoset_t u;
    adf_idele_t e;
    adf_idclass_t k;
    size_t len;
    char * t;

    adf_ucoset_init(u);
    adf_idele_init(e);
    adf_idclass_init(k);
    ucoset_sentinel(u);
    idele_sentinel(e);
    idclass_sentinel(k);

    t = adf_ucoset_dump_str(&len, u);
    check_text("ucoset", t, len, "adf1 Q ucoset 5 6");
    t = adf_idele_dump_str(&len, e);
    check_text("idele", t, len, "adf1 Q idele 1 -3 0 0 0 3 7 5 6");
    t = adf_idclass_dump_str(&len, k);
    check_text("idclass", t, len, "adf1 Q idclass 5 0 0 0 5 6");

    adf_ucoset_clear(u);
    adf_idele_clear(e);
    adf_idclass_clear(k);
}
