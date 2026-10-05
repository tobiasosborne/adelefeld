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

/* The exact unit [1 mod 0] (conventions 5.6), the unit of the texts of this section. */
static void
ucoset_one(adf_ucoset_t u)
{
    fmpz_t c, N;

    fmpz_init_set_ui(c, 1);
    fmpz_init_set_ui(N, 0);
    ADF_CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
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
    /* The two faults of the radius mantissa that the review u-review1 (finding F5) found no test
       for: the bound 2^30 and the parity. The two texts of the review have the even midpoint
       mantissa 40, so a faulty check of the radius is masked by it; the four texts below take an
       odd midpoint whose ball excludes 0 ([3 +- 2] = [1, 5] and [5 +- 1] = [4, 6]), so nothing
       but the radius mantissa decides the status. */
    {"adf1 Q idele 1 1 40 40000001 0 1 1 1 0", ADF_DOMAIN}, /* 2^30 + 1, review F5 */
    {"adf1 Q idele 1 1 40 2 0 1 1 1 0", ADF_DOMAIN},         /* an even radius mantissa, review F5 */
    {"adf1 Q idele 1 5 0 40000001 0 1 1 1 0", ADF_DOMAIN},   /* 2^30 + 1 below no bound of 10.2 */
    {"adf1 Q idele 1 5 0 2 0 1 1 1 0", ADF_DOMAIN},           /* an even radius mantissa */
    {"adf1 Q idele 1 3 0 40000001 0 1 1 1 0", ADF_DOMAIN},
    {"adf1 Q idele 1 3 0 2 0 1 1 1 0", ADF_DOMAIN},
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
    /* the radius mantissa: the bound 2^30 and the parity (10.2), as for the idele above */
    {"adf1 Q idclass 1 0 40000001 0 1 0", ADF_DOMAIN},
    {"adf1 Q idclass 1 0 2 0 1 0", ADF_DOMAIN},
    {"adf1 Q idclass 40 0 40000001 0 1 0", ADF_DOMAIN},      /* the texts of the review F5 */
    {"adf1 Q idclass 40 0 2 0 1 0", ADF_DOMAIN},
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

/* ------------------------------------- the real sign of a dumped ball: exact arithmetic, not the
   method of the loader (review u-review1, findings F1 and F2).

   The loader decides the sign of a real ball in dp_arb_sign (src/dump.c), which compares the two
   powers |m| 2^me and rm 2^re without forming them. The texts of this section are texts that
   function got wrong: a negative midpoint read one byte too far (F1) and a tie of the leading bits
   read as an equality (F2). The expected status is computed here by exact arithmetic on the four
   tokens and compared with what the public constructor says for the same ball. Three verdicts
   (the arithmetic, the loader, the constructor); they must agree on every case. */

/* 1 if the closed ball [m 2^me - rm 2^re, m 2^me + rm 2^re] excludes 0, that is |m| 2^me > rm 2^re:
   the predicate of conventions 5.7 for the inf of an idele (arb_is_nonzero) and, with m > 0, for
   the t of a class (arb_is_positive). The comparison is exact and forms neither power: with
   d = me - re the two differ by the factor 2^d, so |m| 2^d > r holds as soon as d is at least the
   bit length of r (then |m| 2^d >= 2^d > r), it fails as soon as -d is at least the bit length of
   |m| (then r 2^(-d) > |m|), and in between both sides are integers of the size of the mantissa
   and the shift is a small integer. An exponent of 5000 bits, or of 2^5000, is therefore decided
   as exactly as a small one, and no bound of a machine word enters. */
static int
ball_excludes_zero(const fmpz_t m, const fmpz_t e, const fmpz_t r, const fmpz_t f)
{
    fmpz_t d, am, t, u;
    int res;

    fmpz_init(d);
    fmpz_init(am);
    fmpz_init(t);
    fmpz_init(u);
    fmpz_abs(am, m);
    if (fmpz_is_zero(r))
        res = !fmpz_is_zero(am);            /* an exact ball: it excludes 0 iff m != 0 */
    else
    {
        fmpz_sub(d, e, f);
        if (fmpz_sgn(d) >= 0)
        {
            if (fmpz_cmp_ui(d, fmpz_bits(r)) >= 0)
                res = 1;
            else
            {
                fmpz_mul_2exp(t, am, fmpz_get_ui(d));
                res = fmpz_cmp(t, r) > 0;
            }
        }
        else if (fmpz_cmp_si(d, -(slong) fmpz_bits(am)) <= 0)
            res = 0;
        else
        {
            fmpz_neg(u, d);
            fmpz_mul_2exp(t, r, fmpz_get_ui(u));
            res = fmpz_cmp(am, t) > 0;
        }
    }
    fmpz_clear(d);
    fmpz_clear(am);
    fmpz_clear(t);
    fmpz_clear(u);
    return res;
}

/* The radius rm 2^re exactly. A mag is MAG_MAN 2^(MAG_EXP - MAG_BITS) with a mantissa of exactly
   MAG_BITS bits (/usr/include/flint/mag.h:113-119, 135-140; mag_one, mag.h:217-221, is
   MAG_ONE_HALF with exponent 1). mag_set_fmpz_2exp_fmpz is not exact at 30 bits, so the mantissa
   and the exponent are written here, as dp_mag_set_exact does in src/dump.c. */
static void
mag_set_exact(mag_t r, ulong m, const fmpz_t e)
{
    unsigned b = FLINT_BIT_COUNT(m);

    MAG_MAN(r) = m << (MAG_BITS - b);
    fmpz_add_ui(MAG_EXPREF(r), e, b);
}

/* One case of the family: the four tokens of a real ball, and the body keyword. */
typedef struct
{
    const char * m, * e, * rm, * re;   /* lower-case hexadecimal tokens, as a dump holds them */
    const char * kw;                   /* "idele" or "idclass" */
    int tail;                          /* 1 for the idele, whose content and unit follow the ball */
} sign_case;

/* The exact verdict of one case: the ball is admissible for the type of the case. */
static int
sign_case_exact(const sign_case * c)
{
    fmpz_t m, e, rm, re;
    int res;

    fmpz_init(m);
    fmpz_init(e);
    fmpz_init(rm);
    fmpz_init(re);
    fmpz_set_str(m, c->m, 16);
    fmpz_set_str(e, c->e, 16);
    fmpz_set_str(rm, c->rm, 16);
    fmpz_set_str(re, c->re, 16);
    res = ball_excludes_zero(m, e, rm, re) && (c->tail != 0 || fmpz_sgn(m) > 0);
    fmpz_clear(m);
    fmpz_clear(e);
    fmpz_clear(rm);
    fmpz_clear(re);
    return res;
}

/* The text of one case: the four tokens of the ball, then the content and the unit of an idele
   (1, 1 and [1 mod 0], so that nothing but the ball decides) or the unit of a class. */
static size_t
sign_case_text(const sign_case * c, char * buf, size_t buflen)
{
    int n = c->tail ? snprintf(buf, buflen, "adf1 Q idele 1 %s %s %s %s 1 1 1 0", c->m, c->e, c->rm,
                              c->re)
                    : snprintf(buf, buflen, "adf1 Q idclass %s %s %s %s 1 0", c->m, c->e, c->rm,
                               c->re);

    return (size_t) (n < 0 ? 0 : n);
}

/* The verdict of the loader: the status, the output untouched on every status other than ADF_OK,
   and on ADF_OK the dump of the loaded value byte for byte the text of the case. The value starts
   at a sentinel, so "untouched" is a statement about that value. */
static void
sign_case_loader(const sign_case * c, const char * text, size_t len, int want)
{
    adf_idele_t e, e0;
    adf_idclass_t k, k0;
    size_t dl = 0;
    char * back;
    int st;

    if (c->tail)
    {
        adf_idele_init(e);
        adf_idele_init(e0);
        idele_sentinel(e);
        idele_sentinel(e0);
        st = adf_idele_load_str(e, text, len, NULL, NULL);
        ADF_CHECK_MSG(st == (want ? ADF_OK : ADF_DOMAIN), "load \"%s\": %s, expected %s", text,
                      adf_status_str(st), adf_status_str(want ? ADF_OK : ADF_DOMAIN));
        if (st != ADF_OK)
            ADF_CHECK_MSG(adf_idele_identical(e, e0), "load \"%s\" touched the output", text);
        else
        {
            back = adf_idele_dump_str(&dl, e);
            ADF_CHECK_MSG(back != NULL && dl == len && memcmp(back, text, len) == 0,
                          "the dump of \"%s\" is \"%s\"", text, back);
            adf_str_free(back);
        }
        adf_idele_clear(e);
        adf_idele_clear(e0);
        return;
    }
    adf_idclass_init(k);
    adf_idclass_init(k0);
    idclass_sentinel(k);
    idclass_sentinel(k0);
    st = adf_idclass_load_str(k, text, len, NULL, NULL);
    ADF_CHECK_MSG(st == (want ? ADF_OK : ADF_DOMAIN), "load \"%s\": %s, expected %s", text,
                  adf_status_str(st), adf_status_str(want ? ADF_OK : ADF_DOMAIN));
    if (st != ADF_OK)
        ADF_CHECK_MSG(adf_idclass_identical(k, k0), "load \"%s\" touched the output", text);
    else
    {
        back = adf_idclass_dump_str(&dl, k);
        ADF_CHECK_MSG(back != NULL && dl == len && memcmp(back, text, len) == 0,
                      "the dump of \"%s\" is \"%s\"", text, back);
        adf_str_free(back);
    }
    adf_idclass_clear(k);
    adf_idclass_clear(k0);
}

/* The verdict of the public constructor: the same ball, built exactly from the four tokens (the
   mantissas are odd, so this is the arb that arb_dump_str writes, conventions 10.2). The
   constructor returns OK exactly when the ball satisfies the predicate of 5.7, and the value it
   leaves is then canonical. */
static void
sign_case_constructor(const sign_case * c, int want)
{
    arb_t a;
    fmpq_t r;
    adf_ucoset_t u;
    adf_idele_t e;
    adf_idclass_t k;
    fmpz_t m, ex, re;
    ulong rad = 0;
    size_t i;
    int st;

    fmpz_init(m);
    fmpz_init(ex);
    fmpz_init(re);
    fmpz_set_str(m, c->m, 16);
    fmpz_set_str(ex, c->e, 16);
    fmpz_set_str(re, c->re, 16);
    for (i = 0; c->rm[i] != '\0'; i++)
        rad = 16 * rad + (ulong) (c->rm[i] <= '9' ? c->rm[i] - '0' : c->rm[i] - 'a' + 10);
    arb_init(a);
    fmpq_init(r);
    adf_ucoset_init(u);
    adf_idele_init(e);
    adf_idclass_init(k);
    fmpq_set_si(r, 1, 1);
    ucoset_sentinel(u);
    arf_set_fmpz_2exp(arb_midref(a), m, ex);           /* exact: arb.rst:227-229 */
    mag_set_exact(arb_radref(a), rad, re);
    if (c->tail)
    {
        st = adf_idele_set_parts(e, a, r, u);
        ADF_CHECK_MSG(st == (want ? ADF_OK : ADF_DOMAIN),
                      "adf_idele_set_parts of the ball [%s %s %s %s]: %s, expected %s", c->m, c->e,
                      c->rm, c->re, adf_status_str(st), adf_status_str(want ? ADF_OK : ADF_DOMAIN));
        if (want)
            ADF_CHECK_MSG(adf_idele_is_canonical(e),
                          "the idele of the ball [%s %s %s %s] is not canonical", c->m, c->e, c->rm,
                          c->re);
    }
    else
    {
        st = adf_idclass_set_parts(k, a, u);
        ADF_CHECK_MSG(st == (want ? ADF_OK : ADF_DOMAIN),
                      "adf_idclass_set_parts of the ball [%s %s %s %s]: %s, expected %s", c->m, c->e,
                      c->rm, c->re, adf_status_str(st), adf_status_str(want ? ADF_OK : ADF_DOMAIN));
        if (want)
            ADF_CHECK_MSG(adf_idclass_is_canonical(k),
                          "the class of the ball [%s %s %s %s] is not canonical", c->m, c->e, c->rm,
                          c->re);
    }
    adf_idele_clear(e);
    adf_idclass_clear(k);
    adf_ucoset_clear(u);
    fmpq_clear(r);
    arb_clear(a);
    fmpz_clear(m);
    fmpz_clear(ex);
    fmpz_clear(re);
}

/* One case of the family: the exact arithmetic, then the loader and the constructor. */
static void
sign_case_run(const sign_case * c)
{
    char text[6000];
    size_t len = sign_case_text(c, text, sizeof(text));
    int want = sign_case_exact(c);

    ADF_CHECK_MSG((c->tail != 0 && strcmp(c->kw, "idele") == 0)
                      || (c->tail == 0 && strcmp(c->kw, "idclass") == 0),
                  "the keyword %s does not go with the body of the case", c->kw);
    ADF_CHECK_MSG(len > 0 && len < sizeof(text), "the text of a case does not fit");
    if (len == 0 || len >= sizeof(text))
        return;
    sign_case_loader(c, text, len, want);
    sign_case_constructor(c, want);
}

/* The hexadecimal token of 2^bits, negated if neg: 2^b is the digit 2^(b mod 4) of the first
   hexadecimal place followed by b / 4 zeros. */
static void
hex_pow2(char * buf, size_t buflen, size_t bits, int neg)
{
    size_t q = bits / 4, i, off = neg ? 1 : 0;

    if (buflen < off + q + 2)
    {
        buf[0] = '\0';
        return;
    }
    if (neg)
        buf[0] = '-';
    buf[off] = "1248"[bits % 4];
    for (i = 0; i < q; i++)
        buf[off + 1 + i] = '0';
    buf[off + 1 + q] = '\0';
}

/* The texts of the review (lanes/u-review1/result.md, finding F1), with the exact status of
   each: all three balls contain 0, so each is ADF_DOMAIN, the output is left at the sentinel and
   nothing aborts. */
ADF_TEST(idele_negative_midpoint_domain)
{
    static const tcase neg_mid[] = {
        {"adf1 Q idele 1 -1 0 1 0 1 1 1 0", ADF_DOMAIN},        /* the ball [-2, 0] contains 0 */
        {"adf1 Q idele 1 -5 0 1 3 5 3 -1 0", ADF_DOMAIN},      /* [-13, 3] contains 0 */
        {"adf1 Q idele 1 -bb 5 3c3 d 7 6 3 4", ADF_DOMAIN}      /* [-7889880, 7882912] contains 0 */
    };
    sign_case c;

    TYPED_RUN(adf_idele_t, adf_idele_identical, adf_idele_init, idele_sentinel, adf_idele_clear,
              adf_idele_load_str, adf_idele_dump_inspect, adf_idele_dump_str, neg_mid,
              sizeof(neg_mid) / sizeof(neg_mid[0]));
    /* The exact arithmetic says the same of the three texts: no one of them excludes 0. */
    c.m = "-1"; c.e = "0"; c.rm = "1"; c.re = "0"; c.kw = "idele"; c.tail = 1;
    ADF_CHECK(!sign_case_exact(&c));
    c.m = "-5"; c.e = "0"; c.rm = "1"; c.re = "3";
    ADF_CHECK(!sign_case_exact(&c));
    c.m = "-bb"; c.e = "5"; c.rm = "3c3"; c.re = "d";
    ADF_CHECK(!sign_case_exact(&c));
    /* A negative midpoint that does exclude 0 is admissible for an idele and forbidden for a
       class (5.7: every point of the ball of a class is positive). */
    c.m = "-3"; c.e = "0"; c.rm = "1"; c.re = "1";
    ADF_CHECK(sign_case_exact(&c));
    c.tail = 0;
    ADF_CHECK(!sign_case_exact(&c));
}

/* The texts of the review (finding F2): in the same binade, with the midpoint the longer mantissa,
   a tie of the leading bits is a strict inequality, so the ball [1, 5] excludes 0. Each text is
   ADF_OK, and the round trip through the public constructor holds. */
ADF_TEST(tie_of_the_leading_bits_is_a_strict_inequality)
{
    /* idele: midpoint 3, radius 1 2^1 = 2, the ball [1, 5] */
    {
        static const tcase tie[] = {{"adf1 Q idele 1 3 0 1 1 1 1 1 0", ADF_OK}};
        adf_idele_t x, y;
        adf_ucoset_t u;
        arb_t a;
        fmpq_t r;
        size_t len, len2;
        char * t, * t2;
        sign_case c;

        TYPED_RUN(adf_idele_t, adf_idele_identical, adf_idele_init, idele_sentinel, adf_idele_clear,
                  adf_idele_load_str, adf_idele_dump_inspect, adf_idele_dump_str, tie,
                  sizeof(tie) / sizeof(tie[0]));
        c.m = "3"; c.e = "0"; c.rm = "1"; c.re = "1"; c.kw = "idele"; c.tail = 1;
        ADF_CHECK(sign_case_exact(&c));
        /* the round trip of the public constructor: build the value, dump it, load it again */
        adf_ucoset_init(u);
        arb_init(a);
        fmpq_init(r);
        adf_idele_init(x);
        adf_idele_init(y);
        ucoset_one(u);
        fmpq_set_si(r, 1, 1);
        arf_set_si(arb_midref(a), 3);
        mag_one(arb_radref(a));
        mag_mul_2exp_si(arb_radref(a), arb_radref(a), 1);
        ADF_CHECK(adf_idele_set_parts(x, a, r, u) == ADF_OK && adf_idele_is_canonical(x));
        t = adf_idele_dump_str(&len, x);
        ADF_CHECK_MSG(len == strlen("adf1 Q idele 1 3 0 1 1 1 1 1 0")
                          && memcmp(t, "adf1 Q idele 1 3 0 1 1 1 1 1 0", len) == 0,
                      "the dump is \"%s\"", t);
        ADF_CHECK(adf_idele_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK_MSG(adf_idele_identical(x, y), "load(dump(v)) is not v");
        t2 = adf_idele_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t2, t, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_idele_clear(x);
        adf_idele_clear(y);
        adf_ucoset_clear(u);
        arb_clear(a);
        fmpq_clear(r);
    }
    /* idclass: the same ball; the class needs only that every point of it is positive */
    {
        static const tcase tie[] = {{"adf1 Q idclass 3 0 1 1 1 0", ADF_OK}};
        adf_idclass_t x, y;
        adf_ucoset_t u;
        arb_t a;
        size_t len, len2;
        char * t, * t2;
        sign_case c;

        TYPED_RUN(adf_idclass_t, adf_idclass_identical, adf_idclass_init, idclass_sentinel,
                  adf_idclass_clear, adf_idclass_load_str, adf_idclass_dump_inspect,
                  adf_idclass_dump_str, tie, sizeof(tie) / sizeof(tie[0]));
        c.m = "3"; c.e = "0"; c.rm = "1"; c.re = "1"; c.kw = "idclass"; c.tail = 0;
        ADF_CHECK(sign_case_exact(&c));
        adf_ucoset_init(u);
        arb_init(a);
        adf_idclass_init(x);
        adf_idclass_init(y);
        ucoset_one(u);
        arf_set_si(arb_midref(a), 3);
        mag_one(arb_radref(a));
        mag_mul_2exp_si(arb_radref(a), arb_radref(a), 1);
        ADF_CHECK(adf_idclass_set_parts(x, a, u) == ADF_OK && adf_idclass_is_canonical(x));
        t = adf_idclass_dump_str(&len, x);
        ADF_CHECK_MSG(len == strlen("adf1 Q idclass 3 0 1 1 1 0")
                          && memcmp(t, "adf1 Q idclass 3 0 1 1 1 0", len) == 0,
                      "the dump is \"%s\"", t);
        ADF_CHECK(adf_idclass_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK_MSG(adf_idclass_identical(x, y), "load(dump(v)) is not v");
        t2 = adf_idclass_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t2, t, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_idclass_clear(x);
        adf_idclass_clear(y);
        adf_ucoset_clear(u);
        arb_clear(a);
    }
}

/* ------------------------------------------------------------------ the family of the sign

   Every dump whose real ball has the midpoint m 2^e and the radius r 2^f, for an odd m with
   |m| <= 127 in both signs, an odd r <= 127 with the two radius mantissas of 30 bits that the
   bound of 10.2 leaves open (2^29 + 1 and 2^30 - 1), and e, f in -9..9. Both bodies: the inf of
   an idele and the t of a class. The expected status is the exact comparison |m| 2^e > r 2^f (and
   m > 0 for the class), and it is compared with the loader and with the public constructor. The
   family does not use the method of dp_arb_sign: it forms the two powers and compares them, so
   it does not repeat a defect of that method. The strides below cut the product to a size that a
   sanitized build also runs in seconds; every shape of the comparison of dp_arb_sign (the same
   binade with bm > br, bm == br and bm < br, and binades that differ by one, two and more) is in
   the family, and the whole product of m, r, e and f for one exponent is in the corpus of the
   review (lanes/u-review1, gen.py). */

ADF_TEST(sign_of_the_small_balls)
{
    char rbuf[64][10];
    size_t idx[10], nidx = 0;
    size_t nr = 0, am, ai, i, j, e, f, k, ncases;
    long rr;

    for (rr = 1; rr <= 127; rr += 2)
        snprintf(rbuf[nr], sizeof(rbuf[0]), "%lx", (unsigned long) rr), nr++;
    ADF_CHECK(nr == 64);
    snprintf(rbuf[62], sizeof(rbuf[0]), "%lx", (unsigned long) ((1 << 29) + 1));
    snprintf(rbuf[63], sizeof(rbuf[0]), "%lx", (unsigned long) ((1 << 30) - 1));
    ADF_CHECK(strlen(rbuf[62]) == 8 && strlen(rbuf[63]) == 8);
    /* every eighth small radius mantissa, and the two of 30 bits: nine radius mantisses */
    for (i = 0; i < 56; i += 8)
        idx[nidx++] = i;
    idx[nidx++] = 62;
    idx[nidx++] = 63;
    ADF_CHECK(nidx == 9);
    ncases = 0;
    for (am = 1; am <= 127; am += 2)
        for (ai = 0; ai < 2; ai++)
            for (j = 0; j < nr; j++)
                for (e = 9; e < 28; e++)
                    for (f = 9; f < 28; f++)
                    {
                        sign_case c;
                        char mbuf[10], eb[10], fb[10];

                        snprintf(mbuf, sizeof(mbuf), "%s%lx", ai ? "-" : "", (unsigned long) am);
                        snprintf(eb, sizeof(eb), "%ld", (long) e - 9);
                        snprintf(fb, sizeof(fb), "%ld", (long) f - 9);
                        c.m = mbuf;
                        c.e = eb;
                        c.rm = rbuf[j];
                        c.re = fb;
                        c.kw = "idele";
                        c.tail = 1;
                        sign_case_run(&c);
                        ncases++;
                    }
    ADF_CHECK_MSG(ncases == 64 * 2 * 64 * 19 * 19, "%zu idele cases", ncases);
    /* The class of the same family with the stride of the midpoint raised by three, so that a
       second product of the same shape is not repeated case by case. */
    ncases = 0;
    for (am = 1; am <= 127; am += 6)
        for (ai = 0; ai < 2; ai++)
            for (j = 0; j < nidx; j++)
                for (e = 9; e < 28; e++)
                    for (f = 9; f < 28; f++)
                    {
                        sign_case c;
                        char mbuf[10], eb[10], fb[10];

                        snprintf(mbuf, sizeof(mbuf), "%s%lx", ai ? "-" : "", (unsigned long) am);
                        snprintf(eb, sizeof(eb), "%ld", (long) e - 9);
                        snprintf(fb, sizeof(fb), "%ld", (long) f - 9);
                        c.m = mbuf;
                        c.e = eb;
                        c.rm = rbuf[idx[j]];
                        c.re = fb;
                        c.kw = "idclass";
                        c.tail = 0;
                        sign_case_run(&c);
                        ncases++;
                    }
    ADF_CHECK_MSG(ncases == 22 * 2 * 9 * 19 * 19, "%zu idclass cases", ncases);
    /* Twenty cases whose exponents have 100 and 5000 bits: the sign of the exponents alternates,
       so that both a binade that differs by one bit and one that differs by hundreds of bits are
       compared on the digit strings. */
    for (k = 0; k < 20; k++)
    {
        sign_case c;
        char mbuf[10], eb[1300], fb[1300], rb[10];
        size_t b1 = (k % 2) ? 5000 : 100, b2 = (k % 3) ? 5000 : 100;

        snprintf(mbuf, sizeof(mbuf), "%s%lx", (k % 4 < 2) ? "" : "-",
                 (unsigned long) (2 * (k % 63) + 1));
        hex_pow2(eb, sizeof(eb), b1, k % 2);
        hex_pow2(fb, sizeof(fb), b2, (k / 2) % 2);
        snprintf(rb, sizeof(rb), "%lx", (unsigned long) ((1 << (k % 30)) + 1));
        c.m = mbuf;
        c.e = eb;
        c.rm = rb;
        c.re = fb;
        c.kw = (k % 2) ? "idclass" : "idele";
        c.tail = (k % 2) ? 0 : 1;
        sign_case_run(&c);
    }
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
