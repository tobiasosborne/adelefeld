# Lane docs-guide: report

Claude Opus, worktree `/home/tobias/Projects/adelefeld-wt/docs-guide` (branch `lane/docs-guide`), 2026-10-16,
about 20:40 to 21:05 AEDT. No git command that changes state, no `bd`, at most 2 cores, every run under `timeout`.
No C, Julia or test source was changed.

## What was done

`docs/GUIDE.md` (1286 lines): the guide for a BSc reader, in the eight chapters of the brief plus a chapter 0
("Before you start": build, the session form). Every term of chapters 1 and 2 is explained at first use with a
number. 40 driver sessions (204 input lines, 85 distinct driver commands), the full listing of
`docs/examples/hello.c` with its compile line and output, and one Julia example with its output. Every output was
produced by `build/adf`, `build/hello` or the Julia package in this worktree and written into the guide by the
checker's `--fill` mode, never typed.

1. Places and the adeles: `|.|_p` and the product formula (shown with `abs` on the idele of 22/7 at real, 2, 7,
   11, 13), Ostrowski, `Z_p` as digits (`-1` and `1/3` in `Z_5`), `Zhat` and the CRT view of `146 mod 1000`, the
   adeles as a restricted product, thread (22/7) and tube (146 + 1000 Zhat; `contains 22/7` is `false` because of
   the prime 7), `reconstruct` (146; `NO_SOLUTION` for `(3.14159 +/- 1e-5 ; 146 mod 1000)`) against `recover`
   (22/7; `NOT_UNIQUE` in a larger box), the three facts of SPEC section 2.
2. Enclosures and statuses: ball arithmetic, the outward rounding of 0.1, the tight rules with the SPEC 4.3
   table, the set predicates and `compare`, `prec` at real (bits) and at a prime (`O(p^k)`), the four status
   classes with one driver line for each of nine of the ten non-OK statuses (all but `NEEDS_SPLIT`),
   `NOT_DETERMINED` against `DOMAIN` (sin at 2 on `2`, `2 Z_2`, `4 Z_2`; square roots at 2), the combination rule
   of conventions 3.3.
3. The driver by task, 16 sections, each citing `tools/adf/README.md` by line.
4. `docs/examples/hello.c`: finite ball from the raw triple, adele from `arb` and finite ball, adele from text,
   `adf_adele_add`, `adf_adele_reconstruct` (46), projection to 5, `adf_lball_root_count` (2) and
   `adf_lball_root_seed` (branch 1: `36 + O(5^3)`), a `DOMAIN` refusal, every status and value printed, all freed.
5. Julia: the launcher command, `sh tests/test_exports.sh` for the shared object, three results; the link to
   `julia/Adelefeld/README.md` (lane docs-julia) is made although the file is not in this worktree.
6. The four faces: one paragraph each, with the command that builds or opens it, from `ux/*/README.md`.
7. Reading the contracts: SPEC, conventions, headers and `docs/api-*.md`, `docs/proofs/`; the chain from
   `exp_at (* ; 5 mod 25) with 5` to `functions.md` Propositions 6 and 10; how to report a wrong enclosure.
8. Glossary: 42 terms, one line each.

Adjustments to the chapter plan, with reasons: a chapter 0 for the build and the session notation, because every
later chapter depends on it. Chapter 1 uses `abs` on an idele before ideles are introduced (section 3.9), because
it is the only driver command that prints `|x|_p`; the text says so. The milestone 3 to 5 commands are in
sections 3.12 (quotient, `psi`), 3.13 (characters), 3.14 (test functions, Poisson), 3.15 (Tate).

## Files written

- `docs/GUIDE.md`
- `docs/examples/hello.c`, `docs/examples/README.md` (compile and run lines, the output, the valgrind result)
- `lanes/docs-guide/check_examples.sh` (wrapper) and `lanes/docs-guide/check_examples.py`: reruns every ```adf
  session of the guide (prefix by prefix, so a setting that prints an error is handled without guessing), checks
  the hello.c listing against the file, compiles and runs hello.c and diffs its output (in the guide and in
  `docs/examples/README.md`), runs the Julia block with the launcher and diffs its output. Modes `--fill`
  (used while writing) and `--record numbers.md`.
- `lanes/docs-guide/numbers.md`: every session as its `printf ... | build/adf` command and verbatim transcript,
  hello.c and Julia with their commands and outputs, and the output of `arith_checks.py` (731 lines).
- `lanes/docs-guide/arith_checks.py`: the numbers derived in the prose (base-5 digits 4444 and 313132, `3 * 10417
  = 2 * 5^6 + 1`, `36^2 = 46 + 10 * 125`, the product formula, the 8 idempotents modulo 30, the dump midpoint
  `0x3333333333333333 * 2^-65 = 0.0999999999999999999945...`, pi/6, 1/pi, 4/3), each with an assert.
- `lanes/docs-guide/check_docs.py`: the link check of D-DOC7, line widths, and the list of README citations.
- Logs: `build.log`, `exports.log`, `valgrind.log`. Not for commit: `lanes/docs-guide/depot/` (1.5 MB Julia
  precompile cache, like `lanes/ux-notebook/depot/`).

## Checks, with commands and results

| Check | Command | Result |
|---|---|---|
| Build | `timeout 600 make -j2 && timeout 600 make -C tools/adf` | exit 0; `build/adf`, `build/libadelefeld.a` |
| Shared object | `timeout 500 sh tests/test_exports.sh` | exit 0; 625 of 625 declared functions exported; `build/libadelefeld.so` |
| Sessions | `sh lanes/docs-guide/check_examples.sh` | **40 sessions, 0 differ**; 4 other examples (hello.c listing, hello output in GUIDE.md and in examples/README.md, Julia output), 0 differ; exit 0 |
| Checker catches errors | two output lines and the Julia output changed in a copy (`CHECK_GUIDE=copy python3 lanes/docs-guide/check_examples.py`) | 2 sessions differ, 1 other differs, exit 1, each with the first differing line |
| hello.c, brief's flags | `cc -std=c11 -Wall -Wextra -Werror -Iinclude docs/examples/hello.c build/libadelefeld.a -lflint -lgmp -lm` | compiles, no warning; runs, exit 0; output in the guide |
| hello.c, stricter | same with `-O2 -Wpedantic`; and with `clang -Wall -Wextra -Wpedantic -Werror` | both compile; clang build's output identical |
| hello.c, memory | `timeout 120 valgrind --leak-check=full --error-exitcode=3 ./build/hello` | 0 errors; 4112 allocs, 4112 frees; "All heap blocks were freed" |
| Links (D-DOC7) | `python3 lanes/docs-guide/check_docs.py` | **11 checked, 0 broken**; 1 allowed missing: `../julia/Adelefeld/README.md` (lane docs-julia, as the brief asks) |
| Line width | same | 0 prose lines over 116; 3 lines inside fenced blocks over 116 (verbatim driver lines: the `tensor_eval` and `poisson` inputs with two test-function texts, the `tate_fe` output) |
| Tables | a pipe count per table row (inline script) | 0 rows with a wrong cell count; the `\|` inside code spans of the glossary escaped |
| README citations | `python3 lanes/docs-guide/check_docs.py --citations`, then 10 drawn with `random.seed(20261016)` | 72 ranges listed; the 10 drawn (`:83`, `:963-976`, `:723-770`, `:335-337`, `:826`, `:91-95`, `:72`, `:223`, `:275-289`, `:56-57`) each point at the command named; the 9 added late (`:502`, `:575-577`, `:849`, `:867`, `:995-997`, `:1012-1013`, `:242-243`, `:263-267`, `:422-435`) also checked: all correct |
| Prose numbers | `python3 lanes/docs-guide/arith_checks.py` | all asserts hold |
| Julia | the guide's block, run by the checker with the launcher, offline, one thread, depot in the lane | output equals the guide; about 2.6 s with precompilation |

Coverage of the command groups of `tools/adf/README.md`: every group has at least one session. By section of the
guide: table of operations `:54-89` (sections 0 to 3.9), solvers `:144-212` (3.7, 3.8), places `:214-271` (1.2,
3.3, 3.4), ideles `:346-377` (1.1, 3.9), factorial series `:463-478` (2.5, 3.4), local roots `:480-513` (2.5,
3.5), powers at a prime `:515-545` (3.5), roots at all places `:547-581` and series `:583-608` (3.6), Log
`:610-653` (3.9), symbols `:655-676` (3.10), catalogue `:677-702` (3.11), local zeta factors `:704-721` (3.15),
quotient `:723-770` and set queries `:807-822` (3.12), psi `:772-805` (3.12), characters `:824-842`, `:893-915`
(3.13), rfun and ffun `:844-891`, `:917-961` (3.14), Tate `:963-976`, `:987-1054` (3.15), ffun/rfun dumps
`:978-985` and dumps `:116-142` (3.16). Commands not shown individually inside a covered group: `sinh_at`,
`cosh_at`, `sinh`, `cosh`, `powtight`, `hullsimple`, `logabs`, `log_abs_at`, `log_abs_refine`, `profpow`,
`profpowcoarse`, `cyclo_exp_uinv`, `qneg`, `qequal`, `qcontains`, `psi_strict_at`, `char_unit`,
`char_unit_strict`, `char_eval_strict`, `rfun_translate`, `rfun_mul`, `rfun_derivative`, `rfun_norm2`, `ffun`
algebra beyond `ffun_fourier` and `ffun_integral`, `tensor_eval_sball`, `tensor_integral`, `tensor_norm2`,
`tate_epsilon`, `realroots` beyond two lines.

## What is not done

- No screenshots of the four faces (D-DOC4: headless Firefox crashes here); chapter 6 is text only, and I did not
  run the four `ux/*` build scripts, so their commands are quoted from their READMEs, not tested in this lane.
- The guide's chapter 6 and 7 commands (`sh ux/notebook/run.sh` and the others) are not in the checker.
- I did not run the repository's own suites (`make check`, `tests/test_driver.sh`); the lane changed no code.
- Base-p digits are computed in Python for the guide (section 1.3), because the driver prints none.

## Sources pending

None: every formula and theorem named is cited from `refs/src/` by file and line (Evertse notes for `|.|_p`,
the product formula, Ostrowski, `Z_p` and its digits; Kudla for `Z_p` as digits, the adeles, the diagonal
embedding, the local factors and the functional equation; Milne CFT and Hertogh for the restricted product and
`Zhat`; `flint-3.0.1/arb.rst` for balls; the Bristol Hilbert lecture for the symbol at odd primes and the product
formula; the Warwick Tate notes for the zeta integral, `Lambda(s) = Lambda(1-s)` and `zeta(2) = pi^2/6`), or
from `docs/SPEC.md`, `docs/conventions.md` and `docs/proofs/` by section. No `[source pending]` mark is in the
guide.

## Library demands (what the guide wanted to show and could not, or only clumsily)

1. Digits: base-`p` digits of a local ball, and pre-period and period of a rational (`docs/ux/README.md`
   demand 4). Section 1.3 computes `4444` and `313132` in Python.
2. Variables, or a dump as an operand: section 1.1 pastes the printed idele of `idele 22/7` into five `abs`
   lines; sections 3.9 and 3.16 repeat texts by hand. A computed value cannot be passed on losslessly.
3. The failing place under a status: the driver prints `error: DOMAIN` without the place
   (`tools/adf/README.md:253`), so section 2.5 can only say which place failed in prose.
4. A `make` target for `build/libadelefeld.so`: it is built only by `sh tests/test_exports.sh`, which the guide
   has to tell a Julia user to run.
5. A driver line that returns `NEEDS_SPLIT`: none of the 86 fixtures shows one, so section 2.4 names it in prose
   only.
6. A short form for common test functions (the Gaussian is a 72-character `rfun` text): the `tensor_eval` and
   `poisson` lines exceed 116 characters and cannot be wrapped.
7. A version command (HANDOFF item 3): the guide's outputs cannot carry a provenance stamp; the checker reruns
   them instead.

## Findings against the specification

None against `docs/SPEC.md`. Findings against other documents:

1. `tools/adf/README.md:278-280` is stale. It says that `adf_ffun`, `adf_rfun` and `adf_char` have no typed
   parser and give `error: UNSUPPORTED`. In this build `show ffun(D=1, M=1; (1) + (0)*i)` prints the value and
   `show char(q=5, n=2, s=(0) + (0)*i)` prints the character; only `show rfun()` is `error: UNSUPPORTED`, while
   `rfun rfun()` prints `rfun()` (the guide's section 3.1 shows all three lines).
2. The brief's premise "`make -j2 && make -C tools/adf` gives `build/libadelefeld.so`" is false: the Makefile
   builds only `build/libadelefeld.a` (Makefile, `LIB = $(BUILD)/libadelefeld.a`); the shared object comes from
   `sh tests/test_exports.sh`. hello.c links the static archive; the guide says so.
3. Observation, not a fault: `unitof (1 ; 0 mod 4)` answers `UNIT_NOT_CERTIFIED`, although `4 Zhat` contains no
   unit at 2, so `NOT_UNIT` would be provable. The weaker status is allowed (conventions 3.1 forbids only an
   unproved proved code); the guide uses `unitof (1 ; 1 mod 4)` instead.
