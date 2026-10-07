# Lane r-dump2: the shared dump validator converts long hex tokens before the grammar stage ends (bead adf-2w1)

Finding 2 of the milestone 4 review (`docs/reviews/m4/review-m4.md`, lane m4-review1): `dp_w_other` in
`src/dump.c` (about lines 1129-1134) converts the `D` and `M` tokens of an `ffun` dump with `fmpz_set_str`
when a token exceeds 15 hex digits, before the remaining tokens have been validated. `docs/conventions.md`
10.2 and `include/adelefeld/dump.h:292-293` say: validate all tokens before any FLINT call. Reproducer:
`adf1 Q ffun 1 fffffffffffffffff -43 0 0 0 -1b 0 0 0` returns `PARSE` with the output untouched but makes two
`fmpz_set_str` calls (one from `load`, one from `inspect`). Not a BLOCKER (the bytes are hex-checked and copied
into a private buffer), but the stage-order contract is violated; the review's mutated corpus hit it 59 and 63
times in 20000.

Repair: defer the conversion until the grammar stage of the whole body has ended (the validator already reads
every token: carry the long token's position and convert after the last token is checked, or compare the
hex length against the cap without converting). The smallest change; the same for any other body that
`dp_w_other` serves (`rfun` counts; check the other count readers for the same pattern and list them in your
report, repairing only those with the same defect). Then: a test in `tests/test_ffun_dump.c` and
`tests/test_rfun_dump.c` (the shared body is `tests/support/fun_dump_test.h`) that interposes `fmpz_set_str`
(as the review did; see how `tests/test_qclass_dump.c` interposes `arb_load_str`) and asserts zero calls on
every `PARSE` and `LIMIT` text of its corpus, including the reproducer; red first on the current code, then
green; `tests/test_dump*`, `test_qclass_dump`, `test_char_eval`, `test_ffun_dump`, `test_rfun_dump` under
plain, `SAN=1`, `INV=1`; `sh tests/test_driver.sh`; the review's generator `lanes/m4-review1/gen/` (the dump
mutation part) rerun against your build if it runs in under 170 s: report the counts.

Read first: `lanes/COMMON.md`, `lanes/COMMON-C.md`; conventions 10.2 (1427-1480); `src/dump.c` (the shared
validator, `dp_w_other`, the count readers, the r-dump1 repair of the piece count: `lanes/r-dump1/result.md`);
`docs/reviews/m1/dump/` (the loader's review: "validate all bytes before any FLINT load call").

**You own:** the validator change in `src/dump.c` (smallest change), `tests/support/fun_dump_test.h`,
`lanes/r-dump2/`. Everything else is read-only. No git command that changes state, no `bd`. At most 2 cores;
every program under `timeout`. The abort children of `INV` tests clear their objects after the call (for
`tools/memcheck`). Aim to finish within thirty minutes.

Report: `lanes/r-dump2/result.md` (a Claude subagent cannot write `report.md`; the same text as your final
message): the change (file and lines), red and green runs with counts, the other count readers checked,
findings.
