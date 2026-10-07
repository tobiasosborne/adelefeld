# Lane r-dump2: result (bead adf-2w1, review m4 finding 2)

## The change

`src/dump.c:1124-1141`, `dp_w_other`, case `DP_FFUN` (the grammar stage of the ffun body). The `D` and `M`
tokens were converted with `dp_fmpz` (which calls `fmpz_set_str` for a token of more than 15 hex digits) and the
check `D < 0 or M < 0 or 8 D M > tokens left` was done on fmpz values. It is now decided on the tokens, with no
FLINT call: `dp_neg` for the signs; if both are positive, `dp_word` for each (a token of more than 16 digits is
at least 2^64 and fails), and `m > floor(floor(left / 8) / d)`, which for positive integers is the same as
`8 d m > left` and cannot overflow. A zero `D` or `M` gives `dm = 0`, as before. The status of every text is
unchanged; only the FLINT calls are gone. The comment cites conventions 10.2 (CV-52), dump.h:292-293 and the
review finding. Net diff: 24 lines, 10 added, 14 removed.

## The test (tests/support/fun_dump_test.h, shared by test_ffun_dump and test_rfun_dump)

- A counting `fmpz_set_str` interposer (lines 76-88). It does the documented conversion with GMP's
  `mpz_set_str` plus `fmpz_set_mpz`, so valid texts with long tokens still load (checked: a valid ffun text with a
  25-digit exponent loads OK and makes 1 call, after validation).
- `one()` (lines 113, 152-156): when the expected status is PARSE or LIMIT, the load and both inspect calls
  together make zero `fmpz_set_str` calls. This covers every PARSE and LIMIT text of the corpus: the 53 vectors,
  the goldens, `malformed()`, `caps()`.
- The prefix loop of `malformed()` (lines 320-323): every truncated prefix with status PARSE or LIMIT makes zero
  calls.
- New texts, ffun (lines 274-288): the reproducer `adf1 Q ffun 1 fffffffffffffffff -43 0 0 0 -1b 0 0 0`;
  long D with negative M; negative D with long M; long D with too few tokens; a 16-digit M (2^60) with too few
  tokens; two 32-digit dimensions; long D with M = 0 and a trailing token (the case where the grammar accepts the
  dimensions and fails later in the body); all PARSE. Two LIMIT texts (max_items 0), one with a 24-digit
  exponent. rfun (lines 291-293): two PARSE texts with a 17-digit term count and coefficient count.
- The INV abort children (lines 401, 407) now restore the field they corrupted and clear their object after the
  call, for tools/memcheck (reached only if the call does not abort).

## Red and green

- Red (current code, `lanes/r-dump2/red.log`): `test_ffun_dump` fails at fun_dump_test.h:155 on the reproducer:
  "ffun length 51: PARSE after 3 fmpz_set_str calls" (1 from load, 2 from the two inspects). A separate counting
  program over the new texts on the old library: 1, 1, 1, 1, 1, 2, 1 calls on the seven PARSE texts; the DOMAIN
  text `adf1 Q ffun fffffffffffffffff 0` made 4 (one per stage walk). `test_rfun_dump` passed on the old code
  (59471 checks): the rfun counts go through `dp_count`, which makes no FLINT call; the review saw no rfun case.
- Green (`lanes/r-dump2/green.log`): the same program gives 0 calls on all nine texts, statuses unchanged.
  Boundary of the rewritten product check: `ffun 2 3` with 44, 48, 52 zero tokens gives PARSE, OK, PARSE.

## Checks (`lanes/r-dump2/matrix.log`), each with `make -j2 BUILD=lanes/r-dump2/build-<cfg>`

| test | plain | SAN=1 | INV=1 |
|---|---|---|---|
| test_dump | 12259 checks, 0 failed | same | same |
| test_dump_ctx | 28587, 0 failed | same | same |
| test_dump_golden | 763, 0 failed | same | same |
| test_dump_limits | 297, 0 failed | same | same |
| test_dump_local | 36205, 0 failed | same | same |
| test_dump_units | 32505051, 0 failed | same | same |
| test_qclass_dump | 33913 | 33913 | 33915 |
| test_char_eval | 1738298 | 1738298 | 1738325 |
| test_ffun_dump | 50466, 0 failures | 50466 | 50472 |
| test_rfun_dump | 59471, 0 failures | 59471 | 59477 |

All 30 runs exit 0 (ffun and rfun rerun in all three configurations after the abort-child change: exit 0).
`sh tests/test_driver.sh`: 95 cases, 101626 expected lines, all equal (SAN=0), exit 0.

Review generator (`lanes/m4-review1/gen/hunt5.py` with `t.c`, copied into my lane directory and linked against
my builds; `t_old` linked with `git show HEAD:src/dump.c` compiled in the scratch directory),
3000 texts, 3000 dumps, 20000 mutated dumps per run, about 1-2 s each (`lanes/r-dump2/gen.log`):

| build | seed 1 | seed 2 | seed 3 |
|---|---|---|---|
| old dump.c | 70 mut-fmpz fails | 65 | 66 |
| fixed, plain | 0 fails | 0 | 0 |
| fixed, SAN | 0 fails | - | - |

The review reported 59 and 63; its seeds are not recorded, so the counts differ but are of the same size.

## Other count readers checked

- `dp_count` (src/dump.c:272): token arithmetic in size_t only, no FLINT. Callers: the ctx block count
  (`dp_w_ctx`, 812), the archimedean count (`dp_w_arch`, 875), the qclass piece count (1064), the rfun term and
  coefficient counts (1152, 1159), the sball local-ball count (1305), and the re-walks after validation
  (2677, 2822-2845, 2921-2927). Not affected.
- `dp_fn_validate` (2805): the ffun D, M check uses `dp_word` only and runs after `dp_validate` has passed
  the grammar stage. Not affected.
- The char body (1327-1357): `dp_word` and FLINT group setup only in stage 6 (DP_SEM). Not affected.
- Every other `dp_fmpz` call (lines 446, 479-480, 506-534, 606-607, 652-657, 714-728, 1001-1005, 1219-1220) is in
  a stage-6 predicate; 783 is the DP_OCC copy after full validation; 1571 onwards are the loaders after
  validation. Not affected.

Only the ffun dimensions had the defect; nothing else was changed.

## Not done

Mutation testing of the change was not run (the brief asked for the checks above and about thirty minutes).
Build trees and the copied generator binaries are removed; the logs red.log, green.log, matrix.log and gen.log
stay in the lane directory.

## Findings

None against the specification. One note: the DOMAIN text `adf1 Q ffun fffffffffffffffff 0` made 4
`fmpz_set_str` calls on the old code (one per stage walk). The repair removes these too. The test asserts zero
calls only for PARSE and LIMIT, as the brief asked.
