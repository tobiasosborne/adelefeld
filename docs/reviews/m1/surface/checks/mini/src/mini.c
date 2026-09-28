/* mini.c: three functions for the reproducers of the mutation tool (review surface). */
#include <stddef.h>
#include "mini.h"

ptrdiff_t
mini_dist(const char * p, const char * q)
{
    return q - p;
}

int
mini_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    return -1;
}

unsigned long long
mini_count_down(unsigned long long start)
{
    volatile unsigned long long n = start;
    while (n != 0)
        n = n - 1;
    return n;
}
