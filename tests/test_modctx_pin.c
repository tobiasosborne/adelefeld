/* tests/test_modctx_pin.c: the bound of adf_modctx_recombine, [0, K), pinned against an
   independent oracle.

   The gap this file closes. tests/test_modctx_limits.c:353 (recombine_is_the_representative_in_range)
   pins the same bound, but it takes K from adf_modctx_get_modulus, the value of the library, and
   the closure judge of the contexts review (lanes/m1-closure-contexts/report.md:133-134) found
   that this pin is green on this FLINT even when the reduction at src/modctx.c:603 is removed: on
   FLINT 3.0.1 fmpz_multi_CRT_precomp(out, ..., 0) already returns a value in [0, K) (the review
   ran 4400 recombinations, docs/reviews/m1/contexts/review.md, R6). The pin therefore does not
   prove the line of src/modctx.c that carries the bound.

   What this file does differently:
   1. K is computed by the test itself, as the product of the blocks, and not read from the
      library. A bound that is moved inside the library is then seen against a number the library
      did not produce;
   2. the value of the representative is compared with an oracle of the test, which walks [0, K)
      and keeps the one integer with the given residues. The oracle is the definition of
      docs/proofs/policies.md Lemma 17.1 (the unique integer of [0, K) with those residues), not
      the implementation: the test never calls fmpz_multi_CRT_precomp and never uses the
      reduction that the library uses;
   3. the two ends of the range are pinned by value: the residues 0 give 0, and the residues
      q[i] - 1 give K - 1. A bound moved by one at either end is caught here and nowhere else;
   4. the round trip through adf_modctx_reduce supplies the residues of the negative inputs, so
      the pin is exercised where the balanced representative would be -1.

   The promise under test: src/modctx_internal.h, "adf_modctx_recombine(out, ctx, res): the unique
   integer in [0, K) that is res[i] mod q_i for every i", with K = the product of the blocks and
   the residues reduced by adf_modctx_reduce. The line of the library that carries the bound is
   src/modctx.c:603, `fmpz_fdiv_r(out, out, ctx->K)`, whose own range is [0, K) as well
   [source pending: FLINT 3.0.1 documentation of fmpz_fdiv_r under refs/].

   The hidden kernels are declared here by hand, as tests/test_modctx.c:29-30 and
   tests/test_modctx_limits.c:42-43 do. */

#include <stdio.h>

#include <adelefeld/modctx.h>

#include "test_runner.h"

/* Hidden internal kernels of src/modctx.c (docs/conventions.md 5.14, policies.md Lemma 17). */
void adf_modctx_reduce(const adf_modctx_struct * ctx, const fmpz_t a, ulong * res);
void adf_modctx_recombine(fmpz_t out, const adf_modctx_struct * ctx, const ulong * res);

/* A decimal string of an fmpz, for the message of a check. The runner formats the message only
   when a check fails, and every value printed here has at most 33 decimal digits, so eight
   buffers of 64 bytes are enough and no memory is allocated. The call is not thread-safe; no
   test of this file uses a thread. */
static const char *
dec(const fmpz_t v)
{
    static char buf[8][64];
    static int slot = 0;
    char *b = buf[slot];

    slot = (slot + 1) % 8;
    fmpz_get_str(b, 10, v);
    return b;
}

/* The modulus of the test: the product of the blocks, computed here and not read from the
   library. The blocks are pairwise coprime primes below 2^64, so the product is an exact fmpz
   and no step of the oracle needs a division that could fail. K must be initialised by the
   caller: the function writes it and does not initialise it, which is what the memory checker
   of tools/memcheck expects of a helper whose parameter is not initialised in its own body. */
static void
modulus_of_blocks(fmpz_t K, const ulong * q, slong k)
{
    slong i;

    fmpz_set_ui(K, 1);
    for (i = 0; i < k; i++)
        fmpz_mul_ui(K, K, q[i]);
}

/* The oracle: the unique x with 0 <= x < K and x = res[i] mod q[i] for every i. It walks the
   range of the promise and stops at the first x that has all the residues. Sets *found to 0
   when no x of [0, K) has them, which must not happen for pairwise coprime blocks. */
static void
representative_oracle(fmpz_t want, int * found, const fmpz_t K, const ulong * q, const ulong * res,
                      slong k)
{
    fmpz_t x;
    slong i;
    int ok;

    *found = 0;
    fmpz_init(x);
    fmpz_zero(x);
    while (fmpz_cmp(x, K) < 0)
    {
        ok = 1;
        for (i = 0; i < k; i++)
            if (fmpz_fdiv_ui(x, q[i]) != res[i])
            {
                ok = 0;
                break;
            }
        if (ok)
        {
            fmpz_set(want, x);
            *found = 1;
            break;
        }
        fmpz_add_ui(x, x, 1);
    }
    fmpz_clear(x);
}

/* ---- 1. every residue of a small context, against the oracle ---- */

ADF_TEST(every_residue_of_K_35_is_the_representative_in_0_K)
{
    /* K = 5 * 7 = 35, so the range has 35 integers and every one of them is walked. */
    static const ulong q[2] = {5, 7};
    adf_modctx_struct * ctx = NULL;
    fmpz_t K, out, want;
    ulong res[2];
    int found;
    long cases = 0;
    ulong r0, r1;

    fmpz_init(K);
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 2) == ADF_OK);
    modulus_of_blocks(K, q, 2);
    fmpz_init(out);
    fmpz_init(want);

    for (r0 = 0; r0 < 5; r0++)
        for (r1 = 0; r1 < 7; r1++)
        {
            res[0] = r0;
            res[1] = r1;
            adf_modctx_recombine(out, ctx, res);
            ADF_CHECK_MSG(fmpz_sgn(out) >= 0 && fmpz_cmp(out, K) < 0,
                          "out = %s is not in [0, 35) for the residues (%lu, %lu)", dec(out), r0,
                          r1);
            representative_oracle(want, &found, K, q, res, 2);
            ADF_CHECK_MSG(found, "no x in [0, 35) has the residues (%lu, %lu)", r0, r1);
            ADF_CHECK_MSG(fmpz_equal(out, want), "out = %s, the oracle says %s, for the residues "
                                                 "(%lu, %lu)", dec(out), dec(want), r0, r1);
            cases++;
        }
    ADF_CHECK_MSG(cases == 35, "%ld cases, not 35", cases);
    fmpz_clear(out);
    fmpz_clear(want);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

ADF_TEST(every_residue_of_K_105_is_the_representative_in_0_K)
{
    /* K = 3 * 5 * 7 = 105: three blocks, so that a representative that is right for two of them
       and wrong for the third is caught. */
    static const ulong q[3] = {3, 5, 7};
    adf_modctx_struct * ctx = NULL;
    fmpz_t K, out, want;
    ulong res[3];
    int found;
    long cases = 0;
    ulong r0, r1, r2;

    fmpz_init(K);
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
    modulus_of_blocks(K, q, 3);
    fmpz_init(out);
    fmpz_init(want);

    for (r0 = 0; r0 < 3; r0++)
        for (r1 = 0; r1 < 5; r1++)
            for (r2 = 0; r2 < 7; r2++)
            {
                res[0] = r0;
                res[1] = r1;
                res[2] = r2;
                adf_modctx_recombine(out, ctx, res);
                ADF_CHECK_MSG(fmpz_sgn(out) >= 0 && fmpz_cmp(out, K) < 0,
                              "out = %s is not in [0, 105) for (%lu, %lu, %lu)", dec(out), r0, r1,
                              r2);
                representative_oracle(want, &found, K, q, res, 3);
                ADF_CHECK_MSG(found, "no x in [0, 105) has the residues (%lu, %lu, %lu)", r0, r1,
                              r2);
                ADF_CHECK_MSG(fmpz_equal(out, want), "out = %s, the oracle says %s, for (%lu, %lu, "
                                                     "%lu)", dec(out), dec(want), r0, r1, r2);
                cases++;
            }
    ADF_CHECK_MSG(cases == 105, "%ld cases, not 105", cases);
    fmpz_clear(out);
    fmpz_clear(want);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

ADF_TEST(every_residue_of_K_2310_is_the_representative_in_0_K)
{
    /* K = 2 * 3 * 5 * 7 * 11 = 2310: five blocks, and a modulus that no single word holds. The
       residues are those of x itself for every x of the range, so every integer of [0, K) is the
       representative of exactly one residue tuple, and the identity says which value it is. */
    static const ulong q[5] = {2, 3, 5, 7, 11};
    adf_modctx_struct * ctx = NULL;
    fmpz_t K, out, want;
    ulong res[5];
    int found;
    long cases = 0, compared = 0;
    ulong v;

    fmpz_init(K);
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 5) == ADF_OK);
    modulus_of_blocks(K, q, 5);
    fmpz_init(out);
    fmpz_init(want);
    ADF_CHECK(fmpz_equal_ui(K, 2310));

    for (v = 0; v < 2310; v++)
    {
        slong i;

        for (i = 0; i < 5; i++)
            res[i] = v % q[i];
        adf_modctx_recombine(out, ctx, res);
        ADF_CHECK_MSG(fmpz_sgn(out) >= 0 && fmpz_cmp(out, K) < 0,
                      "out = %s is not in [0, 2310) for the residues of %lu", dec(out), v);
        /* The identity: the representative of the residues of x is x itself. */
        ADF_CHECK_MSG(fmpz_equal_ui(out, v), "the residues of %lu gave %s", v, dec(out));
        /* And the oracle of the definition, for every tenth case, which is a walk of the whole
           range. */
        if (v % 10 == 0)
        {
            representative_oracle(want, &found, K, q, res, 5);
            ADF_CHECK_MSG(found, "no x in [0, 2310) has the residues of %lu", v);
            ADF_CHECK_MSG(fmpz_equal(out, want), "out = %s, the oracle says %s, for the residues of "
                                                 "%lu", dec(out), dec(want), v);
            compared++;
        }
        cases++;
    }
    ADF_CHECK_MSG(cases == 2310, "%ld cases, not 2310", cases);
    ADF_CHECK_MSG(compared == 231, "%ld oracle cases, not 231 (0, 10, ..., 2300)", compared);
    fmpz_clear(out);
    fmpz_clear(want);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 2. the two ends of the range, by value ---- */

ADF_TEST(the_lower_end_of_the_range_is_0)
{
    /* The residues 0 must give 0 and not K, not K - 1 and not a negative number: the bound is
       closed at 0. */
    static const ulong sets[4][2] = { {2, 3}, {3, 5}, {5, 7}, {65537, 65539} };
    fmpz_t K, out;
    size_t s;

    fmpz_init(K);
    fmpz_init(out);
    for (s = 0; s < 4; s++)
    {
        adf_modctx_struct * ctx = NULL;
        ulong res[2];

        ADF_CHECK(adf_modctx_new_blocks(&ctx, sets[s], 2) == ADF_OK);
        modulus_of_blocks(K, sets[s], 2);
        res[0] = 0;
        res[1] = 0;
        adf_modctx_recombine(out, ctx, res);
        ADF_CHECK_MSG(fmpz_equal_ui(out, 0), "the residues (0, 0) of K = %s gave %s, not 0", dec(K),
                      dec(out));
        ADF_CHECK(fmpz_sgn(out) == 0);
        adf_modctx_free(ctx);
    }
    fmpz_clear(K);
    fmpz_clear(out);
}

ADF_TEST(the_upper_end_of_the_range_is_K_minus_1)
{
    /* The residues q[i] - 1 must give K - 1 and nothing else: the bound is open at K, so a value
       of K or more is out of range. These are the residues whose balanced representative is -1,
       which is the case tests/test_modctx_limits.c:396-414 uses. */
    static const ulong sets[5][2] = { {2, 3}, {5, 7}, {3, 5}, {65537, 65539}, {2, 65537} };
    fmpz_t K, out, top;
    size_t s;

    fmpz_init(K);
    fmpz_init(out);
    fmpz_init(top);
    for (s = 0; s < 5; s++)
    {
        adf_modctx_struct * ctx = NULL;
        ulong res[2];

        ADF_CHECK(adf_modctx_new_blocks(&ctx, sets[s], 2) == ADF_OK);
        modulus_of_blocks(K, sets[s], 2);
        res[0] = sets[s][0] - 1;
        res[1] = sets[s][1] - 1;
        adf_modctx_recombine(out, ctx, res);
        fmpz_sub_ui(top, K, 1);
        ADF_CHECK_MSG(fmpz_equal(out, top), "the residues (%lu, %lu) of K = %s gave %s, not K - 1 = "
                                           "%s", sets[s][0] - 1, sets[s][1] - 1, dec(K), dec(out),
                      dec(top));
        ADF_CHECK_MSG(fmpz_cmp(out, K) < 0, "out = %s is not below K = %s", dec(out), dec(K));
        ADF_CHECK(fmpz_equal_ui(out, 0) == 0);
        adf_modctx_free(ctx);
    }
    fmpz_clear(K);
    fmpz_clear(out);
    fmpz_clear(top);
}

/* ---- 3. one block, where the representative is the residue itself ---- */

ADF_TEST(one_block_gives_the_residue_itself)
{
    /* K = q and 0 <= res[0] < q, so the only integer of [0, K) with that residue is res[0]. The
       bound K = q is the smallest bound there is, and a bound moved by one at the top gives q,
       which this catches. */
    static const ulong primes[3] = {2, 65537, 4294967291UL};
    fmpz_t out;
    size_t s;

    fmpz_init(out);
    for (s = 0; s < 3; s++)
    {
        adf_modctx_struct * ctx = NULL;
        ulong res[1];
        ulong step = (primes[s] / 64) + 1;

        ADF_CHECK(adf_modctx_new_blocks(&ctx, &primes[s], 1) == ADF_OK);
        for (res[0] = 0; res[0] < primes[s]; res[0] += step)
        {
            adf_modctx_recombine(out, ctx, res);
            ADF_CHECK_MSG(fmpz_equal_ui(out, res[0]), "the residue %lu of K = %lu gave %s", res[0],
                          primes[s], dec(out));
        }
        /* The last residue, q - 1, is the top of the range. */
        res[0] = primes[s] - 1;
        adf_modctx_recombine(out, ctx, res);
        ADF_CHECK_MSG(fmpz_equal_ui(out, primes[s] - 1), "the residue %lu gave %s", primes[s] - 1,
                      dec(out));
        adf_modctx_free(ctx);
    }
    fmpz_clear(out);
}

/* ---- 4. the round trip through the reduction, for the negative inputs ---- */

ADF_TEST(a_negative_input_recombines_to_its_class_in_0_K)
{
    /* The class of -1 in [0, K) is K - 1 and the class of -(K - 1) is 1; both are values of
       Lemma 17.1 and neither uses the library. The residues come from adf_modctx_reduce, and the
       test states them itself: the residue of -1 modulo q is q - 1. */
    static const ulong sets[4][3] = { {5, 7, 0}, {3, 5, 7}, {2, 3, 5}, {65537, 65539, 0} };
    static const slong nblocks[4] = {2, 3, 3, 2};
    fmpz_t a, K, out;
    size_t s;

    fmpz_init(a);
    fmpz_init(K);
    fmpz_init(out);
    for (s = 0; s < 4; s++)
    {
        adf_modctx_struct * ctx = NULL;
        slong k = nblocks[s];
        slong i;
        ulong res[3];

        ADF_CHECK(adf_modctx_new_blocks(&ctx, sets[s], k) == ADF_OK);
        modulus_of_blocks(K, sets[s], k);

        fmpz_set_si(a, -1);
        adf_modctx_reduce(ctx, a, res);
        for (i = 0; i < k; i++)
            ADF_CHECK_MSG(res[i] == sets[s][i] - 1, "the residue of -1 modulo %lu is %lu, not %lu",
                          sets[s][i], res[i], sets[s][i] - 1);
        adf_modctx_recombine(out, ctx, res);
        ADF_CHECK_MSG(fmpz_sgn(out) >= 0 && fmpz_cmp(out, K) < 0, "-1 of K = %s gave %s", dec(K),
                      dec(out));
        ADF_CHECK_MSG(fmpz_cmp_ui(out, 0) != 0, "-1 of K = %s gave 0, the lower end is not the "
                                                   "class of -1 unless K = 1", dec(K));
        fmpz_sub_ui(a, K, 1);
        ADF_CHECK_MSG(fmpz_equal(out, a), "-1 of K = %s gave %s, not K - 1 = %s", dec(K), dec(out),
                      dec(a));

        /* a = -(K - 1) = 1 - K */
        fmpz_sub_ui(a, K, 1);
        fmpz_neg(a, a);
        ADF_CHECK(fmpz_sgn(a) < 0);
        adf_modctx_reduce(ctx, a, res);
        adf_modctx_recombine(out, ctx, res);
        ADF_CHECK_MSG(fmpz_sgn(out) >= 0 && fmpz_cmp(out, K) < 0, "-(K - 1) of K = %s gave %s",
                      dec(K), dec(out));
        ADF_CHECK_MSG(fmpz_equal_ui(out, 1), "-(K - 1) of K = %s gave %s, not 1", dec(K), dec(out));

        adf_modctx_free(ctx);
    }
    fmpz_clear(a);
    fmpz_clear(K);
    fmpz_clear(out);
}
