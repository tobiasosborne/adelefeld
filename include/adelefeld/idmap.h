/* adelefeld/idmap.h: ideles and adeles: the map idele -> adele (the two hulls), the map adele -> idele, and the
   division of an adele by an idele (milestone 2, slice 3, lane i-slice3; docs/PLAN.md 2.3 and 2.4).

   Contract: docs/SPEC.md 4.5 ("division by an adf_adele is not defined"; one divides by an exact non-zero
   rational or by an idele), 5 ("Idele to adele": two versions, the simple ball and the smallest ball, and for an
   exact unit the exact rational r c; "Division of an adele by an idele"); docs/conventions.md 5.5, 5.7 ("Idele
   to adele"; adf_adele_div_idele, decision CV-50 = M0-D7), 3.2 (rows "Adele to idele; inversion of an
   adele-like value" and "Unit coset, idele, idele class arithmetic"), 3.3, 4.1, 4.3, 4.4. Proofs:
   docs/proofs/ideles.md Proposition 3 (line 67), Proposition 16 (line 404: the simple ball and the smallest
   ball), Proposition 17 (line 443: no finite ball of positive radius certifies invertibility), Propositions 18
   and 19 (lines 462 and 472: division by an exact rational, by an idele); docs/api-2.md 3.3, Statements M (the
   two hulls as computed), N (adele to idele) and O (the division as computed). Implemented in src/idmap.c;
   tests tests/test_idmap.c.

   Division of an adele by an exact rational is adf_adele_div_rat of adelefeld/adele.h (ideles.md P18). There is
   no division by an adf_adele (SPEC 4.5): a caller who holds the divisor as an adele converts it with
   adf_idele_set_adele, which returns ADF_UNIT_NOT_CERTIFIED for every finite part of positive radius.

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as an input of its type; an output never
     aliases a part of an input; an adele and an idele are different types.
   - Inputs satisfy their predicates; otherwise undefined (conventions 4.4, CV-09). With -DADF_CHECK_INVARIANTS
     every function checks its inputs on entry and aborts.
   - A function that returns a status leaves its output untouched on a status other than ADF_OK (conventions
     4.3). The finite part of an adele result is in the global backend (adelefeld/fball.h). */

#ifndef ADELEFELD_IDMAP_H
#define ADELEFELD_IDMAP_H

#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idele.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- idele -> adele (PLAN 2.3; SPEC 5 "Idele to adele"; void: nothing can fail) ---- */

/* adf_adele_set_idele(y, x): y = (X ; F), the smallest hull: for x = (X, r, c U(N)), N >= 1,
   F = r c' + r lcm(N, 2) Zhat with c' an odd integer, c' = c modulo N (ideles.md P16.2, P16.3: F contains the
   finite part r u of every point of x, and every finite ball that contains them all contains F). F depends only
   on the set of the unit (P16.4): the stored pair and its normal form give the same F. For an exact unit
   [e] (N = 0), F is the exact rational r e (SPEC 5). The real coordinate is X, a copy (no rounding, no prec).
   Enclosure: every point (xi, r u) of x lies in y. y and x are of different types. Cost: a gcd and the
   canonicalisation of adf_fball_set_fmpz3. */
void adf_adele_set_idele(adf_adele_t y, const adf_idele_t x);

/* adf_adele_set_idele_simple(y, x): y = (X ; r c' + r N' Zhat), (c', N') the normal form of the unit of x
   (ideles.md P16.1, the simple ball of the set; decision i3-4 of api-2.md 3.5: formed from the normal form, so
   that it depends only on the set). It contains the smallest hull; it is the smallest hull when N' is even and
   twice as coarse when N' is odd (P16.4): [2 mod 3] gives 2 r + 3 r Zhat, where adf_adele_set_idele gives
   5 r + 6 r Zhat. For an exact unit, the exact rational r e, as adf_adele_set_idele. y and x are of different
   types. */
void adf_adele_set_idele_simple(adf_adele_t y, const adf_idele_t x);

/* ---- adele -> idele (PLAN 2.3; conventions 3.2 row "Adele to idele") ---- */

/* adf_idele_set_adele(y, x): y = the idele value whose set is the set of units of A in x, when x certifies it:
   the finite part of x is an exact rational a != 0 and the real ball X excludes 0. Then every point of x is
   (xi, a) with xi != 0 and the diagonal a = |a| sign(a) (ideles.md P3.4, line 80), and y = (X, |a|, [sign a]),
   exact in the finite part (api-2.md N).
   Status (conventions 3.2; the maximum of 3.3 when several apply):
     ADF_NOT_UNIT        proved, y untouched: the finite part is the exact 0, or the real ball is the exact 0
                         (no point of x is a unit);
     ADF_UNIT_NOT_CERTIFIED  y untouched: the finite part has radius > 0 (SPEC 4.5; ideles.md P17: it contains
                         non-units, and its non-zero rational points are units), or the real ball contains 0
                         and is not the exact 0;
     ADF_OK              y written.
   A finite part in the local backend is never exact (fball.h), so it gives ADF_UNIT_NOT_CERTIFIED.
   y and x are of different types. Cost: a copy of the ball and of the rational. */
int adf_idele_set_adele(adf_idele_t y, const adf_adele_t x);

/* ---- division (PLAN 2.4; SPEC 5 "Division of an adele by an idele"; conventions 5.7, CV-50) ---- */

/* adf_adele_div_idele(z, x, y, prec): z = x / y for an adele x = (I ; a + M Zhat) and an idele
   y = (Y, r, c U(N)): every quotient of a point of x by a point of y lies in z.
   - Finite part, N >= 1: the smallest ball (a e)/r + (gcd(|a| L, M) / r) Zhat, L = lcm(N, 2), e odd with
     e = c^(-1) modulo N (ideles.md P19, line 472; M0-D7). It is computed as the product rule (adf_fball_mul;
     SPEC 4.3) of the finite part of x with the smallest hull e + L Zhat of the inverse coset, then divided by
     the exact r (adf_fball_div_rat; P18), which is the same ball (P19.6; api-2.md O). a = 0 gives (M/r) Zhat,
     M = 0 gives a e/r + (|a| L/r) Zhat.
   - Finite part, N = 0 (the exact unit [e]): (a e)/r + (M/r) Zhat, the division by the exact rational e r
     (P18; conventions 5.7).
   - Real part: arb_div(I, Y, p) with p = max(prec, 2) (arb.rst:856-876; "rounded as usual", SPEC 5); Y
     excludes 0.
   Status: ADF_OK, z written; ADF_NOT_DETERMINED if arb_div returns a ball that is not finite (conventions 4.4,
   CV-08: a non-finite ball is never stored; no input that reaches it is known), z untouched; ADF_LIMIT if
   prec > ADF_IDELE_PREC_MAX, z untouched (the rule of adelefeld/idele.h).
   z may be x. y is an idele and not part of z. Cost: a modular inverse, the product rule of fball.h, a
   division by r and arb_div at p bits. */
int adf_adele_div_idele(adf_adele_t z, const adf_adele_t x, const adf_idele_t y, slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_IDMAP_H */
