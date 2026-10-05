/* tests/test_dump_local.c: the dump form (docs/conventions.md 10) of adf_lball and adf_sball, the local
   ball at one prime and the partial ball (include/adelefeld/dump.h, bodies "lball" and "sball" of
   conventions 10.1; predicates of conventions 5.8 and 5.9; rules of 10.2: strict, real balls as
   arb_dump_str writes them, CV-52, no context occurrence; limits 8.4; order of the checks 8.5).

   What is checked:

   1. Every row of tests/golden/dump.tsv whose body is "lball" or "sball" (22 rows: 9 valid, 13 with
      a status; counted in Python from the file with grep -c). A valid row is loaded and dumped back
      byte for byte; a row with a status gives that status from the loader and from the inspector,
      leaves the output value untouched and does not write *nctx.
   2. Round trips on 2000 random canonical values of each type, built through the public
      constructors (adf_lball_set_rat, adf_lball_set_rat_ball, adf_sball_set_arb_lballs,
      adf_sball_project) with values up to the bounds of lball.h: load(dump(v)) is identical to v,
      and dump(load(t)) is t byte for byte. A complex component is made by hand, as sball.h:60
      allows ("a binding that fills the struct by hand may").
   3. Strictness: for every field a well formed text that is not canonical is ADF_DOMAIN; the
      grammatical defects are ADF_PARSE; the version "adf2" and the field "K" are
      ADF_UNSUPPORTED; |v| and |N| above max_prec and a length above max_len are ADF_LIMIT; and the
      order of 8.5 is pinned with texts that break two rules at once (a limit before a word
      restriction, a limit before a predicate, a word restriction before a predicate).
   4. adf_x_dump_inspect returns the status of the loader for every text above and writes *nctx = 0
      on ADF_OK only.

   The expected texts and statuses are written from the grammar of conventions 10.1 and the
   predicates of conventions 5.8 and 5.9, not from the output of this library. The 22 rows of
   tests/golden/dump.tsv and the texts of the tables were also run through the reference
   `proto/text_grammar.py` (`dump_load_check`), which agrees with every one of them. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>

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

/* The body keyword of a text: its third space-separated token. 0 for another body. */
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

/* The sentinel values: a fixed value of each type, and the identity of the type. */

static void
lball_sentinel(adf_lball_t x)
{
    adf_place_t v;
    adf_rat_t q;

    ADF_CHECK(adf_place_prime(&v, 5) == ADF_OK);
    adf_rat_init(q);
    fmpq_set_si(q->q, 3, 1);
    ADF_CHECK(adf_lball_set_rat_ball(x, v, q, 4) == ADF_OK);
    adf_rat_clear(q);
}

static void
sball_sentinel(adf_sball_t x)
{
    adf_place_t v;
    adf_lball_struct loc[1];
    adf_rat_t q;
    arb_t r;

    ADF_CHECK(adf_place_prime(&v, 5) == ADF_OK);
    adf_lball_init(&loc[0]);
    adf_rat_init(q);
    arb_init(r);
    fmpq_set_si(q->q, 3, 1);
    ADF_CHECK(adf_lball_set_rat_ball(&loc[0], v, q, 4) == ADF_OK);
    arb_set_si(r, 5);
    ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, loc, 1) == ADF_OK);
    arb_clear(r);
    adf_rat_clear(q);
    adf_lball_clear(&loc[0]);
}

/* ---- the random values of section 2 ---- */

/* A list of small primes, so that the values stay inside the bounds of lball.h: the centre of a
   ball is an integer below p^(N - v), and the functions that build a value form powers p^k with
   k bits(p) at most ADF_LBALL_BITS_MAX. */
static const ulong dp_primes[] = {2,  3,  5,  7,  11, 13, 17, 19, 23, 29,  31,  37,  41,
                                  43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89,  97,  101,
                                  103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163};

#define DP_NPRIMES (sizeof(dp_primes) / sizeof(dp_primes[0]))

/* A random canonical local ball at the place v: with probability 1 in 3 an exact value, else a ball
   whose centre is the canonical one of 5.8. adf_lball_set_rat_ball reduces the centre, so the
   value satisfies the predicate whatever the input. */
static void
random_lball_at(adf_lball_t x, adf_place_t v, flint_rand_t st)
{
    adf_rat_t c;
    slong N;
    int exact = (n_randint(st, 3) == 0);

    adf_rat_init(c);
    fmpq_randtest(c->q, st, 12);
    if (exact)
        ADF_CHECK(adf_lball_set_rat(x, v, c) == ADF_OK);
    else
    {
        N = (slong) n_randint(st, 9) - 3;
        if (adf_lball_set_rat_ball(x, v, c, N) == ADF_LIMIT)
            ADF_CHECK(adf_lball_set_rat(x, v, c) == ADF_OK);  /* the bounds of lball.h */
    }
    ADF_CHECK(adf_lball_is_canonical(x));
    adf_rat_clear(c);
}

/* A random canonical local ball at the place of a random prime. */
static void
random_lball(adf_lball_t x, flint_rand_t st)
{
    adf_place_t v;

    ADF_CHECK(adf_place_prime(&v, dp_primes[n_randint(st, DP_NPRIMES)]) == ADF_OK);
    random_lball_at(x, v, st);
}

/* A random finite real ball whose radius is 0 or a power of 2, so that the dump holds it exactly. */
static void
random_arb(arb_t r, flint_rand_t st)
{
    fmpz_t m;
    slong k;

    fmpz_init(m);
    fmpz_randtest_unsigned(m, st, 8);
    if (n_randint(st, 4) == 0)
        fmpz_zero(m);
    k = (slong) n_randint(st, 9) - 4;
    arb_set_fmpz(r, m);
    arb_mul_2exp_si(r, r, k);
    if (n_randint(st, 3) != 0)
    {
        arb_t e;
        arb_init(e);
        arb_set_si(e, 1);
        arb_mul_2exp_si(e, e, (slong) n_randint(st, 9) - 4);
        arb_add(r, r, e, 64);
        arb_clear(e);
    }
    fmpz_clear(m);
}

/* A random canonical partial ball: the archimedean tag (0, 1 or 2, the last written by hand as
   sball.h:60 allows), then 0 to 4 primes, taken from the list above in increasing order. */
static void
random_sball(adf_sball_t x, flint_rand_t st)
{
    adf_lball_struct loc[4];
    slong m = (slong) n_randint(st, 5), i;
    size_t start = n_randint(st, DP_NPRIMES);
    int arch = (int) n_randint(st, 3);
    arb_t r;

    for (i = 0; i < m; i++)
    {
        adf_place_t v;
        adf_lball_init(&loc[i]);
        ADF_CHECK(adf_place_prime(&v, dp_primes[(start + (size_t) i) % DP_NPRIMES]) == ADF_OK);
        random_lball_at(&loc[i], v, st);
    }
    arb_init(r);
    random_arb(r, st);
    if (arch == 0)
        ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, NULL, loc, m) == ADF_OK);
    else
        ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, r, loc, m) == ADF_OK);
    if (arch == 2)
    {
        /* the COMPLEX tag has no constructor (sball.h:60): the fields are written as sball.h:82-92
           lays them out.  The imaginary part is a copy of the real one, so the value is canonical. */
        acb_set_arb(x->inf, r);
        x->arch = ADF_ARCH_COMPLEX;
        ADF_CHECK(adf_sball_is_canonical(x));
    }
    ADF_CHECK(adf_sball_is_canonical(x));
    arb_clear(r);
    for (i = 0; i < m; i++)
        adf_lball_clear(&loc[i]);
}

/* ------------------------------------------------------------------ the rows of tests/golden */

static void
golden_row(const golden_record * r, int want, size_t * nvalid)
{
    size_t len = r->input_len, nc = 7;
    int st, ist;

    if (body_is(r->input, len, "lball"))
    {
        adf_lball_t x, keep;
        adf_lball_init(x);
        adf_lball_init(keep);
        lball_sentinel(x);
        lball_sentinel(keep);
        st = adf_lball_load_str(x, r->input, len, NULL, NULL);
        ADF_CHECK_MSG(st == want, "line %lu \"%s\": load %s, expected %s", r->line, r->input,
                      adf_status_str(st), adf_status_str(want));
        if (st != ADF_OK)
            ADF_CHECK(adf_lball_identical(x, keep));
        else
        {
            size_t dl = 0;
            char * back = adf_lball_dump_str(&dl, x);
            ADF_CHECK_MSG(back != NULL && dl == r->expected_len && memcmp(back, r->expected, dl) == 0,
                          "line %lu: dumped \"%s\", expected \"%s\"", r->line, back, r->expected);
            adf_str_free(back);
            (*nvalid)++;
        }
        ist = adf_lball_dump_inspect(&nc, NULL, r->input, len, NULL);
        ADF_CHECK(ist == want && (want == ADF_OK ? nc == 0 : nc == 7));
        adf_lball_clear(x);
        adf_lball_clear(keep);
    }
    else
    {
        adf_sball_t x, keep;
        adf_sball_init(x);
        adf_sball_init(keep);
        sball_sentinel(x);
        sball_sentinel(keep);
        st = adf_sball_load_str(x, r->input, len, NULL, NULL);
        ADF_CHECK_MSG(st == want, "line %lu \"%s\": load %s, expected %s", r->line, r->input,
                      adf_status_str(st), adf_status_str(want));
        if (st != ADF_OK)
            ADF_CHECK(adf_sball_identical(x, keep));
        else
        {
            size_t dl = 0;
            char * back = adf_sball_dump_str(&dl, x);
            ADF_CHECK_MSG(back != NULL && dl == r->expected_len && memcmp(back, r->expected, dl) == 0,
                          "line %lu: dumped \"%s\", expected \"%s\"", r->line, back, r->expected);
            adf_str_free(back);
            (*nvalid)++;
        }
        ist = adf_sball_dump_inspect(&nc, NULL, r->input, len, NULL);
        ADF_CHECK(ist == want && (want == ADF_OK ? nc == 0 : nc == 7));
        adf_sball_clear(x);
        adf_sball_clear(keep);
    }
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
        if (body_is(r->input, r->input_len, "lball") || body_is(r->input, r->input_len, "sball"))
        {
            golden_row(r, want, &nvalid);
            nrows++;
        }
    }
    ADF_CHECK_MSG(nrows == 22 && nvalid == 9, "%zu rows, %zu valid", nrows, nvalid);
    golden_close(f);
}

/* ------------------------------------------------------------------ round trips */

/* One round trip: dump, load into y, identical, dump again and compare the bytes. */
static void
one_lball_round_trip(adf_lball_t x)
{
    adf_lball_t y;
    size_t len, len2;
    char * t, * t2;

    adf_lball_init(y);
    t = adf_lball_dump_str(&len, x);
    ADF_CHECK(t != NULL && dump_well_formed(t, len));
    ADF_CHECK(adf_lball_load_str(y, t, len, NULL, NULL) == ADF_OK);
    ADF_CHECK_MSG(adf_lball_identical(x, y), "load(dump(v)) is not v: \"%s\"", t);
    t2 = adf_lball_dump_str(&len2, y);
    ADF_CHECK_MSG(len2 == len && memcmp(t2, t, len) == 0, "dump(load(t)) is not t");
    adf_str_free(t);
    adf_str_free(t2);
    adf_lball_clear(y);
}

static void
one_sball_round_trip(adf_sball_t x)
{
    adf_sball_t y;
    size_t len, len2;
    char * t, * t2;

    adf_sball_init(y);
    t = adf_sball_dump_str(&len, x);
    ADF_CHECK(t != NULL && dump_well_formed(t, len));
    ADF_CHECK(adf_sball_load_str(y, t, len, NULL, NULL) == ADF_OK);
    ADF_CHECK_MSG(adf_sball_identical(x, y), "load(dump(v)) is not v: \"%s\"", t);
    t2 = adf_sball_dump_str(&len2, y);
    ADF_CHECK_MSG(len2 == len && memcmp(t2, t, len) == 0, "dump(load(t)) is not t");
    adf_str_free(t);
    adf_str_free(t2);
    adf_sball_clear(y);
}

ADF_TEST(lball_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 2000; i++)
    {
        adf_lball_t x;
        adf_lball_init(x);
        random_lball(x, st);
        one_lball_round_trip(x);
        adf_lball_clear(x);
    }
    flint_randclear(st);
}

ADF_TEST(sball_round_trip_random)
{
    flint_rand_t st;
    int i, arch = 0;

    flint_randinit(st);
    for (i = 0; i < 2000; i++)
    {
        adf_sball_t x;
        adf_sball_init(x);
        random_sball(x, st);
        one_sball_round_trip(x);
        arch += adf_sball_arch(x);
        adf_sball_clear(x);
    }
    ADF_CHECK_MSG(arch > 0 && arch < 2 * 2000, "%d of 2000 values carried an archimedean place", arch);
    flint_randclear(st);
}

/* ------------------------------------------------------------------ strictness */

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

/* Body "lball p (x num(u) den(u) v | b u v N)". Each text is derived from the predicate of
   conventions 5.8: p a prime, exact = 1: N = 0 and (u = 0 with v = 0, or p divides neither the
   numerator nor the denominator of u); exact = 0: u an integer and (u = 0 with v = 0, or v < N,
   p does not divide u and 0 < u < p^(N - v)). The numbers below are decimal or hexadecimal values
   written out: 1c = 28, 19 = 25, 3e = 62, 5 = 5, 4 = 4. */
static const tcase lball_cases[] = {
    /* canonical */
    {"adf1 Q lball 5 b 3 0 4", ADF_OK},
    {"adf1 Q lball 2 b 1 0 3", ADF_OK},
    {"adf1 Q lball 5 b 0 0 4", ADF_OK},            /* the ball O(5^4) around 0 */
    {"adf1 Q lball 5 b 1 -1 2", ADF_OK},           /* u < 5^(2 + 1) */
    {"adf1 Q lball 5 x 1 3 0", ADF_OK},
    {"adf1 Q lball 5 x 1 1 -1", ADF_OK},           /* 5^-1 */
    {"adf1 Q lball 2 x 0 1 0", ADF_OK},            /* the exact 0 */
    {"adf1 Q lball 5 x -1 3 0", ADF_OK},           /* -1/3, a unit at 5 */
    /* not canonical: the predicate of 5.8 */
    {"adf1 Q lball 5 b 1c 0 2", ADF_DOMAIN},       /* 28 is not below 5^2 = 25 */
    {"adf1 Q lball 5 b 19 0 2", ADF_DOMAIN},       /* 25 is not below 25 */
    {"adf1 Q lball 5 b 3e 0 2", ADF_DOMAIN},       /* 62 is not below 25 */
    {"adf1 Q lball 5 b 5 0 4", ADF_DOMAIN},        /* p divides u */
    {"adf1 Q lball 5 b 0 1 4", ADF_DOMAIN},        /* u = 0 goes with v = 0 */
    {"adf1 Q lball 5 b 3 4 4", ADF_DOMAIN},        /* v < N fails */
    {"adf1 Q lball 5 b 3 0 -1", ADF_DOMAIN},       /* v < N fails */
    {"adf1 Q lball 5 b -1 0 4", ADF_DOMAIN},      /* 0 < u fails */
    {"adf1 Q lball 4 b 3 0 4", ADF_DOMAIN},        /* 4 is not prime */
    {"adf1 Q lball 1 b 1 0 4", ADF_DOMAIN},        /* 1 is not prime */
    {"adf1 Q lball 5 x 5 1 0", ADF_DOMAIN},        /* p divides the numerator */
    {"adf1 Q lball 5 x 1 5 0", ADF_DOMAIN},        /* p divides the denominator */
    {"adf1 Q lball 5 x 0 1 1", ADF_DOMAIN},        /* the exact 0 has v = 0 */
    {"adf1 Q lball 5 x 2 4 0", ADF_DOMAIN},        /* 2/4 is not in lowest terms */
    /* the grammar of 10.1 */
    {"adf1 Q lball 5 y 3 0 4", ADF_PARSE},
    {"adf1 Q lball 5 b 3 0", ADF_PARSE},
    {"adf1 Q lball 5 b 3 0 4 4", ADF_PARSE},
    {"adf1 Q lball 5 b 3  0 4", ADF_PARSE},
    {"adf1 Q lball 5 b 3 0 4 ", ADF_PARSE},
    {"adf1 Q lball 5 b 03 0 4", ADF_PARSE},
    {"adf1 Q lball 5 b 3 0 A", ADF_PARSE},
    {"adf1 Q lball 5 b 3 0 4\n", ADF_PARSE},
    {"adf1 Q lball", ADF_PARSE},
    /* the version and the field */
    {"adf2 Q lball 5 b 3 0 4", ADF_UNSUPPORTED},
    {"adf1 K lball 5 b 3 0 4", ADF_UNSUPPORTED}
};

ADF_TEST(lball_strictness)
{
    TYPED_RUN(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
              adf_lball_load_str, adf_lball_dump_inspect, adf_lball_dump_str, lball_cases,
              sizeof(lball_cases) / sizeof(lball_cases[0]));
}

/* Body "sball (n | r <arb> | c <acb>) <count> {<lb>}". The predicate of 5.9: the tag is the
   archimedean tag, the balls are canonical arbs (so finite), every lb satisfies 5.8 and the primes
   strictly increase. */
static const tcase sball_cases[] = {
    /* canonical */
    {"adf1 Q sball n 0", ADF_OK},
    {"adf1 Q sball n 2 2 x 0 1 0 5 b 3 0 4", ADF_OK},
    {"adf1 Q sball r 3 -1 0 0 1 2 b 1 0 3", ADF_OK},
    {"adf1 Q sball r 1 0 0 0 0", ADF_OK},
    {"adf1 Q sball c 1 0 0 0 1 1 0 0 1 7 b 1 0 2", ADF_OK},
    {"adf1 Q sball r 3 -1 0 0 2 3 x 1 1 0 5 b 3 0 4", ADF_OK},
    /* the real ball (10.2) */
    {"adf1 Q sball r 0 0 0 0 0", ADF_OK},          /* a real ball of 0 is finite: 5.9 asks nothing
                                                       else of it (only the imaginary part of the
                                                       tag REAL is the exact 0) */
    {"adf1 Q sball r 0 -1 0 0 0", ADF_DOMAIN},     /* +inf: not finite */
    {"adf1 Q sball r 4 0 0 0 0", ADF_DOMAIN},      /* an even midpoint mantissa */
    {"adf1 Q sball r 1 0 0 1 0", ADF_DOMAIN},      /* a zero radius with an exponent */
    {"adf1 Q sball c 1 0 0 0 2 0 0 0 0", ADF_DOMAIN}, /* an even imaginary mantissa */
    /* a local ball that breaks 5.8 */
    {"adf1 Q sball n 1 4 b 3 0 4", ADF_DOMAIN},
    {"adf1 Q sball n 1 5 b 3 0 -1", ADF_DOMAIN},
    /* the primes must strictly increase (5.9) */
    {"adf1 Q sball r 1 0 0 0 2 5 b 3 0 4 3 x 1 1 0", ADF_DOMAIN},
    {"adf1 Q sball r 1 0 0 0 2 5 b 3 0 4 5 b 3 0 4", ADF_DOMAIN},
    /* the grammar of 10.1 */
    {"adf1 Q sball m 0", ADF_PARSE},
    {"adf1 Q sball r 1 0 0 0 1 5 b 3 0 4 5 b 3 0 4", ADF_PARSE},   /* the count says one lb */
    {"adf1 Q sball n 2 2 x 0 1 0", ADF_PARSE},                    /* one lb is missing */
    {"adf1 Q sball n 1 2 x 0 1 0 5 b 3 0 4 7", ADF_PARSE},        /* a stray token after one lb */
    {"adf1 Q sball n 01", ADF_PARSE},
    {"adf1 Q sball c 1 0 0 0 1 0", ADF_PARSE},                    /* half an acb */
    /* the version and the field */
    {"adf2 Q sball n 0", ADF_UNSUPPORTED},
    {"adf1 K sball n 0", ADF_UNSUPPORTED}
};

ADF_TEST(sball_strictness)
{
    TYPED_RUN(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
              adf_sball_load_str, adf_sball_dump_inspect, adf_sball_dump_str, sball_cases,
              sizeof(sball_cases) / sizeof(sball_cases[0]));
}

/* ------------------------------------------------------------------ the limits and the order */

/* One text with a limit and its status, through one loader and its inspector.  The value is left
   at the sentinel on every status other than ADF_OK. */
#define LIMIT_CASE(TY, IDENT, INIT, SET, CLEAR, LOAD, INSP, text, lim, want)                           \
    do                                                                                                  \
    {                                                                                                   \
        TY x, keep;                                                                                     \
        size_t _len = strlen(text), _nc = 7;                                                             \
        int _st;                                                                                         \
        INIT(x);                                                                                         \
        INIT(keep);                                                                                      \
        SET(x);                                                                                          \
        SET(keep);                                                                                       \
        _st = LOAD(x, text, _len, NULL, lim);                                                            \
        ADF_CHECK_MSG(_st == (want), "load \"%s\": %s, expected %s", text, adf_status_str(_st),           \
                      adf_status_str(want));                                                              \
        if (_st != ADF_OK)                                                                               \
            ADF_CHECK_MSG(IDENT(x, keep), "load \"%s\" touched the output", text);                       \
        ADF_CHECK_MSG(INSP(&_nc, NULL, text, _len, lim) == _st && (_st == ADF_OK ? _nc == 0 : _nc == 7), \
                      "inspect \"%s\": %s, loader %s", text,                                              \
                      adf_status_str(INSP(&_nc, NULL, text, _len, lim)), adf_status_str(_st));           \
        CLEAR(x);                                                                                        \
        CLEAR(keep);                                                                                     \
    }                                                                                                   \
    while (0)

ADF_TEST(limits_and_order_of_checks)
{
    /* max_prec bounds |v| and |N| of every local ball (conventions 8.4), a limit of stage 4: it
       comes before the predicate of 5.8 and before the word restriction of stage 5.  0x186a1 is
       100001, 0x19 is 25, 0x186a0 is 100000. */
    {
        adf_text_limits_t small;
        const char * p100001 = "adf1 Q lball 5 b 1 0 186a1";      /* |N| above max_prec = 10 */
        const char * p100000 = "adf1 Q lball 5 b 1 0 186a0";
        const char * bad_prime = "adf1 Q lball 4 b 3 0 186a1";    /* LIMIT and DOMAIN */
        const char * wide = "adf1 Q lball 10000000000000000 b 1 0 186a1";   /* LIMIT and UNSUPPORTED */
        const char * centre = "adf1 Q lball 5 b 1c 0 186a1";      /* LIMIT and DOMAIN */
        const char * zero = "adf1 Q lball 5 b 03 0 186a1";        /* PARSE before LIMIT */

        adf_text_limits_default(&small);
        small.max_prec = 10;
        ADF_CHECK(small.max_prec == 10);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, p100001, &small, ADF_LIMIT);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, bad_prime, &small, ADF_LIMIT);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, wide, &small, ADF_LIMIT);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, centre, &small, ADF_LIMIT);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, zero, &small, ADF_PARSE);
        /* With the default limits (max_prec = 100000) the first text is still over, and texts
           whose exponents are small decide by the word restriction and by the predicate. */
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, p100001, NULL, ADF_LIMIT);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, "adf1 Q lball 4 b 3 0 4", NULL, ADF_DOMAIN);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, "adf1 Q lball 10000000000000000 b 1 0 4",
                   NULL, ADF_UNSUPPORTED);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, "adf1 Q lball 5 b 1c 0 2", NULL,
                   ADF_DOMAIN);
        LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
                   adf_lball_load_str, adf_lball_dump_inspect, p100000, NULL, ADF_OK);
    }
    /* The bound of max_prec applies to every local ball of a partial ball, before any predicate:
       the first of the two below is over it, the second is not. */
    {
        adf_text_limits_t small;
        const char * first_over = "adf1 Q sball n 2 2 x 0 1 186a1 5 b 3 0 4";
        const char * second_over = "adf1 Q sball n 2 2 x 0 1 0 5 b 3 0 186a1";
        const char * good = "adf1 Q sball n 2 2 x 0 1 0 5 b 3 0 4";
        const char * bad_prime = "adf1 Q sball n 2 2 x 0 1 0 4 b 3 0 4";   /* 4 is not prime */

        adf_text_limits_default(&small);
        small.max_prec = 10;
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, first_over, &small, ADF_LIMIT);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, second_over, &small, ADF_LIMIT);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, good, &small, ADF_OK);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, bad_prime, &small, ADF_DOMAIN);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, bad_prime, NULL, ADF_DOMAIN);
    }
    /* max_items bounds the number of places of a partial ball (conventions 8.4): a limit of
       stage 4, before the predicate of 5.8 of every local ball. */
    {
        adf_text_limits_t few;
        const char * two = "adf1 Q sball n 2 2 x 0 1 0 4 b 3 0 4";    /* 4 is not prime */
        const char * one = "adf1 Q sball n 1 5 b 3 0 4";

        adf_text_limits_default(&few);
        few.max_items = 1;
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, two, &few, ADF_LIMIT);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, two, NULL, ADF_DOMAIN);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, one, &few, ADF_OK);
        /* max_items = 0 admits the empty set of places only */
        few.max_items = 0;
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball n 0", &few, ADF_OK);
        LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
                   adf_sball_load_str, adf_sball_dump_inspect, one, &few, ADF_LIMIT);
    }
    /* max_len is a limit of stage 1 (8.5 item 1), before the version, the grammar and the
       predicate. */
    {
        adf_text_limits_t shortlim;
        const char * texts[] = {"adf2 Q lball 5 b 3 0 4", "adf1 Q lball 4 b 3 0 4",
                                "adf1 Q lball 5 b 3 0", "adf1 Q lball", "adf1 Q sball n 0"};
        size_t i;

        adf_text_limits_default(&shortlim);
        shortlim.max_len = 8;
        for (i = 0; i < sizeof(texts) / sizeof(texts[0]); i++)
        {
            adf_lball_t x, keep;
            size_t nc = 7, len = strlen(texts[i]);

            adf_lball_init(x);
            adf_lball_init(keep);
            lball_sentinel(x);
            lball_sentinel(keep);
            ADF_CHECK_MSG(adf_lball_load_str(x, texts[i], len, NULL, &shortlim) == ADF_LIMIT,
                          "load \"%s\" with max_len 8", texts[i]);
            ADF_CHECK(adf_lball_dump_inspect(&nc, NULL, texts[i], len, &shortlim) == ADF_LIMIT && nc == 7);
            ADF_CHECK(adf_lball_identical(x, keep));
            adf_lball_clear(x);
            adf_lball_clear(keep);
        }
    }
    /* The header is read before the body, and the grammar of the body before its predicate. */
    {
        adf_lball_t x, keep;
        adf_lball_init(x);
        adf_lball_init(keep);
        lball_sentinel(x);
        lball_sentinel(keep);
        ADF_CHECK(adf_lball_load_str(x, "adf1 K lball 5 b 3 0 zz", strlen("adf1 K lball 5 b 3 0 zz"), NULL,
                                    NULL)
                  == ADF_UNSUPPORTED);
        ADF_CHECK(adf_lball_load_str(x, "adf3 Q lball 4 b 3 0 4", strlen("adf3 Q lball 4 b 3 0 4"), NULL,
                                    NULL)
                  == ADF_UNSUPPORTED);
        ADF_CHECK(adf_lball_load_str(x, "adf1 Q lball 4 b 3 0 04", strlen("adf1 Q lball 4 b 3 0 04"), NULL,
                                    NULL)
                  == ADF_PARSE);
        ADF_CHECK(adf_lball_load_str(x, "adf1 Q lball 4 b 3 0 4", strlen("adf1 Q lball 4 b 3 0 4"), NULL,
                                    NULL)
                  == ADF_DOMAIN);
        /* a dump of another type is not a sentence of the grammar of this loader (9.7) */
        ADF_CHECK(adf_lball_load_str(x, "adf1 Q sball n 0", strlen("adf1 Q sball n 0"), NULL, NULL)
                  == ADF_PARSE);
        ADF_CHECK(adf_lball_identical(x, keep));
        adf_lball_clear(x);
        adf_lball_clear(keep);
    }
    /* A NUL byte inside the text (8.1) and a byte above 0x7e (8.2) are ADF_PARSE. */
    {
        static const char nul_text[] = "adf1 Q lball 5\0 b 3 0 4";
        static const char high_text[] = "adf1 Q lball 5 \xc2\xb1 3 0 4";
        adf_lball_t x, keep;

        adf_lball_init(x);
        adf_lball_init(keep);
        lball_sentinel(x);
        lball_sentinel(keep);
        ADF_CHECK(adf_lball_load_str(x, nul_text, sizeof(nul_text) - 1, NULL, NULL) == ADF_PARSE);
        ADF_CHECK(adf_lball_load_str(x, high_text, sizeof(high_text) - 1, NULL, NULL) == ADF_PARSE);
        ADF_CHECK(adf_lball_identical(x, keep));
        adf_lball_clear(x);
        adf_lball_clear(keep);
    }
    /* The bindings of a body with no occurrence: the count must be 0 (10.2, G3). */
    {
        adf_lball_t x, y;
        adf_lball_init(x);
        adf_lball_init(y);
        ADF_CHECK(adf_lball_load_str_binds(x, "adf1 Q lball 5 b 3 0 4", strlen("adf1 Q lball 5 b 3 0 4"),
                                          NULL, 0, NULL)
                  == ADF_OK);
        adf_lball_set(y, x);                 /* the value of the successful load */
        ADF_CHECK(adf_lball_identical(x, y));
        ADF_CHECK(adf_lball_load_str_binds(x, "adf1 Q lball 5 b 3 0 4", strlen("adf1 Q lball 5 b 3 0 4"),
                                          NULL, 1, NULL)
                  == ADF_DOMAIN);
        ADF_CHECK(adf_lball_identical(x, y));
        adf_lball_clear(x);
        adf_lball_clear(y);
    }
    /* Any context is accepted by the one-context form when the body has no occurrence. */
    {
        adf_lball_t x;
        adf_modctx_struct * ctx = NULL;
        fmpz_t K;

        fmpz_init_set_ui(K, 6);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK && ctx != NULL);
        adf_lball_init(x);
        ADF_CHECK(adf_lball_load_str(x, "adf1 Q lball 5 b 3 0 4", strlen("adf1 Q lball 5 b 3 0 4"), ctx,
                                    NULL)
                  == ADF_OK);
        ADF_CHECK(adf_lball_load_str(x, "adf1 Q lball 5 b 3 0 4", strlen("adf1 Q lball 5 b 3 0 4"), NULL,
                                    NULL)
                  == ADF_OK);
        adf_lball_clear(x);
        adf_modctx_free(ctx);
        fmpz_clear(K);
    }
}

/* The fields v and N of a dumped local ball against the limits that decide them: max_prec
   (conventions 8.4) and the word of the type (lball.h:85-88). The texts below are written with
   max_prec = LONG_MAX, so that the word and not the limit of the caller decides.

   ADF_LBALL_EXP_MAX = 2^60 of lball.h:67 is not a bound of the dump loader, and this test pins
   that: the predicate of conventions 5.8, which the header states for the struct (lball.h:76-80)
   and which conventions 10.2 makes the condition of a strict loader, bounds neither v nor N, and
   adf_lball_is_canonical does not test the bound. So 2^60, 2^60 + 1 and 2^63 - 1 all load, and the
   value form reader of adf_lball_set_str (text.h:304, 331), which does refuse above 2^60, is the
   outlier; lanes/u-repair1/report.md reports the asymmetry for the orchestrator.

   -2^63 = -8000000000000000 is the one value the review u-review1 (finding F3) named: it fits an
   slong, but its absolute value 2^63 is above every max_prec (an slong), so the loader answers
   ADF_LIMIT for it in every build and under every limits struct, and no change of dp_fits_si can
   make that text load. The largest field that does load is 7fffffffffffffff = 2^63 - 1 with
   max_prec = LONG_MAX. dp_fits_si now states the word it means (a negative token of sixteen digits
   whose first digit is 8 is -2^63 and fits), which the limits of the caller make unreachable. */
ADF_TEST(lball_exponent_word_and_prec)
{
    adf_text_limits_t wide;
    const char * below = "adf1 Q lball 5 b 1 0 fffffffffffffff";          /* N = 2^60 - 1 */
    const char * at_2_60 = "adf1 Q lball 5 b 1 0 1000000000000000";       /* N = 2^60 */
    const char * over_2_60 = "adf1 Q lball 5 b 1 0 1000000000000001";     /* N = 2^60 + 1 */
    const char * neg_at_2_60 = "adf1 Q lball 5 b 1 -1000000000000000 0";  /* v = -2^60 */
    const char * neg_over = "adf1 Q lball 5 b 1 -fffffffffffffff 0";       /* v = -(2^60 - 1) */
    const char * exact_min = "adf1 Q lball 5 x 1 1 -8000000000000000";    /* v = -2^63, exact */
    const char * ball_min = "adf1 Q lball 5 b 0 0 -8000000000000000";     /* N = -2^63 */
    const char * word_max = "adf1 Q lball 5 b 0 0 7fffffffffffffff";       /* N = 2^63 - 1 */
    const char * over_word = "adf1 Q lball 5 b 0 0 8000000000000000";     /* N = 2^63 */
    const char * neg_over_word = "adf1 Q lball 5 b 1 -8000000000000000 0"; /* v = -2^63 - 1 */

    adf_text_limits_default(&wide);
    wide.max_prec = LONG_MAX;
    ADF_CHECK(wide.max_prec == LONG_MAX);
    /* every field that fits an slong and is under max_prec is a field of a value of the type */
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, below, &wide, ADF_OK);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, at_2_60, &wide, ADF_OK);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, over_2_60, &wide, ADF_OK);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, neg_at_2_60, &wide, ADF_OK);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, neg_over, &wide, ADF_OK);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, word_max, &wide, ADF_OK);
    /* -2^63 is the least slong, but its absolute value 2^63 is above every limit of the caller
       (max_prec is an slong), so it is ADF_LIMIT whatever dp_fits_si says of the word */
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, exact_min, &wide, ADF_LIMIT);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, ball_min, &wide, ADF_LIMIT);
    /* the word: 2^63 does not fit an slong, and -2^63 - 1 is not a value at all */
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, over_word, &wide, ADF_LIMIT);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, neg_over_word, &wide, ADF_LIMIT);
    /* max_prec is the limit of the caller and comes first: the same texts under the default. */
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, over_2_60, NULL, ADF_LIMIT);
    LIMIT_CASE(adf_lball_t, adf_lball_identical, adf_lball_init, lball_sentinel, adf_lball_clear,
               adf_lball_load_str, adf_lball_dump_inspect, below, NULL, ADF_LIMIT);
    /* The largest field that loads is a field of a value of the type: it is canonical, it dumps
       back to the text it came from, and loading that dump gives an identical value. */
    {
        adf_lball_t x, y;
        size_t len, len2;
        char * t, * t2;

        adf_lball_init(x);
        adf_lball_init(y);
        lball_sentinel(x);
        ADF_CHECK(adf_lball_load_str(x, word_max, strlen(word_max), NULL, &wide) == ADF_OK);
        ADF_CHECK(adf_lball_is_canonical(x) && x->N == LONG_MAX);
        t = adf_lball_dump_str(&len, x);
        ADF_CHECK_MSG(t != NULL && len == strlen(word_max) && memcmp(t, word_max, len) == 0,
                      "the dump of the loaded value is \"%s\"", t);
        ADF_CHECK(adf_lball_load_str(y, t, len, NULL, &wide) == ADF_OK);
        ADF_CHECK(adf_lball_identical(x, y));
        t2 = adf_lball_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t2, t, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_lball_clear(x);
        adf_lball_clear(y);
    }
}

/* The exponents of every local ball of a partial ball, and the radius mantissa of its
   archimedean component. The exponents are decided as for one local ball (lball_exponent_word_and
   _prec above: max_prec, then the word of the type; ADF_LBALL_EXP_MAX is no bound of the dump
   loader). The radius mantissa: 10.2 wants an odd mantissa below 2^30 (mag.h:117), with the tag r
   and with the tag c; the texts of the review u-review1 (finding F5) use the even midpoint 40,
   which masks the radius, so the midpoint 5 (the ball [3, 7]) is used as well. */
ADF_TEST(sball_exponent_and_radius_mantissa)
{
    adf_text_limits_t wide;

    adf_text_limits_default(&wide);
    wide.max_prec = LONG_MAX;
    /* the exponents of a local ball of the partial ball, at the ends of the word */
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball n 1 5 b 1 0 1000000000000001",
               &wide, ADF_OK);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect,
               "adf1 Q sball n 2 2 x 0 1 0 5 b 1 0 1000000000000001", &wide, ADF_OK);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball n 1 5 x 1 1 -7fffffffffffffff",
               &wide, ADF_OK);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball n 1 5 x 1 1 -8000000000000000",
               &wide, ADF_LIMIT);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball n 1 5 b 1 0 8000000000000000",
               &wide, ADF_LIMIT);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball n 1 5 b 1 0 1000000000000001",
               NULL, ADF_LIMIT);
    /* the radius mantissa: 2^30 + 1 is not below 2^30, and 2 is not odd */
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball r 40 0 40000001 0 0", NULL,
               ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball r 40 0 2 0 0", NULL,
               ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball r 5 0 40000001 0 0", NULL,
               ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball r 5 0 2 0 0", NULL,
               ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball c 40 0 40000001 0 0 0 0 0 0",
               NULL, ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball c 40 0 2 0 0 0 0 0 0", NULL,
               ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball c 5 0 40000001 0 1 0 0 0 0",
               NULL, ADF_DOMAIN);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball c 5 0 2 0 1 0 0 0 0", NULL,
               ADF_DOMAIN);
    /* both sides of the radius bound: 2^30 - 1 is odd and below 2^30 */
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball r 5 0 3fffffff 0 0", NULL,
               ADF_OK);
    LIMIT_CASE(adf_sball_t, adf_sball_identical, adf_sball_init, sball_sentinel, adf_sball_clear,
               adf_sball_load_str, adf_sball_dump_inspect, "adf1 Q sball c 5 0 3fffffff 0 1 0 0 0 0",
               NULL, ADF_OK);
}

/* A survivor of the mutation run over src/dump.c: the check that a qclass of the form "pieces"
   with no piece is a semantic failure (conventions 5.10) must not run in a stage of 8.5 other than
   stage 6, or a text with a syntax error after that count would answer DOMAIN instead of PARSE.
   The text is read by adf_modctx_new_from_dump, which validates a dump of any body (10.2). */
ADF_TEST(qclass_stage_of_the_pieces_check)
{
    adf_modctx_struct * c = NULL;
    /* the semantic failure itself: no piece is not canonical (conventions 5.10) */
    const char * semantic = "adf1 Q qclass pieces 0";
    /* a token after the count is a grammar failure, and it must be read as one */
    const char * texts[] = {"adf1 Q qclass pieces 0 x", "adf1 Q qclass pieces 1",
                            "adf1 Q qclass pieces 0 ", "adf1 Q qclass lift 1"};

    ADF_CHECK(adf_modctx_new_from_dump(&c, semantic, strlen(semantic), 0, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_modctx_new_from_dump(&c, texts[0], strlen(texts[0]), 0, NULL) == ADF_PARSE);
    ADF_CHECK(adf_modctx_new_from_dump(&c, texts[1], strlen(texts[1]), 0, NULL) == ADF_PARSE);
    ADF_CHECK(adf_modctx_new_from_dump(&c, texts[2], strlen(texts[2]), 0, NULL) == ADF_PARSE);
    ADF_CHECK(adf_modctx_new_from_dump(&c, texts[3], strlen(texts[3]), 0, NULL) == ADF_PARSE);
    adf_modctx_free(c);
}

/* The dumped text of values made by hand, so that a change of the dump form is noticed. The
   mantissa of a dyadic number is the odd one that arf_get_fmpz_2exp returns (conventions 10.2). */
static void
check_text(const char * what, char * t, size_t len, const char * want)
{
    ADF_CHECK_MSG(t != NULL && len == strlen(want) && memcmp(t, want, len) == 0,
                  "%s: dumped \"%s\", expected \"%s\"", what, t != NULL ? t : "(null)", want);
    adf_str_free(t);
}

ADF_TEST(dump_texts_of_hand_values)
{
    adf_lball_t l;
    adf_sball_t s;
    size_t len;
    char * t;

    adf_lball_init(l);
    adf_sball_init(s);
    lball_sentinel(l);
    sball_sentinel(s);
    /* the ball 3 + 5^4 Z_5, and the partial ball 5 + (3 + 5^4 Z_5) at the place 5 */
    t = adf_lball_dump_str(&len, l);
    check_text("lball", t, len, "adf1 Q lball 5 b 3 0 4");
    t = adf_sball_dump_str(&len, s);
    check_text("sball", t, len, "adf1 Q sball r 5 0 0 0 1 5 b 3 0 4");
    adf_lball_clear(l);
    adf_sball_clear(s);
}
