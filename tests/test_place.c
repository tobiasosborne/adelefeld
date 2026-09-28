/* tests/test_place.c: the opaque place handle (conventions 7, DECISION CV-18 and CV-56; SPEC 4.1).
   The primes tested: 2, 3, 5, 7, 11, 13, 101, 65537, and the largest prime below 2^64,
   18446744073709551557 = 2^64 - 59.  The rejected arguments: 0, 1 and the composites 4, 6, 9,
   100, 10, 15, 49, 121, 1009^2, the product of the first six primes, 2^63, 2^64 - 1 and
   2^64 - 2.  The last two are the two largest values a 64-bit ulong admits, so 2^64 itself is
   not among them: it does not fit in the type (conventions 12.4: one ulong, 8 bytes). */

#include <adelefeld.h>

#include "test_runner.h"

static const ulong primes[] = {2, 3, 5, 7, 11, 13, 101, 65537, 18446744073709551557UL};
static const ulong not_primes[] = {0,      1,      4,      6,      9,      100,
                                   10,     15,     49,     121,    1009 * 1009, 2 * 3 * 5 * 7 * 11 * 13,
                                   9223372036854775808UL,       /* 2^63 */
                                   18446744073709551615UL,      /* 2^64 - 1 */
                                   18446744073709551614UL};     /* 2^64 - 2 */
static const int n_primes = (int) (sizeof(primes) / sizeof(primes[0]));
static const int n_not_primes = (int) (sizeof(not_primes) / sizeof(not_primes[0]));

ADF_TEST(the_place_has_the_fixed_layout_of_the_interface)
{
    /* conventions 12.5 and 12.4: one ulong, 8 bytes, alignment 8. */
    ADF_CHECK(adf_sizeof_place() == 8);
    ADF_CHECK(adf_alignof_place() == 8);
    ADF_CHECK(sizeof(adf_place_t) == 8);
}

ADF_TEST(the_archimedean_place_is_not_a_prime)
{
    adf_place_t v = adf_place_inf();

    ADF_CHECK(adf_place_is_archimedean(v) == 1);
    ADF_CHECK(adf_place_prime_get(v) == 0);
}

ADF_TEST(a_place_made_twice_from_the_same_prime_is_one_place)
{
    int i;

    for (i = 0; i < n_primes; i++)
    {
        adf_place_t v, w;
        int s1, s2;

        s1 = adf_place_prime(&v, primes[i]);
        s2 = adf_place_prime(&w, primes[i]);
        ADF_CHECK(s1 == ADF_OK);
        ADF_CHECK(s2 == ADF_OK);
        ADF_CHECK(adf_place_equal(v, w) == 1);
        ADF_CHECK(adf_place_cmp(v, w) == 0);
        ADF_CHECK(adf_place_cmp(w, v) == 0);
        ADF_CHECK(adf_place_prime_get(v) == primes[i]);
    }
}

ADF_TEST(every_prime_gives_a_finite_place)
{
    int i;

    for (i = 0; i < n_primes; i++)
    {
        adf_place_t v;

        ADF_CHECK(adf_place_prime(&v, primes[i]) == ADF_OK);
        ADF_CHECK(adf_place_is_archimedean(v) == 0);
        ADF_CHECK(adf_place_prime_get(v) == primes[i]);
        ADF_CHECK(adf_place_equal(v, adf_place_inf()) == 0);
    }
}

ADF_TEST(the_largest_prime_below_two_to_the_sixty_four_is_a_place)
{
    adf_place_t v;

    ADF_CHECK(adf_place_prime(&v, 18446744073709551557UL) == ADF_OK);
    ADF_CHECK(adf_place_prime_get(v) == 18446744073709551557UL);
    ADF_CHECK(adf_place_is_archimedean(v) == 0);
}

ADF_TEST(a_composite_prime_argument_is_outside_the_domain)
{
    int i;

    for (i = 0; i < n_not_primes; i++)
    {
        adf_place_t v;

        /* An uninitialised-looking value: the constructor must not write it. */
        v = adf_place_inf();
        ADF_CHECK_MSG(adf_place_prime(&v, not_primes[i]) == ADF_DOMAIN,
                      "%lu is not prime and must give ADF_DOMAIN",
                      (unsigned long) not_primes[i]);
        ADF_CHECK(adf_place_is_archimedean(v) == 1);
        ADF_CHECK(adf_place_prime_get(v) == 0);
    }
}

ADF_TEST(the_output_is_untouched_when_the_argument_is_not_prime)
{
    adf_place_t v, w, before;

    ADF_CHECK(adf_place_prime(&v, 7) == ADF_OK);
    before = v;
    w = adf_place_inf();
    ADF_CHECK(adf_place_prime(&w, 8) == ADF_DOMAIN);
    ADF_CHECK_MSG(adf_place_equal(w, before) == 0,
                  "the output of a rejected adf_place_prime must not become the input prime");
    ADF_CHECK(adf_place_is_archimedean(w) == 1);

    /* The rejected call must leave the caller's own place as it was, too. */
    ADF_CHECK(adf_place_prime(&v, 9) == ADF_DOMAIN);
    ADF_CHECK(adf_place_equal(v, before) == 1);
    ADF_CHECK(adf_place_prime_get(v) == 7);
}

ADF_TEST(the_archimedean_place_comes_before_every_prime)
{
    adf_place_t inf = adf_place_inf();
    int i;

    for (i = 0; i < n_primes; i++)
    {
        adf_place_t p;

        ADF_CHECK(adf_place_prime(&p, primes[i]) == ADF_OK);
        ADF_CHECK(adf_place_cmp(inf, p) == -1);
        ADF_CHECK(adf_place_cmp(p, inf) == 1);
        ADF_CHECK(adf_place_equal(inf, p) == 0);
    }
    ADF_CHECK(adf_place_cmp(inf, inf) == 0);
    ADF_CHECK(adf_place_equal(inf, inf) == 1);
}

ADF_TEST(the_canonical_order_is_the_increasing_order_of_the_primes)
{
    int i, j;

    for (i = 0; i < n_primes; i++)
        for (j = 0; j < n_primes; j++)
        {
            adf_place_t v, w;

            ADF_CHECK(adf_place_prime(&v, primes[i]) == ADF_OK);
            ADF_CHECK(adf_place_prime(&w, primes[j]) == ADF_OK);
            ADF_CHECK(adf_place_cmp(v, w) == (i < j ? -1 : (i > j ? 1 : 0)));
            ADF_CHECK_MSG(adf_place_equal(v, w) == (i == j),
                          "the places of %lu and %lu are %s", (unsigned long) primes[i],
                          (unsigned long) primes[j], primes[i] == primes[j] ? "one" : "two");
            ADF_CHECK((adf_place_cmp(v, w) == 0) == (adf_place_equal(v, w) == 1));
        }
}

ADF_TEST(the_order_is_a_strict_total_order)
{
    /* Transitivity over a fixed list, which the canonical order of conventions 7 must give. */
    adf_place_t list[8];
    int i, j, k;
    const ulong ps[7] = {2, 3, 5, 7, 11, 13, 101};
    int n = 0;

    list[n++] = adf_place_inf();
    for (i = 0; i < 7; i++)
    {
        ADF_CHECK(adf_place_prime(&list[n], ps[i]) == ADF_OK);
        n++;
    }
    /* The list is the archimedean place followed by the primes in increasing order, so it is
       sorted, and every triple in it must be in the same order. */
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            ADF_CHECK(adf_place_cmp(list[i], list[j]) == (i < j ? -1 : (i > j ? 1 : 0)));
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            for (k = 0; k < n; k++)
                if (adf_place_cmp(list[i], list[j]) < 0 && adf_place_cmp(list[j], list[k]) < 0)
                    ADF_CHECK_MSG(adf_place_cmp(list[i], list[k]) < 0,
                                  "the order of places is not transitive at %d, %d, %d", i, j, k);
    for (i = 0; i < n; i++)
        ADF_CHECK(adf_place_cmp(list[i], list[i]) == 0);
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            ADF_CHECK(adf_place_cmp(list[i], list[j]) == -adf_place_cmp(list[j], list[i]));
}

ADF_TEST(a_place_survives_by_value)
{
    /* A place is a plain struct passed by value (conventions 7, 12.5). */
    adf_place_t v;
    adf_place_t copy;

    ADF_CHECK(adf_place_prime(&v, 13) == ADF_OK);
    copy = v;
    ADF_CHECK(adf_place_equal(copy, v) == 1);
    ADF_CHECK(adf_place_prime_get(copy) == 13);
    ADF_CHECK(adf_place_is_archimedean(copy) == 0);
    ADF_CHECK(adf_place_cmp(copy, v) == 0);
}
