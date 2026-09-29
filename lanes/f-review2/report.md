# f-review2

## Part B: closure of f-review1

These judgments concern the original findings and their original counterexamples. The broader limit
promise is still false: R1 and R2 below give new counterexamples with different causes.

| Finding | Judgment | Reproducer and observed result |
|---|---|---|
| F1 | CLOSED | boundaries: both exact endpoint inverses return OK and the expected fields |
| F2 | CLOSED | boundaries: both differences return OK and the expected zero-centred balls |
| F3 | CLOSED | boundaries: all three quotients return OK and the expected fields |
| F4 | CLOSED | check_invariants.py: 9 invalid calls abort, 0 missed checks |
| F5 | CLOSED | text_checks.py: the corrected identity holds, value -2/3 |
| F6 | CLOSED | test_lball runs the wrong-unit rejection test with 0 failed checks |

Reproducers, from the repository root:

    timeout 20 lanes/f-review2/boundaries
    timeout 65 python3 -B lanes/f-review2/check_invariants.py
    timeout 10 python3 -B lanes/f-review2/text_checks.py
    timeout 40 lanes/f-review2/build/test_lball

The first command checks 7 arithmetic cases and 9 canonical inputs; 0 cases fail. F4's nine calls are
place, is_exact, contains_zero, get_prec, set, swap, identical, set_rat and set_rat_ball. Each exits with
SIGABRT and names the called function. The invalid add control also aborts. F5 reads the current L0
proof, rather than restating the old equation. F6's current regression rejects exact 5 of valuation 1;
the full release lball executable reports 26 tests, 3,982,393 checks, 0 failed checks.

## Findings

### R1. MAJOR: direct subtraction still returns LIMIT for an admitted small result

Location: src/lball.c:600; include/adelefeld/lball.h, Limits and sub contracts.
Put p = 2 and k = 33,554,433. Inputs:

    x = (1 + 2^k) + 2^(k+1) Z_2
    y = exact 2^k

Stored fields are x = (p=2, exact=0, u=1+2^k, v=0, N=k+1),
y = (p=2, exact=1, u=1, v=k, N=0). Both are canonical. The ball constructor also returns OK for x.
Their exponents lie within the documented bound. The input integer u has 33,554,434 bits, below the
67,108,864-bit bound. The code returns LIMIT.

Own calculation:

1. A point of x is 1 + 2^k + 2^(k+1) a, with a in Z_2.
2. Subtracting the only point of y gives 1 + 2^(k+1) a.
3. Every point of 1 + 2^(k+1) Z_2 occurs. This is the exact result set and the smallest ball.
4. Its stored fields are u=1, v=0, N=k+1, exact=0. No large integer is required to store them.
5. lb_addsub instead requires the intermediate alignment power 2^k. Its guard uses
   k bits(2) = 67,108,866 > 67,108,864 and returns LIMIT before the cancellation.

This contradicts the new rule that only an input or the result may cause LIMIT. It does not recreate
the old intermediate-negation defect. That particular defect is closed.

Reproduce: `timeout 20 lanes/f-review2/edge limits`. The sub constructor reports OK, both canonicality
checks report 1, and sub reports LIMIT. The command exits 0 because it prints the counterexample.

### R2. MAJOR: direct division rejects a quotient whose stored centre is 1

Location: src/lball.c:184, reached through adf_lball_div; lball.h, Limits and div contracts.
Use the same p and k as R1, with inputs:

    x = 1 + 2^k Z_2
    y = exact (1 + 2^k)

Their stored fields are x = (2,0,1,0,k), y = (2,1,1+2^k,0,0).
Both are canonical and their exponents and materialised integers are admitted. div returns LIMIT.

Own calculation:

1. y is odd, hence a unit at 2.
2. For a point 1 + 2^k a of x,
   (1 + 2^k a)/(1 + 2^k) = 1 + 2^k (a - 1)/(1 + 2^k).
3. Translation by -1 and division by this odd unit are bijections of Z_2.
4. The quotient set is exactly 1 + 2^k Z_2. Its fields are u=1, v=0, N=k, exact=0.
5. lb_make sees the rational unit 1/(1+2^k), skips its positive-integer shortcut, and insists on a
   full modulus 2^k for a modular inverse. Its guard returns LIMIT. The final centre needs no power.

The inverse is no longer stored as a separate lball, but the new promise about the result's storage
still fails. The original F3 cases are closed.

Reproduce: `timeout 20 lanes/f-review2/edge limits`. The div line reports canonical inputs 1,1 and LIMIT.

### R3. MAJOR: a complex component masks a higher-priority LIMIT at a prime

Location: src/sball.c:541; sball.h:39 and its binary-operation status paragraph;
docs/conventions.md:211, combining statuses.

Let E = 2^60. Both partial operands have the same places {inf,2}, arch=COMPLEX, inf=0+0i,
and an exact local component 2^E. Both partial balls are canonical and all input exponents are admitted.
Multiply them at prec=53.

The local multiplication returns LIMIT: the exact result would have valuation 2E > E. The complex
component has status UNSUPPORTED. The numeric precedence is LIMIT=10, UNSUPPORTED=8. The combined
status specified by the maximum rule is therefore LIMIT, with where=2.

adf_sball_mul instead returns UNSUPPORTED with where=inf, before inspecting any prime. The output
remains empty. The header's assertion that the component statuses cannot mix these codes is false.

Reproduce: `timeout 10 lanes/f-review2/edge precedence`.
Output: inputs_canonical=1,1, local_status=LIMIT, partial_status=UNSUPPORTED, where_inf=1,
output_empty=1. Exit 0.

### R4. MAJOR: six partial-ball entry paths omit the invariant check

Location: src/sball.c:116,128,172 and the arch and num_places accessors;
docs/conventions.md:288, invalid input and invariant builds.

An initialised empty partial ball with its arch field changed to 9 is non-canonical. In the invariant
build, these calls all return normally:

- adf_sball_arch: returns 9.
- adf_sball_num_places: returns 1.
- adf_sball_identical(x,x): returns 1.
- adf_sball_set(x,x): returns without checking x.
- adf_sball_swap(x,y): moves the invalid tag to y.
- adf_sball_swap(x,x): returns without checking x.

The required entry behavior is an abort with a function-naming message. The neg control does abort,
so the build flag is active. This is the debug contract, not a claim about release behavior on invalid
objects. A non-self set does check its input; its self branch returns before that check.

Reproduce: `timeout 140 python3 -B lanes/f-review2/edge_runs.py`.
Its seven invariant subprocesses give six exits 0 and one SIGABRT control. The script also runs R5.

### R5. MAJOR under the brief's crash criterion: LONG_MAX precision aborts

Location: src/rfunc.c, direct calls to arb_sin and arb_root_ui;
include/adelefeld/rfunc.h:42, the precision paragraph.

Input is the exact real ball 1. prec=LONG_MAX=9,223,372,036,854,775,807.
adf_real_sin aborts with an allocation request of 8,070,450,532,247,928,976 bytes.
adf_real_root of degree 3 aborts with a request of 17,293,822,569,102,704,648 bytes.
Both subprocesses receive SIGABRT. There is no returned status or value.

Reproduce: `timeout 140 python3 -B lanes/f-review2/edge_runs.py`.
The precision children run under timeout 10, with core dumps disabled and a 1 GiB address-space cap.
The requested allocations exceed that cap and any available laptop memory by many orders of magnitude.

Qualification: the actual header says that a precision arb cannot allocate is the caller's error.
Thus these are findings under the brief's explicit LONG_MAX crash criterion, not evidence of a wrong
enclosure or an out-of-bounds memory access. No finite upper precision bound is stated. The other
three precision probes return OK: log(1)=0, sqrt(1)=1, and exp(1) gives a finite broad enclosure.
The extra exp run prints `[+/- 5.01]`, which does contain exp(1); it is not an enclosure counterexample.

### R6. MINOR: the Julia realtext helper leaks its temporary arb

Location: tests/julia/sball.jl:95 and :118.

newarb allocates 48 bytes with calloc and initialises an arb. realtext obtains its component and
returns text. It neither calls arb_clear nor releases the allocation; it registers no finalizer.
arbtext releases only the returned string. No caller receives the temporary arb pointer to free it.

The focused Julia probe executes the actual helper definitions from the test file, calls realtext
20 times on the real projection of 2/3, and releases the caller-owned adele and partial ball.
Valgrind reports 20 definitely lost calloc blocks: 960 direct bytes and 640 indirect bytes,
1,600 bytes total. This matches 20 lost 48-byte balls and their mantissas. The JIT frames are unnamed;
the allocation sizes, count and the unreleased calloc site identify the helper's loss.

Reproduce with the direct Julia binary, not the juliaup launcher:

    JULIA_NUM_THREADS=1 JULIA_IMAGE_THREADS=1 OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 \
    LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 timeout 170 \
      /home/tobias/.local/bin/valgrind --leak-check=full --show-leak-kinds=definite \
      --errors-for-leak-kinds=definite --error-exitcode=99 --num-callers=20 \
      /home/tobias/.julia/juliaup/julia-1.12.5+0.x64.linux.gnu/bin/julia \
      --startup-file=no --threads=1 --gcthreads=1 lanes/f-review2/julia_leak.jl \
      lanes/f-review2/libadelefeld.so

Exit 99. See julia-valgrind-direct.log, loss record 3,036. The total process leak summary also contains
Julia runtime losses: 50,959 definitely lost bytes in 73 blocks, 640 indirectly lost bytes in 20 blocks,
18 error contexts. Those other losses are not attributed to adelefeld.

Static additional observations, not separately measured: adele_of does not clear its fn and fd
temporary fmpz values, and the author's test does not clear/free the three adele pointers it creates.
The small fn and fd used by that test have immediate integer representations, so their omitted clear
does not itself establish a heap leak in that run.

### R7. MINOR: L4a's stored-centre paragraph needs a zero-centre exception

Location: docs/api-1f.md:166. It calls k = K - (v-w) the result's relative precision and asserts that
the stored centre is obtained by an inverse modulo p^k.

Take x = 2^-5 Z_2 and y = 1 + 2 Z_2. The stored centre of x is zero, with u=0, v=0, N=-5.
For y, w=0 and M=1. The first term of K is absent because u=0. The remaining terms give
K=min(-5,-4)=-5 and the paragraph's k=-5.

There is no positive integer modulus p^k for the claimed integer modular inverse. The actual centre
is zero, and no modular inverse is needed. A zero-centred ball has no fixed finite centre valuation
from which to compute a relative precision. The code correctly handles q=0 before this calculation.
This refutes the unqualified centre paragraph, not the principal quotient-set formula.

Reproduce: `timeout 10 python3 -B lanes/f-review2/text_checks.py`.
Its L4a line prints K=-5 and claimed_modulus_exponent=-5. Exit 0.

### R8. MINOR: S5 promises OK for every ball inside a domain without its range exception

Location: docs/api-1f.md:330, especially :334.

S5 says exp has domain R and that the wrapper returns OK when the ball is inside the domain.
Take the exact finite ball 2^1000 and prec=53. It is entirely in R, but adf_real_exp returns
NOT_DETERMINED. S7 and the earlier decision 8 correctly describe this non-finite-result exception;
S5's unqualified status sentence omits it.

Reproduce: `timeout 10 lanes/f-review2/edge s5`.
Output: input_finite=1 status=NOT_DETERMINED. Exit 0. The implementation's status is appropriate;
the inconsistent statement is in S5.

## Attacks without a counterexample

### Direct lball arithmetic and the quotient formula

oracle.py and bridge.c were copied from f-review1 into this lane, then extended locally for same-object
inputs. The oracle uses Fraction arithmetic and finite point images. It does not import the author's
reference and does not use library contains or equal_set to decide enclosure or the smallest hull.

- 4,740 arithmetic cases: 4,240 successful hulls and 500 expected non-OK results.
  The successful cases enumerate 1,010,640 exact rational point results. There are 644 output-alias
  calls, 4,200 predicate calls, 250 constructors and 250 accessor sets. All comparisons have 0 mismatches.
- Pair counts are 300 at 2, 300 at 3, 150 at 5, 100 at 7 and 200 at 2^64-59.
  Point parameters cover three full digits at 2 and 3, two full digits at 5 and 7, and seven selected
  digits at the large prime. Centres include negative rationals, zero, mixed valuations and exact inputs.
- The same-object extension checks 1,620 sub/div cases, 11,331 point images and 1,341 successful hulls.
  It has 279 expected failures and 3,240 calls with both input arguments pointing to one object,
  including output equal to that object. Valuations range from -4 to 4, relative precisions are 1,2,4,
  and numerator centres include zero. Release and sanitizer runs give 0 mismatches.
- A successful case fails if a sampled image point is not equal to an exact output or its difference
  from the result centre has valuation below N. It also fails if the minimum valuation of differences
  between sampled images differs from N. Two witnesses at that valuation rule out any smaller ball.
  Status cases fail if the status is wrong or the output fields change. Aliasing compares stored fields.

Referee reading of L4a: for a nonzero numerator centre and a zero-free divisor, substituting
M'=M-2w into L3 gives the stated three terms, with absent exact-operand terms. For a zero numerator
centre the first term is absent; for exact zero the result is exact zero. The zero-containing numerator
does not require an invertibility certificate. The denominator must still be zero-free. R7 identifies
the separate defect in the centre paragraph. The mathematical set formula has no established counterexample.

The formula alone does not prove the implementation's resource promise: R1 and R2 are storage-algorithm
counterexamples even though the result formula gives the correct small ball.

### Partial balls, projection, predicates and status outputs

sb_oracle.py computes rational point images, interval endpoints and local hull witnesses independently.
sb_bridge.c only transports raw fields and calls the library. Final release, sanitizer and Valgrind
runs agree, with 0 mismatches:

- 1,005 arithmetic cases, 2,010 successful output-alias calls, 2,060 real endpoint results.
- 56,797 local point results and 7,522 local smallest-hull checks, counting operations and projections.
- 310 successful projections with 2,522 prime components: 150 through the local backend, 160 global.
  Local contexts have blocks 4,9,5. Denominators may share their prime factors. There are explicit exact
  projections of 2/3 and -5/2, all six orders of {inf,2,2^64-59}, an empty place set, and a projection
  to the first 1,000 primes in reverse order plus inf. The real component is checked against its exact
  midpoint 3/2 and radius 1/4. The projected prime fields are compared with independently reduced residues.
- 4 repeated-place failures. The output is checked against the sentinel and where against the first
  repeated place in canonical order. Lists with several repeated places are included.
- 39 operation failure/alias cases: DOMAIN, UNSUPPORTED and LIMIT, in each of three output positions.
  Their reported places and unchanged output fields are checked independently.
- 1,800 partial predicate calls, including both argument orders and reflexive cases. Integer valuations
  of centre differences and exact real interval endpoints decide the expected answers.
- 19,683 additional complex-box predicate calls use small integer endpoints. Equality, intersection
  and containment are calculated from four integer interval inequalities. There are 0 mismatches.
- 72,480 raw canonicality cases, in release and sanitizer builds. Tags, finite/non-finite real and
  imaginary components, negative lengths, NULL storage, component ordering, prime/exact tags, rational
  gcd/sign conditions, centre signs, valuations and precisions are checked against a small-integer oracle.

A projection case fails on a wrong status, place order, real copy, prime field, point enclosure or
smallest-hull witness. An arithmetic case fails on a missing or mispaired component, a point outside
the result, a wrong hull exponent, a changed where on OK, or unequal aliased output fields.
A predicate or canonicality case fails on any boolean disagreement. Output checks compare stored
fields and rational values; they do not certify allocator-address identity or every padding byte.

### Real functions and roots

real_oracle.py uses mpmath intervals at 6,000 bits and exact Fraction input/output endpoints.
It imports no author reference. Its 3,660 cases have 2,475 OK, 876 DOMAIN and 309 NOT_DETERMINED results.
The OK cases check 12,375 point images, including both endpoints, midpoint and two interior points.
There are 0 enclosure, status, finite-output, where or arb-alias mismatches.

The final sanitizer adapter also checks partial output preservation on failures and aliasing of each
function at infinity. Each case is called at both levels with separate and aliased output. The earlier
release adapter had only the separate partial output call; its arb-alias checks were already present.

Precisions are 2,53,4096. Root degrees are 0,1,2,3,2^20,2^63,2^63+1. Inputs include exact zero,
zero endpoints, intervals crossing zero, negative intervals, wide intervals, and dyadic balls around
17 multiples of pi/2. A case fails if its exact endpoints imply another domain status, an output
interval misses the oracle interval of a point, a result is non-finite, an alias differs, or where changes
on OK. R5 covers the separate LONG_MAX subprocesses.

trig_image.py additionally checks the entire image interval, not just sampled points, using mpmath's
interval sin and cos at 6,000 bits. All 360 cases pass. They include the wide interval [-2^20,2^20],
17 balls around multiples of pi/2, and 40 random intervals, each at 2,53,4096 bits. A returned ball
that misses either endpoint of the interval oracle fails. This checks critical values inside wide balls.

The finite-exponent stress probe multiplies partial real components 2^(2^100). It gives canonical
inputs and output, status OK and a finite real component. No non-finite-OK counterexample was found.

### Cost for a valid small answer

cost.c constructs a canonical finite ball 1 + 6^e Zhat and projects to each of four primes separately.
The following times exclude construction of H and the finite input, and include the projection call:

| e | H bits | p | Status | Seconds | Returned centre and precision |
|---|---|---|---|---|---|
| 33,554,433 | 86,736,952 | 3 | OK | 6.281143 | u=1, v=0, N=33,554,433 |
| 33,554,433 | 86,736,952 | 2 | OK | 0.017671 | u=1, v=0, N=33,554,433 |
| 33,554,433 | 86,736,952 | 5 | OK | 0.009053 | u=0, v=0, N=0 |
| 33,554,433 | 86,736,952 | 2^64-59 | OK | 0.008581 | u=0, v=0, N=0 |
| 134,217,729 | 346,947,797 | 3 | OK | 35.626441 | u=1, v=0, N=134,217,729 |
| 134,217,729 | 346,947,797 | 2 | OK | 0.064254 | u=1, v=0, N=134,217,729 |
| 134,217,729 | 346,947,797 | 5 | OK | 0.031786 | u=0, v=0, N=0 |
| 134,217,729 | 346,947,797 | 2^64-59 | OK | 0.036310 | u=0, v=0, N=0 |

Construction of H took 0.259244 and 1.787207 seconds. Each result has constant-sized stored fields.
At 3, v_3(6^e)=e and the centre is already 1. The cost is incurred obtaining the valuation from a large
global radius, not storing the result. This establishes a slow OK case; no contract here supplies a
latency bound, so it is recorded as a cost attack rather than a mathematical or status violation.

### Memory checks

The final partial oracle under Valgrind makes 153,784 allocations and 153,784 frees, totalling
4,960,086 allocated bytes. It exits with 0 bytes in 0 blocks and 0 errors from 0 contexts.
The earlier run, before two exact projections and the status cases were added, had 153,532 allocations
and frees, 4,955,562 bytes, and the same zero leak/error results.

Address/undefined sanitizers with invariant checks find 0 diagnostics in the partial oracle, the
same-object lball oracle, the real oracle, the whole-image trig check and raw canonicality enumeration.
LeakSanitizer cannot run under the environment's ptrace setup. The initial canonicality sanitizer
run therefore exits 1 during leak checking. Subsequent sanitizer commands disable only leak detection.
Valgrind supplies the separate C leak check. Julia has the measured helper leak in R6.

## Commands and results

All commands below ran from the repository root. Every test program, script and build was bounded
by timeout at 180 seconds or less. Builds used -j2 and did not overlap another computation. The real
sanitizer oracle and the direct Julia Valgrind probe overlapped; each used one worker. Julia thread,
image and BLAS counts were set to 1. No git or bd command was run. No file outside this lane was written.

Build commands, each exit 0 and one archive produced:

    timeout 180 make -j2 BUILD=lanes/f-review2/build all
    timeout 180 make -j2 BUILD=lanes/f-review2/invbuild INV=1 all
    timeout 180 make -j2 BUILD=lanes/f-review2/sanbuild SAN=1 INV=1 all
    timeout 180 make -j2 BUILD=lanes/f-review2/picbuild \
      CFLAGS='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fPIC' all
    timeout 60 cc -shared -Wl,--whole-archive lanes/f-review2/picbuild/libadelefeld.a \
      -Wl,--no-whole-archive -lflint -lgmp -lm -o lanes/f-review2/libadelefeld.so

Release probe compiles used this command, substituting T with bridge, boundaries, edge, real_bridge,
sb_bridge, cost, canonical and complex_predicates. Every compile exited 0. bridge was compiled again
after the same-object modes were added. edge was compiled again after adding printing and edge modes.

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude \
      lanes/f-review2/T.c lanes/f-review2/build/libadelefeld.a \
      -lflint -lgmp -lm -o lanes/f-review2/T

Invariant compiles used this command for invariant_probe and edge. Both exited 0. edge's output
name was edge-inv; invariant_probe's output name was invariant_probe.

    timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS -Iinclude \
      lanes/f-review2/T.c lanes/f-review2/invbuild/libadelefeld.a \
      -lflint -lgmp -lm -pthread -o lanes/f-review2/T-inv

Sanitizer compiles used this command for sb_bridge, canonical, real_bridge and bridge. Each exited 0.

    timeout 60 cc -std=c11 -O1 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS \
      -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude lanes/f-review2/T.c \
      lanes/f-review2/sanbuild/libadelefeld.a -lflint -lgmp -lm -pthread -o lanes/f-review2/T-san

Execution commands and results:

| Command (lane paths below are relative to lanes/f-review2/) | Exit | Result |
|---|---|---|
| timeout 20 boundaries | 0 | 7 cases, 9 canonical inputs, 0 failures |
| timeout 160 python3 -B oracle.py | 0 | 4,740 cases, 1,010,640 images, 0 mismatches |
| timeout 65 python3 -B check_invariants.py | 0 | 9 invalid entries abort, 0 missed checks |
| timeout 20 edge limits | 0 | constructor OK; sub and div LIMIT, canonical inputs 1,1 |
| timeout 10 edge nonfinite | 0 | canonical input/output, OK, finite=1 |
| timeout 140 python3 -B edge_runs.py | 0 | 6 missed sball checks; 2 precision aborts |
| timeout 10 edge precedence | 0 | local LIMIT, partial UNSUPPORTED, where inf |
| timeout 10 edge s5 | 0 | finite input, NOT_DETERMINED |
| timeout 175 python3 -B real_oracle.py | 0 | 3,660 cases, 12,375 point images, 0 mismatches |
| timeout 175 python3 -B sb_oracle.py | 0 | final: 310 projections, 39 status/alias cases |
| timeout 30 canonical | 0 | 72,480 cases, 0 mismatches |
| timeout 10 python3 -B text_checks.py | 0 | F5 identity true; L4a k=-5 |
| timeout 175 cost | 0 | four OK projections; slowest 6.281143 seconds |
| timeout 175 cost 134217729 | 0 | four OK projections; slowest 35.626441 seconds |
| timeout 65 python3 -B self_oracle.py | 0 | 3,240 same-object calls, 0 mismatches |
| timeout 30 canonical-san | 1 | LeakSanitizer unavailable under ptrace |
| timeout 20 complex_predicates | 0 | 19,683 predicate calls, 0 mismatches |

edge_runs.py invokes each child as `timeout 10 edge[-inv] MODE CASE`. Its inv cases are arch, num,
ident, setself, swap, swapself, control; its prec cases are exp, sin, root, log, sqrt.
check_invariants.py invokes each of its ten children as `timeout 5 invariant_probe METHOD`.
An additional exp probe ran as follows, exit 0, finite=1 and output `[+/- 5.01]`:

    (ulimit -c 0; ulimit -v 1048576; timeout 10 lanes/f-review2/edge prec exp)

The following commands all exited 0 with no address or undefined-behavior diagnostic:

    ASAN_OPTIONS=detect_leaks=0 timeout 30 lanes/f-review2/canonical-san
    ASAN_OPTIONS=detect_leaks=0 timeout 175 python3 -B lanes/f-review2/sb_oracle.py \
      lanes/f-review2/sb_bridge-san
    ASAN_OPTIONS=detect_leaks=0 timeout 65 python3 -B lanes/f-review2/self_oracle.py \
      lanes/f-review2/bridge-san
    ASAN_OPTIONS=detect_leaks=0 timeout 175 python3 -B lanes/f-review2/real_oracle.py \
      lanes/f-review2/real_bridge-san
    ASAN_OPTIONS=detect_leaks=0 timeout 100 python3 -B lanes/f-review2/trig_image.py

Counts for the first four are those given in the attack sections. The last reports 360 whole-interval
trig cases, 0 mismatches. Partial oracle runs were repeated after the failure/alias cases and explicit
exact projections were added. The earlier counts remain in sb-oracle.log and sb-oracle-san.log;
the final counts are in sb-oracle-final.log and sb-oracle-san-final.log. All runs exit 0.

C Valgrind command, exit 0, final counts as in the partial-ball attack section:

    timeout 175 python3 -B lanes/f-review2/sb_oracle.py lanes/f-review2/sb_valgrind.sh

The wrapper executes valgrind with --leak-check=full, --show-leak-kinds=all,
--errors-for-leak-kinds=definite,indirect and --error-exitcode=99. Its stderr is c-valgrind.log.
The command ran once before the final additions and once afterward; both have 0 errors and 0 leaks.

Author checks, all exit 0:

    timeout 180 make -j2 BUILD=lanes/f-review2/build lanes/f-review2/build/test_lball \
      lanes/f-review2/build/test_sball lanes/f-review2/build/test_rfunc
    timeout 40 lanes/f-review2/build/test_lball
    timeout 45 lanes/f-review2/build/test_sball
    timeout 30 lanes/f-review2/build/test_rfunc
    timeout 175 python3 -B proto/functions_checks.py

The C counts are respectively 26 tests / 3,982,393 checks, 16 / 873,696, and 8 / 59,145;
each has 0 failed tests and 0 failed checks. The author's Python f-slice2 section reports 600 projection
cases, 21,600 tuple points and 200 real-reference cases. Its quotient check reports 12,729 cases.

The author Julia test ran with the same four single-thread environment variables and LD_PRELOAD as R6,
under timeout 60 using the direct Julia binary and these arguments:

    --startup-file=no --threads=1 --gcthreads=1 tests/julia/sball.jl lanes/f-review2/libadelefeld.so

Exit 0; 35 tests, 35 passed. The focused leak probe's Valgrind command is given under R6, exit 99.
An earlier otherwise equivalent command used /home/tobias/.juliaup/bin/julia. It exited 0 and ran the
20-call probe, but Valgrind inspected the launcher rather than the Julia process. Its zero-allocation
summaries are not leak evidence. The direct-binary run replaced it.

## Files written

Only lanes/f-review2/ was changed. Files created by this review:

- Sources: bridge.c, oracle.py, boundaries.c, invariant_probe.c, check_invariants.py, edge.c,
  edge_runs.py, real_bridge.c, real_oracle.py, sb_bridge.c, sb_oracle.py, cost.c, canonical.c,
  text_checks.py, self_oracle.py, complex_predicates.c, trig_image.py, julia_leak.jl, sb_valgrind.sh,
  and this report.md.
- Release binaries: bridge, boundaries, edge, real_bridge, sb_bridge, cost, canonical, complex_predicates.
- Invariant binaries: invariant_probe, edge-inv. Sanitizer binaries: bridge-san, real_bridge-san,
  sb_bridge-san, canonical-san. The Julia shared library is libadelefeld.so.
- Build directories: build/, invbuild/, sanbuild/, picbuild/; their archives, objects and dependencies.
  build/ also contains the three author test executables and support objects.
- Logs: build.log, invbuild.log, sanbuild.log, picbuild.log, author-build.log, author-lball.log,
  author-sball.log, author-rfunc.log, author-proto.log, author-julia.log, boundaries.log, oracle.log,
  invariants.log, limits.log, nonfinite.log, edge-runs.log, prec-exp.log, precedence.log, s5.log,
  real-oracle.log, real-oracle-san.log, sb-oracle.log, sb-oracle-final.log, sb-oracle-san.log,
  sb-oracle-san-final.log, sb-oracle-valgrind.log, sb-oracle-valgrind-final.log, c-valgrind.log,
  canonical.log, canonical-san.log, canonical-san-noleak.log, text-checks.log, cost.log, cost-large.log,
  self-oracle.log, self-oracle-san.log, complex-predicates.log, trig-image.log, julia-valgrind.log,
  julia-valgrind-direct.log.

The brief and harness files lane.log, stdout.log and session.id were not edited by review commands.

## Not done

- No implementation repairs, edits outside the lane, full repository suite, mutation run or long fuzz run.
- No exhaustive check over all admitted exponents, all word primes or all points in an infinite ball.
- No sanitizer instrumentation of the system FLINT shared library itself.
- No successful LeakSanitizer run. No attempt to attribute the Julia runtime's other Valgrind losses.
- No precision allocation attempt above LONG_MAX, or attempt to provide the huge allocations requested there.
- No certified complexity bound for global projection. The cost cases are measurements, not such a proof.
- No proof of real analytic theorems or of mpmath/FLINT interval implementation correctness.
- No claim that these bounded enumerations establish every enclosure on every input.

## Sources pending

refs/ contains manifests and fetch scripts, but not the source documents cited by the reviewed headers.
No external formula is quoted from memory. The counterexamples above have their own stepwise calculations.
The missing sources for the dependency and analytic claims remain:

[source pending: refs/src/flint-3.0.1/arb.rst, including lines 155,405,492,639-682,945,979,1050,1082,1101-1103]

[source pending: refs/src/flint-3.0.1/acb.rst, including lines 224,240,267,302]

[source pending: refs/src/flint-3.0.1/fmpz.rst:1142-1160, fmpz_remove and fmpz_invmod]

[source pending: refs/src/flint-3.0.1/padic.rst:15-18, reduced storage convention]

[source pending: local source document for certified n_is_prime, cited via ulong_extras.h:335]

[source pending: real intermediate value theorem and real/complex exponential normalisations,
already marked pending in docs/proofs/functions.md Lemma 2]

## Findings against the specification

No false statement of docs/SPEC.md 4 or 9 was established. R1-R6 concern implementation, API enforcement,
the brief's crash criterion, or test-helper ownership. R7 and R8 concern docs/api-1f.md. The status-class
gap reported in the earlier lanes remains: conventions 3.2 does not explicitly list the sball class or
the complete lball status subset. That gap does not excuse R3's explicit maximum-rule violation.
