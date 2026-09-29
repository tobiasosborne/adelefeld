# Progress of lane s2-slice4 (roots modulo p above 2^20)

## Done
1. refs/src symlinked; brief, COMMON.md, COMMON-C.md, roots.h, roots.c, solvers P3.4 to P3.7 read.
2. Sources fetched with refs/fetch_sources.sh (new list FLINTSRC_MODP): nmod_poly_factor/roots.c,
   nmod_poly/find_distinct_nonzero_roots.c (_nmod_poly_split_rabin), nmod_poly/powmod_ui_binexp.c,
   nmod_poly/powmod_ui_binexp_preinv.c, nmod_poly/gcd.c, nmod_poly/evaluate_nmod.c. The script rewrote the
   manifests with files of other lanes too; refs/manifest.sha256 was reset to the original plus my 6 lines
   (sha256sum -c: 0 failures); manifest-extra.sha256 unchanged.
3. tests/test_roots_bigp.c written (red 1 link error, red 2 assertions, green): lanes/s2-slice4/redgreen.log.
4. src/roots.c: adf_roots_modp (hidden, route AUTO/EVAL/GCD), adf_roots_modp_check (hidden), padic_search uses
   it; UNSUPPORTED removed from adf_roots_padic_core and from verify_complete.
5. bench/bench_roots_modp.c; run 2026-09-29T144527Z (file moved to lanes/s2-slice4/); crossover by the geometric
   mean between p = 127 (0.94) and 251 (1.82): ADF_ROOTS_P_EVAL_MAX = 128.
6. roots.h: the temporary-status sentences rewritten; the macro 128 with the measurement.
7. tests/test_roots_padic.c: the three UNSUPPORTED checks replaced (red 3, green 2 in the log).
8. Tests pass: bigp 5 tests 825 checks, padic 8 tests 55135 checks, seed, forged.

## Next
- docs/sources.md rows (nmod_poly_roots etc.) and the pending item 4 of milestone S.
- mutants of item 5 of the brief in a scratch copy under build/ (4 mutants).
- tests/fuzz/diff_roots_padic.py: primes above the bound with planted roots; run 180 s.
- make clean && make check-all; SAN=1; CC=clang; check_headers.sh.
- result.md.

## Update
- docs/sources.md: one row in table 1, nine rows in table 3 S.2 (quotes checked line by line against the files by
  a script: 0 mismatches), item 5 under "Sources pending for milestone S". flint.h.in fetched too (7 files).
- Mutants (lanes/s2-slice4/mutants.py, log mutants.log): 4 of 4 killed.
- Next: fuzz script, full checks, result.md.
