# Lane f-repair6: the findings of review f-review8 on the powers at a prime and on R8: three test gaps (MAJOR), six sentences and one test gap (MINOR)

Review f-review8 (`docs/reviews/f1/review-lpow.md`; its programs are in `lanes/f-review8/`) found no defect of the
library in the powers at a prime (`include/adelefeld/lpow.h`, `src/lpow.c`, `docs/api-1f6.md`) or in the branch
enumeration of the roots (`src/lroot.c`, R8 of `docs/api-1f5.md`). It found that three planted faults, each of
which would be a BLOCKER in the library, pass every test program (F6, F7, F8), and it found false or incomplete
sentences (F1 to F5), one more test gap (F9) and a wording of N-D15 (F10, already corrected in `docs/SPEC.md`).
You close F1 to F9. Lane f-repair5 landed in between (`lanes/f-repair5/result.md`): it changed `src/lpow.c` (an
exact integer exponent goes through `pow_si`, statement P9), `src/lroot.c` (`early_status`; one Teichmueller lift
per branch list, statement R9) and sentences of both headers and both `api` files. Read the files as they are
NOW; where a finding cites a line, find the sentence again.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
comment blocks of `lpow.h` and `lroot.h`; rule 5 is replaced by item 3 below), the review (findings F1 to F10,
"Fault tests and assertions", "Fixture generators and tests that can agree with a wrong library"),
`lanes/f-review8/reproduce_survivors.py`, `survivor_minimal.py`, `mutations.py`, `add_root_fault.py`,
`early_probe_build.py` (the reviewer's faults and how they are built), `lanes/f-repair5/result.md`,
`docs/api-1f6.md`, `docs/api-1f5.md`, `tests/test_lpow.c`, `tests/test_lroot.c`, `tests/test_rfunc_prime.c`,
`proto/lpow_checks.py`, `proto/lroot_checks.py`, `lanes/f-review6/oracle.py` and `lanes/f-review7/oracle.py`
(oracles in exact integers). `refs/src/` is on disk.

**You own:** `tests/test_lpow.c`, `tests/test_lroot.c`, `tests/test_rfunc_prime.c` (the pow and root parts),
`proto/lpow_checks.py` and `proto/lroot_checks.py` (additions: generators of the new rows),
`tests/ref/vectors/f-repair6/` (new, below 1 MB), `docs/api-1f6.md`, `docs/api-1f5.md`,
`include/adelefeld/lpow.h` and `lroot.h` (comment blocks only), `lanes/f-repair6/`. `src/` is READ-ONLY: if a new
test fails on the library as it stands, that is a finding (the input, the expected value from your oracle, the
value returned); stop work on that test, write it in `progress.md` at once and go on with the rest. No git
command that changes state, no `bd`. At most 2 cores (other lanes and another session share this laptop); every
program under `timeout`; build into `BUILD=lanes/f-repair6/build`.

## The repairs

1. **F6 (MAJOR): no ball fixture of `powunit` at a prime of one word and none at 65537 where the term `A + B` of
   `R` decides.** Add rows, from an oracle in exact integers with a stated precision, at `p = 65537` and
   `p = 2^64 - 59` (and keep 3, 5, 7): for each of the three terms of `R = min(A + beta, B + alpha, A + B)` rows
   where that term alone is the minimum; the image checked as a set modulo `p^H` where it can be enumerated or by
   witnesses (two image points at distance exactly `p^R`, and a point of the image that a ball of exponent
   `R + 1` would miss) where it cannot (at 65537 an enumeration over `p^2` residues is too large: use witnesses
   and say so). The reviewer's first distinguishing input is `u = 1 + p Z_p`, `s = Z_p`, `N = 2`.
2. **F7 (MAJOR): no test of the CENTRE of a `powrat` ball result at 65537.** Add rows at 65537 and `2^64 - 59`
   with `n' >= 2`, relative precision at least 2, where the centre is certified by exact integers (the root by
   Hensel lifting in Python, `t^n' = x^e'` modulo `p^K`), not by another call of the library. The review notes
   that `powers_at_prime` in `tests/test_rfunc_prime.c` takes most expected answers from the same local routine:
   add independent expected values there or say in a comment what that test does and does not certify.
3. **F8 (MAJOR): the large branch-list test checks 16 sampled roots.** Check EVERY listed branch of the large
   lists (65536 branches at 65537; the `2^64 - 59` list) by a criterion that a wrong lift fails: for example
   `t_i^n = x` modulo `p^K` for each branch with `fmpz_powm`, or the chain `t_{i+1} = t_i * omega(zeta)` together
   with one full power check, whichever costs less; measure it and keep the test under 5 s of CPU on this machine
   (give the number). The reviewer's smaller reproducer: `x = 1764 + p^2 Z_p`, `n = 2`, seed 42, `N = 2`.
4. **F9 (MINOR): `early_status` replaced by "always `OK`" passes.** Lane f-repair5 rewrote `early_status` and
   found that the order of the decision cannot be seen through the interface (only the cost can). Decide: a test
   that bounds the COST of the refused call in CPU time with a wide margin (the pattern of the wall-clock guards
   in `tests/test_lroot.c`), on an input where the listing would take at least 100 times the guard; or, if no
   such input exists within the limits, one sentence in R6 saying that the early decision is a cost property that
   no test asserts, and why.
5. **F1 to F5 (MINOR), sentences.** F1: P1's example (`3` is not a root of 4 in `Q_5`; the roots are 2 and -2)
   and the claim that reduction is NECESSARY for the seed to name a value (it is sufficient; state the reviewer's
   exact condition `gcd(e, gcd(n, p - 1)) = 1`, at 2 with `p - 1` replaced by 2, after checking it yourself in
   exact integers) in `docs/api-1f6.md` and `lpow.h`. F2: `lpow.h` "working precisions are bounded by K" is
   false (`x = 5^-30 (2 + 5^21 Z_5)`, `e/n = 2/3`, seed 3, `N = 0`): state P3's bound. F3: P8 `|e'| < 2^63` is
   `<= 2^63` (`e = LONG_MIN`). F4: the three omitted endpoint arguments (P1 step 5, P7(b) at `H = 1` at 2, R8 at
   `R = 1`): add them as the review supplies them, after checking each. F5: R8 step 10 counts the explicit
   products and omits the modular powerings: complete the cost sentence.

## Order of work and checks

1. Tests first, and FIRST show that each new test is red on the reviewer's fault: build the faulty libraries as
   `lanes/f-review8/reproduce_survivors.py` and `add_root_fault.py` do (copy what you need into your lane
   directory), run your new test programs against the F6, F7, F8 faults and against the unmodified library:
   three red, one green, recorded in `lanes/f-repair6/redgreen.log` with the failed-check counts.
2. The sentences; every changed statement re-read against the code.
3. Show that the tests bite beyond the three faults: run the reviewer's whole fault table again
   (`lanes/f-review8/mutations.py`: 16 faults) against your test programs and give the table: every fault must
   now fail at least one program; a fault that still passes is reported with the reason.
4. Final checks: `test_lpow`, `test_lroot`, `test_rfunc_prime` in your build directory and under `SAN=1`
   (`BUILD=lanes/f-repair6/build-san`, `ASAN_OPTIONS=detect_leaks=1`); `timeout 900 make -j2 check-all` ONCE at
   the end in `build/` (this one run may take 10 minutes; give its last line); the total CPU time of the three
   test programs before and after your additions (they run in every `make check`: keep the increase below 10 s).

Report: `lanes/f-repair6/report.md`, written once, at the end; running notes in `lanes/f-repair6/progress.md`.
In it: each finding F1 to F9 with what was done; the red and green counts; the fault table; the new fixture files
with row counts and the stated precision of each oracle; findings (a test that fails on the library as it
stands; a sentence of the review that is itself wrong, with the counterexample); what is not done.
