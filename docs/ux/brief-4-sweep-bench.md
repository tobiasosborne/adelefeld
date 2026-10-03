# Brief 4: The Bench, a wall of places run by a small team of agents

Status: design brief, divergent phase (2026-10-03). Not a plan. Mock: `mock-4-sweep-bench.html`.

## The idea in three sentences

Adelic questions are asked at many places at once, so the main object on screen is a *wall*: one row per place (the
real place, then the primes in order, then an explicit line where the sweep stops), one column per input, and in
each cell the library's answer or status at that place, drawn as a well in a plate reader. The work is done by a
small team of agents with fixed roles: a *sweeper* that runs grids of calls, a *pattern finder* that reads walls and
proposes conjectures, and a *skeptic* that hunts counterexamples and checks what a finding's evidence covers. The
human sets the question, reads the findings, steers the next sweep and decides what is kept; the bench is built for
the moment when a person says "do that for every prime below a thousand".

## Who uses it and for what

### Scenario A, led by a human: for which primes is 2 a square?

1. The human asks: "which small integers are squares at which primes?". The sweeper proposes a grid and shows its
   cost before running: `n` in `{-1, 2, -2, 3, 5, 6, 7}`, places `R` and the 15 primes up to 47, the call
   `roots_at n with p with 2` (and `realroots` for `R`), 112 calls. The human accepts.
2. The wall fills. Each cell is a well: a filled disc with `2` where `x^2 = n` has two roots at that place, an empty
   ring with a slash where the library proved there is none (`DOMAIN`, the existence criterion of SPEC 9.3.3 fails).
   The row for 2 is empty: none of the seven is a square in `Q_2` (2, -2 and 6 have odd valuation, and -1, 3, 5, 7
   are not 1 modulo 8). The column for `-1` has two roots at 5, 13, 17, 29, 37,
   41.
3. The pattern finder writes under the column `2`: "two roots at 7, 17, 23, 31, 41, 47: exactly the swept odd primes
   with `p = 1 or 7 mod 8`. State: pattern, 14 odd primes swept, no exception." It offers the verb *regroup*: the
   rows are re-sorted by `p mod 8`, and the column becomes two solid blocks.
4. The human asks for more: "every prime below 1000". The sweeper shows the cost (168 rows, one column, 168 calls)
   and runs it. The skeptic reports: "no exception below 1000; the wall stops at 997; above it, nothing is claimed".
5. The finding is kept as: "2 is a square in `Q_p`, `p` odd, exactly when `p = 1, 7 mod 8`: swept for `p < 1000`; a
   theorem (the second supplement of quadratic reciprocity), source not on disk". The bench marks the missing source
   as the one open item, since a statement is not ground truth until its source is under `refs/` (`CLAUDE.md` rule
   4).

### Scenario B, led by agents: a polynomial with a root at every place and no rational root

The human asks the question in one line and leaves.

1. The pattern finder proposes a family: `(x^2 - a)(x^2 - b)(x^2 - ab)`. At a prime not dividing `2ab`, if neither
   `a` nor `b` is a square modulo `p` then `ab` is, because the Legendre symbol is multiplicative; so only the real
   place, 2 and the primes dividing `ab` need a computation. It writes this as an *argument* attached to the family,
   in the argument type, not the computed type.
2. The sweeper runs a grid: squarefree `2 <= a < b <= 30` with `ab` not a square, the real place and every prime
   below 2000, the cell being "one of `a`, `b`, `ab` has a square root at this place". The wall of each pair is
   folded to one column summary: full, or the first place where it fails.
3. Seven pairs keep a full column: `(2, 17)`, `(5, 29)`, `(10, 26)`, `(13, 17)`, `(13, 29)`, `(17, 19)`, `(17, 26)`
   (computed for this brief by the same criterion). The pattern finder flags `(13, 17)`: it is the example of
   SPEC 9.2, which states the fact as proved (M12).
4. The skeptic attacks `(13, 17)`: it checks that the polynomial has no rational root (`realroots` gives six
   irrational isolating balls and no exact point; 13, 17 and 221 are not squares of integers), and checks the scope:
   computed at `R`, 2, 13, 17 and all primes below 2000; argued (step 1) for the rest. The scope is complete only
   with the argument, and the finding says so.
5. In the morning the human reads one finding with three parts: the wall (with the 2-row, the 13-row and the
   17-row picked out, since they are where the argument does not apply), the argument, and the skeptic's report.
   The human sends it to the ledger of brief 3 as a claim family, or simply keeps it.

## The surface

```
 QUESTION  which small n are squares at which places?      sweep: 8 inputs x 16 places = 128 calls
 ┌───────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┬────────────────┬──────────────────────────┐
 │ place │  -1  │   2  │  -2  │   3  │   5  │   6  │   7  │ (* ; 9 mod 5^6)│ findings                 │
 ├───────┼──────┼──────┼──────┼──────┼──────┼──────┼──────┼────────────────┤                          │
 │ R     │  ⊘   │  ●2  │  ⊘   │  ●2  │  ●2  │  ●2  │  ●2  │   —            │ F1 col 2: two roots iff  │
 │ 2     │  ⊘   │  ⊘   │  ⊘   │  ⊘   │  ⊘   │  ⊘   │  ⊘   │   ◌ ND         │    p = 1, 7 mod 8        │
 │ 3     │  ⊘   │  ⊘   │  ●2  │  ⊘   │  ⊘   │  ⊘   │  ●2  │   ◌ ND         │    pattern, 14 primes    │
 │ 5     │  ●2  │  ⊘   │  ⊘   │  ⊘   │  ⊘   │  ●2  │  ⊘   │   ●2 O(5^6)    │ F2 col -1: two roots iff │
 │ 7     │  ⊘   │  ●2  │  ⊘   │  ⊘   │  ⊘   │  ⊘   │  ⊘   │   ◌ ND         │    p = 1 mod 4           │
 │ ...   │      │      │      │      │      │      │      │                │ F3 last col: a ball mod  │
 │ 47    │  ⊘   │  ●2  │  ⊘   │  ●2  │  ⊘   │  ●2  │  ●2  │   ◌ ND         │    5^6 says nothing at   │
 ├───────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴────────────────┤    other primes          │
 │ p >= 53: not swept. Nothing is claimed here.                            │                          │
 └─────────────────────────────────────────────────────────────────────────┴──────────────────────────┘
   ●2 two roots (OK)   ⊘ none, proved (DOMAIN)   ◌ ND not determined   — not defined at this place
```

**The wall.** Rows are places: `R` first, then primes in increasing order, then a *stop line*. Below the stop line
the wall is blank, with one sentence: "not swept". The stop line is the bench's statement of "almost all places":
the bench never fills unswept rows with a guess, and it never draws a thinning gradient that suggests convergence.
A finding may cover the rows below the line only by an argument, which is drawn as a hatched band, separate from the
wells.

**The wells.** A cell is a well with a shape for each class of answer: a filled disc with a count (branches, roots)
for `OK`; an empty ring with a slash for a proved no (`DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`); a dashed ring for an
open status (`NOT_DETERMINED`, `NEEDS_SPLIT`, `NOT_UNIQUE`, `UNIT_NOT_CERTIFIED`); a dotted square for `LIMIT`; a
dash for a question that has no meaning at the place (a finite ball at `R`). Colour repeats the shape, it does not
replace it. A well can show a precision as a small number (`O(5^6)`), and hovering it shows the full record.

**The columns.** An input is a value form: `2`, `-1`, `(* ; 9 mod 15625)`, `(3.14 +/- 0.01 ; 1/7 mod 1)`. The column
header carries the input's own footprint: for `9 mod 5^6` the header says "information at 5 only". The last column
of the example shows the lesson of SPEC 2: a finite ball is all of `Z_p` outside its modulus, so at every other
prime the ball contains 0 and the root is not determined.

**The findings rail.** On the right, the findings of the team, each with its state: *pattern* (seen in the swept
rows), *argued* (an argument covers unswept rows), *sourced* (the argument cites a source on disk), *refuted* (a
counterexample in a well, linked). A finding points at the wells it rests on; hovering it lights them.

**The team strip.** At the top, the three agents and what each is doing: "sweeper: 168 calls queued, 41 done";
"pattern finder: reading column 2"; "skeptic: idle, waiting for F1". The human can pause any of them.

## Interaction model

**Verbs.** Ask (a question in words), sweep (a grid of inputs times places times parameters), extend (more places,
more inputs, more precision), regroup (sort rows by a residue, by the value in a column), fold (a wall to a column
summary), pick (a well, to see its record), attack (send a finding to the skeptic), keep (a finding), export (to a
ledger, a notebook, or a file of records).

**Unit of work.** The *experiment*: a question, the sweeps that answered it, the wall, and the findings. An
experiment is a directory of record files; the wall is drawn from them.

**Who does what.** The human asks and decides what is kept. The sweeper turns a question into grids and runs them,
always showing the number of calls first. The pattern finder reads walls and writes findings. The skeptic looks for
the one well that breaks a finding and for the rows a finding claims without evidence. Agents run in parallel; the
bench is the only screen in this set designed for several agents at once.

**What "agent first" means here, precisely.**

1. *The wall is a table of records, not a picture.* Each well is one record from the library: place, input,
   parameters, status, value form, precision. The pattern finder reads the table; the human reads the picture.
2. *Findings are typed by their evidence.* "Pattern", "argued" and "sourced" are different states with different
   drawings. An agent cannot write "for all p" over a wall that stops at 47 without the bench drawing the gap.
3. *Cost is shown before work.* Every sweep shows its number of calls and an estimate of time and memory before it
   runs; a sweep above a budget needs the human's yes.
4. *The skeptic is part of the design.* A finding is not kept until the skeptic has had it, so the default output of
   the team is a finding that has survived one attack.
5. *Statuses are results.* A column of `NOT_DETERMINED` is a finding in its own right ("this input says nothing at
   these primes"), not a failure of the sweep.

## The Mathematica baseline in this vision

- **Arithmetic and functions.** A cell of the wall is any call of the library at one place: `add`, `mul`, `exp_at`,
  `Log`, `root_at`, `powrat_at`, `recover`. A small command line under the wall evaluates one call for quick work.
- **`Table`.** The sweep is `Table` with places as one axis, and it is the main verb of the bench.
- **`N[...]`.** A precision axis: the same column at `prec` 4, 8, 16 shows where a well changes state (a
  `NOT_DETERMINED` that becomes `OK` at a higher precision is visible as a change along the axis).
- **`Solve`.** `roots` at each place of the wall; the well shows the count and the completeness of the list.
- **`Plot`.** The wall is the plot. Regrouping by a residue is the bench's version of choosing an axis. A column can
  also be drawn as a strip over the primes up to a bound (one tick per prime, filled or empty), which shows
  densities such as "half of the primes".
- **`Manipulate`.** A slider on a parameter of the sweep (the degree of the root, a seed, the precision) redraws the
  wall from cached records where they exist.
- **`Simplify`.** The pattern finder: from a column it proposes a rule in terms of `p mod m` and checks it.
- **Help.** A function's help is a ready-made sweep: "show `exp_at` on its domain edge at the first ten primes"
  (the wall makes the special row at 2 visible: `exp` converges on `4 Z_2` there and on `p Z_p` elsewhere).

## What the library must provide

1. **A batch interface with records.** Many calls in one process, each returning status, failing place, value form,
   precision and dump; the per-place variant of `f_at` (SPEC 9.3.1) is the natural core of a column.
2. **Thread safety as written down** (SPEC 10 item 1), so that a sweeper can run calls in parallel without a lock
   around the library; and no hidden global state, which SPEC already requires.
3. **Cost estimates and budgets.** A query that says, for a call, its expected cost class (the bits of `p^N`, the
   number of branches `gcd(n, p - 1)`, the limits `ADF_LROOT_BRANCH_MAX`, `ADF_LBALL_EXP_MAX`), so that the sweeper
   can price a grid before it runs. The cost findings in PLAN (59 s for `log(1 + p)` at `N = 10000`, 44.6 s for one
   Teichmueller lift per branch) are exactly what a sweep would hit.
4. **The catalogue of SPEC 9.3.7, Tier A** (work package 1F.9, not started): Legendre, Jacobi, Kronecker and Hilbert
   symbols are the cheapest and most natural wall cells of all ("the Hilbert symbol `(a, b)_v` at every place, and
   its product formula").
5. **Places as data.** An iterator over primes up to a bound that returns `adf_place_t` handles (R1 of SPEC 3), so
   that a sweep does not test primality per cell.
6. **Stable statuses and reasons** (as in briefs 1 and 3): a pattern finder that groups wells by status needs the
   reason under `NOT_DETERMINED`, or it will merge unrelated causes into one pattern.

## Risks and what would make it fail

- **Pattern without proof.** The bench produces conjectures fast. If the drawing of "pattern" and "sourced" are too
  alike, a reader will cite a pattern. The states must differ in shape and in words.
- **Cost.** A sweep of 168 primes at high precision is cheap; a sweep of roots of degree 299756 is not. Without
  cost estimates the bench will hang on the first ambitious question.
- **The wall does not show structure across places.** It shows each place alone; a fact that ties places together
  (the product formula, a global rational) needs a summary row, or it is lost. The notebook strip and the atlas
  thread are better at this.
- **Agent chatter.** Three agents can write more findings than a person can read. The rail shows at most five open
  findings and folds the rest.

## A first buildable slice

Two weeks, without agents first, then with one.

1. A sweep runner: a script that takes inputs, places, an operation and parameters, runs the driver, and writes one
   record per well (the slice parses the driver's lines).
2. A static wall renderer for one experiment: rows, columns, wells by status class, the stop line, a findings
   column filled by hand.
3. Two experiments as fixtures: the squares wall of Scenario A, and the `(x^2 - 13)(x^2 - 17)(x^2 - 221)` column.
4. Then one agent: the pattern finder, reading the records and writing findings as files with their state.
