# Red and green runs of lane t5-slice4

Build: `make -j2 BUILD=lanes/t5-slice4/build lanes/t5-slice4/build/test_tate_continue`; run from the root with
`timeout 300 lanes/t5-slice4/build/test_tate_continue [section]` (a section name runs that section alone).

1. RED (compile). `tests/test_tate_continue.c` written in full before any code: `adf_tate_continue` and
   `adf_tate_completed` undeclared (the first test of a new file; a compile error counts once).
2. RED (assertions). Header declarations, the shared body `tt_run` with the mode gate (`tt_domain`), the
   continuation `adf_tate_continue`, T4 (`tt_recip`), the hook `adf_tate_trace`; `adf_tate_completed` a stub
   returning NOT_DETERMINED. `test_tate` 22904 checks green (regression). New test by section: `forbidden` green;
   `statuses` failed at the T4 rectangle `1 + 2^-30 (1 + i) +/- 2^-100` with continue (a test error: the image
   radius sqrt(2) 2^-41 is above the target 2^-41; radius changed to 2^-110 with the computed floor in a
   comment); `overlap` :480, `widths` :597, `values` :245 (completed q=1 s=1/2 status 1), `balls` :322, `poles`
   :344 (completed at exact 0 not DOMAIN), `residues` :368: all assertion failures of `adf_tate_completed`.
   `cutoffs` (27 records, Lambda mode through the hook) and `sides` (6) green at once: they test the attempt,
   which the 5c code already had.
3. GREEN. `adf_tate_completed` = `tt_run(..., TT_COMP)` (Lambda budget, no factor C^-z, width on Lambda): every
   section green; the whole program in one run (count in the report).
4. RED (driver). `tests/driver/tate-continue.cmd` and `.out` written by hand (derive_fixture.py) before the driver
   code: `sh tests/test_driver.sh` fails, every new line `error: PARSE` (unknown command).
5. GREEN (driver). The two commands in `tools/adf/adf.c`: `sh tests/test_driver.sh` 98 cases, 101666 lines, all
   equal, on the first run after the code.
6. RED (Julia). `tests/julia/tate_continue.jl` (expected texts from derive_fixture.py) against a shared object built
   from the HEAD `src/tate.c`: `could not load symbol "adf_tate_completed"` (error in the test set).
7. GREEN (Julia). Against the shared object of the lane's sources (with the libgmp `LD_PRELOAD` of
   `test_julia.sh`): 12 of 12 pass on the first run.
8. Faults, first run (`faults.py`): 11 of 12 detected; "discarded n tail" (E_n not added) survived, and the
   open-rectangle pole test was detected only through a test added for it. Added: (a) in `statuses`, balls in
   end-point contact with a pole at bits 2^21 must be NOT_DETERMINED (the pole geometry comes before the work;
   an attempt would end in LIMIT); (b) in `cutoffs`, the oracle's bound E_n + E_t + both remainders (a new field
   `bound` of the cutoff records) must lie on the imaginary radius of Lambda for real s (18 records). Both green
   against the code at once (they pin what the code already did); red against the two faults: second run 12 of 12
   detected (`faults.out`).
9. Mutation (60 mutants, 15 survivors; the report lists them). Two were gaps: 388 (`nz == 0` to `nz == 1`: one
   nonzero coefficient, no remainder) and 579 (the transform charge `C * C` to `C + C`). Added: zeta values also at
   bits 4 (N = 1 there), and C = 1024 refused by the charge within 0.5 s. Green against the code at once; both
   mutants killed on the re-run (`resurvivors.out`: :247 and :607).
10. valgrind (sections, -O1) reported uninitialised bytes in the test's own sentinel byte comparison (`sent_check`,
    the unused limbs of a fresh acb); the sentinel is now zeroed before `acb_init` (the convention of
    tools/memcheck/README.md). Final: `tate_continue: 24261 checks` (plain, SAN, clang), 24271 (INV).
