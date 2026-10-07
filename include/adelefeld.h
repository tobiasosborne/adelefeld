/* adelefeld.h: the public interface of adelefeld, version 0.1.0.

   Milestone 1 (docs/PLAN.md section 6): the ring. This header includes every public header; each of
   them can also be included alone, in C11 and in C++17 (tests/test_headers.c). Declarations only:
   the library implements them in work packages 1.2 to 1.9. The contract is docs/conventions.md 0.4,
   docs/SPEC.md 1.3 and the proofs cited function by function; docs/api-m1.md lists every function
   with its header, work package and source.

   The public types embed FLINT 3.0.1 types (fmpz, fmpq, arb, acb), so a user needs the FLINT include
   path; adelefeld/common.h refuses any FLINT other than 3.0.x at compile time, and
   adf_version_check() compares at run time (conventions 12.11, CV-44).

   Headers:
     adelefeld/common.h   FLINT includes and version check, ADF_INLINE, adf_modctx_struct,
                          adf_str_free, adf_flint_version_compiled, adf_version_check
     adelefeld/status.h   ADF_OK ... ADF_LIMIT, ADF_CMP_*, adf_status_str
     adelefeld/place.h    adf_place_t and its functions
     adelefeld/rat.h      adf_rat
     adelefeld/fball.h    adf_fball, tight policy
     adelefeld/lball.h    adf_lball, a ball in Q_p at one prime (milestone 1F.3, first slice)
     adelefeld/adele.h    adf_adele, adf_cadele
     adelefeld/sball.h    adf_sball, partial balls over a set of places (milestone 1F.1)
     adelefeld/rfunc.h    exp, log, sin, cos, sqrt, roots at the real place (milestone 1F.2)
     adelefeld/lfunc.h    exp, log and the Iwasawa Log on a local ball at a prime (milestone 1F.4)
     adelefeld/lpow.h     rational powers and powers of principal units at a prime (milestone 1F.6)
     adelefeld/recon.h    rational reconstruction from a full ball
     adelefeld/text.h     value form, limits, adf_text_classify
     adelefeld/modctx.h   contexts, descriptors, local-backend conversions
     adelefeld/scaled.h   adf_scaled (scaled policy), absolute cap
     adelefeld/dump.h     dump form
     adelefeld/ucoset.h   adf_ucoset, unit cosets and the exact units (milestone 2, slice 1)
     adelefeld/idele.h    adf_idele, ideles: product, inverse, the idele of a rational (slice 1)
     adelefeld/idclass.h  adf_idclass, idele classes, the class map; valuations and norm (slice 2)
     adelefeld/idpow.h    powers of unit cosets, ideles and classes: c^k U(N) and the tight M_k (slice 3)
     adelefeld/idmap.h    idele -> adele (two hulls), adele -> idele, adele / idele (slice 3)
     adelefeld/gfunc.h    functions at all places: roots; exp, sin, sinh, cos, cosh (milestone 1F.8) */

#ifndef ADELEFELD_H
#define ADELEFELD_H

#define ADF_VERSION_MAJOR 0
#define ADF_VERSION_MINOR 1
#define ADF_VERSION_PATCH 0

/* ADF_VERSION as the number MAJOR*10000 + MINOR*100 + PATCH, for comparisons. */
#define ADF_VERSION \
    (ADF_VERSION_MAJOR * 10000 + ADF_VERSION_MINOR * 100 + ADF_VERSION_PATCH)

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/lball.h"
#include "adelefeld/adele.h"
#include "adelefeld/qclass.h"
#include "adelefeld/psi.h"
#include "adelefeld/char.h"

#include "adelefeld/ffun.h"
#include "adelefeld/sball.h"
#include "adelefeld/rfunc.h"
#include "adelefeld/lfunc.h"
#include "adelefeld/lroot.h"
#include "adelefeld/lpow.h"
#include "adelefeld/recon.h"
#include "adelefeld/resid.h"
#include "adelefeld/linsolve.h"
#include "adelefeld/roots.h"
#include "adelefeld/text.h"
#include "adelefeld/modctx.h"
#include "adelefeld/scaled.h"
#include "adelefeld/dump.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idele.h"
#include "adelefeld/idclass.h"
#include "adelefeld/idpow.h"
#include "adelefeld/idmap.h"
#include "adelefeld/gfunc.h"
#include "adelefeld/symbol.h"
#include "adelefeld/catalogue.h"
#include "adelefeld/localfactor.h"
#include "adelefeld/rfun.h"

#endif /* ADELEFELD_H */
