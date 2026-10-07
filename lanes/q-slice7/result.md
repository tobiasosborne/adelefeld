# Lane q-slice7: negation and addition of quotient classes (slice 3.1-f) - report

The slice is done end to end: header, code, test against the oracle, driver commands, Julia call, statements.
`adf_qclass_neg` and `adf_qclass_add` construct the exact negations or pair sums (real end points added
or negated exactly; finite part `(a+b) + gcd(N,M) Zhat`). They count R's construction over all pairs
before rounding and deduplication, then run R, Q1, sort and deduplicate.

## Files

- New: `src/qclass_arith.c`, `tests/test_qclass_arith.c`, `tests/ref/vectors/q-slice7/arith.jsonl` (391468 B),
  `tests/driver/qclass-arith.cmd` and `.out`, `tests/julia/qclass_arith.jl`; lane: `gen_vectors.py`,
  `plant_faults.py`, `check_survivors.py`, `redgreen.md`, `fault-results.tsv`, `survivor-results.tsv`, logs.
- Changed (append only or new branches): `include/adelefeld/qclass.h` (two declarations with the design's comment
  blocks appended after the set queries), `tools/adf/adf.c` (verbs `qneg`, `qadd`), `tools/adf/README.md`,
  `tests/test_julia.sh` (one block), `docs/api-3a.md` ("Slice 3.1-f" appended).
- `src/qclass.c` is unchanged. There is no `src/qclass_internal.h`.

## Shared helpers or a copy

I copied the bounded exact read and add, Q1, the storage keys and the R loop into `src/qclass_arith.c`, each with
its `src/qclass.c` line range. This is what q-slice6 did. Reasons:
- Sharing needs new hidden `adf_`-prefixed names, so it means editing several lines of `src/qclass.c`. Lane
  q-slice4 is editing that file at the same time.
- `tests/test_qclass_reduce.c` compiles `src/qclass.c` into its own unit.

A second copy of Q1 could drift from the first. The test guards against that: `x + 0`, `0 + x` and `neg(lift)`
must be bit-identical to `adf_qclass_reduce` of x and of the exactly negated adele, for 30 lifts.

## Steps, commands, counts

- **A. Vectors.** `timeout 300 python3 lanes/q-slice7/gen_vectors.py` writes 79 records (64 add, 15 neg).
  - Raw counts run from 1 to 427. 9 records have duplicates before deduplication, 1 after Q1.
  - Families: crossings of 0, 1 and several integers; moduli 2, 3, 4, 6, 12 and 360 with gcd 1, a proper
    divisor and equal; zero radius on one or both sides; radii 1/2, 2/3 and 7/360; negative centres; 2000-bit
    finite centres with modulus 3 or 0; a 1000-bit real midpoint; four `x + x` self pairs; PIECES inputs,
    including spill; the zero class.
  - Four pairs cross an integer only because of the exact end point addition: an upper end at 1 from an
    unrepresentable midpoint at 53 bits, a lower end at 2 at 2 bits, two points summing to 2^60+1/2, and two
    30-bit radii whose sum needs 61 bits.
  - The generator checks every record against the oracle's own `check_arithmetic` method (`compare` with the
    sums of the exact integer-modulus pieces) and against `direct_member` for each of the 40 points.
- **B, C. C test and code.** Red: link error. Green: `test_qclass_arith` passes 1206757 checks (INV build: 1206766).
  - Every record: the stored list equals the oracle's after Q1 and deduplication (this pins the kernel and
    the count). Every exact piece is enclosed by a stored piece with rho - d <= 2^-28 d. 40 input points per
    record are checked in C to lie in the inputs; their sum or negation, formed in C, lies in the result.
    Every stored radius is a normalised mag.
  - LIMIT at K-1, 0, -1, WORD_MIN and prec cap+1, with struct and member bytes unchanged.
  - Aliasing: z=x, z=y and z=x=y for add; y=x for neg. `y + x` is identical to `x + y`. `add_rat` and `add`
    commute.
  - Exact Q2 containment: neg(neg(x)) contains x, and x + neg(x) contains 0. 28 cases decided, 2 over the
    4000000 budget.
  - Also: the pair-count limit, 10^30 fibres and a 2^101 width refused in under 1 s, the byte product of 2^62
    pieces, the exponent and bit bounds, local inputs (blocks 8, 9, 5) identical to their global equivalents,
    3 INV aborts, and component boundary checks of the copied helpers (the test compiles the source in).
- **D. Driver and Julia.** `sh tests/test_driver.sh` before the commands: exit 1. After: exit 0, 76 cases,
  101433 lines.
  - 21 lines derived by hand, all matching on the first run. Among them: `qadd (0.75 +/- 0.25 ; 0 mod 3) + Q with
    (0.5 +/- 0.5 ; 1 mod 3) + Q with 2` gives `union((0.5 +/- 0.51 ; 0 mod 3), (0.75 +/- 0.26 ; 1 mod 3)) + Q`,
    and `error: LIMIT` with 1.
  - Julia `tests/julia/qclass_arith.jl` uses the section 7 ccalls: 23 of 23 pass.
  - `sh tests/test_exports.sh`: 518 of 518 declared functions exported.
- **E. Statements.** `docs/api-3a.md` "Slice 3.1-f" (items 22-23):
  - the sets each call represents, by reference to Q3, P6, P8, P10, Q1 and precision.md P1;
  - why the construction preserves the set; the count and the order of checks; statuses; bounds; cost;
  - "Check:" lines, a hand example, and choices where the design is silent.
- **Final checks.** Builds under `lanes/q-slice7/`, `make -j2`, every program under timeout:
  `test_qclass_arith`, `test_qclass_reduce` and `test_qclass_sets` exit 0 in each of plain, `SAN=1` (with
  `ASAN_OPTIONS=detect_leaks=1`), `INV=1` and `CC=clang`.
  - Counts: arith 1206757 checks (INV 1206766), reduce 223472 (INV 223475), sets 86357 (INV 86375).
  - LeakSanitizer ran and reported nothing. `sh tests/test_driver.sh`: exit 0.
  - Build trees, root `build/` and the scratch copies are removed. No lane file is above 100 KB.

## Faults (step F, scratch copies; `plant_faults.py`): 9 of 9 rejected

| fault | first failing check |
|---|---|
| finite gcd replaced by lcm | stored H differs (validate) |
| finite gcd replaced by N of x alone | rejected (validate) |
| sign of the finite shift under negation | stored A differs |
| arb_add of the real balls at 53 bits before R | stored end points differ (exact-end vectors) |
| limit compared after deduplication | K-1 not refused |
| z written before the last allocation | sentinel bytes changed on LIMIT |
| end points not reversed under negation | OK at K fails |
| pair loop over len(x) only | y + x not identical |
| pair row index k/len(x) | OK at K fails |

## Mutation (60 of 314 mutants)

Command: `--seed 310207 --san --make "make -s -j2 check INV=1 TEST_SRC='tests/test_qclass_arith.c'"`.
Result: 899 s; 41 killed, 2 timed out (an endless INV macro loop), 7 not compiled, 10 survived. New tests were
added for the survivors; in scratch copies (`check_survivors.py`) 7 of the 10 are now killed. Three remain:

- 184 `qa_add(s->hi, s->hi2, s->hi)`: equivalent. Addition is commutative and qa_add's bounds are symmetric.
- 87 `fmpq_add(z, y, x)`: equivalent for the same reason.
- 231 `j+1 <= fibres`: one extra centre addition after the last fibre. It changes the outcome only if that sum
  alone crosses the 2^21-bit bound. That centre already passed through R's fibre subtractions, so I found no
  input that reaches this case. `src/qclass.c` has the same line.

No entry was added to `tools/mutate/equivalent.txt`.

## Findings

- **Against the oracle.** `reduce` deduplicates per adele, so the raw count is not `len(reduce(...))`. The
  generator computes it separately, as q-slice2's did.
- **Fractional radii (the brief's question).** precision.md P1 covers them: its notation (:9-11) defines gcd of
  rationals as the generator of their subgroup, and Lemma 2 and Proposition 1 are proved for it.
  - FLINT `fmpq_gcd` (fmpq.rst:510-522) and the oracle's `rgcd` compute the same value.
  - gcd(N, 0) = N, and the sum of two exact points stays exact.
- **Design, not a defect.** Negating a LIFT goes through R and Q1, so it is widened. An exact lift of
  `adf_adele_neg(x)` would be exact (adele.h:138). I followed the design and recorded the alternative in api-3a.
- **No HEADER-FINDING.** The appended comment blocks add three things: the order of checks (as reduce), the
  INV entry checks, and "untouched on LIMIT".
- **Test bug found by INV.** The test's first sentinel was a non-canonical PIECES value. It was fixed in the
  test; the code was not affected.
- **Avoidable cost.** Each pair re-reads both stored entries (canonical triple, CRT for local data) in both
  passes.

## Not done

- No fuzzing: no long differential run was asked for or done.
- Union input in the driver waits for slice 3.1-d. The fixtures use lifts and the C tests build PIECES directly.
- No sources pending.
