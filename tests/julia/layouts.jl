# tests/julia/layouts.jl: the documented layouts of the public structs of adelefeld, hand-copied
# from docs/api-m1.md, section "Layouts (64-bit, FLINT 3.0.1)" (lines 31-45 as read on 2026-09-28),
# for the Julia-friendly interface check of work package 1.9 (docs/PLAN.md row 1.9: "the sizes
# against the documented layouts"; docs/conventions.md 12.4, decision CV-40; docs/SPEC.md 10 item 4,
# M0-D12).
#
# This file is typed in from the documentation, not generated from the C headers and not read out
# of the library: a mismatch between this table and what the library's adf_sizeof_*/adf_alignof_*
# functions report is exactly the fault CV-40 exists to catch at load time. tests/julia/smoke.jl
# includes this file and compares every entry against a live ccall of the library.
#
# docs/api-m1.md, "Layouts" table:
#
#   | Type                | Size | Fields (offset)                                                |
#   |----------------------|------|----------------------------------------------------------------|
#   | adf_rat_struct       | 16   | q fmpq (0)                                                     |
#   | adf_fball_struct      | 48   | A (0), H (8), d (16) fmpz; backend int (24); mctx (32); res (40)|
#   | adf_scaled_struct     | 40   | s fmpq (0); u fmpz (16); mctx (24); exact int (32)             |
#   | adf_adele_struct      | 96   | inf arb (0); fin adf_fball_struct (48)                          |
#   | adf_cadele_struct     | 144  | inf acb (0); fin adf_fball_struct (96)                          |
#   | adf_place_t           | 8    | opaque ulong (0)                                               |
#   | adf_text_limits_t     | 32   | max_len size_t (0), max_exp10 (8), max_prec (16), max_items (24) slong |
#   | adf_ctx_desc_t        | 24   | K fmpz (0), k slong (8), q ulong pointer (16)                   |
#   | adf_text_kind         | 4    | enum, values 0 to 12                                           |
#
#   "All alignments are 8, except adf_text_kind (4)."
#
# Every entry below except adf_text_kind has an exported adf_sizeof_<type>/adf_alignof_<type> pair
# (docs/api-m1.md, "Functions" table, work package 1.9 rows); adf_text_kind is a plain enum with no
# accessor of its own (conventions 9.7: "An enum has the size and representation of int"), so it is
# listed separately, for documentation and for tests/test_layouts.c, and is not part of the ccall
# loop of smoke.jl.

# (documented name, sizeof-function symbol, alignof-function symbol, size in bytes, alignment)
const ADF_LAYOUTS = [
    ("adf_rat_struct",     :adf_sizeof_rat,         :adf_alignof_rat,          16,  8),
    ("adf_fball_struct",   :adf_sizeof_fball,       :adf_alignof_fball,        48,  8),
    ("adf_scaled_struct",  :adf_sizeof_scaled,      :adf_alignof_scaled,       40,  8),
    ("adf_adele_struct",   :adf_sizeof_adele,       :adf_alignof_adele,        96,  8),
    ("adf_cadele_struct",  :adf_sizeof_cadele,      :adf_alignof_cadele,      144,  8),
    ("adf_place_t",        :adf_sizeof_place,       :adf_alignof_place,         8,  8),
    ("adf_text_limits_t",  :adf_sizeof_text_limits, :adf_alignof_text_limits,  32,  8),
    ("adf_ctx_desc_t",     :adf_sizeof_ctx_desc,    :adf_alignof_ctx_desc,     24,  8),
]

# adf_text_kind: size 4, alignment 4 (docs/api-m1.md line 45); no adf_sizeof_text_kind/
# adf_alignof_text_kind is exported (it is not in the "Functions" table), so this row is
# informational only, checked on the C side by tests/test_layouts.c, not by ccall here.
const ADF_TEXT_KIND_SIZE = 4
const ADF_TEXT_KIND_ALIGN = 4

# Field offsets of adf_fball_struct (docs/api-m1.md line 36), used by smoke.jl to read the raw
# fmpz fields A and H directly after adf_fball_set_str, exactly as a zero-copy binding would. The
# sizeof/alignof checks above cannot catch a struct whose fields are in the wrong order but whose
# overall size and alignment are unchanged (for example the two fmpz fields A and H swapped): the
# offsets below close that gap for the one struct this lane exercises with raw memory reads.
const ADF_FBALL_OFFSET_A       = 0
const ADF_FBALL_OFFSET_H       = 8
const ADF_FBALL_OFFSET_D       = 16
const ADF_FBALL_OFFSET_BACKEND = 24
const ADF_FBALL_OFFSET_MCTX    = 32
const ADF_FBALL_OFFSET_RES     = 40
