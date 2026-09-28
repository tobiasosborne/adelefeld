/* tests/test_dump_limits.c: the two limits of the dump reader that the review of lane m1-dump
   found (R1: the cap of decision M1-D5 on the block count of a context occurrence; R2: the
   bound on the binary exponents of a piece of a qclass, decision M1-D9, a limit of stage 4).

   Contract: docs/SPEC.md 15 rows M1-D5 and M1-D9; include/adelefeld/modctx.h ("Size (decision
   M1-D5)" and adf_modctx_new_from_dump); include/adelefeld/dump.h (statuses, order of 8.5);
   docs/conventions.md 8.2 (alphabet), 8.4 (limits), 8.5 (order of checks, CV-28), 10.1, 10.2,
   3.3 (DOMAIN before UNSUPPORTED).

   What is claimed and checked, each with the status worked out from the contract in a comment:

   R1, the cap of M1-D5 (65536 blocks).  The cap is a restriction of the implementation and
   therefore a word restriction of stage 5 of conventions 8.5: it is decided from the count k of
   the text alone, before the blocks are read into integers, before the coprimality of stage 6
   and before any allocation in proportion to k.  So a text with 65537 blocks is ADF_UNSUPPORTED
   at every entry point (adf_modctx_new_from_dump, the five inspectors, the four typed loaders
   that have an occurrence) with the outputs untouched and within one second, and a text whose
   first block is not a word is ADF_UNSUPPORTED as well, while the same text with 65536 blocks
   is ADF_DOMAIN (the predicate of 5.14 fails).  A count of 2^40 or of 2^64 in a text with a short
   body is a grammar failure of stage 3 (conventions 10.1: "a count fixes the number of
   repetitions that follow it ... a mismatch is ADF_PARSE"), so it is ADF_PARSE whatever
   max_items and whatever the cap: the cap is never reached.  The order of the two statuses is
   pinned with max_items below and above the cap.

   R2, the bound ADF_DUMP_QCLASS_EXP_MAX = 2^20 on the binary exponents of the real ball of a
   qclass piece (M1-D9).  It is a limit of stage 4, so it is applied to every piece on the digit
   strings, before any semantic check of stage 6: a text with a zero denominator, a negative
   midpoint, a context that fails its predicate, all of them with an exponent above the bound, is
   ADF_LIMIT and not ADF_DOMAIN.  At the bound the piece is valid and the text is ADF_OK; one
   hexadecimal digit above the bound, and an exponent of 40 hexadecimal digits, are ADF_LIMIT.
   The bound is compared as a number of any length, so no word bound hides behind it.  The other
   bodies have no such bound: a dumped adele whose real ball has an exponent of 40 hexadecimal
   digits loads and dumps back byte for byte.

   Slow cases.  The admitted case with 65536 blocks costs about 10 s through the inspectors and
   about 70 s through adf_modctx_new_from_dump, which builds the tables of the context (the two
   fmpz_multi_*_precompute calls of adf_modctx_alloc, about 33 s each on the lane machine).  The
   loaders and adf_modctx_new_from_dump at the cap are therefore run only when
   ADF_DUMP_LIMITS_FULL=1, as tests/test_modctx_limits.c gates its admitted cases; without it the
   same cap is exercised at 65536 blocks through the four inspectors.  This is said in the lane
   report. */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include "test_runner.h"

/* The bound of M1-D9.  The header does not define it (HEADER-FINDING, lane report), so the test
   writes the value the specification fixes. */
#define QCLASS_EXP_MAX (UWORD(1) << 20)

/* 40 hexadecimal digits, the width M1-D9 asks a test for; larger than 2^64, so a comparison
   that reads the exponent into a machine word would give a different answer. */
#define FORTY_F "ffffffffffffffffffffffffffffffffffffffff"

/* The text (s, len) of a literal, without a NUL counted in the length. */
static size_t
lit(const char * s)
{
    return strlen(s);
}

/* QS(text): the status of adf_modctx_new_from_dump on (text, strlen text), with *out untouched.
   It writes the number of blocks of a built context into the local `nb`, -1 when untouched. */
#define QS(str) from_dump_status((str), lit(str), NULL, &nb)

static double
now_seconds(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

/* ------------------------------------------------------------------ texts with k blocks */

/* The k first primes, and a dump of one context occurrence of k of them, in the body `kind`
   ("modctx", "fball", "scaled", "adele", "cadele"; conventions 10.1).  K is the product of the
   blocks, the residues are 0, the real balls are the exact zero ball and d = 1.  With `bad`,
   the first block is 20 hexadecimal digits, which is not a word, so that the predicate of
   conventions 5.14 fails at stage 6.  *len is the byte length and *blocks the offset of the
   first block token.  The caller frees the text with flint_free. */
static char *
ctx_text1(const char * kind, slong k, int bad, size_t * len, size_t * blocks)
{
    slong i;
    ulong p = 1;
    fmpz_t K;
    char * h, * t;
    size_t cap, n = 0;

    fmpz_init_set_ui(K, 1);
    for (i = 0; i < k; i++)
    {
        p = n_nextprime(p, 1);
        fmpz_mul_ui(K, K, p);
    }
    h = fmpz_get_str(NULL, 16, K);
    cap = 128 + 8 * ((size_t) k + 8) + (size_t) fmpz_sizeinbase(K, 16);
    t = (char *) flint_malloc(cap);
    if (strcmp(kind, "fball") == 0)
        n += (size_t) sprintf(t, "adf1 Q fball l 1 %s %lx", h, (unsigned long) k);
    else if (strcmp(kind, "scaled") == 0)
        n += (size_t) sprintf(t, "adf1 Q scaled x 0 1 %s %lx", h, (unsigned long) k);
    else if (strcmp(kind, "adele") == 0)
        n += (size_t) sprintf(t, "adf1 Q adele 1 0 0 0 0 l 1 %s %lx", h, (unsigned long) k);
    else if (strcmp(kind, "cadele") == 0)
        n += (size_t) sprintf(t, "adf1 Q cadele 1 0 0 0 0 0 0 0 0 l 1 %s %lx", h, (unsigned long) k);
    else
        n += (size_t) sprintf(t, "adf1 Q modctx %s %lx", h, (unsigned long) k);
    flint_free(h);
    fmpz_clear(K);
    *blocks = n;
    p = 1;
    for (i = 0; i < k; i++)
    {
        p = n_nextprime(p, 1);
        if (i == 0 && bad)
            n += (size_t) sprintf(t + n, " 1fffffffffffffffffff");  /* 20 digits: not a word */
        else
            n += (size_t) sprintf(t + n, " %lx", (unsigned long) p);
    }
    if (strcmp(kind, "fball") == 0 || strcmp(kind, "adele") == 0 || strcmp(kind, "cadele") == 0)
        for (i = 0; i < k; i++)
            n += (size_t) sprintf(t + n, " 0");
    t[n] = '\0';
    *len = n;
    return t;
}

/* ctx_text1 with valid word blocks. */
static char *
ctx_text(const char * kind, slong k, size_t * len, size_t * blocks)
{
    return ctx_text1(kind, k, 0, len, blocks);
}

/* ------------------------------------------------------------------ entry points */

/* adf_modctx_new_from_dump at occurrence 0 with a sentinel output (conventions 4.6: *out is
   untouched on every status other than ADF_OK). *nblocks is -1 when the output was untouched. */
static int
from_dump_status(const char * s, size_t len, const adf_text_limits_t * lim, long * nblocks)
{
    adf_modctx_struct * sentinel = (adf_modctx_struct *) (void *) 0x40, * c = sentinel;
    int st = adf_modctx_new_from_dump(&c, s, len, 0, lim);

    *nblocks = c == sentinel ? -1 : (long) adf_modctx_nblocks(c);
    if (c != sentinel)
        adf_modctx_free(c);
    return st;
}

/* The status that the inspector of the type i must give on a text of the body `kind`: the
   inspector of the type of the body gives the status of the validation, and every other
   inspector ADF_PARSE, since a dump of another type is not a sentence of its start symbol. */
static int
inspector_status(int i, const char * kind, int st)
{
    static const char * const names[] = {"rat", "fball", "scaled", "adele", "cadele"};

    return strcmp(kind, names[i]) == 0 ? st : ADF_PARSE;
}

/* Every inspector, with two initialised descriptors holding sentinels (closure C2: on a status
   other than ADF_OK, *nctx and every descriptor are untouched). */
static int
all_inspect(const char * s, size_t len, const adf_text_limits_t * lim, const char * kind, int st)
{
    adf_ctx_desc_t d[2];
    size_t i;

    adf_ctx_desc_init(&d[0]);
    adf_ctx_desc_init(&d[1]);
    for (i = 0; i < 5; i++)
    {
        size_t n = 2;
        int r, want = inspector_status((int) i, kind, st);

        adf_ctx_desc_clear(&d[0]);
        adf_ctx_desc_clear(&d[1]);
        adf_ctx_desc_init(&d[0]);
        adf_ctx_desc_init(&d[1]);
        fmpz_set_ui(d[0].K, 55);
        fmpz_set_ui(d[1].K, 56);
        d[0].k = -1;
        d[1].k = -2;
        switch (i)
        {
            case 0: r = adf_rat_dump_inspect(&n, d, s, len, lim); break;
            case 1: r = adf_fball_dump_inspect(&n, d, s, len, lim); break;
            case 2: r = adf_scaled_dump_inspect(&n, d, s, len, lim); break;
            case 3: r = adf_adele_dump_inspect(&n, d, s, len, lim); break;
            default: r = adf_cadele_dump_inspect(&n, d, s, len, lim); break;
        }
        ADF_CHECK_MSG(r == want, "inspector %zu: %s, expected %s", i, adf_status_str(r),
                      adf_status_str(want));
        if (r != ADF_OK)
            ADF_CHECK_MSG(n == 2 && d[0].k == -1 && d[1].k == -2 && fmpz_equal_ui(d[0].K, 55)
                          && fmpz_equal_ui(d[1].K, 56) && d[0].q == NULL && d[1].q == NULL,
                          "inspector %zu wrote its output on status %s", i, adf_status_str(r));
    }
    adf_ctx_desc_clear(&d[0]);
    adf_ctx_desc_clear(&d[1]);
    return st;
}

/* The four typed loaders that can carry a context occurrence, each over a value that is not the
   one of the text (conventions 4.3: the output is untouched on every status other than ADF_OK).
   The context of the text is built first; -1 is returned when the text is refused, so that the
   test can claim that no binding was available. */
static int
all_load(const char * s, size_t len, const adf_text_limits_t * lim, const char * kind, int st)
{
    adf_modctx_struct * c = NULL;
    adf_fball_t f;
    adf_scaled_t sc;
    adf_adele_t ad;
    adf_cadele_t cd;
    adf_fball_struct fcopy;
    fmpq_t scopy;
    fmpz_t ucopy;
    adf_adele_struct acopy;
    adf_cadele_struct ccopy;
    int r;

    if (adf_modctx_new_from_dump(&c, s, len, 0, lim) != ADF_OK)
        return -1;

    memset(&fcopy, 0, sizeof(fcopy));
    adf_fball_init(f);
    fmpz_set_si(f->A, 5);
    fmpz_set_si(f->H, 18);
    memcpy(&fcopy, f, sizeof(fcopy));
    r = adf_fball_load_str(f, s, len, c, lim);
    ADF_CHECK_MSG(r == (strcmp(kind, "fball") == 0 ? st : ADF_PARSE), "fball loader: %s",
                  adf_status_str(r));
    if (r != ADF_OK)
        ADF_CHECK_MSG(memcmp(&fcopy, f, sizeof(fcopy)) == 0, "fball written on status %s",
                      adf_status_str(r));
    adf_fball_clear(f);

    memset(&acopy, 0, sizeof(acopy));
    adf_adele_init(ad);
    arb_set_si(ad->inf, 3);
    memcpy(&acopy, ad, sizeof(acopy));
    r = adf_adele_load_str(ad, s, len, c, lim);
    ADF_CHECK_MSG(r == (strcmp(kind, "adele") == 0 ? st : ADF_PARSE), "adele loader: %s",
                  adf_status_str(r));
    if (r != ADF_OK)
        ADF_CHECK_MSG(memcmp(&acopy, ad, sizeof(acopy)) == 0, "adele written on status %s",
                      adf_status_str(r));
    adf_adele_clear(ad);

    memset(&ccopy, 0, sizeof(ccopy));
    adf_cadele_init(cd);
    acb_set_si(cd->inf, 3);
    memcpy(&ccopy, cd, sizeof(ccopy));
    r = adf_cadele_load_str(cd, s, len, c, lim);
    ADF_CHECK_MSG(r == (strcmp(kind, "cadele") == 0 ? st : ADF_PARSE), "cadele loader: %s",
                  adf_status_str(r));
    if (r != ADF_OK)
        ADF_CHECK_MSG(memcmp(&ccopy, cd, sizeof(ccopy)) == 0, "cadele written on status %s",
                      adf_status_str(r));
    adf_cadele_clear(cd);

    adf_scaled_init(sc, c);
    fmpq_init(scopy);
    fmpz_init(ucopy);
    fmpq_set_si(scopy, -3, 5);
    fmpz_set_si(ucopy, 11);
    fmpq_set(sc->s, scopy);
    fmpz_set(sc->u, ucopy);
    r = adf_scaled_load_str(sc, s, len, c, lim);
    ADF_CHECK_MSG(r == (strcmp(kind, "scaled") == 0 ? st : ADF_PARSE), "scaled loader: %s",
                  adf_status_str(r));
    if (r != ADF_OK)
        ADF_CHECK_MSG(fmpq_equal(sc->s, scopy) && fmpz_equal(sc->u, ucopy) && sc->mctx == c,
                      "scaled written on status %s", adf_status_str(r));
    fmpq_clear(scopy);
    fmpz_clear(ucopy);
    adf_scaled_clear(sc);
    adf_modctx_free(c);
    return st;
}

/* ------------------------------------------------------------------ R1: the cap of M1-D5 */

ADF_TEST(block_cap_refuses_65537_blocks_everywhere)
{
    static const char * const kinds[] = {"modctx", "fball", "scaled", "adele", "cadele"};
    size_t len, blocks, i;
    double t0, t1;
    long nb = -1;

    /* stage 5 gives ADF_UNSUPPORTED to more than ADF_MODCTX_MAX_BLOCKS blocks, so every entry
       point refuses 65537 blocks, whatever the body, with its output untouched. */
    for (i = 0; i < 5; i++)
    {
        char * t = ctx_text(kinds[i], 65537, &len, &blocks);

        t0 = now_seconds();
        ADF_CHECK_MSG(from_dump_status(t, len, NULL, &nb) == ADF_UNSUPPORTED, "%s: from_dump",
                      kinds[i]);
        ADF_CHECK_MSG(nb == -1, "%s: from_dump wrote its output", kinds[i]);
        t1 = now_seconds();
        ADF_CHECK_MSG(t1 - t0 < 1.0, "%s: from_dump took %.3f s for 65537 blocks", kinds[i], t1 - t0);

        all_inspect(t, len, NULL, kinds[i], ADF_UNSUPPORTED);
        ADF_CHECK_MSG(all_load(t, len, NULL, kinds[i], ADF_UNSUPPORTED) == -1,
                      "%s: a context was built for a refused text", kinds[i]);
        flint_free(t);
    }
    /* the same for a context nested in a piece of a qclass */
    {
        char * t = ctx_text("modctx", 65537, &len, &blocks);
        char * u = (char *) flint_malloc(len + 2 * 65537 + 64);
        size_t n = (size_t) sprintf(u, "adf1 Q qclass pieces 1 1 1 0 0 0 l 1 %s", t + 14);

        for (i = 0; i < 65537; i++)
            n += (size_t) sprintf(u + n, " 0");
        u[n] = '\0';
        ADF_CHECK(from_dump_status(u, n, NULL, &nb) == ADF_UNSUPPORTED);
        ADF_CHECK(nb == -1);
        flint_free(u);
        flint_free(t);
    }
    /* and a text that also fails the predicate of its context: the cap is decided from the
       count alone, before the coprimality of stage 6, so the word restriction of stage 5 wins. */
    {
        char * t = ctx_text1("modctx", 65537, 1, &len, &blocks);

        ADF_CHECK(from_dump_status(t, len, NULL, &nb) == ADF_UNSUPPORTED);
        ADF_CHECK(nb == -1);
        flint_free(t);
    }
    ADF_CHECK(ADF_MODCTX_MAX_BLOCKS == 65536);
}

ADF_TEST(block_cap_accepts_65536_blocks)
{
    static const char * const kinds[] = {"fball", "scaled", "adele", "cadele"};
    size_t len, blocks, i;

    /* at the cap the text is admitted: the four typed inspectors validate it and report one
       occurrence, whose descriptor carries the modulus and the 65536 blocks. */
    for (i = 0; i < 4; i++)
    {
        adf_ctx_desc_t d;
        size_t nctx = 7;
        char * t = ctx_text(kinds[i], 65536, &len, &blocks);

        adf_ctx_desc_init(&d);
        switch (i)
        {
            case 0:
                ADF_CHECK(adf_fball_dump_inspect(&nctx, &d, t, len, NULL) == ADF_OK);
                break;
            case 1:
                ADF_CHECK(adf_scaled_dump_inspect(&nctx, &d, t, len, NULL) == ADF_OK);
                break;
            case 2:
                ADF_CHECK(adf_adele_dump_inspect(&nctx, &d, t, len, NULL) == ADF_OK);
                break;
            default:
                ADF_CHECK(adf_cadele_dump_inspect(&nctx, &d, t, len, NULL) == ADF_OK);
                break;
        }
        ADF_CHECK_MSG(nctx == 1 && d.k == 65536 && fmpz_bits(d.K) > 1000000, "%s: nctx %zu k %ld",
                      kinds[i], nctx, (long) d.k);
        ADF_CHECK(d.q != NULL && d.q[0] == 2 && d.q[65535] == 821641);
        adf_ctx_desc_clear(&d);
        flint_free(t);
    }
}

ADF_TEST(block_count_of_a_short_text_is_a_grammar_failure)
{
    /* conventions 10.1: a count fixes the number of repetitions that follow it, and a mismatch
       is ADF_PARSE.  A count of 2^40 or of 2^64 with a short body therefore fails at stage 3,
       before the count limit of stage 4 and before the cap of stage 5, whatever max_items. */
    adf_text_limits_t lim;
    long nb = -1;

    ADF_CHECK(QS("adf1 Q modctx 6 10000000000 2 3") == ADF_PARSE);
    ADF_CHECK(QS("adf1 Q modctx 6 10000000000000000 2 3") == ADF_PARSE);
    ADF_CHECK(QS("adf1 Q fball l 1 6 10000000000 2 3 0") == ADF_PARSE);
    ADF_CHECK(QS("adf1 Q scaled x 0 1 6 10000000000000000 2 3") == ADF_PARSE);
    ADF_CHECK(nb == -1);
    adf_text_limits_default(&lim);
    lim.max_items = 0;
    ADF_CHECK(from_dump_status("adf1 Q modctx 6 10000000000 2 3", 28, &lim, &nb) == ADF_PARSE);
    ADF_CHECK(from_dump_status("adf1 Q modctx 6 10000000000000000 2 3", 33, &lim, &nb) == ADF_PARSE);
    ADF_CHECK(nb == -1);
    all_inspect("adf1 Q modctx 6 10000000000 2 3", 28, &lim, "modctx", ADF_PARSE);
    all_inspect("adf1 Q fball l 1 6 10000000000 2 3 0", 34, NULL, "fball", ADF_PARSE);
}

ADF_TEST(block_cap_and_max_items_in_that_order)
{
    size_t len, blocks;
    adf_text_limits_t lim;
    long nb = -1;

    /* max_items is a limit of stage 4 and gives ADF_LIMIT; the cap of M1-D5 is a word
       restriction of stage 5 and gives ADF_UNSUPPORTED.  Stage 4 comes first. */
    adf_text_limits_default(&lim);
    lim.max_items = 65536;
    {
        char * t = ctx_text("modctx", 65537, &len, &blocks);

        ADF_CHECK_MSG(from_dump_status(t, len, &lim, &nb) == ADF_LIMIT,
                      "max_items at the cap: expected LIMIT (stage 4 before stage 5)");
        ADF_CHECK(nb == -1);
        flint_free(t);
    }
    {
        char * t = ctx_text("fball", 65537, &len, &blocks);

        lim.max_items = 65536;
        ADF_CHECK_MSG(all_inspect(t, len, &lim, "fball", ADF_LIMIT) == ADF_LIMIT,
                      "inspector with max_items at the cap");
        lim.max_items = 65537;         /* at the cap: only the cap of stage 5 is over */
        ADF_CHECK_MSG(all_inspect(t, len, &lim, "fball", ADF_UNSUPPORTED) == ADF_UNSUPPORTED,
                      "inspector with max_items above the cap");
        ADF_CHECK(all_load(t, len, &lim, "fball", ADF_UNSUPPORTED) == -1);
        flint_free(t);
    }
    /* the count limit of stage 4 is below the cap here, so a text with 11 blocks and
       max_items = 10 is ADF_LIMIT, and not ADF_UNSUPPORTED */
    {
        char * t = ctx_text("fball", 11, &len, &blocks);

        lim.max_items = 10;
        ADF_CHECK(all_inspect(t, len, &lim, "fball", ADF_LIMIT) == ADF_LIMIT);
        lim.max_items = 11;
        ADF_CHECK(all_inspect(t, len, &lim, "fball", ADF_OK) == ADF_OK);
        ADF_CHECK(all_load(t, len, &lim, "fball", ADF_OK) == ADF_OK);
        flint_free(t);
    }
    /* a negative max_items is exceeded by every count (8.4) */
    {
        char * t = ctx_text("fball", 1, &len, &blocks);

        lim.max_items = -1;
        ADF_CHECK(all_inspect(t, len, &lim, "fball", ADF_LIMIT) == ADF_LIMIT);
        flint_free(t);
    }
}

ADF_TEST(block_cap_before_the_predicate_of_the_context)
{
    size_t len, blocks;
    long nb = -1;
    char * t;

    /* a block of 20 hexadecimal digits is not a word: the predicate of conventions 5.14 fails
       and stage 6 would give ADF_DOMAIN.  With 65537 blocks the cap of stage 5 is decided
       first, with 65536 blocks the predicate of stage 6 is. */
    t = ctx_text1("modctx", 65536, 1, &len, &blocks);
    ADF_CHECK(from_dump_status(t, len, NULL, &nb) == ADF_DOMAIN);
    ADF_CHECK(nb == -1);
    flint_free(t);
    t = ctx_text1("modctx", 65537, 1, &len, &blocks);
    ADF_CHECK(from_dump_status(t, len, NULL, &nb) == ADF_UNSUPPORTED);
    ADF_CHECK(nb == -1);
    flint_free(t);
    /* the same for a scaled value, whose context the predicate of 5.14 also governs */
    t = ctx_text1("scaled", 65536, 1, &len, &blocks);
    ADF_CHECK(all_inspect(t, len, NULL, "scaled", ADF_DOMAIN) == ADF_DOMAIN);
    flint_free(t);
    t = ctx_text1("scaled", 65537, 1, &len, &blocks);
    ADF_CHECK(all_inspect(t, len, NULL, "scaled", ADF_UNSUPPORTED) == ADF_UNSUPPORTED);
    flint_free(t);
}

/* ------------------------------------------------------------------ R2: the bound of M1-D9 */

ADF_TEST(qclass_exponent_bound_at_and_above_the_bound)
{
    long nb = -1;
    char buf[256];

    /* At the bound the piece is valid: the midpoint is 1 2^-2^20, strictly between 0 and 1, the
       radius is 0, the finite part is (0 + 2 Zhat)/1 with canonical triple (0, 2, 1), and one
       piece imposes no further order (conventions 5.10).  So the text is ADF_OK. */
    (void) sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX);
    ADF_CHECK_MSG(QS(buf) == ADF_OK, "at the bound: %s", buf);
    ADF_CHECK(nb == 1);
    /* one hexadecimal digit above the bound: ADF_LIMIT, a limit of stage 4 */
    (void) sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 1 2 1 2 0",
                (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "above the bound: %s", buf);
    ADF_CHECK(nb == -1);
    /* the radius exponent is bounded as well */
    (void) sprintf(buf, "adf1 Q qclass pieces 1 1 1 0 1 -%lx l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX);
    ADF_CHECK_MSG(QS(buf) == ADF_OK, "radius at the bound: %s", buf);
    (void) sprintf(buf, "adf1 Q qclass pieces 1 1 1 0 1 -%lx l 1 2 1 2 0",
                (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "radius above the bound: %s", buf);
    ADF_CHECK(nb == -1);
    /* a positive exponent at the bound: the midpoint is 2^2^20, far above 1, so stage 6 gives
       ADF_DOMAIN, and stage 4 is silent because the exponent is at the bound */
    (void) sprintf(buf, "adf1 Q qclass pieces 1 1 1 %lx 0 0 l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX);
    ADF_CHECK_MSG(QS(buf) == ADF_DOMAIN, "positive exponent at the bound: %s", buf);
    ADF_CHECK(nb == -1);
    /* an exponent of 40 hexadecimal digits, far beyond 2^64: the bound is compared as a number
       of any length, so a reader that reads the exponent into a machine word would answer
       differently (M1-D7 is the pattern) */
    (void) sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%s 0 0 l 1 2 1 2 0", FORTY_F);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "40 digits: %s", buf);
    ADF_CHECK(nb == -1);
    /* the form "lift" is a single adele, not a piece: the grammar of 10.1 has no piece there
       and the range of a piece, whose exact end points are the cost that M1-D9 pays for, is
       not formed, so the exponents of a lift are read as they are (as the reference vectors
       tests/ref/vectors/m1-dump/dump_ref.jsonl record) */
    (void) sprintf(buf, "adf1 Q qclass lift 1 1 -%lx 0 0 l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_OK, "lift above the bound: %s", buf);
    (void) sprintf(buf, "adf1 Q qclass lift 1 1 -%s 0 0 l 1 2 1 2 0", FORTY_F);
    ADF_CHECK_MSG(QS(buf) == ADF_OK, "lift with 40 digits: %s", buf);
    ADF_CHECK(nb == 1);
}

ADF_TEST(qclass_exponent_bound_is_a_limit_of_stage_four)
{
    long nb = -1;
    char buf[256];
    int n;

    /* A limit of stage 4 is applied to every piece before any semantic check of stage 6, so
       these texts, whose only other fault is a predicate of conventions 5.10 or 5.14, give
       ADF_LIMIT and not ADF_DOMAIN. */
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 0 2 1 2 0", (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "zero denominator: %s", buf);
    ADF_CHECK(nb == -1);
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 -1 -%lx 0 0 l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "negative midpoint: %s", buf);
    ADF_CHECK(nb == -1);
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 1 6 2 2 2 1 0", (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "blocks not coprime: %s", buf);
    ADF_CHECK(nb == -1);
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 g 8 6 1", (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "global triple: %s", buf);
    ADF_CHECK(nb == -1);
    /* the second piece is bounded as well, not only the first */
    n = sprintf(buf, "adf1 Q qclass pieces 2 1 1 0 0 0 l 1 2 1 2 0 1 1 -%lx 0 0 l 1 2 1 2 0",
                (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "second piece: %s", buf);
    ADF_CHECK(nb == -1);
    /* an archimedean count of 2: the second real ball is bounded as well, before stage 6 rejects
       the count (conventions 10.1: the count of an arch must be 1 for Q) */
    n = sprintf(buf, "adf1 Q qclass pieces 1 2 1 0 0 0 1 -%lx 0 0 l 1 2 1 2 0",
                (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_LIMIT, "second archimedean ball: %s", buf);
    ADF_CHECK(nb == -1);
    /* the same texts without the oversized exponent give the status of stage 6 */
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 0 2 1 2 0", (unsigned long) QCLASS_EXP_MAX);
    ADF_CHECK_MSG(QS(buf) == ADF_DOMAIN, "zero denominator at the bound: %s", buf);
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 -1 -%lx 0 0 l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX);
    ADF_CHECK_MSG(QS(buf) == ADF_DOMAIN, "negative midpoint at the bound: %s", buf);
    /* stage 3 before stage 4: a text that is not a sentence gives ADF_PARSE */
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 1 2 1", (unsigned long) QCLASS_EXP_MAX + 1);
    ADF_CHECK_MSG(QS(buf) == ADF_PARSE, "short body: %s", buf);
    /* stage 2 before stage 4: a NUL is not a byte of the alphabet (8.2) */
    n = sprintf(buf, "adf1 Q qclass pieces 1 1 1 -%lx 0 0 l 1 2 1 2 0", (unsigned long) QCLASS_EXP_MAX + 1);
    buf[30] = '\0';
    ADF_CHECK_MSG(from_dump_status(buf, (size_t) n, NULL, &nb) == ADF_PARSE, "NUL: %s", buf);
    ADF_CHECK(nb == -1);
    /* the count limit of stage 4 and the cap of stage 5 of a context in the same text: the
       smaller stage of the two comes first, and both give ADF_LIMIT or ADF_UNSUPPORTED */
    {
        size_t len, blocks;
        char * t = ctx_text("fball", 65537, &len, &blocks);
        char * u = (char *) flint_malloc(len + 64);
        size_t m = (size_t) sprintf(u, "adf1 Q qclass pieces 1 1 1 -%s 0 0 %s", FORTY_F, t + 13);
        adf_text_limits_t lim;

        u[m] = '\0';
        ADF_CHECK(from_dump_status(u, m, NULL, &nb) == ADF_LIMIT);   /* stage 4 before stage 5 */
        adf_text_limits_default(&lim);
        lim.max_items = 3;
        ADF_CHECK(from_dump_status(u, m, &lim, &nb) == ADF_LIMIT);   /* both stage 4: LIMIT */
        flint_free(u);
        flint_free(t);
    }
}

ADF_TEST(no_exponent_bound_in_the_other_bodies)
{
    /* M1-D9: "The other bodies have no such bound: a real ball is stored with its exponent as
       it is."  An adele whose real ball has an exponent of 40 hexadecimal digits loads and
       dumps back byte for byte (CV-38). */
    static const char piece[] = "1 1 -" FORTY_F " 0 0 g 0 0 1";
    char buf[256];
    adf_adele_t x;
    char * d;
    size_t dl = 0;
    int n;
    long nb = -1;

    n = sprintf(buf, "adf1 Q adele %s", piece);
    adf_adele_init(x);
    ADF_CHECK_MSG(adf_adele_load_str(x, buf, (size_t) n, NULL, NULL) == ADF_OK, "adele: %s", buf);
    d = adf_adele_dump_str(&dl, x);
    ADF_CHECK_MSG(dl == (size_t) n && memcmp(d, buf, dl) == 0, "dump of the adele: %s", buf);
    adf_str_free(d);
    adf_adele_clear(x);
    /* the same real ball inside a piece of a qclass, where the bound of M1-D9 does apply; the
       finite part is global, so that the text needs no binding */
    n = sprintf(buf, "adf1 Q qclass pieces 1 %s", piece);
    ADF_CHECK_MSG(from_dump_status(buf, (size_t) n, NULL, &nb) == ADF_LIMIT, "qclass: %s", buf);
    ADF_CHECK(nb == -1);
}

/* The full admitted case at the cap.  About 140 s on the lane machine: each context of 65536
   blocks costs about 70 s in the tables of adf_modctx_alloc, and the loaders need one.  It runs
   only when ADF_DUMP_LIMITS_FULL=1, as tests/test_modctx_limits.c gates its admitted cases. */
ADF_TEST(block_cap_accepts_65536_blocks_in_full)
{
    size_t len, blocks;
    long nb = -1;
    char * t;

    if (getenv("ADF_DUMP_LIMITS_FULL") == NULL)
        return;
    /* the context of the text, and the four typed loaders bound to it */
    t = ctx_text("fball", 65536, &len, &blocks);
    ADF_CHECK_MSG(all_load(t, len, NULL, "fball", ADF_OK) == ADF_OK, "loader at the cap");
    flint_free(t);
    /* adf_modctx_new_from_dump on the modctx body of the same context */
    t = ctx_text("modctx", 65536, &len, &blocks);
    ADF_CHECK_MSG(from_dump_status(t, len, NULL, &nb) == ADF_OK, "from_dump at the cap");
    ADF_CHECK(nb == 65536);
    flint_free(t);
    /* and the scaled body, whose loader needs the same context */
    t = ctx_text("scaled", 65536, &len, &blocks);
    ADF_CHECK_MSG(all_load(t, len, NULL, "scaled", ADF_OK) == ADF_OK, "scaled loader at the cap");
    flint_free(t);
}
