# Lane q-repair1: tests for the two gaps R1 and R2 of review q-review3 (Claude Opus, 2026-10-07)

This file stands in for `report.md` (a Claude subagent cannot write that name).

## What was done

`tests/test_qclass_reduce.c` gained the tests that detect the review's faults F2 (R1) and F8 (R2). Neither
`src/qclass.c` nor any header was changed. Nothing was found against the code: the unmodified source passes
every new check.

Files written:
- `tests/test_qclass_reduce.c`: +168 lines (new functions `own_count`, `input_ends_in`, `tiny_frac`,
  `pow2`, `piece_is`, `r1_smallest`, `r2_second_pass`; `main` runs them, and an argument `repair-vectors`,
  `repair-count`, `repair-r1` or `repair-r2` runs one new part alone, used for the red runs).
- `tests/ref/vectors/q-repair1/tiny_frac.jsonl` (61 lifts, 165 KB) and its generator
  `lanes/q-repair1/gen_vectors.py` (oracle `proto/quotient3_checks.py`, as for the q-slice2 vectors).
- `lanes/q-repair1/faults.py` (F2 and F8 patches copied from `lanes/q-review3/faults.py`), logs
  `faults.log`, `green-run.log`, `san-build.log`, `san-run.log`.

## Tests added

R1 (fault F2, last piece dropped when `0 < frac(h) < 2^-100`):
- (a) `r1_smallest`, 31 checks: the lift `[-2^-212, 2^-210] x {0}` (midpoint `3 * 2^-213`, radius
  `5 * 2^-213`; the ends are checked to be exact), prec 53. Own count 2; limit 1 gives LIMIT with a LIFT `y`
  untouched (bytes, members, identity); limit 2 gives OK, length 2, canonical, and the two stored pieces
  exactly: `[-2^-240, 2^-210 + 2^-240] x (0 ; H = 0)` and `[1 - 2^-212 - 2^-241, 1 + 2^-212 + 2^-241] x
  (1 ; H = 0)` (Q1: midpoints 2^-211 and 1, `d` a power of two, so the radius is `d + 2^-29 d`). The input
  points `(2^-210 ; 0)` and `(-2^-212 ; 0)` are in the result. The aliased call `y = x` at limit 1 gives LIMIT
  with `x` untouched, at limit 2 the same pieces.
- (b) family `tiny_frac.jsonl`: the R1 input plus 60 lifts whose upper end is `a_j + K + eps`, `a_j` a dyadic
  fibre centre (a later fibre when `B > 1`), `eps` in `2^-200`, `2^-101`, `2^-99` (the last a control that
  does not trigger F2), `K` in -2, 0, 1, 3; short widths 1/2, 3/4, 2^-20 and long widths 5, 17, 40, 23/2;
  finite parts `(0;0)`, `(-3;0)`, `(0;1)`, `(1/2;3)`, `(-3;2)`, `(0;1/2)`, `(1/3;2/3)`, `(-5/4;1/4)`,
  `(0;3/2)`, `(7;12)`; prec 20, 53, 300. 1060 raw pieces in all.
  - Through the existing `vectors()` (70598 checks with the R1 row): K-1 gives LIMIT, 0 gives LIMIT, K gives
    OK, stored list equal to the oracle's rounded list exactly, every exact oracle piece inside a stored piece
    with `rho - d <= 2^-28 d`, 40 labelled points per lift (the input end points of every fibre first), alias
    call identical.
  - Through `tiny_frac` (2689 checks): own algorithm R count in `fmpq` (`ceil(h_j) - floor(l_j)` per fibre,
    1 when `l_j = h_j`; `docs/api-3.md:164-165`), equal to the oracle's `raw`; limit K-1 gives LIMIT with a
    LIFT `y` untouched; limit K gives OK with at most K stored pieces; `(lo ; a_j)` and `(hi ; a_j)` of
    every fibre are in the input and in the result (exact membership `union_member`).

R2 (fault F8, `y->form` written before the last LIMIT exit of the second pass), `r2_second_pass`, 32 checks:
the lift `[5/8, 7/8] x (1/3 + 2 Zhat)` (ends checked exact, own count 1), `y` the LIFT
`(7 +/- 2^-5 ; 1/6 + 5 Zhat)`; prec `ADF_REAL_PREC_MAX` = 2097152, limit 10: LIMIT, `y` untouched (bytes,
members, identity); the aliased call `y = x`: LIMIT, `x` identical to a copy and still a LIFT. Time of both
calls 0.000 s plain, 0.001 s under SAN (guard 5 s, checked), so the precision was not reduced. At prec 2097088,
limit 1: OK in 0.001 s (0.002 s SAN), one piece, `(A, H, d) = (0, 2, 1)`, midpoint of at most 2097088 bits
with `|m - 5/12| <= 2^(-2-p)` (this fixes RN_p of the non-dyadic 5/12), `0 <= rho - d <= 2^-28 d` with
`d = max(m - 7/24, 13/24 - m)`, stored ends enclose `[7/24, 13/24]`, input ends in the result.
Note: F8 is detected only because `y` is a LIFT before the call; a PIECES `y` would hide it.

Check count of the whole file: 150122 before, 223472 after (plain build).

## Red and green runs

Red by the review's faults: `timeout 600 python3 lanes/q-repair1/faults.py` (log `faults.log`). It plants
each fault in a scratch copy of `src/qclass.c`, builds the copied test with
`gcc -std=c11 -O2 -w -Iinclude -Isrc -Itests ... lanes/q-repair1/build/libadelefeld.a -lflint -lgmp -lm` and
runs the whole file and each new part:

| Source | whole file | repair-vectors | repair-count | repair-r1 | repair-r2 |
|---|---|---|---|---|---|
| unmodified | exit 0, 223472 checks | exit 0, 70598 | exit 0, 2689 | exit 0, 31 | exit 0, 32 |
| F2 | exit 134, line 187 | exit 134, line 187 | exit 134, line 225 | exit 134, line 225 | exit 0 |
| F8 | exit 134, line 226 | exit 0 | exit 0 | exit 0 | exit 134, line 226 |

Line 187 (`vectors`): limit K-1 gave OK, not LIMIT. Line 225: `refused` got OK, not LIMIT.
Line 226: `refused` found the bytes of `y` changed.

Before this lane both faults passed the file (review, fault table). The F2 run also fails the R1 part on its
own, and the F8 run fails the R2 part on its own.

Green, plain: `timeout 600 make -s -j2 BUILD=lanes/q-repair1/build lanes/q-repair1/build/test_qclass_reduce`
(exit 0), `timeout 170 lanes/q-repair1/build/test_qclass_reduce`: exit 0, 223472 checks, 0.06 s wall.

Green, sanitizers and invariants: `timeout 600 make -s -j2 BUILD=lanes/q-repair1/build-san SAN=1 INV=1
lanes/q-repair1/build-san/test_qclass_reduce` (exit 0, 10.5 s), then `timeout 170
lanes/q-repair1/build-san/test_qclass_reduce`: exit 0, 223475 checks (3 more: the INV-only `debug_entry`),
0.56 s wall, no sanitizer report. The line "ADF_CHECK_INVARIANTS: adf_qclass_reduce: argument x is not a
canonical adf_qclass" in `san-run.log` is the expected abort of the child process in the existing
`debug_entry`. With stdout redirected to a file the progress lines appear twice in that log, most likely
because the forked child inherits the unflushed buffer (existing test code, not from this lane; not
investigated).

The repository's suites were not run as a whole (brief). No mutation run: no file under `src/` was changed.
All build trees and the scratch fault tree were deleted.

## Findings against the code

None. The unmodified `adf_qclass_reduce` gives the review's correct results on both inputs and on the 61 lifts.

## Not done; sources pending; findings against the specification

Not done: nothing from the brief. Sources pending: none (algorithm R and Q1 read in `docs/api-3.md` 2.2 and
section 4; the header comment of `adf_qclass_reduce`). Findings against the specification: none.
