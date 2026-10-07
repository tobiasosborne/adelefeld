# Lane t-slice1: the value form of unit cosets, ideles and idele classes, and the driver (milestone 2)

Milestone 2 is implemented except for its text forms. `docs/conventions.md` section 9 fixes the grammar
of the value form of every type, and `tests/golden/ucoset.tsv`, `idele.tsv`, `idclass.tsv` hold golden
vectors (read `tests/golden/README.md`; `lanes/m0-conventions/write_golden.py` is retired: do not run it).
The reference parser and printer is `proto/text_grammar.py`. The library reads and prints the value
form of the types of milestone 1 in `src/text.c` (`include/adelefeld/text.h`). This lane adds the three
types of milestone 2, and gives the driver `adf` arithmetic on them, so that a user can type ideles at
the prompt.

Functions of the slice (names after `text.h`; the final list is yours):
- For `adf_ucoset`, `adf_idele`, `adf_idclass`: `set_str` (parse, with the limits argument of the other
  readers) and `get_str` (print), with the round-trip rules of conventions 9.6; the classification
  `adf_text_classify` recognises the three kinds (conventions 9.7); the printing of the real part by
  the constrained printing of conventions 9.5 (finding 3 of `lanes/i-slice1/result.md`: `arb_get_str`
  hides the sign of a certified positive ball; do not use it).
- The dump form (conventions 10) of the three types ONLY if it is small after the value form is done;
  else say that it is left, in the result file.
- The driver `tools/adf/adf.c`: the operations `show`, `type`, `mul`, `div`, `neg` where it has a
  meaning, `equal` where the library has the predicate, accept the three new kinds; new operations
  `inv X`, `pow X with K`, `powtight X with K`, `norm X`, `class X` (idele to class), `idele Q` (rational
  to idele), `hull X` and `hullsimple X` (idele to adele), `unitof A` (adele to idele), `div A with X` for
  an adele by an idele, `valuation X with P`, `abs X with PLACE`. One line of output for each command,
  a value text or `error: <STATUS>`.

Decisions where the conventions are silent are yours (TJO: decide and go on): the one a careful
designer would take, listed in the result file with the alternatives. Where the conventions and the
code disagree (known: conventions 5.7 names the accessors `adf_idclass_t_get`, the code
`adf_idclass_get_t`; conventions 3.2 lacks `LIMIT` for ideles), the code stands and you list the
finding.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold
for `text.h`: you add declarations; rule 5 is replaced by item 4 below), `docs/conventions.md` 8, 9, 10,
11, 5.6, 5.7, `tests/golden/README.md`, `include/adelefeld/text.h` and `src/text.c` (the structure of the
reader: grammar before limits, M1-D9; the limits), `docs/reviews/m1/text/review.md` (what a review of the
text reader found: hostile input), `include/adelefeld/ucoset.h`, `idele.h`, `idclass.h`, `idpow.h`,
`idmap.h`, `docs/api-2.md`, `tools/adf/adf.c`, `tools/adf/README.md`, `docs/SPEC.md` 15 (M1-D1, M1-D6,
M1-D7, N-D1, N-D10).

**You own:** `include/adelefeld/text.h` (additions), `src/text.c` (additions; an existing function is
changed only where the classification needs it), `src/text_idele.c` (new), `tests/test_text_idele.c`
(new), `tests/julia/text_idele.jl` (new), `tools/adf/adf.c`, `tools/adf/README.md`,
`tests/driver/i-*.cmd` and `i-*.out` (new), `proto/text_grammar.py` (only if it is wrong or lacks a
type; say what you changed), `tests/fuzz/fuzz_text_idele.c` (new, a target in the way of the existing
`tests/fuzz/fuzz_*.c`), `docs/api-2.md` (a new section 4), `lanes/t-slice1/`. In `tests/test_julia.sh` you
may add the lines your file needs. Everything else is read-only.

1. Header first. 2. Tests first, red then green (`lanes/t-slice1/redgreen.log`): every line of the three
   golden files (parse, print, round trip, the refusals with their statuses); the reference
   `proto/text_grammar.py` on 2000 generated texts (valid and invalid) against the C reader, status
   and value; hostile input: a modulus of 100000 digits, nesting, missing brackets, NUL bytes, a
   length of 0, texts cut at every position of a valid text (each prefix is refused or is a valid
   text of its own, never a crash); the driver cases with expected lines written BY HAND from
   mathematics checked in Python with exact rationals, the script in your lane directory.
3. The code. `tests/julia/text_idele.jl`: a user parses two ideles from text, multiplies them, prints
   the product.
4. Show that the tests bite: four faults of your choice in a scratch copy under `build/`. The fuzz
   target is built and run for 120 s under `timeout` with `ulimit -v 4000000`: a smoke test, say so.
   No mutation run.
5. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh`, `SAN=1 sh tests/test_driver.sh` pass, each under
   `timeout 900`, in the foreground; give the last line of each.

Result: `lanes/t-slice1/result.md` and the same text as your final message. Times in your notes are
read from `date`. Leave no compiled binary in your lane directory.
