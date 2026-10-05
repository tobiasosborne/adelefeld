# Lane u-repair1: repair of the review u-review1 (the dump forms of the idele and the class)

Scope: the two BLOCKER findings of `docs/reviews/m2/review-dump-units.md` (F1, F2), finding F5
(two faults of the radius mantissa that the two test files did not notice), F3 (the exponents of a
dumped local ball), F4 (the blanks around the operand of the driver command `load`), part 3 of the
brief (no abort reachable from a text) and the fault table of item 6. Everything below was run on
this tree; no file outside the list of "You own" was changed.

## Files written

| File | What |
|---|---|
| `src/dump.c` | the repair of `dp_arb_sign` (F1, F2); the three loaders of lane u-dump1 build into a temporary and return `ADF_DOMAIN` instead of `flint_abort`; the two local loaders ask the predicate before the swap; `dp_w_arch` fills the zero ball when the archimedean count is 0; `dp_fits_si` states the word of a negative token |
| `include/adelefeld/dump.h` | the comment blocks of the declarations of the five kinds: the sign rule of 5.7 as the loader decides it, the exchange into the output on `ADF_OK` alone, and that no `ADF_LBALL_EXP_MAX` bound is applied to `v` and `N` |
| `tests/test_dump_units.c` | three new tests (F1 texts, F2 texts and their round trips, the family of the sign), the radius-mantissa cases of F5 for the idele and the class, the helper `ucoset_one` |
| `tests/test_dump_local.c` | `lball_exponent_word_and_prec`, `sball_exponent_and_radius_mantissa` (F3, the radius mantissa with the tags `r` and `c`, review F5) |
| `tests/driver/u-dump-units.cmd`, `.out` | four lines for F4 |
| `tools/adf/README.md` | the sentence on `load` (no change in `tools/adf/adf.c`: the trimming is in the line reader, shared by every command) |
| `lanes/u-repair1/redgreen.md` | the red and green runs, part by part |
| `lanes/u-repair1/faults.py`, `lanes/u-repair1/ft/dump.repaired.c`, `lanes/u-repair1/ft/dump.prerepair.c` | the fault runner of the review, adapted to this lane (F18 and F19 are now the reverse mutations that undo the two repairs; F20 and F21 are new controls), and the two sources it patches |

## Part 1: the tests

`tests/test_dump_units.c`, three new tests:

- `idele_negative_midpoint_domain`: the three texts of the review (F1) are `ADF_DOMAIN`, the
  inspector says the same, the output is left at the sentinel, and the exact arithmetic
  (`ball_excludes_zero`) agrees with each of them. A negative midpoint that excludes 0 is `ADF_OK`
  for an idele and `ADF_DOMAIN` for a class.
- `tie_of_the_leading_bits_is_a_strict_inequality`: `adf1 Q idele 1 3 0 1 1 1 1 1 0` and
  `adf1 Q idclass 3 0 1 1 1 0` are `ADF_OK`, and each is built with the public constructor
  (`adf_idele_set_parts`, `adf_idclass_set_parts`), dumped to that exact text, loaded again and
  found identical.
- `sign_of_the_small_balls`: a family that does not use the method of `dp_arb_sign`. For the idele
  the full product of an odd midpoint `m` with `|m| <= 127` in both signs (128), a radius mantissa
  `r` that is every odd value up to 127 and the two of 30 bits below the bound of 10.2 (64), and
  `e, f` in -9..9 (19 x 19): **2959368 cases**. For the class the same shape with a stride of 6 on
  the midpoint and of 8 on the radius: **142956 cases**. Then 20 cases whose exponents have 100 and
  5000 bits. Each case: the text, the exact verdict `|m| 2^e > r 2^f` (and `m > 0` for the class)
  from `ball_excludes_zero`, the loader, and the public constructor on the same ball. The three
  verdicts must agree; on `ADF_OK` the dump of the loaded value is the text byte for byte.

  `ball_excludes_zero` decides with `d = e - f` and the bit lengths of the two mantissas, and forms
  neither power, so an exponent of 5000 bits (or of `2^5000`) is decided exactly. The test
  program runs **32505050 checks, 0 failed** in 5.0 s.

The radius-mantissa faults of review F5 (F12, F13) are now detected: the tables of
`idele_cases` and `idclass_cases` carry the two texts of the review (`... 40 40000001 0 ...`,
`... 40 2 0 ...`) and four more with the odd midpoint 3 or 5, whose ball excludes 0, so nothing but
the radius mantissa decides. Note that the two texts of the review cannot detect F12 or F13 by
themselves: the midpoint mantissa `40` is even, so the predicate of the arb fails first and the
status is `DOMAIN` in both the right and the faulty code. The texts with an odd midpoint are what
the fault table detects (2 failed checks for F12 and 6 for F13 in `test_dump_units`, 2 each in
`test_dump_local`).

`tests/test_dump_local.c`: `lball_exponent_word_and_prec` (part 4) and
`sball_exponent_and_radius_mantissa` (the exponents inside a partial ball and the radius mantissa
with the tags `r` and `c`, again with the odd midpoint 5 so that F12 and F13 are caught there too).
The program runs **36204 checks, 0 failed**.

## Part 2: the repair of `dp_arb_sign`

Both patches of the review were checked before being taken, and one of them is wrong as written in
its broad form. The proof is now a comment at the function (`src/dump.c`, the `if (res == 0)` of the
same-binade case): with `t = min(bm, br)` the bits below the leading `t` ones of the longer mantissa
are below `2^(bm - t)` resp. `2^(br - t)` and are not all zero, that mantissa being odd
(conventions 10.2); so with `bm > br` the shorter mantissa `rm` is whole and `|m| > rm` holds exactly
when the leading `br` bits of `|m|` are `>= rm`, with `bm < br` the shorter mantissa is `|m|` and
`rm` has bits below it, so a tie is a strict inequality, and with `bm == br` both mantissas are
whole, so a tie is an equality and `>` is right there as well. The code is

```c
        if (bm > br)
            res = fmpz_get_ui(m) >= rt ? 1 : 0;
        else
            res = fmpz_get_ui(m) > rt ? 1 : 0;
```

which is the patch F18 of the review. `bm >= br` (the first form I wrote) is **wrong**: it answers
"excludes 0" for the ball `[0 +- 1]` of the golden file
`adf1 Q idele 1 1 0 1 0 1 1 1 0` (`bm == br == 1`, top `= 1 = rt`), which the golden test caught
at once. The negative midpoint (F1) is the other line: `am.p++; am.n--;`, the sign byte is not a
hexadecimal digit.

The differential check of the review, against the repaired library
(`FZ=$PWD/lanes/u-repair1/w/fz`, `fz.c` compiled from `lanes/u-review1/w/fz.c`):

| command | result | before the repair |
|---|---|---|
| `gen.py idele 900 11` | 61605 cases, 11562 OK, **0 mismatches** | 41 |
| `gen.py idclass 900 12` | 61048 cases, 12085 OK, **0 mismatches** | 6 |
| `gen.py lball 900 13` | 59869 cases, 9579 OK, 0 mismatches | 0 |
| `gen.py sball 900 14` | 63222 cases, 9578 OK, 0 mismatches | 0 |
| `gen.py ucoset 900 15` | 59849 cases, 9271 OK, 0 mismatches | 0 |

## Part 3: no abort reachable from a text

`src/dump.c` had three `flint_abort` calls a text could reach if a stage-6 check were wrong: in
`dp_load_ucoset`, `dp_load_idele` and `dp_load_idclass` (line 2229 of the review), each after a
public constructor refused the value that stage 6 had accepted. Each loader now builds into a
temporary (`adf_idele_init`, `adf_idele_set_parts`, then `adf_idele_swap`) and returns the status
of the constructor, so a text gets `ADF_DOMAIN` with the output untouched. The assertion is kept
under `ADF_CHECK_INVARIANTS` only, as `DP_INV(t, st == ADF_OK, ...)`.

The two local loaders had no abort but called `adf_lball_swap` and `adf_sball_swap`, which assert
the predicate under `ADF_CHECK_INVARIANTS` (`src/lball.c:284-289`, `src/sball.c:130-135`). Each now
asks `adf_lball_is_canonical` resp. `adf_sball_is_canonical` of its temporary and returns
`ADF_DOMAIN` when it fails, so a non-canonical value never reaches the swap.

What the loaders of lane m1-dump do (`rat`, `fball`, `adele`, `scaled`): they build the value
directly and swap on `ADF_OK` alone; none of them calls a constructor that can refuse, so none of
them has an abort of this kind. I did not change them.

Two more hardening changes, both without a change of any status a text gets:

- `dp_w_arch` fills the first ball with the zero ball when the count is 0. The fault F06 of the
  table (the archimedean count test of the idele dropped) then reads a validated arb instead of a
  span no text has filled: it was a `SIGSEGV` (a read of the stack) and is now the same `DOMAIN`
  as the unmutated code.
- `dp_load_idele` asks `P.node.narch != 1` again before it reads the ball (the same fault, on a path
  where the spans are filled).

The three `flint_abort` that stay are unreachable from a text and are kept: `dp_fmpz` (the token
passed `dp_is_h`, so `fmpz_set_str` cannot fail), `dp_sb_finish` (a writer of our own output) and
the one in `adf_scaled_get_str` (a printer of a stored value, lane m1).

Test, as the brief asks: with F01, F15 and F17 planted, `test_dump_units` ends with failed checks
and a count instead of a signal in the release build (5, 5 and 5 failed checks, `rc=1`), and in
the `INV=1` build each stops at the new assertion of its loader (`rc=-6`,
`adelefeld: ADF_CHECK_INVARIANTS: dp_load_idclass: argument t is not a canonical adf_idclass`).

## Part 4: the exponents of a dumped local ball

What the loader does: with `max_prec` raised, `|v|` and `|N|` are accepted up to `LONG_MAX`, so the
loader builds a value that no public function of the type can build: `adf_lball_set_rat_ball`
answers `ADF_LIMIT` above `ADF_LBALL_EXP_MAX` (`lball.h:132`) and `adf_lball_set_str` answers
`ADF_LIMIT` above it (`text.h:304, 331`).

**Decision: the code is left as it was.** The header does not make the bound part of the type:

- the predicate of the struct (`lball.h:76-80`, the predicate of conventions 5.8) bounds neither `v`
  nor `N`;
- `lball.h:26-35` says that `is_canonical`, `init`, `clear`, `set`, `swap`, `identical` and the
  layout queries "have no such limit", and `adf_lball_is_canonical` does not test the bound;
- conventions 10.2 makes the predicates of section 5 the condition of a strict loader, and the
  reference `proto/text_grammar.py` applies no such bound (the differential corpus of the review
  agrees: 0 mismatches on the `lball` and `sball` runs above).

Adding the bound would make the loader stricter than the predicate of the type, and it would break
`tests/test_dump_ctx.c:1355` of lane m1-dump, which pins `ADF_DOMAIN` (the answer of
`adf_modctx_new_from_dump` for a body with no occurrence) for
`adf1 Q lball 5 b 0 0 7fffffffffffffff`, a text that a bound of 2^60 would answer `ADF_LIMIT`.
I verified that: with the bound in `dp_w_lb` that one check of that file fails, and it passes
again without it. That file is not mine, so the change is reported rather than made.

What the review's F3 (`v` or `N` equal to `LONG_MIN`) is worth: nothing observable. The token
`-8000000000000000` is `-2^63`, and `2^63` is above every `max_prec`, because `max_prec` is an
`slong`; so `ADF_LIMIT` comes from `dp_abs_over` before `dp_fits_si` is asked, in every build and
under every limits struct. `dp_fits_si` now states the word it means (a negative token of sixteen
digits whose first digit is 8 is `-2^63` and fits), which is correct but unreachable: the fault F20
of the table (the old line) is not detected, and that is why.

Tests, both sides of every bound: `lball_exponent_word_and_prec` (2^60 - 1, 2^60, 2^60 + 1,
-2^60, -(2^60 - 1), 2^63 - 1 are `ADF_OK` with `max_prec = LONG_MAX`; 2^63, -(2^63 + 1) and -2^63
are `ADF_LIMIT`; with the default limits the same texts above `max_prec` are `ADF_LIMIT`; the
largest field that loads is canonical, dumps back to its text and loads into an identical value) and
`sball_exponent_and_radius_mantissa` (the same inside a partial ball, with the tag `n`).

## Part 5: the driver

`adf_drv_split` (tools/adf/adf.c:822) is called once per line for every command and skips the blanks
(space and tab, `adf_drv_is_blank`) before each operand, including the first. The bytes of the
operand itself are passed on: a blank at the end of the line stays in the operand, which is why
`load adf1 Q ucoset 1 0 ` is `error: PARSE` (the dump form has no trailing space, conventions
10.1) while `load  adf1 Q ucoset 1 0`, `load<TAB>adf1 Q ucoset 1 0` and `load adf1 Q ucoset 1 0` read
the dump. So the trimming is not a property of `load`: it is the line reader, and `load` alone
cannot pass its bytes through. The README sentence was therefore corrected, and the four lines
appended to `tests/driver/u-dump-units.cmd`/`.out` pin it. `tools/adf/adf.c` is unchanged.

Item 3 of the README ("every operand is the exact text between two separators, with no trimming")
has the same defect for the leading blanks; it is not my sentence and I did not change it.

## Part 6: the fault table

`FT_PLAIN=1 timeout 1500 python3 lanes/u-repair1/faults.py` (release build of the two test
programs, `rc` is the exit status of the program; a negative one is a signal). F18 and F19 are the
reverse mutations of the two repairs of part 2 (the review had them as the forward ones), F20 and
F21 are new controls.

| # | Fault | units | local |
|---|---|---|---|
| F00 | baseline, no change | rc=0, 32505050 checks, 0 failed | rc=0, 36204 checks, 0 failed |
| F01 | class ball positivity check dropped | rc=1, **5 failed checks** | pass |
| F02 | v and N exchanged in the ball form | pass | rc=1, 7726 failed checks |
| F03 | primality test skipped | pass | rc=1, 7 failed checks |
| F04 | primes: strict increase non-strict | pass | rc=1, 2 failed checks |
| F05 | `max_items` `>=` instead of `>` | pass | rc=1, 2 failed checks |
| F06 | archimedean count 0 accepted | pass (was SIGSEGV) | pass |
| F07 | `max_items` off by one | pass | rc=1, 2 failed checks |
| F08 | exact form accepts den(u) divisible by p | pass | rc=1, 1 failed check |
| F09 | unit modulus canonicalised on load | rc=1, 403 failed checks | pass |
| F10 | lball loader touches x on DOMAIN | pass | rc=1, 25 failed checks |
| F11 | exact unit -1 refused | rc=1, 2314 failed checks | pass |
| F12 | radius mantissa bound 2^30 to 2^31 | rc=1, **2 failed checks** (was NOT DETECTED) | rc=1, **2 failed checks** (was NOT DETECTED) |
| F13 | radius mantissa parity check dropped | rc=1, **6 failed checks** (was NOT DETECTED) | rc=1, **2 failed checks** (was NOT DETECTED) |
| F14 | real ball of the tag r not validated | pass | rc=1, 14 failed checks |
| F15 | idele content positivity dropped | rc=1, **5 failed checks** (was abort) | pass |
| F16 | ball u = 0 with v != 0 accepted | pass | rc=1, 1 failed check |
| F17 | gcd(c, N) = 1 dropped | rc=1, **5 failed checks** (was abort) | pass |
| F18 | control: the repair of F2 undone | rc=1, 4816 failed checks | pass |
| F19 | control: the repair of F1 undone | rc=1, 3 failed checks | pass |
| F20 | control: the word check of a negative token as before | pass | pass |
| F21 | control: the three loaders abort again | pass | pass |

F12 and F13 are detected now, and nothing that the review detected is lost: F01, F15 and F17 are
detected by failed checks where the review saw an abort. F06, F20 and F21 are not distinguished by
these two programs; the reason is in each case above (F06 is made equivalent by the two hardenings of
part 3, F20 and F21 are on paths that no text of the two files reaches).

The `INV=1` build, the six faults that matter for part 3
(`timeout 900 python3 lanes/u-repair1/faults.py F00 F01 F15 F17 F18 F19`, library in
`lanes/u-repair1/build-inv`):

| # | units | local |
|---|---|---|
| F00 | rc=0, 0 failed | rc=0, 0 failed |
| F01 | rc=-6, `dp_load_idclass: argument t is not a canonical adf_idclass` | pass |
| F15 | rc=-6, `dp_load_idele: argument t is not a canonical adf_idele` | pass |
| F17 | rc=-6, `dp_load_ucoset: argument t is not a canonical adf_ucoset` | pass |
| F18 | rc=1, 4816 failed checks | pass |
| F19 | rc=-6, `dp_load_idele: argument t is not a canonical adf_idele` | pass |

## Checks at the end

Every command under `timeout`, at most 2 jobs, none over 170 s.

    timeout 1500 make -j2 check                      3m45, "check passed: all 83 test programs", 900 "ok" lines, 0 failures
    timeout 900 sh tests/test_driver.sh              2.2 s, "69 cases, 101312 expected lines, all equal (SAN=0)"
    CC=clang INV=1  (lanes/u-repair1/b-clang-inv)     no warning with -Wall -Wextra -Wpedantic -Werror
        test_dump_units   6.9 s,  32505050 checks, 0 failed
        test_dump_local   0.02 s, 36204 checks, 0 failed
    CC=clang SAN=1  (lanes/u-repair1/b-clang-san)     no warning
        ASAN_OPTIONS=detect_leaks=1 test_dump_units  17.8 s, 0 failed, no sanitizer report
        ASAN_OPTIONS=detect_leaks=1 test_dump_local  0.1 s, 0 failed, no sanitizer report

## Sources pending

None. Every formula and convention used is in `docs/conventions.md`, `docs/SPEC.md` or a header
of this tree, with the line in the comment; the mag representation is cited from
`/usr/include/flint/mag.h` (mag.h:113-119, 135-140, 217-221) as the code of `src/dump.c` already
does, and `n_is_prime` from `ulong_extras.h`.

## Findings against the specification

1. The dump loader and the value form reader disagree on the exponents of a local ball. The loader
   accepts `|v|, |N|` up to `LONG_MAX` when `max_prec` allows it; `adf_lball_set_str` answers
   `ADF_LIMIT` above `ADF_LBALL_EXP_MAX = 2^60` (`text.h:304, 331`), and so does every arithmetic
   function of the type (`lball.h:26-35, 132`). The predicate of conventions 5.8, which 10.2 makes
   the condition of a strict loader, does not bound them, and `adf_lball_is_canonical` does not test
   the bound, so a value with `|N| = 2^63 - 1` is a value of the type that the library cannot
   build. One of the two has to change: either the predicate of 5.8 gains the bound (and then the
   loader must refuse with `ADF_LIMIT`, as the value form reader does, and the check of
   `tests/test_dump_ctx.c:1355` must be moved), or the value form reader loses it. I did not change
   either; the decision belongs to the orchestrator and to lane m1-dump.
2. Review F3 cannot be reproduced as a defect: `v = N = LONG_MIN` is `ADF_LIMIT` under every limits
   struct because `2^63` is above every `max_prec`, so the word check never decides that text.
3. Not a defect but worth stating: with the archimedean count 0 of `adele`, `cadele` and `idele`,
   the spans of the first real ball are filled by no text. The library reads them only when stage 6
   has decided the count, and `dp_w_arch` now fills them with the zero ball, so that a defect of
   stage 6 reads a ball instead of the stack. No status of the unmutated code changes.