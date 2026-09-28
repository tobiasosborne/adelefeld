# Lane m1-repair-dump: findings R1 to R4 of the review `dump`

Status: the code is written and the four dump test programs pass. The runs of `make check`, of the
sanitizer build, of the clang build, of the fuzzer, of the mutation run and of the review's
`verify.py unit` and `cost` are listed further down; the numbers are in this file, not in adjectives.

(This file is written in steps. Everything under "Checks" with a number is a number that was read
off a run.)

## 1. What was done

| Finding | Change | Where |
|---|---|---|
| R1 (M1-D5) | A context occurrence of more than `ADF_MODCTX_MAX_BLOCKS = 65536` blocks is `ADF_UNSUPPORTED` at stage 5, decided from the count in the text | `src/dump.c`, `dp_w_ctx` (line 616) |
| R2 (M1-D9) | The bound `ADF_DUMP_QCLASS_EXP_MAX = 2^20` on the binary exponents of the real ball of a piece of a `qclass` is a limit of stage 4, applied to every piece on the digit strings before any semantic check | `src/dump.c`, `dp_w_arb` (line 567), set in `dp_w_qclass` (line 805) |
| R3 | The length of a printed text is read in a statement of its own, after the call that writes it; `tl` is initialised | `tests/fuzz/fuzz_dump.c`, six places |
| R4 | TAB, LF and CR are bytes of the alphabet (8.2), so stage 2 lets them past; the header is judged next | `src/dump.c`, `dp_header` (line 1063) |

The same three repairs were made in the reference `proto/text_grammar.py` (the dump reader only),
and `proto/test_text_grammar.py` has a new test class `TestDumpReviewFindings` for them.

No other source file of the library was changed. `src/modctx.c` needed no change: the cap is
enforced inside the shared validation `adf_dump_ctx_occurrence` of `src/dump.c`, which
`adf_modctx_new_from_dump` calls before it checks the occurrence index and before
`adf_modctx_alloc`.

## 2. Files written

* `src/dump.c`: the three repairs (the only library file changed).
* `tests/test_dump_limits.c`: new, nine tests for R1 and R2.
* `tests/test_dump.c`: new test `header_before_the_whitespace_of_the_body` (R4).
* `tests/test_dump_golden.c`: the row counts of the golden file, which grew by ten rows.
* `tests/golden/dump.tsv`: ten added rows at the end (six for R4, four for R2). No other row touched.
* `tests/fuzz/fuzz_dump.c`: the six places of R3 and a note in the header comment.
* `proto/text_grammar.py`: the dump reader only: the alphabet of stage 2, the block cap of M1-D5, the
  exponent bound of M1-D9 as a limit of stage 4, and the module docstring.
* `proto/test_text_grammar.py`: the new class `TestDumpReviewFindings`, and the "adele" line of the
  new R2 test.
* `lanes/m1-repair-dump/red-green.log`, `lanes/m1-repair-dump/green.log`, and the programs under
  `lanes/m1-repair-dump/checks/` (see section 5).

## 3. Decisions that a reader of the contract has to make

### 3.1 The stage of the cap of M1-D5

The cap is placed in stage 5 of `conventions.md` 8.5, the stage of the "word restrictions".  A
count limit (`max_items`) is stage 4 and gives `ADF_LIMIT`; the cap of M1-D5 is a restriction of the
implementation, not a limit of the text, and the only statuses of 8.5 that are not `ADF_PARSE`,
`ADF_LIMIT` and `ADF_DOMAIN` are the `ADF_UNSUPPORTED` of stage 3 (another version or field) and of
stage 5 (`p` or `q` at least `2^64`).  The cap is of the second kind: it says that this
implementation does not build a context of more than 65536 blocks.  Consequences, all pinned by
`tests/test_dump_limits.c`:

* a text with 65537 blocks is `ADF_UNSUPPORTED` even when its context also fails the predicate of
  conventions 5.14 (the count is decided before the blocks are read into integers);
* a text with 65537 blocks and `max_items = 65536` is `ADF_LIMIT` (stage 4 comes first);
* a text with 65537 blocks and `max_items = 65537` is `ADF_UNSUPPORTED`;
* a text whose count is `2^40` or `2^64` and whose body is short is `ADF_PARSE` whatever
  `max_items`: a count fixes the number of repetitions that follow it (conventions 10.1) and a
  mismatch is a grammar failure of stage 3, so the cap is never reached.

This is the order of `include/adelefeld/modctx.h` as well: "The order of the checks is: ADF_DOMAIN
first, then ADF_UNSUPPORTED", with the size decided "before any table is built" and before the
coprimality.

### 3.2 What "a piece of a qclass" means for the bound of M1-D9

M1-D9 bounds "the real ball of a `qclass` piece".  The bound is applied to every piece of the form
`pieces` and not to the form `lift`.  Reasons, in order of weight:

1. the grammar of conventions 10.1 gives a "piece" only in the repetitions of the form `pieces`; a
   `lift` is a single `arch fb`, one adele, and has no piece;
2. the reason M1-D9 gives for the bound is the cost: "The check of the range of a piece forms the
   exact end points of its real ball, so its cost grows with the exponents."  A lift forms no
   range; only `dp_q_piece` forms `lo` and `hi` with `ARF_PREC_EXACT`;
3. `tests/ref/vectors/m1-dump/dump_ref.jsonl` (a file of another lane, read by
   `tests/test_dump_ctx.c`) contains `qclass lift` texts whose real balls have exponents above
   `2^20` and records them as valid with a round trip.  Bounding the lift form would make the C
   contradict those vectors, and the file is not this lane's to regenerate.

If TJO reads "piece" as including a lift, then the change is one line in `src/dump.c`
(`dp_w_qclass`: drop `&& pieces`) and one in `proto/text_grammar.py`, and the vectors named above
have to be regenerated.  This is listed under "Findings against the specification".

### 3.3 The bytes of a field token

A TAB, LF or CR after the field letter stays inside the field token: `field = upper, {nonspace}`
(conventions 10.1) and "nonspace" is every byte but `0x20`, so `adf1 Q\trat 1 1` writes the field
`Q\trat`, which is not `Q`, and 10.2 ("A reader of version 1 rejects any other version and any
other field with `ADF_UNSUPPORTED`") gives `ADF_UNSUPPORTED`.  A TAB where the grammar wants the one
space after the version, or instead of the field letter (the field must begin with an upper-case
letter), is `ADF_PARSE`.  The reference already behaved this way; both are pinned by
`tests/test_dump.c` and by the reference test.

## 4. The tests

(new section, filled in below)
