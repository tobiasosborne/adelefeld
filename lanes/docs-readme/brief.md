# Lane docs-readme (Claude Opus): the top-level README

Read `lanes/COMMON.md`, `lanes/DOCS-COMMON.md`, then `README.md` (28 lines, status of 2026-09-28: stale),
`HANDOFF.md` (top entry), `docs/SPEC.md` sections 1 to 3, `tools/adf/README.md`, `docs/ux/README.md`,
the four `ux/*/README.md`, and `docs/PLAN.md` section 6 (what is done).

## Goal

Replace `README.md` with a page that makes a reader with a BSc in mathematics or physics want to try the
library in the next five minutes, and lets them. It is the front door of the repository on GitHub.

## What the page has (in this order; you may improve the order with a reason in the report)

1. **Hero.** Title, one sentence ("the adeles as an ordinary number type: C on FLINT, every answer an
   enclosure with a status"), and an animated terminal window `docs/assets/terminal.svg`: a drawn window
   (title bar, three dots) in which a driver session types itself out, about 6 to 8 lines, with real
   outputs (D-DOC3): for example a rational printed as an adele, the square roots of -1 at 5 with their
   two branches, a DOMAIN at a prime where there is no root, a reconstruction. CSS keyframes or SMIL;
   cursor blink; loops; first frame readable. Width 720 to 800 px, dark theme, looks good on the light
   and dark GitHub themes.
2. **What an adele is, in 120 words**, for the BSc reader: the real line and one p-adic line per prime
   seen at once; `22/7` as a thread through all places; a ball `146 + 1000 Zhat` as "known modulo 1000".
   One visual `docs/assets/places.svg` (animated or not): the real ruler on top, three p-adic trees
   below (p = 2, 3, 5, depth 3 or 4), the thread of one rational lit through them, computed digits
   (run the driver for the digits: `tools/adf/README.md` names the local commands; cite the lines).
3. **Why ball arithmetic and statuses**: an answer is a set that provably contains the truth, or a named
   reason why not (`DOMAIN`, `NOT_DETERMINED`, `LIMIT` ...). The four status classes with their shapes
   from `docs/ux/README.md` ("What could be combined"). One small table.
4. **Five minutes**: build (`make -j2 && make -C tools/adf`, the FLINT 3.0.1 requirement from the
   Makefile and `docs/SPEC.md`), then three driver one-liners with their real outputs, then the Julia
   two-liner (`julia/Adelefeld/bin/adelefeld-julia --project=julia/Adelefeld -e 'using Adelefeld;
   println(Rat(1)/3)'`). Then the four UX faces, one line and one link each, with a note that they are
   static pages one opens in a browser.
5. **What is inside**: a mermaid diagram of the layers (FLINT, the C library by milestone, the driver, the
   Julia package, the four UX slices) and a short table of the milestones with their state from
   `HANDOFF.md`/`docs/PLAN.md` (done, reviewed; optional; not started).
6. **Where to read next**: `docs/GUIDE.md` (being written by lane docs-guide: link it even though it does
   not exist in your worktree; the orchestrator merges both), `docs/SPEC.md`, `docs/PLAN.md`,
   `tools/adf/README.md`, `julia/Adelefeld/README.md` (lane docs-julia), `docs/README.md` (lane
   docs-julia), `docs/ux/README.md`, `CLAUDE.md` for the rules of work, the licence (AGPL-3.0, from the
   current README).
7. **Collapsible details** (`<details>`) for anything long: the full list of driver commands, the build
   notes, the project's provenance (spun off from `riemann-channel`).

Badges: local SVGs under `docs/assets/badges/` for language, FLINT version, licence, tests (the counts from
`HANDOFF.md`: 107 test programs, 5369 Julia tests, five suites green; check them in your worktree: `ls
tests/*.c | wc -l`; do not run the suites).

## You own

`README.md`, `docs/assets/**`, `lanes/docs-readme/**`.

## Checks you run and report

- Every SVG parses (`python3 -c 'import xml.dom.minidom,sys; xml.dom.minidom.parse(sys.argv[1])' FILE`),
  has width/height and a viewBox, and no `<script>`.
- Every number in the README appears in `lanes/docs-readme/numbers.md` with the command that produced it.
- The link check of D-DOC7.
- The README's commands of section 4 run as written from a fresh shell in your worktree, each under
  `timeout`, with their output pasted into numbers.md.

## Report

`lanes/docs-readme/result.md` as COMMON.md rule 8 says, with: the files written, the checks (numbers), what
you could not verify (for example how GitHub renders an animation: say that you tested the SVG only by
parsing and by reading it), library demands, findings against the specification.
