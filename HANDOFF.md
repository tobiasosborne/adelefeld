<!-- ROLE: live state. UPDATE POLICY: every session end, newest entry first. -->

# HANDOFF: adelefeld

## Session 2026-10-07 23:11 to about 07:00 on 2026-10-08 (UTC+8; orchestrator Claude Fable on the laptop; Opus and codex lanes): START HERE

**One line.** Milestone 3 is complete (WP 3.1 and 3.2 reviewed by m3-review1: no blocker, no major; WP 3.3 and
3.4 designed (`docs/api-3c.md`) and built in three slices: `adf_char`, text, dumps, `chi(a)`, Gauss sum, root
number, conjugation, the character on unit cosets, idele classes and ideles), and milestone 4 is complete
through all seven slices of its design (`docs/api-4.md`: `adf_ffun`, `adf_rfun`, their transforms, evaluation,
integrals, Poisson summation with certified tails, dump forms); check-all passes on `b4922ae` and the driver
after the last merge has 95 cases. Eighteen lanes landed in one night (`docs/worklog/2026-10-07-night.md`).
The design of milestone 5 landed too (d-tate: `docs/api-5.md`, decisions taken as N-D24). The review of milestone 4 landed too (m4-review1: no blocker; one major against the wording of statement F4;
three minors, one repaired at once, two as beads adf-2w1, adf-l4v; `docs/reviews/m4/review-m4.md`). Everything named here is merged and pushed.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    tools/orch/suites.sh -j 2          # all, san, clang, inv, headers; NOTE "-j 2" with a space
    ~/Projects/quota-app/target/release/quota; date

**Master.** check-all passed on `b4922ae` at 05:24 (100 test programs, driver 93 cases with 101613 expected
lines, exports 609 of 609, Julia, both self-tests); san, clang, inv (99 programs each) and headers passed on
`220c8a3` at 05:10. After them two more merges landed (c-slice3 `9a54305`, f4-slice7 `bb17591`) with their tests
and the driver (95 cases) green; the five suites were not rerun on `bb17591`.

**Rules of TJO for this session** (23:11 and 23:20; memory `orchestration-model-tiers`, top block): work at pace
for Claude and Codex; no Fable subagents; Opus for coding, Sonnet for busywork, codex `gpt-6.1-sol` xhigh,
`gpt-6-astra` for hard cognition; 2 to 3 subagents; "serious progress on the major milestones"; NO per-slice
reviews, ONE code review when a major feature lands. Done so: one review of milestone 3 WP 3.1 and 3.2
(m3-review1), one of milestone 4 (m4-review1, running). Decisions taken by the orchestrator as the designs
recommend: N-D22 (`adf_char`: the modulus cap 65536, status row 223), N-D23 (milestone 4: the caps, paired
factor calls without a tensor type, the partial-place evaluator; plus the stall rule of P1 step 4 from lane
f4-slice6). Quota at 05:40: Claude weekly 16.0%, 1.7 under pace; Codex 10.0%, 0.3 AHEAD (no new codex lane
after that); Fable weekly 20.0%, 2.3 ahead.

**What landed**: the table in `docs/worklog/2026-10-07-night.md` (18 lanes with minutes and counts). In short:
milestone 3 slices 3.1-d, 3.1-e, 3.1-f, 3.2-b, 3.2-c; the character design and slices a, b, c; the repair of the
dump validator (adf-3n2); the review of milestone 3 with its three minors repaired; the milestone 4 design and
slices 4a to 4g. The statements are in `docs/api-3a.md`, `3b.md`, `3d.md`, `4a.md`, `4b.md`, `4c.md`; the review
record in `docs/reviews/m3/review-m3-wp31-wp32.md`.

**The next steps**, in this order:
1. The two open minors of the milestone 4 review: adf-2w1 (the dump validator converts long hex tokens before the
   grammar stage ends; a small repair lane) and adf-l4v (the running time of `adf_tensor_poisson` near the bits cap;
   a decision); the wording of F4 and the dump/load identity in `docs/api-4.md` (adf-cx8).
2. Milestone 5: `docs/api-5.md` (lane d-tate, `lanes/d-tate/report.md`; decisions D1 to D4 taken as N-D24): start
   its first slice (section 9: the local integrals, gamma and epsilon factors extending `localfactor.h`) on
   codex or Opus; the remaining slices in order; one review when milestone 5 lands. Codex should be under pace
   again by the morning.
3. Rerun the five suites on the current master before the first new merge.
4. The two HEADER-FINDINGs of f4-slice7 against `docs/api-4.md` section 2 (the dump/load identity needs the D1
   caps; load's cost includes clearing the old storage): the header comments state the right contract; the
   design text is unchanged (a design is not edited by lanes); decide whether to amend `docs/api-4.md` or let
   `docs/api-4a.md`/`4b.md` stand as the record.
5. Sources pending accumulated by the character slices: FLINT's `dirichlet.rst` (the Conrey pairing and the
   group-init failure semantics) is not under `refs/`; fetching it would close the conditional proof of the
   conjugate label (P3 of `docs/api-3c.md`) and the predicate's setup-failure finding.
6. The list left from before (previous entry, item 5 and the earlier items).

**Waits for TJO.** Nothing blocks.
- NEW: `docs/api-4.md` section 2 promises an unconditional dump/load identity for `adf_ffun`; with the D1 caps
  it holds only when the caps admit the dumped representation (f4-slice7). Whether the design text is amended.
- NEW: the stall rule of `adf_tensor_poisson` (f4-slice6): a precision doubling from 64 bits that does not halve
  the largest radius stops with `NOT_DETERMINED`; taken by the orchestrator; open to reversal.
- NEW: `acb_inv` loses the dependency for a wide canonical `A` in `adf_rfun_fourier`, so such a transform is
  `NOT_DETERMINED` at every precision (f4-slice3); an exact interval inverse would certify it. A cost item.
- NEW: the predicate `adf_char_is_canonical` fail-stops if FLINT's group setup returns 0 (no status in its
  `int` ABI); whether such a failure is reachable for a word-sized modulus is unknown (c-slice1, c-slice2).
- The earlier items (N-D21's annotation at SPEC:944 and api-3.md:523-524 wording, adf-s1v, adf-nyk; the piece
  limit's memory; a lawful copy of Tate's thesis; the older UX and printer questions) stand.

**Things to know.**
- Keep-both merge resolution of appended blocks is right for `README.md`, `test_julia.sh`, `docs/api-*.md`,
  and wrong twice tonight: `src/text.c` (rebuild as head plus the lane's appended block, after checking with
  `cmp` that both are pure appends of the common base) and `tools/adf/adf.c` (a `break` lost; an enum-order
  dependency in `adf_drv_body_slot`, now an explicit table). Run the driver suite after every merge.
- The memcheck self-test of check-all requires the abort children of `INV` tests to clear their objects after
  the call ("Reached only if the entry check is missing"); every new lane's test needed this fix. Put it in the
  next briefs.
- Opus lanes poll their own mutation runs and produce many interim notifications; the briefs now say "wait
  with a single blocking command".
- `tools/orch/suites.sh` writes its logs to `build/suite-logs/`; `pgrep -f suites.sh` matches the caller's own
  shell, so start `make check-all` directly when in doubt.

## Session 2026-10-07 09:30 to about 10:50 (UTC+8; orchestrator Claude Fable on the laptop; Opus and codex lanes): START HERE

**One line.** The two lanes stopped on 2026-10-05 are landed: the review of `adf_qclass_reduce` (q-review3,
now by Opus: no code defect, two test gaps, both closed by q-repair1) and the additive character on adeles
(q-slice3, codex sol: `psi.h`, `src/psi.c`, driver `psi`/`psi_strict`, Julia, `docs/api-3b.md`, `test_psi`
186799 checks; NOT reviewed). The pi agent (`space-bunny-alpha`) is no longer available: lanes run on codex
`gpt-6.1-sol` xhigh and Claude Opus only. Everything named here is merged and pushed.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    tools/orch/suites.sh -j 2          # all, san, clang, inv, headers; NOTE "-j 2" with a space
    ~/Projects/quota-app/target/release/quota; date

**Master.** All five suites pass on `9593815` (the merge of q-slice3): `tools/orch/suites.sh -j 2` from 10:24 to 10:31 on 2026-10-07, in parallel: check-all (85 test programs, driver 72 cases with 101348 expected lines, exports 505 of 505, Julia, both self-tests), `san` (leak detection on, 85 programs), `clang`, `inv`, `headers`. check-all alone had also passed on `44b5476` (after the repair merge, before q-slice3).

**Rules of TJO for this session** (2026-10-07 09:30): "orchestrate a small step (space bunny no longer
available), use codex exec gpt sol 6.1 xhigh and opus". Codex lanes through `tools/orch/wt_lane.sh <lane> codex
gpt-6.1-sol xhigh`; Opus lanes through the Agent tool (model opus) with the brief pasted into the prompt and a
worktree made by `git worktree add` from master (memory `claude-worktree-base`). Reviews of codex code go to
Opus. The rest of the 2026-10-04 rules stand (memory `orchestration-model-tiers`). Quota at 10:08: Claude
weekly 2.0%, 4.0 under pace; Codex weekly 29.0%, 25.5 under.

**What landed** (brief `lanes/<lane>/brief.md`; report `result.md` for Opus lanes, `report.md` for codex;
times in `docs/worklog/2026-10-07-morning.md`).

| Lane | Model | What | Review |
|---|---|---|---|
| q-review3 | Opus | review of `adf_qclass_reduce` with an own exact-containment oracle (coset refinement, not algorithm R): 20000 lifts, 4500 PIECES inputs, 988 re-reductions, 4 million points, 0 findings; printer byte-equal on 2000 values; 37 resource cases; sanitizers 5028 cases; driver 60 lines by hand; 13 faults | `docs/reviews/m3/review-qclass-reduce.md`: no code defect; R1 MAJOR, R2 MINOR test gaps |
| q-repair1 | Opus | tests for R1 (upper end an integer plus `2^-200`; 61 new vectors `tests/ref/vectors/q-repair1/`) and R2 (`LIMIT` in the second pass at `ADF_REAL_PREC_MAX`, `y` untouched); `test_qclass_reduce` 223472 checks; faults F2 and F8 now fail | the review's faults are the red runs |
| q-slice3 | codex sol xhigh (48 min) | slice 3.2-a: `adf_fball_psi_tate_phase`, `adf_adele_psi_tate_phase`, `adf_phase_get_acb`, `adf_adele_psi_tate`, `_strict`; sign from conventions 6.1 and `refs/src/tate-poonen/notes.txt:693-700`; `docs/api-3b.md`; 11 faults rejected; mutation 60 of 284: 46 killed, 7 equivalent, 7 not compiled | NOT reviewed: bead adf-e57 |

**The next steps**, in this order:
1. Review q-slice3 by Opus (adf-e57), own oracle for the phase and the hull bound of design 3.3; leak
   detection (the codex lane could not run LeakSanitizer); a repair lane for what it finds.
2. Milestone 3, the next slices of `docs/api-3.md` section 7: 3.1-d (raw pieces, the union text, dump), 3.1-e
   (the three set queries with the preflight of N-D21), 3.2-b (the class calls), 3.2-c (the local calls). One
   codex or Opus lane per slice; a review by the other family after two slices.
3. Reviews still owed from before: the change of `src/localfactor.c` by f-repair9 (a referee of Y16 steps 1
   to 7); adf-c9b.
4. TJO's answers (below), then the list left from before (unchanged from the previous entry, item 5).

**Waits for TJO.** Nothing blocks.
- NEW: SPEC:944 (the annotation under N-D21) says `0 mod 1/2` is not inside `A_f`; lane q-slice3 shows by the
  restricted product that `(1/2) Zhat` IS in `A_f` (integral at every prime but 2). Which reading the
  library implements is unchanged (the character follows SPEC 6); the annotation and the earlier question
  below should be reworded (bead adf-s1v).
- NEW: `docs/api-3.md:523-524` assumes `l <= m <= h` for the rounded midpoint; false at `l = h = 1/3`, prec
  2; the conclusions hold, argument in `docs/api-3b.md` (bead adf-nyk; wording only).
- NEW: the memory of `adf_qclass_reduce` follows the raw count before deduplication, so a `piece_limit` of
  `10^9` admits 96 GB and the process dies in `flint_malloc` without a status (review q-review3, note N2;
  documented in the header as `O(K b)`). Whether a byte bound on top of the piece limit is wanted.
- NEW: a lawful readable copy of Tate's thesis is wanted under `refs/` (q-slice3 source pending; the signs
  used come from the Poonen notes).
- The earlier items (fractional radius reading, N-D20, N-D21, the dump loader against the text reader, the
  driver's line printer for partial balls, Gelfond-Schneider, the UX questions) stand as in the previous entry.

**Things to know.**
- The pi agent is gone. `tools/orch/pi_lane.sh` and its worktree runner branch are dead code until a
  replacement appears.
- Opus lanes write `lanes/<lane>/result.md` (a Claude subagent cannot write `report.md`); the orchestrator
  commits in the worktree, merges, records reviews under `docs/reviews/`. Costs this session: an Opus review
  292k tokens (32 min), an Opus test repair 136k tokens (6 min).
- A resumed codex lane with no saved session starts afresh on the WIP files; a "Resume note" appended to the
  brief in the worktree naming the files worked (q-slice3 kept the inherited code unchanged after checking
  it, and only strengthened tests and wrote the statements).
- The codex lane ran `make check-all` in its worktree although the brief did not ask for it (262 s); it also
  passed. Opus lanes obeyed "do not run the suites as a whole".

## Session 2026-10-04 22:51 to 2026-10-05 05:55 (CEST; orchestrator Claude Fable on the laptop; Opus, Sonnet, codex and pi lanes)

**One line.** Every work package of milestone 1F has code and a review: the local zeta factor landed (design
d-zeta, code f-slice14, N-D20; review f-review15: no defect; test gaps repaired by f-repair9), the catalogue is
bug-hunted (f-review12: no finding) and refereed (f-review14: one test gap, repaired), `Log` on ideles is
refereed (f-review13) and repaired (f-repair8: eight test gaps, one cost defect). The driver holds local balls
and partial balls (drv-ball); the five newer types have dump forms (u-dump1; review u-review1 found two
BLOCKERS in the loader of the idele and the class, repaired by u-repair1). Milestone 3 has begun: design
`docs/api-3.md` (d-quotient; reviewed q-review1; repaired; N-D21), `adf_qclass` in lift form (q-slice1; review
q-review2: no blocker) and its reduction `adf_qclass_reduce` (q-slice2, not reviewed). Everything named here is
merged and pushed. Two lanes were STOPPED by the harness at about 05:50 (low memory) and are not merged: see
"Stopped" below.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    tools/orch/suites.sh -j 2          # all, san, clang, inv, headers; NOTE "-j 2" with a space
    ~/Projects/quota-app/target/release/quota; date

**Master.** All five suites pass on `2cb9e57` (the merge of q-slice2; only briefs and records came after it):
84 test programs, driver 70 cases, exports 500 of 500, Julia, both self-tests; `san` with leak detection; run
from 05:33 to 05:45 on 2026-10-05, one suite after the other.

**Rules of TJO for this session** (2026-10-04 22:55; memory `orchestration-model-tiers`, top block): work up to
5 points OVER pace on Claude and Codex; no Fable subagents; Opus for coding, Sonnet for busywork and research;
codex `gpt-6.1-sol` xhigh; `gpt-6-astra` allowed for hard cognition; 2 to 3 subagents at a time; the pi agent
with `space-bunny-alpha`. The other Claude session (initech-os) had the same rule; the two agreed shares by
cross-session messages (worklog). Quota at 05:33: Claude weekly 79.0%, 0.7 over pace; Codex weekly 23.0%, 3.8
under (they were 71.0% and 9.0% at 22:51: both sessions).

**What landed** (brief `lanes/<lane>/brief.md`; report `result.md` for Claude lanes, `report.md` for codex and
pi lanes; times and counts in `docs/worklog/2026-10-04-evening.md`).

| Lane | Model | What | Review |
|---|---|---|---|
| f-review12 | Sonnet | bug hunt through `src/catalogue.c`, own oracle | no finding (`docs/reviews/f1/review-catalogue.md`) |
| drv-ball | pi | driver: `lball`, `sball` as value kinds; a stored partial ball as operand of the commands at places | u-review1: no finding in this part |
| d-zeta | codex sol | `docs/design/local-zeta.md` (Z1 to Z9), `proto/zeta_checks.py` | implemented without rework; three findings of the code lane against it |
| f-review13 | Opus | referee of IL1 to IL8, G7 to G12; 37 planted faults | 8 faults passed the tests, 1 cost defect: f-repair8 |
| f-repair8 | codex sol | tests for the eight; a refused refine request decides from the working exponent alone (170 s to 0.4 ms) | 34 mutants, 0 survivors; old against new 26000 requests, 0 differences |
| u-dump1 | pi | dump forms of `ucoset`, `idele`, `idclass`, `lball`, `sball` (20 functions), driver `dump`, `load` | u-review1: 2 BLOCKERS: u-repair1 |
| f-slice14 | Opus | `adf_local_zeta_factor_at` (`localfactor.h`, `src/localfactor.c`), driver, Julia, Y16, Y17 | f-review15 |
| d-quotient | codex astra | `docs/api-3.md` (40 declarations, Q1 to Q5), `proto/quotient3_checks.py` | q-review1 |
| f-review14 | pi | referee of Y1 to Y15; 40 planted faults | 1 fault passed: check added by the orchestrator. Weaker evidence than f-review13 |
| f-review15 | codex sol | review of the zeta code, own oracle (mpmath, 608580 samples); 13 faults | no defect; 5 faults passed: f-repair9 |
| q-slice1 | codex sol | `adf_qclass`, lift only: `qclass.h`, `src/qclass.c`, text, driver `qadd_rat`, Julia, `docs/api-3a.md` | q-review2 (pi): no blocker, 2 MINOR (`docs/reviews/m3/review-qclass-lift.md`) |
| q-slice2 | codex sol (3 hours) | `adf_qclass_reduce` (slices 3.1-b, 3.1-c: algorithm R, the kernel Q1, the piece limit), the union text printed, driver `qreduce`, Julia; `test_qclass_reduce` 150122 checks | NOT reviewed (q-review3 was stopped) |
| f-repair9 | codex sol | three test families; the test no longer crashes under faults; the derivative bound without `log(pi)` (proof in Y16) | the proof read by the orchestrator only; F5 open (adf-c9b) |
| q-review1 | pi | review of `docs/api-3.md`, own oracle, 15 mutants of the author's oracle | VALID 14, MINOR 9, INVALID 0 |
| u-review1 | Sonnet | review of the dump forms and the driver ball kinds, about 900000 variant texts | 2 BLOCKERS, 1 MAJOR test gap, 2 MINOR: u-repair1 |
| d-quotient-repair | pi | repairs R1 to R12 in `docs/api-3.md`; oracle pins the rounding direction and the boundary glue | two of the review's own repair texts corrected |
| u-repair1 | pi | `dp_arb_sign` repaired; no abort reachable from a text; 3.1 million dumps decided three ways | the review's differential check: 0 mismatches |

**Stopped, not merged.** At about 05:50 the harness stopped both running lanes because the machine was
critically low on memory (the session was idle; at 05:53 `free` showed 23 GB available again, swap of 1 GB
full; the other session's certificate with emulators was running). They were NOT started again: the harness
says to restart only when asked.
- Lane q-review3 (pi; the review of `adf_qclass_reduce`; brief `lanes/q-review3/brief.md`): 20 minutes in; an
  exact model, a harness and nine probes exist in `../adelefeld-wt/q-review3/lanes/q-review3/`, no report. To
  continue: `RESUME=1 LANE_TIMEOUT=7200 tools/orch/wt_lane.sh q-review3 pi openrouter/stealth/space-bunny-alpha
  high` (the runner continues the last pi session), or start it afresh.
- Lane q-slice3 (codex sol; the additive character on adeles, slice 3.2-a; brief `lanes/q-slice3/brief.md`,
  bead adf-ewi): 20 minutes in; `include/adelefeld/psi.h`, `src/psi.c`, driver changes and a vector generator
  exist in `../adelefeld-wt/q-slice3/`, no test file yet, no report. To continue:
  `tools/orch/wt_lane.sh q-slice3 codex gpt-6.1-sol xhigh` (the runner resumes the codex session when
  `lanes/q-slice3/session.id` is not empty; else it starts afresh on the files that exist).

**The next steps**, in this order:
1. Restart the two stopped lanes (above) when the machine has memory: `free -g` first; land them (commit in
   the worktree, merge, five suites, push); a repair lane for what q-review3 finds.
2. Milestone 3, the next slices of `docs/api-3.md` section 7 after 3.2-a: 3.1-d (raw pieces, the union text, dump), 3.1-e (the three set
   queries with the preflight of N-D21), 3.2-b, 3.2-c. One codex or Opus lane per slice; a review by the other
   family after two slices.
3. Reviews still owed: q-slice2 (q-review3, stopped); the change of `src/localfactor.c` by f-repair9 (the bound
   without `log(pi)`: a referee of Y16 steps 1 to 7); adf-c9b (is the factor `M0` of that bound needed).
4. TJO's answers (below).
5. Left from before, unchanged: the per-place variant of SPEC 9.3.1; `x^(e/n)` of an exact rational at all
   places; a public function for the local component of an idele; `adf_real_root` at `prec` 2; the bounds
   marked source pending in `docs/api-1f8.md`; adf-7yz, adf-4dj, adf-mds, adf-7gc; stale entries of
   `tools/mutate/equivalent.txt`; the merged worktrees under `../adelefeld-wt/` and `.claude/worktrees/` (now
   about 135) may be removed (deleting is TJO's call).

**Waits for TJO.** Nothing blocks.
- A finite ball with a fractional radius (`0 mod 1/2`) is not a subset of the finite adeles `A_f`, yet SPEC 6
  gives it a meaning in the quotient. Which reading the library implements should be stated (review q-review1,
  last finding; recorded under N-D21).
- Open to reversal: N-D20 (the local zeta factor: the header name, the bound of 64 recurrence factors, the
  complex adele as the driver's carrier of `s`), N-D21 (D3-1 to D3-3 of the quotient design), the earlier ones.
- The dump loader accepts exponents of a local ball above `ADF_LBALL_EXP_MAX` when the caller raises
  `max_prec`, because the canonical predicate has no such bound; the text reader returns `LIMIT` there
  (u-repair1, finding 1). One of the two should change.
- The driver keeps its own line printer for partial balls (`real: r; 5: c + O(5^N)`); the library prints
  `{inf: r; p=5: c + O(5^N)}` (122 fixture lines differ). Switching changes every such fixture.
- Sources pending: Gelfond-Schneider (the statement that no exact input other than 0 is a pole of the finite
  zeta factor; the code does not rely on it); the UX questions of `docs/ux/README.md` as before.

**Things to know.**
- The codex provider REFUSED the brief of lane u-review1 four times ("flagged for possible cybersecurity
  risk": a review of text loaders with malformed input and memory checks). The brief was not reworded; the
  review went to Sonnet. Briefs of that kind go to a Claude or pi lane.
- The pi lane did well as coder and repairer (five lanes, one attempt each, no empty response, two at once
  worked) and its code had real defects that only a review found (u-dump1). As a REFEREE of proofs it is
  untested: its "no false step" is recorded as weaker evidence. It corrected two repair texts of its own
  review before applying them.
- Lane directories can carry large logs: lane f-repair9 committed 12 MB of mutant logs. Look at
  `git diff --cached --stat` and the size before the commit in the worktree (this session rebuilt the local
  history before the push).
- A lane that changes what a kind of the value form answers breaks old driver fixtures by design (drv-ball,
  u-dump1, q-slice1 each did): the lane lists the lines, the orchestrator edits them. The kind without a typed
  parser that those fixtures use is now `rfun()`.
- While the other session runs a full certificate (load-sensitive), run the suites one after the other:
  `suites.sh -j 2 all`, then `san`, then `clang inv headers` (17 minutes instead of 7).
- An Opus lane cost about 1 point of Claude weekly (307k and 426k tokens), a Sonnet review 0.3 (156k, 270k);
  a codex sol xhigh lane about 0.45 points of Codex weekly; the astra design lane (high, 25 minutes) was not
  measured alone.

## Session 2026-10-03 22:37 to 2026-10-04 03:35 (CEST; orchestrator Claude Fable on the laptop; Opus, codex sol, pi and Sonnet lanes)

**One line.** The cloud session branch is merged; milestone 1F is complete through 1F.8: the root at all places
and the five series on a finite part exactly 0 (f-slice10, N-D16; reviewed f-review9, repaired f-repair7) and
`Log` on ideles (design d-idlog, code f-slice11, N-D17; bug hunt f-review10: no finding); 1F.9 has its first
two groups (the quadratic symbols and the Hilbert symbol: f-slice12, N-D18; bug hunt f-review11: no finding;
binomials, the profinite power, the cyclotomic action: f-slice13, N-D19, not reviewed), so that of milestone 1F
only the local zeta factors are left;
the powers at a prime are reviewed to the end and repaired (f-review8, f-repair5, f-repair6); the value form of
`adf_lball` and `adf_sball` exists (t-slice2); the mutation tool is repaired (m-tool2); four design briefs for
the interaction plane are in `docs/ux/`. Everything named here is merged and pushed.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    tools/orch/suites.sh -j 2          # all, san, clang, inv, headers; NOTE "-j 2" with a space
    ~/Projects/quota-app/target/release/quota; date

**Master.** All five suites pass on `e63593a` (the merge of f-slice13; only records came after it):
`tools/orch/suites.sh -j 2` from 03:26 to 03:31 on 2026-10-04: check-all (79 test programs, driver 61 cases,
exports 462 of 462, Julia, both self-tests), `SAN=1` (79, leak detection on), `CC=clang` (79), `INV=1` (79),
`check_headers`.

**Rules of TJO for this session** (2026-10-03 22:40; memory `orchestration-model-tiers`, top block): orchestrate;
stay UNDER pace on Claude and Codex (quota app); no Fable and no `gpt-6-astra` subagents; Opus for coding, Sonnet
for busywork, research and code queries; codex `gpt-6.1-sol` xhigh for review and bug hunting (and computer
use); the pi agent with `stealth/space-bunny-alpha` is free, does not count against the lane budget and is
launched only through `tools/orch/pi_lane.sh`; at most 4 lanes; synchronise quota and machine load with the
other Claude session on the laptop (initech-os) by cross-session messages. Agreed with that session: about 5
points of Claude weekly for this one on 2026-10-03, stop of new Claude lanes below 4 points under pace. At 23:40
the two sessions together burnt about 5 points an hour, so no Claude subagent was started between 22:55 and
01:27; the work went to codex and the pi lane. Quota at 01:27: Claude weekly 56.0%, 5.6 under pace; Codex weekly
3.0%, 7.0 under. At 03:32: Claude weekly 59.0%, 3.8 under pace (the stop); Codex weekly 5.0%, 6.3 under (nine
codex sol xhigh lanes cost 4 points of Codex weekly). Two Sonnet hunts ran after 01:27, above the stop.

**What landed** (brief `lanes/<lane>/brief.md`; report `lanes/<lane>/result.md` for Claude lanes, `report.md` for
codex and pi lanes; details `docs/worklog/2026-10-03-evening.md`).

| Lane | Model | What | Review |
|---|---|---|---|
| (merge) | | `ccr-f62bc633-f7fnqs` fast-forwarded into master: f-review6, f-repair3, f-repair4, f-slice9, f-review7 (see the two entries below) | |
| f-repair5 | Opus | minors of f-review7: `lpow.h` cost sentence; an exact integer exponent of `powunit` goes through `pow_si` (P9: only a `LIMIT` can become `OK`); `early_status` decides every `LIMIT` before the listing (R6 step 6); one Teichmueller lift per branch list (R9; `d = 299756`: 44.6 s to 1.3 s) | not reviewed again (old against new: 0 differences in 186107 branches) |
| f-review8 | codex sol | referee of P1 to P8 and R8, tests that cannot fail, cost: `docs/reviews/f1/review-lpow.md` (with f-review7): 0 blocker; 3 MAJOR test gaps; 7 MINOR; 0 of 36 cost ratios above 1.5 | repaired by f-repair6 |
| f-repair6 | codex sol | f-review8 F1 to F9: 186 fixture rows in exact integers at 65537 and `2^64 - 59`; every branch of the six large root lists checked (106850 centres); a CPU guard on the early `LIMIT`; sentences of P1, P3, P7, P8, R8; `src/` unchanged | all 16 faults of the review fail a test program now |
| f-slice10 | Opus | 1F.8 parts 1 and 2: `gfunc.h`: `adf_rat_root`, `adf_adele_root`, `adf_idele_root`, `adf_adele_exp`, `sin`, `sinh`, `cos`, `cosh`; driver `root X with N [with SIGN]`, `exp X` ...; G1 to G6 in `docs/api-1f8.md`; N-D16 | f-review9 |
| f-review9 | codex sol | review of f-slice10: `docs/reviews/f1/review-gfunc.md`: 0 blocker; 1 MAJOR test gap (an inexact idele of degree 4 or more returned `OK` passed the tests); 3 MINOR | repaired by f-repair7 |
| f-repair7 | pi bunny | f-review9 F1 to F4: the two test gaps; debug invariant entry checks in `gfunc.c` (release code byte-identical); G1 step (`|r| >= 2`); two word-size bounds stated with limits and marked source pending | all 13 faults of the review fail `test_gfunc` now |
| d-idlog | codex sol | design before code of `Log` on ideles: `docs/design/idele-log.md` (IL1 to IL8, six declarations, test plan), oracle `proto/idlog_checks.py` | not reviewed (the review f-review10 checks the code against its own oracle) |
| f-slice11 | codex sol | 1F.8 part 3: `src/gfunc_log.c`: `adf_idele_Log`, `log_abs`, `Log_at`, `log_abs_at`, `Log_refine`, `log_abs_refine`; driver commands of the same names; `gfunc_log.jl`; G7 to G12; N-D17; `test_gfunc_log` 2194049 checks | f-review10 (Sonnet): no finding |
| f-slice12 | codex sol | 1F.9 first slices: `symbol.h`, `src/symbol.c`: Legendre, Jacobi, Kronecker on integers, finite balls, unit cosets; the Hilbert symbol at a place on local balls, real balls, rationals, ideles; driver `legendre`, `jacobi`, `kronecker`, `hilbert_at`; Y1 to Y8 in `docs/api-1f9.md`; N-D18; `test_symbol` 719322 checks | f-review11 (Sonnet): no finding |
| f-slice13 | codex sol | 1F.9 second group: `catalogue.h`, `src/catalogue.c`: `adf_fball_binom`, `binom_tight`; `adf_ucoset_profpow`, `profpow_coarse`, `profpow_fine`; `adf_idclass_cyclo_exp_u`, `cyclo_exp_uinv`; Haar volume and content reuse existing functions; driver `binom`, `binomtight`, `profpow`, `profpowcoarse`, `profpowfine`, `haar_volume`, `cyclo_exp_u`, `cyclo_exp_uinv`; Y9 to Y15; N-D19; `test_catalogue` 131636 checks | NOT reviewed (two PLAN vectors recomputed by the orchestrator: radius 2 for `(0, 8, 4)`; `[49 mod 120]`) |
| t-slice2 | pi bunny | value form of `adf_lball`, `adf_sball` (`adf_lball_set_str`, `get_str`, `adf_sball_set_str`, `get_str`); `test_text_local` 68868 checks | none. The lane's file began with a 1008-line copy of `src/text.c`; the orchestrator joined its code to `src/text.c` (`lanes/t-slice2/orchestrator-note.md`) and later repaired two debug entry checks that did not compile under `INV=1` and two missing final newlines |
| m-tool2 | pi bunny | mutation tool: `--san` asks for the make variable `SAN`; a run with more than half of the mutants not compiled fails (exit 2); no `swap_args` mutant of equal arguments | self-test rerun by the orchestrator |
| UX | Opus | `docs/ux/`: four design briefs (adelica notebook, place atlas, claim ledger, sweep bench), a static HTML mock each, README with three questions for TJO | TJO reads |

Also: `tools/orch/suites.sh`: the `all` suite builds in `build/` (a `BUILD` on the command line reached
`tools/adf/Makefile` and the golden test ran a stale `build/adf`); N-D15 wording (f-review8 F10); SPEC 9.3.1,
9.3.3 and conventions 3.3 rule 4 follow the findings of f-slice10; PLAN rows 1F.6 to 1F.8.

**Nothing is running.** The session stopped dispatching at 03:32: Claude weekly 59.0%, 3.8 points under pace,
below the stop of 4 agreed with the other session (Codex weekly 5.0%, 6.3 under).

**Landed last.** Lane f-review11 (Sonnet; bug hunt through `src/symbol.c`, own oracle): NO finding
(`docs/reviews/f1/review-symbol.md`). Lane f-review10 (Sonnet medium; bug hunt through `src/gfunc_log.c` with its own oracle)
landed at 01:38: NO finding (`docs/reviews/f1/review-gfunc-log.md`: 1060 local images compared as sets, 700 CRT
refinements, statuses, two reproducers under ASan/UBSan with leak detection, 52 hostile driver lines). It did
not referee the proofs IL1 to IL8 and G7 to G12: nobody has.

**The next steps**, in this order:
1. A review of f-slice13 (`catalogue.h`) by a model that is not codex, with its own oracle (the pattern is
   `lanes/f-review11/brief.md`; a Sonnet hunt took 11 to 18 minutes and about 160k tokens). Then a referee of the
   proofs of `Log` on ideles (IL1 to IL8, G7 to G12) and of the catalogue (Y1 to Y15) by a model that is not
   codex, with faults planted against `test_gfunc_log`, `test_symbol`, `test_catalogue`: the two Sonnet hunts
   tested values, not proofs or tests. Both are in beads.
2. The last item of 1F.9: local zeta factors (complex balls; a ball containing a pole returns the pole status,
   never a finite or unbounded ball: PLAN line 279, SPEC 9.3.7). A design note first (complex balls are not
   used by any function at places yet: `rfunc.h` has real balls only), then the code: the pattern of d-idlog
   then f-slice11 (27 minutes of design, 63 minutes of code, no rework). Then milestone 1F is complete.
3. The driver reads and prints `adf_lball`, `adf_sball` through the library (`adf_drv_put_sball` is the driver's
   own code); dump forms of the three types of milestone 2 and of `adf_lball`, `adf_sball`. Then milestone 3.
4. TJO's answers on the UX briefs (`docs/ux/README.md`, three questions); the demands on the core that all four
   briefs share are listed there for SPEC section 10.
5. Left: the per-place variant of SPEC 9.3.1; `x^(e/n)` of an exact rational at all places; a public function
   for the local component of an idele (N-D17: needs a type for multiplicative local cosets); three stale entries
   of `tools/mutate/equivalent.txt` (lines 88, 112, 157; `check_equivalent.py` is not in check-all); the tool's
   `--timeout` also bounds the baseline and a run stopped by its bound loses the killed count; `adf_real_root`
   at `prec` 2 returns a ball containing 0 for a positive input (f-slice10 finding 3); two bounds marked source
   pending in `docs/api-1f8.md` (G1(c), G6(d)); adf-7yz, adf-4dj, f-review4 MINOR 2, adf-mds, adf-7gc; about 110
   merged worktrees under `../adelefeld-wt/` and `.claude/worktrees/` and eleven branches with only "WIP
   autosave" commits of 2026-09-29 may be removed (not done: deleting is TJO's call).

**Waits for TJO.** Nothing blocks. Open to reversal: N-D16 (an idele with an exact unit follows the rational root
contract, where SPEC 9.3.3 said `NOT_DETERMINED` for every idele; `DOMAIN` without a place when only the finite
part fails), N-D17 (the conservative ball `0 + 4 Zhat` also for exact inputs; no public local-component function
yet), N-D18 (the conservative certificate for symbols of finite-precision inputs), N-D19 (strict as the default
of the profinite power; the caps on `k` and `g`), N-D12 to N-D15 as before. The UX questions.

**Things to know.**
- `suites.sh` takes `-j 2`, not `-j2`; a pipe to `tail` hides its exit code. Run ALL FIVE suites after merging
  a lane that did not run them itself: this session merged t-slice2 after `all` and `san` only, and master did
  not build under `INV=1` or clang for an hour (found by lane f-slice11 and by the next full run).
- A Claude subagent with `isolation: "worktree"` starts from the PUSHED commit: push the brief first. After
  inspecting such a worktree with `cd`, the harness may treat it as the working directory: use `git -C`.
- The pi lane behaved: three lanes, one attempt each, no empty responses (21, 56 and 34 minutes). Its weakness
  was a design shortcut (the copied machinery) and small portability slips (no final newline, a `#x` outside a
  macro in debug-only code), not flakiness: say in a brief what to do when a needed function is static in a
  read-only file, and ask for `INV=1` and `CC=clang` builds of the files it writes. It leaves files in `/tmp`.
- Codex sol xhigh lanes took 27 to 63 minutes each; three of them failed their one `check-all` on something
  small of their own (a memcheck self-test, fixture exit metadata) and did not rerun it: the orchestrator's
  five suites after the merge are the acceptance. The codex sandbox cannot run LeakSanitizer (ptrace): the
  orchestrator's `san` suite is the leak check. An Opus lane cost about 1 point of Claude weekly.
- The driver's adele text takes a decimal real part: `exp (0.5 ; 0)`, not `(1/2 ; 0)` (`PARSE`).
- Not committed, regenerable: `lanes/d-idlog/fixtures.jsonl` (`proto/idlog_checks.py --fixtures`), the binaries,
  `.tsv` files and fault trees of f-review9, the fault trees of f-repair7.
- A tracker filter by lane name closes too much (`grep f-review9` matched the issue of f-repair7, whose title
  names the review): close issues by id.

## Session 2026-10-03 00:00 to about 00:15 UTC (orchestrator Claude Fable, cloud container, after a container restart; no lanes): START HERE

**One line.** The cloud container restarted at about 23:55 on 2026-10-02 and killed lane f-review7 (the review of
f-slice9's powers and of f-repair4's `dth_root`) before it wrote `result.md`. Its work is committed as it stood,
with `lanes/f-review7/progress.md` as the record: 3 MINOR, 0 blocker, 0 major; the five attacks it wrote after its
last note were rerun by the orchestrator, 0 failures. No source file changed. Everything is on the SESSION BRANCH
`ccr-f62bc633-f7fnqs`, pushed, NOT on master: merge it as the entry below says.

**First commands.** Those of the entry below (merge the session branch, `tools/orch/suites.sh -j2` on master).

**Quota.** 0.41 (0.43 with overage included) at 00:00 against the 0.45 stop of TJO's rules; the evening's four
Opus lanes cost 0.04 between them. A fresh review lane was not launched for that reason.

**The next steps**, replacing step 3 of the entry below (its steps 1, 2, 4, 5 stand):
1. Merge the session branch into master; `tools/orch/suites.sh -j2`.
2. Read `lanes/f-review7/progress.md` (the lane's notes, then the orchestrator's addendum with the reruns). Its
   three minors are the material of one small repair lane: the stale cost sentence of `lpow.h`; the `LIMIT` for
   a small `powunit` result (the same shape as f-review6 F1 and its repair in f-repair4); the false order sentence
   in `lanes/f-repair4/result.md`. What the review did not get to: refereeing P1 to P8 and R8 as proofs, a cost
   measurement, a lane test that cannot fail. Either a second review lane for those three bullets (brief
   `lanes/f-review7/brief.md`, trimmed to them), or accept the review as partial and record it under
   `docs/reviews/f1/` from `progress.md`.
3. Then step 4 of the entry below (1F.8, the all-places forms).

**Things to know.**
- The lane's binaries (`h`, `hsan`, `at`, `adf`), its `build/` and its input dumps are not committed;
  `progress.md` says how to regenerate the dumps. `h.c` needs `-std=gnu11` (`clock_gettime`).
- The worktree `../adelefeld-wt/f-review7` (branch `lane/f-review7`) is merged and may be removed with the others.
- The quota check-in routine (`send_later`, one shot) has fired and ended; nothing is scheduled.
- The worklog of this session: `docs/worklog/2026-10-03.md`.

## Session 2026-10-02 20:30 to about 22:30 UTC (orchestrator Claude Fable, cloud container; Claude Opus lanes only): START HERE

**One line.** In a cloud container (FLINT absent at the start, installed): the five suites the previous session
could not run on master are green; the local roots (1F.5) are reviewed (lane f-review6: no wrong result, 4 minor)
and repaired with decision N-D14 (`K = min(N, E)` for ball roots, reversing the open item of N-D13; lane
f-repair4); the two minors of f-review5 are repaired (lane f-repair3). Everything is on the SESSION BRANCH
`ccr-f62bc633-f7fnqs`, pushed, NOT on master: merge it (`git merge origin/ccr-f62bc633-f7fnqs` on master, or a PR).

**First commands.**

    cd ~/Projects/adelefeld && git fetch origin && git log --oneline master..origin/ccr-f62bc633-f7fnqs | cat
    git merge origin/ccr-f62bc633-f7fnqs        # or review the branch first; then
    tools/orch/suites.sh -j2                    # all, san, clang, inv, headers in parallel build dirs; one line each
    bd ready | head -20                         # the tracker was NOT available in the cloud container (below)

**Master.** Unchanged this session (`c1494d1`). The session branch after all four merges: `make -j3 check-all` passed
(74 programs; driver 49 cases, 100916 lines; exports 424 of 424; Julia; both self-tests). Not rerun on the final
branch: `SAN=1`, `CC=clang`, `INV=1` as whole suites (the changed programs ran under ASan/UBSan in the lanes and
by the orchestrator); run `tools/orch/suites.sh -j2` after the merge into master. On it, in the container: `make check` 73 programs, driver,
exports, Julia (real Julia 1.13.1), mutate and memcheck self-tests, `SAN=1` 73 of 73, `CC=clang` 73 of 73, `INV=1`
73 of 73, `check_headers.sh`: all passed. On the session branch after the three merges: `make -j3 check-all`
(all steps green after the repair of `tools/adf/Makefile` below) and the three changed test programs under
ASan/UBSan with leak detection on (`test_lroot` 10 tests 1308401 checks, `test_rfunc_prime` 14 tests 521989
checks, `test_lfunc_trig` 7 tests 2174090 checks; 0 failed).

**Rules of TJO for this session** (2026-10-02 20:40): orchestrate; Claude Opus subagents only for coding and
review, Sonnet for miscellany; stop at 45% of the weekly quota; install what is needed, create what tools are
needed. The weekly quota read 0.37 at the start, 0.39 at 21:35 and 0.40 at 23:00 (the session's `rate_limit_event` records,
`seven_day.utilization`; 0.42 with overage included).

**What landed** (brief `lanes/<lane>/brief.md`, result `lanes/<lane>/result.md`; details in the worklog).

| Lane | What | Review |
|---|---|---|
| f-review6 | review of f-slice8 (local roots), own oracle in exact integers, 330015 cases | `docs/reviews/f1/review-lroot.md`: 0 blocker, 0 major, 4 minor (F1 to F4); recommends reversing N-D13: taken as N-D14 |
| f-repair3 | f-review5 R1 (F13 sentence), R2 (paired Horner steps, statement F15) | identical residues on 25340 compared calls; 50 ms to 25 ms; 60 mutants, 14 survivors, none a test gap; NOT reviewed again (minors) |
| f-slice9 | 1F.6: `lpow.h`: `adf_lball_powrat` (rational powers through the branches of `lroot.h`), `adf_lball_powunit` (`exp(s log u)`, Proposition 18 exact image); `_at` forms; driver `powrat_at`, `powunit_at`; Julia; P1 to P8 in `docs/api-1f6.md`; decision N-D15 | NOT reviewed; check-all passed in its worktree and on the merged tree (below) |
| f-repair4 | N-D14; F1 (no power pre-check), F2 (branches `t0 zeta^i` from a primitive root, statement R8; `LIMIT` before listing; the principal-unit root once per call), F4 (R7 step 2 for `j >= 0`) | byte-identical where `K` is unchanged, 111487 cases; listing 65536 branches 5.8 s to 14 ms; the 51.8 s case under 1 ms; 60 mutants, 9 survivors, 1 test gap closed; NOT reviewed |

Also: `.claude/hooks/session-start.sh` (cloud sessions only: FLINT 3.0.1, pdftotext, python-flint, sympy, mpmath,
Julia, `refs/src`; idempotent, synchronous; once on master every cloud session gets it); `tools/orch/suites.sh`;
the two 2.9 MB logs of f-slice8 removed.

**The next steps**, in this order:
1. Merge the session branch into master and run `tools/orch/suites.sh -j2` on master.
2. A review of f-repair4 is not strictly needed (the reviewer's own oracle and attacks were rerun by the lane
   against N-D14, 0 failures), but the new enumeration (R8, `dth_root` by CRT and a digit-by-digit split) is new
   code that only the lane's tests saw: a short Opus review with its own oracle at `p = 65537` and `2^64 - 59`.
3. Lane f-review7 (review of f-slice9 and of f-repair4's `dth_root`; brief `lanes/f-review7/brief.md`) was launched
   at 23:18 at the end of the session: if its worktree `../adelefeld-wt/f-review7` holds a `result.md`, commit and
   merge it and record it under `docs/reviews/f1/`; if not, the lane was cut off by the quota stop and
   `progress.md` says where it was. Its findings decide whether a repair lane follows.
4. Milestone 1F continues: 1F.8 (all-places forms; the lane proposes it next: the rational-root contract and the
   rational power of an exact rational at all places need only `lroot.h` and `lpow.h`), then 1F.9. Text
   forms of `adf_lball`, `adf_sball` (conventions 9). Then milestone 2 dump forms; milestone 3.
5. Left: the mutation tool's `--san` is defeated by `ASAN_OPTIONS` in the `--make` command and a run of 60
   uncompiled mutants printed "passed" (f-slice9); N-D14 made two behaviours explicit (a ball with `N <= j` gives
   the zero ball at `N`; a ball whose capped exponent leaves the limits is `LIMIT` where N-D13 gave `OK`); one
   Teichmueller lift per branch in `adf_lball_roots` (44.6 s for `d = 299756` at `N = 20`; the lift is
   multiplicative, two lifts and `d` products would do; f-repair4 finding 3); the mutation tool's `swap_args`
   generates a no-op mutant when both arguments are the same expression (f-repair3 finding); `count_exp`'s
   `max(1, ...)` branch is unreachable (f-repair3); adf-7yz, adf-4dj, f-review4 MINOR 2, benchmarks on a quiet
   machine, adf-mds, adf-7gc.

**Waits for TJO.** Nothing blocks. Open to reversal: N-D14, N-D15 (and N-D12, N-D13 as before). N-D15 reads
"take the union" of SPEC 9.3.4 and Proposition 18 step 5 as "the smallest ball containing it" (the union is not a
ball); say if you want `NEEDS_SPLIT` or `NOT_DETERMINED` there instead.

**Things to know.**
- The beads tracker is unusable in a cloud container: its data is an embedded Dolt database under
  `.beads/dolt/`, gitignored and absent from the clone; `bd prime` in the hooks fails silently. Issues opened or
  closed this session are therefore recorded here and in the worklog only: f-review5 R1 and R2 repaired;
  f-review6 F1 to F4 repaired; new open items in step 4 above.
- `tools/adf/Makefile` never rebuilt the driver's library `build/drv-plain/libadelefeld.a` after the first build
  (a rule without prerequisites), so `test_driver.sh` on a tree with a changed `src/` ran a STALE driver; repaired
  this session (`FORCE`). Trees built before the fix: `rm -rf build/drv-plain build/adf` once.
- `refs/fetch_sources.sh` rewrites both manifests in fetch mode; restore them with `git checkout` afterwards
  (the hook does). Five files of the live `adeles` clone have moved upstream; the pinned snapshot verifies.
- A Claude subagent cannot write a file named `report.md` (COMMON.md rule 8); the lanes of this session wrote
  `result.md`.
- Lane worktrees under `../adelefeld-wt/` (f-review6, f-repair3, f-repair4) are merged and may be removed.

## Session 2026-10-02 13:12 to the evening (orchestrator Claude Fable; codex astra lanes only): START HERE

(The machine slept during the session; the clock read 2026-10-03 00:12 to 00:35 when it woke and 21:31 on 2026-10-02 at the end, so the times after the sleep in this entry and in the worklog are from a clock that was wrong one way or the other.)

**One line.** Five codex `gpt-6-astra` xhigh lanes: the P1 fixture repair (adf-6fe) landed; milestone 1F gained 1F.7
(`sin`, `cos`, `sinh`, `cosh` at a prime) and 1F.5 (local roots), both merged with proofs, oracles and driver
commands; the repairs of n-repair1 were reviewed (no blocker or major; monotonicity of the printer's level condition
refuted, a lower bound proved); the review of 1F.7 (lane f-review5) found two minors and no blocker.
Everything named here is merged and pushed; the lane branches are pushed too.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    cat ../adelefeld-wt/f-review5/lanes/f-review5/lane.log; ls ../adelefeld-wt/f-review5/lanes/f-review5/report.md
    make clean && make -j2 check-all 2>&1 | tail -1     # expect: check-all passed: ... (73 programs)
    make clean && make -j2 check SAN=1 2>&1 | tail -1; make clean && make -j2 check CC=clang 2>&1 | tail -1
    make clean && make -j2 check INV=1 2>&1 | tail -1; sh lanes/m1-headers/check_headers.sh | tail -1
    ~/Projects/quota-app/target/release/quota; date

**Master.** `make clean && make -j2 check-all` on `2fa244b` (the code of HEAD): 73 test programs, driver, exports, Julia, both self-tests pass (log read at the end of the session). `check_headers.sh` passed at 14:20 on `c09663a`. NOT run on master this session
(battery): `SAN=1`, `CC=clang`, `INV=1`. Run them first on mains power. Each lane ran its own new test programs under
ASan/UBSan (`detect_leaks=0`) and f-slice8 ran `check-all` in its worktree (passed).

**Rules of TJO for this session** (2026-10-02): use the codex quota; only codex `gpt-6-astra` xhigh subagents; at
most two concurrent; avoid compute-heavy tests, the machine is on battery. The rules of 2026-09-29 (HANDOFF below,
memory `orchestration-model-tiers`) stand otherwise.

**What landed** (brief `lanes/<lane>/brief.md`, report `lanes/<lane>/report.md`; details `docs/worklog/2026-10-02.md`).

| Lane | What | Review |
|---|---|---|
| f-fixture1 | adf-6fe: `tests/ref/vectors/f-slice5/stored_large.jsonl`, 465 old-code rows, F8 and F9 at every prime; six planted faults fail the new comparison | closes f-review4 MAJOR 1; MINOR 2 (small-N regression) left open |
| f-slice7 | 1F.7: `adf_lball_sin`, `cos`, `sinh`, `cosh`; `_at` forms at both places; driver `sin_at` etc.; F10 to F14 in `docs/api-1f4.md`; N-D12 | f-review5: 0 blocker, 0 major, 2 minor (see below) |
| n-review2 | review of n-repair1 (N-D11 bound, D1, C1, R5, C2) | `docs/reviews/m2/review-nd11-repairs.md`: 0 blocker, 0 major, 2 minor (texts corrected in `7f67f88`); adf-7yz (optional lower-bound skip) |
| f-slice8 | 1F.5: `include/adelefeld/lroot.h`, `src/lroot.c` (count, seeded branch, all branches); seeded `_at` forms; driver `roots_at`, `root_at`; R1 to R7 in `docs/api-1f5.md`; N-D13 | NOT reviewed |

Also: the wall-clock guard of `constrained_printer_ends_in_bounded_time` is 30 s (adf-4j8; 2 s failed on battery);
`tools/orch/codex_lane.sh` passes `-m MODEL` on resume (a resume had fallen back to gpt-6.1-sol).

**Lane f-review5 landed at the very end** (review of f-slice7; `docs/reviews/f1/review-lfunc-trig.md`): NO blocker or major; two MINOR: R1, the F13 sentence on `LONG_MIN` lacks the qualifier that the domain check comes first (`docs/api-1f4.md` 422-423; the code's order is right); R2, the absent parity doubles the Horner work (76 ms against 36 ms at `N = 2000`, `p = 3`; the reviewer's paired-step version gives identical residues; an own proof that the pairing is exact). Attacked without a finding: enumeration, limits, large `N`, the regression of `exp`/`log`/`Log` against `27f7e5f` (1008 cases, 0 differences), the real `sinh_at`/`cosh_at`, the driver (224 lines, 116 hostile commands). Neither minor is repaired yet.

**The next steps**, in this order:
1. The suites not run on master (above). The two minors of f-review5 (R1 one sentence; R2 pair the Horner steps, identical results required: the stored fixtures and the f-slice7 oracle check it).
2. A review of f-slice8 (local roots; the one with the widest interface decision, N-D13: a ball input returns the
   exact image exponent regardless of the requested `N`, unlike `lfunc.h`). Codex astra with its own oracle, the
   pattern of `lanes/f-review5/brief.md`.
3. Lane f-slice9 (1F.6) was launched at the end of the session (brief `lanes/f-slice9/brief.md`); if its
   worktree `../adelefeld-wt/f-slice9` holds a `result.md`, commit and merge it as the others; if not, the lane
   was cut off by the quota stop and `progress.md` says where it was.
4. Milestone 1F continues after it: 1F.6 (rational powers through the branches of `lroot.h`; principal-unit powers
   `exp(s log u)`, Propositions 17, 18), then 1F.8, 1F.9. Text forms of `adf_lball`, `adf_sball` (conventions 9).
4. Milestone 2: dump forms of the three types. Then milestone 3: a first slice.
5. Left: adf-7yz (optional), adf-4dj, f-review4 MINOR 2, the f-slice8 cost note (all branches recompute the log),
   benchmarks on a quiet machine, adf-mds, adf-7gc. Two 2.9 MB logs of f-slice8 could be deleted from the tree.

**Waits for TJO.** Nothing blocks. Open to reversal: N-D12, N-D13 (`docs/SPEC.md` 15.4), N-D1 to N-D11 as before.

**Things to know** (in addition to the list of 2026-09-30).
- The codex content filter refused n-review2 once after 8 minutes of work ("possible cybersecurity risk"), as it
  did n-review1; the runner's resume then continued the session and finished. Ask lanes to write `progress.md`
  as they go (all five did).
- A brief that says "no run of more than 3 minutes" also caps the lane's `check-all` (about 6 minutes on this
  machine): say `timeout 900` for the one acceptance run explicitly.
- `pkill -f "codex exec resume"` kills the orchestrator's own shell when its command line contains the pattern.
- The lanes' worktrees under `../adelefeld-wt/` of this session (f-fixture1, f-slice7, n-review2, f-slice8, f-review5)
  are all merged and may be removed.
- The `.beads` permission warning (0775) in every worktree is noise from `bd`'s hook.

## Session 2026-09-29 22:37 to 2026-09-30 (night; orchestrator Claude Fable): START HERE

**One line.** Fourteen lanes landed in one night: milestone S is closed (reviewed), the real roots are fast
(exact isolation, two slices), milestone 2 (ideles) is complete and reviewed except for its text and dump
forms, milestone 1F has work packages 1F.1 to 1F.4 (local balls, partial balls, real functions, `exp`,
`log`, `Log` at a prime), reviewed and repaired. Everything named here is merged and pushed. State of the
running lanes at the time of writing (03:40): see "Running".

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make -j2 check-all 2>&1 | tail -1     # expect: check-all passed: ... (70 or more programs)
    make clean && make -j2 check INV=1 2>&1 | tail -1; sh lanes/m1-headers/check_headers.sh | tail -1
    free -g; ~/Projects/quota-app/target/release/quota; date

**Master.** Last full run of all suites (check-all, SAN=1, CC=clang, INV=1, check_headers) at 02:56 on
`00aa1da`: 70 test programs, all pass. Merged after it without a full run on master: lane i-repair1
(`1cf0e42`; the lane ran all five suites in its worktree, 70 programs). Run the suites first.

**Rules of TJO for this and later sessions** (2026-09-29 22:40; memory `orchestration-model-tiers`): land
features as vertical slices; a proper review after each vertical slice; decide what a senior scientist
would decide and record it (`docs/SPEC.md` 15.4, N-D1 to N-D10), ask TJO only for a real blocker; top tier
for intricate or performance code is codex `gpt-6.1-sol` (high or xhigh) and Claude Opus; Sonnet medium for
standard code and for quick bug hunts; `space-bunny-alpha` is free and works; astra and Fable only for
theorem-grade work; no indiscriminate fuzzing or mutation runs; look at quota and at the machine
semiregularly. TJO confirmed S-D20 and ratified the reading of `adf_resid_verify_result` (now S-D21).

**What landed** (each: brief `lanes/<lane>/brief.md`, report `lanes/<lane>/result.md` or `report.md`).

| Lane | Model | What | Review |
|---|---|---|---|
| s2-review4 | codex 6.1-sol | review of roots modulo large primes | no blocker; fuzz oracle and proof text repaired; adf-e0n closed |
| drv-s | space-bunny | driver `roots`, `realroots`, `recover` (N-D1) | none |
| r-slice1 | Opus | real roots by exact isolation (N-D2); 111 s to 0.00007 s | r-review1 (codex): no wrong list, cost |
| r-slice2 | codex 6.1-sol | count-steered isolation, contraction, scaled count, filtered refinement | r-review2 (Sonnet hunt): no defect; certificate argument with file and line |
| f-slice1, f-repair1 | Sonnet | `adf_lball` (N-D3 to N-D5, N-D7) | f-review1, closed by f-review2 |
| f-slice2, f-repair2 | Sonnet | `adf_sball`, projection, real functions, `ADF_REAL_PREC_MAX` (N-D8) | f-review2 (codex); repairs NOT judged again |
| f-slice3 | Sonnet | `p^m w u`, Teichmueller, fractional part, `pow_si` | NOT reviewed |
| f-slice4 | Opus | `exp`, `log`, `Log` at a prime (N-D9) | f-review3 (codex): no wrong enclosure; cost (lane f-slice5) |
| f-slice6 | Sonnet | `_at` functions at a prime; driver `project`, `exp_at`, `log_at` (N-D10) | NOT reviewed |
| i-slice1 | Opus | unit cosets, ideles (N-D6) | i-review1 (Sonnet hunt): no defect |
| i-slice2, i-slice3 | Opus | classes, valuations, norm; powers, hulls, division | i-review2 (codex): no wrong enclosure |
| i-repair1 | Sonnet | repairs of i-review2; documents follow the code (conventions 3.2, 5.6, 5.7, 7; PLAN; SPEC 9.3.2; statement indexes) | repairs NOT judged again |

Reviews are under `docs/reviews/s2/review-bigp.md`, `docs/reviews/r1/`, `docs/reviews/f1/`, `docs/reviews/m2/`.

**Landed after 03:40** (all pushed; last full run of all suites on master at 05:20 on `3ed2586`, 71 test
programs, all pass; after it only documents and lane files were committed):
- f-slice5 (codex 6.1-sol): the series at a prime fast (F8, F9; `log(1 + p)` at `2^64 - 59`, `N = 10000`:
  74.8 s to 0.043 s; results identical). Referee f-review4 (Fable, `docs/reviews/f1/review-lfunc-fast.md`):
  F8 and F9 TRUE as written, code matches the digit counts, 21188 inputs old = new = own oracle; MAJOR 1:
  the stored fixture guards F9 with 9 of its 2000 rows and none at the word prime (issue in beads, P1;
  repair from the OLD code at `1cf0e42`, generator `lanes/f-slice5/gen_stored.py`).
- t-slice1 (Sonnet): value form of unit cosets, ideles, classes; driver `inv`, `pow`, `powtight`, `norm`,
  `class`, `idele`, `hull`, `hullsimple`, `unitof`, `valuation`, `abs`. Dump forms NOT done.
- n-review1 (codex 6.1-sol): the provider REFUSED the session before it wrote its report ("flagged for
  possible cybersecurity risk", four attempts); the orchestrator assembled `docs/reviews/f1/
  review-slices-3-6-text.md` from the lane's notes and logs. Findings: BLOCKER, the driver read the wrong
  field of an idele operand in `project`, `exp_at`, `log_at` (a wrong printed value); 3 MAJOR (overflow
  of `N - v` at the exponent limit; the constrained printer did not end on the worst admitted ball; `prec`
  after the entry check in `sball`/`rfunc`); all repaired by n-repair1 (Sonnet), N-D11 (work bound of the
  printer: balls whose search needs more than about 2^25 / S levels are refused now).
- f-review2 R3 to R8 and i-review2 F1 to F5: CLOSED (n-review1 part A, B).

**Nothing is running.** Quota at 05:20: Claude weekly 15% (8.2 points ahead of pace), Fable weekly 14%
(7.2 ahead: the referee f-review4 cost about 9 points), codex 8% (2.7 ahead). Let the Claude windows fall
back to pace before the next Claude lanes; codex has room.

**The next steps**, in this order:
1. The stored fixture of `tests/test_lfunc.c` (P1 issue above): rows with `K` in 39..64 and 65..3000 at every
   prime, from the old code. One Sonnet lane or the orchestrator; then the planted faults of
   `lanes/f-review4/faults.py` must fail the stored comparison.
2. Milestone 1F continues: `sin`, `cos`, `sinh`, `cosh` at a prime (1F.7; `src/lfunc.c` is free now);
   local roots (1F.5); powers (1F.6); then 1F.8, 1F.9. Text forms of `adf_lball` and `adf_sball`
   (conventions 9; golden vectors exist), where the driver text of N-D10 meets the value form.
3. Milestone 2: dump forms of the three types. Then milestone 3 (quotient and characters): a first slice.
4. A review (codex) of n-repair1 (the printer bound N-D11 in particular) together with the next slice.
5. adf-4dj: the remaining cost of the real roots is FLINT's count; replacing it touches S-D11.
6. Left from before: benchmarks of milestone 1 on a quiet machine; adf-mds, adf-7gc; a long run of
   `diff_roots_padic.py` on the route above 128 (the reviewer's oracle covered it).

**Waits for TJO.** Nothing blocks. Open to reversal: N-D1 to N-D10 (`docs/SPEC.md` 15.4). The one with
the widest reach is N-D7 (`LIMIT` of local balls may be returned when a power of `p` above the bit limit
would be formed; a rule "never because of an intermediate value" is not promised).

**Things to know.**
- A Claude subagent that runs its check suites in the background reports "finished" several times
  before its report arrives: tell it to run everything in the foreground.
- Lanes leave compiled binaries and large logs in their lane directories: delete them before the
  commit in the worktree (`find lanes/<lane> -type f -exec file {} + | grep ELF`).
- The codex sandbox cannot run LeakSanitizer (ptrace): a codex lane reports the default `SAN=1` suite as
  failed and passes with `ASAN_OPTIONS=detect_leaks=0`. The orchestrator's run on master is the check.
- "34 mutants survived" in the log of `check-all` is the self-test of the mutation tool (its weak
  example), not a failure.
- `bd` has one writer: two `bd` commands in one pipe fail with a lock error.
- A codex session can be refused by its provider's filter after it has done its work (n-review1: the
  text about sanitizers and hostile input, probably); the lane's `progress.md` and logs are then the
  report. Ask lanes to write `progress.md` as they go.
- A Fable subagent as a referee costs about 9 points of the Fable weekly window in 95 minutes: use it
  for a proof that decides enclosures, not for a review of code.
- About 90 worktrees exist under `../adelefeld-wt/` and `.claude/worktrees/`; all lanes named above are
  merged and their worktrees may be removed with `git worktree remove`.
- The three design branches `worktree-agent-aa58...`, `acc1...`, `adaf...` are superseded by the slices;
  the real-roots one is merged, the other two are not needed any more.

## Session 2026-09-29, 20:45 to 21:10 (recovery): START HERE

**One line.** The session of 16:24 ended at the quota limit with six lanes running. Recovered: two lanes
landed (s2-slice4, m1-small), one had landed before the end (s-protosync), three design lanes are
UNFINISHED and saved on origin; the four long differential runs ended with 0 disagreements. Nothing is
running (the autosave loop is stopped). Everything is pushed.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make check-all 2>&1 | tail -1     # expect: check-all passed: ... (57 test programs)
    free -g; ~/Projects/quota-app/target/release/quota; date

**Master** (21:06): `make check-all` (57 test programs, driver, exports, Julia, the two selftests),
`make check SAN=1`, `make check CC=clang`, `check_headers.sh` pass, all read before the push.

**What landed now.**
- s2-slice4 (Opus, adf-e0n): `adf_roots_padic` for every prime of a place; the temporary `UNSUPPORTED`
  above `2^20` is gone. Roots modulo `p` by evaluation up to `ADF_ROOTS_P_EVAL_MAX = 128` (crossover
  measured, loaded machine), above by `gcd(h, X^p - X)` and `nmod_poly_roots`, whose list is checked and
  refused by abort (S-D20). FLINT's C sources of these routines are on disk and cited. New
  `tests/test_roots_bigp.c`, `bench/bench_roots_modp.c`; `diff_roots_padic.py` now covers primes up to
  64 bits (smoke run of 180 s only). Its report: `lanes/s2-slice4/result.md`. The orchestrator changed one
  line of `tests/julia/roots.jl` (it expected `UNSUPPORTED` at 1048583; now it checks the three roots).
  NOT YET REVIEWED: a codex review with its own oracle is the next step for this slice.
- m1-small (Sonnet): adf-zbl (text R9 reaches prec 2 and 191; four corpus files), adf-mds items 1 and 2
  (stale comment; `all_load` runs the typed loaders), adf-bgf (conventions 12.1, 11.3, 8.4, prec below 2),
  adf-7gc item 3 (`adf_sizeof_text_kind`, `adf_alignof_text_kind`). Report `lanes/m1-small/result.md`.
- Long differential runs (seed 2026092902, one hour each, code of 14:05): linear systems 13472390
  systems, roots seed 17224715 calls, Algorithm P 9182030 calls, real roots 33653 calls; 0 disagreements
  each (`../adelefeld-wt/longrun/longrun.log`, copied into the worklog). Algorithm P ran on the code BEFORE
  s2-slice4; the new route above 128 has had 180 s only.

**Unfinished lanes, saved as WIP on origin** (branch `worktree-agent-<id>`, worktree
`.claude/worktrees/agent-<id>`; read `lanes/<lane>/brief.md` and `progress.md` in the branch; continue
with a new agent from that branch; none has `result.md`):
- d-realroots (`aa5882a1c1ea56bed`, adf-8di): cause of the 97 s found (FLINT doubles the precision after
  `4 deg + 64` Durand-Kerner steps; linear convergence on close roots). Benchmark
  `bench/bench_roots_real.c` and prototype `proto/real_isolation.py` (Descartes bisection, Collins-Akritas
  form) with tests exist; `docs/design/real-roots.md` NOT written. Sturm text source pending.
- d-ideles (`acc17965b8910c1f2`): facts and choices D2-1 to D2-7 in progress.md; `proto/ideles_checks.py`
  touched; `docs/api-2.md` not written. Finding: ball product `arb_mul` plus a sign test cannot be the kernel
  of sign preservation (SPEC 5).
- d-functions (`adaf36d4414085b09`): FLINT probe done, part 2 of `proto/functions_checks.py` (1057 lines)
  started; `docs/api-1f.md` not written. FLINT's padic sources and `arb_hypgeom.rst` are not on disk.

**The next steps**, in this order:
1. Review of s2-slice4 by codex `gpt-6-sol` (its own oracle; the trust base of `nmod_poly_powmod` and
   `nmod_poly_gcd` named in the header; the three findings against `docs/proofs/solvers.md` in its report:
   S-D10 source-pending line, Algorithm P step 3 covers only evaluation, cost P3.5(7)). Then close adf-e0n.
2. Continue d-realroots (closest to done), then d-ideles and d-functions, each from its branch.
3. A long run of `diff_roots_padic.py` on the new route (an hour, alone).
4. Left of adf-mds: `proto/text_grammar.py` restricts the character modulus; `MAG_MAN`/`MAG_EXP` source
   pending. Left of adf-7gc: `tests/julia/layouts.jl` and `docs/api-m1.md` do not list the new text-kind
   pair; the other items.
5. As before: benchmarks of milestone 1 on a quiet machine; a driver command for milestone S if TJO wants.

**Waits for TJO.** As in the section below (S-D20 confirmation; the reading of `adf_resid_verify_result`).

**Things to know.**
- `tools/orch/autosave.sh` saved the lanes every five minutes and made this recovery possible. It does not
  stop by itself when the lanes die: stop it at a session end (`touch lanes/AUTOSAVE_STOP`).
- The WIP autosave commit `13957cc` of s-protosync is in master's history through the merge `352aef0`.

## Session 2026-09-29, 09:15 to 14:10

**One line.** Milestone S is implemented in slices and reviewed slice by slice: reconstruction from a
residue (S.3), linear systems modulo `N` (S.1), roots at a prime and real roots (S.2). What is missing:
the root search for primes between `2^20` and `2^64`. Nothing is running. Everything is pushed.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make check-all 2>&1 | tail -1     # expect: check-all passed: ... (56 test programs)
    free -g; ~/Projects/quota-app/target/release/quota; date

**Master** (14:05): `make check-all` (56 test programs, driver, exports 260 of 260, Julia, the two
selftests), `make check SAN=1`, `sh lanes/m1-headers/check_headers.sh` pass. `CC=clang` passed at 12:30 (54
programs) and in the lanes since. `INV=1` was last run at 09:55 (47 programs). Read every check BEFORE the
push; run `check_headers.sh` too, it is not part of `check-all`.

**What exists now** (headers `resid.h`, `linsolve.h`, `roots.h`; record `docs/worklog/2026-09-29.md`):
- S.3 complete. S.1 complete. S.2: seed function, Algorithm P (roots modulo `p` by evaluation, temporary
  `UNSUPPORTED` above `2^20`), real roots, all verifiers.
- The user call of each slice is a Julia file under `tests/julia/`, run by `tests/test_julia.sh`. The
  driver `adf` has NO command for milestone S: its grammar takes three operands.
- Reviews: `docs/reviews/s3`, `s1`, `s2` (three files), `s13`. Every finding is repaired or decided, except
  the cost of real roots (adf-8di).

**The next steps**, in this order:
1. adf-8di: real roots are correct and can be very slow (a quadratic with the roots `2^1500` and
   `2^1500 + 1`: 97 s at `prec = 2`; the time is in `arb_fmpz_poly_complex_roots`). First a benchmark row
   and a lower bound (`docs/PERF.md`), then a design: isolation of the real roots in exact arithmetic
   (Descartes or Sturm bisection on `g`), FLINT's count as the check. Design before code; `gpt-6-astra`
   may review the design.
2. adf-e0n: the root search for primes above `2^20` (`solvers` P3.7(2)). One-word primes, so `nmod_poly`;
   its root routine is NOT documented in FLINT 3.0.1 (`docs/sources.md`, lane s2-sources): fetch and read
   the C source first (CLAUDE.md rule 4). Then the temporary `UNSUPPORTED` goes and the bound becomes a
   crossover set by a benchmark.
3. Long differential runs (an hour each, alone, at night) for `tests/fuzz/diff_linsolve.py`,
   `diff_roots_seed.py`, `diff_roots_padic.py`, `diff_roots_real.py`. Only the reconstruction has had one.
4. Milestone 1: benchmarks on a quiet machine; then mark milestones 1 and S in `docs/PLAN.md` section 6.
5. A command of the driver for milestone S, if TJO wants one (a change of its grammar, M1-D1).

**Waits for TJO.**
- S-D20 (`docs/SPEC.md` 15.3): recorded as "a solver aborts when its own check fails". TJO ratified "all
  recommendations" where the orchestrator had asked a question without one. To be confirmed.
- The reading chosen for `adf_resid_verify_result` (it verifies the status that the function returns for
  the given `limit`, and shares the search with the function; Proposition 1.11 and the Python reference
  verify the whole set of solutions, without a bound on the cost). The Python reference
  `recon_verify_result` was NOT changed and now differs from the C function by design.

**Things to know.**
- space-bunny works. Its two failures were ours: `pi -p` ends on an empty response of the provider (the
  runner continues now), and a test program that does not end stops the lane (rule 3 of
  `lanes/COMMON.md`; the watchdog of `tools/orch/pi_lane.sh` is untested). Prefer one bunny lane at a time.
- A brief for a Claude subagent: push first; the brief is a file in the tree; tell the agent to run its
  checks in the foreground and to report once (an agent with background jobs reports "paused" many times).
- A brief must ask for `make check-all` and `check_headers.sh`, and must not say "tests unchanged" where a
  temporary behaviour is replaced.
- Codex reviews cost about one point of the week each; the meter lags. TJO allowed 60% used (it read 50%
  at 14:05; resets 2026-10-03). Claude weekly resets 2026-09-29 18:00.
- Worktrees of the lanes of today are under `../adelefeld-wt/` and `.claude/worktrees/`; all merged, all
  may be removed with `git worktree remove`.

## Session 2026-09-28 22:30 to 2026-09-29 08:30

**One line.** Milestone 1 is reviewed and its review is closed with no blocker open; the design of milestone S
is written, reviewed and closed (draft 3); TJO changed the workflow: thin working slices, no mutation sweep,
less ceremony (`docs/workflow.md`, read it first). Nothing is running. Everything is pushed.

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make -j2 check 2>&1 | tail -1          # expect: check passed: all 42 test programs
    free -g; ~/Projects/quota-app/target/release/quota; date

**Master** (checked 00:58 on 2026-09-29, after the last change of code): `make check` 42 test programs with
gcc, clang, `SAN=1`, `INV=1`, `INV=1 SAN=1`; `sh tests/test_driver.sh` (27 cases), `sh tests/test_julia.sh`,
`sh tests/test_exports.sh`; `python3 tools/mutate/selftest.py`, `python3 tools/memcheck/selftest.py`;
`pytest proto` 35; `python3 proto/solvers_checks.py` 35 checks (03:43). `make clean` between builds with
different flags.

**THE NEXT STEP: slice 1 of milestone S** (issue "Milestone S, slice 1", P1; `docs/workflow.md` rules 1, 2).
Partial rational reconstruction in the range `2 A B < m`, end to end:
- `include/adelefeld/resid.h`: the type `adf_resid` `(c, m)` and ONE function that reconstructs;
  `docs/api-s.md` section 2 has the proposed declarations. Only what the slice needs.
- `src/resid.c`: the library's own Euclidean loop (`docs/proofs/solvers.md` L1.4, P1.6 case `2 A B < m`:
  the row if `gcd(R, T) = 1`, else none). Outside the range the slice returns `UNSUPPORTED`, and says so.
- Tests first (CLAUDE.md rule 1): against brute force over all small `(m, c, A, B)` and against
  `recon_partial` of `proto/solvers_checks.py` (vectors written by a script). The edge cases are listed in
  `docs/reviews/s-design/review.md` ("Your task is to REFUTE", item 1).
- One command of the driver `adf` (or a Julia call) that reconstructs a fraction from a residue.
- One differential fuzz target (workflow rule 5), run for an hour at night.
- Needs three small decisions of TJO before the header is written: S-D1 (a type `adf_resid`), S-D2 (own loop,
  not FLINT's `fmpq_reconstruct_fmpz_2`, which writes its outputs before it decides), S-D5 (`A < 0` or
  `B < 1` is `NO_SOLUTION`). ASK THESE THREE, not all nineteen.
- One lane (`sonnet-medium`, or `space-bunny-alpha` if its provider answers), then one review by codex
  `gpt-6-sol`. Slice 2 widens to `m <= 2 A B` (enumeration, `NOT_DETERMINED`); then S.1.

**Waits for TJO.**
- For slice 1: S-D1, S-D2, S-D5 (`docs/api-s.md` section 5).
- Milestone 1: M1-D10 and M1-D11 (adf-s04; PROPOSED in `docs/SPEC.md` section 15; the tests follow them
  already; two judges found no input against them; once accepted, one sentence each in `fball.h`,
  `scaled.h`, `recon.h`, conventions 4.6); adf-xrt; adf-qs9.
- The other decisions S-D3, S-D4, S-D6 to S-D19 are asked when a slice needs them. S-D13 (roots of the
  squarefree part) reinterprets SPEC 9.1 and is TJO's alone.

**What landed in this session** (details: `docs/worklog/2026-09-28.md` from 22:30, `docs/worklog/2026-09-29.md`).

| What | Commit | State |
|---|---|---|
| Decisions M1-D9, wording of M1-D1 and M1-D6, the words of `compare` | `daddefc` | ratified by TJO |
| Sonnet effort levels compared: medium chosen | `4fbb142` | memory `orchestration-model-tiers` |
| `ADF_CHECK_INVARIANTS` (`make check INV=1`), adf-xk4 | `0418288` | release objects identical (own baseline) |
| Five test programs brought inside the contract, adf-6vy | `1743426` | no check weakened |
| Mutation tool, `equivalent.txt` (108 entries), memory checker | `5baf3cb` | the reasons of the entries are NOT reviewed and not trusted as proofs (workflow rule 4) |
| Six closure checks of the review of milestone 1, adf-igt | `fef212f`, `9aa97c9`, `0cc92bd`, `be32d8d`, `755f68a`, `1fdd494` | NO BLOCKER OPEN; no new defect of the code |
| M1-D1: the false reason for the cap of `prec` corrected | `a974d59` | the cap is unchanged; found by the judge of `surface` |
| Sources of milestone S, table 3 of `docs/sources.md` | `90defed` | corrected twice after the design and its review |
| Design of milestone S: draft 1, review (astra), draft 2, closure (sol), draft 3 | `79b0e3c`, `1207ddc`, `da222e9`, `540f57c`, `9600357` | reviewed and closed; proofs read by two codex reviewers, not by the orchestrator |
| Workflow of TJO | this commit | `docs/workflow.md`, CLAUDE.md rules 2 and 6, `lanes/COMMON.md` rule 8, `lanes/COMMON-C.md` rule 5 |

**What is left of milestone 1.**
1. Tests for the known gaps (issue adf-whv, P2): `src/common.c:161` (no test of the status `UNSUPPORTED`
   there), `src/recon.c:272` (`DOMAIN` for an infinite real ball: release build only), and the weak tests
   the judges named: `test_modctx_limits` (the range pin passes without its line), text R3 (passes on the old
   printer), text R9 (coverage only). The builders of `tests/test_fball_local.c` now use
   `adf_fball_set_local`, so its tests of `set_local` are less independent; the vectors still are.
2. The decisions above; then the sentences in the headers.
3. adf-4lj: a target `check-all` of the Makefile (suite, three scripts, the two selftests).
4. Benchmarks on a quiet machine (`make bench`, nothing else running); then milestone 1 marked done in
   `docs/PLAN.md` section 6.
None of these blocks slice 1 of milestone S.

**The sweep is dropped (TJO).** It ran seven files under `INV=1` with sanitizers and was stopped by the
harness at 04:21 for lack of memory, in `src/recon.c`; record in `lanes/m1-sweep/`. Do not start it again.
Its lessons are in `docs/workflow.md`: in the release build the lines `ADF_INV_...` are dead and their
mutants survive; under `INV=1` the code for non-canonical input is not reached; a mutant that removes a guard
of size can use all memory (rule 9). The worktree `../adelefeld-wt/m1-sweep` may be removed.

**Models and quota** (memory `orchestration-model-tiers`). `sonnet-medium` for demanding lanes;
`space-bunny-alpha` when its provider answers (probe it with a one-line prompt: on 2026-09-28 it returned
empty responses for an hour, then worked; forbid `report.md` before the end in its brief); codex `gpt-6-sol`
xhigh for reviews, `gpt-6-astra` for a design or a proof; Fable by exception. At 08:20 on 2026-09-29: Claude
weekly and Fable weekly reset at 18:00 on 2026-09-29; codex 46% of its week, limit of TJO 50% (resets
2026-10-03). Claude subagents only while the quota app shows the window behind or on pace.

**Things to know.**
- A Claude subagent cannot write `report.md` (the harness refuses): `lanes/COMMON.md` rule 8 now names
  `result.md`. Whether the harness admits that name is NOT tested; if not, take the report from the final
  message by a script, do not copy it by hand.
- The pi runner counts a lane as done when `report.md` exists: a model that writes it as a running record
  stops its lane.
- Never wait with `pgrep -f <pattern>` in a loop: the pattern matches the waiting command itself.
- Times in a record are read from `date`; the orchestrator wrote estimates several times in this session and
  had to correct them.
- Old worktrees under `../adelefeld-wt/` and `.claude/worktrees/` hold merged branches and may be removed
  with `git worktree remove`.

## Session 2026-09-28 (late), 21:00 to 21:50

**One line.** Lane m1-repair-dump is landed (adf-tp2 closed); Sonnet 5.5 replaces the nonfree pi models; a
comparison of Sonnet at effort low, medium, high is prepared and needs a restarted session; nothing is running.

**Master** (pushed): `make check` 40 test programs with gcc, clang, `SAN=1`; `sh tests/test_driver.sh` (27
cases), `sh tests/test_julia.sh`, `sh tests/test_exports.sh`, `pytest proto` (35) pass (21:45). First commands
as in the section below, with 40 for 39.

**Model tiers (TJO, 21:10).** The nonfree pi models (`deepseek-flash`, `mimo-v2.6-pro`, through any provider)
are replaced by Claude Sonnet 5.5. Wherever a section below names one of them in a command, use Sonnet. The
free pi model `space-bunny-alpha`, codex and Claude opus are used as before.
- A Sonnet lane is a normal Claude subagent of the orchestrator (Agent tool, worktree isolation), NOT pi and
  NOT a headless `claude -p` runner (TJO). Rules of the section of the day: push first, paste
  `lanes/COMMON.md` and the whole brief into the prompt, save the report from the final message.
- Code written by Sonnet is code of the Claude family: its reviewer and its judge in the closure check is
  codex, not Claude opus. The table of judges in `lanes/m1-closure/COMMON.md` is right for the repairs already
  landed (pi models wrote them; the report of m1-repair-dump was finished by Claude Fable, its code is pi's).
- Quota at 21:00: Claude weekly 83%, 4.5 points behind pace; Fable weekly 79%, 8.5 behind (both reset
  2026-09-29 18:00); codex weekly 36%.

**The comparison of effort levels (TJO, 21:30: "sonnet low or medium is better as it is faster and cheaper,
try out a couple of levels and compare"). NOT RUN.** The Agent tool takes the effort from the agent's
definition, and definitions are read when a session starts: `.claude/agents/sonnet-low.md`,
`sonnet-medium.md`, `sonnet-high.md` were written in this session and the harness did not know them. In the
next session: check that the three agent types are offered; launch all three in one message, each with the
text of `~/Projects/adelefeld-wt/effort-cmp/PROMPT.md` (LEVEL replaced) and its directory
`~/Projects/adelefeld-wt/effort-cmp/<level>/` (a copy of the lane's tree before the orchestrator's test, with
a finished build); compare verdicts, tests, wall time and tokens. The task is the three survivors of the
mutation run of `src/dump.c`; the expected answers are in the comment at the head of `PROMPT.md` and in
`lanes/m1-repair-dump/report.md` section 6, which the agents must not be shown. Then choose the level and
write it into the memory `orchestration-model-tiers`.

**m1-repair-dump as landed** (`lanes/m1-repair-dump/report.md`; sections 4 to 9 by the orchestrator):
- The four timeouts of the mutation run are killed mutants, not loops without end: each fails checks at once,
  and the time is the building of contexts of 65537 blocks that the mutant lets through.
- Survivors: `:938` and `:883` killed by the new test `grammar_before_limits_in_sball_and_rfun` of
  `tests/test_dump.c` (seen red with each mutant); `:503` is equivalent (line for `equivalent.txt` in the
  report; to be added by m1-repair-tools in its new key format).
- A context of 65536 blocks costs 64.1 CPU seconds to build, 62.3 of them in `fmpz_multi_mod_precompute` and
  `fmpz_multi_CRT_precompute`; the reader of the dump takes 1.3 s. Factor 4 for each doubling of `k`.
- `ADF_DUMP_QCLASS_EXP_MAX` is in `include/adelefeld/dump.h` and conventions 8.4. Only 60 of the 1012
  mutants of `src/dump.c` were run.

**Waits for TJO.** Nothing. TJO ratified on 2026-09-28 (night session): M1-D9 as written (the bound holds for
the form `pieces`, not for `lift`); the wording of M1-D1 and M1-D6; the driver keeps `equal`, `different`,
`undecided` for `compare`, and M1-D1 maps them to the terms of SPEC 4.2. The list "Waits for TJO" of the
section below is settled by this.

**Order of work from here.** The comparison of effort levels (one hour, first thing); m1-invariants alone
(adf-xk4; Sonnet at the level chosen, codex as reviewer); m1-repair-tools alone, overnight (adf-xf4, adf-4lj);
closure check (adf-igt); benchmarks on a quiet machine; milestone S.

## Session 2026-09-28 (evening), 17:25 to 21:05, closed by TJO for a restart

**One line.** Five repair lanes and the review of the dump form are merged; lane m1-repair-dump has its code
done and checked but no finished report and is NOT merged; nothing is running.

**Master** (`origin/master` `c7743e1` plus this commit, pushed): `make check` 39 test programs with gcc, clang,
`SAN=1`; `sh tests/test_driver.sh` (27 cases, also `SAN=1`), `sh tests/test_julia.sh`,
`sh tests/test_exports.sh` pass (20:50).

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make -j2 check 2>&1 | tail -1          # expect: check passed: all 39 test programs
    free -g; ~/Projects/quota-app/target/release/quota

| Lane | State |
|---|---|
| m1-repair-recon (arith R1, R2) | merged `d378c31`; `recon.h` edited by the orchestrator (`9ea33b6`) |
| m1-repair-adele (arith R3, R4, R6) | merged `df32c71`; adf-c43 closed |
| m1-repair-ctx (contexts R1, R2, R6) | merged `2efa43f`; adf-rki closed |
| m1-review-dump (codex) | merged `9d216fd`; adf-8ju closed; no blocker, 4 MAJOR |
| m1-repair-text (text R2 to R4, R9, R10) | merged `705a998`; adf-nhk, adf-b8l, adf-5qq closed |
| m1-repair-driver (surface R7 to R15) | merged; adf-jvi closed; clang failure of `test_dlopen` repaired by the orchestrator (`c7743e1`) |
| m1-repair-dump (adf-tp2) | merged in the late session (section above); what follows about it below is history |
| m1-repair-tools (adf-xf4, adf-4lj) | paused as before (WIP commit on its branch); run it ALONE, overnight |
| m1-invariants (adf-xk4) | brief on master; runs alone, after m1-repair-dump has landed |
| closure check (adf-igt) | rules and table of judges in `lanes/m1-closure/COMMON.md` |

**To land m1-repair-dump** (the next step; about 30 minutes of the orchestrator, no lane needed).
The pi model ended two sessions without finishing `report.md`: sections 1 to 3 are written (the stage of the
cap; the bound applies to the form `pieces`, not to `lift`; the bytes of a field token), section 4 is an empty
heading, and the first paragraph promises checks the file does not contain. The checks were run and their
logs are in `lanes/m1-repair-dump/` of the worktree:
- `check-gcc.log`, `check-san.log`, `check-clang.log`: 39 test programs each. The orchestrator ran
  `make clean && make -j2 check` itself at 20:18: 39 pass.
- `fuzz.log`, `fuzz2.log`: 748916 and 822202 runs in 121 s, no crash. `valgrind-seed2.log`: 0 errors.
- `mutate.log`: 60 mutants, 47 killed, 3 survived (`src/dump.c:938:30`, `:503:30`, `:883:28`), 6 not
  compiled, 4 timed out at 120 s (`:1026:20`, `:656:12`, `:1008:13`, `:358:16`). None is judged yet. A
  mutant that times out may be a loop without end on hostile text: look at each.
- Run by the orchestrator against the repaired library: the reviewer's `status_findings.py` (R4: both
  inputs `UNSUPPORTED`; R2: `LIMIT` before `DOMAIN`, as M1-D9 says; the script still expects the old
  contract there) and `cost.py 65537` (`UNSUPPORTED` in 0.013 s; it was `OK` after 65 CPU seconds).
Still to do: write the rest of the report from the logs (as the orchestrator, and say so in it); item 5 of
the brief (where the 60 CPU seconds of a valid 1 MB dump go) has no recorded numbers; judge the 3 survivors
and 4 timeouts; put `ADF_DUMP_QCLASS_EXP_MAX` into `include/adelefeld/dump.h` and conventions 8.4 and remove
the `#ifndef` in `src/dump.c`; delete `lanes/m1-repair-dump/checks/red/libadelefeld.a` from the branch;
`make check SAN=1` and plain in the worktree; merge; all checks on master (also `CC=clang`, and the three
scripts); close adf-tp2.

**Waits for TJO.**
- M1-D9 (`docs/SPEC.md` section 15), PROPOSED: the bound `2^20` on the binary exponents of a `qclass` piece
  in the dump form is kept, named, documented, applied at stage 4. The lane applies it to the form `pieces`
  only (a `lift` forms no range; reference vectors hold lifts above the bound): the row should say so.
- The wording of M1-D1 and M1-D6 was changed by the orchestrator to agree with `text.h` (the exponent is the
  one FLINT stores, so `2^99999` is the largest power of two printed; a zero is exempt; `prec` of the driver
  is 1 to `ADF_PRINT_EXP_MAX`). The reviewer `surface` read the bound the other way.
- The driver prints `equal`, `different`, `undecided` for `compare`; SPEC 4.2 says "certainly equal".

**Left open by the landed lanes** (none blocks; adf-xf4, adf-xrt, adf-4lj):
- Survivors of mutation: 10 in `src/recon.c`, 13 in `src/adele.c` (reasons in the lane reports). Only 40
  mutants per file were run for `src/scaled.c`, `src/modctx.c`. The run of `src/text.c` (911 mutants, limit
  300) was killed by the tool's timeout of 40 minutes and printed no survivors: `mutate.py` should print a
  survivor when it finds it. The tool cannot mutate `tools/adf/adf.c`.
- `tools/mutate/equivalent.txt` lines 52 to 65 (`src/adele.c`): stale lines, and the reason "same value" is
  false for `arb_mul`, `acb_mul`. Also stale for `src/scaled.c` (447, 547 are now 508, 540).
- `tests/ref/vectors/m1-adele/set_rat.jsonl`: one record edited by hand (M1-D4); the generator
  `lanes/m1-adele/gen_adele_vectors.py` still writes the old answer. Do not regenerate before it is changed.
- `tests/test_modctx_limits.c` runs its three slow cases (about 5 minutes) only with
  `ADF_MODCTX_LIMITS_FULL=1` (`lanes/m1-repair-ctx/run_limits.sh`).
- `make check` runs neither the driver tests nor `test_exports.sh` (adf-4lj). A lane brief must ask for the
  clang build: the driver lane's did not, and the failure reached master's checks.
- Sources pending: the code of `arb_get_interval_fmpz_2exp`; the documentation of `n_root` and of the
  `sign` argument of `fmpz_multi_CRT_precomp`.
- HEADER-FINDING of m1-repair-ctx: for `k` above the cap `adf_modctx_new_blocks` returns `UNSUPPORTED` before
  the per-element `DOMAIN` checks; the header's "DOMAIN first" needs a clause.

**About the lanes on pi.** A runner counts a lane as done when `report.md` exists, whatever it holds: read
the end of the report before believing `DONE`. `space-bunny-alpha` twice returned after 18 to 90 seconds
on its first attempt and worked on the second. The time limit of a lane is 90 minutes (`LANE_TIMEOUT`).

**The clock.** The WIP commits of the afternoon pause carry the time 20:20 and the system clock read 17:27
when this session began (synchronised). Times of the afternoon in logs and in the section below are about
three hours ahead.

**Order of work from here.** Land m1-repair-dump; m1-invariants alone; m1-repair-tools alone (overnight);
closure check; benchmarks on a quiet machine; milestone S. The rules of the section below still bind.

## Session 2026-09-28 (day), paused at 16:10 by TJO (network down)

**One line.** Milestone 1 is implemented (193 of 193 functions) and reviewed; the review found defects;
six lanes of repair and review are paused with their work saved on branches; nothing is running.

**Master** (`origin/master`, everything pushed): `make check` 34 test programs, with gcc, clang, `SAN=1`;
`sh tests/test_driver.sh`, `sh tests/test_julia.sh`, `sh tests/test_exports.sh` pass. Run `make clean`
between a `SAN=1` build and a plain one (stale objects give link errors).

**First commands of the next session.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make -j2 check 2>&1 | tail -1          # expect: check passed: all 34 test programs
    git worktree list; git branch -r                      # the six lane branches below
    free -g; ~/Projects/quota-app/target/release/quota    # memory and quota before launching anything

**The six paused lanes.** Each has a worktree `../adelefeld-wt/<lane>/` on branch `lane/<lane>` (pushed to
origin), with ONE commit "WIP <lane>" on top of its base. The work is unfinished, unreviewed, unmerged. None
has a `report.md`. The briefs (`lanes/<lane>/brief.md` in the worktree) end with a resume note that tells the
lane to continue from its files.

| Lane | Task | Model used | What the WIP commit holds | Resume with |
|---|---|---|---|---|
| m1-repair-recon | review arith R1, R2: wrong result and abort of `adf_adele_reconstruct` for extreme exponents (M1-D3) | pi space-bunny-alpha | `src/recon.c` repaired, red and green logs, a mutation run with 10 survivors of 89 to deal with; `make check` passed at 15:35 | `tools/orch/wt_lane.sh m1-repair-recon pi openrouter/stealth/space-bunny-alpha high` |
| m1-repair-ctx | review contexts R1 (wrong `lost` with `y = x`), R2 (primorial, M1-D5), R6 | pi deepseek (OpenRouter) | 25 minutes of work, 8 files; state not checked | `tools/orch/wt_lane.sh m1-repair-ctx pi "openrouter/~deepseek/deepseek-flash-latest" high` |
| m1-repair-adele | review arith R3, R4, R6 (M1-D4) | pi deepseek (OpenRouter) | `src/adele.c`, comments in `rat.c`, `fball.c`, `tests/test_adele_lowprec.c`; one vector record changed (accepted, see its brief); `make check` passed | same command with `m1-repair-adele` |
| m1-repair-text | review text R2, R3, R4, R9, R10 (M1-D6, M1-D7) | pi deepseek (OpenRouter) | `src/text.c`, `proto/text_grammar.py`, fuzz corpus prefixed (575 files); `make check` passed | same command with `m1-repair-text` |
| m1-repair-tools | review surface R1 to R6; keys of `equivalent.txt` without line numbers; `--san`; then the mutation sweep over all of `src/` | pi space-bunny-alpha | `mutate.py`, `selftest.py`, `equivalent.new.txt`; `make check` passed | `LANE_TIMEOUT=14400 tools/orch/wt_lane.sh m1-repair-tools pi openrouter/stealth/space-bunny-alpha high`; run it ALONE |
| m1-review-dump | review of the dump form (adf-8ju) | codex gpt-6-astra xhigh | 20 minutes of reading and reproducers, no review yet | `MAXRETRY=3 tools/orch/wt_lane.sh m1-review-dump codex gpt-6-astra xhigh` (no `session.id` was saved, so the runner starts the review again; its reproducers so far are in the WIP commit) |

The runners `tools/orch/*.sh` refuse to start while `lanes/STOP` exists in the worktree; none exists now.
Large files `lanes/<lane>/events.jsonl` are ignored by git and are not on the branches.

**How to land a lane** (the orchestrator does this, never the lane): read `lanes/<lane>/report.md`; rerun
`make clean && make -j2 check SAN=1` and `make clean && make -j2 check` in the worktree; read the diff of
anything the lane did not own; `git add -A && git commit` in the worktree (delete binaries first);
`git merge --no-ff lane/<lane>` on master; run all checks on master; `bd close`; push. Expect conflicts in
`tools/mutate/equivalent.txt` (take the lines out of the lane commit and add them after the merge) and in
`src/fball.c` comments between m1-repair-adele and later lanes.

**Rules of this session that bind the next** (memory: `orchestration-model-tiers`, `claude-worktree-base`):
- At most two or three lanes at once, never two mutation runs at once: at 15:30 the harness killed five
  lanes because the laptop was low on memory.
- Cheapest model that can do it. pi models for repairs (free or cents). Author and reviewer of different
  model families. Claude subagents only while the quota app shows the weekly window behind pace (16:00: weekly
  81%, 3 points behind; Fable weekly 75%, 9 behind; both reset 2026-09-29 18:00). Codex: TJO authorised up
  to 48% of the weekly window (meter 32% at 15:25).
- Claude subagents with worktree isolation start from the PUSHED commit: push first, paste the whole task
  into the prompt, save their report from the final message (they cannot write `report.md`).
- Never `pkill -f <pattern>`, and never `kill` from a command line that contains the pattern: it kills the
  calling shell. Use `ps` first, then `kill <pid>` in a second command.

**Decisions of 2026-09-28, all accepted by TJO** (`docs/SPEC.md` section 15): M1-D1 language of the driver;
M1-D2 what `is_canonical` promises about pointers; M1-D3 `ADF_LIMIT` in `adf_adele_reconstruct`
(`ADF_RECON_EXP_MAX = 2^20`); M1-D4 `prec` below 2 is 2, exact integers for rational factors; M1-D5 a
context has at most 65536 blocks; M1-D6 a printer may return NULL (`ADF_PRINT_EXP_MAX = 100000`); M1-D7 no
hidden bound on decimal exponents; M1-D8 milestone S comes directly after milestone 1. The headers carry
M1-D2 to M1-D6 already; the code follows when the repair lanes land.

**The review of milestone 1** (`docs/reviews/m1/<name>/review.md`, rules `lanes/m1-review/COMMON.md`):

| Reviewer | Model | Findings | State |
|---|---|---|---|
| local | codex gpt-6-astra | 1 BLOCKER, 2 MAJOR, 1 MINOR | R2 settled by M1-D2; R1, R4 merged (lane m1-repair-cap); R3 open: adf-xk4 |
| arith | Claude opus | 1 BLOCKER, 2 MAJOR, 3 MINOR | lanes m1-repair-recon, m1-repair-adele (paused) |
| contexts | Claude opus | 1 BLOCKER, 2 MAJOR, 3 MINOR | lane m1-repair-ctx (paused); R3, R4 done by lane m1-dump |
| text | codex gpt-6-astra | 1 BLOCKER, 6 MAJOR, 3 MINOR | R1 by M1-D2; R5 by m1-dump; R6 merged; R7 is adf-xk4; R8 by the header edit; lane m1-repair-text (paused) |
| surface | Claude opus | 0 BLOCKER, 3 MAJOR, 12 MINOR | lane m1-repair-tools (paused); lane m1-repair-driver: brief committed, never started |
| dump | codex gpt-6-astra | none yet | lane m1-review-dump (paused) |

No reviewer found a wrong enclosure in the tight arithmetic, the local backend, the scaled arithmetic or the
reader of the value form (about 600000 cases with their own oracles).

**Order of work to close milestone 1.**
1. Resume m1-repair-recon and m1-repair-ctx (the two blockers); land them.
2. Resume m1-repair-adele and m1-repair-text; land them. Resume m1-review-dump next to them (codex is light).
3. Start m1-repair-driver (`tools/orch/wt_lane.sh m1-repair-driver pi openrouter/stealth/space-bunny-alpha
   high`; its brief is on master) after m1-repair-text has landed (it uses the printer's new rule).
4. `ADF_CHECK_INVARIANTS` (adf-xk4): write the brief; it touches every file of `src/`, so it runs alone after
   the repairs have landed; add a way to run the suite with the flag.
5. Repairs of what the dump review finds.
6. m1-repair-tools alone (best overnight): new key format, then the mutation sweep over all of `src/`.
   `tools/mutate/equivalent.txt` on master is stale for `src/fball.c` and `src/scaled.c`; entries proposed by
   lanes wait in `lanes/m1-local/`, `lanes/m1-scaled/`, `lanes/m1-dump/` (`equivalent-added.txt`), in
   `lanes/m1-repair-cap/report.md` (three lines) and in `lanes/m1-modctx-b/report.md` (two of its ten are
   out-of-bounds reads, not equivalent).
7. Closure check (adf-igt): each reviewer re-judges its findings against the repaired code; codex for the
   code written by Claude (text, local, cap, dump), Claude opus for the rest. Briefs to be written on the
   pattern of `docs/reviews/m0-gate/closure.md`.
8. Benchmarks on a quiet machine (`make bench`, nothing else running); then mark milestone 1 done in
   `docs/PLAN.md` section 6 and update the numbers there.
9. Then milestone S (M1-D8): briefs for S.3 (partial rational reconstruction) and S.1 (systems modulo `N`,
   Hermite form with transformation).

**Open issues** are in beads (`bd ready`, `bd list --status=in_progress`). In progress at the pause:
adf-c43 (repairs arith), adf-rki (repairs contexts), adf-8ju (dump review), adf-gf1 (review round 1).

**Things to know.**
- `deepseek/deepseek-flash` at its own provider has no credit; use `openrouter/~deepseek/deepseek-flash-latest`.
  `openrouter/xiaomi/mimo-v2.6-pro` was slow twice (no report after two hours).
- Valgrind 3.22: `~/.local/bin/valgrind` (unpacked into `~/.local/opt/valgrind`, no root).
- Julia's bundled libgmp lacks a symbol that the system FLINT needs; `tests/test_julia.sh` uses `LD_PRELOAD`.
- TJO's `~/.local/bin/codex-vision` was patched (one argument `errors="replace"`).
- Tate's thesis: `refs/src/tate-thesis/` (not in git): two scans, TeX of 43 pages refereed twice, a README with
  the state. Rule in `docs/sources.md`: the scan is the ground truth, the TeX is an aid. One edit is owed:
  p331, the product for |d| runs over p NOT in S_infty (read in the second scan, recorded in the file, the
  formula not yet changed).
- Old worktrees of finished lanes under `../adelefeld-wt/` and `.claude/worktrees/` can be removed with
  `git worktree remove` when disk space matters; their branches are merged.

## Session 2026-09-27 (evening) to 2026-09-28 (morning): orchestration set up, first wave of milestone 0 launched

**Mode of work** (TJO, 2026-09-27): wide, fast, rigorous; an orchestrator (Claude Fable) dispatches lanes and is
the only one who commits. Model tiers: pi `openrouter/stealth/space-bunny-alpha` (generic code), pi
`deepseek/deepseek-flash` and `openrouter/xiaomi/mimo-v2.6-pro` (more demanding), codex `gpt-6-luna` / Claude
sonnet (routine), codex `gpt-6-sol` xhigh / Claude opus (hard), codex `gpt-6-astra` xhigh (critical derivations and
reviews). Budgets: Claude while the quota app shows the weekly window behind pace
(`~/Projects/quota-app/target/release/quota`); codex up to 30 points of the weekly window (meter was 8% at the
start). Code review rounds only after major features land. Author and reviewer of a proof or of code come from
different model families.

**Mechanics.** `lanes/COMMON.md` holds the rules for every lane (disjoint file ownership, no git, no `bd`, report
in `lanes/<lane>/report.md`). `tools/orch/codex_lane.sh <lane> <model> [effort]` and
`tools/orch/pi_lane.sh <lane> <provider/model> [thinking]` run a lane from `lanes/<lane>/brief.md`, with resume.
`touch lanes/STOP` stops new attempts. Tracker: `bd ready`.

**State (2026-09-28 07:15), session closed by TJO.** Milestone 0 is complete (gate passed after edits, applied).
Milestone 1: public headers (`include/adelefeld/`, `docs/api-m1.md`; not yet reviewed), test infrastructure
(`tests/support/`, `make fuzz`, `make mutate`), and on master the first library code: status strings, places,
exact rationals (`src/status.c`, `place.c`, `rat.c`, `inlines.c`) and finite balls with tight arithmetic in the
global backend (`src/fball.c`), with `bench/bench_fball.c`. `make check` and `make check SAN=1`: 9 test programs
pass. 78 of the 193 declared functions are implemented. No lane is running. Everything is pushed to origin.

**Next.**
1. Lane m1-modctx failed without output (`pi` with `openrouter/xiaomi/mimo-v2.6-pro` returned at once, three
   times). Check the provider's credit with a one-line prompt, then run the lane again, with another model if
   needed: `tools/orch/wt_lane.sh m1-modctx pi <model>`.
2. Wave B, each lane in its own worktree: adeles and complex adeles (1.3), value text with a fuzz target (1.4),
   reconstruction (1.6), scaled policy and cap (1.7), local backend (1.8, after modctx), dump; the remaining
   functions of `common.h` (`adf_str_free`, version check). Briefs are still to be written; the pattern is
   `lanes/m1-fball/brief.md` with `lanes/COMMON-C.md`.
3. Wave C: driver `adf` (1.5), Julia interface check (1.9), benchmark rows on a quiet machine.
4. Then the first code review round (codex `gpt-6-astra`; task: an input that breaks enclosure). It also
   reviews the headers, the 15 choices of `docs/api-m1.md`, and the closure edits E3, C3, C4.

**Open items.**
- Done (see below): the repeat of `make mutate FILES=src/fball.c` with the three excused mutants.
- `ADF_CHECK_INVARIANTS` (conventions 4.4) is promised and not implemented (finding of lane m1-rat).
- `fball.h` documents local-backend behaviour that `src/fball.c` does not implement (HEADER-FINDING 1 of lane
  m1-fball); it belongs to 1.8.
- `lanes/m0-conventions/write_golden.py` does not reproduce `tests/golden/`: do not run it.
- A quiet-machine benchmark run; `ideles.md` Definition 8 against CV-16; four sources pending
  (`lanes/m0-sources/report.md`).
- When committing in a lane's worktree, never build into a directory that git does not ignore (`build-*/` is
  ignored now).

**Machine and quota.** The laptop ran out of memory once (02:39, swap full); build with `make -j2`. After that
the harness stopped the orchestrator's waiting loop, so lanes on pi or codex end without a signal: look at
`lanes/<lane>/lane.log` in the worktree. Quota at 07:10: Claude weekly 69%, Fable weekly 67% (both reset
2026-09-29 18:00), codex 19% of the week (11 of the 30 points TJO authorised are used; resets 2026-10-03).

**Mutation tool fixed (2026-09-28, later session; issue adf-98j closed).** `run_make` of
`tools/mutate/mutate.py` starts each run in a session of its own and kills the whole process group on timeout.
`make mutate-selftest` now also fails if a process of a mutant is alive after the runs (seen failing before the
fix, with two processes left). `make mutate` may be run again. `make mutate FILES=src/fball.c` was run to the
end: 184 mutants, 146 killed, 0 survived, 35 not compiled, 0 timed out, 3 excused; no process left. The run
took 4030 s of wall time and not 320 s; a `cargo build` of another project ran on the machine at the end (load
average 16), the rest of the cause was not examined. The note below is kept for the record.

**Found after the session (2026-09-28 10:45).** `tools/mutate/mutate.py` does not kill the test program of a
mutant that times out: nine such processes ran for eight hours (load average 10) until killed by hand. Do not
run `make mutate` before this is fixed (issue in beads, priority 1). The full swap file is not a fault: 2 GB of
swap hold pages of long-running desktop programs after five days of uptime, with 23 GB of memory available.
The OpenRouter credit is used up (TJO): the models `space-bunny-alpha` (free) may still work, `mimo-v2.6-pro`
through OpenRouter does not; `deepseek-flash` goes through its own provider.
