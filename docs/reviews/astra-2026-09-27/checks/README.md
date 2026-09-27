# Checks for the review

All files here are review artifacts. No project implementation or input document was edited.

Evidence labels in review.md distinguish proof, runs/source inspection, and unverified memory. These commands were run from the repository root:

```sh
python3 proto/precision_rules.py > docs/reviews/astra-2026-09-27/checks/prototype.txt
python3 docs/reviews/astra-2026-09-27/checks/math_checks.py > docs/reviews/astra-2026-09-27/checks/math_checks.txt
gcc -O2 docs/reviews/astra-2026-09-27/checks/flint_probe.c -o docs/reviews/astra-2026-09-27/checks/flint_probe -lflint -lmpfr -lgmp
./docs/reviews/astra-2026-09-27/checks/flint_probe > docs/reviews/astra-2026-09-27/checks/flint_probe.txt
gcc -O2 -S bench/baseline.c -o docs/reviews/astra-2026-09-27/checks/baseline_O2.s
```

- `prototype.txt`: original regression output; 6000 random operations passed.
- `math_checks.py` / `.txt`: 20,000 scaled-policy enclosure checks; fixed-absolute-cap counterexample; full-adelic versus modular reconstruction distinction; diagnostic DFT normalisation checks; arithmetic for space counts. The Python complex DFT is not certified numerics.
- `flint_probe.c` / `.txt`: installed type sizes; non-exact arb conversion of 1/3; xi/completed-zeta distinction at s=3; actual baseline operand sizes; malloc usable sizes; accessibility of RDTSCP. This is not a core-clock calibration or an arithmetic benchmark.
- `environment.txt`: compiler/kernel/CPU report and selected installed FLINT declarations. The hardware report is guest-visible WSL2 data.
- `baseline_O2.s`: assembly from the unchanged benchmark source, used only for dependency inspection. The original timing experiment was not rerun; historical timing numerators are taken from the user-supplied dated output.
- `retrieval.txt`: failed shell network downloads (DNS restriction). No full-source-download success or source hashes are claimed. Primary web sources were inspected through the web tool and are linked at the relevant claims in review.md. Tate's §2.2 text was checked; Hertogh's authored documentation/README was checked, but the entire thesis was not read.

Online primary sources inspected: Tate's original scan at Rutgers; the author's Hertogh package documentation and repository; uops.info's exact ADD/MUL/ADC/VPMULUDQ forms for Zen 2; AMD's 3970X announcement; Intel's instruction manual; PARI's bnfcertify documentation; Harvey–van der Hoeven's Annals paper landing page. Local FLINT headers, rather than inaccessible current web documentation, determine the 3.0.1 ABI observations. The review explicitly leaves fast-division/half-gcd references and the exact immediate IMUL latency unverified from memory.
