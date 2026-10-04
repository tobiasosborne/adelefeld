# Lane d-quotient: design of the public interface of milestone 3, first half (the quotient by `Q`, the additive character)

A design document before code, the map for thin slices (`docs/workflow.md` rules 1 and 3), as `docs/api-2.md`
was for milestone 2 (lane d-ideles: read `lanes/d-ideles/brief.md` and `docs/api-2.md` for the form) and
`docs/design/idele-log.md`, `docs/design/local-zeta.md` were for two functions (each implemented afterwards
without rework). No change of `src/` or `include/`. Write for a reviewer who will try to refute you.

Milestone 3 (`docs/PLAN.md` section 6, lines 316-323): 3.1 `adf_qclass` (reduction with splitting, piece
limit; `k` integers crossed give `k + 1` closed pieces, M0-D4; the invariant on the midpoint, CV-45); 3.2 the
additive character; 3.3 `adf_char` (`t^s chi` with conductor and parity); 3.4 Gauss sums. This lane designs 3.1
and 3.2 completely, and 3.3, 3.4 only as far as 3.2 constrains them (the type of a phase, the name of the
kernel; `docs/conventions.md` lines 930-956 fix names and sign conventions, CV-60).

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`; `docs/SPEC.md` section 6 (line 386 on: the
quotient by `Q` and the additive character), 3.1 (statuses, `NEEDS_SPLIT`), 4 (the examples), 15 (every
decision that names the quotient, M0-D4 among them); `docs/proofs/quotient.md` (all of it, 363 lines) and its
review under `docs/reviews/`; `docs/proofs/analysis.md` where the additive character and its phases are
proved; `docs/conventions.md` 5.10 (`adf_qclass`: the struct, the two forms `lift` and `pieces`, CV-45), 5.1
to 5.5 (adeles, finite balls, contexts), 3 and 4 (statuses, outputs, aliasing, limits), 6 (sign conventions of
the character), 9.2 and 9.4 (the value form `(r ; F) + Q`, `union(...) + Q`), 10 (the dump body `qclass`), 11
(golden vectors: `tests/golden/qclass.tsv`, `psi_phases.tsv`, `gauss.tsv`); `proto/quotient_checks.py` (522
lines: what it checks already); the headers `adele.h`, `fball.h`, `scaled.h`, `rat.h`, `idele.h`, `sball.h`,
`rfunc.h` and `docs/api-m1.md`, `docs/api-2.md` for the style; `docs/reviews/s-design/review.md` for what a
review of a design found.

**You own:** `docs/api-3.md` (new), `proto/quotient3_checks.py` (new; exact arithmetic for everything finite;
real phases by exact rationals where the answer is rational and by `python-flint` or `mpmath` with a stated
margin otherwise; it may import `proto/quotient_checks.py`), `lanes/d-quotient/`. Everything else is
read-only. No git command that changes state, no `bd`. At most 2 cores; every program under `timeout`; none
over 120 s. Other lanes are writing `src/dump.c`, `src/localfactor.c`, `tools/adf/adf.c` in other worktrees:
not your files.

## What `docs/api-3.md` contains

1. **The type `adf_qclass`**: its data (conventions 5.10 as it stands: do the struct and the two forms suffice
   for every operation below? if not, say what is missing, as a finding, not as a silent change), its set
   statement (which subset of `A/Q` a value means, for each form), `is_canonical`, the init value, layout
   queries, text and dump (which reader and printer of `text.h` and `dump.h` are missing: declarations only).
2. **For each function of 3.1**: the declaration with a full comment block; the set statement of the result;
   the statement of `docs/proofs/quotient.md` it implements, with the line; statuses and what is written on
   each; aliasing; cost. At least: the class of an adele (`lift`); reduction to the fundamental domain
   `[0, 1] x Zhat` with splitting (the algorithm: the rational translation that makes the finite part
   integral, then the integer translations of the real ball; `k` integers crossed give `k + 1` pieces; the
   piece limit and its status; the outward rounding and the midpoint invariant CV-45; an end point that is not
   dyadic); equality, containment and overlap of classes as SETS (what is decidable: `OK` with a truth value,
   or `NOT_DETERMINED`; equality of the sets after translation by a rational is a PLAN test); translation by
   a rational is the identity; addition and negation of classes (are they in scope? what SPEC 6 says); the
   projection of a class to `R/Z x` (finite quotient) if SPEC 6 names it. Where `quotient.md` has no statement
   for something the plan asks for, write the statement and its proof in a section "Statements to add to
   quotient.md", steps numbered.
3. **The additive character (3.2)**: the definition with its sign convention quoted from `conventions.md`
   and from the source under `refs/` it cites (open the source); the function(s): the character on an adele,
   on a class (well defined on `A/Q`: proved where?), the local factor at a place; the result type (a complex
   ball on the unit circle: `acb`; or an exact root of unity when the input is exact: which type?), the
   statuses: the phase at `(0 ; 1/3)` is non-trivial (PLAN test); a finite ball whose radius is not an
   integer gives an ambiguous phase: `NOT_DETERMINED`, or an enclosure of the arc? (SPEC 6 decides or you
   recommend); additivity; the width of the result as a function of the input radii, proved.
4. **Acceptance tests for each function**: what the test must check so that it fails for a wrong
   implementation (enumeration of small cases in exact rationals; the PLAN tests of rows 3.1 and 3.2; the
   golden vectors; faults to plant: six per work package).
5. **Decisions for TJO**: a table, each with the question, the recommendation and the alternatives. Few and
   real: a decision that SPEC or the conventions already take is cited, not asked again.
6. **Thin slices**: the order in which 3.1 and 3.2 are built, each slice the smallest piece a user can call
   (driver `adf` command and Julia `ccall`), with its functions, its oracle in `proto/quotient3_checks.py`
   and the decisions it needs first. The first slice should be implementable in one lane of about an hour.
7. **Findings** against `docs/SPEC.md`, `docs/PLAN.md`, `docs/conventions.md` or `docs/proofs/quotient.md`:
   contradictions and false statements, each with the counterexample computed by `proto/quotient3_checks.py`.

`proto/quotient3_checks.py` must run: `timeout 120 python3 proto/quotient3_checks.py` prints each check with
its counts and ends with the number of checks. Every example of the design is computed by it, not by hand. It
provides the functions the implementation lanes will use as oracle: `reduce(real_mid, real_rad, finite ball)`
returning the exact pieces before rounding, and `psi(adele)` returning the exact phase as a rational modulo 1
when it is determined, else the arc.

## Report

`lanes/d-quotient/report.md`, written once, at the end; notes in `lanes/d-quotient/progress.md` as you go (a
note after every function designed and every statement proved). In the report: the interface in fifteen
lines; the decisions; the findings; what the reference checks, with numbers and what would have made a check
fail; what is not designed and why; sources pending. No praise, no summary of what the files already say.
