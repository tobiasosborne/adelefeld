# docs/ux: four visions of the interface (divergent phase, 2026-10-03)

Asked for by TJO: "a super adele-ified version of mathematica/matlab/julia/etc", "agent first", three or four design
briefs. These are four different, self-consistent visions, not variants of one. Nothing here is a plan; the library
demands at the end of each brief feed `docs/SPEC.md` section 10. Each brief has a static mock of its signature
screen (`mock-<n>-*.html`, open in a browser; nothing is published). Every number in the briefs and mocks was
computed for them (p-adic digits, branch seeds, Legendre symbols, the reconstruction `146 mod 1000 = 22/7`).

## The four briefs

**1. Adelica, the notebook where every number has places** (`brief-1-adelica-notebook.md`). The owner's idea taken
literally: a Pluto- or Mathematica-like notebook in which every output is a *place strip* (the real ball, one
column per interesting prime with valuation and p-adic digits, fog for unknown digits, one column for "every other
p"). Statuses are output cards with their place and the actions that can change them; branches are chips whose seed
is written into the code on click. The agent is a second author who writes *ghost cells* with results attached; the
human accepts with Tab, and the human's kernel recomputes before the cell is kept.

**2. The Place Atlas** (`brief-2-place-atlas.md`). A zoomable map: the real line on top, a shelf of p-adic trees
below (a ball `a + p^N Z_p` is a lit subtree at depth `N`), a fog bank for all other primes. A rational is a thread
through all places; an adele that is not rational has no thread, and a choice of different branches at different
places is a visible *braid*. The agent is a guide that writes scenes and drives the camera, narrating with library
calls; the human takes the wheel at any moment.

**3. The Ledger** (`brief-3-claim-ledger.md`). The unit of work is a claim: a statement, the calls that decide it,
the expected outcome, and its *place footprint* (where it was computed, where it is only argued). Agents propose,
argue and split; only the library (through a sealer program) seals; only a person ratifies. Refuted and open claims
stay in the ledger as information. It is the vision closest to how this project already works (`[proved]`,
`[checked]` labels; driver fixtures as claims with expected output).

**4. The Bench** (`brief-4-sweep-bench.md`). A plate reader for places: a wall with one row per place and one
column per input, each cell a well whose shape is the status class, and a hard *stop line* below which nothing is
claimed. A team of three agents (sweeper, pattern finder, skeptic) runs sweeps, proposes patterns and attacks them;
the human asks the questions and keeps the findings. It is the vision built for "now do it for every prime below a
thousand" and for several agents at once.

## Comparison

| | 1 Notebook | 2 Atlas | 3 Ledger | 4 Bench |
|---|---|---|---|---|
| Metaphor | notebook | map of places | register of claims | plate reader |
| Unit of work | cell | scene, tour | claim, family | experiment, sweep |
| Agent's role | co-author | guide, camera | proposer, arguer | team of three |
| Human's role | author | explorer | ratifier | asks, keeps |
| Places shown as | strip of columns | ruler, trees, fog | footprint line | rows to a stop line |
| Enclosure shown as | digits and fog | lit subtree | value in card | well and precision |
| Statuses shown as | cards by class | gates at places | claim states | well shapes |
| Branches | chips, seed in code | lit sibling paths | seed required | count in a well |
| Build cost | low to medium | high | low | medium |
| Best at | daily computing | intuition, teaching | trust, refereeing | scale, conjectures |
| Gives up | scale, overview | long computations | spontaneity | structure across places |

Notes on the table. "Build cost" is for a usable first version beyond the two-week slice of each brief. The notebook
is low to medium because Pluto and the Julia `ccall` layer exist (`tests/julia/`); the ledger is low because the
driver, its fixtures and the dump form exist; the atlas needs a custom renderer.

## What could be combined

- **One status language for all four.** Four classes, each with a shape: open (dashed: `NOT_DETERMINED`,
  `UNIT_NOT_CERTIFIED`, `NEEDS_SPLIT`, `NOT_UNIQUE`), proved no (solid: `DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`), budget
  (dotted: `LIMIT`), not mathematics (plain: `PARSE`, `UNSUPPORTED`). Every brief arrived at it independently.
- **The place strip as the universal printer.** Brief 1's strip is how a value looks in a ledger card, a bench
  well's tooltip and the atlas legend.
- **Footprint and stop line are one idea.** Brief 3's footprint and brief 4's stop line both say: a statement about
  all places shows where it was computed, where it is argued, and where nothing is said. Any combination should keep
  this; it is the honest form of "almost all places are boring".
- **The sealer under everything.** Ghost cells (1), tour steps (2) and findings (4) can all be backed by the sealer
  of brief 3: agent output is a proposal until the library has recomputed it.
- **Views, not products.** A plausible whole: the notebook as the daily surface, "open in atlas" on any value,
  `table` in the notebook becoming a bench sweep, and "keep" in the bench or notebook writing a claim to the ledger.

## What every vision asks of the library

Gathered from the four briefs; each item is argued in at least two of them.

1. A *result record* per call: status, kind, value form, dump, failing place, precision per place; and a machine
   mode of the driver that prints it. Today the driver drops the failing place (`tools/adf/README.md:230`).
2. Value forms of `adf_lball` and `adf_sball` in the driver, and dump forms for unit cosets, ideles, classes, local
   and partial balls (`UNSUPPORTED` in the driver today).
3. Reason codes under a status, with the numbers involved (the guard `N - m < c + v(n)` of SPEC 9.3.3).
4. Digits: base-p digits of a local ball, and pre-period and period for an exact rational.
5. The interesting primes of a value without factorisation: factors found below a bound and an unfactored cofactor.
6. Determinism and a provenance stamp (library, FLINT, build), so that records can be compared and replayed.
7. Cost estimates before a call, and budgets; thread safety as SPEC 10 item 1 requires.
8. The per-place variant of `f_at` that SPEC 9.3.1 promises, and set predicates for local and partial balls.
9. Certificates as objects for roots at a prime and branch lists, with checkers that share no code with the solver.

## Three questions for the owner

1. **Who is the first user: you at the keyboard, or agents working while you are away?** If you, start from the
   notebook (1) and borrow the strip; if agents, start from the ledger (3) or the bench (4), whose trust model does
   not depend on a person watching.
2. **What do you want to keep and cite a year from now: a notebook, a list of sealed claims, or experiments with
   findings?** The answer fixes the unit of work and the file format that everything else is built around.
3. **How much should the interface teach the geometry of the places?** The atlas (2) makes p-adic trees and
   local-global threads visible to students and readers of papers, at the highest build cost; the other three show
   places as text and tables. Is the picture part of the product, or a view to add later?
