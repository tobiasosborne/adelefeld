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
