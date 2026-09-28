# progress (notes; the report is report.md, written at the end)

- Session 2 (this one): selftest passed at start (selftest-start.log, 61 s).
- Bug found and fixed in mutate.py: an entry with `#1` never matched the first of several identical
  mutants (entry_key dropped the number 1). Red: selftest-red-occ.log; green: selftest-green-occ.log.
- listkeys.py lists every mutant key; judge.py builds and judges single mutants (<= 5 per call).
- Plan for equivalent.txt: spec-driven generator build_equiv.py (each entry resolved to exactly one
  mutant, then written with Mutant.entry_text); mutants of new or changed entries judged with judge.py,
  logs judge-*.log.
- Not added, reasons not verifiable from ground truth: recon `<= 0` -> `<= 1` in fmpq_set_dyadic (rests
  on a measurement, source pending), modctx `fmpz_fdiv_r(out, out, ctx->K)` drop (sign argument of
  fmpz_multi_CRT_precomp undocumented), dump `b > 0x7e` >= (not equivalent: "adf2 ~" is UNSUPPORTED
  in the original, PARSE in the mutant), dump `*pos < len` (needs an audit of every caller).
