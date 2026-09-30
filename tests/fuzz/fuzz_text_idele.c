/* tests/fuzz/fuzz_text_idele.c: the fuzzer of the value-form parsers and printers of the unit coset, the idele and
   the idele class (src/text.c, section "adf_ucoset, adf_idele, adf_idclass", and src/text_idele.c; lane t-slice1,
   milestone 2), in the way of tests/fuzz/fuzz_text.c.

   libFuzzer calls LLVMFuzzerTestOneInput with arbitrary bytes. The first two bytes are control bytes, not text:
   byte 0 chooses prec (1 to 189, and below 2 for a few values, so that the rule "a prec below 2 is 2" is reached;
   the value 255 chooses a prec above ADF_IDELE_PREC_MAX), byte 1 chooses digits (1 to 30). The remaining bytes go,
   without a terminator, to adf_text_classify and to the three parsers adf_ucoset_set_str, adf_idele_set_str and
   adf_idclass_set_str. The address and undefined-behaviour sanitizers of `make fuzz` watch every read and every
   allocation.

   src/text.c and src/text_idele.c are included into this file, so that they are compiled with the sanitizers
   and the coverage flags of `make fuzz`; the definitions here take the place of the archive members text.o and
   text_idele.o, which the linker then does not pull in.

   What is asserted (a violation aborts, and libFuzzer keeps the input as a crash):
   - the status is one of conventions 3.2 for a reader: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_DOMAIN, and for the idele and
     the class also ADF_NOT_DETERMINED (never for the unit coset); a prec above ADF_IDELE_PREC_MAX is ADF_LIMIT for
     the idele and the class, whatever the text;
   - classify and the typed parsers agree on the syntax (conventions 9.7): a parser returns PARSE exactly when
     classify does not return its kind, except for the prec rule above;
   - on a status other than ADF_OK the output is untouched (conventions 4.3), compared field by field;
   - on ADF_OK the value satisfies its predicate (5.6, 5.7); its printed text is well formed (ASCII, no whitespace at
     the ends, NUL at len), is classified as the same kind, and is read again: for the unit coset the same set in
     the normal form, for the idele and the class the content and the unit set equal and a ball that contains the
     first one (conventions 9.6: value-text round trips enclose), unless the re-reading is refused with ADF_LIMIT
     (an exponent one above max_exp10) or ADF_NOT_DETERMINED (two ends of the printed interval more than prec
     binades apart);
   - a printer returns NULL with *len = 0 only when tx_arb_printable is false (decision M1-D6), and never for a
     unit coset.

   The limits are the defaults of conventions 8.4 except max_exp10 = 5000, which keeps a run at many executions per
   second (the reading of the printed text uses max_exp10 = 20000, the printed exponent being at most a few above
   the read one plus the digits of a radius). */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/text.c"
#include "../../src/text_idele.c"

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

static void
check_printed_kind(const char * t, size_t len, int k)
{
    adf_text_kind kind;

    REQUIRE(well_formed(t, len));
    REQUIRE(adf_text_classify(&kind, t, len, NULL) == ADF_OK && (int) kind == k);
}

/* The syntax of the parser agrees with classification: PARSE exactly when the kind is not k. */
static void
agree(int st, int cst, adf_text_kind kind, int k, int has_prec, slong prec)
{
    if (has_prec && prec > ADF_IDELE_PREC_MAX)
    {
        REQUIRE(st == ADF_LIMIT);
        return;
    }
    REQUIRE(st == ADF_OK || st == ADF_PARSE || st == ADF_LIMIT || st == ADF_DOMAIN
            || (has_prec && st == ADF_NOT_DETERMINED));
    if (cst == ADF_LIMIT)
        REQUIRE(st == ADF_LIMIT);
    else if (cst == ADF_OK && (int) kind == k)
        REQUIRE(st != ADF_PARSE);
    else
        REQUIRE(st == ADF_PARSE);
}

static int
unit_untouched(const adf_ucoset_t u)
{
    return fmpz_cmp_si(u->c, 5) == 0 && fmpz_cmp_si(u->N, 18) == 0;
}

static void
set_sentinel_unit(adf_ucoset_t u)
{
    fmpz_set_si(u->c, 5);
    fmpz_set_si(u->N, 18);
}

static int
arb_untouched(const arb_t x)
{
    return arf_equal_si(arb_midref(x), 3) && mag_is_zero(arb_radref(x));
}

static void
fuzz_ucoset(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind)
{
    adf_ucoset_t x, y;
    int st;

    adf_ucoset_init(x);
    adf_ucoset_init(y);
    set_sentinel_unit(x);
    st = adf_ucoset_set_str(x, s, n, lim);
    agree(st, cst, kind, ADF_TEXT_UCOSET, 0, 0);
    if (st != ADF_OK)
        REQUIRE(unit_untouched(x));
    else
    {
        size_t len;
        char * t;

        REQUIRE(adf_ucoset_is_canonical(x));
        t = adf_ucoset_get_str(&len, x);
        REQUIRE(t != NULL);
        check_printed_kind(t, len, ADF_TEXT_UCOSET);
        REQUIRE(adf_ucoset_set_str(y, t, len, NULL) == ADF_OK && adf_ucoset_equal_set(x, y)
                && adf_ucoset_is_normal(y));
        flint_free(t);
    }
    adf_ucoset_clear(x);
    adf_ucoset_clear(y);
}

static void
fuzz_idele(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind, slong prec,
           slong digits)
{
    adf_idele_t x, y;
    adf_text_limits_t lim2;
    int st;

    adf_idele_init(x);
    adf_idele_init(y);
    arb_set_si(x->inf, 3);
    fmpq_set_si(x->r, 7, 5);
    set_sentinel_unit(&x->u);
    st = adf_idele_set_str(x, s, n, prec, lim);
    agree(st, cst, kind, ADF_TEXT_IDELE, 1, prec);
    if (st != ADF_OK)
        REQUIRE(arb_untouched(x->inf) && fmpz_cmp_si(fmpq_numref(x->r), 7) == 0
                && fmpz_cmp_si(fmpq_denref(x->r), 5) == 0 && unit_untouched(&x->u));
    else
    {
        size_t len;
        char * t;
        int st2;

        REQUIRE(adf_idele_is_canonical(x));
        t = adf_idele_get_str(&len, x, digits);
        if (t == NULL)
            REQUIRE(len == 0 && !tx_arb_printable(x->inf));
        else
        {
            check_printed_kind(t, len, ADF_TEXT_IDELE);
            adf_text_limits_default(&lim2);
            lim2.max_exp10 = 20000;
            st2 = adf_idele_set_str(y, t, len, prec, &lim2);
            REQUIRE(st2 == ADF_OK || st2 == ADF_NOT_DETERMINED || st2 == ADF_LIMIT);
            if (st2 == ADF_OK)
            {
                REQUIRE(arb_contains(y->inf, x->inf) && fmpq_equal(x->r, y->r)
                        && adf_ucoset_equal_set(&x->u, &y->u));
            }
            flint_free(t);
        }
    }
    adf_idele_clear(x);
    adf_idele_clear(y);
}

static void
fuzz_idclass(const char * s, size_t n, const adf_text_limits_t * lim, int cst, adf_text_kind kind, slong prec,
             slong digits)
{
    adf_idclass_t x, y;
    adf_text_limits_t lim2;
    int st;

    adf_idclass_init(x);
    adf_idclass_init(y);
    arb_set_si(x->t, 3);
    set_sentinel_unit(&x->u);
    st = adf_idclass_set_str(x, s, n, prec, lim);
    agree(st, cst, kind, ADF_TEXT_IDCLASS, 1, prec);
    if (st != ADF_OK)
        REQUIRE(arb_untouched(x->t) && unit_untouched(&x->u));
    else
    {
        size_t len;
        char * t;
        int st2;

        REQUIRE(adf_idclass_is_canonical(x));
        t = adf_idclass_get_str(&len, x, digits);
        if (t == NULL)
            REQUIRE(len == 0 && !tx_arb_printable(x->t));
        else
        {
            check_printed_kind(t, len, ADF_TEXT_IDCLASS);
            adf_text_limits_default(&lim2);
            lim2.max_exp10 = 20000;
            st2 = adf_idclass_set_str(y, t, len, prec, &lim2);
            REQUIRE(st2 == ADF_OK || st2 == ADF_NOT_DETERMINED || st2 == ADF_LIMIT);
            if (st2 == ADF_OK)
                REQUIRE(arb_contains(y->t, x->t) && adf_ucoset_equal_set(&x->u, &y->u));
            flint_free(t);
        }
    }
    adf_idclass_clear(x);
    adf_idclass_clear(y);
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    /* two control bytes choose prec and digits; the text is the rest, copied without a terminator, so a read
       past the end is caught by the sanitizers */
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
    prec = data[0] == 255 ? (slong) ADF_IDELE_PREC_MAX + 1 : (slong) (data[0] % 191) - 1;
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

    fuzz_ucoset(p, n, &lim, cst, kind);
    fuzz_idele(p, n, &lim, cst, kind, prec, digits);
    fuzz_idclass(p, n, &lim, cst, kind, prec, digits);

    free(s);
    flint_cleanup();
    return 0;
}
