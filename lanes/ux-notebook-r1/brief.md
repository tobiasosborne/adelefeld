# Lane ux-notebook-r1: the repair of the Adelica notebook slice after its test drive

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md` (D-UX5 as amended), `ux/notebook/README.md`,
`lanes/ux-notebook/result.md` (the builder's report), `lanes/ux-drive-notebook/report.md` (the test drive:
defects D1 to D4, usability 1 to 6, "FOR THE REPAIR" items 1 to 6) and `lanes/ux-drive-notebook/drive.md` and
the lane's notebooks where the report points to them (`builder_followups.jl`, `extras.jl`, `a_flow`,
`replay_probe.py`). The vision: `docs/ux/brief-1-adelica-notebook.md`. The package: `docs/api-j.md` section 4
and `docs/api-ja.md` (dump forms: `dump_text`, `load_dump`; `docs/SPEC.md` section 10 for why a printed
enclosure is not a lossless dump).

You own: `ux/notebook/**` and `lanes/ux-notebook-r1/**`. Nothing else; `julia/Adelefeld/` is read-only (a
lane j-slice5 is changing it in another worktree: do not depend on anything beyond the merged package).

Repair, smallest first, each with a test in `ux/notebook/test/runtests.jl` (or `page.test.js`) that was red
before and green after:

1. Item 1: the README's constructor example is runnable as written (`FBall(Rat(c), Rat(N))` or whatever the
   package's constructor is; the test runs the README's example lines through Julia); the README names what
   the slice does not do of the scenarios (A5's `equal`/`compare`/`teichmuller`; B's certificates). Under 80
   lines.
2. D3 (item 2): the DOMAIN card's reason: when the driver gives no sub-code, the card lists the possible
   causes the README of the command names (for `root_at`: the place not prime, the existence criterion, the
   seed, the degree) and recommends no single action; test with `root(9, 2; at = 4, seed = 3)`: the card does
   not say "try another seed" and names "the place 4 is not prime" as a possible cause only if the slice
   checks primality in Julia (it may: say so).
3. D4 (item 3): above p = 97 either a refinement control exists and works like the fog click (rewrites the
   cell text to `prec = N + 12`), or the instruction is not printed; test with `root(-1, 2; at = 101,
   seed = 10, prec = 8)`.
4. D1 (item 4): `replay` compares every field the record claims and the driver can reproduce: the value text
   and the status; it marks a record `unchecked` when it has no driver line, prints the count of `same`,
   `differs`, `unchecked`, and exits 1 on any `differs`; the final sentence is "replay: N same, M differs, K
   unchecked" (never "every record is the same" when K > 0); a changed dump or digits in a record is
   `differs` when the driver line reproduces the value from which they derive (recompute the digits from the
   driver's value and compare; the dump through the package's `dump_text` of the parsed value); test with the
   drive's four variants of B cell 2 and with `a_flow` (two cells without driver lines: 2 same, 0 differs, 2
   unchecked, exit 0).
5. D2 (item 5): a computed value is sent to the driver losslessly: the record's driver line of a computed
   `Adele`/`FBall` operand is `load <dump>` plus the command on a value the driver can hold... read
   `tools/adf/README.md` for whether a command can take a dump as an operand; if not, the slice must either
   (a) evaluate `reconstruct`, `project`, `roots` on a computed value through the PACKAGE when the package has
   the function (`reconstruct` it has) and record the driver line as "not reproducible by the driver: computed
   value", marking the record `unchecked` with that reason, or (b) use the exact text when the value is a
   literal. Never the printed, widened text of a real ball as the operand of a new driver call. Test with the
   drive's `builder_followups.jl` cells 7 to 10: `reconstruct(y)` gives the package's answer (NO_SOLUTION on the
   original enclosure) and the record says how it was computed; `replay` reports it `unchecked` or `same`,
   never `differs`.
6. Item 6: provenance and inspection: every successful output has a dump control when the package has a
   dump form for its type (local and partial values: the driver's `dump` of the projection text if the
   package has no type for them; say what is possible) or the words "dump: not available for this type";
   the page footer carries the library and FLINT versions if the package exposes them (read `load.jl` and
   `docs/api-ja.md` for a version gate; else "version: not exposed by the package") and the commit of the
   build; the strip tooltip "the set ..." on the A outputs as on B. Test: every record of both scenarios
   has a non-null `dump` or an explicit `dump_reason`.

Then regenerate both scenario pages (`sh ux/notebook/run.sh`), run `replay.sh` on both, run all tests, and
write `result.md` (COMMON rule 8): each item with its red and green run, the pages' and records' diff
summary, what is left (item 7 of the drive is the next slice: A5's comparison and B's certificates).
