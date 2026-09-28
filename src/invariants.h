/* src/invariants.h: the debug build -DADF_CHECK_INVARIANTS (docs/conventions.md 4.4, row
   "Non-canonical value passed to a public function", line 288, DECISION CV-09; 4.6, "Lifetime
   contract", lines 323-328; include/adelefeld/modctx.h:16-18; docs/SPEC.md 15, row M1-D2).
   Not installed and not part of the interface. Every macro is empty without the flag, so that
   the library built without it is the same object code as before (lanes/m1-invariants/report.md,
   step 6).

   1. Entry check. ADF_INV_RAT(x), ADF_INV_FBALL(x), ADF_INV_SCALED(x), ADF_INV_ADELE(x),
      ADF_INV_CADELE(x) stand at the top of every public function that reads x (the list is
      lanes/m1-invariants/functions.tsv). If adf_<type>_is_canonical(x) is 0, one line is written
      to stderr, naming the function (__func__), the argument (the text of the macro argument) and
      the type, and flint_abort is called (flint.h:219). The predicates never abort themselves
      (decision M1-D2), so a check cannot recurse.

   2. Borrow count. A context has an atomic count of the values that refer to it, a field that
      exists only under the flag (src/modctx.c). A value that stores a context pointer takes one
      borrow, a value that overwrites or drops a context pointer gives one back:
      - ADF_INV_BORROW(c): the value now refers to c (c may be NULL: nothing);
      - ADF_INV_RELEASE(c): the value no longer refers to c (c may be NULL: nothing);
      - ADF_INV_RETARGET(field, c): a value whose context field is `field` is about to be given the
        context c: takes c first, then releases the old one, so an aliased call (the new context is
        the old one) never lets the count touch 0. It stands before the assignment
        `field = c`.
      adf_modctx_free aborts, with a line on stderr, if the count is not zero. A release never takes
      the count below zero (a value with a context pointer that the library did not count, made by
      memcpy or by writing the field by hand, is invisible to the count; clearing it must not abort).
      The struct adf_modctx_struct is incomplete in the public header, so no public layout changes. */

#ifndef ADELEFELD_INVARIANTS_H
#define ADELEFELD_INVARIANTS_H

#include "adelefeld/modctx.h"

#ifdef ADF_CHECK_INVARIANTS

#include <stdio.h>

#include <adelefeld.h>

#include <flint/flint.h>

#if defined(__GNUC__) || defined(__clang__)
#define ADF_INV_HIDDEN __attribute__((visibility("hidden")))
#else
#define ADF_INV_HIDDEN
#endif

/* Defined in src/modctx.c, where the struct is. */
ADF_INV_HIDDEN void adf_inv_borrow(const adf_modctx_struct * ctx);
ADF_INV_HIDDEN void adf_inv_release(const adf_modctx_struct * ctx);

static inline void
adf_inv_fail(const char * fn, const char * arg, const char * type)
{
    fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical %s\n", fn, arg,
            type);
    fflush(stderr);
    flint_abort();
}

/* The check of one type is a function of its own that is never inlined: the predicate is then not
   inlined into the caller (gcc 13 reports a false -Wstringop-overread in adf_cadele_mul when it
   is), and the code of the check is not repeated in every function. */
#if defined(__GNUC__) || defined(__clang__)
#define ADF_INV_NOINLINE __attribute__((noinline, unused))
#else
#define ADF_INV_NOINLINE
#endif

#define ADF_INV_DEFINE(T)                                                                              \
    static ADF_INV_NOINLINE void adf_inv_check_##T(const char * fn, const char * arg,                  \
                                                   const adf_##T##_struct * x)                         \
    {                                                                                                  \
        if (!adf_##T##_is_canonical(x))                                                                \
            adf_inv_fail(fn, arg, "adf_" #T);                                                          \
    }

ADF_INV_DEFINE(rat)
ADF_INV_DEFINE(fball)
ADF_INV_DEFINE(scaled)
ADF_INV_DEFINE(adele)
ADF_INV_DEFINE(cadele)

#define ADF_INV_RAT(x) adf_inv_check_rat(__func__, #x, x)
#define ADF_INV_FBALL(x) adf_inv_check_fball(__func__, #x, x)
/* The same, for a parameter whose C name differs from the name that the header gives it. */
#define ADF_INV_FBALL_NAMED(name, x) adf_inv_check_fball(__func__, name, x)
#define ADF_INV_SCALED(x) adf_inv_check_scaled(__func__, #x, x)
#define ADF_INV_ADELE(x) adf_inv_check_adele(__func__, #x, x)
#define ADF_INV_CADELE(x) adf_inv_check_cadele(__func__, #x, x)

#define ADF_INV_BORROW(c) adf_inv_borrow(c)
#define ADF_INV_RELEASE(c) adf_inv_release(c)
#define ADF_INV_RETARGET(field, c)                                                                     \
    do { const adf_modctx_struct * adf_inv_new_ = (c); adf_inv_borrow(adf_inv_new_);                   \
         adf_inv_release(field); } while (0)

#else

#define ADF_INV_RAT(x) ((void) 0)
#define ADF_INV_FBALL(x) ((void) 0)
#define ADF_INV_FBALL_NAMED(name, x) ((void) 0)
#define ADF_INV_SCALED(x) ((void) 0)
#define ADF_INV_ADELE(x) ((void) 0)
#define ADF_INV_CADELE(x) ((void) 0)
#define ADF_INV_BORROW(c) ((void) 0)
#define ADF_INV_RELEASE(c) ((void) 0)
#define ADF_INV_RETARGET(field, c) ((void) 0)

#endif

#endif
