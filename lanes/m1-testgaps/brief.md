# Lane m1-testgaps: tests for the survivors of the repaired mutation tool

Read `lanes/COMMON-C.md`, `lanes/tools-mutate/report.md` (section "Survivors"), `tests/README.md` (mutation
testing), `include/adelefeld/fball.h`, `adele.h`, `rat.h`, `src/fball.c`, `src/adele.c`, `src/cap.c`,
`src/rat.c`, `src/recon.c`, `tools/mutate/equivalent.txt`.

**You own:** `tests/test_fball_overwrite.c`, `tests/test_adele_prec.c` (both new),
`tools/mutate/equivalent.txt` (append only; never change or reorder a line that is there),
`lanes/m1-testgaps/`. Sources under `src/` and existing tests are read-only.

1. `tests/test_fball_overwrite.c`: every constructor and setter of `fball.h` (`init` is excepted, it has no
   earlier value; `zero`, `one`, `set`, `set_si`, `set_fmpz`, `set_rat`, `set_fmpz3`, `set_center_radius`
   and what else the header declares) is called on a value that holds other data before (a ball with large
   `A`, `H > 0`, `d > 1`), and afterwards every field is compared with the expected one and
   `adf_fball_is_canonical` holds. This must kill the `drop_call` survivors of lines 259 to 308 of
   `src/fball.c`.
   The survivors that drop `flint_free(x->res)` (lines 155, 264, 276, 300, 312) need a value with a residue
   array, which only the local backend makes; it does not exist yet. Do not build such a value by hand.
   List these five in the report as open until work package 1.8.
2. `tests/test_adele_prec.c`: for add, sub, mul, add_rat, mul_rat, div_rat of `adf_adele` and `adf_cadele`
   and for `set_rat`: the result at `prec = 200` has a radius below `2^-150` relative to the midpoint for
   operands that are exact at 200 bits but not at 2 bits (so a mutant that passes `2` for `prec` fails), and
   the result at `prec = 2` still contains the exact value. The finite half of `adf_adele_swap` and
   `adf_cadele_swap` is tested with two different finite parts and equal real parts. The status of
   `adf_adele_get_arb_at` at the real place is `ADF_OK`.
3. Red-green: for each new test show that it fails against the mutant it is written for (apply the mutant
   by hand in a scratch copy of the tree under `build/`, never in `src/`), log in
   `lanes/m1-testgaps/redgreen.log`.
4. Equivalent mutants: the 24 commutative swaps listed in the report. Check each line number against the
   current source, check in the FLINT documentation (`refs/src/flint-3.0.1/*.rst`) or in our header that
   aliasing of the output with an input is permitted for that call, and append one line per mutant to
   `tools/mutate/equivalent.txt` in the format of the lines that are there.
5. Then run, one after the other (never two at once), `make mutate FILES=src/<f>.c JOBS=2 LIMIT=400` for
   `rat.c`, `cap.c`, `recon.c`, `adele.c`, `fball.c`; run the one for `recon.c` three times and say whether
   the three verdict lists are identical (compare the full output). Report the counts and every survivor
   that remains, with the test that is missing. If a run would take more than 25 minutes, stop it and say so.
6. `make -j2 check` and `make clean && make -j2 check SAN=1` pass.
