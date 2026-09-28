/* tests/test_layouts.c: what tests/test_abi.c leaves out of the struct-layout checks of work
   package 1.9 (docs/PLAN.md row 1.9: "the sizes against the documented layouts"; lane m1-julia's
   brief, point 1: "for every public struct, sizeof, _Alignof and offsetof of every field equal
   the numbers documented in the header comment and in docs/api-m1.md, and the exported functions
   adf_sizeof_<type>()/adf_alignof_<type>() return the same").

   What tests/test_abi.c (lane m1-headers) already checks, read in full before this file was
   written, so as not to duplicate it:
     - sizeof, _Alignof and offsetof of every field of adf_rat_struct, adf_fball_struct,
       adf_scaled_struct, adf_adele_struct, adf_cadele_struct, against the header's own layout
       comment (which states the same numbers as docs/conventions.md 5.1, 5.2, 5.4, 5.5 and
       docs/api-m1.md "Layouts") and against docs/api-m1.md itself, by _Static_assert;
     - the type of every field (ADF_FTYPE, via _Generic on an unevaluated operand), which sizes
       and offsets alone cannot pin down (padding can hide a narrower type in a wider slot);
     - sizeof and _Alignof of adf_place_t (both fields: only one, opaque, at offset 0, so no
       separate offsetof line is needed: C11 6.7.2.1p15 gives the first member offset 0);
     - sizeof of adf_text_kind (4 bytes: an enum has the representation of int, conventions 9.7);
     - a run-time test (sizeof_functions_agree_with_the_layouts) that calls every
       adf_sizeof_<type>()/adf_alignof_<type>() of docs/api-m1.md's "Functions" table (place,
       rat, fball, adele, cadele, text_limits, scaled, ctx_desc) and compares each against the
       same fixed numbers the _Static_asserts above pin down for the type itself; since the type
       is already asserted to have exactly those numbers, this closes the loop between the header
       and the exported query functions completely, for all eight types. Not repeated here.
     - a run-time test (array_of_one_types) that the five "_t" array-of-one types (adf_rat_t,
       adf_fball_t, adf_adele_t, adf_cadele_t, adf_scaled_t) have the same sizeof as their
       element struct.

   What is left out, and what this file adds (found by reading tests/test_abi.c line by line
   against docs/api-m1.md's "Layouts" table, docs/api-m1.md:31-45):

   1. docs/api-m1.md:45 states "All alignments are 8, except adf_text_kind (4)." Every alignment
      of 8 is pinned by an ADF_ALIGN of tests/test_abi.c, for every struct that has one. But
      adf_text_limits_t and adf_ctx_desc_t (adelefeld/text.h:67-73, adelefeld/modctx.h:108-113)
      have ADF_SIZE and ADF_OFF lines in tests/test_abi.c, and no ADF_ALIGN line: their alignment
      is checked only at run time, by comparing adf_alignof_text_limits()/adf_alignof_ctx_desc()
      against the literal 8, never against _Alignof of the type itself. Added below (2, 3).
   2. adf_text_kind's own alignment (conventions 9.7: "the representation of int"; docs/api-m1.md
      line 45: alignment 4) has no query function (it is not in docs/api-m1.md's "Functions"
      table: only the eight struct types have adf_sizeof_<T> and adf_alignof_<T>, text.h:88-105)
      and no
      _Static_assert anywhere: tests/test_abi.c pins only its size (ADF_SIZE(adf_text_kind, 4),
      test_abi.c:121). Added below (4): this is exactly the fact a Julia binding needs and that
      no exported function states, since adf_text_classify (adelefeld/text.h:113) writes an
      adf_text_kind through an output pointer that the binding must place at the right alignment.
   3. The five "_t" array-of-one types have their sizeof checked (test_abi.c's
      array_of_one_types) but not their _Alignof. In C this equals the element type's alignment
      by construction (an array's alignment is its element's alignment, C11 6.2.5p20), so this is
      not an independent fact, but it is the exact fact a binding relies on when it allocates
      adf_sizeof_<type>() bytes at adf_alignof_<type>() and treats the block as the "_t" type
      (docs/conventions.md 12.4, CV-40; lane m1-julia's smoke.jl does exactly this with
      Libc.malloc). Pinned below (5) so that a future change of any struct's alignment is caught
      here too, not only through the struct itself. */

#include <stddef.h>

#include <adelefeld.h>

#include "test_runner.h"

#define ADF_ALIGN2(T, n) _Static_assert(_Alignof(T) == (n), "_Alignof(" #T ") != " #n)

/* ---- 1, 2. the two structs whose alignment tests/test_abi.c checks only at run time,
   against the literal 8, never against _Alignof of the type itself (adelefeld/text.h:60-73,
   "size 32, alignment 8"; adelefeld/modctx.h:104-113, "size 24, alignment 8"). ---- */
ADF_ALIGN2(adf_text_limits_t, 8);
ADF_ALIGN2(adf_ctx_desc_t, 8);

/* ---- 3. adf_text_kind: docs/api-m1.md:45, "All alignments are 8, except adf_text_kind (4)";
   adelefeld/text.h:89, "An enum has the size and representation of int on the supported
   platforms (checked by tests/test_abi.c)" -- that comment names only the size check
   (ADF_SIZE(adf_text_kind, 4), test_abi.c:121); the alignment half of the same sentence
   ("representation of int" includes int's alignment) had no check anywhere. There is no
   adf_alignof_text_kind function (it is not in docs/api-m1.md's "Functions" table), so this is
   the only place this fact is pinned; a Julia binding that writes adf_text_kind through
   Ref{Cint} (matching _Alignof(int) == 4) relies on it. ---- */
ADF_ALIGN2(adf_text_kind, 4);
ADF_ALIGN2(int, 4);

/* ---- 4. the "_t" array-of-one types: sizeof is checked by tests/test_abi.c
   (array_of_one_types); _Alignof is not. Mechanically implied by C's array-alignment rule
   (C11 6.2.5p20: an array's alignment is its element type's), but pinned here so that a change
   to any of the five element structs' alignment is also caught through the type a function
   signature actually uses (adf_rat_t, not adf_rat_struct). ---- */
ADF_ALIGN2(adf_rat_t, 8);
ADF_ALIGN2(adf_fball_t, 8);
ADF_ALIGN2(adf_adele_t, 8);
ADF_ALIGN2(adf_cadele_t, 8);
ADF_ALIGN2(adf_scaled_t, 8);

/* A run-time counterpart to the four static assertions above, in the style of test_abi.c's
   sizeof_functions_agree_with_the_layouts: _Alignof read at run time by the compiler that built
   this test file, compared with the fixed numbers docs/api-m1.md states. No library function is
   called for adf_text_kind (there is none); the point of this test is exactly that its alignment
   is a fact only the documentation and this static assertion carry, not a run-time query. */
ADF_TEST(alignments_left_out_by_test_abi_c)
{
    ADF_CHECK(_Alignof(adf_text_limits_t) == 8);
    ADF_CHECK(adf_alignof_text_limits() == _Alignof(adf_text_limits_t));
    ADF_CHECK(_Alignof(adf_ctx_desc_t) == 8);
    ADF_CHECK(adf_alignof_ctx_desc() == _Alignof(adf_ctx_desc_t));
    ADF_CHECK(_Alignof(adf_text_kind) == 4);
    ADF_CHECK(_Alignof(adf_rat_t) == _Alignof(adf_rat_struct));
    ADF_CHECK(_Alignof(adf_fball_t) == _Alignof(adf_fball_struct));
    ADF_CHECK(_Alignof(adf_adele_t) == _Alignof(adf_adele_struct));
    ADF_CHECK(_Alignof(adf_cadele_t) == _Alignof(adf_cadele_struct));
    ADF_CHECK(_Alignof(adf_scaled_t) == _Alignof(adf_scaled_struct));
}

/* A test is not a test if a wrong implementation would pass it (tests/README.md item 6): if this
   file is ever reduced to the two lines above and the _Static_asserts are dropped, a struct whose
   alignment silently changed would still fail to compile any file that redeclares the type with
   the old alignment expectation elsewhere in the tree (test_abi.c), so the coverage is real: the
   red run below shows both this file and test_abi.c catching a mutation, from two different
   angles (compile-time here, run-time comparison to the exported function in the same test). */
