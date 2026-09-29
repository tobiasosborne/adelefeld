# Lane m1-small: result

All five items done. No mutation run (brief). No git, no bd.

## Item 1, adf-zbl
- `tests/test_text_r3r9.c`: the measurement became four assertions (accepted adele text at prec 2, accepted cadele
  text at prec 2, the same two at prec 191, the largest of the range); the sets are kept apart per kind
  (`precs_ade`, `precs_cad`). The stale comment "at_prec_2 is 0 today ... not asserted" was replaced.
- Fuzz target reads prec = 2 + byte0 mod 190, digits = 1 + byte1 mod 30 (`tests/fuzz/fuzz_text.c:287-288`).
  Four new corpus files in `tests/fuzz/corpus/text/` (name = sha1 of content, as the others):
  bytes (0,0)+`(0.1 ; 0)`, (0,4)+`((0.1) + (0.2)*i ; 0)`, (189,7)+`(-0.5 +/- 0.25 ; 0)`,
  (189,29)+`((0.1) + (0.2)*i ; 0)`.
- Red (`redgreen.log`): without the files, 4 failed checks (`precs_ade[2]`, `precs_cad[2]`, `precs_ade[191]`,
  `precs_cad[191]`), "8 tests, 148 checks, 4 failed checks". Green with them: "8 tests, 148 checks, 0 failed
  checks"; corpus 283 -> 287 files, accepted 90 -> 94, prec values reached 65 -> 67 of 190.

## Item 2, adf-mds
- Stale comment (`tests/test_dump_limits.c`, before `QCLASS_EXP_MAX`): "The header does not define it
  (HEADER-FINDING, lane report)". `include/adelefeld/dump.h:72` defines `ADF_DUMP_QCLASS_EXP_MAX 1048576`. The
  comment now says so, and a `_Static_assert(ADF_DUMP_QCLASS_EXP_MAX == (1 << 20))` keeps the test's own copy and
  the header from drifting.
- `all_load` no longer skips the typed loaders when the context of the text cannot be built: it binds a small
  unrelated context (`adf1 Q modctx 6 2 2 3`) and runs the four loaders, which must give the status of the
  text (that status for the loader of the body's kind, PARSE for the others; a refused text is decided in stages
  1-5 of 8.5, before bindings), then still returns -1 for the callers.
- Scratch check (copy of `src/dump.c` in the scratchpad, not in the tree): `adf_cadele_load_str` made to return
  DOMAIN instead of UNSUPPORTED. The test then fails: `FAIL tests/test_dump_limits.c:291:
  block_cap_refuses_65537_blocks_everywhere ... cadele loader: DOMAIN`, "9 tests, 297 checks, 1 failed checks".
  The real tree: "9 tests, 297 checks, 0 failed checks" (16.7 s).

## Item 3, adf-bgf (`docs/conventions.md` only)
- 12.1: says `ADF_INLINE` is `static inline`, FLINT writes `static __inline__` (`fmpz.h:15-19`), and why: no compiler
  extension (`common.h:36-44`, `lanes/m1-common/report.md` section 6).
- 11.3 item 4: `arf_get_mag` (`arf.rst:403-405`), `mag_set_fmpz_2exp_fmpz`, `mag_set_ui_2exp_si`
  (`mag.rst:147-151`) are documented as upper bounds only; probed (m1-text report): the first two add one unit at 30
  bits, `mag_set_ui_2exp_si` is exact below 2^30. The exactness of the latter is NOT in the FLINT documentation
  (only probed, and checked by `tests/test_text_adele.c`); the sentence says so. No `.c` or `mag.h` of FLINT is on
  disk under `refs/`, so no source line for it: `[source pending]` in substance.
- 8.4: the C reader compares exponent digits as a number of any length (M1-D7). Measured with a scratch program
  (`adf_adele_set_str`, prec 64, default limits): `1e100000` OK; `1e100001` LIMIT; 19-digit exponent LIMIT; 39-digit
  exponent positive and negative LIMIT; 38 digits `00...05` OK; `0e` + 39 digits LIMIT; max_exp10 = 10^6 with a
  39-digit exponent LIMIT. Written into the convention. `proto/text_grammar.py` (the reference with the 18-digit
  bound) is not on disk in this worktree; that half of the sentence rests on SPEC M1-D7 (`docs/SPEC.md:872`).
- prec below 2: one sentence added at the `prec` bullet of 8.1 (parsers): taken as 2; measured: `(0.1 ; 0)` at prec
  -5, 0, 1, 2, 3 prints the same `(0.0938 +/- 0.0064 ; 0)`. `text.h` not changed for this (not in my ownership list).

## Item 4, adf-7gc item 3
- Red: `tests/test_abi.c` with the checks -> compile error "implicit declaration of function adf_sizeof_text_kind".
- `include/adelefeld/text.h`: `adf_sizeof_text_kind` and `adf_alignof_text_kind`, header-inline like the others (they
  are exported through `ADF_INLINES_C`). `tests/test_abi.c`: `ADF_ALIGN(adf_text_kind, 4)` static assertion (the
  size assertion existed) and run-time checks 4/4 and equal to `sizeof` / `_Alignof`. Green: "3 tests, 24 checks, 0
  failed checks". The checks call the header-inline functions, so no library symbol is referenced.
- Not done here: `tests/julia/layouts.jl` and `docs/api-m1.md` (generated) do not list the new pair; not mine.

## Item 5
- `make clean && make -j2 check-all`: last line `check-all passed: make check, driver, exports, julia,
  mutate-selftest, memcheck-selftest`.
- `make clean && make -j2 check SAN=1`: `check passed: all 56 test programs`.
- `make clean && make -j2 check CC=clang`: `check passed: all 56 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`.

## Not done
- No mutation run, no long fuzz run (the corpus files are seeds; the assertion is on the corpus, not a fuzz
  campaign). `make clean` leaves the tree built at the state of the last check (clang).
