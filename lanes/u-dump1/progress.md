# lanes/u-dump1: progress

## ucoset, idele, idclass (tests/test_dump_units.c)

RED (link error, the accepted first failure of a file):

    $ timeout 600 make -j2 build/test_dump_units
    ... undefined reference to `adf_ucoset_load_str' ... `adf_idele_load_str' ...
    collect2: error: ld returned 1 exit status

GREEN, after the code at the end of src/dump.c and the declarations in include/adelefeld/dump.h:

    $ timeout 300 ./build/test_dump_units
    9 tests, 35106 checks, 0 failed checks, 0 failed tests

Counts of the file: 20 rows of tests/golden/dump.tsv with one of the three bodies (7 valid),
2000 random values of each of the three types in the round trips, 95 texts in the three
strictness tables, one table of limits and of the order of the checks of 8.5.

The 95 texts of the strictness tables were also run through the reference `proto/text_grammar.py`
(`dump_load_check`): 95 texts, 0 mismatches. The oracle is the reference, not the output of this
library.

## lball, sball (tests/test_dump_local.c)

RED. The code of the five bodies was written in one pass, so the file could not fail to link. The
red run was made by a deliberate defect: the predicate of 5.8 in `dp_w_body` (case `DP_LBALL`)
was switched off, the file rebuilt, and the test failed:

    $ timeout 600 ./build/test_dump_local
    FAIL ...: load "adf1 Q lball 4 b 3 0 4": OK, expected DOMAIN
    FAIL ...: load "adf1 Q lball 5 b 1c 0 2": OK, expected DOMAIN
    7 tests, 35971 checks, 40 failed checks, 3 failed tests

(the defect was then removed and the run repeated)

GREEN:

    $ timeout 300 ./build/test_dump_local
    8 tests, 35978 checks, 0 failed checks, 0 failed tests

Counts of the file: 22 rows of tests/golden/dump.tsv with one of the two bodies (9 valid),
2000 random values of each of the two types in the round trips, 67 texts in the two strictness
tables, the limit and order tables, and one test for a survivor of the mutation run.

The 67 texts of the strictness tables were also run through the reference `proto/text_grammar.py`:
67 texts, 0 mismatches.

## driver

`adf_drv_body_slot` (tools/adf/adf.c:521) now names the nine bodies the driver has a value for,
and the dumper, the inspector switch and the loader switch of `adf_drv_load` have the five new
cases. Two new fixtures, `tests/driver/u-dump-units.cmd` and `tests/driver/u-dump-local.cmd`,
with the expected output written by hand from conventions 10.1, 5.6 to 5.9 and 9.4:

    $ timeout 60 ./build/adf < tests/driver/u-dump-units.cmd   # exit 1, 38 lines, equal to the .out
    $ timeout 60 ./build/adf < tests/driver/u-dump-local.cmd    # exit 1, 39 lines, equal to the .out

Five existing fixtures answer differently now, one line each; they are listed in the report.

## mutation testing

Three runs of `tools/mutate/mutate.py` over `src/dump.c`, 40 mutants each, `--san`, `INV=1`,
the six dump test programs. The last run: 40 mutants, 34 killed, 1 survived, 5 not compiled
(`lanes/u-dump1/mutate.txt`). The survivor is listed in the report; two gaps the runs found
were closed (the branch `t <= 64` of `dp_arb_sign` was dead and is gone, the test of the
first letter of the field and the test of the stage of the `qclass` check were added).
