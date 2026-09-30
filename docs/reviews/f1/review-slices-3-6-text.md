# n-review1: report, assembled by the orchestrator from the lane's notes and logs

The codex session (gpt-6.1-sol high) did the work of parts A to D (04:06 to 04:30) and then could not
write its report: its provider refused every further turn with "flagged for possible cybersecurity
risk" (three attempts of the runner and one plain request of the orchestrator; `lane.log`). What follows
is taken from `progress.md` and the log files in this directory, which the lane wrote as it went. The
orchestrator did not rerun the probes except where said.

## Part A: closure of f-review2 (R3 to R8)

| Finding | Judgment | Evidence |
|---|---|---|
| R3 | CLOSED | `edge precedence`: LIMIT at 2, output empty |
| R4 | CLOSED | the six entry cases and the control abort, exit 134, public names |
| R5 | CLOSED for the abort; OPEN for the contract | five LONG_MAX probes return LIMIT. But under INV=1 all seven `_at` functions and the three ring operations of `adf_sball` allocate 14 times (135160 bytes) BEFORE LIMIT (`partial-alloc-inv.log`; `partial_alloc.c`): the same defect as F1 of i-review2, repaired for ideles by lane i-repair1 and not for partial balls |
| R6 | CLOSED | the Julia probe: the 1600-byte record is gone (the process still leaks 50143 bytes of the Julia runtime, not adelefeld) |
| R7, R8 | CLOSED | the text has the exceptions |
| R1, R2 | as decided (N-D7) | the inputs still return LIMIT |

## Part B: closure of i-review2 (F1 to F5)

F1 CLOSED (all four allocator probes: LIMIT, 0 calls, 0 bytes, release and INV). F2 to F5 are text; the
numerical results are unchanged (the code was right). The lane read the repaired text separately;
its notes record no objection.

## Part C: lane f-slice3 (decomposition, fractional part, powers, early LIMIT)

- Own oracle: 45006 checks, 0 mismatches (256480 power points, 73216 split points, 14014 fractional
  parts); `local-oracle.log`.
- MAJOR C1: `src/lball_decomp.c:213`: signed overflow of `N - v` for a canonical ball with `v = -1`,
  `N = LONG_MAX` at `p = 2` (UBSan, `overflow-ubsan.log`, `edges.c`). The limits of N-D4 are not tested
  before the subtraction. (`is_canonical` admits any slong: the limits are the functions' to check.)
- MINOR C2: `adf_lball_set_fball` with `H = 6^33554433`, `A = 2^67108866` returns LIMIT only after the
  full valuation of `H` (16.8 s; `projection-edge.log`): the early decision of L13 does not reach this
  branch (`bits(A)` above the bound).

## Part D: lanes f-slice6 and t-slice1

- Own text oracle: 5782 calls, 33344 checks, 0 mismatches; 2213 round trips enclose (`text-oracle.log`).
- BLOCKER D1 (driver): `project X with P`, `exp_at`, `log_at` accept a unit coset, an idele or a class as
  `X` and read a field of the wrong type: `project (5 ; 5 * [1]) with 5` prints `5: 0`, `exp_at [5 mod 6]
  with 5` prints `5: 1` (`driver-cross.cmd`, `.expected`, `.actual`). The documented domain is a rational,
  a finite ball or an adele; the answer must be `error: UNSUPPORTED`. A wrong printed value.
- MAJOR D2: the constrained printer (conventions 9.5) on the ball `2^99999 + 1/2 +/- 2^99999` at
  `digits 1` does not end in 170 s (`printer-worst.log`); both exponents are 100000, so M1-D6 admits the
  value. A hang on admitted input.
- The sanitizer runs of the three bridges ended with exit 1 (`local_san_exit=1`, `text_san_exit=1`,
  `prime_san_exit=1`, "FAIL bridge_exit"): the lane's own bridges under ASan, in the codex sandbox where
  LeakSanitizer cannot run (seen in every codex lane of the night). NOT a finding against the library;
  not verified by the orchestrator either.
- Prime oracle of f-slice6: 6600 calls, 18221 checks, 1420 series points (`prime-oracle.log`); no
  mismatch recorded in the notes.
- Decision t-2 (the real ball of the readers), the driver text of N-D10 against conventions 9 and the
  question of the acceptable cost of the printer were not judged in the notes.

## What is not done

The lane's report in its own words; the judgment of t-2; the reading of the repaired texts of F2 to F5
in the lane's words. The findings above are the orchestrator's reading of the lane's evidence.
