# Lane r-dump1: the shared dump validator gives PARSE where conventions 10.1 says DOMAIN (bead adf-3n2)

Finding 2 of lane q-slice4 (`lanes/q-slice4/report.md`, "Findings against the design, goldens, and existing
integration checks", item 2): the dump text `adf1 Q qclass pieces 1 0 g 0 1 1` (one piece, arch count 0)
returns `PARSE`. Its grammar is a piece with arch count zero; `docs/conventions.md` 10.1 (line 1411 and after)
says another arch count gives `DOMAIN`. The shared piece-count preflight in `src/dump.c` assumes eight following
tokens per piece and rejects this short body before the domain validation; a longer zero-arch local body reaches
`DOMAIN`. The lane kept a failing assertion behind `tests/test_qclass_dump --strict-zero-arch`.

Repair: the preflight uses the grammar's MINIMUM token count per piece (what a piece with arch count 0 and the
shortest finite part has) before the domain validation, so that the stage order of conventions 10.2 holds
(bytes, grammar, limits, then domain) for every typed loader that shares the validator. Then:
`--strict-zero-arch` becomes the default of `tests/test_qclass_dump.c` (the flag may stay as a no-op); a golden
row `adf1 Q qclass pieces 1 0 g 0 1 1	!DOMAIN` in `tests/golden/dump.tsv` next to the other `!DOMAIN` qclass
rows (lines 151-154), with `proto/text_grammar.py` agreeing (run `python3 proto/test_text_grammar.py`; if the
reference parser gives `PARSE` for this text, the reference is wrong too: say so, fix it, and report it as a
finding, since conventions 10.1 is the source); check that no other typed loader changes its answer on the
existing goldens (`tests/test_dump*`); the differential script of the lane,
`lanes/q-slice4/differential.py`, rerun against your build (copy it into your lane directory with its
seed; report the counts). Red first: the strict check must fail on the current code; then green.

Read first: `lanes/COMMON.md`, `lanes/COMMON-C.md`; conventions 10.1, 10.2 (1411-1480); `src/dump.c` (the shared
validator and the piece-count preflight; the typed loaders that call it); `tests/test_qclass_dump.c`;
`docs/reviews/m1/dump/` (the review of the loader: its two BLOCKERS were unsafe reads of malformed dumps; your
change must keep "validate all bytes before any FLINT load call").

**You own:** the preflight in `src/dump.c` (the smallest change; no other loader logic), `tests/test_qclass_dump.c`,
the one golden row in `tests/golden/dump.tsv`, `proto/text_grammar.py` only if it is wrong, `lanes/r-dump1/`.
Everything else is read-only. No git command that changes state, no `bd`. At most 2 cores; every program under
`timeout`. Checks at the end: `tests/test_dump*` and `tests/test_qclass_dump` under plain, `SAN=1`, `INV=1`;
`sh tests/test_driver.sh`. Aim to finish within thirty minutes.

Report: `lanes/r-dump1/result.md` (a Claude subagent cannot write `report.md`; the same text as your final
message): the change (file and lines), red and green runs with counts, the differential counts, findings.
