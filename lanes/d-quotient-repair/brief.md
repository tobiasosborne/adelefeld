# Lane d-quotient-repair: apply the repairs of review q-review1 to the design of milestone 3

Review q-review1 (`docs/reviews/m3-design/review.md`, the same text as `lanes/q-review1/review.md`; its checks
`lanes/q-review1/review_checks.py`, its mutants `lanes/q-review1/mutate_oracle.py`) judged the design
`docs/api-3.md` and its oracle `proto/quotient3_checks.py`: VALID 14 / MINOR 9 / INVALID 0, "may be implemented
after the repairs". You apply the repairs. Read the review completely first, then the sections of
`docs/api-3.md` it names.

**You own:** `docs/api-3.md`, `proto/quotient3_checks.py`, `lanes/d-quotient-repair/`. Everything else is
read-only (in particular `docs/SPEC.md`, `docs/conventions.md`, `docs/PLAN.md`: what the review says about
them goes into your report under "For the orchestrator", as replacement sentences with file and line, not into
those files). No git command that changes state, no `bd`. At most 2 cores; every program under `timeout`, none
over 170 s. Leave no files in `/tmp` and none outside the paths you own. Another lane (q-slice2) is writing
`src/qclass.c`, `include/adelefeld/qclass.h`, `tests/` in another worktree: not your files.

1. **The text repairs R1 to R10 and R12** (review section "Repairs"): apply each at its place. A repair text is
   a proposal: check it before you paste it (R1: recompute the two cases with exact rationals for a radius just
   below, at and just above a power of two; R3: the identity; R7: read `include/adelefeld/adele.h` for what
   `arch = 0` means and `conventions.md:220`; R8: read the status table of conventions 3.2). If a repair text
   is itself wrong, do not apply it: write the corrected text and say why. Each applied repair gets one line in
   a new last section of `docs/api-3.md`, "Repairs after review q-review1": the tag, the place, what changed.
2. **The decisions** (design section 6), taken by the orchestrator on 2026-10-05, to be written into section 6
   as taken (they become N-D21 in SPEC 15.4; the orchestrator writes that row):
   - D3-1: the three quotient set queries return a status and write `*truth`: `OK` or `LIMIT`, `truth`
     untouched on `LIMIT`; the budget with the preflight of R5; the point comparisons keep their `CMP` codes.
   - D3-2: the construction count is taken before rounding and deduplication, no full-image shortcut; the
     rounding kernel Q1 (30-bit radius rounded up, then its successor); explicit work and bit bounds as section
     1, with the preflight of R5 wherever a count can be huge.
   - D3-3: `_strict` on a class certifies each stored representative; the status may change under an exact
     reduction, and the sentence that says so is added (review, D3-3).
   Rewrite the affected comment blocks of sections 2.2, 2.3, 3.2 so that no declaration still says
   "proposal" or "if D3-x is taken" for these three.
3. **The oracle** `proto/quotient3_checks.py`: the review's two surviving mutants and its three "weakness"
   lines: (a) the Q1 radius rounded to nearest and then to the successor (M4) passes all 15 groups: add cases
   that pin the rounding direction (exact rationals: a radius whose nearest 30-bit value is below it); (b) the
   boundary glue deleted (M7) passes: add the review's witness `[1/2, 1] x (0 mod 3)` against
   `[0, 1/2] x (2 mod 3)` (they overlap only at the glued class) and a family of such pairs to the `sets`
   group; (c) the group `fault_witnesses` notices none of the 15 mutants: say in a comment what it does prove,
   or strengthen it. Then run the review's mutant script against your oracle
   (`timeout 175 python3 lanes/q-review1/mutate_oracle.py`, or a copy in your lane directory if it writes
   beside itself): all 15 mutants must be killed; record the group that notices each. The unmutated oracle:
   `timeout 120 python3 proto/quotient3_checks.py` exits 0 and prints its counts. Also run
   `timeout 170 python3 lanes/q-review1/review_checks.py` unchanged: it must still pass (it does not import
   the oracle; this only shows you broke nothing it reads).
4. Lines at most 116 characters; files end with a newline; the declarations of `docs/api-3.md` still number 40
   plus the `where` arguments of R7 (count them before and after).

Report: `lanes/d-quotient-repair/report.md` (rule 8 of `lanes/COMMON.md`): each repair with applied / changed /
not applied and why; the mutant table before and after; the oracle's counts before and after; "For the
orchestrator": the sentences of SPEC, conventions and PLAN that the review and the design's findings F1 to F5
say should change, each with file, line, the present sentence and the replacement, ready to paste.
