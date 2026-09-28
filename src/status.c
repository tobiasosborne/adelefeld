/* status.c: the status codes of conventions 3.1 and the names adf_status_str returns.

   include/adelefeld/status.h declares exactly one function, adf_status_str, and it is a
   header-inline function (ADF_INLINE, conventions 12.1): its definition is in the header, where
   a caller sees it as a static inline fast path, and src/inlines.c compiles the same definition
   with external linkage so that the library exports the symbol as well. There is therefore no
   other operation of this header to put in a source file, and lanes/m1-rat/check_symbols.sh
   checks that adf_status_str really is a defined symbol of the library.

   What is left for this file is the one thing a source file can do for a block of constants:
   check at compile time that the header still says what conventions 3.1 and section 2.1 say.
   The numeric values are part of the foreign-function interface (conventions 12.3, and the
   header's own remark that tests/test_headers.c checks them), so an edit of the header that
   changes one of them must break the build of the library, not only the build of a test. */

#include <adelefeld.h>

/* conventions 3.1, the table "The list", in increasing order of precedence; the combination
   rule of 3.3 is the maximum, which is why the order matters and not only the values. */
_Static_assert(ADF_OK == 0, "conventions 3.1: ADF_OK is 0");
_Static_assert(ADF_NOT_DETERMINED == 1, "conventions 3.1: ADF_NOT_DETERMINED is 1");
_Static_assert(ADF_UNIT_NOT_CERTIFIED == 2, "conventions 3.1: ADF_UNIT_NOT_CERTIFIED is 2");
_Static_assert(ADF_NEEDS_SPLIT == 3, "conventions 3.1: ADF_NEEDS_SPLIT is 3");
_Static_assert(ADF_NOT_UNIQUE == 4, "conventions 3.1: ADF_NOT_UNIQUE is 4");
_Static_assert(ADF_NO_SOLUTION == 5, "conventions 3.1: ADF_NO_SOLUTION is 5");
_Static_assert(ADF_NOT_UNIT == 6, "conventions 3.1: ADF_NOT_UNIT is 6");
_Static_assert(ADF_DOMAIN == 7, "conventions 3.1: ADF_DOMAIN is 7");
_Static_assert(ADF_UNSUPPORTED == 8, "conventions 3.1: ADF_UNSUPPORTED is 8");
_Static_assert(ADF_PARSE == 9, "conventions 3.1: ADF_PARSE is 9");
_Static_assert(ADF_LIMIT == 10, "conventions 3.1: ADF_LIMIT is 10");

/* The valid codes are 0 to ADF_STATUS_COUNT - 1 (the header; conventions 3.1 lists 11 of them). */
_Static_assert(ADF_STATUS_COUNT == 11, "there are eleven status codes");

/* conventions 2.1 and the closure finding C1: the comparison of points is not a status. The
   three values are 0, 1, 2, which are also the values of ADF_OK, ADF_NOT_DETERMINED and
   ADF_UNIT_NOT_CERTIFIED; nothing but the kind of the function that returns them tells the two
   apart, which is why the header says of adf_status_str that it takes a status and of the
   comparison that it "is not a status". A caller must not pass a comparison value to
   adf_status_str. */
_Static_assert(ADF_CMP_EQUAL == 0, "conventions 2.1: ADF_CMP_EQUAL is 0");
_Static_assert(ADF_CMP_DIFFERENT == 1, "conventions 2.1: ADF_CMP_DIFFERENT is 1");
_Static_assert(ADF_CMP_UNDECIDED == 2, "conventions 2.1: ADF_CMP_UNDECIDED is 2");
