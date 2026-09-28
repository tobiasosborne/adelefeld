/* adelefeld/cap.c: the absolute cap on adf_fball, global backend (work package 1.7, part).

   Ground truth: docs/proofs/policies.md section 3, Definition 13 (line 253) to Proposition 15
   (line 283); docs/SPEC.md 4.4 item 3; docs/conventions.md 5.4, paragraph "The absolute cap"
   (line 520), DECISION CV-23 (the cap C is an argument, not a field) and CV-47 (an exact
   result keeps its exact tag; the cap never touches it); docs/api-m1.md rows for the five
   functions (adf_fball_cap and friends) and "Choices" item 5 (ADF_DOMAIN when C <= 0, a status
   row that conventions 3.2 does not give).

   Definition 13 (policies.md:253): a cap is a rational C > 0. After a tight operation with
   result c + R Zhat, R > 0, the radius is replaced by gcd(R, C). Proposition 14
   (policies.md:258): this is sound (item 1: the old set is inside the new one) and, for R = 0
   (an exact result), the cap is not applied (item 2, required by SPEC.md 4.1: an exact value
   "stays exact under arithmetic with other exact values"; the literal formula gcd(0, C) = C
   would widen it into a ball). Item 3: gcd(R, C) is the finest radius dividing both R and C,
   the best enclosure among radii that divide C.

   adf_fball_cap applies Definition 13 directly to the set of its input, whatever that set's
   history. The other four functions run the tight operation of fball.h (or adf_fball_mul_rat)
   into a private temporary and then cap that temporary into the output; this is exactly
   Definition 13 ("after a tight operation") and keeps every permitted aliasing of the output
   with an input safe, since the temporary is read out of x and y before the output is written.

   Local-backend inputs (R1 of docs/reviews/m1/local/review.md, replacing a former
   HEADER-FINDING here that refused every local input with ADF_UNSUPPORTED): neither this
   header's comment nor adelefeld/scaled.h's cap block (line 160 to 178) restricts the backend
   of x or y, and adelefeld/fball.h says, of every function it declares, "Every function of
   this header is defined on sets and accepts inputs of either backend" (fball.h, "Backends",
   line 16). adf_fball_add, adf_fball_sub, adf_fball_mul and adf_fball_mul_rat already implement
   that contract for a local input, local pair, or local/global mix (src/fball.c,
   src/fball_local.c, lane m1-fball; conventions 5.3, the operations table and "results that
   leave the context ... are global"); so does adf_fball_get_center, which this file already
   used (fball.c: "The stored centre of the canonical triple ..., for either backend"). This
   file therefore needs no backend check of its own: it reads x->H and x->d directly for the
   radius (valid for either backend: fball.h, "the set (A + H Zhat)/d"; for a local value H = K,
   d the raw denominator, and H/d is the radius of the set whatever A and the residues are,
   adf_fball_get_radius's comment in fball.c makes the same point), and calls the functions
   above, which already dispatch on backend themselves.

   Backend of the result: the cap replaces the radius R > 0 by gcd(R, C) (Definition 13), a
   number with no necessary relation to any context's modulus; no function here is given a
   context to convert into (conventions 4.6: "No function here creates a context"), and the
   caller passed no local backend to keep the value in if it wanted one (contrast
   adf_fball_set_local, which needs a context argument). The simplest correct reading, and the
   one the existing computation already gives without change, is therefore a global result
   whenever the cap acts: adf_fball_set_center_radius, used by adf_fball_cap below, "computed as
   in ... conventions 5.2" (fball.h line 132) and its implementation always writes the global
   canonical triple (fb_store, src/fball.c), local input included. An exact result (H = 0) is
   returned via adf_fball_set, unchanged, so it also keeps its own backend (Proposition 14.2,
   CV-47: the cap must not touch it) -- but Definition 16 (policies.md line 305, "Exact values
   (H = 0) are never local values") makes this branch global-only in practice, since a local
   input always has H = K >= 2 (adf_modctx_new_blocks requires q_i >= 2, k >= 1 for predicate
   L). tests/test_cap_local.c checks both halves: every local input taken to any of the five
   functions gives a result equal, as a set, to the same function applied to its global form
   (adf_fball_set_global), and a local source that yields an exact result (mul_rat_cap, q = 0)
   is untouched by the cap.

   Aliasing (adelefeld/fball.h "Common rules"; the cap block of scaled.h: "Aliasing as in
   adelefeld/fball.h"): an output may be the same object as any input of the same type. */

#include "adelefeld/scaled.h"

#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>

/* -------------------------------------------------------------- adf_fball_cap */

/* adf_fball_cap(y, x, C): y = x with its radius capped (Definition 13, policies.md:253); x of
   either backend (file header above). C <= 0: ADF_DOMAIN, y untouched (docs/api-m1.md Choices
   item 5). Otherwise: x exact (H = 0; global only, Definition 16): y = x unchanged, the exact
   tag kept (Proposition 14.2, CV-47); else y = the centre of x with radius gcd(radius(x), C)
   (Proposition 14.1, 14.3), through adf_fball_set_center_radius, always global (file header
   above). Every field of x needed is read into a local temporary before y is written, so y may
   alias x. */
int
adf_fball_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t C)
{
    fmpq_t Cq, Nq, gq;
    adf_rat_t c, g;
    int status;

    if (fmpq_sgn(C->q) <= 0)
        return ADF_DOMAIN;

    if (fmpz_is_zero(x->H))
    {
        adf_fball_set(y, x);
        return ADF_OK;
    }

    fmpq_init(Cq);
    fmpq_init(Nq);
    fmpq_init(gq);
    fmpq_set(Cq, C->q);
    fmpq_set_fmpz_frac(Nq, x->H, x->d);
    fmpq_gcd(gq, Nq, Cq);

    adf_rat_init(c);
    adf_rat_init(g);
    adf_fball_get_center(c, x);
    fmpq_set(g->q, gq);
    status = adf_fball_set_center_radius(y, c, g);
    adf_rat_clear(c);
    adf_rat_clear(g);
    fmpq_clear(Cq);
    fmpq_clear(Nq);
    fmpq_clear(gq);
    return status;
}

/* ---------------------------------------------------------- the other four */

/* adf_fball_add_cap(z, x, y, C): the tight sum of fball.h (docs/proofs/precision.md
   Proposition 1), then the cap (Definition 13); x and y of either backend, independently
   (file header above). The sum is computed into a private temporary t before anything is
   written to z, so z may alias x, y, or both (adelefeld/fball.h "Aliasing"; adf_fball_add
   itself already tolerates that aliasing, but capping z in place while still reading x or y
   from it would not). */
int
adf_fball_add_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C)
{
    adf_fball_t t;
    int status;

    adf_fball_init(t);
    adf_fball_add(t, x, y);
    status = adf_fball_cap(z, t, C);
    adf_fball_clear(t);
    return status;
}

/* adf_fball_sub_cap(z, x, y, C): the tight difference of fball.h, then the cap, exactly as
   adf_fball_add_cap. */
int
adf_fball_sub_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C)
{
    adf_fball_t t;
    int status;

    adf_fball_init(t);
    adf_fball_sub(t, x, y);
    status = adf_fball_cap(z, t, C);
    adf_fball_clear(t);
    return status;
}

/* adf_fball_mul_cap(z, x, y, C): the tight product of fball.h (docs/proofs/precision.md
   Proposition 2), then the cap, exactly as adf_fball_add_cap. */
int
adf_fball_mul_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C)
{
    adf_fball_t t;
    int status;

    adf_fball_init(t);
    adf_fball_mul(t, x, y);
    status = adf_fball_cap(z, t, C);
    adf_fball_clear(t);
    return status;
}

/* adf_fball_mul_rat_cap(y, x, q, C): adf_fball_mul_rat of fball.h into a private temporary,
   then the cap (the cap acts on the product of an exact scalar with a ball, scaled.h comment
   above adf_fball_cap; Proposition 14.2). x of either backend (file header above); q is a
   plain rational and has no backend. */
int
adf_fball_mul_rat_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t q, const adf_rat_t C)
{
    adf_fball_t t;
    int status;

    adf_fball_init(t);
    adf_fball_mul_rat(t, x, q);
    status = adf_fball_cap(y, t, C);
    adf_fball_clear(t);
    return status;
}
