# Lane m1-repair-text: findings R2, R3, R4, R9, R10 of reviewer `text`; issues adf-b8l, adf-5qq

Read `lanes/COMMON-C.md`. Then `docs/reviews/m1/text/review.md` completely, with its reproducers under
`docs/reviews/m1/text/checks/` (`probe.c`, `resources.py`, `boundaries.py`, `sentinel_padding.c`,
`fuzz_one.c`); `include/adelefeld/text.h` (changed today: decisions M1-D6 and M1-D7, the constant
`ADF_PRINT_EXP_MAX`, a printer may return NULL); `docs/SPEC.md` section 15 rows M1-D6, M1-D7;
`docs/conventions.md` 8.4, 9.5, 9.6; `src/text.c`, `src/dump.c` (`adf_scaled_get_str` prints through the
printer of `adf_fball`, which never returns NULL: check), the four `tests/test_text_*.c`,
`tests/fuzz/fuzz_text.c`, `proto/text_grammar.py` and `proto/test_text_grammar.py`, `tools/adf/adf.c` (its
guard before printing; with the library's own bound the driver's guard becomes a second line of defence:
do not remove it, it is another lane's file).

**You own:** `src/text.c`, `tests/test_text_adele.c`, `tests/test_text_limits.c` (new),
`tests/fuzz/fuzz_text.c`, `proto/text_grammar.py` (only the exponent rule of finding R4),
`proto/test_text_grammar.py` (tests for that rule), `lanes/m1-repair-text/`.

1. **R2 and M1-D6, red first.** `tests/test_text_limits.c`: a real ball with midpoint `2^e`, radius 0, and
   the same with radius `2^e` and midpoint 0, and both in the imaginary part of a complex adele, for
   `e` in `ADF_PRINT_EXP_MAX - 1`, `ADF_PRINT_EXP_MAX`, `ADF_PRINT_EXP_MAX + 1` (mind that FLINT's
   exponent of `2^e` is `e + 1`: the rule is on FLINT's exponent as the header says; work out which `e`
   are at the bound, in a comment), `10^6`, `10^8`, `2^40`, `2^64`, and the negatives: the printer returns
   a text for exponents within the bound and NULL with `*len = 0` above it, within one second and under
   `ulimit -v 2000000` (run from a script in your lane directory). Then the code; remove the
   `flint_abort` of `src/text.c` near line 1078.
2. **R4 and M1-D7, red first.** The exponent of a literal against `max_exp10` for limits of 19 and 20
   digits up to `WORD_MAX`; a zero coefficient with a huge exponent within the limit is the exact zero and
   costs nothing; an exponent above the limit is `ADF_LIMIT` at stage 4; a non-zero coefficient with an
   exponent within a huge limit: the value would need about `10^18` bits; say in the report what the code
   does then and why (the header M1-D6 rule concerns printing; for reading, the limits of conventions 8.4
   are the caller's protection, and `max_exp10` is the limit that bounds this cost: the default is what
   matters, and a caller who raises it asks for the cost). Change the test of
   `tests/test_text_adele.c` near line 1400 that pins the hidden bound. Repair `proto/text_grammar.py` in
   the same way and run `python3 proto/test_text_grammar.py`.
3. **R3.** A test that pins the documented behaviour: `(9.99e100000 ; 0)` read with the default limits,
   printed with `digits = 1`, read back with the default limits gives `ADF_LIMIT`, and with
   `max_exp10` raised by one gives `ADF_OK` and an enclosure.
4. **R9.** `tests/fuzz/fuzz_text.c`: `prec` and `digits` come from two control bytes at the start of the
   input, not from bytes of the text; every accepted text is also printed and read back at `prec = 2` and
   `digits = 1`; a printer that returns NULL is accepted only when the exponent is over the bound. Seed
   corpus: prefix the existing seeds (script in your lane directory). `make fuzz FUZZ_TARGET=text
   FUZZ_SECONDS=120`; report executions, coverage, and the sets of `prec` and `digits` that accepted inputs
   reached.
5. **R10.** The sentinel checks of `tests/test_text_adele.c` lines 420 and 435 and of the fuzz target
   compare whole structs with `memcmp` and read bytes that were never written: zero the storage before
   the ordinary init, or compare field by field. Valgrind on `build/test_text_adele`
   (`valgrind -q --error-exitcode=9 --leak-check=full --errors-for-leak-kinds=definite,indirect`) must
   then report no error.
6. `make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`,
   `sh tests/test_driver.sh`. The reviewer's `resources.py` against the repaired library, with its new
   table.
7. `make mutate FILES=src/text.c JOBS=2 LIMIT=300`; survivors killed or listed with the reason in the
   report (do not edit `tools/mutate/equivalent.txt`).

## Resume note of the orchestrator (2026-09-28, 15:50)

This lane was stopped from outside when the machine ran low on memory. It was not your fault and nothing is
lost: your files are in the worktree as you left them, uncommitted. Before anything else run `git status`
and `git diff --stat`, read the logs in your lane directory, and find the first item of the brief that is
not finished. Do not start again from the beginning and do not rewrite what works. Memory: never run two
builds or two mutation runs at the same time; use `make -j2`; run a mutation run in the foreground, not
with `nohup`; if `free -g` shows less than 6 GB available, wait. Finish with `report.md`.
