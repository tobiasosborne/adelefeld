# Lane ux-atlas-r1: the repair of the Place Atlas slice after its test drive

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md` (D-UX5 as amended), `ux/atlas/README.md`,
`lanes/ux-atlas/report.md` (the builder's report), `lanes/ux-drive-atlas/report.md` (the test drive: the
defect D1, the usability findings U1 to U4, the honesty finding H1, and "FOR THE REPAIR" items 1 to 5), and
`lanes/ux-drive-atlas/drive.md` where the report points to it. The brief of the slice:
`lanes/ux-atlas/brief.md`; the vision: `docs/ux/brief-2-place-atlas.md`.

You own: `ux/atlas/**` and `lanes/ux-atlas-r1/**`. Nothing else.

Repair, smallest first, each with a test that was red before and green after (`ux/atlas/test_scene.py`,
`ux/atlas/test_atlas.mjs`):

1. D1, item 1: a prime above the wheel limit. Decide the limit from the geometry of `atlas.js` (the sibling
   radius against the row width); the builder refuses a prime above it with one line naming the limit and
   the prime, before any driver call, exit 2; the README states the limit. No overlapping sibling circles
   in any accepted scene: a test over primes 2 to the limit at depth 1 asserts centre distance >= 2 r.
2. Item 2: the effective equation beside the lens: the scene carries the polynomial it actually solved
   (`X^2 - X` for the idempotent scene, `X^2 - 9` for sqrt9) and the page's title and accessible label show
   it; test on both fixtures.
3. Item 3: a gate whose status came from a missing coordinate (the finite-only input projected at `real`)
   says so in its one-line reason ("no real coordinate in the input"), while the raw status `DOMAIN` and
   `failing place: not given` stay; test on the README's exp example.
4. Item 4: negative valuation: the tree of a value with `v_p < 0` draws its levels above `Z_p` with a visible
   baseline labelled `Z_p` and an exponent scale on the left (`p^-1`, `p^0`, `p^1`, ...); the hover balls are
   unchanged; test the layout function on `1/5` at 5 (one level above the baseline).
5. H1: the README and the page footer say which numbers are derived from the driver's printed centres
   (digits, hover balls, fog counts) under D-UX5 as amended, and each such number's source driver line is
   reachable from the scene's provenance block (already there: say how).

Then regenerate the two fixtures (`ux/atlas/build_scenes.sh`), run all tests, and write `result.md`
(COMMON rule 8): each item with its red and green run, the regenerated fixtures' diff (expected: changed
by items 2 and 5 only; say which keys changed), what is left (items 6 and the usability finding U1 are the
next slice, not this repair).
