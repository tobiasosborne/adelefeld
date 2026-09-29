/* tests/test_common_unsupported.c: the status ADF_UNSUPPORTED of adf_version_check.

   The gap this file closes: no test reached the `return ADF_UNSUPPORTED;` of src/common.c:161.
   On a machine whose FLINT is the one the library was compiled against (that is, every machine
   the build is tested on, conventions 12.11) the two strings agree, the comparison at
   src/common.c:158 is 1, and the function returns before that line. tests/test_common.c does
   reach the comparison, but through the test-only entry of src/common.c, so the status itself
   was never returned by anything that a test called.

   How the line is reached here. adf_version_check reads one string of FLINT, flint_version
   (flint.h:105, `FLINT_DLL extern char flint_version[];`, an array, and FLINT_DLL is empty on
   this platform, flint.h:43-45), and compares it with adf_flint_version_compiled(), which is the
   string constant FLINT_VERSION of the header the library was compiled against (flint.h:97,
   "3.0.1"). This file gives the program its own definition of the array:

       char flint_version[32] = "3.0.1";

   The static linker resolves the reference of the library to this definition, because it is
   already defined when the archive is read; and at load time the definition of the executable
   comes before the one of libflint.so in the lookup order of the dynamic linker, so the library
   reads this array and not FLINT's own. Nothing of the library is recompiled and no other file
   of the tree is changed: this is a definition in a test program, and the array is writable, so
   a test can put any version string in it and call the real adf_version_check.

   That the override is in force is not assumed: the first test puts "3.0.1" in the array, which
   is the string of the installed FLINT, and requires ADF_OK. If the library read FLINT's own
   array, the other tests of this file would see the same answer for every string and would fail.

   What is checked:
   1. the same major and minor as the compiled version gives ADF_OK, whatever the patch number
      and whatever text follows the minor number (conventions 12.11 reads the major and the minor
      only);
   2. a different minor gives ADF_UNSUPPORTED, and a different major gives ADF_UNSUPPORTED;
   3. a string that is not a version at all gives ADF_UNSUPPORTED, since nothing then proves
      that the two versions agree;
   4. the status is the code 8 of ADF_UNSUPPORTED and its name is "UNSUPPORTED"
      (conventions 3.1, include/adelefeld/status.h:26 and 63);
   5. the function writes nothing: it takes no argument and returns the same answer when it is
      called twice. */

#include <adelefeld.h>

#include <string.h>

#include "test_runner.h"

/* The override of flint_version (flint.h:105); see the comment of this file. The array is
   writable on purpose: a test writes the version string it wants into it. 32 is enough for
   every string below and leaves room for the terminator. */
char flint_version[32] = "3.0.1";

/* Put a version string in the array of the override, and say which one, so that a failure names
   it. */
static void
adf_put_version(const char *v)
{
    size_t n = strlen(v);

    ADF_CHECK_MSG(n + 1 <= sizeof(flint_version), "\"%s\" does not fit in %lu bytes", v,
                  (unsigned long) sizeof(flint_version));
    memcpy(flint_version, v, n + 1);
}

/* ---- 1. the versions agree ---- */

ADF_TEST(the_check_accepts_the_same_major_and_minor)
{
    /* The string of the installed FLINT, the case of the header: ADF_OK. This is also the check
       that the override of flint_version is the array the library reads. */
    adf_put_version("3.0.1");
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "flint_version is \"%s\", the library was "
                                                "compiled against \"%s\", the check said %d",
                  flint_version, adf_flint_version_compiled(), adf_version_check());

    /* The patch number is not read (conventions 12.11: "the same major and minor version"). */
    adf_put_version("3.0.0");
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "the patch 0 gave %d", adf_version_check());
    adf_put_version("3.0.99");
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "the patch 99 gave %d", adf_version_check());

    /* The text after the minor number is not read either. */
    adf_put_version("3.0.1-rc1");
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "a version with a suffix gave %d",
                  adf_version_check());
    adf_put_version("3.0.1 ");
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "a version with a trailing space gave %d",
                  adf_version_check());

    /* A major written with a leading zero is the same number (src/common.c:96-101 strips the
       leading zeros of a run of digits). */
    adf_put_version("03.00.1");
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "a leading zero gave %d", adf_version_check());

    /* conventions 3.1: the code of ADF_OK is 0 and its name is "OK". */
    ADF_CHECK(ADF_OK == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_OK), "OK") == 0);
}

/* ---- 2. the versions do not agree: the line of src/common.c:161 ---- */

ADF_TEST(the_check_rejects_a_different_minor)
{
    /* The minor is the second number. The line under test is reached here. */
    adf_put_version("3.1.0");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the minor 1 gave %d, ADF_UNSUPPORTED is %d",
                  adf_version_check(), ADF_UNSUPPORTED);

    /* A two-digit minor is not the minor of one digit. */
    adf_put_version("3.10.0");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the minor 10 gave %d",
                  adf_version_check());

    /* A minor of one digit is not the minor of two. */
    adf_put_version("3.09.0");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the minor 09 gave %d",
                  adf_version_check());
}

ADF_TEST(the_check_rejects_a_different_major)
{
    adf_put_version("4.0.1");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the major 4 gave %d",
                  adf_version_check());
    adf_put_version("2.99.99");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the major 2 gave %d",
                  adf_version_check());
    adf_put_version("30.0.1");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the major 30 gave %d",
                  adf_version_check());
}

/* ---- 3. a string that is not a version ---- */

ADF_TEST(the_check_rejects_a_string_that_is_not_a_version)
{
    /* Nothing then proves that the two versions agree, and ADF_OK is a promise that they do
       (src/common.c:73-75). */
    adf_put_version("");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "the empty string gave %d",
                  adf_version_check());
    adf_put_version("3");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "a string without a dot gave %d",
                  adf_version_check());
    adf_put_version("3.");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "a dot without a minor gave %d",
                  adf_version_check());
    adf_put_version("x.y");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "a letter is not a digit, gave %d",
                  adf_version_check());
    adf_put_version(" 3.0.1");
    ADF_CHECK_MSG(adf_version_check() == ADF_UNSUPPORTED, "a leading space gave %d",
                  adf_version_check());
}

/* ---- 4. the value and the name of the status ---- */

ADF_TEST(unsupported_is_the_code_eight_and_is_named)
{
    /* include/adelefeld/status.h:26 and 63; conventions 3.1. The test states the codes of the
       status, so that a change of the numbering is caught here. */
    ADF_CHECK(ADF_UNSUPPORTED == 8);
    ADF_CHECK(strcmp(adf_status_str(ADF_UNSUPPORTED), "UNSUPPORTED") == 0);

    /* And the value the library returns is that one, not another status with a name. */
    adf_put_version("5.0.0");
    ADF_CHECK(adf_version_check() == ADF_UNSUPPORTED);
    ADF_CHECK(strcmp(adf_status_str(adf_version_check()), "UNSUPPORTED") == 0);
}

/* ---- 5. the function writes nothing ---- */

ADF_TEST(the_check_writes_nothing)
{
    /* The header says "Writes nothing", and the function has no output argument, so a second
       call on the same input gives the same status, whatever the string was. */
    adf_put_version("3.4.5");
    ADF_CHECK(adf_version_check() == ADF_UNSUPPORTED);
    ADF_CHECK(adf_version_check() == ADF_UNSUPPORTED);
    ADF_CHECK(adf_version_check() == ADF_UNSUPPORTED);

    adf_put_version("3.0.7");
    ADF_CHECK(adf_version_check() == ADF_OK);
    ADF_CHECK(adf_version_check() == ADF_OK);
}
