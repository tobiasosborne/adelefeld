# Lane docs-julia (Claude Opus): the Julia package README and the docs index

Read `lanes/COMMON.md`, `lanes/DOCS-COMMON.md`, then `docs/api-j.md`, `docs/api-ja.md`,
`julia/Adelefeld/src/*.jl` (the docstrings are the contract), `julia/Adelefeld/test/runtests.jl` (what is
exercised), `ux/notebook/README.md`, `HANDOFF.md` (top entry), and the `docs/` listing.

## Goal

Two pages.

**A. `julia/Adelefeld/README.md`**: the package's front page and tutorial. Sections: what the package is
(a thin, safe binding of the C library: handles, finalizers, `try_` results, D-DOC1 explanations); the
launcher and why (`bin/adelefeld-julia`, the libgmp preload; the env of `ux/notebook/julia.sh`; offline, one
thread); a REPL tutorial in eight steps with real outputs (D-DOC3): rationals; finite balls and the set
predicates; adeles, real balls and `parse`; reconstruction (the progression contract of
`include/adelefeld/recon.h`: cite lines); statuses and `AdfResult`/`unwrap`/`AdfError`; text and dumps
round trip; `ModCtx` (five construction families, `to_local`, `enclose_local`, `to_global`, `is_local`);
housekeeping (`close`, `diagnostics`). Then an API table (every exported name, one line, the docstring's
first sentence); what the package does not do yet (local balls, digits, functions, roots: see `docs/api-j.md`
section 9 for the order they arrive in); how to run the tests (`Pkg.test` through the launcher; the count,
run it: 5369 expected); the notebook slice as the way to use the driver from Julia today.

The orchestrator verified these lines today (rerun them, do not paste from here):
`Rat(6, 35) + Rat(1, 7)` prints `11/35`; `FBall(Rat(146), Rat(1000))` prints `(* ; 146 mod 1000)`;
`reconstruct(parse(Adele, "(1.5 +/- 0.5 ; 0 mod 2)"))` prints `2`; `ModCtx(factorial = 6)` prints
`ModCtx(modulus=720, blocks=[16, 9, 5])`; `enclose_local(readvalue("1 mod 5"), ModCtx(blocks = [8, 9]))`
prints `(* ; 0 mod 1)`.

**B. `docs/README.md`**: the index of `docs/`. A reader-facing map, not a file listing: start here
(`GUIDE.md`, being written by lane docs-guide: link it), the specification and plan, the conventions, the
API statement files `api-*.md` by milestone with one line each on what they cover (read each file's first
paragraph), the proofs directory by topic, the reviews, the design notes, the UX visions, the worklog and
HANDOFF, PERF, sources, workflow. Use a table per group.

## You own

`julia/Adelefeld/README.md`, `docs/README.md`, `lanes/docs-julia/**`.

## Checks you run and report

- Every Julia output in the README is in `lanes/docs-julia/numbers.md` and produced by a script
  `lanes/docs-julia/tutorial.jl` that the report runs through the launcher (paste the command and the
  full output); a second script diffs the fenced outputs in the README against that output (N blocks, M
  differ, M = 0).
- The exported-names table covers every name of the `export` lines of `julia/Adelefeld/src/Adelefeld.jl`
  (count both; report).
- `Pkg.test` once, under `timeout 900`, with the depot env of DOCS-COMMON (the count of tests passed).
- The link check of D-DOC7 for both pages.

## Report

`lanes/docs-julia/result.md` per COMMON.md rule 8.
