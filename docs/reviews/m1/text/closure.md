# Milestone 1 closure: text

8 CLOSED, 0 CLOSED WITH EDIT, 0 OPEN, 2 SETTLED BY DECISION. NO BLOCKER OPEN.

| Finding | Severity | Verdict | Evidence |
|---|---|---|---|
| R1 | BLOCKER | SETTLED BY DECISION | M1-D2; two live-pointer predicates return 0. |
| R2 | MAJOR | CLOSED | Eleven old probes exit 0; 20 adjacent print outcomes pass. |
| R3 | MAJOR | SETTLED BY DECISION | M1-D6; old case refused; limit 21/22 case passes. |
| R4 | MAJOR | CLOSED | Eleven boundaries pass; old guard gives six failed checks. |
| R5 | MAJOR | CLOSED | 193 of 193 declarations export; the old link succeeds; nested context load returns OK. |
| R6 | MAJOR | CLOSED | Five old statuses are OK; 24 cap checks pass. |
| R7 | MAJOR | CLOSED | The old live-borrower probe aborts with count 1; seven child cases pass under INV=1. |
| R8 | MINOR | CLOSED | Header corrected; two nonexistent FLINT names remain absent. |
| R9 | MINOR | CLOSED | Four accepted texts with control-byte pairs cover prec 2/191 and digits 1/30. |
| R10 | MINOR | CLOSED | Two minimal Valgrind runs: 0 reports; full test: 0 invalid or uninit reads. |

Paths beginning `checks/` or `closure-checks/` are relative to this directory. The fresh library was built
in `build-closure/`. The normal and `INV=1` suites each passed 42 test programs. The original
`checks/resources.py` ran unchanged. Its four predicate children still signal 11 because they pass address 1
as a context pointer; M1-D2 excludes that input. Its old round-trip child now sees a NULL printer and returns
PARSE when it tries to parse NULL. The old `checks/headers.py` and `checks/boundaries.py` run to their former
failure assertions. Their measured values have changed to zero missing exports and OK for the permitted
19-digit zero exponent. `closure-checks/headers_closure.py` and `closure-checks/boundaries_closure.py` check
those new expectations. The old `checks/probe.c` was run unchanged for `local`, `nested` and `lifetime`.
The old `checks/sentinel_padding.c` and `checks/fuzz_one.c` were rebuilt unchanged and run under Valgrind.
The full adele text test had 20 tests, 84390 checks, no invalid or uninitialized reads, and 21 possible-loss
records under Valgrind; it exited 0 with only definite and indirect leaks counted as errors.

The new test expectations come from M1-D6/M1-D7 and the cap rule in `scaled.h:160-178`: zero times any
permitted power of ten is zero, and capping `1 mod 6` at 4 gives `1 mod 2` since gcd(6, 4) = 2.
The scratch restoration of the old 18-digit guard made `test_text_limits` fail six checks in three tests.
The R2 NULL expectations, R5 symbol calls, R6 local cap assertions and R7 required abort would fail the
old implementations shown in the review. R3's limit-21 case pins the qualified rule but would also have
passed the old printer for that small input. R9's fuzzer change has coverage evidence, not a red assertion.
The two R10 minimal Valgrind reproducers gave errors on the old helper in the review log; they give none now.

## R1: settled by M1-D2

M1-D2 requires every pointer field of an initialized object to be NULL or to point to a live object of its
kind. `fball.h:92-99`, `rat.h:54-56`, `adele.h:90-93`, `scaled.h:76-79`, and `conventions.md:146` use this
scope. The old address-1 input is outside it. `closure-checks/probe_closure.c` gives live contexts to
malformed local and scaled values; both predicates return 0 without an abort in a five-check setup. The four old
invalid-pointer children still signal 11. This does not show a remaining admitted-input failure.

## R3: settled by M1-D6

M1-D6 and `conventions.md` 9.6 condition read-back on a printer that returned text and on reader limits that
admit that text. `text.h:32-42` says the same. The old input `(9.99e100000 ; 0)` is read, then refused by
the printer's binary-exponent limit, so the old default-limit read-back is no longer a valid requirement.
With `(9.99e21 ; 0)`, `digits=1` and `max_exp10=21`, the printer returns text; reading it at 21 gives LIMIT,
and reading it at 22 gives OK and a real ball containing the source. Five checks pass.

## Proposed M1-D10 and M1-D11

These decisions are proposed, not accepted. I found no input contradicting either proposed rule.
`closure-checks/lifetime_closure.c` checks both sides of M1-D10 under `INV=1`: library-created borrowers
are counted; a canonical value assembled by hand is invisible; overwriting a counted context field by hand
leaves a stale borrow and makes the later free abort. The last two cases are outside the proposed lifetime
contract. The qualification is still absent from the broad count sentence in `conventions.md:323-328` and
from `modctx.h:16-19`. Acceptance and those document edits remain pending. For M1-D11, an infinite real
part makes `adf_adele_reconstruct` abort at the entry check under `INV=1`; the release test expects DOMAIN.
The input is non-canonical, as the proposal says. Neither proposal was used to change R1-R10's verdict.

## New findings

None from these checks.

## Not examined

No full independent cap arithmetic oracle across random contexts, no exhaustive dump grammar run, and no
long fuzz or sanitizer run. The one scratch red edit checked the R4 test's sensitivity; the other repair
tests were judged from their assertions, the current runs, and the earlier red logs. The two proposed
decisions still require acceptance. The old review's pending sources for exact `mag_set_ui_2exp_si` behavior
and FLINT allocation failure behavior were not resolved here.
