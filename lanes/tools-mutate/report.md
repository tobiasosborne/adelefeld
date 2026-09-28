# Lane tools-mutate: report (Claude sonnet; summary by the orchestrator of the final message of the lane)

Issue adf-obp. Files changed: `tools/mutate/mutate.py`, `selftest.py`, `equivalent.txt` (header comment
only), `example/` (Makefile, sources, strong test, a `status.h`).

## Causes of mutants that did not compile, measured with the old tool (seed 20260928, all mutants run)

| kind | adele.c | cap.c | fball.c | rat.c | cause |
|---|---|---|---|---|---|
| op `&` | 61 | 0 | 0 | 0 | unary `&` read as binary |
| op `*` | 0 | 0 | 14 | 0 | pointer declarator or dereference read as binary |
| swap_args | 14 | 2 | 1 | 2 | the output argument exchanged with a const input |
| drop_assign | 2 | 5 | 11 | 0 | unused parameter or uninitialised read under -Werror |
| gcd_lcm | 0 | 1 | 7 | 0 | renamed to a bare name that is not declared |
| logic | 0 | 0 | 2 | 0 | -Werror=parentheses; not fixed |
| total | 77 | 8 | 35 | 2 | |

The splitting of `&&` and of `->` reported in the issue did not reproduce.

## Changes

Unary `&`, `*`, `-` and pointer declarators are not mutated as binary operators; `gcd_lcm` renames inside
the identifier and only to a declared name; `swap_args` never moves argument 0 of a call with three or more
arguments; new kinds `status`, `drop_call` (not for `_init`, `_clear`), `call_swap`, `prec`; every run works
under `<scratch>/run-<pid>/` so that two runs sharing a scratch path do not overwrite each other (reproduced
as a crash with the old tool). `selftest.py`: 21 checks of the generation rules (red with the old tool, green
with the new), and a test of two runs at the same time.

## Counts before and after (mutants / killed / survived / not compiled / excused)

| file | before | after |
|---|---|---|
| adele.c | 90 / 13 / 0 / 77 / 0 | 144 / 90 / 29 / 25 / 0 |
| cap.c | 29 / 21 / 0 / 8 / 0 | 48 / 39 / 3 / 6 / 0 |
| fball.c | 184 / 146 / 0 / 35 / 3 | 347, 200 run / 155 / 20 / 24 / 1 |
| rat.c | 8 / 6 / 0 / 2 / 0 | 36 / 20 / 2 / 14 / 0 |

The three excused keys of fball.c are unchanged; two of them were not drawn into the sample of 200.

## Survivors (54): findings for the owners of the tests

- Commutative swaps, equivalent: fball.c 54, 214, 552, 555, 684; adele.c 201, 202, 223, 224, 259, 260, 285,
  450, 451, 464, 465, 492, 493, 514; cap.c 78, 112, 148; rat.c 222, 238.
- fball.c, 15 `drop_call` in init, zero, one, set, set_si, set_fmpz, set_rat, clear (lines 142, 155, 259,
  260, 261, 264, 272, 273, 276, 284, 285, 297, 300, 308, 312): no test calls a constructor on a value that
  holds other data before.
- adele.c, 12 `prec` (256, 259, 283, 285, 318, 319, 489, 492, 512, 514, 543, 544): no test sees whether the
  precision argument is used.
- adele.c 82, 360: `adf_fball_swap` dropped in the two swaps; the finite half of a swap is not tested.
- adele.c 179: `ADF_OK` of `adf_adele_get_arb_at` replaced by `ADF_DOMAIN`; the status is not tested.

## Not done

The `logic` cause; a table of causes for the mutants that still do not compile; `tests/README.md` does not
list the four new kinds.

## Note of the orchestrator

The different survivors in repeated runs of lane m1-recon were sequential runs in one worktree, so the
shared scratch path does not explain them. `tests/test_recon.c` had five uninitialised `fmpq` (fixed in
`cb268ba`) and crashed in some runs, which turns a surviving mutant into a killed one at random. That is
the likely cause; it is to be confirmed by repeated runs on master.
