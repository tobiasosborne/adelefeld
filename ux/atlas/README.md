# Place Atlas

A static viewer for one driver-backed scene. No dependencies beyond Python 3, Node and the built driver.
Run the following commands from the repository root. Nothing is installed or published.

```sh
timeout 180 make -j2 -C tools/adf
timeout 170 sh ux/atlas/build_scenes.sh
timeout 120 python3 -m unittest discover -s ux/atlas
timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs
```

Open `ux/atlas/atlas.html` in Firefox. From `file://`, use **Open scene JSON** and choose a file in
`ux/atlas/scenes/`. A page opened from disk cannot load a query scene; the picker is the supported route.
If already served locally, `atlas.html?scene=scenes/sqrt9.json` loads that scene; sample links then work.
The page makes no driver calls. Each tree's depth handle chooses an already computed result.
Hover or keyboard-focus a node to read its ball in the driver's `[p=P: a + O(P^k)]` value form.

## Scenario A: square roots of 9

1. Pick `sqrt9.json`. The lens is `roots 2`, polynomial `-9 0 1`, with trees at 2, 3, 5 and 7.
2. The ruler shows the driver's exact points -3 and 3. Their blue and amber threads reach all four trees.
3. Each tree has two certified branches. Move each handle to depth 1: at 2 and 3 those branches share one
   drawn path. At depths 2 through 5 all four trees have two distinct paths. Low digits are at the root.
4. Return the handles to depth 5. Read the branch seeds and certificates below each tree, then open the
   transcript. The polynomial solver may certify at greater precision than the requested drawing depth.
5. To inspect the brief's change to -1, generate the command below and pick `minus1.json`. The real line
   and the trees at 2, 3 and 7 have complete empty lists; at 5 two paths remain. This solver prints `none`,
   not `DOMAIN`. The ruler therefore says no real roots, rather than inventing a gate.
6. The deepest handle is 5 in this slice. Branch picking, braids and depth 12 are outside it.

```sh
timeout 130 python3 ux/atlas/scene.py -1 --lens 'roots 2' --primes 2 3 5 7 \
  --output lanes/ux-atlas/minus1.json
```

## Scenario B: idempotents modulo 30

1. Pick `idempotents30.json`. The lens label is `roots 2`; the explicit polynomial is `0 -1 1`, or X^2-X.
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

Values are rationals or finite balls. Lenses are `identity`, `roots n`, `exp`, `log` (the series, not `Log`).
Choose one to four distinct primes below 1000, depths 1..5, and root degree 1..64. Rational coefficients
are cleared to integer coefficients for `roots`; ball-valued root inputs use `roots_at`. An explicit
`--polynomial '0 -1 1'` overrides X^n-x. `--modular 30` adds bounded enumeration and driver checks.
At denominator primes, `roots_at` supplies roots outside Z_p; the polynomial solver only lists Z_p roots.
Use `--driver PATH`, or `ADF=PATH sh ux/atlas/build_scenes.sh`, for another compiled driver.
An isolated lane build uses `timeout 170 make -j2 BUILD=lanes/ux-atlas/build all`, then
`timeout 120 cc -Iinclude tools/adf/adf.c lanes/ux-atlas/build/libadelefeld.a -lflint -lgmp -lm` with
`-o lanes/ux-atlas/build/adf` appended to that same command. Set `ADF=lanes/ux-atlas/build/adf` afterward.
To verify regeneration: pass a fresh output directory to `build_scenes.sh`, then `diff -ru` it with `scenes/`.

## Status and evidence language

| Class | Shape | Every status name from status.h |
|---|---|---|
| OK | lit result | OK |
| Open | dashed ring | NOT_DETERMINED, UNIT_NOT_CERTIFIED, NEEDS_SPLIT, NOT_UNIQUE |
| Proved no | solid gate | DOMAIN, NO_SOLUTION, NOT_UNIT |
| Budget | dotted circle | LIMIT |
| Not mathematics | plain marker | PARSE, UNSUPPORTED |

The transcript is verbatim, including settings and `error:` lines. A gate belongs to the named call's
place; the driver's failing-place field is `not given`. Inexact real dyadic endpoints are `not given`:
the page draws a plain marker and retains the driver's printed digits. Exact dyadic points get brackets.
Digits and hover text come from `scene.py`; it includes a source and finite-digit proof. Periods are not supplied.
