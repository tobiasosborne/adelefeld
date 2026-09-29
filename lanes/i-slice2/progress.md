# Lane i-slice2: progress (running notes)

Started 2026-09-29 23:42 (clock). Worktree at adf87c1; refs/src linked to the main checkout.

## Read
- CLAUDE.md, lanes/COMMON.md, lanes/COMMON-C.md, docs/workflow.md, brief.
- SPEC 5, 15; PLAN milestone 2; ideles.md (all); conventions 2 to 4, 5.6, 5.7, 6, 7, 12; api-2.md 1;
  idele.h, ucoset.h, src/idele.c, i-slice1 result, gen_vectors.py, test_idele.c, ideles.jl, test_julia.sh.
- d-ideles worktree: proto/ideles_checks.py part 2 lines 795-1240 (kernel, ref_real_scale, ref_idele_mul_rat,
  ref_idele_class, ref_idele_valuation, ref_idele_abs_rat, check_api_norm_valuation, check_api_class_map).
  No api-2.md there (the lane stopped before writing it).

## Plan (functions)
- idclass.h (new): struct {arb t; ucoset u}, 64 bytes; init <1;[1]>, clear, set, swap, is_canonical,
  identical, set_parts (OK, DOMAIN), get_t, get_unit, norm (copy of t), mul, inv (OK, ND), set_idele
  (the class map, OK, ND), sizeof/alignof.
- idele.h additions: mul_rat (OK, ND, NOT_UNIT), valuation_at (OK, DOMAIN at inf), abs_at (adf_rat, prime;
  DOMAIN at inf), abs_inf (arb, exact, void), norm (arb, OK, ND).
- src/idele_internal.h: kernel E1, B, defect, and the scaling by n/d (new, Statement F).

## State (00:08)
- [x] headers (idclass.h new; idele.h additions; ADF_IDELE_PREC_MAX added on the orchestrator's message)
- [x] api-2.md section 2 (statements F, G, H, I; decisions i2-1..i2-9; LIMIT note; section 1 rows amended)
- [x] proto part 3 + vectors (python3 proto/ideles_checks.py part3: pass, 8.5 s; 864 KB of vectors)
- [x] tests red (redgreen.log)
- [x] code green (test_idclass 9738 checks, test_idele_maps 10466, test_idele 8613; INV=1 the same)
- [x] julia (43 of 43)
- [x] faults (G1-G4 each caught; faults.log)
- [x] check runs: check_headers, check-all, SAN, clang (final runs 00:22-00:33, all pass)
- [x] result.md
