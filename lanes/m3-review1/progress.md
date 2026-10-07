# m3-review1 progress notes

- Builds: build/ and build-san/ (SAN=1 INV=1) rc=0.
- Sign derived from refs/src/tate-poonen/notes.txt:693-700: psi_R(x)=e^{-2 pi i x}; psi_Qp = Qp -> Qp/Zp ~ Z[1/p]/Z -> R/Z -> T,
  psi(1/p^n)=e^{2 pi i/p^n}. So psi_p(x)=E(fp_p(x)), psi_inf(x)=E(-x); (0 ; 1/3) -> E(1/3). Matches conventions 6.1:844.
- Static reading of src/psi.c: b=(a-m) mod 1 (sign right); Q4 distances; hull fold min/max over entries.
- Static reading of src/qclass_sets.c, src/qclass_arith.c: no defect seen on reading; to be tested.
- Hunt 1 (sets): sets_check.py seeds 1-7, 10000 pairs x 3 queries; 0 findings (LIMIT expectation from own K,L,E;
  truths from own residue/point decision; witnesses re-checked through common.member). edge_sets.py: B=10^100 and
  width 2^1001 refused LIMIT, 9 calls in 2.8 ms total; W up to LONG_MAX; glued family H=2..8 all centres: 70/70.
- Character: psi_check.py seeds 1-6: 6000 cases (LIFT and PIECES), ~3.5M point phases, ~17000 hull checks; 0 findings.
- Hunt 2 (arith): arith_check.py seeds 2-7: 3000 pairs, add and neg each; stored pieces bit-identical to own construction
  (sum/gcd/neg, R, Q1 kernel) on every OK; LIMIT exactly when own K > lim; ~1.04M membership points; z=x=y alias
  identical; 0 findings.
- Local/place character: psi_local.py seeds 1-5: 5000 lball, 5000 psi_at (where checks), 1250 product-over-places; 0.
  E3: lift strict ND untouched, default box real [-1.0000000019,1.0000000019], imag 0; reduction strict OK.
- psi_check.py seed 11 with local backend (ctx 8,9,5) pieces: 1000 cases, 0.
- Hunt 3: uni_check.py seeds 1-11: 5500 union texts, max_items count/count-1, reread of get_str; 0. sp.c probes: OK.
- Hunt 4: dump_check.py seeds 1-7: 7000 classes round trip, ~20000 mutants, interposed FLINT string loaders; 0.
- Hunt 5: san+INV harness with detect_leaks=1 on all checkers (5000 sets pairs, 5000 texts, 5000 dumps, 5000 arith
  pairs, 4800 psi, 2000 lball/psi_at); lanes' 7 test programs under LSan: rc 0, no reports.
- Hunt 6: drv.txt 61 command lines; 59 equal to hand values, 2 differ only in my guesses (2.5 is PARSE; printed radius).
- Hunt 7: faults.py F1-F16, all detected by the lanes' tests (tests that #include ../src/X.c need the fault in that copy).
- MINOR: phase_get_acb normal build, theta 5/4, -3/4, 9/4 -> -i (true +i); 2/4 -> -i (true -1). Precondition (INV aborts).
- MINOR: stale contract text: text.h:185,198; tools/adf/README.md:735,786; api-3a.md duplicate numbers 19-24.
