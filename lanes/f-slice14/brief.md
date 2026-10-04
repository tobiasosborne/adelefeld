# Lane f-slice14: the local zeta factors in C (WP 1F.9, last item; the design is `docs/design/local-zeta.md`)

Lane d-zeta (codex `gpt-6.1-sol`) designed and proved the local zeta factor of the trivial character before any
code: `docs/design/local-zeta.md` (Z1 to Z9, the decision rule Z4 with its proof of soundness, the interface of
section 3, the test plan of section 4 with eight faults, the driver and Julia calls) and the oracle
`proto/zeta_checks.py`. Its report is `lanes/d-zeta/report.md`. You implement it. This closes milestone 1F.

Contract: `docs/SPEC.md` 9.3.7 (row "local zeta factor (trivial character)"), 3.1; `docs/conventions.md`
lines 944-956 (the pole rule and the NAMES, CV-60) and the status table (line 223); `docs/PLAN.md` row 1F.9
("local factors at and near their poles: a ball containing a pole returns the pole status, never a finite or
unbounded ball"); `docs/proofs/catalogue.md` Proposition 9; sources `refs/src/tate-poonen/notes.txt:1733`,
`:1014-1016`, `:62-64`. Read these and the design completely before you write a line (CLAUDE.md rules 3, 4),
and `lanes/COMMON-C.md`. Style of header and code: `include/adelefeld/symbol.h`, `catalogue.h`, `rfunc.h` and
their sources; `docs/api-1f9.md` for the statements.

## Decisions already taken by the orchestrator (they become N-D20; do not reopen them)

1. **Name and files.** `docs/conventions.md` CV-60 (line 953) fixes the name: `adf_local_zeta_factor_at`. The
   design proposed `adf_complex_local_zeta_at` without having read CV-60; the convention wins. Header
   `include/adelefeld/localfactor.h`, source `src/localfactor.c` (the gamma and epsilon factors of CV-60 will
   join them with milestone 3; do not declare them now).

       int adf_local_zeta_factor_at(acb_t y, adf_place_t *where, const acb_t s, adf_place_t v, slong prec);

2. **Types**: raw `acb_t` for `s` and `y`; one function, the place handle selects the prime or the real place;
   primes are word primes through the existing handle (design section 3, rows "Input/output", "Dispatch",
   "Prime size").
3. **Statuses** exactly as the comment block of design section 3: `OK`; `DOMAIN` (a non-finite input; the
   exact pole: the exact 0 at a prime, an exact non-positive even integer at the real place); `NOT_DETERMINED`
   (a ball of positive radius meeting a pole, an undecided exclusion, a non-finite result: CV-08); `LIMIT`
   (`prec` above `ADF_REAL_PREC_MAX`, decided first; the Gamma recurrence fallback needing more than 64
   factors). `where = v` on every failure, untouched on `OK`, may be `NULL`; `y` untouched on every failure;
   `y` may be `s`. The orchestrator adds `LIMIT` to the row of `conventions.md:223`; you do not edit that file.
4. **Certificate**: the procedure of Z4 (midpoint exponential with the proved radius at a prime; integer
   geometry, direct Gamma, then the bounded recurrence at the real place), working precision
   `max(2, prec) + 32`. No adaptive subdivision.
5. **Scope**: no reciprocal function, no product over places, no character (milestone 3 and 5).

If a step of Z4 turns out false or unimplementable on FLINT 3.0.1 (the oracle's simulation ran python-flint on
FLINT 3.3.1: the report says which evidence is version-specific), do not paper over it: implement the sound
part, return `NOT_DETERMINED` where the certificate fails, and write the counterexample under "Findings against
the design" in your result. Soundness is the contract: `OK` never for a ball that contains a pole, and on `OK`
the ball contains `L_v(s)` for every `s` of the input. Whether a pole-free ball gets `OK` or `NOT_DETERMINED`
may differ from the simulated fixtures where the evaluator differs between FLINT versions: list such rows, with
the two statuses and the reason, do not force them.

## The slices, each red then green (keep `lanes/f-slice14/redgreen.md`)

A. **The prime place.** Test first (`tests/test_localfactor.c`, registered as the other test programs are):
   exact values (`s = 1, 2, -1` at `p = 2, 3`: `2`, `4/3`, `-1`, ... as exact rationals contained in the
   result and of radius consistent with `prec`); the fixtures (below); the poles `2 pi i k / log p`, `k =
   -3..3`, at `p` in 2, 3, 5, 7, 65537, `2^64 - 59`: a ball around each, of radius `10^-j`, is
   `NOT_DETERMINED`; the exact 0 is `DOMAIN`; a ball at distance `d` with radius `d/16` is `OK` at `prec`
   256 and contains the reference; every status with the state of `y` (compare the representation, not the
   set) and of `where`; `y = s`; `where = NULL`; `prec` 1, 2, the cap and one above; non-finite `s`; a
   composite, 0 and 1 as the prime (through `adf_place_prime`: what does the handle constructor return, and
   can an invalid handle reach the function? follow `symbol.c`); `Re(s) = +-2^1000`, `Im(s) = 2^1000`.
B. **The real place.** Exact values (`s = 2`: `1/pi`; `s = 1`: `1`, since `Gamma(1/2) = sqrt(pi)`; `s = 4`:
   `1/pi^2`; `s = -1`: `pi^(1/2) Gamma(-1/2) = -2 pi`): each contained; the poles `0, -2, ..., -40` exact
   (`DOMAIN`) and in balls (`NOT_DETERMINED`); negative odd integers regular; the direct-Gamma failure of
   the design's findings (`[-2.1, -1.9] + i [1.5, 1.7]`: pole-free, must be `OK` through the recurrence or,
   if FLINT 3.0.1 behaves differently, say what happens); the fallback bound (64 factors, then `LIMIT`).
C. **The fixtures.** Regenerate them: `timeout 170 python3 proto/zeta_checks.py --fixtures
   lanes/f-slice14/zeta-fixtures.jsonl` (11 MB, NOT to be committed). Commit a subset under
   `tests/ref/vectors/f-slice14/` of at most 600 KB chosen by a script in your lane directory (every status,
   every prime, both places, the precision pairs, the extreme arguments; state the selection rule), read in C
   with `tests/support/jsonl.h`. For each row: the status as the design promises (subject to the paragraph
   above), on `OK` containment of every certified sample value (design section 4: how to compare an enclosure
   with a certified point), and the width target (factor 64) recorded separately from soundness.
D. **A user call.** Driver command `local_zeta_factor_at S with PLACE` (`tools/adf/adf.c`, `README.md`), `S`
   the text of a complex adele used as the carrier of the complex number, as design section 4 "Driver
   surface" says (its finite coordinate is ignored: say so in the README); fixtures
   `tests/driver/localfactor-values.cmd`, `localfactor-status.cmd` with expected lines written from the
   mathematics before the run (first line `#!exit N`, see `tests/driver/README.md`); `tests/julia/
   localfactor.jl` from the design's Julia call, registered as the other Julia tests are.
E. **The statements.** Append to `docs/api-1f9.md`: Y16 (what the function returns, by reference to Z1 to Z4,
   with the steps the CODE adds to the design's procedure and why each keeps the enclosure), Y17 (statuses,
   preservation, aliasing, limits, cost), each with a "Check:" line naming the test.
F. **The faults.** Plant the eight faults of design section 4 one at a time in a scratch copy and show that
   `test_localfactor` fails for each (a table: fault, failing assertion). Then mutation testing of
   `src/localfactor.c` as `lanes/COMMON-C.md` rule 5 says (at most 60 mutants, 20 minutes, `--san`).

**You own:** `include/adelefeld/localfactor.h`, `src/localfactor.c`, `tests/test_localfactor.c`,
`tests/ref/vectors/f-slice14/`, `tests/julia/localfactor.jl`, new `tests/driver/localfactor-*`, the new command
in `tools/adf/adf.c` and its section in `tools/adf/README.md`, the appended part of `docs/api-1f9.md`, the
lines of `Makefile`, of the export list and of the Julia runner that register the new files, `lanes/f-slice14/`.
Everything else is read-only, `docs/design/local-zeta.md` and `proto/zeta_checks.py` included (a defect there
is a finding). Another lane (u-dump1) is changing `src/dump.c`, `tools/adf/adf.c` (the `dump` command),
`Makefile` and the export list in another worktree: keep your changes to shared files small and local (one new
command, new lines at the end of lists), so that the merge is mechanical.

## Checks at the end (commands and numbers in the result)

`timeout 1700 make -j2 check-all` once (if it fails on something of yours, repair and rerun the failing part,
and say which parts were rerun); your test program and the driver under `SAN=1`, `INV=1` and `CC=clang`
(`make -j2 BUILD=lanes/f-slice14/build-san SAN=1 lanes/f-slice14/build-san/test_localfactor` and so on): no
warning, no sanitizer report with `ASAN_OPTIONS=detect_leaks=1`. Debug entry checks under `INV` as the other
sources have them (conventions 4.4). Lines at most 116 characters; files end with a newline. Remove your build
directories and the 11 MB fixture file at the end.

Report: `lanes/f-slice14/result.md`, written once, at the end (the harness refuses the name `report.md` for a
Claude subagent); notes in `lanes/f-slice14/progress.md` as you go. In the result: what is done per slice; the
checks with commands and numbers and what would have made each fail; the fault table; mutation survivors with
one line each; fixture rows where the status differs from the simulation; findings against the design or the
specification; what is not done; sources pending. Give the same text as your final message.
