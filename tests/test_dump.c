/* tests/test_dump.c: the dump form of the types without a context occurrence in these tests:
   adf_rat, adf_fball (global), adf_adele and adf_cadele with a global finite part (lane
   m1-dump; include/adelefeld/dump.h; docs/conventions.md 10, 8.5, 4.3).

   What is claimed and checked:
   - dump, load, identical, dump again, the same bytes (conventions 10.2, CV-38), for random
     values with operands of up to 4096 bits, real balls with radius mantissa 1, 2^29 + 1,
     2^30 - 1 and exponents of both signs and beyond a word;
   - the real ball of a dump is the text of arb_dump_str (conventions 10.2: "Real balls are
     written as arb_dump_str (arb.h:1088) writes them");
   - every loader status with the output untouched (conventions 4.3), and the order of checks
     of conventions 8.5 on texts with two faults;
   - non-canonical texts that denote a valid value are refused (the loader is strict);
   - hostile input: NUL inside, bytes 128 to 255, a text that ends at the end of a page with a
     PROT_NONE page behind it, a heap block of exact size;
   - the inspectors of these types report 0 occurrences and follow closure C2. */

#define _DEFAULT_SOURCE

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>

#include <adelefeld.h>

#include "test_runner.h"

/* ------------------------------------------------------------------ helpers */

/* The status of adf_rat_load_str with a sentinel output; checks the output is untouched on
   every status other than OK (conventions 4.3). */
static int
rat_status(const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_rat_t x;
    adf_rat_struct copy;
    int st;

    memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
    adf_rat_init(x);
    fmpq_set_si(x->q, -12345, 7);
    memcpy(&copy, x, sizeof(copy));
    st = adf_rat_load_str(x, s, len, NULL, lim);
    if (st != ADF_OK)
        ADF_CHECK_MSG(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(fmpq_numref(x->q), -12345)
                      && fmpz_equal_si(fmpq_denref(x->q), 7),
                      "rat output written on status %s", adf_status_str(st));
    adf_rat_clear(x);
    return st;
}

#define RS(lit) rat_status((lit), sizeof(lit) - 1, NULL)

static int
fball_status(const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_fball_t x;
    adf_fball_struct copy;
    int st;

    memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
    adf_fball_init(x);
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 18);
    memcpy(&copy, x, sizeof(copy));
    st = adf_fball_load_str(x, s, len, NULL, lim);
    if (st != ADF_OK)
        ADF_CHECK_MSG(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(x->A, 5)
                      && fmpz_equal_si(x->H, 18) && fmpz_is_one(x->d),
                      "fball output written on status %s", adf_status_str(st));
    adf_fball_clear(x);
    return st;
}

#define FS(lit) fball_status((lit), sizeof(lit) - 1, NULL)

static int
adele_status(const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_adele_t x;
    adf_adele_struct copy;
    arb_t r;
    int st;

    memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
    adf_adele_init(x);
    arb_init(r);
    arb_set_si(x->inf, 3);
    mag_set_ui_2exp_si(arb_radref(x->inf), 1, -3);
    arb_set(r, x->inf);
    fmpz_set_si(x->fin.A, 1);
    fmpz_set_si(x->fin.H, 4);
    memcpy(&copy, x, sizeof(copy));
    st = adf_adele_load_str(x, s, len, NULL, lim);
    if (st != ADF_OK)
        ADF_CHECK_MSG(memcmp(&copy, x, sizeof(copy)) == 0 && arb_equal(r, x->inf)
                      && fmpz_equal_si(x->fin.A, 1) && fmpz_equal_si(x->fin.H, 4),
                      "adele output written on status %s", adf_status_str(st));
    arb_clear(r);
    adf_adele_clear(x);
    return st;
}

#define AS(lit) adele_status((lit), sizeof(lit) - 1, NULL)
#define ASL(lit, lim) adele_status((lit), sizeof(lit) - 1, (lim))

static int
cadele_status(const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_cadele_t x;
    adf_cadele_struct copy;
    acb_t r;
    int st;

    memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
    adf_cadele_init(x);
    acb_init(r);
    acb_set_si_si(x->inf, 3, -5);
    acb_set(r, x->inf);
    fmpz_set_si(x->fin.A, 2);
    memcpy(&copy, x, sizeof(copy));
    st = adf_cadele_load_str(x, s, len, NULL, lim);
    if (st != ADF_OK)
        ADF_CHECK_MSG(memcmp(&copy, x, sizeof(copy)) == 0 && acb_equal(r, x->inf)
                      && fmpz_equal_si(x->fin.A, 2),
                      "cadele output written on status %s", adf_status_str(st));
    acb_clear(r);
    adf_cadele_clear(x);
    return st;
}

#define CS(lit) cadele_status((lit), sizeof(lit) - 1, NULL)

/* A well-formed returned string (conventions 8.1, 12.8): ASCII 0x20..0x7e, NUL at s[len], no
   leading or trailing space, starts "adf1 Q ". */
static int
dump_well_formed(const char * t, size_t len)
{
    size_t i;

    if (t == NULL || len < 8 || t[len] != '\0' || memcmp(t, "adf1 Q ", 7) != 0 || t[len - 1] == ' ')
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

/* A random canonical global fball with up to `bits` bits (conventions 5.2). */
static void
random_fball(adf_fball_t x, flint_rand_t st, flint_bitcnt_t bits)
{
    fmpz_t A, H, d;
    int r;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_randtest(A, st, bits);
    fmpz_randtest_not_zero(d, st, bits);
    if (n_randint(st, 3) == 0)
        fmpz_zero(H);
    else
        fmpz_randtest_not_zero(H, st, bits);
    fmpz_abs(H, H);
    r = adf_fball_set_fmpz3(x, A, H, d);
    ADF_CHECK(r == ADF_OK);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

/* A random finite arb: midpoint of up to `bits` bits, exponent of either sign, sometimes beyond a
   word; radius mantissa from a list that includes 1, 2^29 + 1 and 2^30 - 1. */
static void
random_arb(arb_t x, flint_rand_t st, flint_bitcnt_t bits)
{
    static const ulong rads[] = {0, 1, 3, (UWORD(1) << 29) + 1, (UWORD(1) << 30) - 1, 12345, 1u << 29};
    fmpz_t m, e;

    fmpz_init(m);
    fmpz_init(e);
    fmpz_randtest(m, st, bits);
    if (n_randint(st, 4) == 0)
        fmpz_randtest(e, st, 90);
    else
        fmpz_set_si(e, (slong) n_randint(st, 400) - 200);
    arf_set_fmpz_2exp(arb_midref(x), m, e);
    {
        ulong rm = rads[n_randint(st, sizeof(rads) / sizeof(rads[0]))];
        if (rm == 0)
            mag_zero(arb_radref(x));
        else
        {
            if (n_randint(st, 3) == 0)
                fmpz_randtest(e, st, 80);
            else
                fmpz_set_si(e, (slong) n_randint(st, 200) - 100);
            mag_set_ui_2exp_si(arb_radref(x), rm, 0);
            mag_mul_2exp_fmpz(arb_radref(x), arb_radref(x), e);
        }
    }
    fmpz_clear(m);
    fmpz_clear(e);
}

/* ------------------------------------------------------------------ adf_rat */

ADF_TEST(rat_dump_text)
{
    adf_rat_t x;
    size_t len;
    char * t;

    adf_rat_init(x);
    fmpq_set_si(x->q, -7, 3);
    t = adf_rat_dump_str(&len, x);
    ADF_CHECK(len == 15 && strcmp(t, "adf1 Q rat -7 3") == 0);
    adf_str_free(t);
    adf_rat_zero(x);
    t = adf_rat_dump_str(&len, x);
    ADF_CHECK(len == 14 && strcmp(t, "adf1 Q rat 0 1") == 0);
    adf_str_free(t);
    fmpq_set_si(x->q, 31, 1);
    t = adf_rat_dump_str(&len, x);
    ADF_CHECK(len == 15 && strcmp(t, "adf1 Q rat 1f 1") == 0);
    adf_str_free(t);
    adf_rat_clear(x);
}

ADF_TEST(rat_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 400; i++)
    {
        adf_rat_t x, y;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 4096 : 1 + n_randint(st, 200);

        adf_rat_init(x);
        adf_rat_init(y);
        fmpq_randtest(x->q, st, bits);
        t = adf_rat_dump_str(&len, x);
        ADF_CHECK(dump_well_formed(t, len));
        ADF_CHECK(adf_rat_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK(adf_rat_identical(x, y));
        t2 = adf_rat_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t, t2, len) == 0);
        /* the binds form with no binding, and the one-context form with a context */
        adf_rat_zero(y);
        ADF_CHECK(adf_rat_load_str_binds(y, t, len, NULL, 0, NULL) == ADF_OK && adf_rat_identical(x, y));
        adf_str_free(t);
        adf_str_free(t2);
        adf_rat_clear(x);
        adf_rat_clear(y);
    }
    flint_randclear(st);
}

ADF_TEST(rat_statuses_and_strictness)
{
    adf_text_limits_t lim;

    ADF_CHECK(RS("adf1 Q rat 7 3") == ADF_OK);
    ADF_CHECK(RS("adf1 Q rat 2 4") == ADF_DOMAIN);        /* not in lowest terms */
    ADF_CHECK(RS("adf1 Q rat 7 0") == ADF_DOMAIN);
    ADF_CHECK(RS("adf1 Q rat 7 -3") == ADF_DOMAIN);
    ADF_CHECK(RS("adf1 Q rat 0 2") == ADF_DOMAIN);
    ADF_CHECK(RS("adf1 Q rat 07 3") == ADF_PARSE);        /* leading zero */
    ADF_CHECK(RS("adf1 Q rat 1F 1") == ADF_PARSE);        /* upper-case hexadecimal */
    ADF_CHECK(RS("adf1 Q rat 1f 1") == ADF_OK);
    ADF_CHECK(RS("adf1 Q rat 7  3") == ADF_PARSE);        /* two spaces */
    ADF_CHECK(RS("adf1 Q rat 7 3 ") == ADF_PARSE);        /* a trailing space */
    ADF_CHECK(RS("adf1 Q rat 7 3\n") == ADF_PARSE);       /* a final newline */
    ADF_CHECK(RS(" adf1 Q rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat -0 1") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat - 1") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat 7") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat 7 3 1") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat ") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q ") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q") == ADF_PARSE);
    ADF_CHECK(RS("adf1") == ADF_PARSE);
    ADF_CHECK(RS("adf") == ADF_PARSE);
    ADF_CHECK(RS("") == ADF_PARSE);
    ADF_CHECK(rat_status(NULL, 0, NULL) == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q fball g 7 0 3") == ADF_PARSE);   /* another body: not a rat dump */
    ADF_CHECK(RS("adf1 Q modctx 1 0") == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q ratt 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf2 Q rat 7 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf0 Q rat 7 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf10 Q rat 7 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf2") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf2x Q rat 7 3") == ADF_PARSE);        /* the version is followed by a space */
    ADF_CHECK(RS("adf1x Q rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf01 Q rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf Q rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf1 K rat 7 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf1 QQ rat 7 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf1 F3(T) rat 7 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf1 K") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf1 q rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf1 rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("adf1  Q rat 7 3") == ADF_PARSE);
    ADF_CHECK(RS("ADF1 Q rat 7 3") == ADF_PARSE);
    /* limits: the length before any byte is read */
    adf_text_limits_default(&lim);
    lim.max_len = 13;
    ADF_CHECK(rat_status("adf1 Q rat 7 3", 14, &lim) == ADF_LIMIT);
    lim.max_len = 14;
    ADF_CHECK(rat_status("adf1 Q rat 7 3", 14, &lim) == ADF_OK);
    /* 4096-bit numerator */
    {
        adf_rat_t x;
        char * t;
        size_t len;
        adf_rat_init(x);
        fmpz_one(fmpq_numref(x->q));
        fmpz_mul_2exp(fmpq_numref(x->q), fmpq_numref(x->q), 4095);
        fmpz_add_ui(fmpq_numref(x->q), fmpq_numref(x->q), 1);
        fmpz_set_ui(fmpq_denref(x->q), 5);      /* 2^4095 + 1 = 4 mod 5: lowest terms */
        ADF_CHECK(adf_rat_is_canonical(x));
        t = adf_rat_dump_str(&len, x);
        ADF_CHECK(len == 11 + 1024 + 2);
        ADF_CHECK(rat_status(t, len, NULL) == ADF_OK);
        lim.max_len = len - 1;
        ADF_CHECK(rat_status(t, len, &lim) == ADF_LIMIT);
        adf_str_free(t);
        adf_rat_clear(x);
    }
}

/* conventions 8.5: the first failing stage decides, on texts with two faults. */
ADF_TEST(order_of_checks_two_faults)
{
    adf_text_limits_t lim;

    adf_text_limits_default(&lim);
    /* stage 1 before stage 2 */
    lim.max_len = 5;
    ADF_CHECK(rat_status("adf1 Q rat 7 3\x80", 15, &lim) == ADF_LIMIT);
    /* stage 2 before the header */
    ADF_CHECK(RS("adf2 Q rat 7 3\x80") == ADF_PARSE);
    ADF_CHECK(RS("adf1 K rat 7 3\x01") == ADF_PARSE);
    /* the header (UNSUPPORTED) before the body grammar */
    ADF_CHECK(RS("adf2 Q rat 07 3") == ADF_UNSUPPORTED);
    ADF_CHECK(RS("adf1 K fball x") == ADF_UNSUPPORTED);
    /* the grammar before the semantic stage */
    ADF_CHECK(RS("adf1 Q rat 2 4 1") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball g 8 6") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele 2 2 0 0 0 g 0 0 1") == ADF_PARSE);   /* count 2, 4 tokens */
    /* two semantic faults: DOMAIN */
    ADF_CHECK(AS("adf1 Q adele 1 2 0 0 0 g 8 6 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 2 1 0 0 0 1 0 0 0 g 8 6 1") == ADF_DOMAIN);
}

/* conventions 8.2 and 8.5: TAB, LF and CR are bytes of the alphabet, so stage 2 lets them past;
   the header is judged next, and a version or a field other than "1" and "Q" is ADF_UNSUPPORTED
   whatever the body holds (10.2 "Version and field"; dump.h: "adf_UNSUPPORTED (a version other
   than 1, a field other than Q)"); with the header "adf1 Q " such a byte in the body is a
   grammar failure of stage 3 (conventions 10.1: "Tokens are separated by exactly one space
   0x20; no other whitespace anywhere").  A NUL and a byte above 0x7e are not bytes of the
   alphabet at all, so stage 2 gives ADF_PARSE for them at every position, header included. */
ADF_TEST(header_before_the_whitespace_of_the_body)
{
    static const char body[] = "rat 1 1";    /* 7 bytes */
    static const int at[] = {0, 4, 6};    /* a first, a middle and the last position of the body */
    static const char * const wname[] = {"TAB", "LF", "CR"};
    const unsigned char ws[] = {0x09, 0x0a, 0x0d};
    const unsigned char other[] = {0x00, 0x80};
    char buf[64];
    int w, j, p;

    for (w = 0; w < 3; w++)
    {
        /* the two texts of the review: a version other than 1, a field other than Q */
        memcpy(buf, "adf2 Q rat 1 1", 14);
        buf[8] = (char) ws[w];
        ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_UNSUPPORTED, "%s in the body of version 2",
                      wname[w]);
        memcpy(buf, "adf1 R rat 1 1", 14);
        buf[8] = (char) ws[w];
        ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_UNSUPPORTED, "%s in the body of field R",
                      wname[w]);
        ADF_CHECK_MSG(fball_status(buf, 14, NULL) == ADF_UNSUPPORTED, "%s, fball loader", wname[w]);
        /* the byte in the body, with the header "adf1 Q ": a grammar failure */
        for (p = 0; p < 3; p++)
        {
            memcpy(buf, "adf1 Q ", 7);
            memcpy(buf + 7, body, sizeof(body) - 1);
            buf[7 + at[p]] = (char) ws[w];
            ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_PARSE, "%s at position %d of the body",
                          wname[w], p);
        }
        /* inside the header: where the grammar wants the single space after the version, a
           grammar failure; inside the field token, the field is the run of non-space bytes and
           is not "Q", so ADF_UNSUPPORTED (conventions 10.1, `field = upper, {nonspace}`) */
        /* inside the header: where the grammar wants the single space after the version, and
           where the field token must begin with an upper-case letter (conventions 10.1,
           `field = upper, {nonspace}`), a grammar failure; a byte after the field letter stays
           inside the field token, which is then not "Q", so ADF_UNSUPPORTED (10.2) */
        memcpy(buf, "adf1 Q rat 1 1", 14);
        buf[4] = (char) ws[w];
        ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_PARSE, "%s after the version", wname[w]);
        memcpy(buf, "adf1 Q rat 1 1", 14);
        buf[5] = (char) ws[w];
        ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_PARSE, "%s instead of the field", wname[w]);
        memcpy(buf, "adf1 Q rat 1 1", 14);
        buf[6] = (char) ws[w];
        ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_UNSUPPORTED, "%s inside the field", wname[w]);
    }
    /* a NUL and a byte above 0x7e are no bytes of the alphabet (8.2): ADF_PARSE at every
       position of the body, whatever the header says */
    for (j = 0; j < 2; j++)
    {
        for (p = 0; p < 3; p++)
        {
            memcpy(buf, "adf2 Q rat 1 1", 14);
            buf[7 + at[p]] = (char) other[j];
            ADF_CHECK_MSG(rat_status(buf, 14, NULL) == ADF_PARSE, "byte %02x at position %d",
                          other[j], p);
        }
    }
    /* a NUL inside the field token, and a byte above 0x7e after the version */
    memcpy(buf, "adf1 Q rat 1 1", 14);
    buf[5] = '\0';
    ADF_CHECK(rat_status(buf, 14, NULL) == ADF_PARSE);
    memcpy(buf, "adf1 Q rat 1 1", 14);
    buf[4] = (char) 0x80;
    ADF_CHECK(rat_status(buf, 14, NULL) == ADF_PARSE);
    /* the version itself may not be followed by anything but a space, whatever it is */
    memcpy(buf, "adf1 Q rat 1 1", 14);
    buf[3] = '2';
    ADF_CHECK(rat_status(buf, 14, NULL) == ADF_UNSUPPORTED);
}

/* ------------------------------------------------------------------ adf_fball, global */

ADF_TEST(fball_global_dump_text)
{
    adf_fball_t x;
    size_t len;
    char * t;
    fmpz_t A, H, d;

    adf_fball_init(x);
    fmpz_init_set_si(A, 5);
    fmpz_init_set_si(H, 12);
    fmpz_init_set_si(d, 3);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    t = adf_fball_dump_str(&len, x);
    ADF_CHECK(strcmp(t, "adf1 Q fball g 5 c 3") == 0 && len == 20);
    adf_str_free(t);
    adf_fball_zero(x);
    t = adf_fball_dump_str(&len, x);
    ADF_CHECK(strcmp(t, "adf1 Q fball g 0 0 1") == 0 && len == 20);
    adf_str_free(t);
    fmpz_set_si(A, -7);
    fmpz_zero(H);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);
    t = adf_fball_dump_str(&len, x);
    ADF_CHECK(strcmp(t, "adf1 Q fball g -7 0 3") == 0);
    adf_str_free(t);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    adf_fball_clear(x);
}

ADF_TEST(fball_global_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 400; i++)
    {
        adf_fball_t x, y;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 4096 : 1 + n_randint(st, 200);

        adf_fball_init(x);
        adf_fball_init(y);
        random_fball(x, st, bits);
        t = adf_fball_dump_str(&len, x);
        ADF_CHECK(dump_well_formed(t, len));
        ADF_CHECK(adf_fball_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK(adf_fball_identical(x, y) && adf_fball_is_canonical(y) && y->backend == ADF_GLOBAL
                  && y->mctx == NULL && y->res == NULL);
        t2 = adf_fball_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t, t2, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_fball_clear(x);
        adf_fball_clear(y);
    }
    flint_randclear(st);
}

ADF_TEST(fball_global_statuses)
{
    ADF_CHECK(FS("adf1 Q fball g 2 6 1") == ADF_OK);
    ADF_CHECK(FS("adf1 Q fball g 8 6 1") == ADF_DOMAIN);   /* A >= H */
    ADF_CHECK(FS("adf1 Q fball g 6 6 1") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g 2 6 2") == ADF_DOMAIN);   /* gcd(A, H, d) = 2 */
    ADF_CHECK(FS("adf1 Q fball g 0 0 2") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g 4 0 2") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g -1 6 1") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g 1 -6 1") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g 1 6 0") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g 1 6 -1") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball g -3 0 1") == ADF_OK);
    ADF_CHECK(FS("adf1 Q fball x 1 6 1") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball g 1 6") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball g 1 6 1 1") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball G 1 6 1") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball g 1 06 1") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball g 1 6 1\n") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q fball") == ADF_PARSE);
    ADF_CHECK(FS("adf1 Q rat 7 3") == ADF_PARSE);
    /* a local ball needs a binding: the convenience form with ctx = NULL is DOMAIN */
    ADF_CHECK(FS("adf1 Q fball l 1 6 2 2 3 0 2") == ADF_DOMAIN);
    ADF_CHECK(FS("adf1 Q fball l 1 6 2 2 3 0") == ADF_PARSE);
}

/* ------------------------------------------------------------------ adf_adele, adf_cadele */

/* The text of the real ball in a dump equals arb_dump_str (conventions 10.2). */
static void
check_arb_text(const char * t, size_t len, size_t at, const arb_t r)
{
    char * a = arb_dump_str(r);
    size_t n = strlen(a);

    ADF_CHECK_MSG(at + n <= len && memcmp(t + at, a, n) == 0 && (at + n == len || t[at + n] == ' '),
                  "the dump \"%s\" does not carry arb_dump_str \"%s\" at %zu", t, a, at);
    flint_free(a);
}

ADF_TEST(adele_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 400; i++)
    {
        adf_adele_t x, y;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 4096 : 1 + n_randint(st, 200);

        adf_adele_init(x);
        adf_adele_init(y);
        random_arb(x->inf, st, bits);
        random_fball(&x->fin, st, bits);
        t = adf_adele_dump_str(&len, x);
        ADF_CHECK(dump_well_formed(t, len));
        ADF_CHECK(len > 15 && memcmp(t, "adf1 Q adele 1 ", 15) == 0);
        check_arb_text(t, len, 15, x->inf);
        ADF_CHECK_MSG(adf_adele_load_str(y, t, len, NULL, NULL) == ADF_OK, "%s", t);
        ADF_CHECK(adf_adele_identical(x, y) && adf_adele_is_canonical(y));
        /* the same ball as FLINT reads from the validated text (the arb part only) */
        {
            char * a = arb_dump_str(x->inf);
            arb_t z;
            arb_init(z);
            ADF_CHECK(arb_load_str(z, a) == 0 && arb_equal(z, y->inf));
            arb_clear(z);
            flint_free(a);
        }
        t2 = adf_adele_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t, t2, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_adele_clear(x);
        adf_adele_clear(y);
    }
    flint_randclear(st);
}

ADF_TEST(cadele_round_trip_random)
{
    flint_rand_t st;
    int i;

    flint_randinit(st);
    for (i = 0; i < 300; i++)
    {
        adf_cadele_t x, y;
        size_t len, len2;
        char * t, * t2;
        flint_bitcnt_t bits = (i % 10 == 0) ? 4096 : 1 + n_randint(st, 200);

        adf_cadele_init(x);
        adf_cadele_init(y);
        random_arb(acb_realref(x->inf), st, bits);
        random_arb(acb_imagref(x->inf), st, bits);
        random_fball(&x->fin, st, bits);
        t = adf_cadele_dump_str(&len, x);
        ADF_CHECK(dump_well_formed(t, len));
        ADF_CHECK(len > 16 && memcmp(t, "adf1 Q cadele 1 ", 16) == 0);
        check_arb_text(t, len, 16, acb_realref(x->inf));
        ADF_CHECK(adf_cadele_load_str(y, t, len, NULL, NULL) == ADF_OK);
        ADF_CHECK(adf_cadele_identical(x, y) && adf_cadele_is_canonical(y));
        t2 = adf_cadele_dump_str(&len2, y);
        ADF_CHECK(len2 == len && memcmp(t, t2, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_cadele_clear(x);
        adf_cadele_clear(y);
    }
    flint_randclear(st);
}

/* Real balls at the edges of the radius mantissa (2^30 is MAG_BITS, mag.h:117) and with
   exponents of both signs, set exactly, dumped, loaded. */
ADF_TEST(adele_radius_edges)
{
    static const ulong rm[] = {1, (UWORD(1) << 29) + 1, (UWORD(1) << 30) - 1, UWORD(1) << 29, 7};
    static const slong re[] = {0, 1, -1, 1000, -1000, WORD(1) << 40, -(WORD(1) << 40)};
    size_t i, j;

    for (i = 0; i < sizeof(rm) / sizeof(rm[0]); i++)
        for (j = 0; j < sizeof(re) / sizeof(re[0]); j++)
        {
            adf_adele_t x, y;
            fmpz_t e;
            size_t len;
            char * t;

            adf_adele_init(x);
            adf_adele_init(y);
            fmpz_init_set_si(e, re[j]);
            arb_set_si(x->inf, -3);
            mag_set_ui_2exp_si(arb_radref(x->inf), rm[i], 0);
            mag_mul_2exp_fmpz(arb_radref(x->inf), arb_radref(x->inf), e);
            t = adf_adele_dump_str(&len, x);
            check_arb_text(t, len, 15, x->inf);
            ADF_CHECK(adf_adele_load_str(y, t, len, NULL, NULL) == ADF_OK && adf_adele_identical(x, y));
            adf_str_free(t);
            fmpz_clear(e);
            adf_adele_clear(x);
            adf_adele_clear(y);
        }
    /* the texts of the radius mantissa at 1, 2^29 + 1, 2^30 - 1 are accepted as written */
    ADF_CHECK(AS("adf1 Q adele 1 1 0 1 0 g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 20000001 -5 g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 3fffffff 7 g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 -1 -7fffffffffffffffffff 3fffffff 7fffffffffffffffffff g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 40000001 0 g 0 0 1") == ADF_DOMAIN);   /* 2^30 + 1 */
    ADF_CHECK(AS("adf1 Q adele 1 1 0 40000000 0 g 0 0 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 20000000 0 g 0 0 1") == ADF_DOMAIN);   /* even */
    ADF_CHECK(AS("adf1 Q adele 1 1 0 100000001 0 g 0 0 1") == ADF_DOMAIN);
}

ADF_TEST(adele_statuses)
{
    adf_text_limits_t lim;

    ADF_CHECK(AS("adf1 Q adele 1 1 -1 0 0 g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 0 0 0 0 g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 0 0 1 5 g 0 0 1") == ADF_OK);
    ADF_CHECK(AS("adf1 Q adele 1 2 0 0 0 g 0 0 1") == ADF_DOMAIN);       /* even mantissa */
    ADF_CHECK(AS("adf1 Q adele 1 -2 0 0 0 g 0 0 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 1 0 -1 0 0 g 0 0 1") == ADF_DOMAIN);      /* +inf of arb_dump_str */
    ADF_CHECK(AS("adf1 Q adele 1 0 -3 0 -1 g 0 0 1") == ADF_DOMAIN);     /* nan */
    ADF_CHECK(AS("adf1 Q adele 1 0 0 0 -1 g 0 0 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 1 0 5 0 0 g 0 0 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 -1 0 g 0 0 1") == ADF_DOMAIN);      /* negative radius */
    ADF_CHECK(AS("adf1 Q adele 1 1 0 0 1 g 0 0 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 0 0 g 0 0 2") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 0 g 0 0 1") == ADF_DOMAIN);               /* count 0 */
    ADF_CHECK(AS("adf1 Q adele 2 1 0 0 0 1 0 0 0 g 0 0 1") == ADF_DOMAIN);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 0 0 5 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele 1 1 0 0 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele 1 A 0 0 0 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele 1 1 00 0 0 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele -1 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q adele ffffffffffffffffffff 1 0 0 0 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(AS("adf1 Q cadele 1 1 0 0 0 1 0 0 0 g 0 0 1") == ADF_PARSE);
    ADF_CHECK(CS("adf1 Q cadele 1 1 0 0 0 1 -1 0 0 g 2 6 1") == ADF_OK);
    ADF_CHECK(CS("adf1 Q cadele 1 1 0 0 0 0 -1 0 0 g 2 6 1") == ADF_DOMAIN);
    ADF_CHECK(CS("adf1 Q cadele 1 4 0 0 0 1 -1 0 0 g 2 6 1") == ADF_DOMAIN);
    ADF_CHECK(CS("adf1 Q cadele 1 1 0 0 0 1 -1 0 0 g 7 6 1") == ADF_DOMAIN);
    ADF_CHECK(CS("adf1 Q cadele 2 1 0 0 0 1 -1 0 0 1 0 0 0 1 -1 0 0 g 2 6 1") == ADF_DOMAIN);
    ADF_CHECK(CS("adf1 Q cadele 1 1 0 0 0 1 -1 0 g 2 6 1") == ADF_PARSE);
    ADF_CHECK(CS("adf1 Q adele 1 1 0 0 0 g 2 6 1") == ADF_PARSE);
    /* a local finite part without a binding */
    ADF_CHECK(AS("adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2") == ADF_DOMAIN);
    /* stage 4 before stage 6: the block count of the nested context */
    adf_text_limits_default(&lim);
    lim.max_items = 1;
    ADF_CHECK(ASL("adf1 Q adele 2 2 0 0 0 1 0 0 0 l 0 6 2 2 3 0 2", &lim) == ADF_LIMIT);
    ADF_CHECK(ASL("adf1 Q adele 1 1 0 0 0 l 0 6 1 6 0", &lim) == ADF_DOMAIN);
}

/* ------------------------------------------------------------------ inspectors without contexts */

ADF_TEST(inspect_without_contexts)
{
    adf_ctx_desc_t d[2];
    size_t n;

    adf_ctx_desc_init(&d[0]);
    adf_ctx_desc_init(&d[1]);
    fmpz_set_ui(d[0].K, 77);
    /* descs = NULL: the incoming *nctx is ignored, the count written on OK */
    n = 99;
    ADF_CHECK(adf_rat_dump_inspect(&n, NULL, "adf1 Q rat 7 3", 14, NULL) == ADF_OK && n == 0);
    n = 99;
    ADF_CHECK(adf_fball_dump_inspect(&n, NULL, "adf1 Q fball g 2 6 1", 20, NULL) == ADF_OK && n == 0);
    n = 99;
    ADF_CHECK(adf_adele_dump_inspect(&n, NULL, "adf1 Q adele 1 1 0 0 0 g 0 0 1", 30, NULL) == ADF_OK
              && n == 0);
    n = 99;
    ADF_CHECK(adf_cadele_dump_inspect(&n, NULL, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 g 0 0 1", 39, NULL)
              == ADF_OK && n == 0);
    /* capacity 0 suffices for no occurrence; the descriptors are untouched */
    n = 0;
    ADF_CHECK(adf_rat_dump_inspect(&n, d, "adf1 Q rat 7 3", 14, NULL) == ADF_OK && n == 0);
    n = 2;
    ADF_CHECK(adf_rat_dump_inspect(&n, d, "adf1 Q rat 7 3", 14, NULL) == ADF_OK && n == 0);
    ADF_CHECK(fmpz_equal_ui(d[0].K, 77) && d[0].k == 0 && d[0].q == NULL);
    /* failures leave *nctx untouched */
    n = 5;
    ADF_CHECK(adf_rat_dump_inspect(&n, NULL, "adf1 Q rat 2 4", 14, NULL) == ADF_DOMAIN && n == 5);
    ADF_CHECK(adf_rat_dump_inspect(&n, d, "adf1 Q rat 7 3 ", 15, NULL) == ADF_PARSE && n == 5);
    ADF_CHECK(adf_rat_dump_inspect(&n, d, "adf2 Q rat 7 3", 14, NULL) == ADF_UNSUPPORTED && n == 5);
    ADF_CHECK(adf_fball_dump_inspect(&n, NULL, "adf1 Q fball g 8 6 1", 20, NULL) == ADF_DOMAIN && n == 5);
    ADF_CHECK(adf_fball_dump_inspect(&n, NULL, "adf1 Q rat 7 3", 14, NULL) == ADF_PARSE && n == 5);
    ADF_CHECK(fmpz_equal_ui(d[0].K, 77));
    adf_ctx_desc_clear(&d[0]);
    adf_ctx_desc_clear(&d[1]);
}

/* ------------------------------------------------------------------ hostile input */

ADF_TEST(hostile_bytes)
{
    char buf[64];
    int b;

    /* a NUL inside, at the end, at the start */
    ADF_CHECK(rat_status("adf1 Q rat 7\0 3", 15, NULL) == ADF_PARSE);
    ADF_CHECK(rat_status("adf1 Q rat 7 3\0", 15, NULL) == ADF_PARSE);
    ADF_CHECK(rat_status("\0adf1 Q rat 7 3", 15, NULL) == ADF_PARSE);
    /* the text is (s, len): bytes after len are not read, a NUL after len does not matter */
    ADF_CHECK(rat_status("adf1 Q rat 7 3 junk", 14, NULL) == ADF_OK);
    /* every byte from 128 to 255, and every control byte, in the middle of a valid dump */
    for (b = 0; b < 256; b++)
    {
        size_t n = 14;
        if (b >= 0x20 && b <= 0x7e)
            continue;
        memcpy(buf, "adf1 Q rat 7 3", 14);
        buf[11] = (char) b;
        ADF_CHECK_MSG(rat_status(buf, n, NULL) == ADF_PARSE, "byte %d", b);
        ADF_CHECK_MSG(fball_status(buf, n, NULL) == ADF_PARSE, "byte %d", b);
    }
    ADF_CHECK(rat_status("adf1\tQ rat 7 3", 14, NULL) == ADF_PARSE);
    ADF_CHECK(rat_status("adf1 Q rat 7\r3", 14, NULL) == ADF_PARSE);
    ADF_CHECK(RS("adf1 Q rat \xe2\x88\x92" "7 3") == ADF_PARSE);   /* UTF-8 minus */
}

/* Every prefix of the texts below, placed at the end of a page with a PROT_NONE page behind it,
   and as a heap block of exact size: a reader that looks at s[len] faults (conventions 8.1). */
static void
page_end_all_loaders(const char * text, size_t len, char * page_end)
{
    char * p = page_end - len;
    char * h;

    memcpy(p, text, len);
    (void) rat_status(p, len, NULL);
    (void) fball_status(p, len, NULL);
    (void) adele_status(p, len, NULL);
    (void) cadele_status(p, len, NULL);
    {
        size_t n = 3;
        (void) adf_adele_dump_inspect(&n, NULL, p, len, NULL);
    }
    h = flint_malloc(len > 0 ? len : 1);
    memcpy(h, text, len);
    (void) rat_status(h, len, NULL);
    (void) fball_status(h, len, NULL);
    (void) adele_status(h, len, NULL);
    (void) cadele_status(h, len, NULL);
    flint_free(h);
}

ADF_TEST(input_at_the_end_of_a_page)
{
    static const char * texts[] = {
        "adf1 Q rat 7 3", "adf1 Q rat -7f 3", "adf1 Q fball g 5 c 3", "adf1 Q fball l 1 6 2 2 3 0 2",
        "adf1 Q adele 1 c90fcf80dc337 -32 a7c5ac5 -2c g 5 12 3", "adf1 Q adele 1 0 0 0 0 g 0 0 1",
        "adf1 Q cadele 1 1 0 0 0 1 -1 0 0 g 2 6 1", "adf2", "adf1 K", "adf1 Q adele 1 -"};
    long pg = sysconf(_SC_PAGESIZE);
    char * base;
    size_t i, j;

    ADF_CHECK(pg > 0);
    if (pg <= 0)
        return;
    base = mmap(NULL, (size_t) pg * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ADF_CHECK(base != MAP_FAILED);
    if (base == MAP_FAILED)
        return;
    ADF_CHECK(mprotect(base + pg, (size_t) pg, PROT_NONE) == 0);
    for (i = 0; i < sizeof(texts) / sizeof(texts[0]); i++)
    {
        size_t n = strlen(texts[i]);
        for (j = 0; j <= n; j++)
            page_end_all_loaders(texts[i], j, base + pg);
    }
    /* and the full valid texts load at the end of the page */
    memcpy(base + pg - 14, "adf1 Q rat 7 3", 14);
    ADF_CHECK(rat_status(base + pg - 14, 14, NULL) == ADF_OK);
    memcpy(base + pg - 30, "adf1 Q adele 1 1 0 0 0 g 0 0 1", 30);
    ADF_CHECK(adele_status(base + pg - 30, 30, NULL) == ADF_OK);
    munmap(base, (size_t) pg * 2);
}

/* A value dumped and loaded into itself: the output is replaced, not merged (conventions 4.3). */
ADF_TEST(load_over_a_value)
{
    adf_adele_t x;
    size_t len;
    char * t;

    adf_adele_init(x);
    arb_set_si(x->inf, 5);
    adf_fball_set_si(&x->fin, 9);
    t = adf_adele_dump_str(&len, x);
    ADF_CHECK(strcmp(t, "adf1 Q adele 1 5 0 0 0 g 9 0 1") == 0);
    ADF_CHECK(adf_adele_load_str(x, "adf1 Q adele 1 1 -1 1 -3 g 1 2 1", 32, NULL, NULL) == ADF_OK);
    ADF_CHECK(arf_equal_si(arb_midref(x->inf), 0) == 0);
    adf_str_free(t);
    t = adf_adele_dump_str(&len, x);
    ADF_CHECK(strcmp(t, "adf1 Q adele 1 1 -1 1 -3 g 1 2 1") == 0);
    adf_str_free(t);
    adf_adele_clear(x);
}
