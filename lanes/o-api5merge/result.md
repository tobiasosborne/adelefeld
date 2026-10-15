# Lane o-api5merge (bead adf-82bj, N-D25 item 9): result

## Done
- `docs/api-5s.md` (new, 756 lines = 22 header + 289 + 445; 0 lines over 116 characters): the 22-line header with the
  mapping table, then `docs/api-5a.md` and `docs/api-5b.md` verbatim. One change in the body: the sentence of slice 5c
  (old api-5b lines 7-8) that said the file was new and might be merged now says the two are merged; same line count.
- `docs/api-5a.md` and `docs/api-5b.md` are two-line stubs ("Moved (2026-10-16)", pointing to `docs/api-5s.md`).
- Statement count before (S lines of api-5a + api-5b) 20, after 20.
- Citations fixed (files, lines changed): src/tate.c 3, src/localfactor.c 2, include/adelefeld/tate.h 4,
  tests/test_tate.c 2, test_tate_continue.c 2, test_tate_fe.c 2, test_tate_gamma.c 1, tests/julia/tate_fe.jl 1,
  tests/driver/tate.cmd 1, tate-continue.cmd 1, tate-fe.cmd 1, tools/adf/adf.c 4, tools/adf/README.md 4,
  docs/reviews/m5/review-m5.md 10, docs/SPEC.md 1 (N-D25 item 9 now says the merge is done). 15 files, 39 lines.

## Mapping
Finding: the old api-5a has NO numbered statements (headings only), and only slices 5c to 5e of api-5b number them
(S1 to S20). So renumbering into one sequence is the identity: old api-5b S<k> = new S<k>, k = 1..20 (S1-S8 slice 5c,
S9-S14 slice 5d, S15-S20 slice 5e). Lines: api-5a line n -> api-5s line n + 22; api-5b line n -> n + 311.
The numbered decisions of each slice keep their numbers (they are per slice; cite with the slice name).
The same table is at the top of `docs/api-5s.md`.

Line-number citations moved by text: review-m5 api-5a:101-104 -> api-5s:123-126; api-5b:250-252 -> 561-563;
:58 -> 369; :255 -> 566; :396-401 -> 707-712; :401 -> 712; the two audit lists (11 + 19 numbers) shifted by +22 / +311.
tests/test_tate.c "decision 5 (:146-148)": the old citation was already stale (old lines 146-148 are decision 4);
decision 5 of slice 5c is at api-5s:460-462, now cited so. tests/test_tate_continue.c "api-5b.md decision 5" now reads
`api-5s.md "Slice 5c" decision 5` (my reading: the work-count rule; it did not say which slice).

## Checks
- Preprocessed identity: for each of src/*.c, tools/adf/adf.c, tests/*.c,
  `gcc -std=c11 -E -Iinclude -Itests -Itools/adf FILE | grep -v '^# [0-9]'`, all concatenated, before the edits and
  after: `cmp` identical (2169976 lines each). (-E drops comments, so this proves only that no code changed.)
- `timeout 600 make -j2`: builds, exit 0 (-Werror build); a second run: nothing to be done.
- `grep -rn "api-5a\|api-5b"` (excluding .git, worklog, lanes, build) now finds: the two stubs, api-5s.md (header
  and table name the old files), review-m5.md:318 (a description, not a citation), HANDOFF.md, ux/ledger/*.

## Not done
- Test programs were not run (comments only changed; no behaviour). Not rebuilt tests/driver outputs.
- HANDOFF.md (lines 54, 115, 116, 119) still names the old files: not in the brief's list; the orchestrator owns it.
- ux/ledger (ledger.html, claims/fx-tate*.json) still cite `docs/api-5b.md "Slice 5c/5d/5e"`: outside my ownership;
  they are sealed copies of the fixture comment lines. The three tests/driver/*.cmd comment lines I did change now
  differ from those copies, so a re-import or re-seal of the ledger may be needed. The stubs keep the old
  citations landing on a file, but not on the text.
- The three review-m5 audit line lists were shifted by the offset only; I did not re-check that each old number
  still pointed at the intended text before the merge.
