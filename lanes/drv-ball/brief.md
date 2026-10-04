# Lane drv-ball: the driver reads and prints local balls and partial balls through the library

The driver `tools/adf/adf.c` (one file) has value kinds for `rat`, `fball`, `adele`, `cadele`, `ucoset`,
`idele`, `idclass`: a literal of the kind is read by the library reader (lines 380-400: `adf_rat_set_str`, ...)
and printed by the library printer (lines 415-440: `adf_rat_get_str`, ...). For local balls (`adf_lball`) and
partial balls (`adf_sball`) the library has had readers and printers since lane t-slice2
(`include/adelefeld/text.h` lines 286-342: `adf_lball_set_str`, `adf_lball_get_str`, `adf_sball_set_str`,
`adf_sball_get_str`; texts `[p=P: L]` and `{E; E; ...}`, `docs/conventions.md` 9.4), but the driver does not
use them as value kinds: `tools/adf/README.md` line 253 says "A kind with no typed parser in this build
(`adf_lball`, `adf_sball`, ...)", and the driver prints partial balls with its own code
(`adf_drv_lball_text`, line 1695; `adf_drv_put_sball`, line 1725). Line 2721 already calls
`adf_lball_set_str` for one command: read it to see how.

Task, in three small steps. Each step: FIRST a failing driver fixture (`tests/driver/<name>.cmd` with the
expected `<name>.out`; read `tests/driver/README.md` for the format, the `#!exit` metadata line and how the
runner is called), see it fail, then the code, see it pass. Keep a line in `lanes/drv-ball/progress.md` for
each red and each green run with the command.

1. **`lball` and `sball` as value kinds.** A line whose value is the text of a local ball or of a partial ball
   is read with `adf_lball_set_str` / `adf_sball_set_str` (precision: the session's `prec`, as for `adele`)
   and stored like the other kinds; `show` prints it with `adf_lball_get_str` / `adf_sball_get_str` (digits: the
   session's `digits`); `type` names it `lball` / `sball`. Follow exactly what the seven existing kinds do
   (the value union, init, clear, copy, the classifier `adf_text_classify` or whatever the driver calls to
   find the kind of a literal, equality if the other kinds have it). A status other than `OK` from the reader
   is printed as the driver prints it for the other kinds. Fixture `drv-ball-values`: at least twelve lines
   (a local ball at 2, 3, 5, `2^64 - 59`; negative valuation; an exact value; a partial ball with the real
   place and two primes; `{}`; and for each status of the two readers one line: `PARSE`, `DOMAIN` (a prime
   that is not prime, the same prime twice), `UNSUPPORTED` (`p >= 2^64`), `LIMIT`).
2. **The commands that already produce a partial ball** (`project`, `exp_at`, `log_at`, `sin_at`, ...,
   `powunit_at`: they call `adf_drv_put_sball`) accept a stored `sball` value as operand `X` where they now
   accept an adele and project it, if and only if this needs no new library function. Fixture
   `drv-ball-ops`: `project` then `exp_at` on the stored partial ball gives the same line as `exp_at` on the
   adele.
3. **One printer.** Compare, for every existing fixture under `tests/driver/` that prints a partial ball or a
   local ball, the driver's present output with the text `adf_sball_get_str` / `adf_lball_get_str` gives for
   the same value. Write the comparison as a table in `lanes/drv-ball/printer-diff.md` (fixture, line, driver
   text, library text). If the two are byte-identical in all cases: replace the bodies of `adf_drv_put_sball`
   and `adf_drv_lball_text` by calls of the library printers and delete the driver's own formatting code.
   If they differ anywhere: change NOTHING in this step, and report the table: the orchestrator decides.
   Do not edit an existing `.out` fixture in any case.

Then update `tools/adf/README.md` (the list of kinds, line 122 and line 253; the new operand form of step 2).

**You own:** `tools/adf/adf.c`, `tools/adf/README.md`, new files `tests/driver/drv-ball-*.cmd` and `.out`,
`lanes/drv-ball/`. Everything else is read-only: in particular `src/`, `include/` and the existing fixtures.
If a function you need is `static` in a file you do not own, or the library lacks a function (for example a
copy or an equality of `adf_sball`), do NOT copy code and do NOT write a private reimplementation: use what
the public headers offer, and if that is not enough, leave the step out and say so in the report under "not
done" with the declaration you would need.

Checks to run, each under `timeout`, at most 2 cores (`-j2`); give the command and the numbers in the report:
- `timeout 900 make -j2 BUILD=build build/adf` (or the target the Makefile has for the driver: read
  `Makefile` and `tools/adf/Makefile`), then the driver test target (`make check-driver` or what
  `tests/driver/README.md` names): all cases pass, the count before and after.
- The same driver build with `CC=clang` and with `INV=1` and with `SAN=1` in separate build directories
  (`BUILD=lanes/drv-ball/build-clang` and so on): no warning, and the new fixtures pass under `SAN=1`.
  Files end with a newline.
- `git diff --stat` at the end (read-only git is allowed): only files you own.
Remove `lanes/drv-ball/build-*` at the end; leave no files in `/tmp`.

Report: `lanes/drv-ball/report.md` (rule 8 of `lanes/COMMON.md`).
