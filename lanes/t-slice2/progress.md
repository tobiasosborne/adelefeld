# lane t-slice2: notes as I go

Read: CLAUDE.md, lanes/COMMON.md, lanes/COMMON-C.md, docs/conventions.md 5.8, 5.9, 7, 8, 9 (all),
tests/golden/README.md, lball.tsv (55 lines, 52 vectors), sball.tsv (29 lines, 25 vectors),
include/adelefeld/{text,lball,sball,place}.h, src/text.c, src/text_idele.c, tests/test_text_idele.c,
proto/text_grammar.py (Parser.lcoord 359-372, Parser.sentry 382-394, _syntax lball 465-471 / sball 472-481,
_lcoord 702-714, _padic_centre 716-729, _fmt_lcoord 751-757, _check_limits 579-599, _check_unsupported 607-617,
_build_and_print lball 834-836 / sball 837-856), docs/api-2.md section 4 (how t-slice1 documented its
functions), tools/adf/adf.c 1660-1700.

Findings so far (before the code):

- `adf_sball` stores `acb_t inf` and the tag `arch` in {NONE, REAL, COMPLEX}, so a complex `inf` entry of the
  golden file (`{inf: (1) + (2)*i}`) CAN be stored; the reader makes arch = ADF_ARCH_COMPLEX. The arithmetic
  functions of sball.h return ADF_UNSUPPORTED on a COMPLEX tag, but reading text is not arithmetic. No status
  is needed.
- No sign condition (9.3) applies to the archimedean component of a partial ball, so stage 7 (NOT_DETERMINED)
  does not occur for `adf_sball_set_str`; the real parts are read with the enclosing ball of 9.5 only.
- The C round trip is not a fixed point (conventions 9.6, gate finding G4): `{inf: 1.5 +/- 1e-9}` prints from C
  as `{inf: 1.5 +/- 1.01e-9}` (the radius 1/10^9 is rounded up to 30 bits, then to two digits). The golden
  comparison therefore compares the text exactly where the input is exact (dyadic within prec), as the adele
  test does, and compares the printed text with the reference otherwise (fixed point of the reference).
- The brief says the files are "55 lines" and "29 lines" of lball.tsv and sball.tsv; the README says 52 and
  25 vectors: 55 = 52 vectors + 3 comment lines, 29 = 25 + 4. No disagreement of the conventions and the
  golden files.
- src/text.c is read-only for this lane, so the reader and the printer copy the static machinery of text.c
  (cursor, literals, exact decimals, enclosing ball, printer of 9.5) into src/text_local.c, each function
  with a comment naming the origin. Reported under "what would be shared".
## The state after the first green run

10 tests, 68852 checks, 0 failures, 0.15 s (`lanes/t-slice2/green1.out`). Three defects of the library were
found by the tests and repaired (details in `redgreen.log`): the centre of a ball printed as the unit, a
double ";" in the printer, and a trailing ";" accepted by the reader of a partial ball.

What the tests run:
- every row of tests/golden/lball.tsv (52 rows: 29 read, 23 refused) and of sball.tsv (25 rows: 11 read,
  14 refused), with the printed text compared with the reference character by character, the round trip, the
  classification, and the untouched output on every status;
- the stored fields of 12 hand-computed local balls (conventions 5.8);
- the complex tag, the canonical order of places, `{}`;
- 2000 generated texts from `lanes/t-slice2/vectors_local.jsonl` (lanes/t-slice2/gen_vectors.py, the reference
  proto/text_grammar.py): 1000 of each kind, 1195 of them a status, 805 valid; the text of every valid one is
  compared with the text of the reference where the real part is read exactly, otherwise the stored ball
  contains the exact interval of the text (the parts are in the record), the printed interval contains it,
  every local entry is printed as the reference prints it, and the printed text is read back;
- 2000 generated local balls and 2000 generated partial balls: print, read, identical fields;
- hostile input: a prime of 100 digits, an exponent of 100 digits, N above ADF_LBALL_EXP_MAX and at it, "O(5^"
  cut at every position, missing and wrong brackets, a NUL byte at every position of three texts, s = NULL and
  len = 0, whitespace only, every prefix of three valid texts, 10000 entries against max_items and max_len,
  max_prec and max_exp10 boundaries;
- the seven stages of conventions 8.5 in order, with two faults in one text;
- the printer bounds: the centre that does not fit (NULL), M1-D6 on a real part (NULL), digits 4 and 20.

## The faults of brief item 4 (`lanes/t-slice2/bite.py`, one run per fault)

`python3 lanes/t-slice2/bite.py <name>` regenerates `src/text_local.c` from the tail, applies one literal
replacement, builds into `lanes/t-slice2/build-bite`, runs `tests/test_text_local.c`, and regenerates the file
again. Seven faults, all of them detected:

| name | the fault | the run |
|---|---|---|
| sign | the sign of `N` in `O(p^N)` is dropped | 3 failed tests, 3648 failed checks |
| order | the components of a COMPLEX value are put in the array in the reverse order | 1 failed test, 4 failed checks (`sball_the_complex_tag`) |
| commit | the guard on the commit of the value is dropped (the output is written on every status) | 1 failed test, 1 failed check (`hostile_input`) |
| prime | the primality test of the prime is dropped (`p = 4` is accepted) | 5 failed tests, 8519 failed checks |
| base | the base inside `O(...)` is not compared with the prime | 3 failed tests, 6 failed checks |
| items | the item limit is not applied to a partial ball | 2 failed tests, 5 failed checks |
| centre | the centre of a ball is printed as the stored unit `u` instead of `p^v u` | 7 failed tests, 7818 failed checks |

The "order" fault is only visible at the tag `COMPLEX`: `adf_sball_set_arb_lballs` sorts the components itself
(decision u-7 of `docs/api-1f.md`), so the explicit `qsort` of the reader is redundant for the other two tags.
A first attempt (dropping the `qsort` altogether) changed nothing and was replaced; that is recorded because it
is a fact about the code, not about the tests.

## The end of the lane

- `docs/api-1f.md`: the section "Slice 1F.5-c" at the end (functions, decisions u-1 to u-8, the statements
  L14 and L15, what is not done, findings).
- `lanes/t-slice2/bite.py`: the seven faults, all detected (numbers in the report).
- `check-all` at the end: `check-all passed: make check, driver, exports, julia, mutate-selftest,
  memcheck-selftest`, 75 test programs, 0 FAIL lines, 4 min 16 s.
- `SAN=1` build of `test_text_local` and its run with `ASAN_OPTIONS=detect_leaks=1`: 10 tests, 68868 checks,
  0 failures, no leak (one leak of the test file was found there and repaired).
- `lanes/t-slice2/report.md` is written.
