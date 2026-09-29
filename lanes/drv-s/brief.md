# Lane drv-s: commands of the driver `adf` for milestone S (roots and reconstruction from a residue)

The driver `tools/adf/adf.c` (language: decision M1-D1 in `docs/SPEC.md` 15; `tools/adf/README.md`) has no
command for the solvers of milestone S. This lane adds three, so that a user can type them at the prompt.
The grammar of M1-D1 stays: one command per line, an operation name, then one to three operands separated
by the word ` with `; one line of output per command, a result or `error: <STATUS>`.

Decisions taken by the orchestrator (they will be recorded as an addition to M1-D1):
- A polynomial operand is its integer coefficients in decimal, separated by single spaces, the CONSTANT
  TERM FIRST: `-2 0 1` is `X^2 - 2`. At least one coefficient; an operand that is not of this form is
  `error: PARSE` (or the status the driver already uses for a text it cannot read: look and use the same).
- `roots POLY with P with K`: `adf_roots_padic(L, f, p, K, depth)` with `depth` = 64. Output: the roots
  in the order of the list, separated by `; `, each as `A mod P^K` with `A` the centre in `[0, P^K)` and
  `P^K` written as the two numbers `P^K` (for example `3 mod 7^10`); the word `none` for an empty
  complete list; `error: <STATUS>` otherwise.
- `realroots POLY`: `adf_roots_real(L, f, prec)` with the driver's setting `prec`. Output: the balls in
  the order of the list, separated by `; `, each printed as the driver prints a real ball today (find the
  routine that `show` uses for the real part and use it, with the setting `digits`); `none` for no real
  root.
- `recover C mod M with A with B`: `adf_resid_reconstruct` with `limit` = 1000 for the residue class
  `C` modulo `M` and the bounds `A` (numerator) and `B` (denominator). Output: the rational `n/d` as the
  driver prints a rational, or `error: <STATUS>` (`NO_SOLUTION`, `NOT_UNIQUE`, `NOT_DETERMINED`).
  The name `reconstruct` is taken by the reconstruction from a full ball and is not changed.

Read first: `lanes/COMMON.md`, `CLAUDE.md`, `tools/adf/adf.c` (all of it), `tools/adf/README.md`,
`tests/test_driver.sh`, two or three cases under `tests/driver/`, `include/adelefeld/roots.h`,
`include/adelefeld/resid.h` (the comment block of every function you call: statuses, what is written),
`tests/julia/roots.jl` and `tests/julia/resid.jl` (how a user calls these functions today).

**You own:** `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/s-*.cmd` and `tests/driver/s-*.out`
(new), `tests/test_driver.sh` (only what is needed so that the new cases run), `lanes/drv-s/`.
Everything else is read-only. You work in your own git worktree; do not run git commands that change
state.

1. Tests first. Write the cases `tests/driver/s-roots.cmd`, `s-realroots.cmd`, `s-recover.cmd` with their
   `.out` files BY HAND from mathematics you can check, not from the output of the program, and say in
   the first comment of each where every expected line comes from. Examples that must be among them:
   `roots -2 0 1 with 7 with 5` (two roots; their centres are the square roots of 2 modulo `7^5`: compute
   them in Python and check the square); `roots -2 0 1 with 5 with 5` gives `none`; `roots 0 with 7
   with 5` is `error: DOMAIN`; a prime of 64 bits (`18446744073709551557`) with a polynomial that has
   planted integer roots; `realroots -2 0 1`; `realroots 1 0 1` gives `none`; `recover 7 mod 19 with 3
   with 3` style cases with one solution, none, several (find them by enumeration in Python and keep the
   script in your lane directory). Run `sh tests/test_driver.sh` and see the new cases FAIL before you
   write the code; keep the output in `lanes/drv-s/red.log`.
2. The code. Every status of the library is printed as `error: <STATUS>`; the driver never aborts on a
   text of the user; every value is cleared on every path.
3. Hostile input: a coefficient of 100000 digits, 5000 coefficients, `P` not a prime, `P` = 0, `K` = 0
   and negative, `K` of 30 digits, a missing operand, a fourth operand. One case file `s-hostile.cmd`.
4. Run under `timeout`: `make -j2 && sh tests/test_driver.sh`, `SAN=1 sh tests/test_driver.sh`,
   `make clean && make check-all`, `make clean && make -j2 check CC=clang`. Give the last line of each
   in the report. `make clean` between builds with different flags.

Report: `lanes/drv-s/report.md`, written ONCE, AT THE END, never as a running record (the runner takes
its existence as the end of the lane). Keep running notes in `lanes/drv-s/progress.md`. In the report:
what was built, the commands run with their last lines, what is not done, findings against the headers.
