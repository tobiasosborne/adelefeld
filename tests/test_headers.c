/* tests/test_headers.c: the public headers of milestone 1 (lane m1-headers).

   What it checks, with no library function called (the library has no implementation yet; only
   functions marked ADF_INLINE in the headers are called, and they are compiled into this file):
   1. All headers together, and each header a second time (include guards), compile in C11 under
      the Makefile's -Wall -Wextra -Wpedantic -Werror. Each header alone, and C++17 with g++, are
      compiled by lanes/m1-headers/check_headers.sh, which the Makefile does not run yet.
   2. The status codes have the numeric values of docs/conventions.md 3.1, and adf_status_str
      spells them as the golden files do (conventions 11.1).
   3. The point comparison values of conventions 2.1 (closure finding C1), the text kinds of
      conventions 9.7, the limits of conventions 8.4, the version 0.1.0 of the brief.
   4. The signatures that conventions write out in full (sections 7, 8.1, 9.7, 10.2, 5.4 with the
      closure edits C2 and C5, 4.6, 12.8, 12.11) are pinned by _Generic on the function's address.
      The controlling expression of _Generic is not evaluated (C11 6.5.1.1p3), so no symbol is
      referenced and nothing needs to be linked. */

#include <adelefeld.h>

/* Include guards: every header a second time, after the umbrella header. */
#include <adelefeld/common.h>
#include <adelefeld/status.h>
#include <adelefeld/place.h>
#include <adelefeld/rat.h>
#include <adelefeld/fball.h>
#include <adelefeld/adele.h>
#include <adelefeld/recon.h>
#include <adelefeld/text.h>
#include <adelefeld/modctx.h>
#include <adelefeld/scaled.h>
#include <adelefeld/dump.h>
#include <adelefeld.h>

#include <string.h>

#include "test_runner.h"

/* ---- 4. signatures, checked at compile time ---- */

#define ADF_SIG(f, T) _Static_assert(_Generic(&(f), T: 1, default: 0), #f ": signature differs")

/* conventions 7 */
ADF_SIG(adf_place_inf, adf_place_t (*)(void));
ADF_SIG(adf_place_prime, int (*)(adf_place_t *, ulong));
ADF_SIG(adf_place_is_archimedean, int (*)(adf_place_t));
ADF_SIG(adf_place_prime_get, ulong (*)(adf_place_t));
ADF_SIG(adf_place_cmp, int (*)(adf_place_t, adf_place_t));
ADF_SIG(adf_place_equal, int (*)(adf_place_t, adf_place_t));

/* conventions 4.2, 12.8, 12.11 */
ADF_SIG(adf_str_free, void (*)(char *));
ADF_SIG(adf_flint_version_compiled, const char * (*)(void));
ADF_SIG(adf_version_check, int (*)(void));

/* conventions 8.1: parsers and printers of the value form */
ADF_SIG(adf_rat_set_str, int (*)(adf_rat_struct *, const char *, size_t, const adf_text_limits_t *));
ADF_SIG(adf_fball_set_str, int (*)(adf_fball_struct *, const char *, size_t, const adf_text_limits_t *));
ADF_SIG(adf_adele_set_str,
        int (*)(adf_adele_struct *, const char *, size_t, slong, const adf_text_limits_t *));
ADF_SIG(adf_cadele_set_str,
        int (*)(adf_cadele_struct *, const char *, size_t, slong, const adf_text_limits_t *));
ADF_SIG(adf_rat_get_str, char * (*)(size_t *, const adf_rat_struct *));
ADF_SIG(adf_fball_get_str, char * (*)(size_t *, const adf_fball_struct *));
ADF_SIG(adf_adele_get_str, char * (*)(size_t *, const adf_adele_struct *, slong));
ADF_SIG(adf_cadele_get_str, char * (*)(size_t *, const adf_cadele_struct *, slong));

/* conventions 8.1: loaders and dumpers (binds with the added const, see dump.h) */
ADF_SIG(adf_fball_load_str, int (*)(adf_fball_struct *, const char *, size_t,
                                    const adf_modctx_struct *, const adf_text_limits_t *));
ADF_SIG(adf_fball_load_str_binds,
        int (*)(adf_fball_struct *, const char *, size_t, const adf_modctx_struct * const *, size_t,
                const adf_text_limits_t *));
ADF_SIG(adf_scaled_load_str_binds,
        int (*)(adf_scaled_struct *, const char *, size_t, const adf_modctx_struct * const *, size_t,
                const adf_text_limits_t *));
ADF_SIG(adf_adele_dump_str, char * (*)(size_t *, const adf_adele_struct *));

/* conventions 9.7 (gate finding G14) */
ADF_SIG(adf_text_classify, int (*)(adf_text_kind *, const char *, size_t, const adf_text_limits_t *));

/* conventions 10.2 (closure finding C2) */
ADF_SIG(adf_ctx_desc_init, void (*)(adf_ctx_desc_t *));
ADF_SIG(adf_ctx_desc_clear, void (*)(adf_ctx_desc_t *));
ADF_SIG(adf_fball_dump_inspect,
        int (*)(size_t *, adf_ctx_desc_t *, const char *, size_t, const adf_text_limits_t *));
ADF_SIG(adf_modctx_new_from_dump,
        int (*)(adf_modctx_struct **, const char *, size_t, size_t, const adf_text_limits_t *));

/* conventions 4.6, 5.14: constructors take adf_modctx_struct ** first */
ADF_SIG(adf_modctx_new_blocks, int (*)(adf_modctx_struct **, const ulong *, slong));
ADF_SIG(adf_modctx_new_fmpz, int (*)(adf_modctx_struct **, const fmpz *));
ADF_SIG(adf_modctx_free, void (*)(adf_modctx_struct *));

/* conventions 5.4 (closure finding C5, verbatim signature) */
ADF_SIG(adf_scaled_set_context,
        int (*)(adf_scaled_struct *, int *, const adf_scaled_struct *, const adf_modctx_struct *));
ADF_SIG(adf_scaled_init, void (*)(adf_scaled_struct *, const adf_modctx_struct *));

/* conventions 2.1, 3.2: predicates and the point comparison return int; ring arithmetic is void */
ADF_SIG(adf_fball_equal_set, int (*)(const adf_fball_struct *, const adf_fball_struct *));
ADF_SIG(adf_fball_overlaps, int (*)(const adf_fball_struct *, const adf_fball_struct *));
ADF_SIG(adf_fball_contains, int (*)(const adf_fball_struct *, const adf_fball_struct *));
ADF_SIG(adf_fball_compare, int (*)(const adf_fball_struct *, const adf_fball_struct *));
ADF_SIG(adf_fball_add, void (*)(adf_fball_struct *, const adf_fball_struct *, const adf_fball_struct *));
ADF_SIG(adf_fball_mul, void (*)(adf_fball_struct *, const adf_fball_struct *, const adf_fball_struct *));
ADF_SIG(adf_adele_add, void (*)(adf_adele_struct *, const adf_adele_struct *, const adf_adele_struct *,
                                slong));
ADF_SIG(adf_scaled_add, int (*)(adf_scaled_struct *, const adf_scaled_struct *, const adf_scaled_struct *));
ADF_SIG(adf_rat_div, int (*)(adf_rat_struct *, const adf_rat_struct *, const adf_rat_struct *));

/* ---- 2. status values (conventions 3.1) ---- */

ADF_TEST(status_values_are_those_of_conventions_3_1)
{
    ADF_CHECK(ADF_OK == 0);
    ADF_CHECK(ADF_NOT_DETERMINED == 1);
    ADF_CHECK(ADF_UNIT_NOT_CERTIFIED == 2);
    ADF_CHECK(ADF_NEEDS_SPLIT == 3);
    ADF_CHECK(ADF_NOT_UNIQUE == 4);
    ADF_CHECK(ADF_NO_SOLUTION == 5);
    ADF_CHECK(ADF_NOT_UNIT == 6);
    ADF_CHECK(ADF_DOMAIN == 7);
    ADF_CHECK(ADF_UNSUPPORTED == 8);
    ADF_CHECK(ADF_PARSE == 9);
    ADF_CHECK(ADF_LIMIT == 10);
    ADF_CHECK(ADF_STATUS_COUNT == 11);
}

ADF_TEST(status_names_are_those_of_the_golden_files)
{
    static const char * const names[11] = {
        "OK", "NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT", "NOT_UNIQUE", "NO_SOLUTION",
        "NOT_UNIT", "DOMAIN", "UNSUPPORTED", "PARSE", "LIMIT"};
    int i;

    for (i = 0; i < ADF_STATUS_COUNT; i++)
        ADF_CHECK_MSG(strcmp(adf_status_str(i), names[i]) == 0, "status %d: got %s, want %s", i,
                      adf_status_str(i), names[i]);
    ADF_CHECK(strcmp(adf_status_str(-1), "UNKNOWN") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_STATUS_COUNT), "UNKNOWN") == 0);
    /* The combination rule of conventions 3.3 is the maximum; it needs the order above, in which
       every proved failure (NO_SOLUTION, NOT_UNIT, DOMAIN) is above every repairable one. */
    ADF_CHECK(ADF_NO_SOLUTION > ADF_NOT_UNIQUE && ADF_NOT_UNIQUE > ADF_NEEDS_SPLIT);
    ADF_CHECK(ADF_NOT_UNIT > ADF_UNIT_NOT_CERTIFIED && ADF_UNIT_NOT_CERTIFIED > ADF_NOT_DETERMINED);
}

/* ---- 3. comparison, text kinds, limits, version, backend tags ---- */

ADF_TEST(point_comparison_values_are_those_of_conventions_2_1)
{
    ADF_CHECK(ADF_CMP_EQUAL == 0);
    ADF_CHECK(ADF_CMP_DIFFERENT == 1);
    ADF_CHECK(ADF_CMP_UNDECIDED == 2);
}

ADF_TEST(text_kinds_are_the_thirteen_start_symbols_in_order)
{
    ADF_CHECK(ADF_TEXT_RAT == 0);
    ADF_CHECK(ADF_TEXT_FBALL == 1);
    ADF_CHECK(ADF_TEXT_ADELE == 2);
    ADF_CHECK(ADF_TEXT_CADELE == 3);
    ADF_CHECK(ADF_TEXT_UCOSET == 4);
    ADF_CHECK(ADF_TEXT_IDELE == 5);
    ADF_CHECK(ADF_TEXT_IDCLASS == 6);
    ADF_CHECK(ADF_TEXT_LBALL == 7);
    ADF_CHECK(ADF_TEXT_SBALL == 8);
    ADF_CHECK(ADF_TEXT_QCLASS == 9);
    ADF_CHECK(ADF_TEXT_FFUN == 10);
    ADF_CHECK(ADF_TEXT_RFUN == 11);
    ADF_CHECK(ADF_TEXT_CHAR == 12);
}

ADF_TEST(text_limits_default_are_those_of_conventions_8_4)
{
    adf_text_limits_t lim;

    memset(&lim, 0xff, sizeof(lim));
    adf_text_limits_default(&lim);
    ADF_CHECK(lim.max_len == 1048576);
    ADF_CHECK(lim.max_exp10 == 100000);
    ADF_CHECK(lim.max_prec == 100000);
    ADF_CHECK(lim.max_items == 1048576);
    ADF_CHECK(ADF_DIGITS_DEFAULT == 20);
    ADF_CHECK(ADF_DIGITS_MAX == 1000000);
}

ADF_TEST(version_is_0_1_0)
{
    ADF_CHECK(ADF_VERSION_MAJOR == 0);
    ADF_CHECK(ADF_VERSION_MINOR == 1);
    ADF_CHECK(ADF_VERSION_PATCH == 0);
    ADF_CHECK(ADF_VERSION == 100);
}

ADF_TEST(backend_tags_are_those_of_conventions_5_2)
{
    ADF_CHECK(ADF_GLOBAL == 0);
    ADF_CHECK(ADF_LOCAL == 1);
}

ADF_TEST(header_compiled_against_flint_3_0)
{
    ADF_CHECK(__FLINT_VERSION == 3);
    ADF_CHECK(__FLINT_VERSION_MINOR == 0);
    ADF_CHECK(FLINT_BITS == 64);
}
