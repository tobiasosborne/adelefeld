# Progress, lane d-zeta

- Read CLAUDE.md, workflow, common lane rules, the named contracts, proofs and headers.
  api-1f2.md and cadele.h do not exist; their material is in api-1f.md and adele.h.
- Z1 proved: the finite continuation, complete simple pole set, residue 1/log(p), and no zeros.
- Z2 proved using tate-poonen/notes.txt:62-64: real continuation, complete simple pole set,
  residues 2 (-1)^n pi^n/n!, and no zeros. This closes catalogue Proposition 9's Gamma source gap.
- Z3 proved conditionally: only zero is a dyadic finite pole, using Gelfond-Schneider.
  No on-disk statement of that theorem was found. It remains explicitly source pending.
- Z4 proved: the midpoint exponential certificate and real integer-geometry test never accept a pole ball.
- Z5 proved: R/d<=1/8, log(p) R<=1 and measured error <=|D(m)|/16 suffice for finite pole exclusion.
  No universal prec threshold follows from the documented acb contract.
- Z6 proved: derivative bounds and variation R B using scalar bounds and the Gamma recurrence/integral.
- Z7 proved: near-pole scales, extreme finite half-plane bounds, and sampled lower image-width certificates.
  arf has arbitrary integer exponents. Evaluator refusal is NOT_DETERMINED; a declared precision cap is LIMIT.
- Z8 proved: independent Gamma point enclosures from a degree-2200 Taylor Mellin integral on [0,512],
  with finite and infinite tails, recurrence and scalar arb complex assembly at 2048 bits.
- Z9 specified: mpmath 320-digit references have independently certified error <=1e-200 max(1,|value|).
  Status simulation is separate from the reference. The real acb candidates are checked against that reference.
- Initial checks exposed general arb **2 rejecting sign-indefinite intervals, loss of reference digits near
  poles at 240 digits, non-finite direct Gamma on a separated box, and large rational export denominators.
  Fixed interval squaring, increased reference precision to 320, added a bounded recurrence fallback,
  and exported exact dyadic bound endpoints. No pole fixture was removed or weakened.
- First complete run: 1349 status cases, 5719 certified samples, 630 positive width witnesses; 35.44 s.
- Stronger real candidate check: 1890 sample-containment checks, 0 failures; 4 width failures,
  worst ratio 279.62142125231634. Midpoint plus derivative refinement repairs the candidate;
  the four derivative enclosure ratios were 13.9089, 14.4468, 14.7879 and 15.0277.
- Refined complete run: 1349 cases, 5719 simulated sample-containment checks, 630 width checks;
  32.01 s, 42088 KiB peak. Boundary, general complex-box and extreme-argument cases were then added.
- The extreme-phase case exposed correlation loss in the scalar expm1 simulator. Intersecting its
  denominator with the separately computed 1-exp enclosure repairs that certificate without changing a status test.
- Final oracle run: 63 pole/residue cases, 7 Gamma-integral checks, 12 precision cases, 1377 fixtures;
  statuses OK 701, ND 642, DOMAIN 32, LIMIT 2; 5845 sample-containment checks and 643 width checks.
  Exit 0; 108.59 s; 41896 KiB peak. The run is deterministic, not a long fuzz run.
- Finished the proposed acb interface, five TJO points, eight implementation mutants, driver commands,
  Julia call and source-obligation list. No C code or header was changed.
- Final environment audit: python-flint 0.8.0 uses FLINT 3.3.1; the system headers name 3.0.1.
  Recorded that version difference and its limits. A ctypes version-symbol probe exited with signal 11;
  after reading flint.h.in:105, the corrected char-array probe confirmed system FLINT 3.0.1, exit 0.
  No oracle result used either probe. One incidental oracle bytecode file was removed.
