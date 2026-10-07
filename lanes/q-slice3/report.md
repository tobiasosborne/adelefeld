# Lane q-slice3: slice 3.2-a

Completed on 2026-10-07. The inherited implementation was retained after checking its contracts and sources.
The resumed work strengthened the tests, completed the statements and fault checks, and verified the user calls.
There are five public functions. Class and local character calls are not declared by psi.h.

Final results: test_psi has 186799 checks. check-all passed 85 C programs, 72 driver cases with 101348
expected lines, 505 exported functions, Julia, and both tool self-tests. Julia's Tate psi test has 11 assertions.
All 11 planted faults were rejected by assertions. The 60 automatic mutation candidates are accounted for below.
All source/prose lines written by this lane are at most 116 characters and end with a newline.

## Work per step

A. Regenerated all 80 oracle vectors from lanes/q-slice3/gen_vectors.py: 299781 bytes and 3200 points.
The real midpoints and radii are exact stored dyadics. Finite centres and radii are rational.
Cases include denominators 1,2,3,4,6,12,360, negative centres, 2000-bit operands, and a 2001-bit root order.
Each record stores the singleton phase or a nonsingleton marker, four exact Q4 distances, and 40 point phases.
JSONL records retain the required one-record-per-line format; the 116-character rule covers source and prose.

B. The C test consumes every record and every golden row: 15 valid finite-angle lists and 2 parser errors.
It checks the 29 listed golden angles. The parser statuses are DOMAIN and PARSE, as reserved by the author.
For vectors it checks singleton statuses and reduced phases, strict/default statuses, and all sampled phases.
Each point is checked inside its input using exact real endpoint comparisons and finite rational membership.
The 3200 distinct points are tested at p=2,20,53,128, giving 12800 complex containment checks.
There are 640 coordinate-hull checks, each asserting both containment and the design 3.3 endpoint bound:

    4*2^-p + 2^-28*(W/2 + 2*2^-p).

The phase evaluator additionally checks 100 rational angles at those four precisions and the same bound.
The four cardinal phases are exact, including at the cap. The noncardinal phase 1/3 fails certification at
the cap with NOT_DETERMINED and an untouched output. Precision above the cap gives LIMIT first.
There are 500 exact dyadic diagonal rationals with theta=0 and exact numerical psi=1, 200 exact-additivity
pairs, and 200 ball-additivity pairs with 40 sampled products each. Each sampled product is contained in
both the character-of-sum rectangle and the product rectangle. Q4:625-626 permits the latter to be wider.
An independently rounded diagonal 1/3 encloses 1 but has real uncertainty, so its phase getter is NOT_DETERMINED.
The existing adele type cannot store a non-dyadic exact real rational or the correlation of that diagonal point.

Failure wrappers seed allocated, nonzero output sentinels and compare both struct bytes and copied values.
Tests cover inclusive exponent, mantissa, raw finite numerator/radius, and projected carry boundaries.
They cover finite radius zero, integral and fractional radii, precision 2 and below, the cap and one above it.
The local-backend case has raw d>1 but integral reduced H/d; its phase and strict character succeed.
Normal builds check nonfinite real balls with DOMAIN. INV checks 8 invalid-input child aborts.
Invalid theta outside [0,1) or unreduced is a precondition violation, checked under INV; precision LIMIT is first.
All five signatures have different output and input types. No output/input alias is permitted, including members.

C. Retained include/adelefeld/psi.h and src/psi.c. Their five functions use exact modular angles, bounded
preflight, certified trig intervals, and Q1 rounding of Q4's rectangular hull. All entry points check INV.
There is one psi.h include in include/adelefeld.h. No production-code repair was needed in this resume.

D. Verified the inherited psi and psi_strict driver commands, their mathematical fixtures, and Julia call.
The driver prints an ordinary complex ball through the same cadele printer used by local_zeta_factor_at.
The expected files existed before the resumed runs and were not adjusted from observed output.
Moved only the owned README section after the quotient commands, so it has its own heading.
The Julia script uses exported size/alignment queries, initialized storage, GC.@preserve, and finally cleanup.
tests/test_julia.sh registers the psi script and its existing GMP-preload retry pattern.

E. Added docs/api-3b.md. It states all five results, statuses, bounds, costs, and enclosure-preserving steps.
It cites design 3.1-3.3, Q4, analysis Lemma 2, and the opened primary sources. Each statement has Check lines.
It documents canonical theta as a precondition, the raw-nonfinite courtesy, and conservative projected bounds.
It also supplies the rounding argument that does not require the rounded midpoint to stay in its input interval.

F. Planted the six slice faults from design:721-737, the brief's four faults, and one extra universal square.
Each was compiled in scratch storage and failed an assertion. The automatic sweep tried exactly 60 candidates.
Added tests for reachable survivors: radius mantissa rollover, exact bit/carry boundaries, forced valid trig
refinement, the width=epsilon boundary, and missing temporary clears. The source remained unchanged.
The refinement harness widens a correct trig enclosure; it does not substitute an incorrect oracle value.
The lifecycle harness counts direct q/arf init/clear pairs in a scratch compilation. It is not a general leak check.

## Hand derivation of the sign

The implemented convention is conventions 6.1:844:

    psi_p(x)=E(fp_p(x)), psi_inf(x)=E(-x), psi(x)=psi_inf(x_inf) product_p psi_p(x_p).

Conventions:876-878 gives the B-th-root family on a fractional finite radius; :881-887 fixes default versus strict.
CV-54 at :852-874 conjugates psi only in the Fourier kernel.
Opened refs/src/tate-poonen/notes.txt:693-694 gives the negative real sign. Lines :695-700 give the positive
finite sign and triviality on Z_p; the raised n is extracted separately at :699. Lines :733-740 distinguish
the transform convention. The same sign calculation is before the code in tests/test_psi.c.

1. At (0 ; 1/3), fp_3(1/3)=1/3 because 1/3 belongs to Z[1/3] and its difference from itself is integral.
2. For every other prime, 1/3 is integral, so its primary fractional part is zero.
3. The real factor is E(-0)=1. Thus psi=E(1/3)=-1/2+i sqrt(3)/2, with positive imaginary part.
4. On a diagonal rational q, analysis Lemma 2:87-88 gives sum_p fp_p(q)=q modulo integers.
   The real factor cancels it. Descent to A/Q is the separate step :92-94, not merely the triviality step.

The trig contract is refs/src/flint-3.0.1/arb.rst:1125-1139; its ball enclosure guarantee is :6-12.
Correct arf rounding is arf.rst:24-35, and exponent-sensitive rational conversion is :310-315.
The fixed 30-bit mag format and permitted extra conversion ulps are mag.rst:6-15.

## Executed checks

Commands were run from the repository root, with at most make -j2. Every test/script had an outer timeout.
The named-fault and recheck scripts also put each compiler, archiver, and test under timeout 150.

Initial and expanded normal build command B:

    timeout 180 make -s -j2 BUILD=lanes/q-slice3/build lanes/q-slice3/build/test_psi
    timeout 150 lanes/q-slice3/build/test_psi

B returned 0. Successive test runs returned 0 with 172069, 186717, 186744, and 186772 checks.
One intervening input-membership check exited 1: converting a 2000-bit exact real point to a 512-bit ball
cannot prove membership. The test now compares exact rational endpoints. No production code was changed.

    timeout 30 lanes/q-slice3/build/test_psi strict

Exit 0, 5 checks. Historical initial reds are retained in redgreen.md and identified as inherited, not rerun.

    timeout 150 python3 -B proto/quotient3_checks.py
    timeout 120 python3 -B lanes/q-slice3/gen_vectors.py

Both exited 0. The oracle passed all 15 groups, including phases 335, hulls 98, local_images 128,
ball_additivity 180 with 1354 witnesses, and golden_phases 15. The generator wrote 80 rows, 3200 points,
299781 bytes. The full oracle output is oracle.log. Generation was deterministic and the C test reads all rows.

Variant builds, each followed by timeout 150 on its test_psi executable:

    ASAN_OPTIONS=detect_leaks=0 timeout 180 make -s -j2 BUILD=lanes/q-slice3/san SAN=1 \
      lanes/q-slice3/san/test_psi
    timeout 180 make -s -j2 BUILD=lanes/q-slice3/inv INV=1 lanes/q-slice3/inv/test_psi
    timeout 180 make -s -j2 BUILD=lanes/q-slice3/clang CC=clang lanes/q-slice3/clang/test_psi
    ASAN_OPTIONS=detect_leaks=0 timeout 180 make -s -j2 BUILD=lanes/q-slice3/fault-base \
      CC=clang SAN=1 INV=1 lanes/q-slice3/fault-base/test_psi

Each successful build and test returned 0; each test had 186772 checks. The first INV build exited 2 because
the child ignored freopen's return value under -Werror. It now checks the return. That build was rerun.
SAN executable runs used ASAN_OPTIONS=detect_leaks=0. This sandbox cannot run LeakSanitizer.

After the mutation-driven test additions:

    ASAN_OPTIONS=detect_leaks=0 timeout 180 make -s -j2 BUILD=lanes/q-slice3/final \
      CC=clang SAN=1 INV=1 lanes/q-slice3/final/test_psi
    ASAN_OPTIONS=detect_leaks=0 timeout 150 lanes/q-slice3/final/test_psi

Both exited 0; the final test had 186799 checks.

Driver checks:

    timeout 180 make -s -j2 -C tools/adf
    timeout 30 build/adf < tests/driver/psi-values.cmd > lanes/q-slice3/psi-values.got
    cmp tests/driver/psi-values.out lanes/q-slice3/psi-values.got
    timeout 30 build/adf < tests/driver/psi-status.cmd > lanes/q-slice3/psi-status.got
    cmp tests/driver/psi-status.out lanes/q-slice3/psi-status.got

Build exit 0. Values exit 0, comparison 0, 9 expected lines. Statuses exit 1 as expected, comparison 0,
10 expected lines. The whole-tree driver check later reran both fixtures among its 72 cases.

Named faults:

    FAULT_CC=clang ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B \
      lanes/q-slice3/run_faults.py lanes/q-slice3/fault-base
    FAULT_CC=clang ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B \
      lanes/q-slice3/run_faults.py /tmp/adf-q-slice3-pristine

First exit 0, 10 assertion failures. Final exit 0, 11 assertion failures, including the extra universal square.
The saved baseline executable has 186772 checks; the newly compiled final public tests have 186799 checks.
An earlier invocation exited 2 because the new script was initially at the root. It was moved into this lane.
There is no stray root script. The scratch copies were removed even on failure by TemporaryDirectory.

Whole-tree acceptance, run once after the final tests:

    PYTHONDONTWRITEBYTECODE=1 timeout 1700 make -j2 check-all

Exit 0. All 85 C programs passed, including test_psi with 186799 checks. The driver passed 72 cases and
101348 expected lines. Exports passed 505/505 declarations, with 0 missing and 0 undeclared functions.
The Julia script passed, including 11/11 Tate psi assertions, with the existing system-GMP preload retry.
Both tool self-tests passed. The memory-tool tree check examined 124 files with 0 findings.
This took 262 seconds by the acceptance log's creation/final-write timestamps.
check-all.log retains summaries: its complete 3256-line, 132767-byte output was reduced to 13927 bytes.
The mutate self-test's example candidates are its own fixture, separate from the 60 psi candidates below.

Static checks and cleanup:

    timeout 30 python3 -B lanes/q-slice3/audit.py
    timeout 30 python3 -B lanes/q-slice3/cleanup.py

All audit runs exited 0. Before this report: 17 files, 1719 lines <=116, 5 Python parses, 5 declarations,
1 vector file of 299781 bytes. A preliminary awk width check found 0 overlength source/prose lines.
Final audit including this report: 18 files, 2030 lines <=116, 5 Python parses, 5 declarations; exit 0.
Cleanup exited 0. Five lane builds were removed before the sweep; the final lane build, pristine archive,
and empty mutation root were removed afterward. No lane build or fault scratch copy remains.
There are 0 generated logs over 100000 bytes. Shared build/ holds the required acceptance artifacts.

sha256sum returned 0 for these two files:

    src/psi.c
    12af690bfdfcdb77c9f9e83998918e3272f07aa151354e9dcd97e082707916bd
    tests/ref/vectors/q-slice3/psi.jsonl
    08ffedcc11e08aa6ccf71ae205a2f61e58bfca2b608b623242185eb177f2aded

## Fault table

All rows below compiled and exited 1 on the stated final assertion. The pristine builds exited 0.

| Fault | Assertion that rejected it |
|---|---|
| D1 finite sign reversed | golden complex phase containment |
| D2 numerator used for root count | golden complex phase containment |
| D3 strict writes on ambiguity | unchanged output struct bytes |
| D4 getter ignores real radius | singleton status against oracle |
| D5 getter chooses one finite family member | finite getter NOT_DETERMINED |
| D6 pi instead of 2pi | numerical phase containment |
| D7 extra universal square | endpoint excess bound |
| O1 real factor has finite factor's sign | exact phase against oracle |
| O2 phase not reduced modulo one | canonical theta in [0,1) |
| O3 endpoint-only hull misses an interior extremum | hull endpoint containment |
| O4 finite radius ignored | strict finite-integrality status |

D1-D6 correspond to the slice-relevant faults in design:721-737. O1-O4 are the brief's four faults.
D7 also exercises the oracle's universal-square control. Local ordinary-fractional-part faults belong to 3.2-c.
Full final assertion locations are in fault-results.tsv and faults-final.log.

## Mutation sweep and survivor audit

    timeout 10 python3 -B tools/mutate/mutate.py --files src/psi.c --limit 60 --seed 320307 --list

Exit 0: 284 candidates, the selected 60 recorded in mutants.txt.

    ASAN_OPTIONS=detect_leaks=0 timeout 1180 python3 -u -B tools/mutate/mutate.py --root . \
      --scratch /tmp/adf-q-slice3-mutate --files src/psi.c --limit 60 --seed 320307 \
      --jobs 1 --timeout 150 --san \
      --make 'make -s -j2 check INV=1 CC=clang TEST_SRC=tests/test_psi.c' \
      --copy Makefile include src tests lanes

Exit 1. Baseline passed in 13.3 s. All 60 ran in 1036.6 s: 38 killed, 14 survived, 7 not compiled,
1 timed out, 0 excused. One worker with make -j2 keeps compiler cores at two, as in q-slice2's restricted sweep.
The source and public test were frozen during the sweep. No mutation-tool or equivalent.txt edit was made.

    ASAN_OPTIONS=detect_leaks=0 timeout 150 python3 -B lanes/q-slice3/recheck_survivors.py \
      /tmp/adf-q-slice3-pristine
    ASAN_OPTIONS=detect_leaks=0 timeout 40 python3 -B lanes/q-slice3/recheck_survivors.py \
      /tmp/adf-q-slice3-pristine timed-out

Both exited 0. First: the 14 original survivors were rechecked; 7 failed assertions and 7 preserved behavior.
Second: the timeout candidate failed an instrumented assertion immediately. There were no new candidates.
Pristine checks before and after each recheck passed: public 186799 checks, instrumented 37 checks,
lifecycle 1 character call with 0 unmatched q/arf temporaries. The clear-loop fault left 1 q and 1 arf unmatched.
The two rechecks took 94 and 4 seconds by their log timestamps. Sweep plus rechecks was under 20 minutes.
Thus 46 distinct selected candidates ultimately failed tests, 7 are equivalent below, and 7 did not compile.
The initial tool aggregate remains recorded above; the follow-up results are not presented as that aggregate.

Remaining equivalent survivors, one line each:

- psi.c:63 return 0->1: the equal-denominator carry guard is unreachable for these callers.
- psi.c:52 projected denominator 1->0: both are below the same positive cap in the integer branch.
- psi.c:187 &&->||: extra divisions change neither the value nor the eventual status; proof below.
- psi.c:241 remove st=OK: all four successful cosine calls already leave st=OK.
- psi.c:63 carry +1->-1: the equal-denominator carry guard is unreachable for these callers.
- psi.c:106 exchange lo and hi in exact arf_add: exact addition is commutative.
- psi.c:63 carry +1->0: the equal-denominator carry guard is unreachable for these callers.

For :63, inspect the four callers of psi_sub. The centre/midpoint subtraction has two phases in [0,1).
If their denominators agree, that denominator is dyadic; each numerator has at most denominator_bits-1 bits.
Its extra carry fits the cap. Quarter/b has equal denominator only at 1,2,4. The one/u call has equal
denominator only at u=0, handled before this guard. The distance/radius call has a mag dyadic radius,
with at most 30 mantissa bits and the stated exponent bound, strictly below the bit cap. The guard cannot fire.

For :187, an added division either divides by one or divides zero. It preserves the rational distance.
When B=1, the preceding one-u subtraction has already required denominator_bits+1<=cap.
When u=0, the added check can reject only B with cap bits. At least one of the four distinct quarter targets
then has nonzero t-b, and its projected multiplication by B already forces LIMIT in the original function.
Default and strict perform all four preflights, so the combined status is unchanged in that boundary case.

The 7 compile failures were :236 removed cosine status assignment (unused helper), :208 missing work
initialization, :27 removed radius copy (unused parameter), :144 two mixed &&/|| changes under -Werror,
:50 missing exponent initialization, and :116 missing mantissa initialization. They are not test kills.
The original timeout was :167 cap equality changed to inequality. The instrumented recheck rejected it.

## Findings against the specification

SPEC:944, N-D21's final annotation, says that 0 mod 1/2 is not inside A_f. This is false under the
restricted-product definition quoted from refs/src/milne-cft/CFT.txt:9373-9376 and SPEC:106-109.

1. A point of (1/2) Zhat has components z_p/2 with z_p in Z_p.
2. At every prime other than 2, division by the unit 2 preserves Z_p.
3. The 2-component is in Q_2; only that one component may fail integrality.
4. Every such point is integral at all but finitely many primes. Thus (1/2) Zhat is contained in A_f.

The same argument applies to any rational centre and radius, outside the finitely many denominator primes.
This agrees with SPEC:74-77 and its section 6 phase-family statement. The implementation follows that meaning.
The finding concerns the decision annotation, not the additive-character formula. No SPEC file was changed.

## Findings against the design

api-3.md:523-524 says the rounding argument uses l<=m<=h. A rounded midpoint need not satisfy this:
l=h=1/3 and p=2 give m=3/8. The enclosure and excess conclusions remain valid. Put c=(l+h)/2 and
eta=abs(m-c); then max(m-l,h-m)=(h-l)/2+eta without that hypothesis. docs/api-3b.md supplies the argument.
This is a wording/proof-justification finding, not a production enclosure defect. No design source was changed.

## Files written or retained

Retained and verified: include/adelefeld/psi.h, src/psi.c, the one include in include/adelefeld.h,
the two commands in tools/adf/adf.c, tests/driver/psi-*.cmd and .out, tests/julia/psi.jl, and its registration
in tests/test_julia.sh. Modified: tests/test_psi.c and the owned tools/adf/README.md section.
Regenerated: tests/ref/vectors/q-slice3/psi.jsonl using the retained lane generator.
Added: docs/api-3b.md; run_faults.py, recheck_survivors.py, test_retry.c, test_lifecycle.c, audit.py,
cleanup.py in this lane. Updated redgreen.md. Kept small check/fault logs and TSV result tables in this lane.
This report was written after the work, tests, and cleanup. No git state-changing command or bd was run.

## Not done and sources pending

Class characters, local characters, adf_char, and Gauss sums are later slices, as the brief requires.
No overnight differential fuzzing, benchmark, or general allocator-leak audit was run.
LeakSanitizer was disabled; the focused lifecycle instrumentation does not replace a general leak check.
The historical initial red-green runs belong to the stopped run; their log is identified as inherited.
Conservative preflight may reject cancellations whose reduced answer would fit. This is documented.
There is avoidable exact modular/gcd work and temporary construction; no optimization was attempted.

[source pending: lawful readable Tate thesis to verify section 2.2 and Tate's own signs].
All signs used in the implementation are independently sourced from the opened Poonen notes above.
No pending source is needed for the own rounding argument or the restricted-product counterexample.
