# Place Atlas

A static viewer for one driver-backed scene. It needs Python 3, Node and the built driver; run from the repo root.

```sh
timeout 180 make -j2 -C tools/adf
timeout 170 sh ux/atlas/build_scenes.sh
timeout 170 python3 -m unittest discover -s ux/atlas
timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs
```

Open `ux/atlas/atlas.html` in Firefox; from `file://` use **Open scene JSON** on a file in `ux/atlas/scenes/`
(served locally, `?scene=scenes/sqrt9.json` and the sample links work). The page makes no driver calls; each
depth handle picks a stored result. Hover or focus a node to read its ball as `[p=P: a + O(P^k)]`.
The scale left of a tree gives each level's exponent k (balls a + p^k Z_p); the dashed line is the `Z_p`
level. A negative valuation draws levels above it: for 1/5 at 5 the root `5^-1` stands one level higher.

## Scenario A: square roots of 9

1. Pick `sqrt9.json`. The lens reads `roots 2 · X^2 - 9 = 0`, with trees at 2, 3, 5 and 7.
2. The ruler shows the driver's exact points -3 and 3. Their blue and amber threads reach all four trees.
3. Each tree has two certified branches. Move each handle to depth 1: at 2 and 3 those branches share one
   drawn path. At depths 2 through 5 all four trees have two distinct paths. Low digits are at the root.
4. Return the handles to depth 5. Read the branch seeds and certificates below each tree, then open the
   transcript. The polynomial solver may certify at greater precision than the requested drawing depth.
5. For the brief's change to -1, generate the command below and pick `minus1.json`. The real line and the
   trees at 2, 3 and 7 have complete empty lists (`none`, not `DOMAIN`, so no gate); at 5 two paths remain.
6. The deepest handle is 5 in this slice. Branch picking, braids and depth 12 are outside it.

```sh
timeout 130 python3 ux/atlas/scene.py -1 --lens 'roots 2' --primes 2 3 5 7 \
  --output lanes/ux-atlas/minus1.json
```

## Scenario B: idempotents modulo 30

1. Pick `idempotents30.json`. The lens reads `roots 2 · X^2 - X = 0` (coefficients `0 -1 1`).
2. Set all handles to depth 1. Each of 2, 3 and 5 lights 0 and 1. The table lists the eight choices:
   0:(0,0,0), 1:(1,1,1), 6:(0,0,1), 10:(0,1,0), 15:(1,0,0), 16:(0,1,1), 21:(1,0,1), 25:(1,1,0).
3. Set the 2-tree to depth 4. There are still two paths: 0 and 1 modulo 16. Every table row has a driver
   square and a `contains` check, `true`. A modular residue such as 6 is not drawn as a rational root.
4. Read the fog bank: every other p; zero undrawn modulus factors below 1000. This is the input's
   footprint, not a claim that the lens was computed there. The input is a finite ball, so it has no thread.
5. Agent narration, eight CRT braids, folding 5, and switching to modulus 36 are outside this slice.

## Build another scene

```sh
timeout 130 python3 ux/atlas/scene.py '(* ; 5 mod 25)' --lens exp --primes 2 3 5 \
  --depth-min 1 --depth-max 5 --output lanes/ux-atlas/exp5.json
```

Values are rationals or finite balls. Lenses: `identity`, `roots n`, `exp`, `log` (the series, not `Log`).
Choose one to four distinct primes up to 13, depths 1..5, root degree 1..64. For `roots`, rational
coefficients are cleared to integers and ball inputs use `roots_at`; `--polynomial '0 -1 1'` overrides X^n-x;
`--modular 30` adds bounded enumeration and driver checks. At denominator primes `roots_at` gives roots
outside Z_p; the polynomial solver lists Z_p roots only. The limit 13 is the geometry of `atlas.js`: a row
of 280 units holds 27 circles of radius 5, and two branches put 2p siblings in one row. A larger prime is
refused before any driver call, exit 2, with one line naming prime and limit; so is a row of over 27.
`--driver PATH` (`ADF=PATH` for script, tests) picks a driver; regenerate into DIR, `diff -ru` it with `scenes/`.

## Status and evidence language

| Class | Shape | Every status name from status.h |
|---|---|---|
| OK | lit result | OK |
| Open | dashed ring | NOT_DETERMINED, UNIT_NOT_CERTIFIED, NEEDS_SPLIT, NOT_UNIQUE |
| Proved no | solid gate | DOMAIN, NO_SOLUTION, NOT_UNIT |
| Budget | dotted circle | LIMIT |
| Not mathematics | plain marker | PARSE, UNSUPPORTED |

The transcript is verbatim, with settings and `error:` lines. A gate belongs to the named call's place; the
failing-place field is `not given`. A `(* ; ...)` input has no real coordinate: if the driver also refuses its
real projection, the gate says "no real coordinate in the input". Inexact real endpoints: `not given`.

Derived numbers (D-UX5 as amended): `scene.py` computes path digits (base-p digits of a printed centre),
sibling hover balls, branch and path counts, fog counts and table digits from printed driver lines, listed in
the scene's `derived` block. A path's `source_record` (the fog's, a table row's `records`) indexes
`provenance.records`, command and output line; each path card names it, and the transcript lists every record.
