# Lane u-review1: result

Scope: the dump loaders of `adf_ucoset`, `adf_idele`, `adf_idclass`, `adf_lball`, `adf_sball` (lane u-dump1) and
the driver's ball kinds (lane drv-ball). Everything below was run on this tree; nothing was changed outside
`lanes/u-review1/`. Build trees and executables are deleted; the sources of my instruments stay in
`lanes/u-review1/w/` (reference `ref.py`, generator `gen.py`, harness `fz.c`) and `ft/` (fault runner).

## Findings

### F1 BLOCKER: process abort on a grammatical idele text with a negative midpoint (`dp_arb_sign`, src/dump.c:625-660)

Text: `adf1 Q idele 1 -1 0 1 0 1 1 1 0` (mid -1, radius 1: the ball [-2, 0] contains 0).
Returns: `adf_idele_load_str` never returns; `flint_abort()` at src/dump.c:2229 ("cannot happen: stage 6 checked
the predicate of 5.7"). Stage 6 said OK, `adf_idele_set_parts` refused. The driver dies too:
`load adf1 Q idele 1 -1 0 1 0 1 1 1 0` gives SIGABRT, exit 134, no `error:` line.
True: the text is outside the predicate of 5.7 (`arb_is_nonzero(inf)`), so `ADF_DOMAIN` (8.5 item 6; 10.2
"the fields must satisfy the predicates of section 5 (`ADF_DOMAIN` otherwise)").
Cause: `if (dp_neg(am)) am.p++;` advances the pointer but not `am.n--`. `dp_fmpz` then reads `n` characters from
the byte after the sign, so one byte past the token (the following space) is taken as a hex digit
(`dp_hexval(' ')` is a wrapped `unsigned`, about 4.29e9). The mantissa is misread as about 2^32 and the ball is
judged to exclude 0. It needs a negative midpoint and a nonzero radius (a zero radius returns before the read).
Reach: 16203 of the 55k idele variants of one seed had this shape. In the first 61605-case run 39 aborted.
Other variants answered with the right status by luck (the misread value is always larger, so it only errs
towards "excludes 0"). In `idclass` the same code is reached but a negative mid is `DOMAIN` anyway, so no abort.
Reproduce: `printf 'adf1 Q idele 1 -1 0 1 0 1 1 1 0'` through `adf_idele_load_str` (any build); or the driver line above.
Other aborting texts: `adf1 Q idele 1 -5 0 1 3 5 3 -1 0`, `adf1 Q idele 1 -bb 5 3c3 d 7 6 3 4`.

### F2 BLOCKER: a canonical idele and idclass are refused (`dp_arb_sign`, same function, `res = ... > rt`)

Texts: `adf1 Q idele 1 3 0 1 1 1 1 1 0` and `adf1 Q idclass 3 0 1 1 1 0` (mid 3, radius 1*2^1 = 2: ball [1, 5]).
Returns: `ADF_DOMAIN` (driver: `error: DOMAIN`). True: `ADF_OK`; the ball excludes 0 (5.7 `arb_is_nonzero`,
`arb_is_positive`) and the text is canonical (odd mantissas, radius mantissa 1).
Round trip broken with the public constructor: `adf_idele_set_parts(x, 3 +/- 2, 1, [1 mod 0])` returns OK,
`adf_idele_dump_str` gives `adf1 Q idele 1 3 0 1 1 1 1 1 0`, and `adf_idele_load_str` of it returns 7
(program `w/rt.c`).
Cause: in the same binade, with `bm > br`, the top `br` bits of |m| equal `rm` is a strict inequality |m| > rm
(m is odd, so the dropped low bits are nonzero), but the code returns "contains 0" on equality. The test must be
`>=` when `bm > br` and `>` when `bm == br`.
Both F1 and F2 are repaired by a two-line change (control F18, F19 below). With it the same corpora give 0
mismatches against my reference (idele 61605 cases, idclass 61048 and 60657).
Reproduce: `w/gen.py idele 900 11` and `w/gen.py idclass 900 12` (the original code gives 41 and 6 mismatches).

### F3 MINOR: `v` or `N` equal to LONG_MIN is refused (`dp_fits_si`, src/dump.c:868)

Text `adf1 Q lball 5 x 1 1 -8000000000000000` with `max_prec = LONG_MAX`: `ADF_LIMIT`. The value -2^63 fits an
`slong`; `-7fffffffffffffff` loads. The report's decision 1 says the fit check is for "what an slong holds"; this
is one value too strict. No effect under the default `max_prec` (100000), and no public constructor can make such a
value (`ADF_LBALL_EXP_MAX` is 2^60). Reproduce: `w/fz` line with limits `1000 1 9223372036854775807 5`.

### F4 MINOR: driver `load` trims blanks around its operand; README says it does not

`load adf1 Q ucoset 1 0 ` (trailing space), `load  adf1 Q ucoset 1 0` (two blanks after the verb) and a tab before or
after give `[1]`. tools/adf/README.md says "`load` does not trim its operand" and 10.1 forbids a leading and a
trailing space. The loaders themselves are right (my differential run gives `PARSE` for every such text).
Reproduce: `w/d1.cmd` lines 1 to 4.

### F5 MAJOR (tests that cannot fail), see the fault table

Neither test file detects F12 (radius mantissa bound 2^30 widened to 2^31) or F13 (radius mantissa parity check
dropped). Smallest texts that the faulty code would accept and the right code refuses with `DOMAIN`:
`adf1 Q idele 1 1 40 40000001 0 1 1 1 0` (31-bit radius mantissa), `adf1 Q idele 1 1 40 2 0 1 1 1 0` (even radius
mantissa). They are caught by the older `test_dump` (1 failed check each; F12 also by `test_dump_golden`), so at
suite level this is MINOR. Also, neither file detects the real defects F1 and F2 (controls F18, F19: the repaired
code and the broken code both pass 35106 and 35978 checks). No text with a negative midpoint and a nonzero
radius, and none with a leading-bit tie, is in `tests/test_dump_units.c` or `tests/test_dump_local.c`.

### Not findings, noted

- A complex-tagged partial ball in the driver: `project`, `exp_at` with only a prime place give `error: UNSUPPORTED`
  (README states it; the place is not archimedean, so the library could answer). A design remark.
- The driver `load` of a valid dump with a very large exponent gives `error: LIMIT` (the print guard of the README).
  42 such lines in one run; they are not loader errors.
- Leading-zero versions (`adf01 Q ...`) give `PARSE`; 10.1 does not say (my reference follows the same reading).

## What I tried without result (counts; what would have made a case fail)

Instruments: my reference `w/ref.py` (written from 10.1, 10.2, 5.6 to 5.9, 8.2, 8.4, 8.5); `w/fz.c` loads each text
with `load_str` (mode `l`), `load_str_binds` with `binds` NULL and `nbinds` 0 (a), a one-entry array (b), NULL with
`nbinds` 1 (c), then `dump_inspect` with `descs` NULL; one fork per case. A case fails when: the status differs
from the reference; on `OK` `is_canonical` is 0 or the dump is not the text byte for byte or inspect differs or
`*nctx` is not 0; on another status the dump of the output or `identical` to a copy changed, or inspect status or
`*nctx` (77 before) differs. Texts are heap blocks of exact length (ASan sees an overread). Limits struct on 20%
of the cases: `(len, ..)`, `(len-1, ..)`, 0, 1, `max_prec` 0, 1, 5, 2^62, LONG_MAX, `max_items` 0, 1, 3.

1. Three-way. Mine against the C loader after the repairs of F1, F2: 0 mismatches on the corpora below. Mine against
   `proto/text_grammar.py` (`dump_roundtrip`): 30750 texts (6150 per type), 0 disagreements. No finding against
   either reference.
2. Grammar-directed differential, unmodified code, cases per run (OK counts in brackets), mismatches other than
   F1 and F2 and the harness artefact below: ucoset 59002, 59465, 59278 (ASan); lball 59415, 60032, 59527 (ASan);
   sball 63242, 62286, 63674 (ASan), plus 31495 and 31712 structured edits (`w/sb.py`: repeated prime, swapped
   order, count 1 too small or large, a count of 2^70 with nothing behind it, tags, a complex ball read as real);
   idele 61605 (plain), 38697 (ASan, texts of the F1 shape skipped, 16203 skipped); idclass 61048, 60657 (ASan).
   Mutations: delete, duplicate, swap a token; a nasty token from a list of 50 (`-0`, `00`, `F`, 2^30, 2^30-1,
   2^63, 2^64, 2^64-59, 2^64-1, 2^64+1, 1250-digit numbers, empty, `+1`, `0x1`); +-1, +-2, +-2^30, +-2^63, +-2^64
   on a token; leading zero; upper case; sign; trailing digit or letter; two spaces, TAB, LF, CR, NUL, 0x7f, 0xe9
   anywhere or at the ends; one random byte; truncation at every byte for 3% of the bases; other body name;
   other version or field; counts +-1; even mantissas; radius mantissas 30 and 31 bits; exponents of 63, 64, 65,
   200 and 5000 bits. The harness artefact: one case with `max_len = (size_t) -1` that my generator produced
   (`strtoull("-1")`), not a library case.
   `max_items` and the sball count: with `max_items` 1 the text `r .. 1 ..` and 2 places loads (the author's
   decision 3, same reading as `proto`), no disagreement with my reference, which makes the same choice.
3. Struct filled by hand. `w/fn.c` runs, in the INV build (plain and ASan+UBSan), on each loaded lball and
   sball and on a second one: `is_canonical`, `set`, `identical`, `neg`, `add`, `sub`, `mul`, `inv`, `div`,
   `valuation`, `abs`, `decompose`, `decompose_teich` (two precisions), `frac`, `unit_mod` (k 3, 0), `pow_si`
   (-3, 0, 5, LONG_MAX, LONG_MIN), `get_center`, `get_prec`, `contains_zero`, the three set predicates, the
   value-form printer, a dump and load-back with `identical`; for sball also `arch`, `num_places`, `get_place`
   (index -1 to n), `has_place`, `get_lball`, `get_arb`, `neg`, `add`, `sub`, `mul` at 30, 64, 200 bits, printer
   at 20 digits. Corpora: 4152, 10452, 13452 lines (INV, fork per line): 0 aborts, 0 round-trip failures; the
   same corpora 1 and 2 under ASan+UBSan: 0 reports (no leak report either). The corpora include v, N and the
   slong extremes (+-2^30, +-2^60, 2^60+1, +-2^61, +-2^62, 2^63-1, 2^63-2, -2^63+1) at p = 2, 3, 5, 2^64-59, and 300
   sballs with those, loaded with `max_prec` LONG_MAX: no abort and no sanitizer report; every function that
   cannot take such a value returned a status (not tested which). A failure here would have been an abort in INV.
   Against the public constructors: `w/cmp.c` builds each loaded lball with `set_rat` or `set_rat_ball` from its
   tokens, each real or no-archimedean sball with `set_arb_lballs`, and asks `identical`: 8076 compared (2376
   skipped: complex tag, exponent above 20000, a text the loader refused), 0 not identical. (My first builder
   rounded the radius mantissa 0x20000001 up with `mag_set_fmpz_2exp_fmpz`; that was my construction, not the
   loader.)
   The complex tag: stored, copied, compared, dumped and loaded back identically (the round-trip check is in the
   battery); `add`, `neg`, `mul` on it did not abort (I did not read their statuses).
4. Memory: all runs of item 2 marked ASan were `-fsanitize=address,undefined` with `detect_leaks=1`: 0 reports other
   than F1 (an abort in the INV library, not a sanitizer report). Failure paths: the loader validates first, then
   allocates, then cannot fail except by out-of-memory; I did not inject allocation failure (not done). Printers:
   `get_str` over the extreme values returned a string or NULL without a report.
   `dump_inspect` with `descs` non-NULL and capacity 0 and 3 on 10 texts (valid and not): count 0 written only on OK,
   descriptors untouched, 0 bad (`w/insp.c`). `s` NULL with `len` 0: `PARSE` for all five types in all modes.
5. Driver (`make -C tools/adf SAN=1`, ASan+UBSan): 3629 `load` and `dump` lines of the five kinds in one
   run (2303 in another): valid dumps, `dump` of the printed value, and malformed lines (texts of the F1 shape,
   blanks around the operand and the print guard excluded from the comparison): 0 sanitizer reports, status of each
   equals my reference. The `dump(load(D))` of a real-ball kind differs from `D` because the value form is a decimal
   enclosure (as the README says); lball round trips 200 of 200 equal. `project X with PLACES` on a partial ball of
   1000 primes: all 1000 primes in a permuted order (components equal to `show`, increasing order), plus `real`
   (equal), a repeat (`DOMAIN`), a prime not in the ball (`DOMAIN`), 4, 0, a 65-bit number (`DOMAIN`), 2^64-59
   not in the ball (`DOMAIN`), `real real` (`DOMAIN`), the empty ball with `real` or 3 (`DOMAIN`), an empty list
   (`PARSE`), a complex-tagged ball with `real`, 3, and both (`UNSUPPORTED`), `exp_at`, `sin_at` at a prime and
   at `real`, `powrat_at`. Settings carried between lines: `prec` 2, 3, 100000, 2000000 (`LIMIT`, then the old
   setting stays) and `digits` 3, 1000000 between reading and printing: no wrong value found (the printer encloses
   at fewer digits: `1.234567890123 +/- 0.001` prints `1.23 +/- 0.0056` at 3 digits, correct).
   Not done: the driver lines of the F1 shape were excluded from the run after the abort was recorded.

## Fault table (item 6)

Scratch copy of `src/dump.c`, one fault each, built with `tests/test_dump_units.c` and `tests/test_dump_local.c`
(`ft/faults.py`; plain build and INV build). F00 is the unmodified file: 9 tests, 35106 checks, 0 failed; 8 tests,
35978 checks, 0 failed. "abort" is a crash of the test program, not a failed check.

| # | Fault | units | local |
|---|---|---|---|
| F01 | class ball positivity check dropped | abort (SIGABRT in the loader) | pass |
| F02 | v and N exchanged in the ball form | pass | 7714 failed checks (plain), abort (INV) |
| F03 | primality test skipped | pass | 12 failed (plain), abort (INV) |
| F04 | primes: strict increase made non-strict | pass | 5 failed (plain), abort (INV) |
| F05 | `max_items` test `>=` instead of `>` | pass | 2 failed checks |
| F06 | idele archimedean count 0 accepted | SIGSEGV | pass |
| F07 | `max_items` off by one (+1) | pass | 2 failed checks |
| F08 | exact form accepts den(u) divisible by p | pass | 1 failed (plain), abort (INV) |
| F09 | unit modulus canonicalised on load | 403 failed checks | pass |
| F10 | lball loader changes output on DOMAIN | pass | 25 failed checks |
| F11 | exact unit -1 (N = 0) refused | 2314 failed checks | pass |
| F12 | radius mantissa bound 2^30 to 2^31 | pass | pass: NOT DETECTED |
| F13 | radius mantissa parity check dropped | pass | pass: NOT DETECTED |
| F14 | real ball of the sball tag r not validated | pass | 6 failed checks |
| F15 | idele content positivity dropped | abort | pass |
| F16 | ball u = 0 with v != 0 accepted | pass | 1 failed (plain), abort (INV) |
| F17 | gcd(c, N) = 1 dropped (unit coset) | abort | pass |
| F18 | control: repair of F2 (tie of leading bits) | pass | pass |
| F19 | control: repair of F1 (length of the negative token) | pass | pass |

F12 and F13: see F5. F18 and F19 are repairs, not faults: both pass, so the two files cannot tell the broken code
from the repaired code. Run on `test_dump`, `test_dump_golden`, `test_dump_limits` (plain): F12 fails 1 check in
`test_dump` and 4 in `test_dump_golden`; F13 fails 1 check in `test_dump`; `test_dump_limits` passes both.
F05, F07, F08, F16 are detected by one or two checks (thin, but detected).

## Not done

- No allocation-failure injection (item 4).
- The driver sanitizer run did not repeat the 37369 single lines of the orchestrator.
- Texts of the F1 shape were left out of the ASan driver and idele ASan runs after F1 was recorded, to reach
  the rest; with the two repairs applied the plain idele and idclass runs include them (0 mismatches).
- `max_exp10` is not read by these loaders and was not varied.
- Fault runs used the plain and INV builds of the test programs, not `make check`.
