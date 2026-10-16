# Lane docs-readme: result (Claude Opus, 2026-10-16)

The top-level `README.md` is replaced by a front door for a reader with a BSc in mathematics or physics. In order:
a hero with badges and an animated terminal; "What an adele is" with the places picture; "Why balls and
statuses" with the four status classes; "Five minutes" (build, three driver one-liners, one Tate's-thesis line, the
Julia lines, the four UX faces); "What is inside" (a mermaid diagram of the layers, a milestone table); "Where to
read next"; five collapsible `<details>` blocks (driver commands by group, build notes, the rest of the tree, how
the pictures are made, provenance and licence). Every driver and Julia output on the page was produced in this
worktree and is checked by a script that runs the README's commands as written.

## Files written

- `README.md` (331 lines; replaces the stale 28-line page of 2026-09-28).
- `docs/assets/terminal.svg` (780 x 389): a drawn window (title bar, three dots), seven real one-liners
  `echo '...' | build/adf` typing themselves out (CSS keyframes, `steps()`, a blinking cursor), outputs appearing
  after each command, "proved: no" and "open" pills on the `DOMAIN` and `NOT_DETERMINED` lines, loop of 28 s with
  a fade. With animation off (or `prefers-reduced-motion`) the whole session is shown.
- `docs/assets/places.svg` (800 x 560): the real ruler `[0, 5]` with `22/7` and the driver's `real: 3.142857 +/-
  1.5e-7`; the trees of `Z_2`, `Z_3`, `Z_5` to depth 4. At each level the lit node fans out into its `p` children
  labelled by digit, and the residue `c mod p^k` is written beside it. A gold thread runs from the ruler down every
  lit path, with an animated spark on it and a pulse on the lit nodes. Under each tree are the driver line of depth 4
  and the digit sum, and in the footer the valuations at 2, 3, 5, 7, 11.
- `docs/assets/badges/{language,flint,licence,tests,julia,suites}.svg`: local two-part badges.
- `docs/assets/status/{open,proved-no,budget,not-math}.svg`: 22 x 22 icons of the four status classes (dashed ring,
  ring with slash, dotted square, plain `!`, as `ux/bench` draws its wells).
- `lanes/docs-readme/make_assets.py`: writes every SVG above from live driver runs; refuses to write if a session
  output differs from the strings the README quotes. Two runs give byte-identical files (12 of 12).
- `lanes/docs-readme/terminal_session.sh`, `places_digits.sh`: the driver commands of the two pictures.
- `lanes/docs-readme/check_readme_commands.py`, `check_links.py`, `check_svgs.py`: the checks below.
- `lanes/docs-readme/frames.mjs`: renders frames of an SVG at given animation times (Playwright, existing headless
  Chromium); used for the visual check only.
- `lanes/docs-readme/numbers.md`: every number with its command and pasted output.

## Checks run (commands from the worktree root)

| Check | Command | Result |
|---|---|---|
| build | `timeout 600 make -j2 && timeout 600 make -C tools/adf` (fresh `env -i` shell) | exit 0 |
| clean build time | the same in a scratch copy of `Makefile src include tests/support tools/adf`, load 1.91 | wall 18.31 s |
| shared library | `timeout 600 sh tests/test_exports.sh` | 625 of 625 exported, 17.3 s |
| README commands as written | `timeout 900 python3 lanes/docs-readme/check_readme_commands.py` (each `$ ` line in `env -i`, under `timeout`, Julia with a new empty depot; byte comparison with the README's lines) | 9 commands: 9 agree, 0 differ (about 11 s) |
| terminal session | `sh lanes/docs-readme/terminal_session.sh`; also EXPECT in `make_assets.py` | 7 of 7 outputs as shown |
| places numbers | `sh lanes/docs-readme/places_digits.sh` | 12 residues (10, 61, 271 at depth 4), real line, 5 valuations, as drawn |
| status examples | the four driver lines of the status table | `NOT_DETERMINED`, `DOMAIN`, `LIMIT`, `PARSE` as shown |
| independent arithmetic | Python integers: `14557^2+1`, `1068^2+1` mod `5^6`; `7*146` mod 1000; squares mod 3 | 0, 0; 1022 = 22; no root of -1 |
| independent analysis | mpmath `pi/6`, `pi^(-1/4) Gamma(1/4) zeta(1/2)` | 0.52359877559829887308, -3.9769662255065128793, inside the driver's balls |
| SVGs | `python3 lanes/docs-readme/check_svgs.py` (minidom parse; width, height, viewBox; no `<script>`, no `on*=`, no http href) | 12 files, 0 with a problem |
| links (D-DOC7) | `python3 lanes/docs-readme/check_links.py` | 41 relative targets checked: 38 exist, 0 broken, 3 pending other lanes (`docs/GUIDE.md`, `julia/Adelefeld/README.md`, `docs/README.md`); 1 in-page anchor |
| anchors | GitHub slug rule against the headings of `docs/SPEC.md` and `README.md` | 4 of 4 resolve |
| command names | each of the 79 names of the details list as a string literal in `tools/adf/adf.c` | 0 missing |
| line width | Python, characters per line of `README.md` | 1 line over 116: line 275, verbatim `tate_fe` output (123 characters), kept unwrapped because it is output |
| visual | `node lanes/docs-readme/frames.mjs` at t = static, 0.2, 1.5, 4, 9.8, 12.5, 27.9 s; places static and 2 s; badges and icons on white and on `#0d1117` | read by eye: typing, cursor, outputs, fade, the static frame complete; badges and icons legible on both backgrounds |
| counts | `ls tests/test_*.c \| wc -l` | 107 |

## What I could not verify

- How GitHub renders the SVGs. I checked them by parsing and by rendering them in the local headless Chromium
  (Playwright's `chromium_headless_shell-1248`), with the animation clock set by `document.getAnimations()`. GitHub
  serves them through `<img>`, where CSS animations normally play, but I did not see the page on GitHub. The same
  goes for Safari, and for fonts other than the ones on this machine. The text is pinned with `textLength`, so the
  layout does not depend on the font's width.
- The mermaid diagram: I had no mermaid renderer offline. I read it for syntax only (flowchart, subgraph with
  `direction LR`, a dotted labelled edge).
- The README as a whole page on GitHub (the HTML status table, `<details>`, the centred hero) was not rendered.
- The Julia count 5369 and "5 of 5 suites green" are quoted from `HANDOFF.md` line 22 (commit `a9a3bf4`), not
  computed: the brief forbids running the suites. The badge names the commit. The build notes say the counts come
  from `HANDOFF.md`.
- The `git clone` line was not run (network). The build lines were run in the worktree and in a scratch copy.
- The milestone states come from `HANDOFF.md` and `docs/reviews/`. For milestone 3 I found a review of WP 3.1 and
  3.2 only (`docs/reviews/m3/review-m3-wp31-wp32.md`), so the table says just that.

## Deviations from the brief, with reasons

- Section 2 is titled "What an adele is", not "... in 120 words": it has 169 words. D-DOC1 wants place, p-adic,
  Zhat and ball each defined in one or two sentences, and I kept the definitions rather than the count.
- The terminal shows shell one-liners (`echo '...' | build/adf`), not a prompt with an answer per line. The driver
  reads the whole script before it answers (`tools/adf/adf.c:4455`, "the whole script is read first"; a pty test
  showed no answer per line). An animated REPL would therefore be untrue.
- Order: a short paragraph for the reader ("you know leading digits, you know trailing digits") sits between the
  hero and section 2, as the bridge to it. A "subtlety" box after the places picture explains why `recover` gives
  `22/7` although `22/7` is not in `146 + 1000 Zhat` (SPEC 9.2). The brief's wording "a ball 146 + 1000 Zhat as
  known modulo 1000" invites that misreading, and the docs/ux line "the reconstruction 146 mod 1000 = 22/7"
  means the residue sense.
- Added beyond the brief: one Tate's-thesis line in "Five minutes" (`tate_integral` at `s = 2` gives `pi/6`, checked
  against mpmath), and `tate_fe` at `s = 1/2` in the details.

## Findings against the brief and DOCS-COMMON (not against SPEC)

1. `DOCS-COMMON.md` says `make -j2 && make -C tools/adf` gives `build/libadelefeld.so`. It gives only
   `build/libadelefeld.a` (`Makefile:81`) and `build/adf`; `sh tests/test_exports.sh` builds the `.so`. This is
   the orchestrator's correction too. The README's Julia block has `sh tests/test_exports.sh` first, and I ran
   it as written.
2. `DOCS-COMMON.md`'s session example `echo 'roots 2 -1 at 5 prec 8' | build/adf` is not driver syntax: it prints
   `error: PARSE`. The README uses this very line as its example of a "not mathematics" status. The driver's forms are
   `roots 1 0 1 with 5 with 8` (polynomial `1 + X^2`) or `printf 'prec 8\nroots_at -1 with 5 with 2\n'`.
3. For the orchestrator: a working headless browser exists on this machine. Playwright's
   `~/.cache/ms-playwright/chromium_headless_shell-1248/.../chrome-headless-shell` renders SVG and HTML, and through
   the global `@playwright/cli` package (`lanes/docs-readme/frames.mjs`, with `executablePath`, since the package
   asks for build 1212) a page's animations can be paused at any time. HANDOFF item 6 says no UX slice has a
   screenshot because Firefox headless exits by signal 11; this may close that gap. `--virtual-time-budget` does not
   advance CSS animations in that shell; setting `currentTime` does.

## Findings against the specification

None. The SPEC header still says "nothing of the C library is implemented" (line 3 to 4, version 1.3 of
2026-09-28). That is a stale status line, not a wrong rule, and the README does not repeat it.

## Library demands

- None new. The page would use a `version` command of the driver (already a UX demand in `HANDOFF.md`, step 3) to
  print the library and FLINT versions in the terminal, and a driver mode that answers line by line (a prompt) to
  make a truly interactive session.

## Not done

- No `<picture>` variants for the light theme: both pictures are drawn on their own dark card, which reads on both
  GitHub themes.
- Nothing under `ux/`, `julia/`, `docs/GUIDE.md` or `docs/README.md` was touched. The three links to the other
  docs lanes' files are pending their merge.
- Sources pending: none. The formulas cited (Tate's zeta integral) were read in
  `refs/src/tate-warwick/tatesthesis_notes.txt` lines 510 to 512, by way of SPEC lines 476 to 477.
