# Lane m1-text: report (Claude opus; saved by the orchestrator from the final message of the lane, shortened
# in wording, not in content)

Worktree based on `948e1b5`; the lane read its brief with `git show a03f72d:lanes/m1-text/brief.md`.

## What was done

Every function of `include/adelefeld/text.h` in `src/text.c` (1489 lines): `adf_text_classify`, and
`set_str`, `get_str` of `adf_rat`, `adf_fball`, `adf_adele`, `adf_cadele`.

- Parser by hand over `(s, len)`; no `strlen`, `strtol`, `sscanf`, `atoi` on the input; no raw input to a
  FLINT string function; a digit string is copied into its own buffer after stages 3 and 4 accepted it, then
  `fmpz_set_str`; `arb_set_str` and `arb_load_str` are not called; no recursion.
- Order of checks (conventions 8.5): 1 `len > max_len`; 2 alphabet; 3 grammar; 4 `max_exp10` on the exponent
  digit strings (an exponent of more than 18 significant digits is over any limit, as in the reference);
  6 zero denominators. Stages 5 and 7 cannot occur for these types.
- Output built in temporaries and swapped in on `ADF_OK` only.
- Reading a real ball: exact decimal `m`, `r` in integers; midpoint `t 2^e` with `t` of exactly
  `p = max(prec, 2)` bits; the rounding error added exactly to `r`; one rounding up to a `mag` with
  `mag_set_ui_2exp_si`. Exact when `m` and `r` are dyadic and fit.
- Printing: conventions 9.5, unconstrained case, exact `fmpq` arithmetic.
- Classifier: recognisers for the 13 start symbols of 9.2.
- `tests/fuzz/fuzz_text.c` includes `src/text.c`, so that the file is compiled with sanitizer and coverage.

## Files

`src/text.c`; `tests/test_text_rat.c`, `test_text_fball.c`, `test_text_adele.c`, `test_text_classify.c`;
`tests/fuzz/fuzz_text.c`; `tests/fuzz/corpus/text/` (283 seeds); `tests/ref/vectors/m1-text/` (12355 records
in 7 files); `bench/bench_text.c`; `lanes/m1-text/gen_text_vectors.py`, `seed_text_corpus.py`,
`redgreen.log`, `fuzz.log`, `mutate.log`.

## Checks (by the lane)

| Command | Result |
|---|---|
| `make -j2 check` | 13 programs pass; text_rat 13 tests / 18122 checks; text_fball 10 / 21545; text_adele 20 / 84400; text_classify 8 / 20776 |
| `make clean && make -j2 check SAN=1` | pass, no report |
| `make clean && make -j2 check CC=clang` | pass |
| `make fuzz FUZZ_TARGET=text FUZZ_SECONDS=170` | no crash; 1334484 executions; corpus 445 |
| `make mutate FILES=src/text.c JOBS=2 LIMIT=150` | 150 mutants, 1264 s: 62 killed, 13 survived, 72 not compiled, 3 timed out |

Golden rows, all run at prec 128: rat 51, fball 68, adele 69, cadele 16, dispatch 29, realball_read 30,
realball_print 26. Coverage of `src/text.c` over the fuzz corpus: lines 1011, 11 missed; branches 638, 18
missed. Not reached: line 66 (`len > max_len`; inputs are at most 4096 bytes), 1080-1083 (`flint_abort`,
exponent beyond a word), 1422-1423, 1437-1438, 1449-1450 (later list item of `union`, `ffun`, `rfun` that
fails; reached by unit tests).

Orchestrator, in the worktree: `make clean && make -j2 check SAN=1`: 13 programs pass.

## Mutation survivors (13)

- 1328 zero_one: killed afterwards by the new test `signs_and_denominators_of_the_other_literals` (checked by
  hand; run not repeated).
- 374, 377 zero_one (`+ 1` for the NUL byte): one-byte heap overflow; invisible to plain `make check`, found by
  the address sanitizer (checked by hand).
- Proposed as equivalent: 100 cmp (two columns), 114 cmp, 713 zero_one, 753 cmp, 753 zero_one, 804 zero_one,
  889 cmp, 889 zero_one, 1096 zero_one; reasons in the final message (keyword followed by Z or z is PARSE
  either way; "+/-" never ends a sentence; exponent 0 gives the same numbers in both branches).
- Timed out: 157, 321 (negated loops), 377 (`cap *= 2` to `/=`).
- Tool: a killed mutant is counted as not compiled when the name of the failing test contains `error:`
  (`tools/mutate/mutate.py:43`).

## Bugs found by the tests during the work

1. `mag_set_fmpz_2exp_fmpz` adds one unit to a 30-bit mantissa; `1 +/- 0.25` was not read exactly.
2. The first estimate of the midpoint exponent could be one too high for a decimal not in lowest terms.

## Not done

Constrained printing (9.5, k >= 2). A local `adf_fball` prints `(* ; 0)` until work package 1.8 exists (no
test). No floor for the benchmark rows. Mutation run not repeated after the added test.

## Header findings

1. `text.h:32` "a printer never fails": the printer of 9.5 costs time and memory that grow with the binary
   exponent of the `arb`; an exponent beyond a word aborts; an exponent like 2^30 runs out of time or memory.
   The header offers no way to refuse.
2. `prec < 2` is not defined by the header for `adf_adele_set_str`, `adf_cadele_set_str`; implemented as
   `max(prec, 2)`.
3. `digits` outside 1..10^6 is a precondition and is not checked.

## Findings against the conventions

1. 11.3 item 4 "set the arb exactly": `arf_get_mag` and `mag_set_fmpz_2exp_fmpz` are not exact at 30 bits;
   `mag_set_ui_2exp_si` is. Matters for the dump loader.
2. 8.4: the reference refuses an exponent of more than 18 digits whatever `max_exp10`; conventions are silent.

## Sources pending (at the time of the lane; fetched by the orchestrator at 10:20)

`mag.rst` (`mag_set_ui_2exp_si`), `arf.rst` (`arf_get_fmpz_2exp`), `fmpz.rst` (`fmpz_set_str`).
