#include <adelefeld.h>
#include <stdio.h>

int main(void)
{
    fmpz_t big;
    adf_place_t place;
    ulong word;
    int status, is_prime;

    fmpz_init(big);
    fmpz_one(big);
    fmpz_mul_2exp(big, big, 64);
    fmpz_add_ui(big, big, 13);
    is_prime = fmpz_is_prime(big);
    word = fmpz_get_ui(big);
    status = adf_place_prime(&place, word);
    printf("prime_bits=%lu flint_is_prime=%d ulong_bits=%zu converted_word=%lu constructor_status=%d\n",
           fmpz_bits(big), is_prime, 8 * sizeof(ulong), word, status);
    fmpz_clear(big);
    return is_prime == 1 && word == 13 && status == ADF_OK &&
           adf_place_prime_get(place) == 13 ? 0 : 1;
}
