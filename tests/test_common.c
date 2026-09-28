/* tests/test_common.c: adf_str_free, adf_flint_version_compiled and adf_version_check
   (conventions 4.2, 12.8, 12.11; decisions CV-43 and CV-44; the comment blocks of
   include/adelefeld/common.h).

   The file includes src/common.c instead of linking it, so that the static helper which
   compares two version strings is reachable. The cases that the installed FLINT cannot produce
   (a minor version 1, a string without a dot, the empty string) have to be tested somewhere,
   and the public function only ever sees flint_version. ADF_COMMON_TEST opens the one test-only
   entry in src/common.c; the library is built without the macro and does not have that symbol.

   What is checked:
   1. the compiled version string names the FLINT of the headers, is static, and is "3.0.x",
      which is what the compile-time check of common.h leaves possible;
   2. the run-time check accepts the installed FLINT, which is 3.0.1, and writes nothing;
   3. adf_str_free reaches flint_free, seen through the memory functions of FLINT 3
      (flint.h:211-217), and adf_str_free(NULL) does nothing;
   4. the comparison of two version strings reads the major and the minor number only: the nine
      cases of the brief and the ones that a wrong reader would get wrong (a zero, a nine, a
      two-digit minor, a leading zero, trailing text). */

#define ADF_COMMON_TEST 1
#include "../src/common.c"

#include <string.h>

#include "test_runner.h"

/* The test-only entry of src/common.c, declared here as the brief asks. */

int adf_test_version_major_minor_equal(const char *a, const char *b);

/* ---- 1. the compiled version string ---- */

ADF_TEST(the_compiled_version_string_names_the_flint_of_the_headers)
{
    const char *s = adf_flint_version_compiled();

    ADF_CHECK(s != NULL);
    /* The library was compiled against the FLINT whose header this file includes. */
    ADF_CHECK_MSG(strcmp(s, FLINT_VERSION) == 0, "\"%s\" against FLINT_VERSION \"%s\"", s,
                  FLINT_VERSION);
    /* The run-time string of the same library, flint.h:105. */
    ADF_CHECK_MSG(strcmp(s, flint_version) == 0, "\"%s\" against flint_version \"%s\"", s,
                  flint_version);
    /* common.h refuses at compile time any FLINT whose minor version is not 0, so the string
       begins with "3.0". */
    ADF_CHECK_MSG(strncmp(s, "3.0", 3) == 0, "\"%s\" does not begin with 3.0", s);
}

ADF_TEST(the_compiled_version_string_is_static)
{
    /* The header says the caller must not free it: it is a static string, so two calls return
       the same pointer. */
    ADF_CHECK(adf_flint_version_compiled() == adf_flint_version_compiled());
}

/* ---- 2. the run-time check ---- */

ADF_TEST(the_run_time_check_accepts_the_installed_flint)
{
    ADF_CHECK_MSG(adf_version_check() == ADF_OK, "the installed FLINT is %s, the library was "
                                                 "compiled against %s, the check said %d",
                  flint_version, adf_flint_version_compiled(), adf_version_check());
    /* conventions 3.1: the code of ADF_OK is the name "OK". */
    ADF_CHECK(strcmp(adf_status_str(adf_version_check()), "OK") == 0);
    /* Nothing is written, so a second call gives the same answer. */
    ADF_CHECK(adf_version_check() == adf_version_check());
}

/* ---- 3. the string free ---- */

/* The memory functions of FLINT 3 (flint.h:211-217): the test replaces the free function by
   one that counts the calls and then calls the one it replaced. Nothing else in the test
   changes them, and they are put back before the test returns. */

static void (*adf_saved_free)(void *);
static unsigned long adf_free_calls;

static void
adf_counting_free(void *p)
{
    adf_free_calls++;
    adf_saved_free(p);
}

ADF_TEST(adf_str_free_releases_the_memory_of_flint_malloc)
{
    void *(*alloc)(size_t);
    void *(*calloc)(size_t, size_t);
    void *(*realloc)(void *, size_t);
    char *s;

    __flint_get_memory_functions(&alloc, &calloc, &realloc, &adf_saved_free);
    adf_free_calls = 0;
    __flint_set_memory_functions(alloc, calloc, realloc, adf_counting_free);

    s = flint_malloc(6);
    ADF_CHECK(s != NULL);
    memcpy(s, "3.0.1", 6);
    ADF_CHECK(strcmp(s, "3.0.1") == 0);
    adf_str_free(s);
    ADF_CHECK_MSG(adf_free_calls == 1, "flint_free was called %lu times, not once", adf_free_calls);

    /* A second string: the count rises by one, so the first call did not free the memory twice
       and the second one did not skip the free. */
    s = flint_malloc(1);
    ADF_CHECK(s != NULL);
    *s = 0;
    adf_str_free(s);
    ADF_CHECK_MSG(adf_free_calls == 2, "flint_free was called %lu times after two frees, not twice",
                  adf_free_calls);

    __flint_set_memory_functions(alloc, calloc, realloc, adf_saved_free);
}

ADF_TEST(adf_str_free_of_null_does_nothing)
{
    /* The header: "s = NULL does nothing (as free(NULL))". The claim is that the call returns;
       how often the free function of FLINT is entered for a null pointer is not part of the
       contract, and free(NULL) does nothing either way. */
    adf_str_free(NULL);
    ADF_CHECK(1);
}

/* ---- 4. the comparison of two version strings ---- */

ADF_TEST(the_version_comparison_reads_the_major_and_the_minor_number)
{
    /* The nine cases of the brief, then the ones that a wrong reader gets wrong. The expected
       value is 1 when the major and the minor agree and 0 otherwise, including when a string is
       not a version at all: nothing then proves the two to agree. */
    static const struct
    {
        const char *a;
        const char *b;
        int same;
        const char *why;
    } cases[] = {
        /* the nine cases of the brief */
        { "3.0.1", "3.0.1", 1, "equal" },
        { "3.0.1", "3.0.2", 1, "only the patch differs" },
        { "3.0.1", "3.1.0", 0, "the minor differs" },
        { "3.0.1", "3.0", 1, "3.0 against 3.0.1" },
        { "4.0.1", "3.0.1", 0, "the major differs" },
        { "2.9.9", "3.0.0", 0, "the major differs, the other way" },
        { "3", "3.0.1", 0, "a string without a dot has no minor" },
        { "3.0.1", "3", 0, "a string without a dot has no minor, the other way" },
        { "", "3.0.1", 0, "the empty string is not a version" },
        { "3.10.0", "3.1.0", 0, "the minor 10 is not the minor 1" },
        /* the rest: the reader has to be exact about digits and lengths */
        { "", "", 0, "two empty strings" },
        { "3.1.0", "3.10.0", 0, "the minor 1 is not the minor 10" },
        { "3.10.0", "3.10.1", 1, "the same minor 10, a different patch" },
        { "3.1.0", "3.11.0", 0, "the minor 1 is not the minor 11" },
        { "3.11.0", "3.1.0", 0, "the minor 11 is not the minor 1" },
        { "3.10.0", "3.0.0", 0, "the minor 10 is not the minor 0" },
        { "3.0.0", "3.10.0", 0, "the minor 0 is not the minor 10" },
        { "3.1.0", "3.2.0", 0, "the minors 1 and 2 differ" },
        { "10.1", "0.1", 0, "the major 10 is not the major 0" },
        { "3.0.1", "3.0.0", 1, "only the patch differs, the other patch" },
        { "0.0.0", "0.0.0", 1, "a major and a minor that are the digit 0" },
        { "3.0.9", "3.0.1", 1, "a minor ending in 9" },
        { "30.9.0", "30.9.1", 1, "a two-digit major ending in 0" },
        { "03.00.1", "3.0.1", 1, "leading zeros are not part of the number" },
        { "3.", "3.0.1", 0, "nothing after the dot is no minor" },
        { "x", "3.0.1", 0, "a letter is not a digit" },
        { "3.0.1x", "3.0.1", 1, "the text after the minor is not read" },
        { "3.0.1-rc1", "3.0.1", 1, "a version with a suffix has the minor of its first part" },
        { ".0.1", "0.0.1", 0, "a string that begins with the dot has no major" },
        { ".0.1", ".0.1", 0, "a string that is not a version does not agree with itself" },
        { "0.0.1", "0.0.1 ", 1, "a trailing space is not read" },
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        ADF_CHECK_MSG(adf_test_version_major_minor_equal(cases[i].a, cases[i].b) == cases[i].same,
                      "\"%s\" against \"%s\": the major and the minor %s (case %lu)", cases[i].a,
                      cases[i].b, cases[i].same ? "agree" : "do not agree",
                      (unsigned long) i + 1);
        /* The comparison is an equality, so it does not depend on the order of the two
           strings. */
        ADF_CHECK_MSG(adf_test_version_major_minor_equal(cases[i].b, cases[i].a) == cases[i].same,
                      "\"%s\" against \"%s\": the answer differs when the two are exchanged "
                      "(case %lu)", cases[i].b, cases[i].a, (unsigned long) i + 1);
    }
}

ADF_TEST(the_version_comparison_of_the_installed_flint_says_the_versions_agree)
{
    /* The public function goes through the same comparison, with the run-time string of
       FLINT. */
    ADF_CHECK(adf_test_version_major_minor_equal(flint_version, adf_flint_version_compiled()) == 1);
    ADF_CHECK(adf_test_version_major_minor_equal(adf_flint_version_compiled(), flint_version) == 1);
}
