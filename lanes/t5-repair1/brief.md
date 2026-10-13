# Lane t5-repair1: the test gaps left by slices 5a to 5d

Slices 5a to 5d of milestone 5 (`docs/api-5.md`) are merged (reports `lanes/t5-slice1/report.md`,
`lanes/t5-slice2/result.md`, `lanes/t5-slice3/result.md`, `lanes/t5-slice4/result.md`). Their mutation runs left
survivors that the lanes call test gaps, not equivalents, and some sections were not run under valgrind. You
close the gaps with TESTS (and generator vectors where a test needs them). You do not change `src/` or
`include/`: a survivor that turns out to be a real defect is a finding in your report with the smallest input,
not a patch. No new API.

The gaps, in order of importance (line numbers are those the reports quote; re-find them in the current
files, the merges moved nothing inside these files but check):

1. `src/tate.c`, the second branch of Lemma 14 in `tt_jbound` (`docs/proofs/analysis.md` 627-651, design
   `docs/api-5.md` section 4: "J_bound is BOTH branches of L14: if `b >= 2 max(r, 0)/R` use `2 R^r exp(-bR)/b`;
   otherwise `r > 0`, `R0 = 2r/b`, `t* = min(max(r/b, R), R0)`, and use `(R0 - R) t*^r exp(-b t*) + 2 R0^r
   exp(-bR0)/b`"). Lane t5-slice4's survivor 247 (`exp(-(R0 - R))` for `exp(-b R0)`) and lane t5-slice2's
   survivor 551 passed all tests: no test reaches the second branch with a case where the value matters. Write a
   test that evaluates `tt_jbound` through the existing hidden hooks (`adf_tate_cutoffs`, `adf_tate_trace`:
   read `src/tate.c` for what they expose; if neither exposes the bound, the test must reach the branch through
   `adf_tate_continue` at points where `r = Re(z) - 1 > 0` is large against `b = pi/C` and `R` is small, so
   that the second branch is the one taken, and compare the cutoffs `N`, `R` and the bound added to the radius
   with your own evaluation of L14 in Python (mpmath) in a vector file): the mutated bound must change a
   recorded cutoff or radius bound. Prove in a comment, from L14's statement, that the case you chose is in the
   second branch. Then replant survivor 247 and 551 in scratch copies and show the new test fails.
2. `src/tate.c`, the work charging: survivor 161 of t5-slice4 (`tt_series` charges 0 instead of 1 per Lemma 6
   step). Add a test through `adf_tate_cutoffs` (it "returns the work charged", `docs/api-5b.md` decision 7)
   that pins the exact work count of a small case against the count rule of `docs/api-5b.md` decision 5
   computed by hand in a comment. The mutant must change the count.
3. `src/localfactor.c`, the inherited Gamma certificate (`real_derivative_bound`, `real_factor`, about lines
   200-260 and 330-370; design `docs/design/local-zeta.md` Z1 to Z9; statements Y16, Y17 of `docs/api-1f9.md`):
   lanes t5-slice1 and t5-slice3 report the same survivor class (`:217, :221, :222, :223, :237, :240, :244,
   :248, :250, :252, :255, :344, :367`). For each: read the mutant (regenerate it with `tools/mutate/mutate.py
   --files src/localfactor.c --limit 60 --seed 53001` and the seed of t5-slice1, `--dry-run` or its listing
   option: read `tools/mutate/README.md`), decide whether it weakens the bound (then a test where the weakened
   bound gives a result that is not an enclosure or is wider than Y16 promises, with your own mpmath Gamma as
   the oracle) or is equivalent (one sentence a reader checks in the code at once, in your report; NOT in
   `tools/mutate/equivalent.txt` unless the sentence meets `lanes/COMMON-C.md` rule 5). A bound that can be
   weakened without any test noticing is a MAJOR finding against the tests of `test_localfactor`, even if the
   code is right.
4. valgrind of the `test_tate_continue` sections `values`, `widths`, `statuses` (t5-slice4 did not run them:
   each over 180 s): run each under `timeout 900 valgrind --error-exitcode=9 --track-origins=yes
   --leak-check=full --errors-for-leak-kinds=definite,indirect` on an `-O1 -g` build (note the lane's tool
   finding: `tools/memcheck/run.sh` fails at `-O1 -Werror` on `src/lpow.c:259` and `src/rfun.c:339,424` with
   this gcc: build with `-Wno-error=maybe-uninitialized -Wno-error=stringop-overread` and REPORT those three
   warnings with the gcc version as a finding; do not change the files). Also valgrind `test_tate_gamma` in
   full. Report the exit codes and the error counts.
5. The Julia fixture of t5-slice3 was never seen red (`tests/julia/tate_gamma.jl`): make it fail once by
   pointing it at a wrong expected value in a scratch copy, record the failure, restore.

Read first: `lanes/COMMON.md`, `lanes/COMMON-C.md`; the four reports above (their "Mutation" and "Not done"
sections); `tools/mutate/README.md`; `docs/api-5b.md` (decisions 5, 7, the hooks; "Slice 5d" S9 to S14);
`docs/proofs/analysis.md` Lemma 14 (627-651), Lemma 6 (213-259); `docs/api-1f9.md` Y16, Y17;
`docs/design/local-zeta.md`; `src/tate.c`, `src/localfactor.c` (read-only), `tests/test_tate.c`,
`tests/test_tate_continue.c`, `tests/test_tate_gamma.c`, `tests/test_localfactor.c`, `tests/test_tate_local.c`.

**You own:** `tests/test_tate.c`, `tests/test_tate_continue.c`, `tests/test_tate_gamma.c`,
`tests/test_tate_local.c`, `tests/test_localfactor.c` (append tests; do not weaken or remove any),
`tests/ref/vectors/t5-repair1/`, `lanes/t5-repair1/`. Everything else is read-only. No git command that
changes state, no `bd`. Lane t5-slice5 writes `tools/adf/adf.c`, `tests/test_tate_fe.c`, `docs/api-5b.md` and
the Julia files in another worktree: you do not touch those.

**Checks at the end** (commands and numbers; at most 2 jobs; every program under `timeout`; valgrind runs
excepted from the 3-minute rule but under `timeout 900` each): every test program you changed under plain,
`SAN=1` with leak detection, `INV=1`, `CC=clang`, in build directories under your lane directory; the fault
replants of items 1 and 2 (the scratch copies removed afterwards); NOT `check-all`. No generated log above
100 KB in the lane directory. Lines at most 116 characters; files end with a newline. Aim to finish within
about ninety minutes, items 1 to 3 first.

Report: `lanes/t5-repair1/result.md` (a Claude subagent cannot write `report.md`; give the same text as your
final message): for each item what was added, the commands and counts, which mutants now die; the
equivalence sentences; the valgrind table; findings (MAJOR test gaps, the gcc warnings, any defect of the
code with its smallest input); what is not done.
