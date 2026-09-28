# Lane m1-adele: adeles and complex adeles (work package 1.3)

## What was done

`src/adele.c` implements every function that `include/adelefeld/adele.h` declares, for
`adf_adele` and `adf_cadele`, except the two inline layout queries. The finite coordinate is
reached only through `adelefeld/fball.h`; the archimedean coordinate through `arb` and `acb`.
The real and complex parts of `add`, `sub`, `mul`, `neg`, `add_rat`, `mul_rat`, `div_rat` are
the corresponding FLINT operations at `prec`; the finite parts are the tight `fball.h`
functions and do not depend on `prec`. `adf_adele_set_arb_fball`, `adf_cadele_set_acb_fball`,
`adf_adele_get_arb_at`, `adf_adele_div_rat` and `adf_cadele_div_rat` check before the first
write, so the output is untouched on `ADF_DOMAIN` or `ADF_NOT_UNIT`.

`tests/ref/adfref/adele_ref.py` is the oracle: a rational interval (and a pair of them for the
complex coordinate) and an `adfref.fball`. `lanes/m1-adele/gen_adele_vectors.py` writes the
three vector files. `tests/test_adele.c` and `tests/test_cadele.c` read the vectors with
`tests/support/jsonl.h`, check the finite result against both the vector and the `fball.h`
function alone, check the real/complex output against every rational witness and against the
ends of the true result interval, and add the direct tests: life cycle, aliasing of every
permitted combination, representation identity, the statuses with untouched outputs, and the
exact `(i ; 0)^2 = (-1 ; 0)`. `bench/bench_adele.c` measures the eight rows of `PLAN.md` 1.3.

## Files written

| File | Content |
|---|---|
| `src/adele.c` | the implementation, 552 lines |
| `tests/test_adele.c` | 13 tests, 6712 checks |
| `tests/test_cadele.c` | 15 tests, 6141 checks |
| `tests/ref/adfref/adele_ref.py` | interval, adele and cadele reference, dyadic exactness |
| `tests/ref/vectors/m1-adele/adele_ops.jsonl` | 97 records |
| `tests/ref/vectors/m1-adele/cadele_ops.jsonl` | 80 records |
| `tests/ref/vectors/m1-adele/set_rat.jsonl` | 37 records |
| `tests/ref/vectors/m1-adele/README.md` | the schema and the coverage |
| `bench/bench_adele.c` | the eight `adf_adele` rows |
| `bench/Makefile` | added `bench_adele` to `all`, `run` and `clean`, with its rule |
| `lanes/m1-adele/gen_adele_vectors.py` | the generator (seed 20260928) |
| `lanes/m1-adele/red-green.log` | the red, green, sanitizer and mutation log |
| `bench/results/2026-09-28T074850Z_adele.txt` | the one short benchmark run |

## Checks run

- Reconstructed red (see `lanes/m1-adele/red-green.log`): the tests linked against an archive
  without `adele.o`. Result: 148 undefined references for `test_adele`, 152 for `test_cadele`,
  exit 1.
- `make -j2 all`: `build/libadelefeld.a` built, exit 0.
- `make -j2 check`: `check passed: all 11 test programs`; `test_adele` 13 tests, 6712 checks,
  0 failed; `test_cadele` 15 tests, 6141 checks, 0 failed.
- `make clean && make -j2 check SAN=1`: `check passed: all 11 test programs`, exit 0.
- `make clean && make -j2 check CC=clang`: `check passed: all 11 test programs`, exit 0.
- `make mutate FILES=src/adele.c JOBS=2 LIMIT=150`: 90 mutants in 64.5 s, 13 killed,
  0 survived, 77 not compiled, 0 timed out, 0 excused; target passed.
- `python3 lanes/m1-adele/gen_adele_vectors.py`: wrote 97 + 80 + 37 records, exit 0.
- `./bench/bench_adele --run --trials 3 --cpu 2`: 8 rows, checksum 34313, result file above.

The first mutation run had one survivor, `src/adele.c:368` `adf_cadele_is_canonical`
(`&&` -> `||`); the new test `cadele_is_canonical_needs_both_coordinates` makes the finite part
non-canonical with a finite complex part and then the complex part non-finite with a canonical
finite part, and the second run has 0 survivors. The 77 "not compiled" mutants are the tool's
`&` -> `|` rewrite inside `&&`, which produces `&|` and does not parse.

Benchmark, `bench/results/2026-09-28T074850Z_adele.txt` (provisional, `quiet_machine: no`,
seed 20260928, 3 trials, pinned to cpu 2); the columns are ns per operation, min/median/max:

| row | min | median | max |
|---|---|---|---|
| `adele_add_chain_53_word` | 198.7260 | 199.3390 | 200.2913 |
| `adele_mul_chain_53_word` | 1727.1270 | 2461.3017 | 3171.8950 |
| `adele_add_chain_53_4096` | 857.8110 | 863.5855 | 870.0670 |
| `adele_mul_chain_53_4096` | 1293.2333 | 1355.8933 | 1421.3400 |
| `adele_add_chain_4096_word` | 251.5588 | 256.6393 | 259.5057 |
| `adele_mul_chain_4096_word` | 585.6540 | 674.8560 | 767.5360 |
| `adele_add_chain_4096_4096` | 941.4450 | 948.8863 | 957.9688 |
| `adele_mul_chain_4096_4096` | 1379.6800 | 1389.4400 | 1390.0300 |

The "word" and "4096 bit" labels of the finite part name the starting operand (the record's
`bit_lengths` says "start ..."). As in `bench_fball.c`, the mul chain uses the exact second
operand `(3 ; 3)`, so the radius of the finite part grows by about one bit per multiplication;
the add chain adds `(3 ; 3)` and keeps the radius. The numbers are not comparable to the
`fball` rows: they add one `arb` operation and one `fball` operation per call.

## What is not done

- **The red-green and reference-first ordering was not followed.** `src/adele.c` was written
  before `tests/ref/adfref/adele_ref.py`, the vectors and the test files. The reference and the
  vectors are independent of the C (they use exact rational arithmetic and `adfref.fball` only),
  and the red link errors in `lanes/m1-adele/red-green.log` were reconstructed after the fact by
  linking the finished tests against a library without `adele.o`. This is a process deviation,
  recorded here so the orchestrator can judge it.
- No `adf_cadele` benchmark row was asked for or written; `PLAN.md` 1.3 asks for add and mul of
  `adf_adele` only.
- The finite coordinate depends on the merged `fball.h` backend. This worktree has the global-only
  `src/fball.c` of work package 1.2: its functions silently write nothing for an `ADF_LOCAL`
  input. A local `adf_fball` cannot be constructed here (no `modctx`), so no test can reach that
  path; when work package 1.8 lands, `adf_adele_*` will hand the local operand to `fball.h` and
  use its result, as the header says. The `adf_adele`/`adf_cadele` code itself needs no change.
- No `-DADF_CHECK_INVARIANTS` build (the whole-library check is not in this lane; see the report
  of lane m1-rat for the same note).
- The benchmark is one short run, provisional, on a shared laptop.

## Avoidable costs (COMMON-C rule 7)

- `adf_adele_set_rat`, `add_rat`, `mul_rat`, `div_rat` and the cadele counterparts copy the
  rational through `adf_rat_get_fmpq` into an `fmpq` and then into a ball. Since the struct is
  public in the library, `q->q` could be passed to `arb_set_fmpq` / `acb_set_fmpq` directly; the
  copy is one `fmpq_set`.
- `adf_adele_add_rat` (and the cadele one) initialise a temporary `adf_fball` of the exact `q` and
  then call `adf_fball_add`; `fball.h` has no add-with-rational, so the temporary is needed.
- `adf_adele_div_rat` / `adf_cadele_div_rat` compute the finite result into a temporary and
  `swap`, to keep the output untouched if `adf_fball_div_rat` returns a non-`ADF_OK` status. For
  the global backend the call cannot fail after the zero check, so a direct write would be safe
  and cheaper; the temporary is kept for the status rule.

## Sources pending

None. Every formula is either in `docs/SPEC.md`, `docs/proofs/precision.md` or the FLINT
documentation on disk. The file paths of the FLINT documents are
`refs/src/flint-3.0.1/arb.rst` and `refs/src/flint-3.0.1/acb.rst`, not
`refs/src/flint-3.0.1/doc/source/` as the brief writes.

## Findings against the specification

None. No statement of `docs/SPEC.md` 4.1, 4.3 or 4.5 was found wrong or unprovable in the
implementation.

## Header findings (COMMON-C rule 6)

1. **`arb_add_fmpq`, `arb_mul_fmpq`, `arb_div_fmpq`, `acb_add_fmpq`, `acb_mul_fmpq`,
   `acb_div_fmpq` do not exist in FLINT 3.0.1.** The comment blocks of `adf_adele_add_rat`,
   `adf_adele_mul_rat`, `adf_adele_div_rat` and of the two `adf_cadele` counterparts name them.
   `nm -D /usr/lib/x86_64-linux-gnu/libflint.so | grep fmpq` lists `arb_set_fmpq`,
   `arb_contains_fmpq`, `arb_get_rand_fmpq`, `arb_pow_fmpq`, `acb_set_fmpq`,
   `acb_contains_fmpq` and no `*_add_fmpq`, `*_mul_fmpq`, `*_div_fmpq`; `arb.rst` and `acb.rst`
   document none. Reading used: convert `q` with `arb_set_fmpq` (`arb.rst:167`) or `acb_set_fmpq`
   (`acb.rst:128`) and then use `arb_add`/`arb_mul`/`arb_div` (`arb.rst:767`, `:798`, `:856`) or
   `acb_add`/`acb_mul`/`acb_div` (`acb.rst:429`, `:463`, `:521`). This is an enclosure of the
   exact result, so the proof of the finite coordinate and the enclosure rule are unaffected; the
   finite coordinate remains exact and independent of `prec`. The marking `HEADER-FINDING` is in
   the file comment of `src/adele.c`.
2. **The path in the brief.** `refs/src/flint-3.0.1/doc/source/` is not the layout on disk; the
   `.rst` files are directly under `refs/src/flint-3.0.1/`. No content is missing.
