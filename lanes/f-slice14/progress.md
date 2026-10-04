# f-slice14 progress notes

- 23:41 read CLAUDE.md, lanes/COMMON.md, COMMON-C.md, brief, docs/design/local-zeta.md, proto/zeta_checks.py,
  SPEC 9.3.7 row (line 703), conventions 944-956, 223, 4.4; catalogue.md Proposition 9; sources
  refs/src/tate-poonen/notes.txt:62-64, :1014-1016, :1733 (refs live in the main checkout, not in the worktree:
  /home/tobias/Projects/adelefeld/refs/src, read-only); acb.rst 130-149, 224-313, 509-523, 658-673, 871-902.
- 23:43 fixtures regenerated:
  `timeout 170 python3 proto/zeta_checks.py --fixtures lanes/f-slice14/zeta-fixtures.jsonl`
  exit 0; 1377 rows (701 OK, 642 ND, 32 DOMAIN, 2 LIMIT), 5845 samples, 643 width witnesses; same counts as the
  design's section 6. File 11162934 bytes.
- Export list = include/adelefeld.h (tests/test_exports.sh reads declarations through <adelefeld.h>); one line.
- Plan: selection script lanes/f-slice14/select_fixtures.py -> tests/ref/vectors/f-slice14/zeta.jsonl (<= 600 KB);
  the C test can also run the full 11 MB file when ZETA_FIXTURES=path is set (not part of make check).
- Slices A, B green (see redgreen.md). Full 11 MB fixture file: 1371 rows with an input, statuses identical to the
  simulation, 701 OK, 5845 samples contained, 643 width checks at factor 64 all met.
- Findings so far: (1) acb_rising_ui on FLINT 3.0.1 is far wider than the product loop for wide balls (rec_probe.c);
  code uses the loop. (2) acb_gamma of the design's box is non-finite on 3.0.1 too (gamma_probe.c). (3) arf_get_mag
  and mag_set_fmpz_2exp_fmpz are not exact for a 30-bit dyadic (mag_exact.c; test-side only). (4) the oracle's
  absolute point error (1e-229) does not resolve components of modulus 1e-302 or 2^(-2^1000): 26 subset samples
  need the raised certificate of design section 4 (acb_pow at 4000 bits).
- Slice D: driver command and fixtures green on the first run; Julia green; README section; test_julia.sh lines.
- Slice E: Y16, Y17 appended to docs/api-1f9.md.
- Slice F faults (lanes/f-slice14/plant_faults.py, log faults.log): 10 of 11 planted variants caught; fault 1 alone
  (the k = 0 pole test) is not observable: the final finiteness check (acb_inv of a ball containing 0 is
  non-finite) still returns NOT_DETERMINED; with fault 7 added it is caught (OK with a non-finite ball).
- Mutation: minimal root lanes/f-slice14/build/mutroot (Makefile, include, src/{localfactor,place}.c and the two
  internal headers, tests/{support,test_runner.h,test_localfactor.c,ref/vectors/f-slice14}); one mutant takes about
  53 s with SAN=1 INV=1 (test 33 s under the sanitizers), so the limit is 40 mutants to stay inside 20 minutes.
- Mutation run 1 (mutate.log): 40 of 218 mutants, 541 s: 25 killed, 12 survived, 3 not compiled. Survivors 313,
  319, 329 (refinement off) were a gap: the subset held no row that needs the refinement. plant_faults.py R1 on
  the full file found 4 such rows (near_imaginary -4, -6, -8, -10, radius 10^-1); rule 7 of select_fixtures.py
  adds them (subset now 270 rows, 563960 bytes). R1, R2 then caught by the subset alone.
- Mutation run 2 (mutate2.log, same seed and set): 555 s: 30 killed, 7 survived, 3 not compiled.
