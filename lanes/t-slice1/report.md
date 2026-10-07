# Lane t-slice1: slice 5a

Implemented adf_local_tate_at end to end. Plain, SAN, INV and clang checks pass, as do the driver and
Julia fixture. Valgrind reports zero errors and zero bytes at exit. LeakSanitizer cannot run under ptrace.
Two mutations of inherited Gamma bounds remain unresolved; they are listed below, not declared equivalent.

## Work and files

A. Added lanes/t-slice1/generate_vectors.py and tests/ref/vectors/t-slice1/local.jsonl.
There are 394 vectors and 1861 independently enclosed samples, in 325947 bytes. The six primes, six alpha
choices, conductor 1 and the requested prime powers, both real parities, exact/mixed/adjacent poles and
all three examples are covered. Conductor 65537 and its square are LIMIT descriptors; no large group is built.
E(1/3) is enclosed by an actual alpha ball, not misrepresented as an exact dyadic. Exact rational and
cyclotomic expressions accompany applicable values. Numerical references have 60 digits with margins.
The generator verifies each exported decimal interval contains its independent point enclosure.
The real reference uses the zeta oracle's integrated Taylor polynomial and tails, not FLINT Gamma.
Every input corner and centre is tested. The width test is 64 times a certified sampled separation plus
2^(-prec+8) times a whole-image upper bound. This is a test target, not a new API accuracy promise.

B. Added tests/test_tate_local.c. Every vector and sample is read. Calls repeat z=s, z=alpha and where=NULL;
z=s=alpha is tested separately on success and failure. Failure output bytes and where are checked.
The tests cover nonfinite inputs, invalid conductor, zero-containing alpha, huge regular values,
closed pole boundaries, precision 2/53/cap/one above, first LIMIT precedence and INV aborts.
The abort children clear their objects if the expected abort fails to occur, and disable core dumps.
The new finite precision guard is checked through first-log-call instrumentation on the target Linux ABI.
Inactive arf bytes are initialized before byte snapshots. FLINT process caches are cleared at test exit.

C. Appended the declaration/comment to include/adelefeld/localfactor.h, with its required char.h include.
Appended the function and private helpers to src/localfactor.c. Existing factor functions are unchanged.
Conductor 1 uses the geometric factor; exact alpha=1 delegates identically to the existing local factor.
The prescribed ramified integral is exact 1. It does not contain a Gauss sum. The real call delegates at s+e,
with an exact odd-integer pole check before rounding the shift. All value writes occur only after OK.
The output encloses 4/3; it does not promise an exact acb representation of that non-dyadic rational.

D. Added tate_local to tools/adf/adf.c and documented it in tools/adf/README.md. Both script and direct argv
calls use S with PLACE with ALPHA with CHI. Numerical lines print continuation mode and S.
Added hand-derived tests/driver/tate-local.cmd and .out, with the three examples, odd parity and poles.
Added tests/julia/tate_local.jl and registered its call in both paths of tests/test_julia.sh.
Julia obtains Place from the constructors and passes it by value. It checks containment and atomic failures.

E. Added docs/api-5a.md: value/enclosure steps, pole proofs, statuses, caps, cost, source lines and Check lines.
The local call uses the Characters, Gauss sums, local factors row at conventions.md:223, not the global
Integrals, Poisson row at :225. LIMIT is already in the former row. No specification or proof file was changed.

F. Added lanes/t-slice1/run_faults.py, prepare_mutations.py and scan_gamma_mutants.c.
There are 14 killed scratch faults. Two completed bounded mutation runs evaluated 40 records, 39 unique.
The first full-rebuild attempt timed out. Three later survivor gaps received tests and targeted replays.
The two inherited bound survivors received a further differential scan but no equivalence claim.
See redgreen.md, faults.tsv and the small logs for the records.

## Commands and results

Every test/script invocation used timeout. Compilations used at most two jobs; mutation workers used one
compiler each with two workers. No packages were installed. No git state command, bd, or check-all was run.
C headers identify FLINT 3.0.1. The oracle prints python-flint 0.8.0 / FLINT 3.3.1 and mpmath 1.3.0;
that Python FLINT version is not a claim about the C ABI.

- `timeout 120 python3 -B proto/tate_checks.py`: exit 0, 466 checks. Relevant groups: local_exact 34,
  local_numeric 24, local_poles 18, real 16; faults_51 has 6 witnesses.
- `timeout 120 python3 -B lanes/t-slice1/generate_vectors.py`: final exit 0, 394 rows, 1861 samples,
  325947 bytes. Earlier helper/fixture errors and corrections are recorded in redgreen.md.
- First C build with the test before the function: exit 2. After correcting test helper signatures and
  adding the declaration, the link failed on the missing adf_local_tate_at. This is the first-function red.
- Initial implemented C run: 4 tests, 22630 checks, 59 failures, caused by invalid fixture inputs and a stale
  reference imaginary component. Corrected those inputs. First green: 4 tests, 22438 checks, 0 failures.
- Large odd exact-pole regression: red 5 tests, 22454 checks, 1 failure; green 0 failures after the pole fix.
- Driver red: `timeout 180 sh tests/test_driver.sh`, exit 1, seven PARSE lines. The command and the actual
  character value grammar were added/corrected before the green run. Expected numeric lines were written by hand.

The four final builds used this command, with B and flags from the table:

```text
timeout 180 make -s -j2 BUILD=B FLAGS B/test_tate_local B/test_localfactor
```

| B under lanes/t-slice1/ | FLAGS | Build exit |
|---|---|---|
| build-plain | none | 0 |
| build-san | SAN=1 | 0 |
| build-inv | INV=1 | 0 |
| build-clang | CC=clang | 0 |

Both tests were run for every build with `timeout 120 B/test_tate_local` and
`timeout 120 B/test_localfactor`. SAN runs additionally set ASAN_OPTIONS=detect_leaks=0.

| Configuration | Tate tests/checks/failures | Existing factor tests/checks/failures | Run exits |
|---|---|---|---|
| plain | 6 / 22747 / 0 | 13 / 306730 / 0 | 0, 0 |
| SAN=1, leak detection disabled | 6 / 22747 / 0 | 13 / 306730 / 0 | 0, 0 |
| INV=1 | 7 / 22762 / 0 | 14 / 306749 / 0 | 0, 0 |
| CC=clang | 6 / 22747 / 0 | 13 / 306730 / 0 | 0, 0 |

`ASAN_OPTIONS=detect_leaks=1 timeout 120 B/test_tate_local` and the same existing-factor command
were also attempted in build-san: both reached zero failed checks, then exited 1 with LeakSanitizer's
fatal ptrace diagnostic. The successful leak-disabled runs check address/undefined behaviour, not leaks.

```text
timeout 120 /home/tobias/.local/bin/valgrind --error-exitcode=9 --track-origins=yes \
  --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=definite,indirect \
  lanes/t-slice1/build-plain/test_tate_local
```

Final exit 0: 6 tests, 22747 checks, 0 failures; 638886 allocations and 638886 frees;
0 bytes in 0 blocks at exit; 0 errors from 0 contexts. Earlier runs had 804, 220 and 2 errors from byte
snapshots of unspecified inactive arf storage. The test storage initialization and fresh cap sentinel
removed them. Clearing FLINT caches removed the remaining reachable/tagged cache allocations.

- `timeout 30 python3 -B tools/memcheck/check_uninit.py src/localfactor.c tests/test_tate_local.c`:
  exit 0; use-before-init 0, clear-before-init 0, init-without-clear 0.
- `timeout 180 sh tests/test_driver.sh`: final exit 0, 96 cases, 101633 expected lines;
  its 100000-line case took 0.153 s. The last run is in driver-final.log.
- `timeout 30 build/adf tate_local '((2) + (0)*i ; 0)' with 2 with '((1) + (0)*i ; 0)'
  with 'char(q=1, n=1, s=(0) + (0)*i)'` (one shell line): exit 0, prints an enclosure of 4/3.
- Shared fixture library: `timeout 180 make -s -j2 BUILD=lanes/t-slice1/build-shared
  CFLAGS='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fPIC' all`: exit 0.
  Linked with `timeout 120 cc -shared -o lanes/t-slice1/libadelefeld.so -Wl,--whole-archive
  lanes/t-slice1/build-shared/libadelefeld.a -Wl,--no-whole-archive -lflint -lgmp -lm`: exit 0.
- `timeout 60 julia --startup-file=no tests/julia/tate_local.jl lanes/t-slice1/libadelefeld.so`:
  exit 1 at dlopen, undefined __gmpn_modexact_1_odd, the existing Julia bundled-GMP mismatch.
- Same command with `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10`: exit 0; 23 checks, 23 passes.
  The final fixture was run directly; the entire Julia suite was not run.
- `timeout 180 python3 -B lanes/t-slice1/run_faults.py`: final exit 0, 14 compiled faults, 14 killed.
- `timeout 30 python3 -B lanes/t-slice1/prepare_mutations.py`: exit 0, staged dependency archive and 2 tests.
- Formatting scripts under `timeout 30 python3 -B`: 0 overlong lines in each new source/prose/helper file;
  every checked file ends with a newline. JSONL keeps the required one-record-per-line machine format.

## Fault table

All applicable scratch C faults compiled and made test_tate_local exit 1. Counts are failed assertions.
The inverse fault replaces the canceled unit integral by the wrong zero average; the non-real (5,2)
character is separately checked to distinguish eta0 from conj(eta0).

| Fault | Failed checks |
|---|---|
| Inverse unit-character cancellation omitted | 1026 |
| alpha omitted from the geometric factor | 780 |
| Real parity omitted | 46 |
| Unit volume doubled | 1025 |
| alpha^-1 used for alpha | 603 |
| Odd real integral uses the trivial pole list | 42 |
| where omitted on failure | 397 |
| z written before validation/pole check | 1520 |
| New finite guard cap raised | 1 |
| Odd regular s=0 falsely treated as a pole | 2 |
| Exact-power recognition boundary excluded | 1 |
| Out-of-bound integer falsely treated as a pole | 2 |
| Finite exponent sign reversed | 826 |
| Mixed pole returned as DOMAIN | 73 |

The six oracle faults_51 witnesses all pass as part of the 466-check oracle. In production 5a:
G_minus-positive and conductor exponent a=1 at conductor 4 have no mutation site. They concern gamma.
The oracle's alpha-omission witness also concerns gamma; its geometric-factor analogue was killed here.
The extra requested positive-kernel/G_minus-inverse faults therefore cannot be honest C mutations of 5a.
They are not counted as killed, and no unused Gauss-sum implementation was added to fabricate a target.

## Mutation runs and survivors

The first command was bounded by timeout 180, with ASAN_OPTIONS=detect_leaks=0:

```text
ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B tools/mutate/mutate.py \
  --root . --scratch lanes/t-slice1/mutations \
  --files src/localfactor.c --limit 20 --seed 51001 --jobs 2 --timeout 150 --san \
  --make 'timeout 120 make -s -j1 CC=clang SAN=1 INV=1 build/test_tate_local build/test_localfactor \
  && timeout 120 ./build/test_tate_local && timeout 120 ./build/test_localfactor' \
  --copy Makefile include src tests
```

Exit 124 at 180 s; baseline passed in 28.8 s; 2 running commands were terminated and scratch was removed.
It has no completed-run summary. Its false DOMAIN survivor outside the exact-power bound received a test.
The completed runs used the staged clang/SAN/INV archive, recompiling localfactor.c and both test programs:

```text
ASAN_OPTIONS=detect_leaks=0 timeout 180 python3 -B tools/mutate/mutate.py \
  --root lanes/t-slice1/mutation-root --scratch lanes/t-slice1/mutations \
  --files src/localfactor.c --limit 20 --seed SEED --jobs 2 --timeout 150 --san \
  --make 'timeout 150 sh run.sh' --copy Makefile include src tests prebuilt run.sh
```

SEED 51001: exit 1, 20 in 90.5 s: 12 killed, 6 survived, 2 compile refusals, 0 timeouts.
SEED 51002: exit 1, 20 in 80.5 s: 11 killed, 5 survived, 4 compile refusals, 0 timeouts.
There were 60 selections across the three invocations, including the timed/repeated first batch.
The completed batches cover 39 unique mutants and 40 records, with one overlap. All runs stayed inside
three minutes each and the total mutation work stayed below twenty minutes.

Survivor lines below refer to the source positions at the time of mutation. Three test gaps were then
killed by exact targeted replays. The shared tools/mutate/equivalent.txt is outside ownership and unchanged.

- :247 mag_mul(oldsum,oldsum,m0) -> mag_add: unresolved inherited previous Gamma-bound certificate.
- :504 guard > -> >=: equivalent; at equality prec+32 equals the cap in either branch.
- :438 guard cap subtraction -> addition: killed by first-log precision instrumentation, 1 failed check.
- :262 mag_mul operand swap: equivalent by commutative multiplication and permitted aliasing.
- :446 acb_mul operand swap: equivalent by commutative multiplication and permitted aliasing.
- :470 odd-pole >=0 -> >0: killed by the regular odd integral at s=0, 2 failed checks.
- :252 remove mag_mul(sum,sum,m0): unresolved inherited current Gamma-bound certificate.
- :417 exact-power > -> >= boundary: killed by the exact bounded pole, 1 failed check.
- :253 mag_add operand swap: equivalent by commutative addition and permitted aliasing.
- :344 mag_mul operand swap: equivalent by commutative multiplication and permitted aliasing.
- :167 acb_mul operand swap: equivalent by commutative multiplication and permitted aliasing.

For each unresolved bound, scratch renamed-object scans compared every accepted rectangle against
unmutated certified 512-bit exact-point factors. Build commands used timeout 120 cc with
-Dadf_local_tate_at=mut_tate -Dadf_local_zeta_factor_at=mut_local_zeta, then linked scan_gamma_mutants.c
against the ordinary archive. Each `timeout 120 lanes/t-slice1/gamma-scratch/NAME` exited 0:
755 accepted boxes, 6795 exact-point samples, 0 missed enclosures. This is not an equivalence proof or a
long fuzz run. No reproduction of a missed value was found. Both survivors remain open review obligations.

## Findings, pending sources and limits

Findings against SPEC: no counterexample to the specified integral or pole/status rule was found.

HEADER-FINDING against design: api-5:33 says guards count against the precision cap, while :80 demands
exact delegation to the existing factor. That factor explicitly works at prec+32, including at the cap.
Both promises cannot hold simultaneously there. Preserve exact delegation on trivial/real paths and clip
only the new finite path. The header, source comment and api-5a state this choice.

Brief/design qualification: the prescribed ramified integral is 1, not alpha^a p^(-as) G_minus.
The latter is gamma in P9 and api-5:63. The requested Gauss-sum/kernel/exponent production faults are 5b work.
A place outside Q cannot be made by place.h; a forged handle violates a precondition and aborts under INV,
rather than constituting a valid DOMAIN test for this signature. Odd integral poles are -1,-3,...;
odd gamma poles start at 2. They were not conflated. No new oracle/proof formula defect was found.

Source obligations: the universal FLINT Conrey pairing identification remains pending as api-3d:283-290
already records. [source pending: universal FLINT Conrey pairing identification]
Inherited measure/holomorphic-integration premises of P9/P10 remain as recorded in api-5:402-403.
[source pending: inherited Haar/product compactness, Fubini/dominated convergence, holomorphic integration]
No new Gauss-sum sign theorem is assumed. On-disk sources read and used:
refs/src/flint-3.0.1/acb.rst:6-17,637-643,658-675,893-902 and
refs/src/tate-poonen/notes.txt:62-64. The project definitions/proofs are P9/P10/P11 and conventions 6.4/6.5.

Not done: gamma/epsilon or global calls; production Gauss-sum/kernel faults; a complete Julia-suite run;
LeakSanitizer under ptrace; an equivalence proof or killing case for the two inherited bound survivors;
a long differential fuzz run. No unrelated specification, validator or proof was changed.

Avoidable costs: an unused shifted acb is initialized in finite cases, and the optional pole recognizer can
construct a bounded power before a simpler sign/size comparison would reject alpha. No optimization was attempted.

Removed all six lane build trees, mutation-root, mutations, gamma-scratch and the lane shared library.
Fault scratch already removes itself. Existing milestone-2 files and its three vector files in this reused
lane directory were preserved; only the new local.jsonl counts against this slice's 400 KB vector budget.
The required driver script's ordinary build/ artifacts were not removed outside lane ownership.
New logs are below 100 KB. The harness stdout.log is capped separately to its final 80 KB.
