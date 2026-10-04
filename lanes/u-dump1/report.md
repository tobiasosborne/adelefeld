# Lane u-dump1: the dump form of the unit coset, the idele, the idele class, the local ball and the partial ball

Contract: `docs/conventions.md` section 10 (grammar 10.1 lines 1384-1426, rules 10.2 lines 1428-1494),
section 5.6 to 5.9, section 8.4 and 8.5. Ground truth on disk: `docs/conventions.md`, the reference
`proto/text_grammar.py` (dump part, lines 1014-1521), the public headers of this tree and the FLINT
headers in `/usr/include/flint`; `refs/src/flint-3.0.1/arb.rst:285-293` for `arb_dump_str`.

## 1. What was done, type by type

All five types are done: code, tests, driver. Nothing of the five is left out.

### `adf_ucoset`, body `ucoset c N`

Loader `adf_ucoset_load_str` builds the value with the public constructor `adf_ucoset_set_fmpz2`
(`include/adelefeld/ucoset.h:88`), so the modulus of the text is the modulus of the value (CV-17,
"Unit moduli are dumped as stored"). The predicate of 5.6 is `dp_v_ucoset` (`src/dump.c:593`). The
dumper writes `x->c` and `x->N` (the fields of `include/adelefeld/ucoset.h:47`).

### `adf_idele`, body `idele <arch> num(r) den(r) c N`

Loader through `adf_idele_set_parts`; the real ball is rebuilt from the four validated tokens by
`dp_set_arb`, so the dump holds the ball and not an enclosure of it. Stage 6 checks the predicate of
5.7: the archimedean count is 1 (10.1: "For `Q` the archimedean count must be 1 (`ADF_DOMAIN`
otherwise)"), the `arb` is canonical (`dp_v_arb`, the helper of lane m1-dump), the ball excludes 0
(`dp_arb_sign` of `src/dump.c:625`, exact on the tokens, exponents read as `fmpz`), the content is a
canonical positive rational (`dp_v_fmpq` plus `dp_pos`), and the unit satisfies 5.6.

### `adf_idclass`, body `idclass <arb> c N`

Same shape; the ball must be positive (`dp_arb_sign(...) != 1`), and the type has no count.

### `adf_lball`, body `lball p (x num(u) den(u) v | b u v N)`

`dp_w_lb` reads the five tokens and keeps them (`src/dump.c:877`); stage 4 bounds `|v|` and `|N|` by
`max_prec` and by what an `slong` holds, stage 5 refuses `p >= 2^64`. `dp_v_lb` (`src/dump.c:678`)
is the predicate of 5.8, primality with `n_is_prime` (`/usr/include/flint/ulong_extras.h:335`);
the power `p^(N - v)` is not formed when it is larger than `u` (a comparison of bit lengths, as
`adf_lball_is_canonical` does, `include/adelefeld/lball.h:105-107`).

### `adf_sball`, body `sball (n | r <arb> | c <acb>) <count> {<lb>}`

The tag is the archimedean tag of 5.9; `max_items` bounds the count (8.4), the arbs are canonical
(so finite, 5.9), each `lb` satisfies 5.8 and the primes strictly increase. The loader allocates the
array `loc` with `flint_malloc` and initialises every component, as `include/adelefeld/sball.h:82-92`
requires of a binding that fills the struct by hand; the same clause is the only way to build a
COMPLEX value (sball.h:60).

### Driver

`adf_drv_body_slot` (`tools/adf/adf.c:521`) now names the nine bodies the driver has a value for, and
the dumper, the inspector switch and the loader switch of `adf_drv_load` have the five new cases.

## 2. The files written

- `include/adelefeld/dump.h`: five comment blocks and twenty declarations (four per type), the
  includes of `ucoset.h`, `idele.h`, `idclass.h`, `lball.h`, `sball.h`, and the header comment.
- `src/dump.c`: `dp_uco`, `dp_lb`, the fields of `dp_node`, `dp_pos`, `dp_v_ucoset`, `dp_arb_sign`,
  `dp_v_lb`, `dp_fits_si`, `dp_read_lb`, `dp_sb_si`, `dp_sb_lb`, the five cases of `dp_w_body`, the
  removal of the five cases from `dp_w_other`, `DP_INV`, and the twenty public functions at the end
  of the file. 1855 -> 2541 lines.
- `tests/test_dump_units.c` (new, 851 lines), `tests/test_dump_local.c` (new, 819 lines).
- `tools/adf/adf.c`, `tools/adf/README.md`.
- `tests/driver/u-dump-units.cmd`, `.out`, `tests/driver/u-dump-local.cmd`, `.out` (new).
- `lanes/u-dump1/progress.md`, `lanes/u-dump1/mutate.txt`, this report.

No Makefile line and no export-list line were needed: `Makefile:70` takes every `tests/test_*.c` by
`$(wildcard ...)`, and `tests/test_exports.sh` reads the declarations out of the headers with
`-aux-info`. Both are checked in section 6.

`tests/ref/vectors/u-dump1/` was not written: `tests/golden/dump.tsv` holds rows for all five bodies
(20 rows with a body of `ucoset`, `idele` or `idclass`, 7 of them valid; 22 rows with a body of
`lball` or `sball`, 9 of them valid), and every one of those rows is run by the two test files.

## 3. The declarations added

For each of the five types, in the style and with the comment block of the `adf_rat` group
(`include/adelefeld/dump.h:83-91`):

    int adf_<t>_load_str(adf_<t>_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                         const adf_text_limits_t * lim);
    int adf_<t>_load_str_binds(adf_<t>_t x, const char * s, size_t len,
                               const adf_modctx_struct * const * binds, size_t nbinds,
                               const adf_text_limits_t * lim);
    char * adf_<t>_dump_str(size_t * len, const adf_<t>_t x);
    int adf_<t>_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                             const adf_text_limits_t * lim);

The `ctx` argument is accepted and ignored, as for `adf_rat`: none of the five bodies has a context
occurrence, and a binding array of any length but 0 is `ADF_DOMAIN` (10.2, gate finding G3), which
the tests pin.

## 4. Decisions where section 10 is silent (each with the alternative)

1. **`v` and `N` of a dumped local ball that do not fit in an `slong`.** The fields are `slong`
   (`include/adelefeld/lball.h:85-88`) and the grammar's `h` has no length bound, so a text such as
   `adf1 Q lball 5 b 1 0 8000000000000000` is a sentence of 10.1 that no value of the type can hold.
   Decision: `ADF_LIMIT`, decided in stage 4 (`dp_fits_si`, `src/dump.c:868`), beside the `max_prec`
   check of 8.4, which is a limit on a literal. Alternatives: `ADF_DOMAIN` (a semantic constraint of
   5.8 that the loader cannot state, since the predicate holds for the `fmpz`) or `ADF_UNSUPPORTED`
   (the word restriction of 8.5 item 5 is about `p` and `q`). With the default `max_prec = 100000`
   the check never fires.
2. **The archimedean count of an idele.** 10.1 writes `idele <arch> ...` and says that for `Q` the
   count is 1 (`ADF_DOMAIN` otherwise); the same sentence as for an adele, and the reference decides
   it the same way (`_one`, `proto/text_grammar.py:1163`). Not a decision of mine, but the test
   `adf1 Q idele 0 1 1 1 0` (count 0) and `adf1 Q idele 2 ...` pin it.
3. **`max_items` and a partial ball.** 8.4 lists "the number of ... places". The count token of the
   body is the number of primes and the archimedean place is in the tag, so I applied `max_items` to
   the count token, as the reference does (`proto/text_grammar.py:1303`). A text with one prime and
   the real place is then 2 places and 1 item; the alternative reading would refuse it with
   `max_items = 1`. Reported as an ambiguity, see section 8.
4. **The loader of one type refuses the dump of another with `ADF_PARSE`.** This is the rule already
   of `dp_validate` (a `want_kind`), with the `HEADER-FINDING` comment of lane m1-dump; `dump.h`
   does not say it. I kept it and tested it (`adf1 Q ucoset ...` through the idele loader).

## 5. Fields read or written directly, and where that is sanctioned

`src/dump.c` already wrote the fields of `adf_rat`, `adf_fball` (local) and `adf_scaled`; the
assumption is stated in the file header. For the five new types:

- `adf_ucoset`: the loader uses the constructor, the dumper reads `x->c` and `x->N`
  (`include/adelefeld/ucoset.h:47-50`).
- `adf_idele`, `adf_idclass`: the loaders use `adf_idele_set_parts` and `adf_idclass_set_parts`; the
  dumpers read `x->inf`, `x->r`, `x->u` and `x->t` (`include/adelefeld/idele.h:52-56`,
  `idclass.h:47-50`, the layout that conventions 12.4 makes the contract of a binding that allocates
  the struct inline).
- `adf_lball`: no constructor from raw data exists (only `set_rat`, `set_rat_ball`, `set_fball`,
  `teichmuller`, `decompose`, `pow_si`, all of which go through a place), so the fields are written
  and read directly as `include/adelefeld/lball.h:81-90` lays them out: "The field `p` is read
  through `adf_lball_place`; the fields are otherwise the contract of a binding that allocates the
  struct inline (conventions 12.4)".
- `adf_sball`: likewise, as `include/adelefeld/sball.h:82-92` lays them out; sball.h:60 says in words
  that a binding may fill the struct by hand, which is the only way to make a COMPLEX value.

No type was left out for a missing accessor.

## 6. The checks, with their numbers

    $ timeout 600 make -j2 build/test_dump_units        # red, first run of the file
      ... undefined reference to `adf_ucoset_load_str' ... collect2: error: ld returned 1 exit status
    $ timeout 300 ./build/test_dump_units                # green
      9 tests, 35106 checks, 0 failed checks, 0 failed tests

    $ timeout 600 ./build/test_dump_local               # red, with the predicate of 5.8 switched off
      7 tests, 35971 checks, 40 failed checks, 3 failed tests
    $ timeout 300 ./build/test_dump_local               # green
      8 tests, 35978 checks, 0 failed checks, 0 failed tests

    $ timeout 1500 make -j2 check
      check passed: all 81 test programs        (79 before this lane; +2 new files)

    $ timeout 900 sh tests/test_driver.sh
      test_driver: 13_dump: the output differs from tests/driver/13_dump.out
      (one line, section 7; the script stops there)

    $ timeout 900 sh tests/test_exports.sh
      passed: 482 of 482 declared functions are exported, 0 are not implemented yet,
      no exported name is undeclared, no variadic function      (462 before this lane: +20)

    $ timeout 900 make -j2 CC=clang BUILD=lanes/u-dump1/build-clang \
        lanes/u-dump1/build-clang/test_dump_units lanes/u-dump1/build-clang/test_dump_local
      0 warnings, 0 errors; 9 tests / 35106 checks and 8 tests / 35978 checks, 0 failed
    $ timeout 900 make -j2 INV=1 BUILD=lanes/u-dump1/build-inv ...   (same two programs)
      0 warnings, 0 errors; 0 failed checks
    $ timeout 900 make -j2 SAN=1 BUILD=lanes/u-dump1/build-san ...   (same two programs)
      0 warnings, 0 errors; 0 failed checks, no sanitizer report
    $ timeout 900 make -C tools/adf CC=clang SAN=1 ; ./build/adf-san < tests/driver/u-dump-*.cmd
      0 warnings; both fixtures equal to their .out, exit 1, stderr empty
    $ rm -rf build/drv-plain; make -j2 BUILD=build/drv-plain INV=1 all;
      clang -Iinclude -DADF_CHECK_INVARIANTS ... tools/adf/adf.c ... -pthread -o build/adf-inv
      ./build/adf-inv < tests/driver/u-dump-*.cmd: both fixtures equal to their .out, exit 1,
      stderr empty
      (tools/adf/Makefile forwards neither INV nor -DADF_CHECK_INVARIANTS to adf.c; the flags were
      given by hand, as above)

    $ python3 -B tools/mutate/mutate.py --root . --scratch build/mutate-u-dump1 --files src/dump.c \
        --limit 40 --seed 7 --jobs 2 --san --timeout 300 \
        --make 'make -s -j2 BUILD=bm INV=1 SAN=1 bm/test_dump_units bm/test_dump_local \
                bm/test_dump_golden bm/test_dump bm/test_dump_ctx bm/test_dump_limits && ...' \
        --copy Makefile include src tests lanes tools
      mutate: src/dump.c: 1349 mutants, 40 of them run (--limit 40, --seed 7)
      mutate: 40 mutants in 682.4 s: 34 killed, 1 survived, 5 not compiled, 0 timed out, 0 excused
      (three runs; the first found three survivors, the second two, the last one. The full log is
      lanes/u-dump1/mutate.txt of the last run.)

    $ python3 (proto/text_grammar.py, dump_load_check) over the strictness tables
      95 texts of tests/test_dump_units.c: 0 mismatches
      67 texts of tests/test_dump_local.c: 0 mismatches
      (the texts of the limit block with the default limits of the reference: 11 texts, and the
      library and the reference agree on every one of them)

The build directories under `lanes/u-dump1/` and the scratch `build/mutate-u-dump1` were removed at
the end; no file of this lane is left in `/tmp`.

### Mutation testing: survivors and not-compiled mutants

Survivor of the last run (one line each):

- `src/dump.c:1323` in `dp_header`, `b > 0x7e` -> `b >= 0x7e`: the byte `0x7e` is `~`, which is in no
  token of the grammar of 10.1, so a text that holds one is `ADF_PARSE` at stage 3 either way. The
  mutant is equivalent. `tools/mutate/equivalent.txt` is not a file of this lane, so the entry was
  not added.

Two survivors of the earlier runs were gaps of the tests and are closed:

- the branch `t <= 64` of `dp_arb_sign` (`:658`) was dead code, because `t = min(bits(|m|),
  bits(rm))` and `bits(rm) <= MAG_BITS = 30`; the branch is gone.
- the first letter of the field in `dp_header` (`:1338`, `s[pos] < 'A'`): the texts `adf1 A ucoset
  1 6` (`ADF_UNSUPPORTED`) and `adf1 @ ucoset 1 6` (`ADF_PARSE`) are in `tests/test_dump_units.c`.
- the stage of the check "a qclass of the form `pieces` with no piece is not canonical"
  (`src/dump.c:1018`): `tests/test_dump_local.c` has the test `qclass_stage_of_the_pieces_check`.

Five mutants did not compile, all of them in the code of lane m1-dump and none in a line of this
lane: `src/dump.c:2039` (the removal of an assignment makes `len` an unused parameter),
`:1321`, `:1343`, `:1380` (removing or changing the status of a check of `dp_header` leaves `b`
maybe uninitialised or `dp_header` unused), `:2479`, `:2130`, `:1995` (an `op` mutant inside a
type name or a cast).

## 7. Existing driver fixtures that answer differently now (not edited: not this lane's)

| Fixture | Line | Command | Old | New |
|---|---|---|---|---|
| `13_dump` | 50 | `load adf1 Q lball 5 b 3 0 4` | `error: UNSUPPORTED` | `[p=5: 3 + O(5^4)]` |
| `drv-ball-pairs` | 18 | `dump [p=5: 3]` | `error: UNSUPPORTED` | `adf1 Q lball 5 x 3 1 0` |
| `i-02-ucoset` | 60 | `dump [5 mod 6]` | `error: UNSUPPORTED` | `adf1 Q ucoset 5 6` |
| `i-03-idele` | 55 | `dump (1 ; 1 * [1])` | `error: UNSUPPORTED` | `adf1 Q idele 1 1 0 0 0 1 1 1 0` |
| `i-04-idclass` | 31 | `dump <1 ; [1]>` | `error: UNSUPPORTED` | `adf1 Q idclass 1 0 0 0 1 0` |

One line in each file, no other difference: `13_dump.cmd` line 35 (`dump (0.5 ; 0) + Q`) stays
`error: UNSUPPORTED`, that kind has no body of 10.1. The expected output of the five lines is what
the conventions give: `[p=5: 3]` is the exact local ball `3` at 5, so `x num(u) den(u) v` is
`x 3 1 0`; `[5 mod 6]` is stored as `(5, 6)` because the modulus is dumped as supplied (CV-17), and
the value form prints the normal form `[2 mod 3]`. A `13_dump` change breaks `tests/test_driver.sh`
until the fixture is corrected; the correction belongs to the orchestrator.

## 8. Findings against `docs/conventions.md` section 10 (and against 8.4)

1. **8.4 has no limit for a literal that no value of the type can hold.** `max_prec` (100000) keeps
   `v` and `N` inside an `slong` under the defaults, but a caller may raise it, and then
   `adf1 Q lball 5 b 1 0 8000000000000000` is a sentence of 10.1 whose `N` no `adf_lball_struct`
   can hold (`include/adelefeld/lball.h:85-88`: `slong N`). Section 10 says nothing about it. I
   return `ADF_LIMIT` at stage 4 (decision 1 of section 4). The conventions should say so.
2. **`max_items` and "the number of places" of a partial ball.** 8.4 lists "the number of ...
   places" among the things `max_items` bounds, but the body `sball` has a count of primes and the
   archimedean place is in the tag. `adf1 Q sball r 1 0 0 0 1 2 b 1 0 3` has 2 places and 1 count
   token: it loads with `max_items = 1` and is `ADF_LIMIT` with `max_items = 0`. The reference reads
   it the same way (`proto/text_grammar.py:1303`), so the two agree; the conventions should say
   which is meant.
3. **No body cannot hold a canonical value and no value has two texts.** For the five bodies of this
   lane, every canonical value has exactly one text and every text that the loader accepts denotes
   one value: the fields of `ucoset`, `idele` and `idclass` are written as stored, the two forms of
   `lb` are distinguished by the keyword `x` and `b`, and the tag of `sball` carries the
   archimedean tag. The round trips of 2000 random values of each type are the test of this.
4. **A real ball of exactly 0 with the tag `r` is canonical.** 5.9 asks of the tag REAL that the
   imaginary part is the exact 0 and nothing of the real ball besides finiteness, so
   `adf1 Q sball r 0 0 0 0 0` loads. The reference agrees (`_v_arb` of the tag `r`). The test
   `sball_cases` says `ADF_OK`; a reader who expects a nonzero real ball there would be wrong, and
   the conventions could say it in words.

Nothing else in section 10 contradicts what the library does for these five bodies.

## 9. What is not done

- `qclass`, `ffun`, `rfun`, `char` and `modctx` have no typed loader: they are not in this brief,
  and their grammar, limits and predicates stay as they were (stage 6 of those bodies is still
  `DP_NOSEM` inside `adf_dump_ctx_occurrence`, which answers `ADF_DOMAIN` as before).
- The five fixtures of section 7 are not edited; `tests/test_driver.sh` therefore stops at
  `13_dump`.
- `tools/mutate/equivalent.txt` is not a file of this lane, so the one equivalent survivor has no
  entry there.
- The mutation run covers the whole of `src/dump.c`, not only the lines of this lane:
  `tools/mutate/mutate.py` has no option for a line range (`--help` lists `--files`, `--limit`,
  `--seed`, `--jobs`, `--timeout`, `--make`, `--copy`, `--equivalent`, `--list`, `--keys`,
  `--keep`, `--san`), and `tools/mutate/README.md` does not exist in this tree; the header of
  `tools/mutate/mutate.py` (lines 1-40) was read instead.
- `adf_scaled` and `adf_modctx` were not touched; `dp_the_occurrence` still answers for the five
  bodies of lane m1-dump only, which is right, since the five new bodies have no occurrence.
- A faster `dp_arb_sign` would compare the two exponents without the `fmpz` of the mantissa when the
  binades differ (it already does: the `fmpz` work happens only when they are equal). Nothing else
  was optimised.

## 10. Sources pending

None. Every formula and convention of this lane is on disk: `docs/conventions.md` (10.1, 10.2, 5.6,
5.7, 5.8, 5.9, 8.4, 8.5), `proto/text_grammar.py` (`_v_ucoset` 1243-1248, `_v_lb` 1251-1277,
`_arb_sign` 1122-1143, `_one` 1161-1164, `_dump_validate` 1300-1400), the public headers of this
tree, and the FLINT headers `/usr/include/flint/{mag.h,arb.h,ulong_extras.h,fmpz.h,fmpq.h}`
(`MAG_BITS` at mag.h:117, `n_is_prime` at ulong_extras.h:335). The one probe of this lane
(`arb_dump_str` of -3, of 1*2^3 and of 5, which give `-3 0 0 0`, `1 3 0 0` and `5 0 0 0`) agrees
with `refs/src/flint-3.0.1/arb.rst:285-293` and with the probe already recorded in conventions 10.2.
