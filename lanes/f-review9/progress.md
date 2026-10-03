# f-review9 running notes

2026-10-03. Scope: adversarial review of gfunc.c and its public surfaces. Only this lane is writable.
Read CLAUDE.md, the named SPEC and PLAN sections, gfunc.c, gfunc.h, api-1f8.md, and f-slice10's report.
No git state changes or tracker commands. No subagents. Archive build is next, with two jobs.

Initial proof targets: G1's negative integer root wording; G2's finite-place implication; G3's
low-precision branch enclosure; G4's inexact-unit statement; G6's machine-word prime bound.
No finding is established yet.

Archive built once with the prescribed command, exit 0. Driver built in this lane, exit 0.
Baseline test_gfunc: 10 tests, 1088693 checks, 0 failed checks, exit 0.
Twelve scratch faults built and run: eleven killed; F11 (normalise the unit at degree 1) survives.
The independent reproducer confirms it changes [5 mod 6] to [2 mod 3], while baseline preserves it.
Both results are canonical; the changed representation violates the promised exact copy.
Logs: fault-results.json, faults/*/build.log and test.log, identity-baseline.log, identity-fault.log.

Own oracle: 6168 exact rational rows and 14652 real rows, intervals at 1200 bits.
First oracle attempt wrote rat.tsv then failed: this mpmath interval context has no sinh/cosh methods.
Replaced those calls with the difference/sum of exp(x), exp(-x), divided by 2; generated real.tsv.
First attack compile failed on three nonexistent convenience functions; corrected to field setters.
First identity reproducer had a wrong literal byte length; corrected to strlen.
Initial independent attack completed with exit 1; its log is being investigated.
The one 100000-bit first-prime cost measurement completed with exit 0; see cost.log.

Independent oracle corrected further: exact rational endpoint roots now use integer arithmetic, and tiny
series arguments use 1200 + 2 * max(0, -e, -re) interval bits (up to 9392).
Initial real comparisons had 2832 failed checks because oracle enclosures were wider than exact C results,
or lost tiny-argument cancellation. These are not library findings. No tolerance was added.
Final attack: 6168 rational rows, 14652 real rows, 98604 calls, 684324 checks, 0 failures.
ASan and UBSan instrument attack.c and gfunc.c. Other archive objects are release builds.
LeakSanitizer cannot run in this environment (fatal ptrace diagnostic); detect_leaks=0 for final runs.
Driver: initial malformed complex/class examples gave 12 expectation mismatches, corrected to valid grammar.
Final: 156 own cases, 0 mismatches; 6 selected golden files, 120 expected lines, all byte-equal.
Julia gfunc.jl: 4 + 7 tests, 0 failures, using a lane-local shared build and one Julia thread.
Cost: 100000 bits, 6913 initial odd primes, expected and reported prime 69761, 0.014608318 seconds.

Additional fault F13: accept every inexact idele at n >= 4 as though its unit were exact.
test_gfunc still returns exit 0, 1088693 checks, 0 failures. Own degree-4 reproducer fails this mutant:
input (1 ; 1 * [1 mod 8]), baseline NOT_DETERMINED, mutant OK with exact unit.
A member whose 3-adic unit is 2 has no fourth root (fourth powers of nonzero residues modulo 3 are 1).
This is a MAJOR test gap; the degree-1 nonnormal-unit gap is a MINOR test gap.

Invariant diagnostic: compiled gfunc and its direct callees with ADF_CHECK_INVARIANTS.
Root of an adele/idele with a NaN real part and series on the NaN adele all return DOMAIN, exit 0.
The control adf_adele_set aborts, exit 134, with the required invariant message.
This violates conventions 4.4's debug-build entry check; invalid release inputs are outside the contract.
First debug link missed adf_inv_borrow/release; adding src/modctx.c resolved it.

Status matrix first run hit timeout 180, exit 124, with no completed counts.
Removed costly nontrivial evaluations exactly at ADF_REAL_PREC_MAX from the bounded matrix.
They are recorded as skipped, not passed. Checks at one above the cap remain for every input.
Final matrix results are in status-final.log.
G1 proof step 2 falsely says r >= 2 for any integer r with q = r^n >= 2: q=4, r=-2, n=2.
The correct inequality is abs(r) >= 2; the conclusion about bit lengths is unaffected.

Completed status matrix: 29880 root calls, 3240 series calls, 144 idele identity calls;
33264 total calls, 155134 checks, 0 failures. 720 costly precision-cap evaluations explicitly skipped.
Own finite-place search: 500 canonical rationals, 7500 calls, 0 failures; denominator up to 14971 bits.
G1 counterexample reproducer: q=4, n=2, r=-2, r^n=4, r>=2 false, abs(r)>=2 true.
The first proof reproducer build referenced nonexistent fmpz_cmpabs_ui; replaced with fmpq_abs and comparison.

No wrong value/enclosure, false DOMAIN, or memory fault established for valid inputs of the baseline.
No full repository suite and no long fuzz run. One original status-matrix run timed out as recorded above.
Sources pending: an explicit FLINT/GMP size bound for G1(c)'s bits(f) <= WORD_MAX;
and a proved word-prime bound for G6's claim that the candidate prime fits ulong for every representable A.
The printed G6 proof bounds the number of candidates, not their numerical size.
Final findings: one MAJOR test gap, three MINOR issues (identity test gap, debug entry checks, G1 inequality).
