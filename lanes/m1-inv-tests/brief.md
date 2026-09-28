# Lane m1-inv-tests: the test suite passes under `INV=1` (adf-6vy)

Read `lanes/COMMON.md` and `lanes/COMMON-C.md`. Then `lanes/m1-invariants/report.md` (all of it; the section
"Tests that step outside the contract" is your list), `src/invariants.h`, `docs/conventions.md` 4.4, 4.6 and
section 5, `docs/SPEC.md` section 15 rows M1-D2, M1-D10, M1-D11 (the last two are proposed; work by them).

**You own:** `tests/test_dump_ctx.c`, `tests/test_dump_golden.c`, `tests/test_fball_local.c`,
`tests/test_fball.c`, `tests/test_recon.c`, `tests/README.md` (one paragraph), `lanes/m1-inv-tests/`.
Nothing under `src/` or `include/` is yours. If a test can pass under the flag only by a change of `src/`,
stop on that test and report it.

## The rule for every change

Without the flag every test checks exactly what it checked before: the same calls, the same expected values,
the same number of checks (compare the summary lines of each of the five programs before and after; the
numbers must be equal). You never delete a check and never weaken an expected value.

1. **A helper that builds a local value by writing the context field by hand** (`sc_init`, `sc_clear`,
   `mklocal_*`, `make_local_shaped` and their like): where the test needs a canonical value, build it through
   the public interface (the functions that the header offers for it; read `fball.h` and `scaled.h`) and
   clear it with the public clear. Where the test needs a value that is NOT canonical (to test a predicate or
   a raw function that the header exempts), the hand-built value is kept, it starts from a fresh `init`, never
   from a live counted value, and a comment says which header line admits it.
2. **A sentinel pointer in a value** (0x10, 0x30, 0x40: the tests use them to show that a failed load leaves
   the output untouched): keep the idea, change the means. Under the flag a sentinel must be a live context
   made for the purpose (M1-D2: a pointer field is NULL or a live object), and "untouched" is checked by
   comparing the field with that context. Without the flag the test may keep its constant.
3. **A free of a context with a live borrower** (`tests/test_fball_local.c:415`): read what the test wants to
   show. If it shows something about the freed context, reorder (clear the value first) so that the contract
   holds with and without the flag; if the order is the point of the test, say so in the report and leave it.
4. **A call with a non-canonical input to a function that its header does not exempt**
   (`tests/test_fball.c:1100`, `tests/test_recon.c:1313`): M1-D11. The test is compiled only without the flag
   (`#ifndef ADF_CHECK_INVARIANTS`), with a comment that cites M1-D11; with the flag a test of the same name
   forks a child and requires the abort with the line of `src/invariants.h` (the pattern is in
   `tests/test_invariants.c`).

## Work

- First the state before: `make clean && make -j2 check` and `make clean && make -j2 -k check INV=1`, logs in
  your lane directory, with the summary line of each of the five programs.
- Then one program at a time; after each: both builds, that program run, its summary lines noted.
- At the end, each preceded by `make clean`: `make -j2 check`, `make -j2 check SAN=1`,
  `make -j2 check CC=clang`, `make -j2 check INV=1`, `make -j2 check INV=1 SAN=1`. All five must pass with
  all test programs. Give the last line of each in the report, and the table of summary lines before and after.
- Other lanes build on this machine: one build at a time, `free -g` before each, wait below 6 GB available.
  Do not run the mutation tool.
