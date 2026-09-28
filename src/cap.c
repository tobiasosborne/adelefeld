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

   HEADER-FINDING (this file, all five functions): scaled.h does not say what a capped
   operation does when an input ball is in the local backend (ADF_LOCAL). src/fball.c of lane
   m1-fball implements predicate G (global) only and, for a local input, leaves its own outputs
   untouched rather than writing a value it cannot certify (its own HEADER-FINDING, recorded in
   lanes/m1-fball/report.md); calling those functions on a local input would silently produce
   the untouched, possibly-stale contents of the temporary as if it were a valid global result.
   Guessing a result this file cannot certify is against lanes/COMMON-C.md rule 3 ("a function
   that cannot certify its result ... returns the status the header names; it never guesses"),
   so every function below checks the backend of every adf_fball input itself and returns
   ADF_UNSUPPORTED, with the output untouched, when one is not ADF_GLOBAL. docs/conventions.md
   3.3 (statuses combine by maximum; ADF_UNSUPPORTED = 8 > ADF_DOMAIN = 7, status.h) fixes the
   order when both a non-global input and C <= 0 hold: ADF_UNSUPPORTED wins.

   Aliasing (adelefeld/fball.h "Common rules"; the cap block of scaled.h: "Aliasing as in
   adelefeld/fball.h"): an output may be the same object as any input of the same type. */

#include "adelefeld/scaled.h"

#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>

/* -------------------------------------------------------------- adf_fball_cap */

/* adf_fball_cap(y, x, C): y = x with its radius capped (Definition 13, policies.md:253).
   C <= 0: ADF_DOMAIN, y untouched (docs/api-m1.md Choices item 5). x not ADF_GLOBAL:
   ADF_UNSUPPORTED, y untouched (HEADER-FINDING, file header above). Otherwise: x exact
   (H = 0): y = x unchanged, the exact tag kept (Proposition 14.2, CV-47); else y = the centre
   of x with radius gcd(radius(x), C) (Proposition 14.1, 14.3), through
   adf_fball_set_center_radius, which canonicalises. Every field of x needed is read into a
   local temporary before y is written, so y may alias x. */
int
adf_fball_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t C)
{
    fmpq_t Cq, Nq, gq;
    adf_rat_t c, g;
    int status;

    if (x->backend != ADF_GLOBAL)
        return ADF_UNSUPPORTED;
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
   Proposition 1), then the cap (Definition 13). The sum is computed into a private temporary
   t before anything is written to z, so z may alias x, y, or both (adelefeld/fball.h
   "Aliasing"; adf_fball_add itself already tolerates that aliasing, but capping z in place
   while still reading x or y from it would not). x or y not ADF_GLOBAL: ADF_UNSUPPORTED, z
   untouched (HEADER-FINDING, file header above); checked before the cap C <= 0 check so that
   the combined status is the maximum of the two conventions 3.3). */
int
adf_fball_add_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C)
{
    adf_fball_t t;
    int status;

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return ADF_UNSUPPORTED;

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

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return ADF_UNSUPPORTED;

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

    if (x->backend != ADF_GLOBAL || y->backend != ADF_GLOBAL)
        return ADF_UNSUPPORTED;

    adf_fball_init(t);
    adf_fball_mul(t, x, y);
    status = adf_fball_cap(z, t, C);
    adf_fball_clear(t);
    return status;
}

/* adf_fball_mul_rat_cap(y, x, q, C): adf_fball_mul_rat of fball.h into a private temporary,
   then the cap (the cap acts on the product of an exact scalar with a ball, scaled.h comment
   above adf_fball_cap; Proposition 14.2). q has no backend to check. */
int
adf_fball_mul_rat_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t q, const adf_rat_t C)
{
    adf_fball_t t;
    int status;

    if (x->backend != ADF_GLOBAL)
        return ADF_UNSUPPORTED;

    adf_fball_init(t);
    adf_fball_mul_rat(t, x, q);
    status = adf_fball_cap(y, t, C);
    adf_fball_clear(t);
    return status;
}
