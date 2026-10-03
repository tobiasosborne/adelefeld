# Lane f-review8: referee of the proofs P1 to P8 and R8, the tests that cannot fail, and the cost (the part of review f-review7 that was not done)

Your task is to REFUTE, not to confirm. Review f-review7 of the powers at a prime (lane f-slice9, WP 1F.6) and of
the branch enumeration of the local roots (lane f-repair4) was cut off by a container restart. What it did is in
`lanes/f-review7/progress.md` (its notes and the orchestrator's addendum): attacks on values, exponents, statuses,
branch lists, `_at` forms and driver, about 80000 cases against its own oracle in exact integers, 0 failures, three
MINOR findings. Do NOT repeat those attacks. Three bullets of its brief (`lanes/f-review7/brief.md`) were not
reached. They are your task.

The code and the statements under review (author: Claude Opus; you are of another model family on purpose):
`include/adelefeld/lpow.h`, `src/lpow.c`, `docs/api-1f6.md` (P1 to P8, "Interface and decisions"), the oracle
`proto/lpow_checks.py`, the tests `tests/test_lpow.c`, the pow parts of `tests/test_rfunc_prime.c`,
`tests/julia/lpow.jl`, `tests/driver/pow-*`; `include/adelefeld/lroot.h`, `src/lroot.c` (`dth_root`,
`identifiers`, `early_status`), `docs/api-1f5.md` (R8 and the sentences of R2, R4, R6, R7 that N-D14 changed),
the f-repair4 additions to `tests/test_lroot.c`. Contract: `docs/SPEC.md` 9.3.3, 9.3.4, 15.4 (N-D13, N-D14,
N-D15); `docs/proofs/functions.md` Lemma 3, Lemma 9, Propositions 11, 13, 15, 17, 18; `docs/api-1f.md` L12;
`docs/conventions.md` 3.1, 3.2, 4.1, 4.3. `refs/src/` is on disk (FLINT 3.0.1 under `refs/src/flint-3.0.1/`).

**You own:** `lanes/f-review8/` only. Everything else is read-only. No git command that changes state, no `bd`.
At most 2 cores (another session on this machine runs emulators). Build the archive ONCE with
`timeout 600 make -j2 BUILD=lanes/f-review8/build lanes/f-review8/build/libadelefeld.a` and link your programs
against it (`-Iinclude -lflint -lgmp -lm`; `-std=gnu11` if you use `clock_gettime`); you may reuse
`lanes/f-review7/h.c` and `oracle.py` by copying them into your directory. Do not run the test suites of the
repository as a whole; you may build and run single test programs into your own build directory
(`make -j2 BUILD=lanes/f-review8/build lanes/f-review8/build/test_lpow`). Every program under `timeout`; none over
180 s. Lane f-repair5 is changing `src/lpow.c` (a `LIMIT` case), `src/lroot.c` (the Teichmueller lift per branch)
and three sentences in another worktree at this moment: you review the tree as it stands in yours.

## Part 1: referee P1 to P8 and R8 as proofs

Read each statement and its proof line by line, as a referee who is paid for each false step. For each: the claim
in your own words; every step checked, and for each step that is not immediate your own derivation or a
counterexample computed in exact integers (a program in your directory; state the precision of every comparison).
The points named by the first brief, none of which was examined:
- P1: the counterexample `2/2` at 5 and the order root-then-power; is the reduced fraction really necessary and
  sufficient for the branch to be well defined by the seed of the `n'`-th root?
- P2: `E' = e' j + (M - m) - v_p(n') + v_p(e')` with `e' < 0` and with `j < 0`; the case `p | n'` and the case
  `p | e'` together; the exact image (two image points at distance exactly `p^E'`) or only an enclosure?
- P3: the working precision of the composition: is the root computed at a relative precision that suffices for
  EVERY input the header admits (large `|e'|`, `v_p(e') > 0`, `j` very negative), or is there an input where the
  result ball is claimed at `min(N, E')` and the centre is known to fewer digits?
- P4, P5: the domain and the 2-adic cases: the hull `1 + 2 Z_2`, the claim that the union misses `5 + 8 Z_2` for
  sign `-1`, `B = 0`, and every case where the lane returns an exact image "when that is one coset": enumerate
  the union modulo `2^H` for small `A`, `B` and compare with the statement, not with the code.
- P6: `alpha = v(w0 u0 - 1)` computed by an exact subtraction: is it `v(log u0)` in every admitted case (at 2:
  `v(log u) = v(u - 1)` holds for `v(u - 1) >= 2`; what does the proof say at `A = 1`, and for `u0 = -1`)?
- P7: what the oracle proves at its stated precision: does `H'` suffice as claimed (the density argument)?
- P8: limits, transactions, aliasing: one sentence each that the code must keep; find one it does not.
- R8: distinctness and completeness of `t0 zeta^i`; the CRT idempotents of `p - 1`; the digit search in each Sylow
  `q`-part (termination, the bound on `q`, `d` with a repeated prime factor, `q = 2` with `p = 3 mod 4`, the case
  `T^d = w^e` without solution: is "not listed" proved, and is `DOMAIN` then the status?).
A step that is true but not proved as written is a MINOR with the missing argument supplied; a false step with a
counterexample input to the LIBRARY is a BLOCKER or MAJOR by its consequence; a false step that the code does not
rely on is a MINOR.

## Part 2: tests that cannot fail

For `tests/test_lpow.c`, the f-repair4 additions to `tests/test_lroot.c` and the pow and root parts of
`tests/test_rfunc_prime.c`: find assertions that hold for a wrong library. Method: plant faults in a scratch copy
of `src/lpow.c` and `src/lroot.c` under your build directory (your OWN faults, at least 12, different from the
lanes' five each; among them: `E'` off by one in each of its four terms; `R` with one of its three terms dropped;
the hull at 2 replaced by the exact image of one sign; `K = N` instead of `min(N, E)`; `zeta` of order `d/q`; one
CRT idempotent swapped; `early_status` returning `OK` always; the seed compared modulo `p - 1` instead of `p`),
build the test programs against each, and record which fault each program detects. A fault that every test passes
is a finding with the smallest input that distinguishes it. Also read the fixtures' generators: is a stored row
computed by the same formula as the code (then it guards against regression only: say which rows)?

## Part 3: cost, measured once

One table: `powrat` and `powunit` at `p = 3`, `65537`, `2^64 - 59`, `N = 100`, `1000`, `10000`, exact and ball
inputs, against the cost of the components the header names (one root, one `pow_si`; one `Log`, one product, one
`exp`): the ratio, and every case where the composition costs more than 1.5 times its named components, with the
line of code responsible. `docs/PERF.md` says how speed is judged. KNOWN and not to be reported again: one
Teichmueller lift per branch in `adf_lball_roots` (lane f-repair5 repairs it); the stale cost sentence of
`lpow.h`; `powunit` `LIMIT` for a small result; the false order sentence about `early_status`.

## Report

`lanes/f-review8/report.md`, written once, at the end; running notes in `lanes/f-review8/progress.md` as you go
(if the session is cut off they are the report: write a note after every statement refereed). For each finding:
severity (BLOCKER: a wrong enclosure, a wrong exponent that loses points, a memory fault, undefined behaviour;
MAJOR; MINOR), the input or the proof step, what the code or the text says, what is true and why, and the command
that reproduces it with a program in your lane directory. Then, statement by statement, what you checked and how
(counts, precisions, what would have made a case fail). Then the fault table of part 2 and the cost table of
part 3. No praise, no summary.
