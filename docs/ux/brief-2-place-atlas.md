# Brief 2: The Place Atlas, a zoomable map of all places at once

Status: design brief, divergent phase (2026-10-03). Not a plan. Mock: `mock-2-place-atlas.html`.

## The idea in three sentences

The screen is a map, not a page: across the top runs the real line, below it stands a shelf of p-adic trees, one per
interesting prime, and at the right edge a fog bank holds all the other primes. A value is drawn where it lives: an
interval on the ruler, a lit subtree in each tree (the ball `a + p^N Z_p` is the subtree below one node at depth
`N`), and, when the value is one rational, a thread that runs through all places and ties them together. The agent
is a guide that builds scenes and drives the camera through them, narrating with library calls, and the human can
take the wheel at any moment; precision, branches and places are handles on the map, not arguments in a text box.

## Who uses it and for what

### Scenario A, led by a human: where do the square roots of 9 live?

1. The human types `9` into the address bar of the atlas and chooses the lens "square roots". The atlas asks which
   places to show; the default is the real place and the primes 2, 3, 5, plus the fog bank.
2. The ruler shows two points, `-3` and `3`. Each tree shows two lit paths:
   - at 2: `3 = ...0011` and `-3 = ...1101` (binary, low digit at the root of the tree), branches `[-1]` and `[+1]`
     in the driver's notation (`tools/adf/README.md:461-462`: the sign labels the unit modulo 4, not the real sign);
   - at 3: `3 = ...0010` and `-3 = ...2220` (ternary), branches `[1]` and `[2]`;
   - at 5: `3 = ...0003` and `-3 = ...4442`, branches `[3]` and `[2]`.
   The fog bank says "every other prime: 2 branches, 3 and -3" (computed at 7, 11, 13 when the human hovers).
3. Two threads, one per rational root, run vertically through the ruler and the three trees. They are the two
   rational square roots; SPEC 9.3.3 says these are what the all-places root returns for an exact rational.
4. The human clicks `+3` on the ruler and at 2, and `-3` at 3 and 5. The threads break; the selection is now a
   *braid*: a square root of 9 in `R x Q_2 x Q_3 x Q_5` that is not a rational. A banner says what this is: "an
   element of the partial product over {inf, 2, 3, 5}; 1 has continuum many square roots in the adeles, 9 too; only
   two of them are rational" (SPEC 9.3.3, "All branches are listed only over a finite named set of places").
5. The human changes 9 to `-1`. The ruler goes dark (no real root: `DOMAIN` at the real place, drawn as a closed
   gate on the ruler). The tree at 3 goes dark too (`-1` is not a square modulo 3). At 5 two paths light up with
   first digits 2 and 3, and the paths now end in fog at depth 8: the roots are irrational, known to `O(5^8)`.
   At 2, `DOMAIN` (`-1` is 7 modulo 8). Every dark place has a one-line reason under it.
6. The human drags the depth handle of the 5-tree down to 12. The two paths grow four nodes each. This is the atlas
   form of `N[x, 12]`: precision at one named prime.

### Scenario B, led by an agent: why does x^2 = x have eight solutions modulo 30?

The human asks the question in the command line of the atlas. The agent builds a tour of four scenes.

1. *Scene 1.* Trees at 2, 3, 5, depth 1. In each tree the agent lights the nodes `0` and `1`, the only idempotents
   of `Z_p` (SPEC 2, fact 1). Narration: "at each prime there are exactly two solutions, 0 and 1".
2. *Scene 2.* The agent draws all eight threads that choose 0 or 1 at each of the three primes and labels each with
   its residue modulo 30, computed by the Chinese remainder theorem: `0 (0,0,0)`, `6 (0,0,1)`, `10 (0,1,0)`,
   `15 (1,0,0)`, `16 (0,1,1)`, `21 (1,0,1)`, `25 (1,1,0)`, `1 (1,1,1)`, in the order (at 2, at 3, at 5). These
   are SPEC 2's list `0, 1, 6, 10, 15, 16, 21, 25`.
3. *Scene 3.* The camera zooms the 2-tree to depth 4. The two lit nodes stay two: modulo 16 the idempotents are
   still 0 and 1. Narration: "depth adds no solutions; primes do". Each step of the tour carries its record: the
   calls `mul e with e` on the exact integers and `contains` of each square in `e + 30 Zhat`, all `true`. (Squaring
   the ball `e + 30 Zhat` itself would give a smaller ball, `36 + 180 Zhat` for `e = 6`: tight, not equal.)
4. *Scene 4.* The camera pulls back to the fog bank. Narration: "in `Zhat` the choice is free at every prime, so
   the solutions are continuum many; a finite ball of positive radius that holds one of them holds infinitely many,
   because it is all of `Z_p` at the primes outside its modulus". The fog bank pulses once per prime it names, then
   stops: the agent does not pretend to draw infinity.
5. The human takes the wheel at scene 3: sets the 2-tree and the 3-tree to depth 2, folds 5 into the bank, and asks
   "and modulo 36?". The agent resumes from the human's camera, not from its own script: four threads, `0, 1, 9,
   28`.

## The surface

```
 ┌─ R ──────────────────────────────────────────────────────────────────────────────────┐
 │ ───────────●───────────────────────┼───────────────────────●───────────────────────  │
 │           -3                       0                       3                         │
 ├─ 2 ────────────────┬─ 3 ──────────────────┬─ 5 ──────────────────────┬─ fog bank ────┤
 │         root       │         root         │           root           │ 7 11 13 ...   │
 │        /    \      │       /   |   \      │     /   /   |   \   \    │ two branches  │
 │       0    [1]     │    [0]   1    2      │    0   1  [2]  [3]  4    │ each: 3, -3   │
 │            / \     │    / \               │            |    |        │ (hover to     │
 │          [0] [1]   │  [1] [2]             │           [4]  [0]       │  compute)     │
 │  lit: 3 = ...0011  │  lit: 3 = ...0010    │  lit: 3 = ...0003        │               │
 │      -3 = ...1101  │      -3 = ...2220    │      -3 = ...4442        │               │
 │  depth < 4 >       │  depth < 3 >         │  depth < 2 >             │               │
 └────────────────────┴──────────────────────┴──────────────────────────┴───────────────┘
   [d] = a lit node, digit d at that depth; two threads, +3 and -3, run down through all columns
```

**The ruler.** The real place is a horizontal axis at the top, with its own zoom. A real ball is a bracket whose
width is the radius; when the radius is below a pixel the bracket turns into a tick with a printed radius.

**The trees.** `Z_p` is drawn as a rooted tree with `p` children per node, the root at the top, the child at depth
`k` labelled by the digit of `p^(k-1)`. A ball `a + p^N Z_p` is the node at depth `N` on the path of `a`, with its
whole cone below it shaded: "everything below here is possible". An exact value is a path that runs off the bottom
edge, marked with its period (the expansion of a rational is eventually periodic). Negative valuation: the tree
grows upward above the root by `-v` levels into `p^(-1) Z_p` and beyond, so `1/5` sits one level above the root of
the 5-tree. Above `p = 13` a tree is drawn as a *residue wheel*: a circle of `p` sectors per level, nested rings
for depth, because a fan of 101 children is not readable.

**The fog bank.** Primes where nothing is known or nothing happens are not drawn; they are counted. For an exact
rational the bank says "unit at every other prime"; for a finite ball with modulus `N` it says "`Z_p`, no
information, at every prime not dividing N". When the primes of `N` are not known, the bank holds the unfactored
cofactor as a labelled stone; factoring it is an explicit act with a cost.

**Threads and braids.** A thread joins the places of one global object. A rational has one thread through all
places. An adele that is not rational has no thread: its places are drawn as separate lights, and the absence of
the thread is the visible statement "this is not a point of Q". The braid of Scenario A is the drawn form of an
element of a partial product.

**Statuses.** A status is drawn at its place, as a state of the place: a closed gate on the ruler or over a tree
(`DOMAIN`, `NO_SOLUTION`: proved, solid), a fogged node with a dashed ring (`NOT_DETERMINED`: open, the ring offers
"split" or "deeper"), a cut at the bottom of a path (`LIMIT`: the budget, with its value). There is no global error
dialog; a status belongs to a place, as the library reports it (the `where` output of `adf_sball_*`).

**Branches.** A fan of roots at a prime is a set of lit sibling paths. Each path carries its seed as a flag at the
node where it leaves the others (for unit roots at an odd prime, depth 1, since the seed is the residue).

**The product formula balance.** For an idele or a rational, a horizontal balance at the bottom: one bar per place
of length `log |x|_v`, to the left when negative and to the right when positive; for 6/35 the bars are
`log(6/35) = -1.764`, `-log 2 = -0.693`, `-log 3 = -1.099`, `log 5 = 1.609`, `log 7 = 1.946`, and they sum to 0. The
balance is level, and it is drawn level only when the library says the norm is exactly 1.

## Interaction model

**Verbs.** Place (a value on the map), lens (which function: identity, square roots, exp, Log, a map), deepen and
raise (precision at a place), pick (a branch, by clicking a path), braid (pick different branches at different
places), fold (send a prime to the fog bank) and unfold (bring it out), tour (play the agent's scene sequence), take
the wheel.

**Unit of work.** The *scene*: a set of places, a depth per place, a camera, the objects on the map and their
records. Scenes are saved, linked and replayed. A *tour* is an ordered list of scenes with narration.

**Pointer first, keyboard second.** Drag the depth handles, click paths, scroll to zoom, drag a prime from the bank
into the shelf. Keys: `+` and `-` deepen and raise at the place under the pointer, number keys pick a branch by its
seed, space plays or pauses a tour. Voice suits this vision: "show me 5 deeper", "why is 2 dark" are natural
requests to the guide.

**What "agent first" means here, precisely.**

1. *The scene is a document the agent writes.* A scene is plain text: places, depths, objects as value forms, the
   lens, the camera. The agent writes scene files; the renderer draws them. A human edit on the map changes the same
   file. The agent can therefore read what the human looked at and where the human stopped.
2. *Every light on the map is a library result.* The renderer draws only what a record says: a lit node comes from a
   ball in a record, a gate from a status. An agent that wants to show "the root at 7" must make the call; it cannot
   paint a node.
3. *The agent narrates with calls.* Each narration line is attached to the calls that justify it; a line with no
   call is marked as an argument, in a different type.
4. *The agent yields.* A tour stops when the human touches the map; the agent continues from the human's state, and
   says what it will do before it moves the camera again.

## The Mathematica baseline in this vision

- **Arithmetic.** The command line of the atlas evaluates `x + y` and `x * y` and places the result on the map next
  to its operands. A loss of precision is seen as a node that moves up its path: `(1/3) * (5 mod 18)` is
  `5/3 mod 6`, one level higher at 3 (SPEC 4.3).
- **Functions.** Lenses: `exp`, `log`, `Log`, `sin`, roots, powers, at the places shown, each place computed by the
  `_at` form of SPEC 9.3.1.
- **`N[...]`.** The depth handle of one tree, or the zoom of the ruler.
- **`Solve`.** `roots(poly)` places each root as a lit path, with a completeness badge per place; a place where the
  list is not complete shows an open ring.
- **`Plot`.** The *map view*: two copies of a tree, the domain above and the image below, and an arc from each node
  at depth `k` to the node of its image ball. The picture of `x -> x^2` on `Z_5` at depth 2 shows the two-to-one
  fold on the units. A Monna-map scatter (`sum a_k p^k -> sum a_k p^(-k-1)`) is the flat alternative.
- **`Table`.** A row of small multiples: the same scene for a list of primes.
- **`Manipulate`.** The handles are the manipulators: depth, branch, place, lens.
- **`Simplify`.** "Find the thread": rational reconstruction tries to tie the lights of an adele into one thread and
  draws it if it exists (`adf_adele_reconstruct`).
- **Help.** The guide; and a legend that explains the drawing of the current scene only.

## What the library must provide

1. **Digits and paths.** For a local ball, the digits from its valuation to its precision; for an exact rational,
   the pre-period and the period. The renderer must not compute p-adic expansions on its own.
2. **Records with places.** Every call returns a record with the status and the failing place (brief 1, item 1).
   The atlas draws a status at a place and cannot do so when the place is dropped, as the driver does today.
3. **Partial balls built from per-place choices.** The braid needs `adf_sball_set_arb_lballs`
   (`include/adelefeld/sball.h:136`) in the driver and the value form `{inf: ...; p=2: ...; p=5: ...}` (conventions
   9.2) in the driver's parser and printer.
4. **Image balls for the map view.** For each node at depth `k`, the image ball under the lens: the existing
   functions on local balls give it, but the cost is `p^k` calls. A batch form, one call for all nodes at a depth,
   keeps the map view interactive.
5. **The support of a value without factorisation**, with the unfactored cofactor (brief 1, item 5).
6. **Exact end points of real balls** (the dyadic midpoint and radius of `arb`), to draw brackets without rounding.
7. **A cost estimate before a call.** A drag of the depth handle from 8 to 2000 at a large prime must be refused or
   warned before the call. The limits exist (`ADF_LBALL_EXP_MAX`, `ADF_LROOT_BRANCH_MAX`); they need a query form.

## Risks and what would make it fail

- **A toy.** A map is good for 9 and 30 and poor for a computation of 200 steps. The atlas is a view on values made
  elsewhere (brief 1 or 3), not the place where work is written down.
- **Exponential trees.** A tree has `p^k` nodes at depth `k`. The renderer draws only the lit paths and their
  siblings; the rest is a shaded cone. Above depth 6 only the paths are drawn.
- **Large primes.** Wheels help up to `p` of a few hundred; beyond that a place is a column of residues.
- **Many places.** More than six trees do not fit; the fog bank and folding are the only answer, and the human must
  choose what to unfold.
- **Build cost.** A custom renderer, scene files, a tour engine and a guide are a large build for one small team.

## A first buildable slice

Two weeks: a static viewer, no server, no agent.

1. A small program that runs the driver (`project`, `roots_at`, `exp_at`) for one value over a chosen set of places
   and writes the results as one JSON file per scene (the slice parses the driver's text, which is acceptable for
   the slice and is why item 2 of the list above matters).
2. A web page that reads a scene file and draws the ruler, up to four trees at depth up to 5, the fog bank label,
   threads for exact rationals, and gates for statuses. Depth handles re-read a precomputed range of depths.
3. Two scenes hand-written as files, the square roots of 9 and the idempotents modulo 30. The agent's tour is the
   second step: once scenes are files, an agent can write them.
