# Lane m1-text: the value form, parser and printer (work package 1.4, first part)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `rat.h`, `fball.h`, `adele.h`,
`text.h` completely, `docs/api-m1.md`; `docs/conventions.md` sections 3, 4.2 to 4.4, 8 and 9 completely, 11
and 12.8; `docs/SPEC.md` section 10; `proto/text_grammar.py` (the reference implementation) and
`proto/test_text_grammar.py`; `tests/golden/README.md`; `tests/README.md` (golden reader, fuzzing). The dump
form (`dump.h`) is not yours; another lane writes it later.

This code reads untrusted input. It is the part of the library where a memory error is most likely.

**You own:** `src/text.c`, `tests/test_text_rat.c`, `tests/test_text_fball.c`, `tests/test_text_adele.c`,
`tests/test_text_classify.c`, `tests/fuzz/fuzz_text.c`, `tests/fuzz/corpus/text/`,
`tests/ref/vectors/m1-text/`, `bench/bench_text.c` (you may add its name to `bench/Makefile`).

Implement every function that `text.h` declares. `adele.h` is implemented by another lane at the same time:
do not call its functions. Reach the fields of `adf_adele_struct` and `adf_cadele_struct` directly
(`x->inf` with `arb` or `acb` functions, `&x->fin` with the functions of `fball.h`); in tests, initialise
and clear such values field by field. `adf_str_free` is also written by another lane: in your tests free
strings with `flint_free` and say so in a comment.

Rules of this lane:
- The parser is written by hand over `(s, len)`; it never relies on a NUL terminator, never calls `strlen`,
  `strtol`, `sscanf` or `atoi` on the input, and never passes raw input to a FLINT string function. A number
  literal is copied into a buffer of its own, after the grammar and the limits have accepted it, and only
  then given to `fmpz_set_str`. Real balls are built by your own code from the validated decimal literals
  with exact rational arithmetic and outward rounding (conventions 9.5); `arb_set_str` and `arb_load_str`
  are not called.
- The order of checks of conventions 8.5 is part of the interface: a text with two faults gets the status of
  the earlier stage. Test it with texts that have one fault of each pair of stages.
- Output is built in a temporary and swapped in only on `ADF_OK`.
- No recursion whose depth depends on the input; no allocation whose size is not bounded by the limits.

Tests: every row of `tests/golden/rat.tsv`, `fball.tsv`, `adele.tsv`, `cadele.tsv`, `dispatch.tsv`,
`realball_read.tsv`, `realball_print.tsv` through the golden reader of `tests/support/` (see conventions
11.3); do not change files under `tests/golden/` and do not run `lanes/m0-conventions/write_golden.py`.
Round trips of conventions 9.6 on random values (print, parse, identical set; parse, print, parse). Every
limit of 8.4 at the limit and one above. `len = 0`, `s = NULL` with `len = 0`, a NUL byte inside, bytes 128
to 255, input without a terminator placed at the end of a page or of a heap block of exact size (so that
the sanitizer sees a read past the end). Outputs untouched on every status.

Fuzzing: `tests/fuzz/fuzz_text.c` as `tests/fuzz/fuzz_support.c` does: the input goes to
`adf_text_classify` and to the four parsers; on `ADF_OK` the value is printed, parsed again and must be
identical; the predicates `is_canonical` must hold. Seed the corpus from the golden files. Run
`make fuzz FUZZ_TARGET=text FUZZ_SECONDS=170` and report the coverage table of `src/text.c`; name the
lines that the fuzzer does not reach.

Benchmark rows of PLAN row 1.4 (print, parse; word-sized and 4096-bit) in `bench/bench_text.c`; provisional.
Mutation run: `make mutate FILES=src/text.c JOBS=2 LIMIT=150`.
