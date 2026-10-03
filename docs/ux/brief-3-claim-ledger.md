# Brief 3: The Ledger, where agents propose, the library seals and people ratify

Status: design brief, divergent phase (2026-10-03). Not a plan. Mock: `mock-3-claim-ledger.html`.

## The idea in three sentences

The unit of work is not a cell but a *claim*: a mathematical statement in plain words, the library calls that decide
it, the outcome they must have, and the places it speaks about. Anyone may write a claim, and agents write most of
them, but only the library can *seal* one (its result matched the expected outcome, recorded with version, inputs
and dump), and only a person can *ratify* one, which is the step that lets it be cited. The screen is a ledger of
claims in their states (proposed, sealed, refuted, open, argued, ratified), and each claim shows its *place
footprint*: the places it asserts something about and the places where that assertion has actually been computed.

## Who uses it and for what

### Scenario A, led by a human: refereeing a table of 2-adic and 3-adic values

A referee has a manuscript with a table of local values and wants each line checked.

1. The referee types the table's lines as claims, one per line, in the claim bar:
   `cos(4) = 1 mod 16 at 2`; `v_2(log 3) = 2`; `exp(3) = exp(12) mod 3^8 at 3`; `sin(3) = 3 mod 9 at 3`.
   The bar turns each into a claim with a call script; the referee sees the script before it runs
   (for the first: `cos_at 4 with 2` at `prec 4`, expected `2: 1 + O(2^4)` or a ball inside `1 + 16 Z_2`).
2. The sealer runs the four scripts. Two claims are sealed: `v_2(log 3) = 2` and `sin(3) = 3 mod 9` (both are test
   vectors of the project, SPEC 9.3.2 and PLAN 1F.7).
3. `cos(4) = 1 mod 16 at 2` is *refuted*. The card shows the library's answer, `9 + O(2^4)`, the expected one, and
   the difference in digits: `...1001` against `...0001`, the digit of `2^3` differs.
4. `exp(3) = exp(12) mod 3^8` is *refuted* as well, and the card explains by a second, automatic claim: exp
   preserves distances on `3 Z_3` (SPEC 9.3.2, "these maps preserve distances"), so `v_3(exp 3 - exp 12) = v_3(9) =
   2`: the two agree modulo 9 and not modulo 27. SPEC 9.3.2 records that FLINT's own equality test says "equal" here
   because it ignores precision; the ledger shows that line as a note on the claim, because a referee of a paper
   that used FLINT will want it.
5. The referee ratifies the two sealed claims, writes "wrong in the manuscript; correct value 9 mod 16" on the first
   refuted one, and exports a referee report: a page that lists each claim, its state and its seal.

### Scenario B, led by an agent: an overnight batch on powers and logarithms

An agent was asked in the evening to "write down what is true about `Log`, `exp` and square roots at small primes".
In the morning the human opens the review queue.

1. *Sealed, waiting for ratification (9 claims)*, among them: `exp(Log 5) = 1 at 5`, with the agent's argued note
   "`Log(p^m w u) = log u`, and `5 = 5^1 * 1 * 1`, so `Log 5 = 0`" (SPEC 9.3.2), and the product formula
   `|6/35|_inf * |6/35|_2 * |6/35|_3 * |6/35|_5 * |6/35|_7 = 1` with the other places argued (`6/35` is a unit at
   every other prime).
2. *Refuted (2 claims)*. `exp(Log 5) = 5 at 5`, which the agent wrote first and then refuted itself: the queue keeps
   it, because a refuted claim is information (PLAN 1F.6's test "`exp(Log p) = 1` is not `p`"). And
   `x^2 + 1 has a root in Z_2`: `roots 1 0 1 with 2 with 8` gives `none`; the agent's note says why it guessed
   wrong: `x = 1` is a root modulo 2 and none lifts to modulo 4 (SPEC 9.1).
3. *Open (1 claim)*. `the square roots of 1 + 4 Z_2 are determined to 2 digits`: the library said `NOT_DETERMINED`
   (the guard of SPEC 9.3.3). The agent has already proposed the split as two child claims: on `1 + 8 Z_2` the roots
   are `1 + O(2^2)` and `3 + O(2^2)`, sealed; on `5 + 8 Z_2` there is no root, `DOMAIN`, sealed. The parent stays
   open with a link to its children; it cannot be sealed, because it is not true as stated.
4. *Argued, scope gap (1 claim)*. `2 is a square in Q_p exactly when p = 1 or 7 mod 8 (p odd)`. Its footprint shows
   the primes from 3 to 97 computed and sealed, and "every larger p: argued". The agent's argument cites the second
   supplement of quadratic reciprocity. The ledger refuses to ratify it until the argument cites a local source by
   file and line under `refs/` (the project's own rule, `CLAUDE.md` rules 3 and 4); the human adds the citation and
   ratifies with the scope written in: "computed for p < 100, proved by the cited source for all p".
5. The human spends twelve minutes: nine ratifications, one citation, two refutations read. Nothing was computed
   by the human; everything the human ratified was computed by the library and argued by the agent.

## The surface

```
 LEDGER  ·  powers-and-logs  ·  42 claims  ·  sealed 31 · refuted 4 · open 3 · argued 2 · ratified 26
 ┌───┬─────────────────────────────────────────────────────┬─────────────────────┬──────────┬───────────────┐
 │ # │ claim                                               │ places              │ state    │ by            │
 ├───┼─────────────────────────────────────────────────────┼─────────────────────┼──────────┼───────────────┤
 │ 17│ exp(Log 5) = 1 at 5                                 │ R 2 3 [5] 7 ...     │ SEALED   │ agent · night │
 │ 16│ exp(Log 5) = 5 at 5                                 │ R 2 3 [5] 7 ...     │ REFUTED  │ agent · night │
 │ 15│ x^2 + 1 has a root in Z_2                           │ R [2] 3 5 7 ...     │ REFUTED  │ agent · night │
 │ 14│ sqrt of 1 + 4 Z_2 determined to 2 digits            │ R [2] 3 5 7 ...     │ OPEN  ↳2 │ agent · night │
 │ 13│ 2 is a square in Q_p iff p = 1, 7 mod 8             │ R 2 [3 ... 97] ~~~~ │ ARGUED   │ agent · night │
 │ 12│ |6/35|_v multiply to 1                              │ [R 2 3 5 7] ~~~~~~~ │ RATIFIED │ agent, TJO    │
 └───┴─────────────────────────────────────────────────────┴─────────────────────┴──────────┴───────────────┘
   [ ] computed at these places     ~~~ argued for all other places
```

**The claim card** (opens to the right of the list, or full screen on a phone):

- *Statement*, in plain mathematics, as the author wrote it.
- *Script*: the calls, in the driver's language (`tools/adf/README.md`), so that anyone can rerun them by hand.
- *Expectation*: what the result must satisfy: equal to a value, contained in a ball, a status (`DOMAIN` is a
  perfectly good expected outcome: "3 has no square root in `Q_2`"), a completeness flag, a count of branches.
- *Result*: the value form, the status, the failing place, and, behind a fold, the dump (conventions 10.1).
- *Seal*: the library version (`adf_version_check`), the FLINT version, the build, the date, a hash of the inputs
  and of the result record. A seal says "this library computed this", not "this is true"; the card says so in its
  footer.
- *Argument*: prose for what the library cannot compute (all primes, a lemma), with citations; shown in a different
  type, so that a reader never mistakes argued for computed.
- *Thread*: who wrote, who sealed, who ratified, who commented, children and parents.

**The place footprint.** Each claim has a one-line strip: `R`, then the primes in order, then a tail for "every
other place". A filled box is a place where the claim was computed; a hatched tail is "argued"; an empty tail is
"not claimed". A claim whose statement covers all primes but whose footprint covers four of them is visibly
incomplete. This is the ledger's answer to "almost all places are boring": boring places are allowed to be argued,
never silently assumed.

**States, with shapes as well as colours.** Proposed (outline), sealed (filled seal), refuted (seal struck through),
open (dashed seal with the library status inside: `NOT_DETERMINED`, `NEEDS_SPLIT`, `NOT_UNIQUE`, `LIMIT`), argued
(hatched), ratified (filled seal with a signature mark). An open claim is not a failure; it is a precise question
with the library's reason attached.

**Branches.** A claim about a root must name its branch by seed, or say "all branches". The ledger rejects a claim
that says "the square root of -1 at 5 is 2 mod 5" without a seed and offers the two claims with `[2]` and `[3]`.

## Interaction model

**Verbs.** Propose, seal (only the sealer), refute (a sealed result that contradicts), split (an open claim into
children), argue (attach prose and citations), ratify, retract, cite (one claim in another), export (a report, or
the ledger as plain files).

**Unit of work.** The claim. A *family* is a claim with a parameter (for each prime below 100, for each branch);
it seals member by member and shows a footprint for the family. A *ledger* is a set of claims with a question at
the top.

**Who does what.** Agents propose, argue, split and refute their own claims. The sealer, a program that runs the
library and nothing else, seals. People ratify, comment, retract and set the questions. An agent can see that a
person ratified; it cannot ratify.

**What "agent first" means here, precisely.**

1. *Claims are files.* A claim is a small text file: statement, script, expectation, places, argument. An agent
   writes files; it needs no screen. The screen is a renderer of the directory.
2. *The trust boundary is the sealer.* The agent's transcript is never evidence. A claim's evidence is the record
   that the sealer produced by running the script against the library. An agent that misreports a result cannot get
   a seal; the sealer recomputes.
3. *Refutation is cheap and public.* The agent is rewarded for refuting its own guesses fast; refuted claims stay.
4. *The human's time is the scarce resource.* The review queue orders claims by what a person must decide (scope
   gaps, arguments without sources, open claims whose children are all sealed), and puts the sealed routine at the
   bottom in one batch.
5. *The project already works this way.* `[proved]`, `[checked]`, `[quoted]` and `[design]` in `docs/SPEC.md` are
   ledger states; `tests/driver/*.cmd` with their `.out` files are claims with scripts and expectations.

## The Mathematica baseline in this vision

The ledger has a scratchpad, a plain REPL on the driver's language, for the everyday work. A line in the scratchpad
can be promoted to a claim with one key (it becomes a claim whose expectation is "the result I just saw").

- **Arithmetic and functions.** In the scratchpad as in `adf`: `add`, `mul`, `exp_at`, `roots_at`, `recover`. In a
  claim, the same calls, with an expectation.
- **`N[...]`.** The precision is part of the claim's script; a claim at `prec 8` and the same claim at `prec 40` are
  two claims, and the second cites the first.
- **`Solve`.** A claim about a solution set must state completeness: "the roots of `x^2 - 2` in `Z_7` are exactly
  two, `[3]` and `[4]`" seals only when the library's list is complete (SPEC 9.1).
- **`Table`.** A family; the footprint is the table's index.
- **`Plot`.** Evidence, not a claim: a figure attached to a claim, regenerated from the sealed records.
- **`Manipulate`.** A family over a parameter, explored in the scratchpad, kept as a family.
- **`Simplify`.** `recognize` in the scratchpad; in a claim, "`146 mod 1000` with `|n| <= 30`, `0 < d <= 15` is
  `22/7`, unique", sealed with the reconstruction certificate (`adf_recon_cert_check` checks it independently).
- **Help.** Every function's contract is itself a family of sealed claims: the help page of `Log` is the ledger of
  its test vectors, which a user can read, rerun and extend.

## What the library must provide

1. **A batch runner with records.** The driver, run on a script, returns one record per line: status, kind, value
   form, dump, failing place, precision per place. Today the failing place is dropped (`tools/adf/README.md:230`).
2. **Determinism and a provenance stamp.** Same script, same build, same bytes. A function that returns the library
   version, the FLINT version and a build identifier as one string.
3. **Certificates for everything that can have one, with checkers that do not share code with the solver.** Exists
   for reconstruction (`adf_recon_cert_check`) and linear systems (`adf_linsol_verify`); needed for roots at a prime
   (the Hensel certificate of SPEC 9.1 as an object) and for branch lists (count `gcd(n, p-1)` and the seeds).
4. **Expectation predicates in the library, not in the ledger.** Set predicates exist for finite balls and unit
   cosets (`equal`, `contains`, `overlaps`, `compare`); they are needed for local and partial balls, ideles
   (`docs/api-2.md` 3.5 says ideles have none today) and real balls, so that "the result lies in `1 + 16 Z_2`" is
   decided by the library.
5. **Dump forms for every type** (unit cosets, ideles, classes and partial balls are `UNSUPPORTED` in the driver),
   versioned (`adf1`), so that a seal can be checked against a later build.
6. **Stable status semantics across versions.** If a later build turns a `NOT_DETERMINED` into `OK`, the ledger
   reopens the claim; it needs the status codes of `include/adelefeld/status.h` never to change meaning.

## Risks and what would make it fail

- **Bureaucracy.** If every `1 + 1` must be a claim, nobody will use it. The scratchpad must stay free and fast;
  only what is kept becomes a claim.
- **A seal read as truth.** The library has had defects found by review (the worklogs show it). The seal names the
  build; a later build that disagrees reopens the claim, and the screen says so loudly.
- **Flooding.** An agent can write a thousand claims a night. The queue must summarise, and a ledger needs a budget
  per question.
- **The scope gap is easy to ignore.** If the footprint is small and grey, readers will read "for all p" where only
  four primes were computed. The footprint belongs in the first column, not the last.
- **Arguments are not checked.** The ledger checks that an argument cites a source; it does not check the argument.
  A proof assistant is a later layer, not part of this brief.

## A first buildable slice

Two weeks, almost all of it on what exists.

1. A claim file format: statement, script (driver lines), expectation (exact output lines, as `tests/driver/*.out`
   are today), places, argument.
2. A sealer: a script that runs `adf` on each claim, compares, and writes a seal file with versions, date and
   hashes.
3. A static HTML renderer of a directory of claims: the ledger list with states and footprints, one card per claim.
4. The first ledger: the driver's own cases (`tests/driver/*.cmd`) imported as claims, plus the ten claims of the
   two scenarios above. An agent's first job is to write the next ten as files.
