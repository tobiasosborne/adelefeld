# Lane r-dump1 result: a piece with arch count 0 is DOMAIN, not PARSE (bead adf-3n2)

## The change

- `src/dump.c:1043-1045`: new constant `DP_QPIECE_MIN 5`, the fewest tokens of one qclass piece under the grammar
  of conventions 10.1: an arch count of 0 is 1 token (`arch = h {arb}`, line 1394), and `fb` is at least 4 tokens
  (`g A H d`, or `l d K k` with k = 0; lines 1396-1398).
- `src/dump.c:1059-1063`: the piece-count preflight in `dp_w_qclass` (the shared validator) uses
  `dp_count(c, DP_QPIECE_MIN, &n)` instead of `dp_count(c, 8, &n)`. Every token is still read one at a time with
  `dp_next`, so no read goes past the text; "validate all bytes before any FLINT load call" is unchanged (the test
  still interposes `arb_load_str` as an assertion failure).
- `src/dump.c:2652`: the re-read of the count in `dp_qclass_start` (typed loader, after validation) uses the same
  constant, for consistency. On a validated text both bounds give the same `n` (every valid piece has 9 or more
  tokens), so no behaviour of the loader changes. If the orchestrator wants that line untouched, reverting it is
  safe.
- `proto/text_grammar.py:1087-1091`: the reference had the same bug (`T.count(8)`); now `T.count(5)`, with a
  comment citing 10.1. See the findings.
- `tests/golden/dump.tsv:155`: new row `adf1 Q qclass pieces 1 0 g 0 1 1	!DOMAIN`, after the other `!DOMAIN`
  qclass rows.
- `tests/test_qclass_dump.c`: new `zero_arch()`, run by default (`--strict-zero-arch` is now a no-op; argv is
  ignored). Cases: DOMAIN for `pieces 1 0 g 0 1 1`, `pieces 1 0 l 0 1 0`, `pieces 2 0 g 0 1 1 0 g 0 1 1`, and two
  mixed texts with one zero-arch piece (first or second); PARSE for three texts with fewer tokens than the
  shortest pieces; LIMIT before DOMAIN for two zero-arch pieces with `max_items = 1` (order of checks 8.5,
  line 1137). Every case also checks `adf_qclass_dump_inspect` and that the output is untouched. The golden
  row count of this test went from 13 to 14.

## Red and green

- Red (`lanes/r-dump1/red.log`): `test_qclass_dump` on the old `src/dump.c`: exit 1,
  `adf1 Q qclass pieces 1 0 g 0 1 1: want DOMAIN, got PARSE`. Reference red (`proto-red.log`):
  `python3 proto/test_text_grammar.py` 35 tests, 1 failure: `dump.tsv:155 ... expected '!DOMAIN', got '!PARSE'`.
- Green (`green-plain.log`, `checks-final.log`): `test_qclass_dump` 33913 checks, 2000 random round trips, exit 0.
  Reference green (`proto-green.log`): 35 tests OK.

## Checks at the end (`lanes/r-dump1/checks-final.log`), plain, SAN=1, INV=1, all under `timeout`

| test | plain | SAN=1 | INV=1 |
|---|---|---|---|
| test_qclass_dump | 33913 checks, ok | 33913, ok | 33915, ok (the expected invariant abort in a child) |
| test_dump | 17 tests, 12259 checks, 0 failed | same | same |
| test_dump_ctx | 14 tests, 28587 checks, 0 failed | same | same |
| test_dump_limits | 9 tests, 297 checks, 0 failed | same | same |
| test_dump_local | 10 tests, 36205 checks, 0 failed | same | same |
| test_dump_units | 12 tests, 32505051 checks, 0 failed | same | same |
| test_modctx (also reads dump.tsv) | 11 tests, 10197 checks, 0 failed | same | same |
| test_dump_golden | 763 checks, **2 failed** (row counts only, see below) | same | same |

- `test_dump_golden` fails only on its hard-coded counts of `dump.tsv`: `tests/test_dump_golden.c:269`
  `nrows == 175` (now 176) and `:272` `nstatus == 115` (now 116). Every per-row check passes on the new row
  (`adf_modctx_new_from_dump` gives DOMAIN). That file is not mine; the orchestrator must change 175 to 176 and
  115 to 116 (and the comment at lines 266-268). `tests/golden/README.md:26` already says 165 and is stale.
- No other typed loader changes its answer on the existing goldens: the new build run against the `dump.tsv` of
  HEAD (copied into a scratch directory) passes every dump test with 0 failed checks, plain, SAN=1 and INV=1:
  test_dump_golden 761 checks, test_dump 12259, test_dump_ctx 28587, test_dump_local 36204, test_dump_units
  32505050, test_dump_limits 297 (`green-original-golden.log`).
- `sh tests/test_driver.sh`: exit 0, 77 cases, 101427 expected lines, all equal (`driver.log`).
- The build trees (lanes/r-dump1/build*, and build/ made by the driver script) are removed.

## Differential (`lanes/r-dump1/differential.py`, seed 310408, against `lanes/r-dump1/build/libadelefeld.so`)

Copied from `lanes/q-slice4/differential.py`; changed only the library path, plus two options: `--q-slice4-bases`
(leaves out the new golden row, so the random stream is that of lane q-slice4) and `--old-ref DIR` (also runs
the reference of HEAD and counts the texts whose answer moved).

- Current tree, 32 base texts: 30000 texts, 26210 edits, 3633 valid inspections, 1433 global loads, 0 status,
  context-count, output-preservation or dump-byte mismatches; 173 texts on which the reference of HEAD answered
  otherwise, all `!PARSE -> !DOMAIN` (zero-arch pieces), and C agrees with the repaired reference on all
  (`differential.log`).
- q-slice4 stream, 31 base texts: 30000 texts, 26206 edits, 3761 valid inspections, 1495 global loads (the
  counts of `lanes/q-slice4/differential.log` exactly), 0 mismatches, 0 moved texts
  (`differential-qslice4-bases.log`). So the q-slice4 corpus never reached a zero-arch piece with few tokens;
  the new base row makes the stream reach it 173 times.

This is a 30000-text differential of seconds, a smoke-scale run, not a long fuzzing campaign.

## Mutants of the change (by hand, `mutants.log`)

- `DP_QPIECE_MIN` 6 and 8: killed by `test_qclass_dump` (DOMAIN expected, PARSE got).
- `DP_QPIECE_MIN` 4: survives, equivalent. The DP_SYNTAX pass reads the whole body with `dp_next` before the
  limits pass (`src/dump.c:1467-1476`), so any bound from 1 to 5 gives the same status; the bound is an early
  reject only. 5 is kept because it is the true minimum and matches the reference, which the differential checks.

No tool mutation run (the change is two constants); not added to `tools/mutate/equivalent.txt`.

## Findings

1. The reference `proto/text_grammar.py:1087` had the same error as C: `T.count(8)` for the pieces of a qclass,
   so `adf1 Q qclass pieces 1 0 g 0 1 1` gave PARSE where conventions 10.1 (lines 1411, 1420-1421) and the order
   of checks 8.5 give DOMAIN. Repaired to `T.count(5)`. The q-slice4 differential agreed with C only because both
   were wrong the same way and its corpus never reached the case.
2. `tests/test_dump_golden.c:269,272` hard-codes the row counts of `dump.tsv`; it needs 176 rows and 116 status
   rows after this lane (not owned; for the orchestrator). `tests/golden/README.md:26` (165) is stale already.
3. Every other count preflight in `src/dump.c` already uses the minimum of its grammar (arch 4/8, ctx 1, sball 5,
   rfun 25, rfun P 8); only qclass pieces used more than the minimum.
