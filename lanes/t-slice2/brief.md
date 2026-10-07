# Lane t-slice2: the value form of local balls and partial balls (`adf_lball`, `adf_sball`) in the library

`docs/conventions.md` section 9 fixes the grammar of the value form of every type (9.2: `lball_v`, `sentry`,
`sball_v`, `lcoord`; 9.3 the semantic constraints; 9.4 the printing templates; 9.5 real balls; 9.6 round trips;
9.7 the type of a text). Golden vectors exist: `tests/golden/lball.tsv` (55 lines) and `tests/golden/sball.tsv`
(29 lines); read `tests/golden/README.md`. The reference parser and printer is `proto/text_grammar.py` (it has
the types `lball` and `sball`). The library reads and prints the value form of the types of milestones 1 and 2
in `src/text.c` and `src/text_idele.c` (`include/adelefeld/text.h`); the classification `adf_text_classify`
already names `ADF_TEXT_LBALL` and `ADF_TEXT_SBALL`. What is missing is the reader and the printer of the two
types. You add them, as lane t-slice1 added the three types of milestone 2 (`lanes/t-slice1/brief.md` and
`result.md` are the pattern; `src/text_idele.c` and `tests/test_text_idele.c` are the code to imitate).

Functions (names after `text.h`):
- `int adf_lball_set_str(adf_lball_t x, const char * s, size_t len, const adf_text_limits_t * lim);`
  `char * adf_lball_get_str(size_t * len, const adf_lball_t x);`
- `int adf_sball_set_str(adf_sball_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim);`
  `char * adf_sball_get_str(size_t * len, const adf_sball_t x, slong digits);`
  (`prec` and `digits` as the adele reader and printer take them, for the entry `inf: ...`; a complex `inf`
  entry: look at what `sball.h` can store and at what the golden file expects; if the library cannot store it,
  the reader returns the status the conventions name and you say so in the report.)
The statuses are those of conventions 9.3 and of the existing readers (grammar before limits, decision M1-D9;
`UNSUPPORTED` for `p >= 2^64`; `DOMAIN` for a prime twice or two `inf` entries; the limits of
`adf_text_limits_t`); on a status other than `OK` the output is untouched. The printing of the real part is the
constrained printing of conventions 9.5 that `src/text.c` already has (do NOT use `arb_get_str`).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for `text.h`: you ADD
declarations; rule 5, mutation testing, is replaced by item 4 below), `docs/conventions.md` 5.8, 5.9, 7, 9
(all of it), `tests/golden/README.md`, `lball.tsv`, `sball.tsv`, `include/adelefeld/text.h`, `src/text.c`,
`src/text_idele.c`, `tests/test_text_idele.c`, `include/adelefeld/lball.h`, `sball.h`, `place.h`,
`proto/text_grammar.py` (lines 465-483, 835-840 and what they call), `docs/api-2.md` section 4 (how t-slice1
documented its functions), `tools/adf/adf.c` lines 1660-1700 (`adf_drv_put_sball`: the driver prints a partial
ball with its own code today; READ ONLY, another lane is editing this file).

**You own:** `include/adelefeld/text.h` (additions only), `src/text_local.c` (new), `tests/test_text_local.c`
(new), `tests/julia/text_local.jl` (new), `docs/api-1f.md` (one new section at the end, the two readers and the
two printers: what they accept, every status, the round-trip statement), `lanes/t-slice2/`. In
`tests/test_julia.sh` you may add the line your file needs. `src/text.c` is read-only unless a static function
you need must become shared: then move NOTHING, and say in the report what you would share (copy the few lines
you need into your file with a comment naming the origin). `tools/adf/adf.c` and everything else are read-only.
No git command that changes state, no `bd`. At most 2 cores; every command under `timeout`; build into
`BUILD=lanes/t-slice2/build` (for example `timeout 600 make -j2 BUILD=lanes/t-slice2/build
lanes/t-slice2/build/test_text_local`); `make check-all` only once, at the end.

1. Header first: the comment block of each of the four declarations (the grammar rule with the section of the
   conventions, every status and the input that gives it, the output untouched on failure, who frees the
   string, the round trip).
2. Tests first, red then green (`lanes/t-slice2/redgreen.log`; a link error counts only for the first test):
   - every line of `tests/golden/lball.tsv` and `sball.tsv`: parse, print, compare with the canonical text of
     the second column, and the refusals with their statuses (read the README for the format of the files);
   - round trips (conventions 9.6): for 2000 generated values (primes 2, 3, 5, 7, 65537, `2^64 - 59`; exact
     rationals and balls; exponents negative, zero and positive; several places; the empty set of places),
     `set_str(get_str(x))` equals `x` (use the library's equality or compare the fields);
   - the reference `proto/text_grammar.py` on 1000 generated texts, valid and invalid (a script in your lane
     directory writes them with the reference's verdict to a file; the C test reads the file and compares
     status and canonical text);
   - hostile input: a prime of 100 digits, an exponent of 100 digits, `O(5^` cut at every position, missing
     brackets, NUL bytes, a length of 0, every prefix of three valid texts (each prefix is refused or is a
     valid text of its own; never a crash), 10000 entries in one partial ball against the item limit.
3. The code. `tests/julia/text_local.jl`: a user parses `[p=5: 3 + O(5^4)]` and a partial ball from text and
   prints them again.
4. Show that the tests bite: four faults of your choice in a scratch copy under your build directory (for
   example: the sign of the exponent in `O(p^N)` dropped; the canonical order of places not enforced; the
   output written before the last check; `p=4` accepted), each with the test that fails. No mutation run.
5. Final checks: `timeout 900 make -j2 check-all` once in `build/` (give its last line); one build of
   `test_text_local` with `SAN=1` in `BUILD=lanes/t-slice2/build-san` and its run with
   `ASAN_OPTIONS=detect_leaks=1` (give its last line).

Decisions where the conventions are silent are yours (the one a careful designer would take), listed in the
report with the alternatives. Where the conventions, the golden files, the reference and the existing code
disagree, do not paper over it: write the case down under "Findings".

Report: `lanes/t-slice2/report.md`, written once, at the end: what was built (functions, files, lines); the
decisions; every check with its command and its numbers, and what would have made a case fail; the fault table;
what is not done; findings. Write running notes in `lanes/t-slice2/progress.md` as you go. Your work counts as
finished only when `report.md` exists. Leave no compiled binary in your lane directory outside `build*/`.
