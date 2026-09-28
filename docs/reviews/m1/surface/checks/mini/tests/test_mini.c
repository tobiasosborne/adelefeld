#include "mini.h"
#include "test_runner.h"

ADF_TEST(the_distance_is_read)
{
    const char s[] = "abcdef";
    ADF_CHECK(mini_dist(s, s + 4) == 4);
}

ADF_TEST(digits_are_read)
{
    ADF_CHECK(mini_digit('0') == 0);
    ADF_CHECK(mini_digit('7') == 7);
    ADF_CHECK(mini_digit('9') == 9);
}

ADF_TEST(a_non_digit_is_a_parse_error)
{
    ADF_CHECK(mini_digit('x') == -1);
    ADF_CHECK(mini_digit('/') == -1);
    ADF_CHECK(mini_digit(':') == -1);
}

ADF_TEST(the_count_down_ends)
{
    ADF_CHECK(mini_count_down(3) == 0);
}
