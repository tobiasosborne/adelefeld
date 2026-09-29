# Lane d-functions: running progress (for an agent that continues this lane)

Order of work: 1F.1, then 1F.3, then 1F.2, then 1F.4. Files: `docs/api-1f.md`, `proto/functions_checks.py`.

## State

- [x] Preflight: brief and `tools/orch/autosave.sh` exist; `refs/src` linked.
- [x] Read `docs/proofs/functions.md` (all), SPEC 4.1 to 4.4, 9, 10, 15.
- [ ] Read PLAN section 4 and 6, conventions, api-s.md, headers, review of s-design.
- [ ] 1F.1 designed
- [ ] 1F.3 designed
- [ ] 1F.2 designed
- [ ] 1F.4 designed

## Found so far

- `proto/functions_checks.py` EXISTS already (955 lines, 7.5 s, cited by `docs/proofs/functions.md` line 8 as
  the check program of its 22 statements). The brief calls it "new". Decision of this lane: the existing
  checks are kept untouched and the checks of the design are appended in a marked section at the end, so
  that `functions.md` keeps its checks. Goes into the report as a finding against the brief.

## 2026-09-29, after reading (before writing the design)

Read: functions.md, SPEC 4, 9, 10, 15, PLAN 4 to 7, conventions 2 to 5.9, 7, 9, 10, 12, api-s.md,
s-design review, place.h, status.h, common.h, adele.h, roots.h (accessors), padic.rst, arb.rst, acb.rst.

Facts found (all go into docs/api-1f.md section 6 or 7):

- conventions 5.8 and 5.9 already fix the structs of `adf_lball` and `adf_sball`; `adf_places_t` is named in
  conventions 7 and 2.2 but has no struct anywhere.
- No statement of functions.md gives: the projection of a finite ball to a prime; the local sum, product,
  inverse as exact balls; the decomposition of a BALL (only of a point, Prop 4); evaluation of an exact input
  to a requested precision. These are "Statements to add".
- FLINT probe (lanes/d-functions/probe/probe.c, output probe.out): padic_exp(3) = 958, padic_exp(12) = 5125
  modulo 3^8 (differ at digit 2); padic_exp with input known modulo 9 and output variable of precision 8
  returns 958 with N = 8; padic_log refuses 3 and -1 at p = 2; on refusal the output is untouched;
  arb_root_ui(-8, 3) and arb_root_ui(0, 3) are NaN; arb_sqrt(0) = 0; acb_log of a ball across the cut is a
  finite ball with imaginary radius 3.14.
- The source of FLINT's padic module is NOT on disk (refs/src/flint-src-3.0.1 has no padic directory):
  source pending. arb_hypgeom.rst is not on disk: erf and Bessel are source pending.
- Sources found for two statements marked pending in functions.md: Teichmueller representative,
  conrad-hensel:hensel.txt:187-188 and pari-doc:usersch3.tex:16487-16488; the convention Log(p) = 0,
  pari-doc:usersch3.tex:16235-16237 (the name "Iwasawa" is still without a source).

Next: write Part 2 of proto/functions_checks.py, then docs/api-1f.md section by section.
