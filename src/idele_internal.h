/* src/idele_internal.h: the real kernel of src/idele.c (docs/api-2.md 1.3, Statement E; 2, Statement F),
   shared with src/idclass.c (lane i-slice2). Not installed and not part of the interface: the symbols are
   hidden in a shared library (tests/test_exports.sh fails if a symbol is exported that no public header
   declares), as in src/modctx_internal.h.

   adf_idele_ends_of_abs(l, h, x, p): E1. For a finite ball x = [m +- rho] that excludes 0, l = RD_p(|m| - rho)
     and h = RU_p(|m| + rho); returns the sign of m (+1 or -1), which is the sign of every point of x. l > 0.
   adf_idele_ends_scale(lo, hi, l, h, a, b, p): Statement F. For dyadic 0 < l <= h and integers a, b >= 1,
     lo = RD_p(l a / b) and hi = RU_p(h a / b), each one correct rounding of the exact rational; lo > 0.
   adf_idele_ball_from_ends(z, lo, hi, sign, p): kernel B (E5). From dyadic 0 < lo <= hi of at most p bits
     each: ADF_NOT_DETERMINED (B1, z untouched) if e(hi) - e(lo) > p; otherwise ADF_OK and z a finite ball
     that contains sign * [lo, hi] and excludes 0, with midpoint of the sign `sign`.
   adf_idele_kernel_defect(what): a result that fails its own test is a defect of the library (decision i1-6
     of docs/api-2.md 1.4, as S-D20): a line on stderr and flint_abort; never returns. */

#ifndef ADELEFELD_IDELE_INTERNAL_H
#define ADELEFELD_IDELE_INTERNAL_H

#include <flint/arb.h>

#ifndef ADF_INTERNAL
#if defined(__GNUC__) || defined(__clang__)
#define ADF_INTERNAL __attribute__((visibility("hidden")))
#else
#define ADF_INTERNAL
#endif
#endif

ADF_INTERNAL int adf_idele_ends_of_abs(arf_t l, arf_t h, const arb_t x, slong p);
ADF_INTERNAL void adf_idele_ends_scale(arf_t lo, arf_t hi, const arf_t l, const arf_t h, const fmpz_t a,
                                       const fmpz_t b, slong p);
ADF_INTERNAL int adf_idele_ball_from_ends(arb_t z, const arf_t lo, const arf_t hi, int sign, slong p);
ADF_INTERNAL void adf_idele_kernel_defect(const char * what);

#endif
