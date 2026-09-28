/* tests/test_scaffold.c: work package 0.1. The build scaffold has to prove that the FLINT include
   path, the link path and the test runner work. This test does nothing else: it is not a test of
   adelefeld, which has no public types yet. It must keep working: if it stops, the build is
   broken, not the mathematics. */

#include <flint/fmpz.h>
#include <flint/flint.h>

#include <string.h>

#include <adelefeld.h>

#include "test_runner.h"

/* The FLINT the plan names is 3.0.1. The macros are in flint.h, lines 94 to 99. */

ADF_TEST(flint_is_version_3_x)
{
    ADF_CHECK(__FLINT_VERSION == 3);
    ADF_CHECK_MSG(__FLINT_VERSION_MINOR == 0 && __FLINT_VERSION_PATCHLEVEL == 1,
                  "expected FLINT 3.0.1, found %d.%d.%d", __FLINT_VERSION, __FLINT_VERSION_MINOR,
                  __FLINT_VERSION_PATCHLEVEL);
    ADF_CHECK(__FLINT_RELEASE == 30001);
    ADF_CHECK(strcmp(FLINT_VERSION, "3.0.1") == 0);
    ADF_CHECK(strcmp(flint_version, "3.0.1") == 0);
}

/* fmpz arithmetic from libflint, so that the link path is exercised as well as the headers. */

ADF_TEST(fmpz_two_plus_three_is_five)
{
    fmpz_t a, b, s;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(s);
    fmpz_set_si(a, 2);
    fmpz_set_si(b, 3);
    fmpz_add(s, a, b);
    ADF_CHECK(fmpz_equal_si(s, 5));
    ADF_CHECK_MSG(fmpz_is_zero(s) == 0, "2 + 3 is zero");
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(s);
}

/* The public header carries the version macros; its types are tested in test_headers.c and test_abi.c. */

ADF_TEST(public_header_version)
{
    ADF_CHECK(ADF_VERSION_MAJOR == 0);
    ADF_CHECK(ADF_VERSION_MINOR == 1);
    ADF_CHECK(ADF_VERSION_PATCH == 0);
}
