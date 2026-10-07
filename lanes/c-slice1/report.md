# c-slice1 report

Slice a is implemented end to end: 22 public functions, value text, three driver commands, and Julia.
One conditional HEADER-FINDING remains for the predicate's setup-failure contract. Sources are listed below.
No SPEC or design file was changed. No git command or bd was run. No subagent was used.

## Work per step

A. The lane generator imports proto/char_checks.py and proto/text_grammar.py.
Five JSONL files total 189975 bytes, below 400 KB:

| File | Records | Bytes |
|---|---:|---:|
| inputs.jsonl | 1 shared list of 40 operands | 2548 |
| lower.jsonl | 1966 characters, q<=80 | 70375 |
| phases.jsonl | 285 primitive pairs, C<=40 | 79491 |
| gauss.jsonl | 45 primitive pairs, C<=64 | 33459 |
| texts.jsonl | 32 texts | 4102 |

There are 11400 exact phase/zero cases. The operands include negative integers, zero, multiples of C,
and positive/negative 2001-bit integers. Lowering records carry C, label, parity and order.
Gauss records contain exact rational mpmath coordinates and certified rational error bounds for tau and W.
The Gauss bound is the oracle's gauss result. W is certified independently by python-flint division.
Text records contain all char.tsv inputs and four exact s endpoints for each accepted text.
D1 overrides the older text reference at q=65537: the required result is LIMIT before setup.

B. test_char reads every record of all five files and all 17 gauss.tsv rows.
It checks the exact phases, zero flags, fields/order, status precedence and untouched output bytes.
Golden tau/W balls overlap their stored expected balls. Tau also overlaps FLINT's default Gauss sum at 256 bits.
W overlaps that FLINT value divided by i^e sqrt(C), computed at 256 bits.
The golden test uses the primitive character built from the parsed, lowered value.
It checks the P4 coordinate radius bounds and radii below 2^-60 at precision 128.
The identities tested are |tau|^2=C, tau(chi)tau(conj chi)=(-1)^e C, |W|=1 and W W_conj=1.
All eight real-character golden input rows contain W=1 after lowering. No universal sign claim is made.
Conjugate labels are computed in the test, without adding the deferred conjugation API.

Additional tests cover F1, (8,7)->(4,3) with exact tau=2i, (16,9)->(8,5), principal (8,1),
raw domain errors, nonfinite s, deep copies, self-aliases, printer refusal and decimal endpoint enclosure.
Both precision caps and constructor/parser stage order are checked before invalid INV inputs.
The primitive pair (65536,5) tests inclusive phase/chi/order bounds and a complete direct Gauss/root run at p=2.
Above-cap constructors invoke zero wrapped group setups. The measured refusals are below 0.010 seconds.
Setup and phase failures are injected to exercise preserved flags, rationals, scalars, bytes and acb outputs.
Overwide enclosing intermediates test each final coordinate certificate separately.
These injections test the guards; they do not claim that the phase API normally returns overwide balls.

C. The fixed struct owns only s. Every group and FLINT character is a call-local temporary.
Constructors use dirichlet_char_lower, including the nontrivial 2-adic examples.
chi_phase uses k/G->expo and a separate zero flag. chi uses adf_phase_get_acb.
The Gauss loop shares one initialized group, combines the exact chi phase with +a/C, and calls the phase API.
It adds coordinate endpoints exactly and rounds once with the prescribed radius kernel.
Root divides by positive sqrt(C), rotates by i^-parity, and checks both radius bounds before committing.
Precision and modulus limits precede INV/setup in bounded numerical calls. No context or cache is retained.
The umbrella header adds one include. The conventions row adds one clause naming raw constructors and D1.

D. char, chi and gauss are available as script commands and direct CLI calls.
The 17 fixture lines were derived by hand before the red run. Four direct CLI examples also match exactly.
Gauss prints both formatted balls only after both printers succeed, on one e/tau/W line.
Julia allocates through layout queries, uses GC.@preserve, calls the design's Gauss ABI,
frees its returned string, and clears all values in finally blocks. It is registered in test_julia.sh.

E. docs/api-3d.md states the return values, enclosure steps, P3/P4 arguments, statuses, caps and costs.
It explains the exact endpoint sum and final radius bound. It records the predicate finding and source gaps.
P1/P2 belong to later coset evaluation; they are read and identified, not claimed as implemented here.

F. Eleven named scratch faults were compiled and rejected. The 60-mutant sweep and repairs are below.
The three 3.3 faults involving strict cosets, hulls and the idele sign are outside this slice.
They are not counted as C faults killed by this lane.

## Files written

- include/adelefeld/char.h, src/char.c.
- The appended character reader/printer in src/text.c; one include in include/adelefeld.h.
- One clause in docs/conventions.md, row Characters, Gauss sums, local factors.
- tests/test_char.c and tests/ref/vectors/c-slice1/*.jsonl.
- The three character commands/direct calls in tools/adf/adf.c and their README section.
- tests/driver/char-values.cmd and char-values.out.
- tests/julia/char.jl and its registration block in tests/test_julia.sh.
- docs/api-3d.md.
- Lane scripts: gen_vectors.py, run_checks.py, plant_faults.py, recheck_survivors.py,
  check_calls.py, clean_builds.py; header_probe.c, cap_probe.c; redgreen.md and bounded check logs.
- Mutation selection/results, fault results, survivor results, and this report.

## Commands and results

All builds used at most two jobs. Test programs/scripts were bounded with timeout.
The detailed staged history is in redgreen.md. Final logs are under this lane directory.

1. `timeout 120 python3 -B proto/char_checks.py`: exit 0; 323350 checks.
   python-flint 0.8.0 uses FLINT 3.3.1; the oracle also requires its C adapter to be FLINT 3.0.1.
   `timeout 120 python3 -B lanes/c-slice1/gen_vectors.py`: final exit 0; 2329 records, 189975 bytes.
   Initial generation: 186956 bytes; adding exact s endpoints gave 187427 bytes;
   adding the shared operand record gave 189975 bytes.
2. Final character configurations, invoked by `timeout 180 python3 -B lanes/c-slice1/run_checks.py`:

       timeout 180 make -s -j2 BUILD=lanes/c-slice1/build lanes/c-slice1/build/test_char
       timeout 60 lanes/c-slice1/build/test_char
       timeout 180 make -s -j2 BUILD=lanes/c-slice1/san SAN=1 lanes/c-slice1/san/test_char
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/c-slice1/san/test_char
       timeout 180 make -s -j2 BUILD=lanes/c-slice1/inv INV=1 lanes/c-slice1/inv/test_char
       timeout 60 lanes/c-slice1/inv/test_char
       timeout 180 make -s -j2 BUILD=lanes/c-slice1/clang CC=clang lanes/c-slice1/clang/test_char
       timeout 60 lanes/c-slice1/clang/test_char

   Each build and program exits 0. Plain/SAN/clang: 129725 checks each. INV: 129797 checks.
   The 24 debug children abort as expected, including both swap/identity arguments and acb member aliases.
   Allocator callbacks observe 1800 FLINT blocks per 100 warmed cycles in plain/SAN/clang, 5800 in INV,
   and 0 blocks remaining after every cycle. This checks the observed FLINT allocations, not all GMP internals.
   Final cap refusal times: 0.000000159, 0.000000331, 0.000000169, 0.000000142 seconds respectively.
3. `ASAN_OPTIONS=detect_leaks=1 timeout 60 lanes/c-slice1/san/test_char`: exit 1.
   LeakSanitizer reports a fatal error and states that it does not work under ptrace.
   The SAN run with detect_leaks=0 passes. No LeakSanitizer coverage is claimed.
4. Wrapped INV uses the same test file and INV archive. Its compile command is in run_checks.py:
   cc, -DADF_CHAR_WRAP_SETUP, -DADF_CHECK_INVARIANTS, and linker wraps for dirichlet_group_init,
   adf_phase_get_acb and arb_sqrt_ui. Compile under timeout 30, then
   `timeout 60 lanes/c-slice1/inv/test_char_wrap`: exit 0; 129825 checks.
   Final cap refusal: 0.000000213 seconds; 0 setups for all three constructor cap rejections.
5. Combined configuration:

       timeout 180 make -s -j2 BUILD=lanes/c-slice1/san-inv SAN=1 INV=1 \
         lanes/c-slice1/san-inv/test_char
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/c-slice1/san-inv/test_char

   Both exit 0; 129780 checks before the final 17 normalized-FLINT-W assertions.
   The separate final plain/SAN/INV/clang/wrapped-INV runs include those additional assertions.
   The wrapped combined run before the final boundary-only addition
   passed 129799 checks. All repaired survivor rechecks below use the final combined archive/test source.
6. `timeout 180 python3 -B lanes/c-slice1/run_checks.py text`: exit 0.
   It builds all eight tests/test_text*.c targets with make -s -j2 BUILD=lanes/c-slice1/build,
   then runs each binary under timeout 60:

| Program | Tests | Checks | Failed checks/tests | Exit |
|---|---:|---:|---|---:|
| test_text_adele | 20 | 84390 | 0/0 | 0 |
| test_text_classify | 8 | 20776 | 0/0 | 0 |
| test_text_fball | 10 | 21545 | 0/0 | 0 |
| test_text_idele | 14 | 73465 | 0/0 | 0 |
| test_text_limits | 11 | 105 | 0/0 | 0 |
| test_text_local | 10 | 68868 | 0/0 | 0 |
| test_text_r3r9 | 8 | 148 | 0/0 | 0 |
| test_text_rat | 13 | 18122 | 0/0 | 0 |

7. `timeout 180 sh tests/test_driver.sh`: exit 0; 78 cases, 101444 expected lines, 0 differences.
   The character fixture alone has 17 lines and expected process exit 1 for its error rows.
   `timeout 30 python3 -B lanes/c-slice1/check_calls.py`: exit 0; 4 CLI calls, 4 expected lines.
8. `JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 timeout 180 sh tests/test_julia.sh`: exit 0.
   Character smoke: 16/16 tests. The suite uses its existing system-GMP preload workaround.
   Its shared-library build also runs test_exports: 543/543 exported, 0 missing, 0 undeclared, 0 variadic.
   An earlier direct character Julia launch exited 1 because the shared library did not yet exist;
   the built-library direct launch and both final suite runs exit 0.
9. Standalone header probe: timeout 30 cc in C11, timeout 30 c++ compile/link in C++17,
   with -Wall -Wextra -Wpedantic -Werror and the lane archive; both timeout 30 programs exit 0.
   It checks initialization, canonicality, layout queries and cap 65536.
10. `timeout 30 cc ... lanes/c-slice1/cap_probe.c ...` and
    `timeout 60 lanes/c-slice1/build/cap_probe`: both exit 0.
    Constructor status 0, conductor 65536; direct Gauss status 0; 0.048656 CPU seconds including construction.
    The default test subsequently checks both Gauss and root at that conductor.
11. `timeout 180 python3 -B lanes/c-slice1/plant_faults.py`: final exit 0;
    11 compiles exit 0, 11 test processes abort (-6), 0 timeout. Earlier and final runs agree.
    The script records its complete timeout 30 cc/link and timeout 30 test commands.
12. Mutation and survivor rechecks are detailed below.
13. Final style scan: 16 authored source/prose files and five appended owned blocks;
    0 lines over 116, 0 missing newlines. Machine JSONL records/logs keep their machine layout.
    The existing conventions table row remains one line to honor the one-clause-only edit.
    `timeout 20 python3 -B lanes/c-slice1/clean_builds.py`: exit 0; removed five build trees,
    faults and survivors. Mutation scratch is absent. No lane log exceeds 100000 bytes.
    Root build artifacts of the mandated driver/export/Julia scripts are left for the orchestrator.
    check-all was not run.

Development failures were not concealed: missing declarations/build exit 2, the scaffold test helper error
exit 2, assertion reds exit 134, the incorrect nested-ball test exit 134, generator bytes/str error exit 1,
a subsequent missing s_bounds assertion exit 134, the first INV test compile exit 2, and missing-binary
INV launch exit 127. Each correction and the ensuing green is recorded in redgreen.md or the check logs.

## Named fault table

All fault copies were separate from src/char.c. Every final compile exits 0 and every test aborts (-6).

| Fault | Change | Independent rejection |
|---|---|---|
| 3.3 lowering | n mod C as label | Lowered oracle fields differ |
| 3.3 parity | n mod 2 | Oracle parity differs |
| 3.3 zero | nonunit reported as phase 0 with zero flag 0 | Exact zero flag differs |
| 3.4 sign | E(-a/C) | FLINT/independent positive tau differs |
| 3.4 conjugation | inverse label in sum | Independent tau differs |
| 3.4 principal | omit C=1 term | Expected tau=1 differs |
| 3.4 rotation | omit i^e denominator | Certified W interval differs |
| 3.4 conductor | denominator 8 for stored conductor 4 | Expected exact tau=2i differs |
| 3.4 exponent | k/order, reduced modulo 1 | Exact phase vector differs, including F1 |
| Extra cap | compare cap after setup | Wrapped setup count is nonzero |
| Extra transaction | write x before last raw check | Sentinel bytes differ |

The original six 3.3 faults also include strict-coset writes, a missing hull extremum and a dropped idele sign.
Those implementations are not in slice a. The Python oracle tests them, but that is not a C mutation kill.

## Mutation sweep and survivors

    ASAN_OPTIONS=detect_leaks=0 timeout 1100 python3 -u tools/mutate/mutate.py --root . \
      --scratch /tmp/adf-c-slice1-mutate --files src/char.c --limit 60 --seed 310308 \
      --jobs 1 --timeout 45 --san \
      --make "make -s -j2 check INV=1 TEST_SRC=tests/test_char.c \
        CPPFLAGS='-Iinclude -DADF_CHECK_INVARIANTS -DADF_CHAR_WRAP_SETUP' \
        LDFLAGS='-Wl,--wrap=dirichlet_group_init -Wl,--wrap=adf_phase_get_acb \
          -fsanitize=address,undefined'" --copy Makefile include src tests lanes

Exit 1. Baseline passes in 18.1 seconds. 60 of 315 candidates run in 1018.2 seconds:
44 killed, 5 survived, 9 not compiled, 2 timed out, 0 excused.
The selected mutations are saved by the same --files/--limit/--seed command with --list (exit 0).
No source or test was changed during the sweep. No mutation-tool file or equivalent.txt was changed.

Three survivors exposed test gaps. New tests reject the same mutations under SAN+INV:

- char.c:37: drop first swap INV check; the newly added invalid first argument is accepted and the test aborts.
- char.c:193: use OR for radius checks; one overwide coordinate is accepted and the certificate test aborts.
- char.c:136: reject C equal to 65536; the new primitive boundary phase call returns LIMIT and the test aborts.

    timeout 120 python3 -B lanes/c-slice1/recheck_survivors.py

Both recheck runs exit 0. Each has 3 compiles exit 0 and 3 test processes abort (-6), 0 timeout.
First script: 12.529 seconds; final script: 12.813 seconds. The script contains full compile/link commands
with SAN, INV and all three wrappers, and runs only the final test_char.
Only these three already selected mutations were rechecked; no further distinct sweep mutants were added.
The 1018.2-second sweep and both short recheck scripts are below the 20-minute mutation budget.

Remaining equivalent survivors, one line each:

- char.c:45: swap n_gcd arguments; the set of common divisors and thus the gcd is unchanged.
- char.c:240: remove the principal root shortcut; tau=1, sqrt(1)=1 and parity=0 give the same exact W=1.

Compile failures: char.c:211 (zero unset), :68 (mixed logical operators), :71 (parity unset),
:119 (order unset), :241 (work precision unset), :76 twice (parity/label unset),
:59 and :45 (mixed logical operators under -Werror). They are not counted as test kills.
Both timeouts change CHAR_INDEPENDENT's while(0) to an infinite loop (:17, two mutation kinds).
They are classified as timeouts, not assertion kills.

## Ground truth used

The explicit brief exception permits quotes from /usr/include/flint/dirichlet.h:

- :51: `ulong expo; /* exponent = largest order in G */`.
- :68: `int dirichlet_group_init(dirichlet_group_t G, ulong q);`.
- :90: `void dirichlet_char_init(dirichlet_char_t x, const dirichlet_group_t G);`.
- :110: `int dirichlet_parity_char(const dirichlet_group_t G, const dirichlet_char_t x);`.
- :111: `ulong dirichlet_conductor_char(const dirichlet_group_t G, const dirichlet_char_t x);`.
- :112: `ulong dirichlet_order_char(const dirichlet_group_t G, const dirichlet_char_t x);`.
- :114: `void dirichlet_char_log(dirichlet_char_t x, const dirichlet_group_t G, ulong m);`.
- :116-120: dirichlet_char_exp returns x->n.
- :136: dirichlet_char_lower(y,H,x,G), with y/x character and H/G group arguments.
- :139: `#define DIRICHLET_CHI_NULL UWORD_MAX`.
- :162: `ulong dirichlet_chi(const dirichlet_group_t G, const dirichlet_char_t chi, ulong n);`.

The local Gauss definition/sign is refs/src/flint-3.0.1/acb_dirichlet.rst:358-364.
Its :366-378 states the factor method and real/primitive/theta assumptions. Tests use only the default method.
Rounding and enclosure sources are arf.rst:24-35, mag.rst:6-17, arb.rst:6-12,951-953,1125-1138,
acb.rst:6-18,449-451,519-523, and memory.rst:9-24.
The implementation cites the read P3/P4 design and analysis Lemma 8/Proposition 13.

## Findings against the specification

No counterexample was found against the named SPEC statements. The specification was not edited.

## Findings against the design, oracle and goldens

HEADER-FINDING: api-3c section 1 requires is_canonical never to abort on initialized valid pointers;
N-D22 and the same design forbid predicate false for a resource failure. There is no error result in its int ABI.
The group-init failure semantics are missing. The implementation fail-stops if setup returns 0.
It never reports a false mathematical predicate for that failure. This is explicitly conditional on whether
such setup failure is reachable for a positive word-sized q; that cannot be settled from the header alone.
An error-bearing predicate would be the alternative, but this lane does not change the design declaration.

The brief's general print/read identity request is stronger than the design's decimal enclosure contract.
The code/tests follow the design: exact fitting s round-trips identically; general balls re-read to an enclosure.
The discarded nested-ball assertion was also stronger than that contract; exact input endpoint tests replace it.
No golden file or oracle was changed. (8,7) lowers to (4,3) as the design's normalization check requires.
The text oracle's older cap is handled in the lane generator, not presented as a C acceptance requirement.

## Sources pending and not done

- [source pending: FLINT 3.0.1 dirichlet pairing/exponent implementation under refs/].
- [source pending: full-word dirichlet_group_init failure semantics under refs/].
- [source pending: signed primitive quadratic Gauss evaluation, already pending in analysis P13].

`timeout 20 curl -LfsS` for the official FLINT 3.0.1 group_init.c URL exited 6: DNS resolution unavailable.
The signed quadratic evaluation is not used. Finite adapter/oracle evidence is not called a universal proof.

No adf_char_conj, unit-coset/class/idele evaluations, dump forms, products or CRT Gauss shortcut were added.
No long differential fuzzing, exhaustive mutation claim or LeakSanitizer claim is made.
Simple avoidable costs remain: H setup even when C=q, repeated canonicality setup under INV,
setup for each separate exact integer call, and two Gauss sums when the driver prints both tau and W.
