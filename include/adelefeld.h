/* adelefeld.h: the public interface of adelefeld, version 0.0.0.

   Work package 0.1 is a provisional build scaffold. There is no public type and no public
   function yet, and none is added here: the types and their storage invariants are fixed in
   work package 0.4, and the header is frozen after 0.3, 0.4 and 0.6 are reviewed
   (docs/PLAN.md section 1, principle 1; docs/PLAN.md section 6, milestone 0).

   So this header holds the include guard, the version macros, and nothing else. It must stay
   includable from C11 without any FLINT header: a user of the library should not need the
   include path of FLINT until a type from it appears. The test tests/test_scaffold.c includes
   FLINT itself and then this header, which is the order the milestones will use. */

#ifndef ADELEFELD_H
#define ADELEFELD_H

#define ADF_VERSION_MAJOR 0
#define ADF_VERSION_MINOR 0
#define ADF_VERSION_PATCH 0

/* ADF_VERSION as the number 0*10000 + 0*100 + 0, for comparisons. */
#define ADF_VERSION \
    (ADF_VERSION_MAJOR * 10000 + ADF_VERSION_MINOR * 100 + ADF_VERSION_PATCH)

/* No public type, no public function, no public macro, before the review of milestone 0. */

#endif /* ADELEFELD_H */
