# tests/driver: the scripts of the driver test and their expected output

Every file `NAME.cmd` is a script of commands for the driver `adf` (tools/adf), and
`NAME.out` holds the line the driver is expected to write for each command.  A line
`#!exit N` in the script gives the expected exit status; without it the status is 0.
`tests/test_driver.sh` runs them all and stops at the first difference.

The expected lines were written by hand from `docs/SPEC.md` and `docs/conventions.md`
before the program existed, not copied from its output; the first comment of every script
names the part of the specification it is taken from.  Where a line was wrong, it is
corrected in the script and the correction is written down in
`lanes/m1-driver/report.md`; where the program was wrong, the program was changed.

| Script | What it takes from |
|---|---|
| `01_spec_4_1.cmd` | the table and the examples of SPEC 4.1 (values, canonical form, the two adele types) |
| `02_spec_4_2.cmd` | the table of SPEC 4.2 (three rows) and the text after it (radius zero, the non-transitive overlap) |
| `03_spec_4_3.cmd` | the five rows of the table of SPEC 4.3 (the tight operations) |
| `04_spec_4_4_cap.cmd` | item 3 of SPEC 4.4 (the absolute cap) and the paragraph on what is not a policy |
| `05_spec_4_5.cmd` | SPEC 4.5 (no division by an adele or by a finite ball) |
| `06_pairs.cmd` | every operation with every pair of the four types, the refused pairs included, and the unimplemented kinds |
| `07_status.cmd` | one command for every status the library can return through the driver |
| `08_show_type.cmd` | conventions 9.3 (canonicalisation on input), 9.4 (the templates) and 9.7 (the thirteen kinds) |
| `09_settings.cmd` | conventions 9.5 (the reading and the printing of a real ball) through `prec` and `digits` |
| `10_line_language.cmd` | the line language of the driver: comments, empty lines, whitespace, the separator |
| `11_guard.cmd` | the guard on printing: M1-D6 and the binary exponent FLINT stores |
| `12_status_order.cmd` | the order of the checks of one command, step by step |
| `13_dump.cmd` | `compare` (SPEC 4.2), `dump` and `load` (conventions 10) |

`hostile/` holds the expected output of the hostile inputs.  Their bytes are written by
`gen_hostile.sh` into `build/driver/`, because they hold NUL bytes, bytes 128 to 255, lines of
65536 and 65537 bytes, and trailing blanks, tabs and a CR that a diff cannot be trusted to
show:

| Expected file | Input | What it checks |
|---|---|---|
| `h1_nul.out` | `h1_nul.cmd` | a NUL byte in a line, in an operand and in the second operand |
| `h2_high.out` | `h2_high.cmd` | the bytes 128 to 255, in an operand, in an operation word, in a second operand |
| `h3_line_65536.out` | `h3_line_65536.cmd` | a line of exactly 65536 bytes, the largest the driver accepts |
| `h4_line_65537.out` | `h4_line_65537.cmd` | a line of 65537 bytes: the error for that line, and the rest of it skipped |
| `h5_no_final_newline.out` | `h5_no_final_newline.cmd` | a file without a final newline |
| `h6_empty.out` (empty) | `h6_empty.cmd` | an empty file: no line, status 0 |
| `h9_settings_ws.out` | `h9_settings_ws.cmd` | blanks, tabs and a CR around the number of a setting |
| (generated) | `h7_many_lines.cmd` | 100000 lines, timed; the expected output is written beside the input |
| (checked in the script) | a directory | `fopen` succeeds and the first read fails: a usage error, status 2, nothing on standard output |
