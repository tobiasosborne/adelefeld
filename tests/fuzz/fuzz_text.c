/* tests/fuzz/fuzz_text.c: the fuzzer of the value-form parsers and printers (src/text.c, work
   package 1.4; briefs of lanes m1-text and m1-repair-text).

   libFuzzer calls LLVMFuzzerTestOneInput with arbitrary bytes. The first two bytes are control
   bytes, not text: byte 0 chooses prec and byte 1 chooses digits. The remaining bytes go, without
   a terminator, to adf_text_classify and to the four parsers adf_rat_set_str, adf_fball_set_str,
   adf_adele_set_str and adf_cadele_set_str. The address and undefined-behaviour sanitizers of
   `make fuzz` watch every read of the input and every allocation.

   Finding R9 of reviewer text: the old target took prec and digits from the last and first bytes
   of the text, so an accepted adele or cadele reached only a few values (prec = 11, 12, 15, 34,
   42; digits = 3, 10, 11, 12, 14). The control bytes make every prec in 2..191 and every digits in
   1..30 reachable. Every accepted text is also printed and read back at prec = 2 and digits = 1,
   the two values the old scheme could not reach.

   src/text.c is included into this file, so that it is compiled with the sanitizers and with the
   coverage flags of `make fuzz` (the library archive is built by gcc without them); the definitions
   here take the place of the archive member text.o, which the linker then does not pull in.

   What is asserted (a violation aborts, and libFuzzer keeps the input as a crash):
   - every call returns a status of conventions 3.2 for a parser (OK, PARSE, LIMIT, DOMAIN; the four
     types have no word restriction and no sign condition), OK, PARSE or LIMIT for classify;
   - the status of stage 1 is the same for all five calls (len > max_len);
   - classify and the typed parsers agree on the syntax (conventions 9.7): a parser returns PARSE
     exactly when classify does not return its kind;
   - on a status other than ADF_OK the output is untouched (conventions 4.3): equal in value to a
     copy of the sentinel, compared field by field (finding R10: a whole-struct memcmp reads
     mantissa limbs and padding that the ordinary init did not define);
   - on ADF_OK the value satisfies its predicate (adf_rat_is_canonical, adf_fball_is_canonical and
     global, arb_is_finite / acb_is_finite for the real part); its printed text is well formed
     (ASCII, no whitespace at the ends, NUL at len), is classified as the same kind, and parses
     again: identical for adf_rat and adf_fball (conventions 9.6), and for the adele types with the
     identical finite part and a real part that contains the first one (9.6: C value-text round
     trips enclose);
   - a printer of a value with a real or complex part returns NULL with *len = 0 only when
     tx_arb_printable is false (decision M1-D6); adf_rat_get_str and adf_fball_get_str never return
     NULL.

   The limits are the defaults of conventions 8.4 except max_exp10 = 5000, which keeps a run at
   many executions per second; the code paths of larger exponents are the same. */

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

/* The finite part is still the sentinel (finding R10: field by field, not memcmp). */
static int
fin_untouched(const adf_fball_t f)
{
    return fmpz_cmp_si(f->A, 5) == 0 && fmpz_cmp_si(f->H, 18) == 0 && fmpz_is_one(f->d)
           && f->backend == ADF_GLOBAL && f->mctx == NULL && f->res == NULL;
}

static void
fuzz_rat(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind)
{
    adf_rat_t x, y;
    int st;

    adf_rat_init(x);
    adf_rat_init(y);
    fmpq_set_si(x->q, 12345, 7);
    st = adf_rat_set_str(x, s, n, lim);
    agree(st, cst, kind, ADF_TEXT_RAT);
    if (st != ADF_OK)
        REQUIRE(fmpz_cmp_si(fmpq_numref(x->q), 12345) == 0 && fmpz_cmp_si(fmpq_denref(x->q), 7) == 0);
    else
    {
        size_t len;
        char * t;

        REQUIRE(adf_rat_is_canonical(x));
        t = adf_rat_get_str(&len, x);
        REQUIRE(t != NULL);
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
    int st;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 18);
    st = adf_fball_set_str(x, s, n, lim);
    agree(st, cst, kind, ADF_TEXT_FBALL);
    if (st != ADF_OK)
        REQUIRE(fin_untouched(x));
    else
    {
        size_t len;
        char * t;

        REQUIRE(adf_fball_is_canonical(x) && x->backend == ADF_GLOBAL);
        t = adf_fball_get_str(&len, x);
        REQUIRE(t != NULL);
        check_printed_kind(t, len, ADF_TEXT_FBALL);
        REQUIRE(adf_fball_set_str(y, t, len, NULL) == ADF_OK && adf_fball_identical(x, y));
        flint_free(t);
    }
    adf_fball_clear(x);
    adf_fball_clear(y);
}

/* An accepted adele or cadele text is read back at prec and digits and, for the coverage of R9,
   again at prec = 2 and digits = 1. */
static void
roundtrip_adele(const adf_adele_t x, slong prec, slong digits)
{
    adf_adele_t y;
    size_t len;
    char * t;

    arb_init(y->inf);
    adf_fball_init(&y->fin);
    t = adf_adele_get_str(&len, x, digits);
    if (t == NULL)
    {
        REQUIRE(len == 0 && !tx_arb_printable(x->inf));
    }
    else
    {
        check_printed_kind(t, len, ADF_TEXT_ADELE);
        REQUIRE(adf_adele_set_str(y, t, len, prec, NULL) == ADF_OK);
        REQUIRE(adf_fball_identical(&x->fin, &y->fin) && arb_contains(y->inf, x->inf));
        flint_free(t);
    }
    arb_clear(y->inf);
    adf_fball_clear(&y->fin);
}

static void
roundtrip_cadele(const adf_cadele_t x, slong prec, slong digits)
{
    adf_cadele_t y;
    size_t len;
    char * t;

    acb_init(y->inf);
    adf_fball_init(&y->fin);
    t = adf_cadele_get_str(&len, x, digits);
    if (t == NULL)
    {
        REQUIRE(len == 0 && (!tx_arb_printable(acb_realref(x->inf))
                             || !tx_arb_printable(acb_imagref(x->inf))));
    }
    else
    {
        check_printed_kind(t, len, ADF_TEXT_CADELE);
        REQUIRE(adf_cadele_set_str(y, t, len, prec, NULL) == ADF_OK);
        REQUIRE(adf_fball_identical(&x->fin, &y->fin) && acb_contains(y->inf, x->inf));
        flint_free(t);
    }
    acb_clear(y->inf);
    adf_fball_clear(&y->fin);
}

static void
fuzz_adele(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind, slong prec,
           slong digits)
{
    adf_adele_t x;
    int st;

    arb_init(x->inf);
    adf_fball_init(&x->fin);
    arb_set_si(x->inf, 3);
    fmpz_set_si(x->fin.A, 5);
    fmpz_set_si(x->fin.H, 18);
    st = adf_adele_set_str(x, s, n, prec, lim);
    agree(st, cst, kind, ADF_TEXT_ADELE);
    if (st != ADF_OK)
        REQUIRE(arf_equal_si(arb_midref(x->inf), 3) && mag_is_zero(arb_radref(x->inf))
                && fin_untouched(&x->fin));
    else
    {
        REQUIRE(arb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin) && x->fin.backend == ADF_GLOBAL);
        roundtrip_adele(x, prec, digits);
        roundtrip_adele(x, 2, 1);        /* the R9 values, reached for every accepted text */
    }
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

static void
fuzz_cadele(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind, slong prec,
            slong digits)
{
    adf_cadele_t x;
    int st;

    acb_init(x->inf);
    adf_fball_init(&x->fin);
    acb_set_si(x->inf, 3);
    fmpz_set_si(x->fin.A, 5);
    fmpz_set_si(x->fin.H, 18);
    st = adf_cadele_set_str(x, s, n, prec, lim);
    agree(st, cst, kind, ADF_TEXT_CADELE);
    if (st != ADF_OK)
        REQUIRE(arf_equal_si(arb_midref(acb_realref(x->inf)), 3) && arb_is_zero(acb_imagref(x->inf))
                && fin_untouched(&x->fin));
    else
    {
        REQUIRE(acb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin) && x->fin.backend == ADF_GLOBAL);
        roundtrip_cadele(x, prec, digits);
        roundtrip_cadele(x, 2, 1);       /* the R9 values, reached for every accepted text */
    }
    acb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    /* two control bytes choose prec and digits; the text is the rest, copied without a
       terminator, so a read past the end is caught by the sanitizers */
    char * s;
    const char * p;
    size_t n;
    adf_text_limits_t lim;
    adf_text_kind kind;
    int kind_sentinel = 77, cst;
    slong prec, digits;

    if (size < 2)
        return 0;
    s = (char *) malloc(size);
    if (s == NULL)
        return 0;
    memcpy(s, data, size);
    prec = 2 + (slong) (data[0] % 190);
    digits = 1 + (slong) (data[1] % 30);
    p = s + 2;
    n = size - 2;

    adf_text_limits_default(&lim);
    lim.max_exp10 = 5000;

    memcpy(&kind, &kind_sentinel, sizeof(kind));
    cst = adf_text_classify(&kind, p, n, &lim);
    REQUIRE(cst == ADF_OK || cst == ADF_PARSE || cst == ADF_LIMIT);
    if (cst != ADF_OK)
        REQUIRE(memcmp(&kind, &kind_sentinel, sizeof(kind)) == 0);
    else
        REQUIRE((int) kind >= ADF_TEXT_RAT && (int) kind <= ADF_TEXT_CHAR);

    fuzz_rat(p, n, &lim, cst, kind);
    fuzz_fball(p, n, &lim, cst, kind);
    fuzz_adele(p, n, &lim, cst, kind, prec, digits);
    fuzz_cadele(p, n, &lim, cst, kind, prec, digits);

    free(s);
    flint_cleanup();
    return 0;
}
