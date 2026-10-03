# Brief 1: Adelica, the notebook where every number has places

Status: design brief, divergent phase (2026-10-03). Not a plan. Mock: `mock-1-adelica-notebook.html`.

## The idea in three sentences

The owner's first thought, taken literally: a notebook in the line of Mathematica, Pluto and Jupyter, in which an
adele is as ordinary as a float, and in which every output is drawn as a *place strip*: the real ball on the left,
the few interesting primes as digit columns, and one quiet label for all the other primes. The agent is a second
author inside the same notebook: it writes *ghost cells* under the human's cells, each with its result already
computed, and the human accepts, edits or dismisses them with one key. Nothing about the notebook is hidden state:
a chosen branch, a precision and a place are written into the code of the cell, so the file is a script that
replays byte for byte.

## Who uses it and for what

### Scenario A, led by a human: the square roots of 9, then of -1, at 5

1. The human types `roots(9, 2; at = 5)`. The output is a *branch fan*, two chips:
   `[2] -3` and `[3] 3`. These are the identifiers of `roots_at 9 with 5 with 2` in the driver
   (`tools/adf/README.md:459-460`): the seed is the residue of the root modulo 5. Below each chip the 5-adic digits:
   `-3 = ...4442` drawn with a bar over the repeating `4`, `3 = ...0003` with a bar over `0`.
2. The human clicks the chip `[3]`. The cell is rewritten to `root(9, 2; at = 5, seed = 3)` and its output becomes
   the single value 3. The click edited the code; it did not set a mode. Reopening the file gives the same branch.
3. The human changes 9 to -1. The roots are irrational now and the output needs a precision. The chips read
   `[2] 280182 + O(5^8)` and `[3] 110443 + O(5^8)` at the default precision 8; the digits are `...32431212` and
   `...12013233` (low digit on the right; computed for this brief), and the eight known digits are followed on the
   left by a band of fog: the digits above `5^8` are not known.
4. The human drags the fog edge to the left (the adele form of `N[x, 20]`). The cell is rewritten to
   `root(-1, 2; at = 5, seed = 2, prec = 20)` and twelve more digits appear.
5. A ghost cell appears under it, written by the agent: `equal(teichmuller(2; at = 5), root(-1, 2; at = 5,
   seed = 2, prec = 20))`, with its result already shown in grey: `true: the same ball, 5^20`. Next to it, in the
   same weight, `compare(...)` gives `undecided`: two balls can be the same set while the two points inside are not
   proved equal (SPEC 4.2). The margin note supplies the proof the library cannot: "branch [2] of the square root of
   -1 is the Teichmueller lift of 2; its square is the lift of 4, and 4 = -1 mod 5". The human presses Tab; the
   cell turns black and is marked "agent, accepted". The note stays marked "argument, not computed".

### Scenario B, led by an agent: which rational is this?

The agent works for a human who has a long computation modulo 1000 (an image of a rational in `Zhat/1000 Zhat`).

1. The agent receives the residue `146 mod 1000` and writes a ghost cell `recover(146 mod 1000; num = 30, den =
   15)`. It runs it in its shadow kernel first. The result is `22/7`, status `OK`, unique because `2 * 30 * 15 =
   900` is below 1000 (SPEC 9.2, `proofs/solvers.md` Propositions 1.5 to 1.8). Check by hand: `7 * 146 = 1022 = 22
   mod 1000`.
2. The agent does not stop at the residue. It writes a second ghost cell that builds the full adelic ball from the
   human's real estimate, `reconstruct((3.14 +/- 0.01 ; 1/7 mod 1))`, and gets `22/7` from SPEC 9.2's first form:
   the progression `1/7 + k` meets `[3.13, 3.15]` only at `k = 3`.
3. Both cells arrive as a pair with a one-line summary: "146 mod 1000 is 22/7, unique for |n| <= 30, d <= 15;
   the real ball confirms it". The human accepts the pair. The notebook keeps the two certificates (the
   reconstruction certificate of `include/adelefeld/resid.h`, `adf_recon_cert_check`) as attachments of the cells.
4. The human asks "and with bounds 40 and 30?". Now `2 * 40 * 30 = 2400` is above 1000 and the simple uniqueness
   test no longer applies. The agent reruns and shows what changed in the certificate, not only the answer: the set
   is still `{22/7}` (enumeration for this brief agrees), decided from two rows of the Euclidean algorithm since
   `A < m` (SPEC 9.2). Had the search limit cut the search short, the output would have been the status card
   `NOT_DETERMINED: uniqueness not certified` with its two ways forward, a larger limit or tighter bounds.

## The surface

One column of cells, as in every notebook. What is new is the output, and a right margin that belongs to the agent.

```
 In [4]  root(-1, 2; at = 5, seed = 2, prec = 8)
 Out[4]  ┌──────────────────────────────────────────────────────────────────────┐
         │  5 [2]   ░░░░░░░ 3 2 4 3 1 2 1 2      280182 + O(5^8)    8 digits    │
         └──────────────────────────────────────────────────────────────────────┘
 In [5]  x = 6/35
 Out[5]  ┌───────────┬────────┬────────┬─────────┬─────────┬───────────────────┐
         │ R         │ 2      │ 3      │ 5       │ 7       │ every other p     │
         │ 0.1714285…│ v = 1  │ v = 1  │ v = -1  │ v = -1  │ unit, |x|_p = 1   │
         │ exact     │ |x|=1/2│ |x|=1/3│ |x| = 5 │ |x| = 7 │                   │
         └───────────┴────────┴────────┴─────────┴─────────┴───────────────────┘
         product of all |x|_v:  6/35 * 1/2 * 1/3 * 5 * 7 = 1        exact
 ┄┄┄┄┄┄  agent ┄┄ prod(abs(x; at = v) for v in places(x)) == 1      true     [Tab] accept  [Esc] dismiss
```

**The place strip.** Every value of an adelic type prints as a strip of columns.

- The real place is always first and always present, labelled `R` (or `C` for a complex adele). Its ball is drawn
  with its certain digits in ink and its uncertain digits faded, the radius written once at the end.
- Then one column per *interesting* prime, in increasing order: the primes dividing the modulus, the denominator or
  the numerator of the centre. Each column shows the valuation, the absolute value and the p-adic digits, the low
  digit on the right, as in `...4442`. An exact rational has an eventually periodic expansion; the period is drawn
  with an overline (`-1/3` at 5 is `...1313` with the bar over `13`). A ball has fog to the left of its last known
  digit; fog has a fixed texture, not a colour, so it survives a print and colour blindness.
- The last column is always the same sentence for the rest: `every other p: unit` for an exact rational,
  `every other p: Z_p (no information)` for a finite ball. "Almost all places are boring" is one column, not a
  hundred. When the interesting primes are not known because the modulus is not factored, the column says so:
  `primes of 2^3 * 5^3 * c, c = 391 digits, unfactored`. The library never factors silently (SPEC 2, "no
  factorisation"); the notebook offers a button that factors, with a cost estimate.

**Statuses.** A status is an output of its own kind, a *status card*, never a red error banner. It has the status
name in small capitals, the place where it happened (the library reports it through the `where` argument of
`adf_sball_add` and its kin, `include/adelefeld/sball.h:220`), one sentence of reason, and the actions that can
change the answer. The classes get different shapes, so that a reader can tell them apart without colour:

- *Open: more work may help.* `NOT_DETERMINED`, `UNIT_NOT_CERTIFIED`, `NEEDS_SPLIT`, `NOT_UNIQUE`. Dashed border,
  the actions listed.
- *Proved: the answer is no.* `DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`. Solid border, a short proof line.
- *Budget.* `LIMIT`. Dotted border, the bound that was hit and its value.
- *Not a question of mathematics.* `PARSE`, `UNSUPPORTED`. Plain, with the grammar rule or the milestone that brings
  the feature.

**Branches.** A function with several branches returns a branch fan: one chip per branch, labelled by its seed in
square brackets, as the driver prints it. A click pins the seed into the cell's code. A fan never collapses to "the"
answer by default; SPEC 9.3.3 says it plainly: "Whatever the library returns" is not a branch.

**Enclosure versus value.** Hovering any output shows its set in words: `the set 5/3 + 3 Z_3` or `every rational of
the progression 1/7 + Z`. The dump of the value (conventions 10.1) is one click away, for the human who wants to
see exactly what the agent sees.

## Interaction model

**Verbs.** Evaluate, refine (more digits at one place), project (keep only some places), pick (a branch), ask (the
agent), accept and dismiss (an agent cell), pin (a value as a named input), export (the notebook as a driver
script).

**Unit of work.** The cell, with one input and one output. A cell may carry attachments: a certificate, a dump, the
agent's note. A ghost cell is a cell whose author is the agent and that has not been accepted.

**Keyboard first.** Shift-Enter evaluates; Tab accepts the nearest ghost cell; Esc dismisses it; `Alt-Up` and
`Alt-Down` move the fog edge (precision) at the place under the cursor; `[` and `]` cycle through the branches of a
fan. Pointer: drag the fog edge, click a chip, click a place column to project onto it. Voice is not needed.

**What "agent first" means here, precisely.**

1. *The agent reads the same notebook through a different window.* Each output is stored as a record: kind, value
   form, dump, status, failing place, precision per place, the call, the library version. The pretty strip is drawn
   from the record. The agent never parses a picture or a decimal string, which SPEC 10 item 2 calls an enclosure on
   input and not a lossless form.
2. *The agent proposes; the human's kernel decides.* A ghost cell carries the agent's result for preview, but on
   accept the cell is recomputed in the human's kernel and the two records are compared byte for byte. A mismatch is
   shown, not smoothed over.
3. *Statuses are data for the agent.* A `NOT_DETERMINED` with its place is something the agent can act on (raise the
   precision at that one place, split the ball). The agent's ghost cells for an open status are its proposed next
   steps, and the status card lists them.
4. *The agent's notes are anchored.* A note sits next to a place column or a branch chip, not in a chat pane.
5. *No hidden choice.* The agent cannot choose a branch or a precision without writing it into a cell.

## The Mathematica baseline in this vision

Each line: what a Mathematica, MATLAB or Julia user expects; what Adelica offers; the library call underneath.

- **Arithmetic.** `x + y`, `x * y`, `x - y` on rationals, finite balls and adeles, by the tight rules of SPEC 4.3
  (`adf_fball_add`, `adf_adele_mul`, ...). Exact rationals stay exact until they meet a ball.
- **Division.** `x / q` by an exact non-zero rational, `x / y` by an idele (`adf_adele_div_idele`). `x / y` for an
  adele `y` is a status card that explains SPEC 4.5 and offers `unitof(y)`.
- **`N[x, 20]`.** `refine(x; at = 5, prec = 20)`, or a drag of the fog edge. `N` with no place is refused: precision
  is per place. Underneath: the `prec` argument of every function.
- **Elementary functions.** `exp(x; at = 5)`, `Log(x; at = 5)` (Iwasawa), `sin`, `cos`, `root(x, n; at, seed)`
  (`adf_sball_exp_at`, `adf_sball_Log_at`, `adf_sball_root_seed_at`). Without `at` only where SPEC 9.3.1 allows the
  all-places form.
- **`Solve`.** `roots(poly; at = 5, prec = 10)` with a completeness badge, `realroots(poly)` with isolating balls
  (`adf_roots_padic`, `adf_roots_real`).
- **`Simplify`, `Rationalize`.** `recognize(x)`: rational reconstruction, whose outputs are "one", "none", "several"
  and "not certified" (`adf_adele_reconstruct`, `adf_resid_reconstruct`).
- **`Table`.** `table(p -> exp(p; at = p, prec = 4), primes(30))`: a table whose rows are places. The row for 2 is a
  status, `DOMAIN`, because the series for `exp` converges on `4 Z_2` only (SPEC 9.3.2); the table shows it in
  place.
- **`Plot`.** `plot(f; at = 5)`: the Monna map sends `sum a_k 5^k` to `sum a_k 5^(-k-1)` in `[0, 1]`, and the graph
  of `f` on `Z_5` is drawn as a point set in the unit square, one point per ball of a grid. At `R` the usual plot,
  drawn as a ribbon whose width is the radius.
- **`Manipulate`.** `@bind prec slider(1:40)`, `@bind seed branches(roots(...))`: sliders over precision, place and
  branch, in the front end only.
- **Help.** `?Log` opens a page with the domain *per place* (the table of SPEC 9.3.2), the statuses the function can
  return and one worked example per place, linked to SPEC and `docs/api-*.md`.

## What the library must provide

1. **A result record.** One call per output that returns, in a fixed form: the status, the kind (`adf_text_kind`),
   the value form, the dump, the failing place, and the precision at each place. Today these are separate calls and
   the driver prints only the value text or `error: STATUS`; the failing place "is not printed"
   (`tools/adf/README.md:230`). The driver needs a machine mode (one JSON object per line, or the dump plus a
   header).
2. **Value forms in the driver for `adf_lball` and `adf_sball`.** The library's grammar has them (conventions 9.2,
   `lball_v`, `sball_v`), the driver prints its own text for partial balls (`tools/adf/README.md:240-242`).
3. **Dump forms for unit cosets, ideles and classes** (`UNSUPPORTED` in the driver today, README line 120-121).
4. **Digits.** An accessor for the base-p digits of a local ball between its valuation and its precision, and, for
   an exact rational, the pre-period and period of its p-adic expansion. Cheap from the centre, but it belongs in
   the library so that the agent and the screen agree.
5. **The interesting places of a value without factorisation.** A function that returns the prime factors of `H`,
   `d` and the numerator found below a bound, and the unfactored cofactor. The screen must show the cofactor
   honestly.
6. **Reason codes under a status.** `NOT_DETERMINED` covers many causes (the guard of SPEC 9.3.3, a ball meeting a
   pole, a sign not certified). A sub-code and the numbers involved (`N - m = 1 < c + v(n) = 2`) let the card
   propose the right action.
7. **The per-place variant of `f_at`** that SPEC 9.3.1 promises: a map of results and statuses, one per place.
8. **Determinism.** The same call on the same inputs gives the same bytes on the same build, so that accept-and-
   recompute can compare records. A provenance stamp: library version (`adf_version_check`), FLINT version, build
   id.

## Risks and what would make it fail

- **Mathematica with a skin.** If the strip is only decoration and the semantics are the float semantics, users will
  write `x / y` and `x == y` and be surprised. Mitigation: no `==` on balls; `equal`, `contains`, `overlaps`,
  `compare` are distinct and the three-valued `compare` prints `undecided` in the same weight as `equal`.
- **Ghost cells as noise.** An agent that proposes after every keystroke is a distraction. The agent proposes only
  on a status, on an explicit ask, or when a check of the human's claim fails; a cap of one open ghost cell per
  cell.
- **Factorisation.** "The interesting primes" is a factorisation problem in disguise. A notebook that factors
  silently will hang on a 300-digit modulus. The cofactor label has to be in the first release.
- **The host.** Pluto or Jupyter bring their own reactivity. Pluto's reactive cells and Adelica's "the branch is in
  the code" are compatible; Jupyter's out-of-order execution is not, and would break replay.
- **Too many digits.** p-adic digits of large primes are not readable as digits. Above `p = 97` the column switches
  to the residue form `a + O(p^N)` and shows the digit count only.

## A first buildable slice

Two weeks, on what exists: the Julia `ccall` layer (`tests/julia/lroot.jl` shows the pattern) and Pluto.

1. A Julia module with four wrapper types (`Rat`, `FBall`, `Adele`, `LBall`) over the existing C functions, each
   with a `show(io, MIME"text/html")` that draws the place strip. Places from the trial division of item 5 above,
   done in Julia for the slice.
2. `roots(x, n; at)` returning a branch fan, and `root(x, n; at, seed)`; in Pluto, a click on a chip edits the cell
   text (the slice may do this through `@bind` and a code suggestion instead of a rewrite).
3. Status cards for the statuses that the wrapped functions return, with the place.
4. A sidecar file `notebook.records.jsonl` with one record per output (the agent's window), and a command that
   replays a notebook through the driver and compares the records.

Out of the slice: plots, ghost cells inside the editor (the agent writes cells through the sidecar), help pages.
