/* tests/test_status.c: the status codes and their names (conventions 3.1, 11.1). */

#include <adelefeld.h>
#include <string.h>

#include "test_runner.h"

ADF_TEST(the_numeric_values_of_the_status_codes)
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

ADF_TEST(the_values_of_the_point_comparison)
{
    /* conventions 2.1 and the closure finding C1: a comparison, not a status. */
    ADF_CHECK(ADF_CMP_EQUAL == 0);
    ADF_CHECK(ADF_CMP_DIFFERENT == 1);
    ADF_CHECK(ADF_CMP_UNDECIDED == 2);
}

ADF_TEST(every_status_code_has_the_name_of_the_list)
{
    ADF_CHECK(strcmp(adf_status_str(ADF_OK), "OK") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_NOT_DETERMINED), "NOT_DETERMINED") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_UNIT_NOT_CERTIFIED), "UNIT_NOT_CERTIFIED") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_NEEDS_SPLIT), "NEEDS_SPLIT") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_NOT_UNIQUE), "NOT_UNIQUE") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_NO_SOLUTION), "NO_SOLUTION") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_NOT_UNIT), "NOT_UNIT") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_DOMAIN), "DOMAIN") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_UNSUPPORTED), "UNSUPPORTED") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_PARSE), "PARSE") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_LIMIT), "LIMIT") == 0);
}

ADF_TEST(the_names_carry_no_prefix)
{
    int s;

    for (s = 0; s < ADF_STATUS_COUNT; s++)
    {
        const char *name = adf_status_str(s);

        ADF_CHECK(name != NULL);
        ADF_CHECK(strncmp(name, "ADF_", 4) != 0);
        ADF_CHECK(strlen(name) > 0);
    }
}

ADF_TEST(the_names_are_distinct)
{
    int i, j;

    for (i = 0; i < ADF_STATUS_COUNT; i++)
        for (j = i + 1; j < ADF_STATUS_COUNT; j++)
            ADF_CHECK_MSG(strcmp(adf_status_str(i), adf_status_str(j)) != 0,
                          "the codes %d and %d have the same name \"%s\"", i, j,
                          adf_status_str(i));
}

ADF_TEST(a_code_outside_the_list_has_the_name_UNKNOWN)
{
    ADF_CHECK(strcmp(adf_status_str(-1), "UNKNOWN") == 0);
    ADF_CHECK(strcmp(adf_status_str(ADF_STATUS_COUNT), "UNKNOWN") == 0);
    ADF_CHECK(strcmp(adf_status_str(11), "UNKNOWN") == 0);
    ADF_CHECK(strcmp(adf_status_str(1000), "UNKNOWN") == 0);
    ADF_CHECK(strcmp(adf_status_str(-2147483647 - 1), "UNKNOWN") == 0);
    ADF_CHECK(strcmp(adf_status_str(2147483647), "UNKNOWN") == 0);
}

ADF_TEST(the_name_of_a_code_is_stable)
{
    /* The string is static and must not be freed; two calls give the same pointer. */
    ADF_CHECK(adf_status_str(ADF_LIMIT) == adf_status_str(ADF_LIMIT));
    ADF_CHECK(strcmp(adf_status_str(ADF_LIMIT), "LIMIT") == 0);
}
