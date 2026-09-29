# Lane s2-review4: adversarial review

## Findings

### 1. MINOR: the large-prime fuzz oracle does not independently check reduced

Location: tests/fuzz/diff_roots_padic.py:283 to 307, especially the metadata comparison at 291.
The small and medium branch compares reduced with an independent degree calculation. check_big does not.
It reads reduced but checks it through adf_rootlist_verify_entries, which shares normalise with the solver.

Input: p = 18446744073709551557, prec = 2, depth = 0. Put r_j = 2^99 + j for j = 0, ..., 5, and
f = (X - r_0)^2 product_(j=1..5) (X - r_j). All six roots have distinct residues modulo p.
The normalised polynomial is the product of the six distinct factors. Therefore reduced must be 1.

The unmodified library returns OK, reduced = 1, and the six correct balls. There is no production
counterexample here. In a scratch copy, shared_reduced.c changes only normalise's flag calculation:
it sets reduced = 0 when the largest coefficient has at least 512 bits. It leaves the polynomial and
root calculations intact. Both the solver and entries verifier use that changed calculation.

For this input the scratch library returns OK, complete = 1, reduced = 0, with six correct balls.
Its entries and completeness verifiers return 1. The unchanged check_big returns zero failures.
The polynomial has a repeated factor, so that accepted flag is false. A direct fault only in the output
assignment was caught; this finding concerns the shared computation, not absence of any flag check.

Reproduce, after the PIC build described below:

~~~sh
timeout 120 python3 lanes/s2-review4/faults.py
timeout 20 python3 lanes/s2-review4/flag_reproducer.py
~~~

The deterministic reproducer prints:

~~~text
original status= 0 reduced= 1 expected_reduced=1 check_big_failures= 0 details= []
shared_reduced status= 0 reduced= 0 expected_reduced=1 check_big_failures= 0 details= []
~~~

The unchanged fuzz script also accepted this scratch library for 8,859 inputs in 15 seconds:
7,570 OK, 679 NOT_DETERMINED, 610 DOMAIN; 2,671 inputs at primes of 21 to 64 bits.
That timed run is a smoke test. The deterministic input proves the missing independent assertion.
The repair is to compare reduced with the degree drop from f to the independently computed g in check_big,
as the other branch already does.

## Attacks without a production counterexample

### Independent arithmetic and lifting

Command: timeout 175 python3 lanes/s2-review4/oracle.py. Final result in oracle-content.log:
1,174 inputs, 0 failures. This is 433 modular-list inputs and 741 public solver inputs, across 39 primes.
The primes include one proved prime immediately below each 2^b for b = 33, ..., 64, a prime above 2^63,
131, 137, 251, 65521, 1048583, 4294967291, and 2^64 - 59. Duplicates are removed.
Each prime is proved again by adf_place_prime in probe.c. Python's probable-prime function is only a filter.

The modular-list inputs comprise:

- 234 products with planted roots, repeated factors and quadratics excluded by a Python power test.
  The inputs include zero and p - 1 as roots, nonmonic leading coefficients, 4096-bit signed offsets
  by multiples of p, and higher leading coefficients divisible by p.
- 78 reductions of degree zero or one, from integer polynomials of higher degree with signed coefficients
  of more than 4096 bits.
- 117 polynomials X^k - 1, k = 3, 8, 30. Python constructs all gcd(k, p - 1) roots and proves their count.
- Four polynomials of degree above 1000: X^1008 - 1 at 131, 1048583 and 2^64 - 59, and a product with the
  1001 distinct roots 0, ..., 1000 at 2^64 - 59. The last input has thousands-bit integer coefficients.

A modular case fails on a wrong count, any omitted or extra root, a wrong residue, or a list out of order.
The oracle computes products, modular powers, inverses and expected lists with Python integers.
It does not call the author's reference, FLINT's modular powering, gcd or root finder.

The public inputs comprise:

- 156 perturbed polynomials whose simple modular roots are constructed in advance. Python lifts every
  root independently, one digit at a time, to precision 5. These cases include a leading coefficient
  divisible by p and up to 1024-bit perturbations.
- 78 quadratic inputs: X^2 - a with a either a constructed square or excluded by the power test.
  Python lifts the two square roots to precision 7.
- 78 degree-drop inputs lifted independently to precision 3.
- 195 calls on products with two separate close pairs, at depths 0 through 4. Their derivative valuations
  are 3, 3, 2, 2, 0. These exercise the large-prime route inside descended classes.
- 78 calls with a repeated factor, large negative content -p^3 2^2048, a quadratic without a root,
  and the factor p X - 1 whose root is outside Z_p. The independent expected reduced flag is 1.
- 156 DOMAIN or argument-LIMIT inputs. A populated sentinel tests output preservation.

The 741 strict statuses were 429 OK, 156 NOT_DETERMINED, 39 LIMIT and 117 DOMAIN.
Complete lists supplied 975 independently expected certificates. Partial lists supplied 234 unresolved
classes. A case fails on a wrong status, centre, K, s, count, coverage, complete flag, reduced flag when
independently prescribed, verifier result, ordering, or a write to either output on non-OK.

The centre, K and s checks compare against a complete independent oracle for every complete list.
For partial lists the probe checks root coverage and that every ball holds one planted root and every
class holds a planted root. It does not claim to reconstruct every unresolved class independently.
The proofs, including why the perturbed polynomials cannot have additional roots in Z_p, are written
stepwise in oracle_proofs.md.

No unmodified input reached an internal abort. The six intentional zero-modular-polynomial aborts in
the author's test are hidden-function contract violations, not public inputs reaching S-D20.

### Missing-root faults in the unchanged differential harness

faults.py changes a copy of src/roots.c inside this lane. It leaves tests/fuzz/diff_roots_padic.py unchanged.

- missing_root.c drops one returned root after the candidate check. The fuzz script exits 1 on case 1,
  at p = 4294967291, prec = 4, depth = 0. A planted root lies in zero balls or classes, and the complete
  list differs from the oracle.
- small_gcd.c replaces d by 1 after the gcd call. The fuzz script exits 1 on the same case. All six planted
  roots are absent, and the complete list is empty.
- reduced_flag.c changes only the output flag for p > 1031. The script exits 1 on case 1 because the
  unchanged entries verifier rejects the flag.
- shared_reduced.c is the surviving fault in finding 1. It also changes the flag calculation used by
  the verifier. All other oracle comparisons still pass.

Thus the script would fail for the two planted missing-root defects. It asserts root coverage on the
new route. It does not independently assert every metadata field on the large-prime branch.

The original shared library was run through the unchanged script for 15 seconds, seed 294:
13,499 inputs, 11,575 OK, 1,030 NOT_DETERMINED, 894 DOMAIN; 2,687 medium-prime inputs and 4,127
large-prime inputs; 0 disagreements. This is a smoke test, not a long fuzz run.

### The smaller-gcd trust case

The count is trusted, as the header says. The candidate checks do not independently establish that
deg gcd is correct. A gcd of smaller degree containing only true roots is not caught by those checks.
The public completeness verifier repeats the same computation and has the same trust base.

The deterministic command timeout 20 python3 lanes/s2-review4/trust_probe.py uses f = X(X - 1),
p = 2^64 - 59, prec = 3, depth = 0. It prints:

~~~text
original status= 0 complete= 1 certs= [(0, 3, 0), (1, 3, 0)] entries= 1 verify_complete= 1
small_gcd status= 0 complete= 1 certs= [] entries= 1 verify_complete= 1
missing_root status= 0 complete= 1 certs= [(0, 3, 0)] entries= 1 verify_complete= 1
~~~

The last two lines are defect simulations in lane copies. They are not findings against correct FLINT
or against the current production library. The unchanged independent fuzz harness catches both.

The exact required statements, what the code checks, what it trusts, and the source quotations with file
and line are in trust_base.md. In particular, correctness depends on FLINT's integer reduction,
X^p modulo h, actual gcd degree, modular evaluation and field arithmetic. nmod_poly_roots supplies
candidates whose mathematical correctness and completeness are tested conditional on those dependencies.

The sources read were the seven files fetched by the author under refs/src/flint-src-3.0.1, and the
associated nmod_poly.rst and fmpz_poly.rst passages under refs/src/flint-3.0.1. The gcd wrapper dispatches
to Euclidean or half-gcd kernels; this review does not prove those kernels correct.
The powering buffers depend on degree, and the loop processes exponent bits. The root finder's only
explicit residue loop is conditional on p < 10, unreachable by the automatic route above 128.
No p-sized allocation or p-step enumeration was found on that route.

The fixed-seed splitter has no finite retry bound in its source. The header admits that limitation.
No contrary runtime promise was found. The generic verifier sentence saying it never aborts remains
imprecise beside its explicit S-D20 exception, already identified in the author's report. This review
found no valid input reaching that exception without a planted defect.

### Memory, status preservation and the author's test

The final 232-input replay covers complete and partial lists, DOMAIN, argument LIMIT, LIMIT during descent
with a hidden bits_max of 256, degree drops, high-word primes, content and repetition, and reuse of outputs.
It contains 32 modular-list inputs and 200 solver inputs: 84 OK, 48 NOT_DETERMINED, 20 LIMIT and 48 DOMAIN.
The checker verifies the expected statuses and preservation of populated outputs on non-OK.

Valgrind's final result: exit 0; 126,702 allocations, 126,702 frees, 3,113,976 total allocated bytes;
0 live bytes, 0 live blocks, 0 reported errors. Log: valgrind-content.log.
The ASan/UBSan replay also exits 0 with zero diagnostic lines and 232 checked output rows.
ASan instruments probe.c and src/roots.c; the remainder of the library and FLINT are not instrumented.

The initial Valgrind run reported 116 errors in my probe's memcmp of struct padding. This was a probe
defect. I replaced it with field comparisons and checked the sentinel's arrays and polynomial contents.
The next 216-input run reported 0 errors and 0 live blocks. It used 112,395 allocations and frees.
The final replay adds the 16 content/repetition cases and again reports 0 errors and 0 live blocks.

The first ASan/UBSan run exited 1 because LeakSanitizer cannot operate under this environment's ptrace.
It reported a fatal LeakSanitizer error, not a library address or arithmetic error. The final run uses
ASAN_OPTIONS=detect_leaks=0; leak checking is supplied by the separate Valgrind run.

The unchanged tests/test_roots_bigp.c was compiled into this lane and run: 5 tests, 825 checks,
0 failed checks, 0 failed tests. Its aggregate helper checks do propagate failures to the runner.
No assertion in that file was found to be incapable of failing its stated condition. This does not
claim exhaustive mutation coverage of that test file.

## Benchmark

Compiled bench/bench_roots_modp.c and bench/harness.c into this lane and linked build/libadelefeld.a.
Ran the source without edits from the lane directory so its results also stayed in the lane.
Command: timeout 175 ./bench_roots_modp --run --trials 3, with cwd lanes/s2-review4.

Result: exit 0, 11 primes, 65 degree/family cases, 16 polynomials per case, 3 trials, checksum 7407.
Machine: i7-1365U, pinned to CPU 2, FLINT 3.0.1, -O2 -g. The machine was shared with other lanes.
The full min/median/max results are in bench/results/2026-09-29T204307Z_roots_modp.txt.

Ratio is median evaluation time / median degree-route time:

| p | Geometric mean over families | Degree 32, split ratio |
|---|---:|---:|
| 61 | 0.602 | 0.226 |
| 127 | 0.946 | 0.374 |
| 251 | 1.753 | 0.650 |
| 509 | 3.105 | 1.174 |
| 4093 | 18.365 | 6.883 |
| 65521 | 229.174 | 83.757 |
| 1048573 | 2618.597 | 980.411 |

The geometric-mean crossover is between 127 and 251, consistent with the chosen power-of-two threshold 128.
The split degree-32 family crosses later. The benchmark does not establish an optimal threshold for every
degree and root count, and the author already states that distinction. No unsupported-crossover finding.

## Judgments on the three requested proof updates

Exact replacement text is in proof_replacements.md. docs/proofs/solvers.md was not edited.

- The S-D10/source-pending update is required for this one-word implementation. The C sources are present.
  The old wording about every prime and no one-word bound also needs to match the current finite place type.
  The earlier prime-width finding is not repeated as a new specification finding.
- Reject the assertion that Algorithm P step 3 covers only evaluation. It says to process every root.
  That specifies a set, not how it is found. Proposition 3.7 already gives both methods, and the partition
  proof needs only the completeness of that set. The file supplies an optional clarification, not a repair.
- The cost sentence P3.5(7) needs updating. Its p evaluations apply to the evaluation route, and the code
  evaluates the derivative at roots only. The replacement counts powering, gcd, candidate evaluation,
  derivative evaluation, child shifts and lifting separately, without inventing a worst-case split bound.

## Checks run, commands and results

Every test executable, Python script, compiler invocation and make command was under timeout.
All make invocations used -j2. No git command, bd command, installation or clean was run.
Source reads and log inspections are not computational checks.

Compile commands, with line continuations for readability:

~~~sh
timeout 180 make -j2
timeout 90 cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude \
  lanes/s2-review4/probe.c build/libadelefeld.a -lflint -lgmp -lm -o lanes/s2-review4/probe
timeout 90 cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Itests \
  tests/test_roots_bigp.c build/libadelefeld.a -lflint -lgmp -lm -o lanes/s2-review4/test_roots_bigp
timeout 90 cc -std=c11 -O2 -g -Iinclude -Ibench \
  -DBENCH_CFLAGS='"-O2 -g, static adelefeld library"' bench/bench_roots_modp.c bench/harness.c \
  build/libadelefeld.a -lflint -lgmp -lm -o lanes/s2-review4/bench_roots_modp
timeout 180 make -j2 BUILD=lanes/s2-review4/pic \
  CFLAGS='-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror'
timeout 90 cc -std=c11 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Iinclude \
  -c src/roots.c -o lanes/s2-review4/roots-san.o
timeout 90 cc -std=c11 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Iinclude \
  lanes/s2-review4/probe.c lanes/s2-review4/roots-san.o build/libadelefeld.a \
  -lflint -lgmp -lm -o lanes/s2-review4/probe-san
~~~

Results: every compile/build above exited 0. probe.c was compiled five times as its comparisons,
hidden-limit support and output layout were corrected or extended. probe-san was linked twice.
No failed compile was omitted. The original make built 17 library objects and build/libadelefeld.a.
The PIC make built the same 17 objects in the lane.

Run commands and results:

- timeout 175 python3 lanes/s2-review4/oracle.py ran four times. The first log oracle.log has
  38 primes, 646 solver inputs, 798 certificates and 0 failures. Adding the 33-bit prime produced
  oracle-final.log with 39 primes, 663 solver inputs, 819 certificates and 0 failures.
  After replacing the padding comparison and checking both outputs, oracle-verified.log reports
  the same 39 primes, 663 inputs, 819 certificates and 0 failures. The final oracle-content.log
  adds the content/repetition cases: 741 solver inputs, 975 certificates and 0 failures.
  All four runs exited 0. The bridge itself is run by timeout 170 inside the script.
- timeout 120 lanes/s2-review4/test_roots_bigp exited 0: 5 tests, 825 checks, 0 failures.
- timeout 175 ./bench_roots_modp --run --trials 3, cwd lanes/s2-review4, exited 0:
  65 measured cases, checksum 7407.
- timeout 120 python3 lanes/s2-review4/faults.py initially exited 1 because I expected the output-only
  flag fault to survive; the unchanged entries verifier caught it. All three copy compilations and
  shared links exited 0, and all three fuzz invocations exited 1 on case 1. Log: faults.log.
- The revised timeout 120 python3 lanes/s2-review4/faults.py exited 0. Four scratch copies compiled
  and linked with exit 0. missing_root, small_gcd and reduced_flag fuzz runs exited 1 on case 1.
  shared_reduced exited 0 after 8,859 inputs; original exited 0 after 13,499 inputs.
  Each child command is under timeout 90 and each fuzz run requests --seconds 15 --seed 294.
  The exact compiler, linker and fuzz commands and their exits are retained in faults-final.log.
  Each copy uses cc -std=c11 -O2 -g -fPIC -Iinclude -c, then cc -shared with the lane PIC objects
  except pic/roots.o, and -lflint -lgmp -lm. original.so links all 17 original PIC objects.
- timeout 20 python3 lanes/s2-review4/flag_reproducer.py ran twice, both exit 0, one prescribed input
  per library. The final root construction uses 2^99 + j so it lies inside the author's drawing range.
  The original prescribed case used 2^100 + j. Both show the same surviving false reduced flag.
- timeout 20 python3 lanes/s2-review4/trust_probe.py exited 0: one input against three libraries;
  2, 0 and 1 certificates respectively, all accepted by their own reruns.
- timeout 10 python3 lanes/s2-review4/mem_inputs.py ran twice, exit 0: first 216, then 232 replay rows.
- The Valgrind command below ran three times. The first two used the 216-row replay; the last used 232.
  Initial exit 99: 116 probe padding errors; 106,948 allocations and frees; 0 live blocks.
  Second exit 0: 0 errors; 112,395 allocations and frees; 0 live blocks.
  Final exit 0: 0 errors; 126,702 allocations and frees; 0 live blocks.

~~~sh
timeout 175 valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
  --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
  --log-file=lanes/s2-review4/valgrind-content.log lanes/s2-review4/probe \
  < lanes/s2-review4/memory.inputs > lanes/s2-review4/memory-content.outputs
~~~

The earlier log/output names were valgrind.log/memory.outputs and valgrind-final.log/memory-final.outputs.

- timeout 120 lanes/s2-review4/probe-san with memory.inputs exited 1 on the initial 216-row replay:
  LeakSanitizer fatal error under ptrace; log san.log.
- The final sanitizer command below exited 0 with 232 output rows and 0 diagnostic lines.

~~~sh
ASAN_OPTIONS=detect_leaks=0 timeout 120 lanes/s2-review4/probe-san \
  < lanes/s2-review4/memory.inputs > lanes/s2-review4/memory-san-final.outputs \
  2> lanes/s2-review4/san-final.log
~~~

- timeout 10 python3 lanes/s2-review4/check_memory.py first checked the 216-row memory-final.outputs:
  exit 0, 32 modular rows, 68 OK, 48 NOT_DETERMINED, 20 LIMIT, 48 DOMAIN, 0 failures.
  The checker was then extended with the replay. The commands with memory-content.outputs and
  memory-san-final.outputs each exited 0: 232 rows, 32 modular rows, 84 OK, 48 NOT_DETERMINED,
  20 LIMIT, 48 DOMAIN, 0 failures.
- A timeout 10 Python line-length check of oracle_proofs.md, proof_replacements.md and trust_base.md
  exited 0: 3 prose files, 0 lines longer than 116. It checks each split line's len against 116.

## Files written

All new source, evidence and scratch files are in lanes/s2-review4:

- probe.c, oracle.py, oracle_proofs.md: independent arithmetic, lifting, statuses and preservation.
- mem_inputs.py, check_memory.py: bounded replay generation and output assertions.
- faults.py, flag_reproducer.py, trust_probe.py: scratch defects and deterministic reproductions.
- proof_replacements.md and trust_base.md: judgments, exact proposed text and local source quotations.
- missing_root.c, small_gcd.c, reduced_flag.c, shared_reduced.c: full scratch copies of roots.c.
- probe, probe-san, test_roots_bigp, bench_roots_modp; roots-san.o; the four scratch .o/.so pairs;
  original.so; pic/*.o, pic/*.d and pic/libadelefeld.a: build products.
- oracle*.log, faults*.log, the per-copy compile/link/fuzz logs, original link/fuzz logs,
  flag-reproducer*.log, trust-probe.log, pic-build.log, test_roots_bigp.log, benchmark.log,
  valgrind*.log, san*.log, check-memory.log, memory.inputs and memory*.outputs: evidence.
- bench/results/2026-09-29T204307Z_roots_modp.txt and this report.

The authorised make also generated the normal build/ objects and static library.
brief.md and lane.log were pre-existing lane files and were not edited.

## Not done

No production fix or proof-file edit: those paths are read-only for this lane.
No exhaustive search over word primes, full-project check-all, whole-library sanitizer build,
mutation sweep or long differential fuzz run. The independent tests are constructed finite samples.
FLINT's arithmetic kernels and the actual fixed-seed splitter are trusted, not formally verified.
The surviving metadata fault is a test-oracle gap; no current-production wrong complete list was found.

## Sources pending

None needed for the claims made here. External algorithm statements are quoted from local refs/ files
with file and line in trust_base.md and proof_replacements.md. The independent oracle arguments are
written out in oracle_proofs.md. No claim of a deterministic bound for Python's prime filter is used.

## Findings against the specification

No new finding against docs/SPEC.md. The two stale proof passages and the optional Algorithm P
clarification are addressed in proof_replacements.md. They do not require changing the specification.
