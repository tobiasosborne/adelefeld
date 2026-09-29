# Lane m1-small: four small issues of milestone 1 (adf-zbl, adf-mds, adf-bgf, adf-7gc item 3)

Read first: `lanes/COMMON.md` (rule 3: every test under `timeout`), `lanes/COMMON-C.md` (no mutation run
in this lane). Each item is small; finish one before the next; red first where a test is written
(`lanes/m1-small/redgreen.log`).

**You own:** `tests/fuzz/corpus/text/` (additions), `tests/test_text_r3r9.c`, `tests/test_dump_limits.c`,
`docs/conventions.md` (ONLY the sentences of item 3), `include/adelefeld/text.h` and `src/inlines.c` or
the file where the other `adf_sizeof_*` functions live (ONLY for item 4), `tests/test_abi.c` (additions
for item 4), `lanes/m1-small/`. Everything else is read-only.

1. adf-zbl: no accepted adele or cadele text of the fuzz corpus reaches `prec = 2`
   (`lanes/m1-leftovers/report.md`, finding 1; `tests/test_text_r3r9.c` prints the measurement). Read how
   the fuzz target `tests/fuzz/fuzz_text.c` takes `prec` and `digits` from the input; add corpus files
   (one accepted adele text and one accepted cadele text with the control bytes for `prec = 2`, and for the
   largest `prec` of the range) in the naming of the corpus; then make the measurement of
   `tests/test_text_r3r9.c` an assertion: the test fails if the value 2 is not reached by an accepted adele
   text and by an accepted cadele text. Red: the assertion without the new files.
2. adf-mds: the stale comment at `tests/test_dump_limits.c:61` (find it by its text; say what was stale);
   the helper `all_load` of the same file skips the typed loaders after a failed construction: make it
   run them, or say in a comment why skipping is right, and show with a scratch copy that the test still
   fails when a typed loader is broken.
3. adf-bgf: `docs/conventions.md`: 12.1 says `static __inline__` and the headers use `static inline` on
   purpose (say so, with the reason that `lanes/m1-common/report.md` gives); 11.3: which of `arf_get_mag`,
   `mag_set_fmpz_2exp_fmpz`, `mag_set_ui_2exp_si` is exact at 30 bits (from `lanes/m1-text/report.md`,
   and from the FLINT documentation under `refs/src/flint-3.0.1/mag.rst` with file and line); 8.4: the
   reference refuses decimal exponents of more than 18 digits, what does the C reader do (test it, write
   what is true); `prec` below 2 in `text.h` (decision M1-D4: taken as 2): one sentence in the convention.
4. adf-7gc item 3: `adf_sizeof_text_kind` and `adf_alignof_text_kind`, as the other types have them, with
   the pin in `tests/test_abi.c` (checked inside `sizeof`, without a reference to a symbol of the library:
   see how the file does it for `resid.h`).
5. `make clean && make check-all`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check
   CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass. Give the last line of each.
