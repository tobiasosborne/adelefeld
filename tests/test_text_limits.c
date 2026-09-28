/* tests/test_text_limits.c: the resource limits of the value form (work package 1.4; decisions
   M1-D6 and M1-D7 of docs/SPEC.md section 15; findings R2, R3 and R4 of reviewer text).

   M1-D6: a printer of a value with a real or complex part returns NULL, with *len = 0, when the
   binary exponent (ARF_EXP, arf.h:87; MAG_EXP, mag.h:114) of a non-zero midpoint or radius is
   above ADF_PRINT_EXP_MAX in absolute value, tested before any conversion. The printers of
   adf_rat and adf_fball never return NULL (include/adelefeld/text.h:32-38).

   Mind FLINT's exponent of 2^e: probed on this machine, ARF_EXP(2^e) = e + 1 and
   MAG_EXP(2^e) = e + 1. So |ARF_EXP| <= ADF_PRINT_EXP_MAX is e in [-(ADF_PRINT_EXP_MAX + 1),
   ADF_PRINT_EXP_MAX - 1]. At ADF_PRINT_EXP_MAX = 100000: e = 99999, -100000 and -100001 are
   inside; e = 100000, 100001 and -100002 are outside. The table below marks this per row.

   M1-D7: the exponent of a decimal literal is compared with max_exp10 as a number of any length,
   with no hidden 18-digit bound; a literal whose coefficient is zero is the exact zero whatever
   its exponent, as long as the exponent is within max_exp10 (docs/SPEC.md section 15; conventions
   8.4 and 9.3). R4: the code compared the digit count with 18 and built 10^exponent even for a
   zero coefficient.

   R3: the default reader is not closed under the printer. A value read at the limit max_exp10 may
   print with a decimal exponent one above it, so reading the printed text again with the default
   limits gives ADF_LIMIT (conventions 9.6, include/adelefeld/text.h:39-42). */

#include <string.h>
#include <stdlib.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/mag.h>

#include <adelefeld.h>

#include "test_runner.h"

/* The adele and cadele fields are initialised here, field by field, as in tests/test_text_adele.c
   (the lane that owns adele.c initialises them the same way; nothing else is needed for a
   printer test). */
static void
adele_init(adf_adele_t x)
{
    arb_init(x->inf);
    adf_fball_init(&x->fin);
}

static void
adele_clear(adf_adele_t x)
{
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

static void
cadele_init(adf_cadele_t z)
{
    acb_init(z->inf);
    adf_fball_init(&z->fin);
}

static void
cadele_clear(adf_cadele_t z)
{
    acb_clear(z->inf);
    adf_fball_clear(&z->fin);
}

/* midpoint = 2^e exactly, radius = 0. */
static void
set_mid_pow2(arb_t r, const char * e_text)
{
    fmpz_t m, e;

    fmpz_init_set_ui(m, 1);
    fmpz_init(e);
    fmpz_set_str(e, e_text, 10);
    arf_set_fmpz_2exp(arb_midref(r), m, e);
    fmpz_clear(m);
    fmpz_clear(e);
}

/* midpoint = 0, radius = 2^e exactly. */
static void
set_rad_pow2(arb_t r, const char * e_text)
{
    fmpz_t m, e;

    fmpz_init_set_ui(m, 1);
    fmpz_init(e);
    fmpz_set_str(e, e_text, 10);
    mag_set_fmpz_2exp_fmpz(arb_radref(r), m, e);
    fmpz_clear(m);
    fmpz_clear(e);
}

/* The e of 2^e, whether the printer must return a text (1) or NULL with *len = 0 (0). The
   boundary row 99999 / 100000 / -100001 / -100002 is the e + 1 shift of FLINT's exponent. */
typedef struct
{
    const char * e;
    int text;
} exp_row;

static const exp_row print_rows[] = {
    {"99999", 1},                /* ARF_EXP = 100000, at the bound */
    {"100000", 0},               /* ARF_EXP = 100001, above */
    {"100001", 0},               /* ARF_EXP = 100002, above */
    {"-99999", 1},               /* ARF_EXP = -99998, inside */
    {"-100000", 1},              /* ARF_EXP = -99999, inside */
    {"-100001", 1},              /* ARF_EXP = -100000, at the bound */
    {"-100002", 0},              /* ARF_EXP = -100001, above */
    {"1000000", 0},              /* 10^6 */
    {"100000000", 0},            /* 10^8 */
    {"1099511627776", 0},        /* 2^40 */
    {"18446744073709551616", 0}, /* 2^64, an exponent beyond a word */
    {"-1000000", 0},
    {"-100000000", 0},
    {"-1099511627776", 0},
    {"-18446744073709551616", 0},
};

/* The printer returns a text for a row of kind 1 and NULL with *len = 0 for a row of kind 0. */
static void
check_printed(const adf_adele_t x, const adf_cadele_t z, int real_part, const char * e, int want_text,
              const char * where)
{
    size_t len = (size_t) 12345;
    char * t;

    if (real_part)
        t = adf_adele_get_str(&len, x, ADF_DIGITS_DEFAULT);
    else
        t = adf_cadele_get_str(&len, z, ADF_DIGITS_DEFAULT);
    if (want_text)
    {
        ADF_CHECK_MSG(t != NULL && len == strlen(t), "%s: e=%s expected a text, len=%zu",
                      where, e, len);
        if (t != NULL)
            adf_str_free(t);
    }
    else
    {
        ADF_CHECK_MSG(t == NULL && len == 0, "%s: e=%s expected NULL with len 0, t=%p len=%zu",
                      where, e, (void *) t, len);
        adf_str_free(t);
    }
}

/* Print once per row after that row's value has been set. */
#define CHECK_ROW(field, call, sign)                                                        \
    do                                                                                      \
    {                                                                                       \
        size_t k_;                                                                          \
        for (k_ = 0; k_ < sizeof(print_rows) / sizeof(print_rows[0]); k_++)                 \
        {                                                                                   \
            field(x->inf, print_rows[k_].e);                                                \
            check_printed(x, NULL, call, print_rows[k_].e, print_rows[k_].text, sign);      \
        }                                                                                   \
    } while (0)

ADF_TEST(printer_bound_adele_midpoint)
{
    adf_adele_t x;

    adele_init(x);
    CHECK_ROW(set_mid_pow2, 1, "adele midpoint 2^e");
    adele_clear(x);
}

ADF_TEST(printer_bound_adele_radius)
{
    adf_adele_t x;

    adele_init(x);
    CHECK_ROW(set_rad_pow2, 1, "adele radius 2^e");
    adele_clear(x);
}

ADF_TEST(printer_bound_cadele_imaginary_midpoint)
{
    adf_cadele_t z;
    size_t k;

    cadele_init(z);
    for (k = 0; k < sizeof(print_rows) / sizeof(print_rows[0]); k++)
    {
        set_mid_pow2(acb_imagref(z->inf), print_rows[k].e);
        check_printed(NULL, z, 0, print_rows[k].e, print_rows[k].text, "cadele imaginary midpoint 2^e");
    }
    cadele_clear(z);
}

ADF_TEST(printer_bound_cadele_imaginary_radius)
{
    adf_cadele_t z;
    size_t k;

    cadele_init(z);
    for (k = 0; k < sizeof(print_rows) / sizeof(print_rows[0]); k++)
    {
        set_rad_pow2(acb_imagref(z->inf), print_rows[k].e);
        check_printed(NULL, z, 0, print_rows[k].e, print_rows[k].text, "cadele imaginary radius 2^e");
    }
    cadele_clear(z);
}

/* The real part is checked too: a bounded midpoint with an over-bound real radius must refuse. */
ADF_TEST(printer_bound_cadele_real_part)
{
    adf_cadele_t z;
    size_t len = (size_t) 12345;
    char * t;

    cadele_init(z);
    set_mid_pow2(acb_realref(z->inf), "99999");
    set_rad_pow2(acb_realref(z->inf), "100000");
    t = adf_cadele_get_str(&len, z, ADF_DIGITS_DEFAULT);
    ADF_CHECK(t == NULL && len == 0);
    adf_str_free(t);
    cadele_clear(z);
}

/* A zero value with no real part may still carry the other coordinate; the printer of adf_rat and
   adf_fball never returns NULL (text.h:38). */
ADF_TEST(fball_printer_never_null)
{
    adf_fball_t f;
    adf_rat_t q;
    size_t len;
    char * t;

    adf_fball_init(f);
    adf_rat_init(q);
    adf_rat_set_si(q, 7);
    adf_fball_set_rat(f, q);
    t = adf_fball_get_str(&len, f);
    ADF_CHECK(t != NULL && len == strlen(t));
    adf_str_free(t);
    adf_rat_clear(q);
    adf_fball_clear(f);
}

/* ---- M1-D7 / R4: the decimal exponent is compared with max_exp10, no hidden 18-digit bound ---- */

/* Parse as an adele and return the status; on ADF_OK check that the value is exactly zero. */
static int
parse_zero_ok(const char * s, const adf_text_limits_t * lim, const char * where)
{
    adf_adele_t x;
    int st;

    adele_init(x);
    st = adf_adele_set_str(x, s, strlen(s), 64, lim);
    ADF_CHECK_MSG(st == ADF_OK, "%s: status %d, expected OK", where, st);
    if (st == ADF_OK)
        ADF_CHECK_MSG(arb_is_zero(x->inf) && fmpz_is_zero(x->fin.A) && fmpz_is_zero(x->fin.H)
                          && fmpz_is_one(x->fin.d),
                      "%s: not the exact zero", where);
    adele_clear(x);
    return st;
}

static int
parse_status(const char * s, const adf_text_limits_t * lim, const char * where)
{
    adf_adele_t x;
    int st;

    adele_init(x);
    st = adf_adele_set_str(x, s, strlen(s), 64, lim);
    adele_clear(x);
    ADF_CHECK_MSG(st == ADF_LIMIT || st == ADF_OK, "%s: status %d", where, st);
    return st;
}

ADF_TEST(exp10_limit_of_nineteen_digits)
{
    adf_text_limits_t lim;

    adf_text_limits_default(&lim);
    lim.max_exp10 = 1000000000000000000;      /* 10^18, nineteen digits */
    parse_zero_ok("(0e1000000000000000000 ; 0)", &lim, "zero at 10^18");
    parse_zero_ok("(0e-1000000000000000000 ; 0)", &lim, "zero at -10^18");
    ADF_CHECK(parse_status("(0e1000000000000000001 ; 0)", &lim, "zero above") == ADF_LIMIT);
    ADF_CHECK(parse_status("(1e1000000000000000001 ; 0)", &lim, "nonzero above") == ADF_LIMIT);
    ADF_CHECK(parse_status("(0e-1000000000000000001 ; 0)", &lim, "zero below") == ADF_LIMIT);
}

ADF_TEST(exp10_limit_of_nineteen_significant_digits)
{
    adf_text_limits_t lim;

    adf_text_limits_default(&lim);
    lim.max_exp10 = WORD_MAX;               /* 9223372036854775807, nineteen digits */
    parse_zero_ok("(0e9223372036854775807 ; 0)", &lim, "zero at WORD_MAX");
    parse_zero_ok("(0e9223372036854775806 ; 0)", &lim, "zero one below");
    parse_zero_ok("(0e-9223372036854775807 ; 0)", &lim, "zero at -WORD_MAX");
    ADF_CHECK(parse_status("(0e9223372036854775808 ; 0)", &lim, "zero above WORD_MAX") == ADF_LIMIT);
    ADF_CHECK(parse_status("(1e9223372036854775808 ; 0)", &lim, "nonzero above WORD_MAX") == ADF_LIMIT);
    ADF_CHECK(parse_status("(0e-9223372036854775808 ; 0)", &lim, "zero below WORD_MAX") == ADF_LIMIT);
}

ADF_TEST(exp10_limit_of_twenty_digits)
{
    adf_text_limits_t lim;

    adf_text_limits_default(&lim);
    lim.max_exp10 = WORD_MAX;
    /* Twenty digits: above any value that fits a signed 64-bit limit. */
    ADF_CHECK(parse_status("(0e92233720368547758070 ; 0)", &lim, "20 digits zero") == ADF_LIMIT);
    ADF_CHECK(parse_status("(0e99999999999999999999 ; 0)", &lim, "20 digits zero") == ADF_LIMIT);
    /* Leading zeros of a short exponent are still that short exponent. */
    parse_zero_ok("(0e00000000000000000000000000000001 ; 0)", &lim, "many leading zeros");
}

ADF_TEST(exp10_limit_negative_and_zero)
{
    adf_text_limits_t lim;

    adf_text_limits_default(&lim);
    lim.max_exp10 = 0;
    parse_zero_ok("(0e0 ; 0)", &lim, "zero at max 0");
    ADF_CHECK(parse_status("(0e1 ; 0)", &lim, "zero above max 0") == ADF_LIMIT);
    lim.max_exp10 = -1;
    ADF_CHECK(parse_status("(0e0 ; 0)", &lim, "zero at max -1") == ADF_LIMIT);
    parse_zero_ok("(0 ; 0)", &lim, "no exponent at max -1");
}

/* ---- R3: a value at the max_exp10 boundary prints one decimal exponent above it ---- */

/* M1-D6 makes the reader closed at the default limit: a value at decimal exponent 100000 has a
   binary exponent near 332000, above ADF_PRINT_EXP_MAX, so the printer returns NULL and there is
   no text to read back. The qualification of conventions 9.6 and text.h:39-42 is still visible
   with a smaller max_exp10, where the binary bound is not reached: a value read at max_exp10 = 21
   prints with decimal exponent 22 (scientific notation, since X > 20), so the reader at
   max_exp10 = 21 gives ADF_LIMIT and max_exp10 = 22 gives ADF_OK and an enclosure. */
ADF_TEST(value_at_exp10_boundary_prints_one_above)
{
    adf_adele_t x, y;
    adf_text_limits_t lim;
    const char * refused_text = "(9.99e100000 ; 0)";
    const char * text = "(9.99e21 ; 0)";
    size_t len = (size_t) 12345;
    char * t;
    int st;

    adele_init(x);
    adele_init(y);

    /* the reviewer's example is refused by M1-D6 before any conversion */
    st = adf_adele_set_str(x, refused_text, strlen(refused_text), 128, NULL);
    ADF_CHECK_MSG(st == ADF_OK, "input status %d", st);
    t = adf_adele_get_str(&len, x, 1);
    ADF_CHECK_MSG(t == NULL && len == 0, "M1-D6 did not refuse the over-bound value");
    adf_str_free(t);

    /* the documented round-trip qualification at a small limit */
    adf_text_limits_default(&lim);
    lim.max_exp10 = 21;
    st = adf_adele_set_str(x, text, strlen(text), 128, &lim);
    ADF_CHECK_MSG(st == ADF_OK, "boundary input status %d", st);
    t = adf_adele_get_str(&len, x, 1);
    ADF_CHECK_MSG(t != NULL, "digits = 1 refused a printable value");
    if (t != NULL)
    {
        ADF_CHECK_MSG(adf_adele_set_str(y, t, len, 128, &lim) == ADF_LIMIT,
                      "the reader at max_exp10 = 21 accepted its own printed text: %s", t);
        lim.max_exp10 = 22;
        st = adf_adele_set_str(y, t, len, 128, &lim);
        ADF_CHECK_MSG(st == ADF_OK, "raised reader status %d for %s", st, t);
        if (st == ADF_OK)
            ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "the re-read value does not enclose");
        adf_str_free(t);
    }
    adele_clear(x);
    adele_clear(y);
}
