/* place.c: the opaque place handle (conventions 7, DECISION CV-18 of version 0.1 and CV-56;
   docs/seams.md section 5, R1; SPEC 4.1; conventions 3.1 for the statuses and 4.3 for the state
   of the output after a status; conventions 12.5 for the layout of the handle).

   Version 1 has the archimedean place of Q and the primes below 2^64 (conventions 5.8, part of
   DECISION CV-18, D8: "Primes are one word in version 1 ... Larger primes: ADF_UNSUPPORTED").
   A prime at or above 2^64 cannot be passed at all, because the argument is a `ulong`.

   The encoding. The handle is a struct of one `ulong` whose field is private: "the integer
   inside is not part of the contract, and no arithmetic or comparison on it is offered"
   (conventions 7). The two constructors below write 0 for the archimedean place and the prime
   p itself for a finite place, which no arithmetic needs and no reader may rely on; every
   question a caller may ask is answered by the functions of this file. With this encoding a
   handle built by the two constructors is never both archimedean and finite, because
   adf_place_prime refuses every p < 2 (n_is_prime(0) = n_is_prime(1) = 0), and a place made
   twice from the same prime is the same handle, so adf_place_equal answers the question
   "same place" without a table.

   The canonical order of conventions 7, "the archimedean place first, then the primes in
   increasing order", is then the order of the stored words, with 0 first; adf_place_cmp
   implements it without a case distinction on validity, and is a total order on every word.

   Primality is certified, not probable: n_is_prime (/usr/include/flint/ulong_extras.h:335) is
   documented as "Tests if n is a prime. This first sieves for small prime factors, then simply
   calls n_is_probabprime. This has been checked against the tables of Feitsma and Galway"
   (refs/src/flint-3.0.1/ulong_extras.rst:833-836). The return value is therefore a decision and
   not a guess: 1 is proved prime and 0 is proved composite, which is what conventions 5.8 asks
   for ("primality is then certified by n_is_prime"). */

#include <adelefeld.h>
#include <flint/ulong_extras.h>   /* n_is_prime, the primality test of conventions 5.8 */

/* The word of the archimedean place. Private to this file; the field itself is private. */

#define ADF_PLACE_INF_WORD ((ulong) 0)

/* adf_place_inf(): the archimedean place of Q (conventions 7). Cost: constant. */

adf_place_t
adf_place_inf(void)
{
    adf_place_t v;

    v.opaque = ADF_PLACE_INF_WORD;
    return v;
}

/* adf_place_prime(v, p): the place of the prime p.
   Status: ADF_OK, *v written; ADF_DOMAIN when p is not prime, *v untouched (conventions 3.1,
   "a constructor ... received data that violates the type's invariant (zero denominator,
   composite prime, non-finite real ball)", and 4.3, "on every status other than ADF_OK every
   output is left untouched"). The check is made before the write, so that *v is untouched also
   when it is one of nothing else: the argument is a value, not an address of an input.
   Aliasing: v is an output only. Cost: one n_is_prime on a word. */

int
adf_place_prime(adf_place_t * v, ulong p)
{
    if (!n_is_prime(p))
        return ADF_DOMAIN;
    v->opaque = p;
    return ADF_OK;
}

/* adf_place_is_archimedean(v): 1 for the archimedean place, else 0 (conventions 7). */

int
adf_place_is_archimedean(adf_place_t v)
{
    return v.opaque == ADF_PLACE_INF_WORD;
}

/* adf_place_prime_get(v): the prime of a finite place, 0 for the archimedean place
   (conventions 7). */

ulong
adf_place_prime_get(adf_place_t v)
{
    return v.opaque;
}

/* adf_place_cmp(v, w): -1, 0 or 1 in the canonical order of places (conventions 7): the
   archimedean place first, then the primes in increasing order. The word of the archimedean
   place is 0 and the word of a finite place is a prime, that is at least 2, so the numeric order
   of the words is exactly the canonical order of places and the comparison needs no case. */

int
adf_place_cmp(adf_place_t v, adf_place_t w)
{
    return (v.opaque > w.opaque) - (v.opaque < w.opaque);
}

/* adf_place_equal(v, w): 1 if v and w are the same place, else 0 (conventions 7). One place has
   one handle, so this is the comparison being 0 and needs no table. */

int
adf_place_equal(adf_place_t v, adf_place_t w)
{
    return adf_place_cmp(v, w) == 0;
}
