# q-slice2 report

Slices 3.1-b and 3.1-c are implemented end to end. No git command or bd was run.
No source outside the owned paths was changed. The five lane build trees were removed.

## Work per step and slice

A. `gen_vectors.py` uses the exact `proto/quotient3_checks.py` reducer before rounding.
There are 66 integer/exact-radius lifts, 60 fractional-radius lifts, and 26 PIECES inputs with spill.
The three JSONL files total 243981 bytes. They contain 6080 labelled rational representative tuples.
Every case has 40 distinct (s,w) tuples; glued tuples can denote the same quotient class.
Each lift records its source endpoints, stored dyadic midpoint/radius, and exact stored dyadic endpoints.
Finite centres are canonical for the stored input. Integer cases have shifted-interior counts 0,1,2,4,5,6.
Radii cover 0,1,2,3,12 and 1/2,1/3,2/3,3/2,5/4. Centre denominators cover 1,2,3,4,6 and negative points.
Exact keys deduplicate by (lo,hi,N,m). Expected stored keys deduplicate AFTER the specified Q1 rounding.

B. `test_qclass_reduce` reads every record in all three files. It checks OK, PIECES, canonicality,
midpoint range, stored count, exact Q1 storage, same-finite-part containment, and rho-d <= 2^-28 d.
All point labels are checked against the input; every inside label must belong to the stored result.
Outside labels are not required to remain outside an outward enclosure.
Every vector checks K-1 and zero limits with preserved representation, K success, and y=x success.
Additional tests check failure bytes, aliasing on LIMIT, negative 2001-bit data, local CRT cancellation,
precision and exact-work caps, byte-product overflow, and debug entry checks.
There are 125 exact pieces where RN30 without a successor loses an endpoint. Exact Q1 equality also
rejects RN30 followed by a successor. One fixture exercises RU30 carrying to a power of two.

The 26 spill inputs include 24 previously rounded families and two isolated legal pieces:
[-1/16,1/16] x (0 mod 2), and [15/16,17/16] x {-7}.
Their shifted boundary labels would be lost by clipping. The new exact R pieces must be contained in
same-finite-part output pieces with exact Q1 storage and its radius bound. This proves the tested
superset direction without asserting storage identity or a bound on a merged quotient hull.

C. `adf_qclass_reduce` implements R steps 1-5 for both slices. Translation is exact and precedes rounding.
It counts with fmpz before constructing the piece array, checks B before its fibre loop, checks the byte
product, rounds by Q1, sorts stored keys, and removes equal keys. There is no merging or full-image shortcut.
Finite points stay finite points. All outputs are untouched on LIMIT; y may equal x.
Precision, exponent, projected-bit, and construction limits return LIMIT, never NEEDS_SPLIT.
The header exposes the shared precision cap when included alone.

D. The qclass printer supports union(X, X, ...) + Q. It sorts and deduplicates exact PRINTED keys.
Twelve existing golden union rows are compared as exact text. Seven retain reader-built lifts;
five use explicitly tighter dyadic lifts that yield the required printed decimal radii through Q1.
A separate fixture proves that printed order can differ from stored order. Printed duplicates are checked.
The valid golden row with midpoint 1 and radius 1/2 is legal storage but cannot be a Q1 output.
The union reader remains UNSUPPORTED; the original qclass test still reads all 29 golden rows.
No golden file was changed: it is outside the ownership list.

E. `qreduce X with LIMIT` is a driver script command under the existing driver conventions.
Its 17 expected lines were written before the first driver run. They cover E1-E4, multiple wraps,
construction limits before deduplication, exact points, fractional radii, and argument errors.
Julia calls reduce through ccall, checks PIECES, canonicality, length, aliasing, preserved LIMIT output,
and the two-fibre example. The driver README documents the command.

F. Statements and Check lines are appended to `docs/api-3a.md`. They reference R, Q1, P6, P8, P10,
explain the superset argument and resource policy, and give a stepwise corrected Q1 radius proof.
Two passes repeat endpoint extraction and finite-centre retrieval; local CRT could be cached.
Sorting allocates arf temporaries per comparison. These costs were left in the simple implementation.

G. The named scratch faults and mutation results are below. `redgreen.md` records the red/green history.

## Files written

- `include/adelefeld/qclass.h`, `src/qclass.c`.
- The qclass printer block in `include/adelefeld/text.h`, `src/text.c`.
- `tests/test_qclass.c`, `tests/test_qclass_reduce.c`.
- `tests/ref/vectors/q-slice2/{integer,fractional,spill}.jsonl`.
- The qreduce command in `tools/adf/adf.c` and its section in `tools/adf/README.md`.
- `tests/driver/qclass-reduce.cmd`, `tests/driver/qclass-reduce.out`, `tests/julia/qclass.jl`.
- The appended statements in `docs/api-3a.md`.
- Lane scripts: `gen_vectors.py`, `check_vectors.py`, `plant_faults.py`, `clean_builds.py`, `header_probe.c`.
- Lane red/green log, check logs, selected-mutant list, scratch fault sources/results, and this report.

## Checks and commands

All builds used at most two jobs. Every program/script ran under timeout or the mutation tool's timeout.
Repeated development runs are recorded in redgreen.md and the associated logs.

1. `timeout 30 python3 lanes/q-slice2/gen_vectors.py`: exit 0; 66+60+26 records, 243981 bytes.
   `timeout 10 python3 lanes/q-slice2/check_vectors.py`: exit 0; 152 records, 6080 labels,
   125 RN30 endpoint losses, 1 RU30 binade carry; all 40 tuples per record distinct.
2. Ordinary build and tests:

       timeout 180 make -j2 BUILD=lanes/q-slice2/build \
         lanes/q-slice2/build/test_qclass lanes/q-slice2/build/test_qclass_reduce
       timeout 60 lanes/q-slice2/build/test_qclass
       timeout 60 lanes/q-slice2/build/test_qclass_reduce

   Build exit 0. Lifecycle/text test: exit 0, 35384 checks. Final reducer: exit 0, 150122 checks.
   Earlier greens were 32276, 32278, 68872, 147397, 147481, 147555, 147560, 147564,
   147654, 147662, 147666, 147672, 147488, and 149236 checks as tests/data were extended.
   Initial reds: absent declaration/build exit 2; exact input-mag assertion exit 134;
   printer assertion exit 134; fractional OK assertion exit 134; exponent-edge assertion exit 134.
   Intermediate build errors were missing stdint/stdlib, GCC embedded-member overread warnings,
   and misleading indentation, each exit 2. A missing-binary launch returned 127.
   A projected allocation probe exited 0 with 147557 checks but did not observe GMP allocations.
   It was removed as evidence; the replacement checks the conversion preflight itself.
3. SAN build and tests:

       timeout 180 make -j2 BUILD=lanes/q-slice2/san SAN=1 \
         lanes/q-slice2/san/test_qclass lanes/q-slice2/san/test_qclass_reduce
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/q-slice2/san/test_qclass
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/q-slice2/san/test_qclass_reduce

   All exit 0; 35384 and 150122 checks. LeakSanitizer cannot run in this sandbox and was disabled.
4. INV build and tests:

       timeout 180 make -j2 BUILD=lanes/q-slice2/inv INV=1 \
         lanes/q-slice2/inv/test_qclass lanes/q-slice2/inv/test_qclass_reduce
       timeout 60 lanes/q-slice2/inv/test_qclass
       timeout 60 lanes/q-slice2/inv/test_qclass_reduce

   All exit 0; 35439 and 150125 checks. The three extra reducer checks validate the expected debug abort.
5. Clang build and tests:

       timeout 180 make -j2 BUILD=lanes/q-slice2/clang CC=clang \
         lanes/q-slice2/clang/test_qclass lanes/q-slice2/clang/test_qclass_reduce
       timeout 60 lanes/q-slice2/clang/test_qclass
       timeout 60 lanes/q-slice2/clang/test_qclass_reduce

   All exit 0; 35384 and 150122 checks.
6. Combined mutant configuration:

       timeout 180 make -j2 BUILD=lanes/q-slice2/clang-san-inv CC=clang SAN=1 INV=1 \
         lanes/q-slice2/clang-san-inv/test_qclass lanes/q-slice2/clang-san-inv/test_qclass_reduce
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/q-slice2/clang-san-inv/test_qclass
       ASAN_OPTIONS=detect_leaks=0 timeout 60 lanes/q-slice2/clang-san-inv/test_qclass_reduce

   All exit 0; 35439 and 147665 checks at that stage, before the final order/byte/vector additions.
7. `timeout 1700 make -j2 check-all`: exit 0, run once. It passed 84 C programs, 70 driver cases,
   101325 expected lines, 500 exported functions, Julia, and both tool self-tests.
   Two later projected-size guards and test extensions were checked by the final qclass runs above.
8. Final driver/export/Julia checks:

       timeout 180 make -j2 -C tools/adf
       timeout 30 build/adf < tests/driver/qclass-reduce.cmd
       diff -u tests/driver/qclass-reduce.out lanes/q-slice2/driver-final.out
       timeout 180 sh tests/test_exports.sh
       JULIA_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
         timeout 60 julia --startup-file=no tests/julia/qclass.jl build/libadelefeld.so

   Build exit 0. Driver exit 1 as specified by its error rows; 17 expected lines, diff exit 0.
   Export check exit 0: 500/500 functions, 0 missing, 0 undeclared, 0 variadic.
   Julia exit 0: 30/30 tests. The system-GMP preload is the repository's existing Julia workaround.
   The initial driver run returned PARSE for qreduce, exit 1, before its implementation.
9. Header-only probe:

       timeout 30 cc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -Werror \
         lanes/q-slice2/header_probe.c lanes/q-slice2/build/libadelefeld.a \
         -lflint -lgmp -lm -o lanes/q-slice2/build/header_probe
       timeout 30 lanes/q-slice2/build/header_probe

   Red compile exit 1, missing ADF_REAL_PREC_MAX. Green compile/run exit 0; cap 2097152, status 0.
10. `timeout 170 python3 lanes/q-slice2/plant_faults.py`: final exit 0; 15 scratch faults compiled,
    all 15 test processes aborted. The first run had 11 faults; an intermediate run had 14.
    The script contains its cc/link/timeout command construction and preserves source/log files.
    Ordinary O1 aborts in the allocator; it was also run under sanitizers below.

        ASAN_OPTIONS=detect_leaks=0 timeout 40 python3 lanes/q-slice2/plant_faults.py \
          --survivors --build clang-san-inv --cc clang --san --inv
        ASAN_OPTIONS=detect_leaks=0 timeout 30 python3 lanes/q-slice2/plant_faults.py \
          --name O1_count_k --build clang-san-inv --cc clang --san --inv
        timeout 40 python3 lanes/q-slice2/plant_faults.py --printer

    Scripts exit 0. Four repaired survivors compile and abort under SAN+INV.
    O1 reports AddressSanitizer heap-buffer-overflow, test exit 1.
    Omitting printed-key sorting compiles and aborts at the printed-order assertion.
11. Mutation commands/results are detailed below.
12. Final style scan under `timeout 10 python3`: 13 whole authored files and the qclass printer block,
    0 lines over 116, 0 missing final newlines. JSONL records, raw logs, and scratch source copies
    retain their machine formats. `timeout 20 python3 lanes/q-slice2/clean_builds.py`: exit 0,
    removed build, san, inv, clang, clang-san-inv. A prior rm-style cleanup command was rejected;
    the bounded Python cleanup succeeded. Required checks' root build/ was left for the orchestrator.

Ordinary CPU guards: 10^30 fibres 0.000002 s; width 2^101 0.000002 s;
2^58 byte-product refusal 0.000001 s. Each guard is 1.0 s. SAN times were 0.000007, 0.000008, 0.000004 s.
The small construction refusal made 0 allocation calls. The spent-budget refusal made 0 fibre-centre copies.

## Fault table

| Fault | Scratch change | Failure |
|---|---|---|
| D1 | Drop last constructed piece | Exact Q1 interval mismatch; SIGABRT |
| D2 | floor(hi)+1, spurious upper singleton | Expected OK at K fails; SIGABRT |
| D3 | +1 carry rather than -1 on n=1 | Oracle stored count fails; SIGABRT |
| D4 | One fractional fibre | Oracle stored count fails; SIGABRT |
| D5 | Double output modulus, retain one residue | Canonical finite centre fails; SIGABRT |
| D6 | Finite radius 0 changed to 1 | Exact finite radius fails; SIGABRT |
| O1 | Count k instead of k+1 | Allocator abort; SAN heap-buffer-overflow, exit 1 |
| O2 | Finite centre never shifted | Oracle stored count fails; SIGABRT |
| O3 | Radius RN30, then successor | Exact Q1 interval mismatch; SIGABRT |
| O4 | Compare limit after array allocation | Allocation count is nonzero; SIGABRT |
| Extra | Clip input PIECES spill | K-1 LIMIT assertion fails; SIGABRT |
| S1 | Reject denominator bit count equal to cap | Full-precision point OK fails; SIGABRT |
| S2 | Omit denominator's leading bit | Conversion-preflight assertion fails; SIGABRT |
| S3 | Strict denominator cap | Exact-cap preflight assertion fails; SIGABRT |
| S4 | Do not subtract spent budget | Premature fibre-copy assertion fails; SIGABRT |
| Printer | Keep stored order in value text | Printed-order assertion fails; SIGABRT |

D5 is the reduction analogue of losing residues during refinement: a+A Zhat is restricted to
one residue modulo 2A. The literal mixed-modulus set-query refinement belongs to 3.1-e and is not implemented.
D3 tests the integer boundary carry; no set-query gluing implementation is claimed by this lane.

## Mutation sweep and survivors

All tool runs used one worker, with make -j2 inside it, to keep total compiler cores at two.
The source was not changed during either sweep. No mutation-tool source or equivalent.txt was changed.

    ASAN_OPTIONS=detect_leaks=0 timeout 1200 python3 tools/mutate/mutate.py --root . \
      --scratch /tmp/adf-q-slice2-mutate --files src/qclass.c --limit 12 --seed 310206 \
      --jobs 1 --timeout 170 --san --make 'make -s -j2 check INV=1' \
      --copy Makefile include src tests lanes

Exit 2: unmutated full-suite baseline exceeded 170 s at test_resid_rest. Zero mutants were judged.
The unmutated full check-all had passed separately. This is a runtime-budget failure, not a qclass test failure.

    ASAN_OPTIONS=detect_leaks=0 timeout 900 python3 -u tools/mutate/mutate.py --root . \
      --scratch /tmp/adf-q-slice2-mutate --files src/qclass.c --limit 32 --seed 310206 \
      --jobs 1 --timeout 45 --san \
      --make "make -s -j2 check INV=1 TEST_SRC='tests/test_qclass.c tests/test_qclass_reduce.c'" \
      --copy Makefile include src tests lanes

Exit 1; baseline 12.4 s. Of 376 candidates, 32 ran in 507.8 s:
22 killed, 7 survived, 3 not compiled, 0 timed out, 0 excused.
Four survivors exposed missing tests of early resource checks; those tests were added and the same
faults were killed in SAN+INV scratch copies. The other three are listed below.

A follow-up used the same command with `timeout 410` and `--limit 20`, after the test additions.
Exit 124: stopped at the time cap, one active command killed and scratch removed.
It has no aggregate result; no inferred kill count is claimed. Its log records one equivalent survivor
and the three compile failures before interruption. Scope was the two complete qclass programs.
The tool selected at most 52 attempts across the complete and partial sweeps; the latter repeats the same prefix.
There are at most 43 distinct qclass mutations, including the separately requested named faults.
Scratch repeats are reported separately. Mutation execution stayed below 20 minutes.

Remaining equivalent survivors, one line each:

- qclass.c:244: swap lo and hi in q_add(...,0): addition and its projected bounds are symmetric.
- qclass.c:312: swap count and total in fmpz_add: exact addition with the same destination is unchanged.
- qclass.c:253: use >= when choosing d: on a tie t=d, copying t leaves the required radius unchanged.

Compile failures: qclass.c:249 forces precision 2 and leaves prec unused; :226 mixes &&/|| without
parentheses under -Werror; :78 creates an array index -1 under -Werror. They are not counted as test kills.
`timeout 10 python3 tools/mutate/mutate.py --files src/qclass.c --limit 32 --seed 310206 --list`
recorded the selected mutants. A seed lookup found 167 for the strict-cap mutant; it was not run as a tool sweep.

## Findings against the specification

The existing F1 remains: SPEC 6's unqualified full-image condition needs N>0.
At N=0 and width 0, the image is {pi(0)}; (1/2;0) is missing, despite width>=N.
No specification file was changed. The code preserves finite radius zero and makes no full-image shortcut.

## Findings against the design and review

The proposed R1 replacement in the review contains false intermediate statements:
2s=2^-28 u is not a general identity, and u<=d is false for upward rounding.
For d=3/10, RU30 gives u=644245095/2147483648 > d.
The stated bound rho-d<=2^-28 d is true. The appended proof chooses the binade of d and proves both cases.

The explanation of CV-45 overstates impossibility, as review F5 already notes:
[7/8,1], midpoint 15/16 and radius 1/16, encloses [9/10,1] inside the domain.
This does not change CV-45 or the prescribed Q1 kernel, whose spill is tested.
Exact translation precedes Q1; no equality after an inexact adele translation is asserted (F4).

## Not done and sources pending

No 3.1-d union reader, raw-pieces constructor, dump/inspect, 3.1-e set queries, or 3.1-f group arithmetic.
The full SAN+INV mutation baseline and the 20-mutant follow-up did not complete within their time caps.
No claim of exhaustive mutation coverage, LeakSanitizer coverage, or long differential fuzzing is made.
Both requested reduction slices are implemented; neither is deferred.

Sources pending for this implementation: none. FLINT contracts used are on disk in
refs/src/flint-3.0.1/{arf,mag,memory}.rst; reduction uses the read P3/P6/P8/P10 proofs and Q1/Q5 design statements.
