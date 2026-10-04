# Lane d-zeta: design and proofs of the local zeta factors (milestone 1F, WP 1F.9, last item), before any code

PLAN row 1F.9 (`docs/PLAN.md` line 279) lists "local zeta factors" with the test "local factors at and near
their poles: a ball containing a pole returns the pole status, never a finite or unbounded ball". SPEC 9.3.7
(`docs/SPEC.md`, table "Tier A", row "local zeta factor (trivial character)"): `(1 - p^(-s))^(-1)` at a prime,
`pi^(-s/2) Gamma(s/2)` at the real place, on a complex ball; poles at `s = 2 pi i k / log p`, and at
`s = 0, -2, -4, ...` at the real place, all simple; "An exact input at a proved nonremovable pole returns
`ADF_DOMAIN`; an input ball that meets a pole and also contains regular points, or whose exclusion of poles is
undecided, returns `ADF_NOT_DETERMINED`. Both leave the value output untouched and no non-finite ball is
stored." Proofs on disk: `docs/proofs/catalogue.md` Proposition 9 (lines 184-200, "proved modulo Gamma
continuation": line 359), `docs/proofs/analysis.md` statement 10; sources `refs/src/tate-poonen/notes.txt:1733`
(finite) and `:1014-1016` (real). `proto/catalogue_checks.py` has `check_zeta`.

Nothing of this exists in code. No function at a place takes a complex argument yet: `include/adelefeld/rfunc.h`
has real balls only; `sball.h` carries a real or complex tag (PLAN 1F.1); `cadele.h` is the complex adele. The
other pieces of 1F.9 are `symbol.h` (lane f-slice12) and `catalogue.h` (lane f-slice13): read both headers,
`docs/api-1f9.md` (Y1 to Y15) and their decisions N-D18, N-D19 (`docs/SPEC.md` 15.4) for the style of names,
statuses and `where`.

`docs/workflow.md` (table): a review of a design before code finds the defects cheaply. The pattern of this lane
is lane d-idlog (`lanes/d-idlog/brief.md`, `docs/design/idele-log.md`, `proto/idlog_checks.py`), which was
implemented afterwards without rework. You write the design, prove it, and build its oracle. You write NO C
code and change no header.

**You own:** `docs/design/local-zeta.md` (new), `proto/zeta_checks.py` (new), `lanes/d-zeta/`. Everything else
is read-only. No git command that changes state, no `bd`. At most 2 cores; every program under `timeout`; none
over 120 s. `refs/src/` is on disk (FLINT 3.0.1 documentation and source of `acb`, `arb`: read what `acb_gamma`,
`acb_rgamma`, `acb_pow`, `acb_exp`, `acb_contains_zero`, `acb_is_finite` promise before you rely on them, and
cite file and line). Two other lanes run in other worktrees (a review in `lanes/f-review12/`; a driver lane in
`tools/adf/adf.c`): you do not touch their files.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `docs/SPEC.md` 3.1 (statuses; `DOMAIN` "keeps
its definition in 3.1"), 9.3.6 (the row on real and complex functions of `arb`/`acb` and branch cuts), 9.3.7,
15.4; `docs/conventions.md` 2 to 5 (statuses, `where`, precision, limits, aliasing); `docs/proofs/catalogue.md`
Proposition 9 and its review; `docs/proofs/analysis.md` 10; `include/adelefeld/rfunc.h`, `sball.h`, `cadele.h`,
`place.h` (or where `adf_place_t` is declared), `symbol.h`, `catalogue.h`; `docs/api-1f2.md` if present (the
archimedean wrappers: guard digits, translation of statuses) and `docs/api-1f9.md`.

## What `docs/design/local-zeta.md` contains

1. **The mathematics, as statements with complete proofs**, steps numbered, each step checkable:
   - The finite factor `L_p(s) = (1 - p^(-s))^(-1)` as a meromorphic function of `s`; its poles are exactly
     `2 pi i k / log p`, `k` an integer, all simple; residue. The real factor `pi^(-s/2) Gamma(s/2)`; its poles
     are exactly `0, -2, -4, ...`, simple; no zeros. Close the gap "modulo Gamma continuation" of Proposition 9
     or say exactly which quoted statement (file and line under `refs/`) it rests on.
   - **Which exact inputs are poles.** An exact input is a complex number with dyadic real and imaginary part
     (a ball of radius 0). At a prime: `s = 0` is a pole; prove that no other exact input is one (`2 pi k /
     log p` is not rational for `k != 0`: give the proof, with the theorem it needs quoted from a source on
     disk or marked `[source pending]`). At the real place: the exact poles are the non-positive even integers.
   - **The decision rule for a ball**, as a theorem: a procedure on the input ball `S` and the prime `p` (or
     the real place) with three outcomes: (a) certified pole-free: then the enclosure of `L(S)` is finite and
     contains `L(s)` for every `s` in `S`; (b) `DOMAIN`: `S` is a single point and it is a pole; (c)
     `NOT_DETERMINED`: everything else, with `where` the place. Prove soundness: outcome (a) is never returned
     for a ball that contains a pole. Say which computation certifies (a) (for example: the ball of
     `1 - p^(-s)` computed by `acb` does not contain 0; at the real place the ball of `s/2` contains no
     non-positive integer, or `acb_rgamma` of it does not contain 0: compare the alternatives, their cost and
     their tightness, and prove the one you recommend). State how tight the rule is: a ball at distance `d`
     from the nearest pole with radius `r < d`: for which `r/d` and `prec` is (a) guaranteed? A ball that
     contains a pole and regular points is (c); a ball of positive radius that is pole-free but too wide for
     the certificate is (c) too: say so.
   - The enclosure near a pole: the size of `L(S)` for `S` at distance `d`: no hidden overflow; what happens
     at `Re(s)` hugely negative or positive (`p^(-s)` overflows or underflows the `arf` exponent range: which
     status? `LIMIT`? read conventions on limits) and at huge `|Im s|`.
2. **The interface**, as declarations with full comment blocks in the style of `symbol.h`, `catalogue.h` and
   `rfunc.h` (in the design document, NOT in a header): the name(s); the type of `s` and of the result (`acb_t`
   directly, as `rfunc.h` takes `arb_t`? a complex one-place `adf_sball`? compare, recommend); the place
   argument (`adf_place_t`, prime and real place in one function or two); `p` as `ulong` or `fmpz`, and what a
   non-prime `p` returns (follow `symbol.h` and the text readers: `DOMAIN`); `prec`; every status with the input
   that gives it and `where`; aliasing of `y` and `s`; limits; cost. Whether the reciprocal `1 - p^(-s)` (an
   entire function, no pole status) is offered as a separately named function, and whether a product over a
   finite list of places belongs here or with milestone 5 (Tate integrals). For each decision the alternatives
   and your recommendation with the reason (the orchestrator records it as N-D20 in SPEC 15.4).
3. **The oracle** `proto/zeta_checks.py`, written and RUN. It must not be a second call of the same `acb`
   routines: compute the reference values with `mpmath` at 200 digits or more (if importable; else with
   `python-flint` `arb` used only for `log`, `exp` of real arguments and your own complex assembly, and say
   so) and state the error margin of every comparison. It provides: `expected(place, s_mid, s_rad, prec)`
   returning the status the design promises and, for `OK`, a point value and a rigorous bound on the variation
   of `L` over the ball (from a bound of `L'` on the ball: prove it), so that the implementation lane can check
   "the returned ball contains `L(s)` for sampled `s` in `S`" and "the returned ball is not wider than a stated
   factor of the true image". Run it against the statements of part 1: poles (`k = -3..3` at `p` in 2, 3, 5, 7,
   65537, `2^64 - 59`; `s = 0, -2, ..., -40` at the real place), balls of radius `10^-j` around and near poles,
   the decision rule simulated in `mpmath` interval arithmetic or `python-flint`. Give the counts and what
   would have made a case fail.
4. **The test plan for the implementation lane**: the fixtures to generate from the oracle; the statuses to
   cover; eight faults to plant (for example: the pole test only at `k = 0`; `DOMAIN` returned for a ball of
   positive radius around a pole; the sign of `s` in `p^(-s)`; `Gamma(s)` in place of `Gamma(s/2)`; `pi^(-s)`
   in place of `pi^(-s/2)`; the output written before the status is known; a non-finite ball returned `OK`
   next to a pole; `where` not written); the driver commands (the driver's complex text: see how
   `tools/adf/adf.c` reads a `cadele` and what a complex number looks like in `docs/conventions.md` 9) and the
   Julia call a user would run, with the lines of output you expect.
5. **Open points for TJO**, at most five, each with the alternatives and what you recommend.

## Report

`lanes/d-zeta/report.md`, written once, at the end; running notes in `lanes/d-zeta/progress.md` as you go (a
note after every statement proved, so that an interruption loses little). In the report: the statements proved
(one line each) and the ones you could not prove as the SPEC suggests them, with the counterexample; the
oracle's runs with commands and numbers; the recommended interface in ten lines; findings against the
specification or the proofs (a false sentence of SPEC 9.3.7, Proposition 9 or `analysis.md` 10 is a finding
with its counterexample); sources pending. No praise, no summary of what the files already say.
