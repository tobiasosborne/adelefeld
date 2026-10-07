# Progress

Read CLAUDE.md, workflow rules, the named SPEC/PLAN sections, analysis Propositions 9-15,
api-4 and the current character, factor and tensor contracts. No source or header is changed.

Design direction:
- Separate the uncompleted Tate value from Lambda; use acb results and explicit domain calls.
- Use (alpha, primitive prime-power unit character) locally, with separate real parity calls.
- Use splitting and theta Mellin integrals for the global algorithm. Euler products are a bounded check.
- Give an elementary certified Taylor quadrature as the primary rule. The on-disk FLINT snapshot lacks
  acb_hypgeom documentation, so incomplete Gamma must not become an undocumented certification premise.
- Check the literal norm-1 split against the conductor-scaled theta split and independent L values.

Environment probe: timeout 20 python3 -B (imports and gamma_upper docstring), exit 0.
mpmath 1.3.0, sympy 1.14.0, python-flint 0.8.0, bundled FLINT 3.3.1 are importable.
The bundled version is not the project's C FLINT 3.0.1.

Draft api-5: 402 lines before the final small proof edit; no line exceeded 116 characters.
The oracle reached 434 checks, exit 0, including independently certified Taylor quadrature,
completed FLINT comparisons, 24 distinct wrong-formula witnesses, all 17 root-number goldens,
and the norm-1/Poisson seam. Added explicit finite local poles/residues and printed width witnesses next.

Development failures were in this lane's oracle:
1. The odd real gamma test at s=2 tried to evaluate a genuine pole as a finite value. It now checks the pole.
2. The radius-loss witness used too small a displacement for its asserted numerical separation. Replaced
   it by the zeta values at 1.125 and 1.25, with separation greater than 1.
3. The golden parser returns endpoint data, not an acb. Tests now compare exact dyadic endpoints.
4. A truncated Poisson sum's midpoint was compared at 1e-45 although its certified tail was larger.
   The check now tests the specified enclosure against the independent reference and checks its width.
No mathematical statement or golden was changed. The six faults per package still all fail their targets.

No new mathematical counterexample found. The brief's pole list applies only to trivial local integral
factors. Odd integral and gamma poles differ. The (8,7) golden is lowered to conductor 4, as required.

Final oracle: 466 checks, exit 0, 6.73 seconds wall time, 76524 KiB maximum RSS, one FLINT thread.
Of these, 462 are lane checks and 4 are inherited analysis assertions, reported separately.
Reference functions were replaced by raising stubs while continuation ran; two functional-equation
calls were replaced by distinct sentinels to check that neither side is copied from the other.
The final design has 409 lines, at most 109 columns. The oracle has 7 required entry points.
The report is written after the final artifact check. No src/include file, SPEC, convention or golden changed.
