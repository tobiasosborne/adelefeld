# The test drive of a UX slice (lanes ux-drive-*), 2026-10-15

You are the test driver of one slice of the interface of adelefeld (`docs/ux/README.md`: four visions; each
built as its "first buildable slice" under `ux/<dir>/`). You act as the first USER of the slice, not as its
author and not as a code reviewer: a mathematician who has read the vision's brief and wants to do the two
scenarios of the brief with the thing that was built. You did not read the builder's report before the drive;
you read it only at the end, for the comparison (step 5).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md` (the decisions D-UX1 to D-UX6 the slice was
built under; judge against them), the brief of the vision named in your lane brief, and then ONLY
`ux/<dir>/README.md`. Everything else under `ux/<dir>/` you open as a user would: when the README tells you
to, or when something breaks.

You own: `lanes/<lane>/**`. You change NOTHING under `ux/` or anywhere else. If a command needs a fix to run,
you record the fix you would make in the report and stop that path; you do not apply it.

The drive, in this order; record each step as you go in `lanes/<lane>/drive.md` (a log with times from
`date`, the command, what happened, what you expected):

1. Setup from the README alone, in a fresh shell at the repository root (`make -j2` and `make -C tools/adf`
   under `timeout 600` if the README says so or if `build/adf` is missing). Every command under `timeout`.
   Note every step where the README was wrong, missing or ambiguous, and what you had to guess.
2. Scenario A of the brief, step by step, as the brief tells it, with the slice's commands and pages. For
   every number, status, digit, path, well or claim on the screen or in a file, check it yourself: by the
   driver `build/adf` directly (`tools/adf/README.md`), or by your own computation written out in the log
   (p-adic digits, Legendre symbols, roots modulo p^k, reconstruction). A number the slice shows that you
   cannot reproduce is a DEFECT; a number it shows that is right but that you had no way to verify from the
   screen is a FINDING on provenance. Pages: open the HTML in headless Firefox
   (`firefox --headless --profile "$(mktemp -d)" --screenshot lanes/<lane>/<name>.png file://$PWD/ux/<dir>/<page>`)
   where it works, and in any case read the page's DOM by `node` (the pure functions the slice exports, or a
   small script that loads the HTML text and checks the elements by regular expressions: no DOM library is
   installed). Record what you could and could not see.
3. Scenario B of the brief, the same way.
4. Three things the brief promises that the slice should allow beyond the scenarios (your choice, named from
   the brief's "The surface" and "Interaction model"): try them; record whether the slice does them, refuses
   them honestly, or does them wrongly.
5. Now read `lanes/<build lane>/result.md` or `report.md` (the builder's report). Compare: every check the
   builder claims, rerun one of each kind yourself; every "library demand unmet" listed, confirm it is unmet
   (try the driver); every claim about the page, check against what you saw.
6. The verdict and the report `lanes/<lane>/report.md`:
   - DEFECTS: wrong numbers, wrong statuses, crashes, a page that does not render, a README command that
     fails; each with the exact command, the output, the expected value and how you computed it.
   - USABILITY: what a user of the brief's scenarios could not do, had to guess, or misread; in order of
     cost to the user, with the brief's sentence that promised it.
   - HONESTY: every place where the slice shows a value or a state it did not compute, or hides a status
     (D-UX3, D-UX5); every "not given" and whether it is right.
   - VISION: three sentences, no more: does this slice make the vision's case or not, and what one change
     would make the biggest difference for the next slice. This is the sentence the owner will read.
   - The counts: steps taken, checks made, defects, usability findings, honesty findings; the time.
   - FOR THE REPAIR: a numbered list of what the builder's tier must fix, smallest first, each testable.

Rules of COMMON.md hold: 2 cores, 3 minutes per computation, no git state change, no bd, no package
installation. Lines at most 116 characters. Plain English, numbers not adjectives. A screenshot you cannot
take is reported as such, not described from the HTML.
