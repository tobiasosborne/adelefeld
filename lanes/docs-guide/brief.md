# Lane docs-guide (Claude Opus): the user guide, docs/GUIDE.md, with runnable examples

Read `lanes/COMMON.md`, `lanes/DOCS-COMMON.md`, then `tools/adf/README.md` (the driver manual, whole),
`docs/SPEC.md` sections 1 to 4 and 9, `docs/conventions.md` sections on status codes and text grammar,
`docs/PLAN.md` section 6, `HANDOFF.md` (top entry), the public headers `include/adelefeld/*.h` (each header's
top comment is the contract of its functions), and `tests/driver/*.cmd` with their `.out` (86 fixtures: the
best catalogue of what the driver does and prints).

## Goal

`docs/GUIDE.md`: the one document a BSc reader works through in an afternoon, from "what is a place" to
computing a Tate integral, with every example run and its output pasted (D-DOC3). The guide is to the
library what a textbook chapter is to a theorem: concepts first, then the thing in action, then where the
contract lives.

## Chapters (adjust with a reason in the report)

1. **Places and the adeles in one afternoon.** The absolute values of Q (Ostrowski; cite `refs/src/` by
   file and line if a source is on disk, else `[source pending]`), Z_p as digits, Zhat as "all primes at
   once", the adeles as pairs (real ball ; finite ball), why `22/7` is a thread and `146 + 1000 Zhat` a
   tube. Worked numbers only, computed with the driver.
2. **Enclosures and statuses.** Ball arithmetic; the status codes and their four classes (from
   `docs/ux/README.md` and `docs/conventions.md`); what `NOT_DETERMINED` means versus `DOMAIN`; a precision
   argument (`prec`) and what `O(p^k)` says. Each illustrated by one driver line and its output.
3. **The driver, by task.** Short sessions, each with the exact `build/adf` input and output: reading and
   printing values; arithmetic on rationals, finite balls, adeles; local balls at one prime and digits;
   functions (`exp`, `log`, `sin`, roots and branches with seeds); symbols (`legendre`, `jacobi`,
   `kronecker`, `hilbert_at`); characters and the quotient (milestone 3 commands); functions on the
   adeles (milestone 4); Tate integrals (milestone 5); reconstruction and `recover`; dumps and loading.
   Every command group of `tools/adf/README.md` gets at least one example; cite the README line of each
   command.
4. **The C API in one program.** `docs/examples/hello.c`: builds a finite ball and an adele, adds them,
   takes a root at a prime, prints the status and value with the text functions, frees everything. With
   the compile line (`cc hello.c -I include -L build -ladelefeld -lflint ...`; derive it from the
   Makefile), run it, paste the output. Also `docs/examples/README.md` with the compile and run lines.
5. **The Julia package in one page**: a pointer to `julia/Adelefeld/README.md` (lane docs-julia; link it
   even though it does not exist in your worktree) with the launcher command and three lines of example.
6. **The four faces** (`docs/ux/README.md` and `ux/*/README.md`): what each is for, how to open it, one
   paragraph each.
7. **Reading the contracts**: how a header's top comment, `docs/api-*.md` statements, `docs/proofs/` and
   `tests/driver/` fit together; how to find the proof behind an answer; how to report a wrong enclosure.
8. **Glossary**: every term of chapters 1 and 2, one line each.

## You own

`docs/GUIDE.md`, `docs/examples/**`, `lanes/docs-guide/**`.

## Checks you run and report

- Every driver session in the guide is in `lanes/docs-guide/numbers.md` as (input, output) and was run in
  your worktree on your build; a script `lanes/docs-guide/check_examples.sh` reruns every session and diffs
  its output against the guide's fenced block (report: N sessions, M differ; M must be 0).
- `docs/examples/hello.c` compiles with `-Wall -Wextra -Werror` and runs; its output is in the guide.
- The link check of D-DOC7.
- Every `tools/adf/README.md:LINE` citation points at the command it names (check 10 at random; report).

## Report

`lanes/docs-guide/result.md` per COMMON.md rule 8: files, checks with numbers, what is not done, source
pending, library demands (what the guide wanted to show and could not), findings against the specification.
