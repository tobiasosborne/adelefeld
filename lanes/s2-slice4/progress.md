# Progress of lane s2-slice4 (roots modulo p above 2^20)

## Done
1. refs/src symlinked; brief, COMMON.md, COMMON-C.md, roots.h, roots.c, solvers P3.4 to P3.7 read.
2. Sources fetched with refs/fetch_sources.sh (new list FLINTSRC_MODP): nmod_poly_factor/roots.c,
   nmod_poly/find_distinct_nonzero_roots.c (_nmod_poly_split_rabin), nmod_poly/powmod_ui_binexp.c,
   nmod_poly/powmod_ui_binexp_preinv.c, nmod_poly/gcd.c, nmod_poly/evaluate_nmod.c. The script rewrote the
   manifests with files of other lanes too; refs/manifest.sha256 was reset to the original plus my 6 lines
   (sha256sum -c: 0 failures); manifest-extra.sha256 unchanged.

## Design (decided)
- hidden `slong adf_roots_modp(ulong * roots, const nmod_poly_t h, int route)`: route 0 auto (evaluation for
  p <= ADF_ROOTS_P_EVAL_MAX), 1 evaluation, 2 gcd route. Roots sorted ascending. h must be nonzero (abort).
- hidden `int adf_roots_modp_check(const nmod_poly_t h, const ulong * cand, slong m)`: 1 iff every candidate
  is < p, a root of h by evaluation, the candidates distinct, and m = deg gcd(h, X^p - X).
- gcd route: t = X^p mod h (nmod_poly_powmod_ui_binexp), d = gcd(h, t - X), candidates from
  nmod_poly_roots(d, 0); refused list -> abort (S-D20), not NOT_DETERMINED.

## Next
- tests/test_roots_bigp.c (red), then code, bench, bound, header, fuzz, mutants, full checks.
