# Result of lane t-slice1: the value form of unit cosets, ideles and classes, and the driver (milestone 2)

Worktree at commit 8c0f789, `lanes/t-slice1/brief.md` and `include/adelefeld/idmap.h` present, `refs/src` linked.
Times below are read from `date` (start 02:59, last check 04:03, 2026-09-30). Nothing was committed (no git command
that changes state, no `bd`).

## What was done

1. **Header.** `include/adelefeld/text.h`: three new includes (`ucoset.h`, `idele.h`, `idclass.h`) and a block with
   `adf_ucoset_set_str`, `adf_ucoset_get_str`, `adf_idele_set_str`, `adf_idele_get_str`, `adf_idclass_set_str`,
   `adf_idclass_get_str`; the statuses, the canonicalisation, the order of the stages, the `prec` rule and the
   real-ball rule are in the comment block.
2. **Code.** `src/text.c` (new section at the end, about 500 lines: the reader for the three forms, the constrained
   printer of conventions 9.5, the printers, two hidden functions `adf_tx_read_unit_form` and `adf_tx_write_unit_form`;
   the beginning of the file got one include and one paragraph of comment; no existing function changed).
   `src/text_idele.c` (new, the six public functions, built on the two hidden ones and on the constructors of
   `ucoset.h`, `idele.h`, `idclass.h`, so that the output is committed only on `ADF_OK`).
3. **Tests.** `tests/test_text_idele.c` (13 tests, 73457 checks). Oracles: the three golden files, every row (38, 24, 15
   rows: status, value, printed text, classification, stored unit, exact interval); 2200 generated and mutated texts of
   the three kinds against `proto/text_grammar.py` (841 valid), each valid one read at prec 128, 2, 7 and 30 (308
   NOT_DETERMINED answers, each checked against the gap rule: OK is required when `e(hi) - e(lo) <= p - 1`);
   1500 dyadic balls against the constrained printing of the reference (real part compared character by character);
   hostile input (modulus of 100000 digits, a residue of 100000 digits, a content of 100000 digits, nesting, missing and
   swapped brackets, case and non-ASCII, a NUL byte at every position of six valid texts, each valid text cut at every
   position and with each byte deleted, length 0 and `s = NULL`, `max_len` and 1048576 / 1048577 bytes, `max_exp10`
   boundaries, prec below 2 and above `ADF_IDELE_PREC_MAX`, NOT_DETERMINED at prec 30 against OK at 64 and 128, the
   order of the stages); round trips (print then read encloses). Vectors: `tests/ref/vectors/t-slice1/*.jsonl`, written by
   `lanes/t-slice1/gen_vectors.py` (COMMON-C rule 2; the directory is not in the brief's list of owned paths).
4. **Driver.** `tools/adf/adf.c`: the three kinds are types of the driver; `show`, `type`, `mul`, `div`, `neg`, `equal`,
   `contains`, `overlaps` accept them where defined; new commands `inv`, `pow`, `powtight`, `norm`, `class`, `idele`,
   `hull`, `hullsimple`, `unitof`, `valuation`, `abs`; `div` of an adele by an idele. `tools/adf/README.md`: the operations
   table and a section "Ideles and classes". Cases `tests/driver/i-01-show-type` to `i-06-status` (6 scripts, 207
   expected lines written by hand before the driver code; re-derived by `lanes/t-slice1/check_driver_cases.py` with
   exact integers and rationals: 207 lines, 0 disagreements; with one line altered it reports it).
5. **Julia.** `tests/julia/text_idele.jl` (32 checks: parse two ideles from text, multiply, print the product, the
   statuses, unit cosets, classes, classify) and a block in `tests/test_julia.sh`.
6. **Fuzz target.** `tests/fuzz/fuzz_text_idele.c`, seeds by `lanes/t-slice1/seed_corpus.py` (writes under `build/`).
7. **Docs.** `docs/api-2.md` section 4 (functions, Statements P and Q, the decisions t-1 to t-12, what is not done).
8. `proto/text_grammar.py`: not changed (it was right for the three kinds; 2200 + 77 vectors agree with the C).

## Checks that were run (last line of each; all in the foreground)

The four `make` runs and `check-all` ran under `timeout 900`. `sh lanes/m1-headers/check_headers.sh` and
`SAN=1 sh tests/test_driver.sh` ran without the `timeout` wrapper: the tool of this session refused `timeout sh ...`
as too complex a command, and a plain `sh script` was accepted; both ended in seconds.

| Command | Last line |
|---|---|
| `make clean && make -j2 check-all` | `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest` |
| `make clean && make -j2 check SAN=1` | `check passed: all 71 test programs` |
| `make clean && make -j2 check CC=clang` | `check passed: all 71 test programs` |
| `make clean && make -j2 check INV=1` | `check passed: all 71 test programs` |
| `sh lanes/m1-headers/check_headers.sh` | `check_headers: passed` |
| `SAN=1 sh tests/test_driver.sh` | `test_driver: 41 cases, 100726 expected lines, all equal (SAN=1)` |

Also: `sh tests/test_exports.sh` (inside check-all): 407 of 407 declared functions exported, no exported name undeclared (the
two hidden functions do not appear); `python3 lanes/t-slice1/check_driver_cases.py`: 207 lines, 0 disagreements.
Each `make` run finished in 2 to 8 minutes (the tool's 120 s limit was lifted with its own timeout argument, so nothing
ran in the background except the 120 s fuzz run, which the tool moved to the background and which I waited for).

## Red and green

`lanes/t-slice1/redgreen.log`. C tests: red against a stub at 03:10 (44107 checks, 9614 failed; assertions, not a link
error), green at 03:13 after two defects of the test (not of the code) were repaired; driver: the cases were written
first and `tests/test_driver.sh` failed at `i-01-show-type` with the old driver; green on the first run of the new driver
after four old cases were edited (see below).

## The tests bite (`lanes/t-slice1/bite.sh`, `bite.log`)

Seven faults in scratch copies under `build/bite/`: coprimality test dropped (953 failed checks), constraint of the
printer dropped (1022), fallback to kernel B removed (97), idele condition reduced to positive (560), unit printed as stored
(853), exponent limit not applied (SIGFPE), digits off by one in the printer (441). All seven are detected. Two earlier
faults survived and were replaced, and the reason is that the line is redundant: the reduction of the residue into 1..N
(`adf_ucoset_set_fmpz2` does it as well) and the refusal of a content 0 (`adf_idele_set_parts` refuses it with the same
status). Two more were replaced because one aborted in `adf_idele_kernel_defect` and one did not build.

## Fuzz (a smoke test, `lanes/t-slice1/fuzz.log`)

`fuzz_text_idele` (clang 18, libFuzzer, ASan and UBSan, 547 seeds): 3908379 runs in 121 s, no crash. The brief's
`ulimit -v 4000000` cannot be applied to a sanitized program (AddressSanitizer needs 15 TB of address space and stops); the
libFuzzer limit `-rss_limit_mb=2048` was used (peak 463 MB). The existing `fuzz_driver` with the new commands seeded: 27666
runs in 61 s, no crash. Both are smoke tests. No mutation run (brief item 4).

## Decisions (alternatives are in `docs/api-2.md` 4.4; the main ones)

- **The real ball of the readers** (t-2, Statement P): the enclosing ball of 9.5 first (tight for dyadic input); if it fails
  the sign condition, kernel B of `adf_idele_ball_from_ends` on the end points of the exact interval rounded outwards. So
  NOT_DETERMINED only when `e(hi') - e(lo') > p`, and a larger prec always succeeds (which conventions 9.3 promises and
  the enclosing ball alone could not keep: the radius of an `arb` has 30 bits, `1 +/- 0.99999999999` gives `1 +/- 1`).
  Alternatives: the enclosing ball only; kernel B only (not tight for dyadic input).
- **prec above `ADF_IDELE_PREC_MAX` is LIMIT** before the text is read, in the readers with a real part (t-1; the rule of
  `idele.h`). Alternative: no limit (as `adf_adele_set_str`, which allocates `prec` bits).
- **Where the code lives** (t-4): reader and printer in `src/text.c` (they need its static cursor, literals and printer),
  glue in `src/text_idele.c`, two hidden functions between them, the two declarations repeated in both files (no header
  file is in the lane's list). Alternatives: `#include "text.c"`, a new `src/text_internal.h`, a copy of the tokenizer.
- **Driver semantics** (t-5 to t-11): `neg` of an idele negates every coordinate exactly; `neg` of a class is DOMAIN (the
  class of `-x` is the class of `x`); `equal`, `contains`, `overlaps` only for unit cosets (the library has none for
  ideles, decision i3-1); `idele * rational` and `idele / rational` allowed (`adf_idele_mul_rat`), `rational / idele` DOMAIN;
  second operand of `valuation` and `abs` is a place (one token, else PARSE); `pow` exponent an integer that fits a word
  (not an integer: DOMAIN, beyond a word: LIMIT); `norm` and `abs ... with real` print the real ball as a real part.
  `dump`/`load` of the three kinds: UNSUPPORTED. Every other pair: DOMAIN.

## Files outside the list of owned paths that were changed

- `tests/driver/06_pairs.cmd`, `07_status.cmd`, `12_status_order.cmd`, `13_dump.cmd` (each `[5 mod 6]` that stood for "a kind with
  no typed parser" became `[p=5: 3]`; comment lines adjusted; the four `.out` files unchanged). This follows from the
  brief itself (the driver accepts unit cosets now, so those cases would fail) and is the only change of a non-owned file
  besides the following.
- `tests/ref/vectors/t-slice1/` (new, allowed by COMMON-C rule 2).
`tests/driver/README.md` (not owned) does not list the new `i-*` cases; the table of `tools/adf/README.md` does.

## Not done

- The dump form (conventions 10) of the three types: `adf_x_dump_str`, `_load_str`, `_dump_inspect` for `ucoset`, `idele`,
  `idclass`, and `dump`/`load` of the driver for them. It needs the loader and dumper of `src/dump.c` (not this lane's) and
  is not small; left, as the brief allows.
- No mutation run (the brief replaces it by item 4). No fuzz run longer than 121 s; no differential fuzz against the Python
  reference beyond the vector test.
- `tests/driver/README.md` and `docs/conventions.md` are not updated (not owned).

## Findings

1. **Against conventions 9.6 and PLAN 5** (not a fault): the example of `docs/PLAN.md` 5, `(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])`,
   prints back from C as `(2.5 +/- 1.1e-9 ; ...)` and `<1.25 +/- 1e-30 ; [5 mod 36]>` as `<1.25 +/- 1.1e-30 ; ...>`: the
   radius is rounded up to 30 bits and then to two digits. The golden files hold the reference text; the C test compares
   the text only where the input is exact (dyadic), as the adele test does.
2. **Cost of the constrained printer**: the least `k` is searched linearly and each level costs the size of the numbers.
   The ball `2^b + 1 +/- 2^b` prints in 0.10 s for `b = 4000`, 2.1 s for `b = 16000`, 24 s for `b = 40000` (measured with
   `-O2`); the bound of M1-D6 allows `b` up to 10^5, about seven minutes. A proved lower bound of `k` would remove it; none
   is proved, and the levels are not nested (docs/api-2.md 4.2). Not repaired.
3. Known, as the brief says: conventions 5.7 names `adf_idclass_t_get`, `adf_idclass_unit_get` (the code: `adf_idclass_get_t`,
   `adf_idclass_get_unit`); conventions 3.2 has no LIMIT for ideles (the code has one for prec, and this slice too).
4. The sentence of conventions 9.3 "if it fails only there, the status is ADF_NOT_DETERMINED, and a higher prec will succeed" is
   true with Statement P and false for the enclosing ball alone.
5. Avoidable costs (COMMON-C rule 7): the gcd of a unit is formed in the reader and again in `adf_ucoset_set_fmpz2`; the
   content is reduced in the reader and again in `adf_idele_set_parts`; every printer copies the unit to normalise it.

No `HEADER-FINDING`; no `[source pending]` statement; no finding against `docs/SPEC.md`.

## Files

New: `src/text_idele.c`, `tests/test_text_idele.c`, `tests/julia/text_idele.jl`, `tests/fuzz/fuzz_text_idele.c`,
`tests/driver/i-01-show-type.{cmd,out}` ... `i-06-status.{cmd,out}`, `tests/ref/vectors/t-slice1/{golden_parts,text_idele,
print_constrained}.jsonl`, `lanes/t-slice1/{gen_vectors.py,check_driver_cases.py,seed_corpus.py,bite.sh,bite.log,fuzz.log,
redgreen.log,progress.md,red1.out,green1.out,result.md}`.
Changed: `include/adelefeld/text.h`, `src/text.c`, `tools/adf/adf.c`, `tools/adf/README.md`, `docs/api-2.md`, `tests/test_julia.sh`,
`tests/driver/{06_pairs,07_status,12_status_order,13_dump}.cmd`. No compiled binary in `lanes/t-slice1/`.
