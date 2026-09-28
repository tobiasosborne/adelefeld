/* tests/fuzz/fuzz_text.c: the fuzzer of the value-form parsers and printers (src/text.c, work
   package 1.4; brief of lane m1-text).

   libFuzzer calls LLVMFuzzerTestOneInput with arbitrary bytes. The bytes go, without a
   terminator, to adf_text_classify and to the four parsers adf_rat_set_str, adf_fball_set_str,
   adf_adele_set_str and adf_cadele_set_str. The address and undefined-behaviour sanitizers of
   `make fuzz` watch every read of the input and every allocation.

   src/text.c is included into this file, so that it is compiled with the sanitizers and with the
   coverage flags of `make fuzz` (the library archive is built by gcc without them); the definitions
   here take the place of the archive member text.o, which the linker then does not pull in.

   What is asserted (a violation aborts, and libFuzzer keeps the input as a crash):
   - every call returns a status of conventions 3.2 for a parser (OK, PARSE, LIMIT, DOMAIN; the four
     types have no word restriction and no sign condition), OK, PARSE or LIMIT for classify;
   - the status of stage 1 is the same for all five calls (len > max_len);
   - classify and the typed parsers agree on the syntax (conventions 9.7): a parser returns PARSE
     exactly when classify does not return its kind;
   - on a status other than ADF_OK the output is untouched (conventions 4.3): bitwise equal to a
     copy of the sentinel, and equal in value;
   - on ADF_OK the value satisfies its predicate (adf_rat_is_canonical, adf_fball_is_canonical and
     global, arb_is_finite / acb_is_finite for the real part); its printed text is well formed
     (ASCII, no whitespace at the ends, NUL at len), is classified as the same kind, and parses
     again: identical for adf_rat and adf_fball (conventions 9.6), and for the adele types with the
     identical finite part and a real part that contains the first one (9.6: C value-text round
     trips enclose).

   The limits are the defaults of conventions 8.4 except max_exp10 = 5000, which keeps a run at
   many executions per second; the code paths of larger exponents are the same. prec and digits
   are taken from the first and the last byte. */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/text.c"

static int
well_formed(const char * t, size_t len)
{
    size_t i;

    if (t == NULL || len == 0 || t[len] != '\0' || t[0] == ' ' || t[len - 1] == ' ')
        return 0;
    for (i = 0; i < len; i++)
        if ((unsigned char) t[i] < 0x20 || (unsigned char) t[i] > 0x7e)
            return 0;
    return 1;
}

#define REQUIRE(cond) do { if (!(cond)) abort(); } while (0)

static int
parser_status_ok(int st)
{
    return st == ADF_OK || st == ADF_PARSE || st == ADF_LIMIT || st == ADF_DOMAIN;
}

/* The parse status agrees with classification: PARSE exactly when the kind is not k. */
static void
agree(int st, int cst, adf_text_kind kind, int k)
{
    REQUIRE(parser_status_ok(st));
    if (cst == ADF_LIMIT)
        REQUIRE(st == ADF_LIMIT);
    else if (cst == ADF_OK && (int) kind == k)
        REQUIRE(st != ADF_PARSE);
    else
        REQUIRE(st == ADF_PARSE);
}

static void
check_printed_kind(const char * t, size_t len, int k)
{
    adf_text_kind kind;

    REQUIRE(well_formed(t, len));
    REQUIRE(adf_text_classify(&kind, t, len, NULL) == ADF_OK && (int) kind == k);
}

static void
fuzz_rat(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind)
{
    adf_rat_t x, y;
    adf_rat_struct copy;
    int st;

    adf_rat_init(x);
    adf_rat_init(y);
    fmpq_set_si(x->q, 12345, 7);
    memcpy(&copy, x, sizeof(copy));
    st = adf_rat_set_str(x, s, n, lim);
    agree(st, cst, kind, ADF_TEXT_RAT);
    if (st != ADF_OK)
        REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_cmp_si(fmpq_numref(x->q), 12345) == 0
                && fmpz_cmp_si(fmpq_denref(x->q), 7) == 0);
    else
    {
        size_t len;
        char * t;

        REQUIRE(adf_rat_is_canonical(x));
        t = adf_rat_get_str(&len, x);
        check_printed_kind(t, len, ADF_TEXT_RAT);
        REQUIRE(adf_rat_set_str(y, t, len, NULL) == ADF_OK && adf_rat_identical(x, y));
        flint_free(t);
    }
    adf_rat_clear(x);
    adf_rat_clear(y);
}

static void
fuzz_fball(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind)
{
    adf_fball_t x, y;
    adf_fball_struct copy;
    int st;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 18);
    memcpy(&copy, x, sizeof(copy));
    st = adf_fball_set_str(x, s, n, lim);
    agree(st, cst, kind, ADF_TEXT_FBALL);
    if (st != ADF_OK)
        REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_cmp_si(x->A, 5) == 0 && fmpz_cmp_si(x->H, 18) == 0
                && fmpz_is_one(x->d));
    else
    {
        size_t len;
        char * t;

        REQUIRE(adf_fball_is_canonical(x) && x->backend == ADF_GLOBAL);
        t = adf_fball_get_str(&len, x);
        check_printed_kind(t, len, ADF_TEXT_FBALL);
        REQUIRE(adf_fball_set_str(y, t, len, NULL) == ADF_OK && adf_fball_identical(x, y));
        flint_free(t);
    }
    adf_fball_clear(x);
    adf_fball_clear(y);
}

static void
fuzz_adele(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind, slong prec,
           slong digits)
{
    adf_adele_t x, y;
    adf_adele_struct copy;
    int st;

    arb_init(x->inf);
    adf_fball_init(&x->fin);
    arb_init(y->inf);
    adf_fball_init(&y->fin);
    arb_set_si(x->inf, 3);
    fmpz_set_si(x->fin.A, 5);
    fmpz_set_si(x->fin.H, 18);
    memcpy(&copy, x, sizeof(copy));
    st = adf_adele_set_str(x, s, n, prec, lim);
    agree(st, cst, kind, ADF_TEXT_ADELE);
    if (st != ADF_OK)
        REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && arf_equal_si(arb_midref(x->inf), 3)
                && mag_is_zero(arb_radref(x->inf)) && fmpz_cmp_si(x->fin.A, 5) == 0);
    else
    {
        size_t len;
        char * t;

        REQUIRE(arb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin) && x->fin.backend == ADF_GLOBAL);
        t = adf_adele_get_str(&len, x, digits);
        check_printed_kind(t, len, ADF_TEXT_ADELE);
        REQUIRE(adf_adele_set_str(y, t, len, prec, NULL) == ADF_OK);
        REQUIRE(adf_fball_identical(&x->fin, &y->fin) && arb_contains(y->inf, x->inf));
        flint_free(t);
    }
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
    arb_clear(y->inf);
    adf_fball_clear(&y->fin);
}

static void
fuzz_cadele(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind, slong prec,
            slong digits)
{
    adf_cadele_t x, y;
    adf_cadele_struct copy;
    int st;

    acb_init(x->inf);
    adf_fball_init(&x->fin);
    acb_init(y->inf);
    adf_fball_init(&y->fin);
    acb_set_si(x->inf, 3);
    fmpz_set_si(x->fin.A, 5);
    fmpz_set_si(x->fin.H, 18);
    memcpy(&copy, x, sizeof(copy));
    st = adf_cadele_set_str(x, s, n, prec, lim);
    agree(st, cst, kind, ADF_TEXT_CADELE);
    if (st != ADF_OK)
        REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && arf_equal_si(arb_midref(acb_realref(x->inf)), 3)
                && arb_is_zero(acb_imagref(x->inf)) && fmpz_cmp_si(x->fin.A, 5) == 0);
    else
    {
        size_t len;
        char * t;

        REQUIRE(acb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin) && x->fin.backend == ADF_GLOBAL);
        t = adf_cadele_get_str(&len, x, digits);
        check_printed_kind(t, len, ADF_TEXT_CADELE);
        REQUIRE(adf_cadele_set_str(y, t, len, prec, NULL) == ADF_OK);
        REQUIRE(adf_fball_identical(&x->fin, &y->fin) && acb_contains(y->inf, x->inf));
        flint_free(t);
    }
    acb_clear(x->inf);
    adf_fball_clear(&x->fin);
    acb_clear(y->inf);
    adf_fball_clear(&y->fin);
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    /* a copy of exactly size bytes, without a terminator: a read past the end is caught */
    char * s = (char *) malloc(size > 0 ? size : 1);
    const char * p = size > 0 ? s : NULL;
    adf_text_limits_t lim;
    adf_text_kind kind;
    int kind_sentinel = 77, cst;
    slong prec = 64, digits = ADF_DIGITS_DEFAULT;

    if (size > 0)
    {
        memcpy(s, data, size);
        prec = 2 + (slong) (data[0] % 190);
        digits = 1 + (slong) (data[size - 1] % 30);
    }
    adf_text_limits_default(&lim);
    lim.max_exp10 = 5000;

    memcpy(&kind, &kind_sentinel, sizeof(kind));
    cst = adf_text_classify(&kind, p, size, &lim);
    REQUIRE(cst == ADF_OK || cst == ADF_PARSE || cst == ADF_LIMIT);
    if (cst != ADF_OK)
        REQUIRE(memcmp(&kind, &kind_sentinel, sizeof(kind)) == 0);
    else
        REQUIRE((int) kind >= ADF_TEXT_RAT && (int) kind <= ADF_TEXT_CHAR);

    fuzz_rat(p, size, &lim, cst, kind);
    fuzz_fball(p, size, &lim, cst, kind);
    fuzz_adele(p, size, &lim, cst, kind, prec, digits);
    fuzz_cadele(p, size, &lim, cst, kind, prec, digits);

    free(s);
    flint_cleanup();
    return 0;
}
