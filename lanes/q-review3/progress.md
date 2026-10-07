# q-review3 progress (Claude Opus), 2026-10-07

- pi's files moved to old-pi/ (not used: the model there was unverified; harness rewritten).
- Built build/ and build-san/ (SAN=1 INV=1) archives.
- harness.c: exact input (radius via mag_set_ui_2exp_si, checked equal), prints stored pieces exactly.
- model.py: own Q1, own algorithm R, exact containment in A/Q without R (coset refinement), point membership.
- selftest.py: dropping/shrinking a stored piece is detected by the containment oracle where not redundant.
- hunt 1/2 lifts: seeds 101.. 2000 cases each, 0 findings so far.
- hunt 3: logs/hunt3-resource.txt; all statuses as documented. Notes: prec = cap gives LIMIT for a
  non-dyadic piece (bound on midpoint bits); count within limit but huge -> flint_malloc abort
  (SIGABRT, 0.22 s) for count 999999997 with limit 10^9, also when deduplication leaves 3 pieces.
- hunt 4: 2000 values, printer equal to own printer of 9.4/9.5 in all; 0 enclosure failures.
- reduce of reduce grows the piece count (spill of Q1 successor) - observation.
- hunt 1/2 done: 20000 lifts (seeds 101-110), 4500 PIECES (201-203), 988 re-reductions (301-304): 0 findings.
- hunt 5: 4991 cases under ASan/UBSan/LSan + INV, and all 37 resource cases: nothing reported.
- hunt 6: 60 driver lines, all as derived by hand; SAN driver same output, no report.
- hunt 7: 13 faults; lane test misses F2 (loses enclosure, wrong status) and F8 (y written on a
  pass-1 LIMIT). Own checker detects all 13 (F8, F10 through resource cases).
- cleaned: build trees, binaries, fault/ scratch, old-pi/ (pi's unverified files, superseded).
