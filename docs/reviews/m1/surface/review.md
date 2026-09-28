# Review of milestone 1, reviewer `surface`: driver, common functions, places, statuses, tools

NO BLOCKER FOUND. Findings: 0 BLOCKER, 3 MAJOR, 12 MINOR.

Reviewer: Claude Opus (Anthropic), refute mode. Authors under review: pi models (space-bunny-alpha,
deepseek-flash); the repair of the mutation tool is by Claude sonnet (lane tools-mutate) and was reviewed
the same way. Worktree: `/home/tobias/Projects/adelefeld/.claude/worktrees/agent-a1450cd62e05c3d4f`, at
commit a19cd71. Every reproducer is under `docs/reviews/m1/surface/checks/` and was run; commands are given
from the repository root after `make -j2` and `make -C tools/adf` (and `SAN=1` for `build/adf-san`).

## Findings

### R1 MAJOR: the memory checker misses a use that stands before the init

- File: `tools/memcheck/check_uninit.py:494-519` (`Analyzer.pop`, the test at line 496) with `record_use`,
  lines 527-531.
- Requirement: the docstring, lines 10-11: "a local ... is used (read or passed to a function other than an
  init) before any init call with it"; `tools/memcheck/README.md`, "The three categories", same words.
- Behaviour: first use and first init are recorded separately; `use-before-init` is reported only when the
  variable has no init at all in its scope (`var.init_line is None and var.use_line is not None`). A read
  that comes before the init in straight-line code is not reported.
- Input: `checks/memcheck/s1_use_then_init.c` (`fmpz_add_ui(a, a, k); fmpz_init(a);`) and
  `checks/memcheck/s4_adf_use_then_init.c` (`adf_fball_add(x, x, y); adf_fball_init(x);`).
- Reproducer: `sh docs/reviews/m1/surface/checks/memcheck/run_snippets.sh` compiles each snippet, runs the
  checker, and runs the snippet under valgrind to prove the defect is real:

      s0_control.c: checker use-before-init 1, clear-before-init 1; valgrind exit 77, 1 error line(s): ...
      s1_use_then_init.c: checker use-before-init 0, clear-before-init 0; valgrind exit 77, 3 error line(s):
          Conditional jump or move depends on uninitialised value(s)
      s4_adf_use_then_init.c: checker use-before-init 0, clear-before-init 0; valgrind exit 77,
          137 error line(s): Conditional jump or move depends on uninitialised value(s)

  `s0_control.c` (no init at all) is the one shape it catches. A one-method variant that reports a use
  before the first init in text order, `checks/memcheck/ordered_check.py`, finds s1 and s4 and finds 0
  such uses in `src/*.c tests/*.c tools/adf/adf.c`: the tree has no defect of this shape today; the tool
  would not have told.

### R2 MAJOR: `mutate.py --keep` reports every mutant that fails to build or times out as killed

- File: `tools/mutate/mutate.py:827-838` (the `args.keep` branch of `run`).
- Requirement: the docstring, lines 46-51: "killed: the tests fail ... not compiled: the mutant does not
  build; timed out: the tests do not finish in the time given". `--keep` is documented as keeping the scratch
  copies (line 752), not as changing the judgement.
- Behaviour: with `--keep` the status is `"survived" if code == 0 else "killed"`; the compile-error test and
  the timeout (`code is None`) are skipped. A mutant that was not built, or whose test never finished, is a
  false kill.
- Input: the tree `checks/mini/` (three functions; `q - p` on two pointers mutates to `q + p`, which does not
  compile; `n - 1` in a count-down loop mutates to a loop that never ends).
- Reproducer:

      python3 tools/mutate/mutate.py --root docs/reviews/m1/surface/checks/mini \
          --scratch build/review_mutate_mini --files src/mini.c --timeout 5 --equivalent /dev/null
      -> 12 mutants: 6 killed, 0 survived, 4 not compiled, 2 timed out, 0 excused
      (the same command with --keep)
      -> 12 mutants: 12 killed, 0 survived, 0 not compiled, 0 timed out, 0 excused

  Of the 4 "not compiled" of the first run, 1 is real (`invalid operands to binary +`) and 3 are R5.

### R3 MAJOR: 45 of 76 excuses in `tools/mutate/equivalent.txt` match no mutant at HEAD; one excuses another

- Files: `tools/mutate/mutate.py:170-171` (the key is `(path, line, kind)`) and 849 (the excuse);
  `tools/mutate/equivalent.txt`, e.g. lines 72, 83, 108.
- Requirement: equivalent.txt's header: each line excuses the mutant its reason describes; the commit
  ce1fb15 says "excused mutants of scaled.c and fball_local.c taken into equivalent.txt", and the commit
  ec9112e reports "0 survived, 28 excused" for scaled.c.
- Behaviour: the key is a line number. `src/fball.c` and `src/scaled.c` changed after the lines were written:
  17 entries of fball.c and 28 of scaled.c match no mutant. `make mutate FILES=src/scaled.c` now reports the
  excused survivors as survivors and fails. Worse, `src/fball.c:142:drop_call` (reason: "fmpz_zero(x->H) in
  adf_fball_init") now keys the drop of `fmpq_set_fmpz_frac(N, x->H, x->d);` in `fb_radius`, a different
  statement: if that mutant survived, the tool would call it "excused" under a reason about another line.
- Reproducers:

      python3 -B docs/reviews/m1/surface/checks/equiv_keys.py
      -> 45 lines "STALE (no mutant has this key)", 31 lines "LIVE", among them
         LIVE src/fball.c:142:drop_call -> drop_call: 'fmpq_set_fmpz_frac(N, x->H, x->d);' -> '' ...
              reason: fmpz_zero(x->H) in adf_fball_init: fmpz_init sets the value to zero ...
         entries 76, covering more than one mutant 0, stale 45
      python3 docs/reviews/m1/surface/checks/stale_excuse.py     (runs two mutants with the tool's own code)
      -> excuse entries: True, True
         src/scaled.c:201:9 swap_args: 'fmpz_mul(t, d, K)' -> 'fmpz_mul(t, K, d)': tests say survived;
             the tool reports SURVIVED (key src/scaled.c:201:swap_args in equivalent.txt: False)
         src/scaled.c:54:5 drop_call: 'fmpq_zero(x->s);' -> '': tests say survived; the tool reports SURVIVED

  The mutation result of work packages 1.4 and 1.7 is therefore not reproducible at HEAD. A key by line
  number cannot survive an edit; a key on the mutant's text (old and new token and a hash of the line) would.

### R4 MINOR: the memory checker is path-, element- and jump-insensitive, and the README does not say so

- File: `tools/memcheck/check_uninit.py` (whole analysis; `argument_base`, lines 387-397).
- Requirement: `tools/memcheck/README.md`, "Limits", lists macros, pointers, struct fields and cross-file
  initialisers; it does not list branches, array elements or `goto`.
- Inputs and reproducer (same script as R1):

      s2_init_on_one_branch.c (`if (flag) fmpq_init(q); fmpq_set_si(q, 7, 3);`): checker 0; valgrind exit 77
      s3_array_element.c (`fmpz_init(v[0]); fmpz_add(v[0], v[0], v[1]);`): checker 0; valgrind exit 77
      s5_goto_skips_init.c (`goto done;` over `arb_init(x)`, then `arb_clear(x)`): checker 0 (also 0
          clear-before-init); valgrind exit 77

### R5 MINOR: a killed mutant is reported "not compiled" when the failing test's name ends in `error`

- File: `tools/mutate/mutate.py:149` (`COMPILE_ERROR = r"(error:|...)"`) and 719-722; the failure line of
  `tests/test_runner.h:86` is `FAIL file:line: <test name>: check failed: ...`.
- Requirement: docstring lines 48-50 (killed means the tests fail). Lane m1-text reported this pattern
  (`lanes/m1-text/report.md:60`); the tool is unchanged. Tests of the tree with such names:
  `tests/test_text_rat.c:322` `a_nul_byte_anywhere_is_a_parse_error`, `:339`
  `every_byte_outside_the_alphabet_is_a_parse_error`.
- Reproducer: the first command of R2 prints

      NOT COMPILED src/mini.c:16:13 zero_one: '1' -> '0'
        FAIL tests/test_mini.c:19: a_non_digit_is_a_parse_error: check failed: mini_digit('x') == -1

  three times. The effect is a lower kill count; it cannot hide a survivor.

### R6 MINOR: a mutation run stopped with SIGTERM leaves its looping test programs running and its scratch files

- File: `tools/mutate/mutate.py:675-696` (`run_make`: the child is in its own session) and 778-782 (the
  `finally` does not run on SIGTERM).
- Requirement: `run_make`'s docstring (issue adf-98j: a looping mutant must not be left running) and the
  cleanup promised at lines 771-772.
- Reproducer: `python3 docs/reviews/m1/surface/checks/mutate_sigterm.py`

      tool exit code after SIGTERM: -15
      files left under the scratch directory: 12
      mutant test programs still running: 2

  (the script kills the two programs and removes the directory afterwards). `timeout`, CI runners and
  `kill` send SIGTERM; the processes are orphans in their own session and run until killed by hand.

### R7 MINOR: the print guard refuses a real ball whose binary exponent is exactly 100000

- File: `tools/adf/adf.c:205-218` (`adf_drv_exp2_ok`), constant at line 60.
- Requirement: SPEC 15.2, M1-D1 (`docs/SPEC.md:860`): "refuses to print a real ball whose binary exponent
  exceeds 100000 in absolute value". FLINT stores the exponent `e` with `x = m 2^e`, `0.5 <= |m| < 1`
  (`refs/src/flint-3.0.1/arf.rst:14-20`), which is one more than the usual exponent (`1 <= m < 2`) for which
  `2^100000` has exponent 100000. The driver compares FLINT's `e` with 100000, so `2^100000` (e = 100001) is
  refused, and on the small side the exponent -100001 of the usual reading is accepted.
- Input: `show (<the 30103 digits of 2^100000> ; 0)`, the same as a radius and as an imaginary part.
- Reproducer: `python3 docs/reviews/m1/surface/checks/hostile.py`, first four cases:

      ok       guard: mid 2^99999      rc=0 ['(4.9950104650719225397e30102 +/- 2.1e30082 ; 0)']
      MISMATCH guard: mid 2^100000     rc=1 ['error: LIMIT']
      MISMATCH guard: rad 2^100000     rc=1 ['error: LIMIT']
      MISMATCH guard: mid 2^100000 in cadele imag  rc=1 ['error: LIMIT']

  The expected texts were computed by `oracle.r_print` (conventions 9.5). Either the constant or the words
  of M1-D1 should name the convention.

### R8 MINOR: `prec` accepts 100001 to 1000000, where no rounded real result can be printed

- File: `tools/adf/adf.c:64` (`ADF_DRV_PREC_MAX`) against the guard, lines 205-218; `tools/adf/README.md:66`.
- Behaviour: at `prec` above about 100000 the radius of every rounded result is below `2^-100000`, so every
  inexact real result is `error: LIMIT`; only exact values print.
- Reproducer: `build/adf docs/reviews/m1/surface/checks/guard_example.cmd`, last five lines:

      prec 100100 / div (1 ; 0) with 3   -> error: LIMIT
      prec 100100 / add (1 ; 0) with 1/3 -> error: LIMIT
      prec 99000  / div (1 ; 0) with 3   -> (0.33333333333333333333 +/- 3.4e-21 ; 0)

### R9 MINOR: a setting with a trailing blank or a trailing CR is `error: PARSE`

- File: `tools/adf/adf.c:476-501` (`adf_drv_setting` accepts only `-?[0-9]+`).
- Requirement: `tools/adf/README.md:46-50`: "a command may be written with spaces around it"; conventions
  8.2 (`docs/conventions.md:1043-1046`): CR is whitespace. Every value operand accepts both (the driver
  passes them to the value parser), so a CRLF script runs every value command and fails every setting.
- Reproducer: `python3 docs/reviews/m1/surface/checks/hostile.py`:

      MISMATCH prec 64 CRLF            rc=1 ['error: PARSE', '7/3']
      MISMATCH prec 64 trailing blank  rc=1 ['error: PARSE', '7/3']

### R10 MINOR: UNSUPPORTED is not "decided before every other check"; reconstruct's end points bypass the checks

- File: `tools/adf/adf.c:762-770` (the second operand is parsed, and its error returned, before the type of
  the first is looked at) and 871-885 (operands 2 and 3 of `reconstruct` go straight to `adf_rat_set_str`).
- Requirement: `tools/adf/README.md:84-87` ("gives `error: UNSUPPORTED`, and that is decided before every
  other check") and line 103 ("Every other pair is `error: DOMAIN`", as `cap` does for a ball as its cap).
- Reproducer: `build/adf docs/reviews/m1/surface/checks/status_order.cmd` (README expects UNSUPPORTED on the
  first five lines, DOMAIN on the last three):

      add [5 mod 6] with 1/0                               -> error: DOMAIN
      add 1/0 with [5 mod 6]                               -> error: DOMAIN
      add [5 mod 6] with (1e100001 ; 0)                    -> error: LIMIT
      reconstruct (* ; 1 mod 2) with [5 mod 6] with 1      -> error: PARSE
      reconstruct (* ; 1 mod 2) with 0 with [5 mod 6]      -> error: PARSE
      cap (* ; 3 mod 12) with (* ; 1 mod 2)                -> error: DOMAIN
      reconstruct (* ; 1 mod 2) with (* ; 0 mod 1) with 1  -> error: PARSE
      reconstruct (* ; 1 mod 2) with (0 ; 0) with 1        -> error: PARSE

### R11 MINOR: the README's example of the guard on a computed value does not show it, and no test covers it

- File: `tools/adf/README.md:131-132`; `tests/driver/07_status.cmd` has only `show` cases of the guard.
- Behaviour: in `add (1e50000 ; 0) with (1e50000 ; 0)` each operand alone is already over the guard (binary
  exponent about 166097), so the example is refused whether or not the result is checked. The driver does
  check results (it is correct), but no expected line depends on it.
- Reproducer: `build/adf docs/reviews/m1/surface/checks/guard_example.cmd`, first four lines:

      show (1e50000 ; 0)                   -> error: LIMIT
      add (1e50000 ; 0) with (1e50000 ; 0) -> error: LIMIT
      show (1e20000 ; 0)                   -> (9.9999999999999999994e19999 +/- 6.1e19980 ; 0)
      mul (1e20000 ; 0) with (1e20000 ; 0) -> error: LIMIT

### R12 MINOR: `tests/test_dlopen.c` passes with 0 checks when the shared object is absent

- File: `tests/test_dlopen.c:96-101` and 121-125; the path `build/libadelefeld.so` is relative (line 37).
- Requirement: COMMON.md item 3 (a test that cannot fail). `make check` does not build the `.so`
  (`tests/test_exports.sh` does, line 59), and the mutation tool's copies hold no `build/`, so in a fresh
  `make check` and in every mutant run the test checks nothing. A stale `.so` from an older tree is loaded
  without a check of its age.
- Reproducer (run from a directory without `build/libadelefeld.so`):

      cd docs/reviews/m1/surface/checks && ../../../../../build/test_dlopen
      ->    build/libadelefeld.so is not there (run sh tests/test_exports.sh first); nothing to load
         ok   the_shared_object_is_loaded ... 2 tests, 0 checks, 0 failed checks, 0 failed tests   (exit 0)

  Only `adf_version_check` of the five looked-up functions is checked with `dladdr` (lines 150-155), though
  the header comment (lines 8-10) claims it for all five.

### R13 MINOR: `tests/test_exports.sh` does not judge exported names that begin with an underscore

- File: `tests/test_exports.sh:102` (`grep -v '^_'`) and 136-138.
- Requirement: the script's own rule (lines 17-22): an exported symbol that no header declares is a fault;
  conventions 4.1 item 5 (`docs/conventions.md:237`) makes `_adf_*` functions part of the interface.
- Reproducer: `python3 docs/reviews/m1/surface/checks/exports_underscore.py` (a copy of the tree with one
  extra file under `build/review_exports_tree`):

      _adf_undeclared exported: exit 0
          == exported names that begin with an underscore, not judged
          _adf_undeclared
          test_exports: passed: ... no exported name is undeclared, no variadic function
      adf_undeclared exported: exit 1

### R14 MINOR: `tests/test_place.c` says it tests 2^64; it tests 2^64 - 2

- File: `tests/test_place.c:2-4` against lines 13-15. 2^64 is not a `ulong`; the array holds
  `18446744073709551614UL` (2^64 - 2). The comment is wrong; the test itself is fine.
- Reproducer: none is needed; the two places of the file are the evidence (`sed -n '2,4p;13,15p'
  tests/test_place.c`).

### R15 MINOR: `tests/driver/09_settings` pins FLINT's rounding where the conventions ask only for an enclosure

- File: `tests/driver/09_settings.cmd:5-9, 19-21`, expected lines 2 and 6 of `09_settings.out`
  (`(1.5 +/- 0.13 ; 1)`, `(2 +/- 0.63 ; 1)`).
- Requirement: conventions 9.5 "Reading" (`docs/conventions.md:1210-1214`): the `arb` must contain the
  interval; it must be exact only when the odd mantissa has at most `prec` bits. 1.625 has the odd mantissa
  13 (4 bits) and `prec` is 2, so any enclosure is conforming; the lines agree with round-to-nearest in
  FLINT, not with a rule of the specification. The driver README says the lines were written from the
  specification. I computed both lines from 9.5 given FLINT's rounding and they are right for it.
- Reproducer: the argument above; `tests/test_driver.sh` passes, so the program gives these lines.

## What I examined and found correct

- **Expected lines of the driver scripts.** An independent oracle in exact rationals (`checks/oracle.py`:
  SPEC 4.2 predicates, SPEC 4.3 tight sum and product, SPEC 4.4 cap, conventions 9.4 templates and the 9.5
  printer; nothing imported from the code or from `tests/ref`) recomputed every command line of scripts
  01-08 and 10: 299 lines, 221 modelled and all equal, 0 different
  (`python3 docs/reviews/m1/surface/checks/replay_scripts.py`). The 78 lines it does not model
  (statuses, `type`, `reconstruct`, unsupported kinds) I checked by hand against conventions 3.1, 3.2, 9.3,
  9.7 and SPEC 9.2. Scripts 01-05 have 58 command lines; 56 are modelled and equal, the other 2 are
  `error: UNSUPPORTED` for a local and a partial ball, which is right. I found no line of 01-05 that agrees
  with the program but not with the specification. Script 09 I recomputed by hand from 9.5 (7 lines).
- **Differential test of the driver and the library behind it.** `checks/fuzz_diff.py`: random commands of
  `add sub mul neg div equal contains overlaps cap show` over the four types, rationals with denominators up
  to 25, radii up to 36 with fractional radii, decimals with and without `+/-`. Exact text for rationals,
  finite balls, predicates and statuses; for real and complex coordinates, enclosure of the exact interval
  by the printed interval and, for dyadic inputs, the exact 9.5 text. Seeds 1, 2, 3: 60000 commands,
  0 failures (48565 exact-text checks, 8830 enclosure checks, 2605 enclosure-and-text checks); seed 8 on
  `build/adf-san`: 20000 commands, 0 failures, no sanitizer report; seed 7 under valgrind: 3000 commands,
  0 failures, no valgrind error.
- **Hostile input** (`checks/hostile.py`, 36 cases, on `build/adf` and on `build/adf-san` with
  `ASAN_OPTIONS=exitcode=99`): lines of 65536 bytes with CRLF, 65537 bytes followed by a valid line, the
  separator at the start, at the end, twice, with tabs, in upper case, an operand of blanks only, four and
  five operands, NUL in the operation word and after the separator, a NUL-only line, a vertical tab,
  `prec` with 26 digits, leading zeros, `-0`, `+5`, a 65000-digit rational, a product of two
  16000-digit balls, a 64000-digit decimal, `prec 1000000` with `digits 1000000`, exponents near the guard.
  No crash, no sanitizer report, every case under 0.1 s; the mismatches are R7, R9, R10. The driver's
  `ferror` path works: `build/adf tests/driver/06_pairs.cmd > /dev/full` exits 2.
- **Memory of the driver**: `valgrind --leak-check=full` over all ten scripts (the concatenation): no error,
  no definite leak; the "possibly lost" blocks are FLINT's mpz cache (the driver does not call
  `flint_cleanup`). `SAN=1 sh tests/test_driver.sh`: 23 cases, 100320 lines, all equal.
- **Reconstruction through the driver** (`checks/reconstruct_probe.cmd`, 10 commands with radii up to
  `10^100000` and moduli up to `10^50`): all answers equal to my hand computation, no delay.
- **The driver's line splitting and arithmetic dispatch**, read line by line: rational on the left of `sub`
  (`q - x = -(x + (-q))`), the embedding of an adele into `C x A_f` for both operand orders, `div` only by
  an exact rational, `NOT_UNIT` for 0, the arity table, the setting parser's overflow guard (no overflow up
  to any number of digits), the exit statuses.
- **`src/place.c`**: `checks/place_check.py` builds `checks/place_check.c` against the library and compares
  `adf_place_prime` with my own deterministic Miller-Rabin (bases 2 to 37) on 156016 numbers (0 to 2999,
  2^64 - 2999 to 2^64 - 1, 100000 random 64-bit, 50000 random 32-bit, strong pseudoprimes to several
  bases, Carmichael numbers, products of two primes near 2^32): 5063 primes, 0 disagreements; the output is
  untouched on DOMAIN (sentinel checked for every composite); `adf_place_cmp` and `adf_place_equal` against
  the numeric order on 2000 x 2000 pairs and against the archimedean place: 0 wrong.
- **`src/status.c`, `include/adelefeld/status.h`**: the eleven values and names against conventions 3.1
  (read); the static asserts compile.
- **`src/common.c`**: `adf_version_major_minor_equal` read against conventions 12.11; the excused mutant
  at line 96 is equivalent (I checked the argument: with the mutant no zero is stripped and the comparison
  pads with '0'). `adf_str_free(NULL)` calls `flint_free(NULL)`, which is `free(NULL)` with the default
  allocator.
- **`src/inlines.c`**: every `ADF_INLINE` function I found in the headers is in the export list of
  `sh tests/test_exports.sh` (172 exported, 21 dump functions declared and not yet written, 0 extra).
- **Citations in `adf.c`**: `arf.h:87` is `ARF_EXP` and `mag.h:114` is `MAG_EXP` in the installed FLINT
  3.0.1 header (the refs copy holds only the `.rst` files); correct.
- **Mutation tool**: `python3 tools/mutate/selftest.py` passes (the weak test leaves a survivor, the strong
  one none, no process left). The normal (non-`--keep`) run classifies a compile error and a timeout
  correctly (R2, first command). A normal run leaves nothing under its scratch directory.

## What I did not examine

- `tools/memcheck/run.sh` in its valgrind mode (valgrind is now on the PATH, which the README says it is
  not): a run over all 30 test programs exceeds the 3-minute budget. I ran valgrind on the driver only.
- A full `make mutate` of any file (budget); I ran the tool on the mini tree and 2 mutants of `scaled.c`.
- The libFuzzer target `tests/fuzz/fuzz_driver.c` was read, not run.
- The text parsers and printers themselves (`src/text.c`) beyond what the driver reaches, the dump form,
  contexts and the local backend: other reviewers.
- The Julia layer and `tests/test_julia.sh`.
