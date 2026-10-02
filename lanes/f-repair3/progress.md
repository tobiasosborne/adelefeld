# f-repair3 progress (running notes)

- 20:47 UTC: read brief, CLAUDE.md, COMMON.md, COMMON-C.md, workflow.md, review-lfunc-trig.md, F10-F14,
  src/lfunc.c 250-335 (exp) and 690-813 (parity), tests/test_lfunc_trig.c, proto/lfunc_trig_checks.py header
  (check mode = run without arguments: selftest only).
- Plan R2: loop for k = L, L-2, ..., 2: F = F k (k-1) mod p^W, A = x^2 A + eps_(k-2) F mod p^W; odd L: A = x A.
  parity_coefficient's parity test becomes dead (every degree reached has the retained parity): replace it
  by a sign function.
- Step 1 done: ref_lfunc.c + compare.c; 0 differences before; a planted fault gives 11714 (redgreen.log).
- Step 2 done: pinned test (20 residues, cross-checked with the Fraction oracle 20/20), red on planted fault.
- 20:51: orchestrator: refs/src now on disk (symlink); cite FLINT docs as refs/src/flint-3.0.1/fmpz.rst:<line>.
- Step 3 done: paired loop; fmpz_mul2_uiui (fmpz.rst:758-760); F11 one sentence, F13 two sentences, new F15.
- Step 4 done: all green (redgreen.log). Next: mutation testing.
- 20:55-: mutation run (tools/mutate, 60 mutants, seed 1, 2 jobs, --san, INV=1, test_lfunc_trig + test_lfunc)
  in background; first survivor 739 drop fmpz_mod (equivalent: residue argument).
- 21:16: mutation run done: 60 run, 41 killed, 14 survived, 4 not compiled, 1 timed out.
- 21:16-21:32: lanes/f-repair3/loop_mutants.py: all 51 tool mutants of lines 690-739 (parity_sign, parity_centre
  up to the odd step): 37 killed by test_lfunc_trig, 1 by compare only (timeout), 13 survived (all equivalent).
- 21:33: final rerun of the three test programs green; result.md written.
