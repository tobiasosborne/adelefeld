# Milestone 1 review: local backend and absolute cap

BLOCKER FOUND: 1 BLOCKER, 2 MAJOR, 1 MINOR.

The blocker concerns the explicitly unrestricted input contract of `adf_fball_is_canonical`.
No wrong enclosure or memory error was found for values produced by the public constructors.

## R1 - MAJOR: every cap function rejects valid local inputs

Location: `src/cap.c:62`, `:108`, `:126`, `:144`, `:163`.
Contract: `include/adelefeld/scaled.h:160-178` specifies plain finite-ball inputs, the capped tight result,
and statuses `ADF_OK` or `ADF_DOMAIN` for `C <= 0`. It gives no global-only precondition.
`docs/SPEC.md:277-281` defines the cap without a backend restriction.

Input: construct `x = y = (2 + 4 Zhat)/2` in context `(4)`, `C = 1`, scalar `q = 1`.
These inputs satisfy predicate L. Call cap, add_cap, sub_cap, mul_cap and mul_rat_cap.
Every result should be `Zhat`, with status 0. Each function returns 8 (`ADF_UNSUPPORTED`) instead.
All five leave the sentinel output unchanged. Converting the same input sets to global makes all five succeed.

Reproducer: `checks/cap_local.c`. Run result: local statuses `8,8,8,8,8`; global statuses `0,0,0,0,0`.
The explanation at `src/cap.c:24-35` predates the local backend and no longer justifies refusing these inputs.
The cap lane report records this refusal; it does not amend the public contract.

## R2 - BLOCKER: the unrestricted canonicality predicate reads beyond a non-context allocation

Location: `src/fball.c:358`, through `src/modctx.c:410`.
Contract: `include/adelefeld/fball.h:92-94` says the predicate returns 0 when the invariant fails and
"never aborts, whatever the fields hold". This is an explicit exception to the normal G/L input precondition
at `include/adelefeld/fball.h:22-27`. `docs/conventions.md:146` also says it never aborts.

Input: initialize a finite ball, set `backend = ADF_LOCAL`, set initialized integer fields to `(0, 4, 1)`,
allocate one readable residue word containing 0, and set `mctx` to a separate one-byte allocation.
Only `is_canonical` is called on this malformed object. It should reject the fields under its stated contract.
Instead, it calls `adf_modctx_nblocks` and reads eight bytes at offset 8 from that one-byte allocation.
AddressSanitizer reports a heap-buffer-overflow and exits 1.

Reproducer: `checks/probe.c`, argument `bad-context`; diagnostic: `checks/bad-context.log`.
This is not an arithmetic failure on a valid local value. Its BLOCKER severity follows the review rule for a
memory error on input admitted by a public function's explicit contract. The API must settle the conflict:
ordinary C pointer fields cannot be treated as safely verifiable arbitrary addresses. A readable initialized
storage and valid-context precondition would exclude this input, but the present header promises more.
The author already noted this conflict at `src/fball.c:347-348`.

## R3 - MAJOR: ADF_CHECK_INVARIANTS does not enforce the documented entry check

Location: `src/fball.c:692-704`, demonstrated on `adf_fball_neg`.
Contract: `docs/conventions.md:288` requires every public function to check the invariant and call `flint_abort`
with a message on invalid input when compiled with `-DADF_CHECK_INVARIANTS`.

Input: build the library and reproducer with that flag. Construct `(1; 1)` in context `(4)`, then set its
initialized denominator to 0 and call `adf_fball_neg(y, x)`. All pointers and allocations remain valid.
Expected: the documented abort. Actual: the call returns and writes a local output with denominator 0.

Reproducer: `checks/invariant_check.c`. Result: exit 0; `neg returned; output canonical=0`.
This is a missing debug-mode contract, not a claim that ordinary arithmetic accepts noncanonical inputs.
The local lane report lists the mechanism as not done.

## R4 - MINOR: an unchanged local canonicalisation test still uses an invalid fixture

Location: `tests/test_fball.c:1010-1014`, helper at `:115-124`.
Contract: `include/adelefeld/fball.h:138-139` requires a local input to `canonicalise` already to satisfy L.
The test supplies `A = 2`, `H = 4`, `d = 2`, residues `(0, 1)` and `mctx = &dummy` for a single `char`.
Predicate L requires `A = 0` and a real context (`include/adelefeld/fball.h:58-63`).

The test then requires status 0 and unchanged fields. These assertions exercise behavior outside the function's
domain; they do not test its local no-op contract. A conforming invariant-check build would reject the fixture.
Reproducer: `checks/invalid_fixture.c`. Result: `fixture satisfies L=0; canonicalise status=0;
original assertions pass=1`. Use a valid raw local value such as `(2; 2)` in context `(4)` for this test.

## Examined and found correct within the checks

Reviewed the assigned C files, local tests, Python reference, lane reports, policies sections 3 and 4,
conventions 5.2, 5.3 and 5.14, and the relevant specification and plan sections.

The independent oracle in `checks/oracle.py` uses Python integers and `Fraction`. It checks canonical triples,
raw local data, context selection, statuses, loss flags and predicates. Its arithmetic oracle is justified as
follows:

1. Form the four rational results using operand members `a + u N`, `b + v M`, with `u, v` in `{0, 1}`.
2. For products, subtract `a b`. The differences generate `a M`, `b N`, `N M` as an integer group.
3. Let `g` be their rational gcd. Every product difference for profinite `u, v` lies in `g Zhat`.
4. Any containing ball must contain the four witnesses, so its radius divides each difference and hence `g`.
5. Thus the resulting ball is both an enclosure and tight. Sums use the two generating differences `N`, `M`.

Results:

- 40,208 C requests, 0 mismatches, in both ordinary and ASan/UBSan builds. This includes 12,288 exhaustive
  arithmetic/alias cases at block 4, all residues, and denominators 1 through 8.
- The remaining cases cover shared denominator factors, canonical cancellation, changed canonical H, mixed
  backends, equal moduli with different block orders/pointers, blocks `2^64-1` and `2^64-2`, 128 blocks,
  denominators above 4096 bits, conversions, failed-output preservation, copying and swapping.
- Products include 5,896 cases. The oracle independently checks the tight ball and whether it can stay local.
  No counterexample to the `h | d e` criterion or the global fallback was found.
- Predicates include 2,120 requests, each checking six results. Equal sets in different raw contexts compare
  equal as sets and unequal as representations. Distinct raw values in one fixed context cannot be equal sets:
  the radius fixes d, and the centre modulo the radius fixes every residue.
- Global cap checks include 20 chains of 50 operations. Every step encloses the independently accumulated tight
  result, every positive capped radius divides C, and applying the cap again preserves the set.
- The project Python reference matches the independent oracle on 2,100 binary-operation cases.
- Valgrind: 900 requests, 0 mismatches, 0 errors; 13,829 allocations and frees; 0 bytes live at exit.
- Existing ASan/UBSan suite: 30 programs, 348 tests, 1,200,445 checks, 0 failed checks. The two dlopen tests
  perform 0 checks because `build/libadelefeld.so` is absent. LeakSanitizer was disabled after 30 sandbox tracing
  failures in the initial run; Valgrind supplied the separate leak check.

The reported loop mutant is not present in production. `src/fball.c:784` uses `i < k`.
`checks/mutant.py` changes only a review-directory copy to `i <= k`. On the same valid one-block input,
the original exits 0 and the mutant exits 1 with an eight-byte heap-buffer-overflow. The mutation claim is real;
it is not evidence of an existing out-of-bounds access in that loop.

The four replacements in merge `def075f` remove fake contexts and obsolete placeholder expectations.
No valid global-input assertion was weakened. The new mixed-backend arithmetic tests compare against global
arithmetic, so they check backend agreement rather than provide an independent arithmetic oracle.
The vector tests also check against the separately written Python reference. R4 concerns a fifth, unchanged test.

## Sources and limits

The cited P19-P25 statements support the implemented conversions, scalar rule and tight-product fallback.
External source anchors were checked on disk: CRT at `refs/src/baker-padic/padicnotes.txt:374-387`;
Zhat at `refs/src/tate-poonen/notes.txt:1603-1605`; word multiplication at
`refs/src/flint-3.0.1/ulong_extras.rst:368-373`; rational gcd at
`refs/src/flint-3.0.1/fmpq.rst:510-522`; allocator hooks at `refs/src/flint-3.0.1/memory.rst:16-21`.
No new source is pending for the findings. One inherited source remains pending in the accessor contract:
[source pending: a Haar-measure reference stating local scaling], `docs/proofs/catalogue.md:209-210` and
`include/adelefeld/fball.h:179-182`. This review does not close it.
No counterexample to the specification's mathematical statements was found.

Not examined: performance, concurrency, allocation failure, independent proofs of every FLINT routine,
or implementations outside the assigned scope. Existing test execution is not a review of those other modules.
No production file was changed. Commands and full results are in `lanes/m1-review-local/report.md`.
The reusable command recipe is `checks/run.sh`; source hashes are in `checks/reviewed.sha256`.
