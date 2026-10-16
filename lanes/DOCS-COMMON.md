# Rules shared by the docs lanes (docs-readme, docs-guide, docs-julia), 2026-10-16

Ordered by TJO on 2026-10-16: "launch a docs round ... build a best-in-class docs for this package. Create a
gorgeous inviting readme that uses all the tricks, animated windows/terminals, visualisations, etc. It should be
inviting and attractive to anyone with basic BSc math/phys background."

`lanes/COMMON.md` binds you, with these decisions of the orchestrator on top:

- D-DOC1. **Audience.** A reader with a BSc in mathematics or physics who has never met an adele. Every page
  explains a term the first time it is used (place, p-adic, Zhat, ball, enclosure, status) in one or two
  sentences, with a concrete number. No sentence assumes the reader has read SPEC.md.
- D-DOC2. **Tone.** The README (and only the README) is exempt from the "no marketing words" half of
  COMMON.md rule 6: it may be warm, inviting and visual. It stays true: no claim that the code does not
  support, no number that was not computed. The guide and the package README keep the sober style.
- D-DOC3. **Every number shown is computed in the lane.** Each driver output, Julia output or digit string
  in a page is produced by running `build/adf` or the package in your worktree and pasted verbatim. Keep the
  exact commands and outputs in `lanes/<lane>/numbers.md` (command, output) so the orchestrator can rerun
  them. A number from memory or from a brief is not allowed.
- D-DOC4. **No new dependencies, no JavaScript in the README, no external services.** Animation is done with
  SVG files under `docs/assets/` (CSS keyframes or SMIL inside the SVG; GitHub renders them through `<img>`
  tags and `![]()` links; scripts in SVG are not run). Every animated SVG must be readable when animation is
  off (its first frame is complete). Mermaid diagrams in fenced ```mermaid blocks are allowed (GitHub renders
  them). Badges: local SVG files, not shields.io. No screenshots that need a browser (headless Firefox crashes
  on this machine, signal 11); draw the picture as SVG instead.
- D-DOC5. **Ground truth for mathematics.** A formula or a theorem named in a page is cited from `refs/src/`
  by file and line (COMMON.md rule 4) or from `docs/proofs/` by file and section. The project's own
  conventions are cited from `docs/SPEC.md` or `docs/conventions.md` by section.
- D-DOC6. **No C, Julia source or test changes.** The lanes write documentation and assets only. What a doc
  page needs and the code does not give is listed under "Library demands" in the report, not built.
- D-DOC7. **Links.** Every relative link in a page is checked by a script in the lane (a Python loop over
  `[text](path)` targets that tests `os.path.exists`); the report gives the count checked and the count broken
  (must be 0).
- D-DOC8. **Line width.** Prose lines at most 116 characters as in COMMON.md; HTML and SVG files are exempt.

The driver: `tools/adf/README.md` is its whole manual (commands, text forms, statuses). Build in your
worktree: `timeout 600 make -j2 && timeout 600 make -C tools/adf` gives `build/libadelefeld.a` and `build/adf`;
`sh tests/test_exports.sh` builds `build/libadelefeld.so`, which the Julia package loads (found by lane docs-julia);
`echo 'roots 2 -1 at 5 prec 8' | build/adf` is the session form (the README shows the forms). The Julia
package: `julia/Adelefeld/bin/adelefeld-julia --project=julia/Adelefeld` from the worktree root (the launcher
preloads the system libgmp); set `JULIA_DEPOT_PATH="$PWD/lanes/<lane>/depot:$HOME/.julia:"`,
`JULIA_PKG_OFFLINE=true`, `JULIA_NUM_THREADS=1` as `ux/notebook/julia.sh` does. The UX slices and their
READMEs: `ux/notebook`, `ux/atlas`, `ux/ledger`, `ux/bench`; the visions: `docs/ux/README.md`.
