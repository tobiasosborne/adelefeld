# Rules for every lane of adelefeld (read first, they bind you)

You are one of several agents working in parallel in the same working tree of the repository `adelefeld`
(adeles as a number type: C on FLINT 3.0.1, ball arithmetic). Read `CLAUDE.md`, then the parts of `docs/SPEC.md`
and `docs/PLAN.md` your brief names. `docs/SPEC.md` is the specification; you implement or prove it, you do not
change it.

1. **File ownership.** You may create and change only the paths listed under "You own" in your brief, and files in
   your lane directory. Every other file is read-only for you. Other agents are writing elsewhere in the tree at the
   same time; never delete, move or reformat files you do not own.
2. **No git and no tracker.** Do not run any git command that changes state (add, commit, checkout, stash, reset,
   clean, push) and do not run `bd`. The orchestrator commits.
3. **Laptop.** Use at most 2 cores. No computation longer than about 3 minutes. No installation of system packages;
   Python: standard library, `python-flint`, `mpmath`, `sympy` only if already importable.
4. **Ground truth.** A formula or convention of other people is quoted from a file under `refs/` with file and line.
   If the source is not on disk, do not cite from memory as if it were: mark the statement
   `[source pending: <what is needed>]` and list it in your report. Your own proofs are written out stepwise and
   need no source.
5. **Honesty.** Never weaken a test or a statement to make it pass. If something in `docs/SPEC.md` is wrong or
   cannot be proved as stated, do not paper over it: write the counterexample and put it under
   "Findings against the specification" in your report.
6. **Style of prose.** Plain, sober English; short sentences; no marketing words. Lines at most 116 characters.
   Mathematics in plain text as in `docs/SPEC.md` (`Zhat`, `v_p`, `a + N Zhat`), no LaTeX in Markdown.
7. **Write incrementally**, file by file, so that an interruption loses little.
8. **Report.** Finish by writing `report.md` in your lane directory: what was done; the files written; every check
   that was run, with its command and its result (numbers, not adjectives); what is not done; sources pending;
   findings against the specification. The report is the only thing the orchestrator reads first, so it must be
   true and complete. Your work counts as finished only when this file exists.
