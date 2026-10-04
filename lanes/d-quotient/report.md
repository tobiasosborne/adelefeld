# d-quotient report

Design and reference work is complete. Implementation awaits review and the decisions below.
Only owned paths were written. No git command, tracker command, package installation, or subagent was used.
Every Python execution had timeout 120. No execution reached that limit. Tests ran one process at a time.

## Interface in fifteen lines

1. Keep the existing qclass struct: form, len, owned adele array; contexts remain borrowed.
2. LIFT means the exact image set of one stored adele in A/Q.
3. PIECES means a union of quotient images, including real spill and exact finite points.
4. Init is LIFT zero; copy, swap, identity, invariant, layout, and entry queries are specified.
5. set_adele copies without rounding; set_rat returns the zero class without an arb conversion.
6. Raw pieces are validated, stably sorted by canonical global keys, and deduplicated.
7. Reduction splits fractional radius A/B, shifts rational centres, then splits real integer crossings.
8. The explicit limit counts constructed pieces before rounding and deduplication; overflow gives LIMIT.
9. Q1 gives a bounded outward rounding kernel with midpoint in [0,1].
10. Exact represented-set equality, containment, and overlap use Q2, including zero-radius fibers.
11. Rational translation copies the class; optional addition and negation operate on independent sets.
12. Missing typed text/dump readers, printers, inspection, and context-binding declarations are specified.
13. Default psi returns a rectangular acb enclosure of all phases; adele strict tests finite integrality.
14. Exact roots of unity use rational angles modulo 1; local, place, and class entry points are specified.
15. Phase extrema use four rational distances; Gauss tau keeps the positive finite sign, G_minus negative.

## Decisions

| ID | Recommendation | Alternative |
|---|---|---|
| D3-1 | Bounded SET query: status plus truth, OK or LIMIT | Unbounded bool; separately named undecided query |
| D3-2 | Count construction, Q1 rounding, explicit work/bit bounds | Merged count; tighter kernel; unbounded work |
| D3-3 | Class strict certifies each stored representative | Singleton-only semantics; provenance extension |

D3-1 needs an explicit exception to the current status-free set-predicate row.
D3-2 also adds LIMIT to the raw qclass-constructor row. The numerical thresholds are proposed policy,
not measured maximum safe workloads. Optional group arithmetic is included in that scope decision.
D3-3 permits status to change after reduction while preserving the character image.
No decision about the already fixed signs, default/strict adele split, or CV-45 is reopened.

## Findings against the specification

- F1: SPEC:402 and conventions:745 need the positive-radius qualification for width >= N to imply
  full quotient image. At N = width = 0, the class is just zero and misses (1/2 ; 0).
  This is a scope clarification if the preceding positive-radius hypothesis is inherited.
- F2: conventions:753-755 uses CMP_UNDECIDED while :200 makes SET predicates Boolean.
  SPEC:413-415 also allows undecided ball comparisons. Exact dyadic endpoints determine the stored sets;
  Q2 proves decidability. The nonsingleton interval [0,1] with finite ball 0 mod 2 equals itself.
- F3: the class strict rule is missing. The lift (0 ; 0 mod 1/2) fails the adele finite-integrality test;
  its two exact reduced pieces each pass. Both images are {+1,-1}. Current storage retains no provenance.
- F4: translation equality requires exact translation before rounding. Translating zero by 1/3 and
  rounding to midpoint 171/512 with radius 715827883/1099511627776 gives a strictly larger quotient set.
  P10.3 already states the correct containment result; the prose and PLAN test need that qualification.
- F5: conventions:735-739 overstates impossibility of staying inside the domain. The dyadic ball
  [15/16 +/- 1/16] = [7/8,1] encloses [9/10,1] and stays in [0,1]. Generic rounding can spill;
  exact representation of the original non-dyadic interval is what is impossible. CV-45 remains valid.

These witnesses are computed in check_examples_findings. No stated positive-radius proposition in
quotient.md was refuted. Its missing extensions are Q1 to Q5 in the design, with numbered proofs.
No source specification, convention, plan, or proof file was edited.

## Reference checks and failure criteria

The final required command exits 0 and ends with `15 checks`:

```text
timeout 120 python3 proto/quotient3_checks.py
```

Saved output: lanes/d-quotient/checks-final.txt. Each group prints a case count.
Counts below describe finite evidence; they are not counts of independent general proofs.

| Group | Cases | A failure would mean |
|---|---:|---|
| reduction | 288 | A mismatch among 19,470 rational membership comparisons, or translated pieces differ |
| count_limit | 18 | Wrong endpoint count, singleton count, or failure to refuse huge counts before loops |
| sets | 120 | A mismatch among 37,720 direct membership pairs with mixed moduli, points, and spill |
| arithmetic | 600 | Pairwise piece sums or negated pieces differ from the exact quotient result |
| rounding | 363 | Inward rounding, midpoint outside [0,1], violated Q1 bound, or missing planned spill |
| phases | 335 | Wrong local additivity, rational product formula, or finite split phase list |
| hulls | 98 | Four-distance extrema differ from explicit arc extrema by >= 1e-75 |
| local_images | 128 | Exact or exponent -3 through 3 image differs at primes 2,3,5,7 |
| ball_additivity | 180 | A mismatch among 1,354 endpoint/gap witnesses of independently summed phase arcs |
| golden_phases | 15 | Valid golden finite-angle lists differ exactly |
| golden_qclass | 29 | One of 17 printed set enclosures or 12 malformed-subset statuses fails |
| full_and_width | 66 | Fractional positive-radius threshold or one of six diameter/width cases fails |
| gauss_boundary | 17 | Primitive lowering, parity, positive sum, or a golden component enclosure fails |
| examples_findings | 11 | A displayed reduction or a findings/implementation witness fails |
| fault_witnesses | 12 | One of six quotient or six character wrong alternatives is not distinguished |

The Gauss check has 12 odd-character sign controls. It uses exact FLINT character angles and direct
mpmath summation; it does not call the FLINT Gauss-sum evaluator. Real numerical work uses 90 decimal
digits and margin 1e-75. Rational angles, finite balls, interval endpoints, and set comparisons are exact.
The numerical checks are not interval certificates. The C implementation must use the specified arb bounds.

Two malformed rows in psi_phases.tsv are reserved for existing parser acceptance tests, not checked here.
The qclass golden checker is a small exact reader for that subset, not a replacement for the C parser.
It checks expected-text enclosure, not bitwise real rounding or the decimal printer's fixed-point algorithm.
The twelve fault controls are explicit wrong alternatives, not a production source mutation run.

## Every executed check

**`timeout 120 python3 -B lanes/d-quotient/check_contract.py`**: First: exit 1, missing oracle import. Next 2:
exit 0, 5 assertions each.

**`timeout 120 python3 -B proto/quotient3_checks.py`**: First: exit 1 after 4 groups, p=20 spill assertion.
Next: exit 0, 10 groups. Next: exit 0, 13 groups.

**`timeout 120 python3 proto/quotient3_checks.py`**: Exit 0, 15 groups.

**`timeout 120 python3 proto/quotient3_checks.py > lanes/d-quotient/checks-final.txt`**: Exit 0, 15 groups.

**`timeout 120 python3 -B lanes/d-quotient/audit.py`**: 3 runs, exit 0: 1652, 1661, then 1662 lines; 40
declarations each.


The initial rounding test was retained. The candidate tight-RU-only kernel was replaced by the explicitly
documented RU30-plus-successor kernel, and its error bound was proved and tested. This did not weaken
the enclosure, midpoint, or positive-spill assertions. It does add a small documented outward margin.

Other checks/probes, all exit 0:

- `timeout 120 python3 -B -c 'import mpmath; print("mpmath", mpmath.__version__)'`: mpmath 1.3.0.
- `timeout 120 python3 -B -c 'import flint; print(flint.__version__); print(flint.ctx)'`:
  python-flint 0.8.0; its reported thread count was 1.
- Two inline width inventories invoked as `timeout 120 python3 -B -`:
  first found 33 overlength design table rows among 790 lines;
  second found 0 overlength rows in the 870-line design and 1 in the 699-line oracle.
- An inline owned-file table-reflow script invoked as `timeout 120 python3 -B -` reported 0 remaining
  overlength design rows. It also removed accidental leading plus signs from progress notes.
  The subsequent reproducible audit supersedes these inventories; inline bodies were not retained as files.

The final report formatting check used `timeout 120 python3 -B -`: 0 lines above 116 characters.
It reflowed the report's long command table into paragraphs before validating the final text.

Final static audit: 5 source files, 1662 lines <= 116, 3 Python AST parses, 40 unique declarations,
closed code fences, 7 required design labels, and existence of every explicit refs path cited in backticks.
This audit does not compile the proposed C declarations or validate all prose citations automatically.

## Files written

- docs/api-3.md: full 3.1/3.2 design and only their phase/sign constraints on 3.3/3.4.
- proto/quotient3_checks.py: exact reduce/psi oracles, supporting set/arithmetic/local/hull routines, 15 groups.
- lanes/d-quotient/check_contract.py: the initial five-assertion independent contract harness.
- lanes/d-quotient/audit.py: reproducible static checks of owned sources.
- lanes/d-quotient/checks-final.txt: final required-command output.
- lanes/d-quotient/progress.md: incremental design, proof, and check notes.
- lanes/d-quotient/report.md: this final report, assembled after the work and checks.

## Not done

No C implementation, header change, C/Julia build, ABI probe, driver execution, or allocation test.
The acceptance matrix specifies those checks for the implementation lanes; none is claimed as run here.
No production mutations, overnight fuzzing, or benchmark measurements were performed.
The exact oracle does not enforce the proposed C bit/exponent bounds; those remain explicit C tests.

No single-piece-only reducer was added: a list result plus explicit piece limit covers PLAN 3.1.
LIMIT is therefore used for list bounds; no proposed function misuses NEEDS_SPLIT as a limit status.
No finite-quotient projection was designed because SPEC 6 names none and the boundary carries the finite part.
No character constructor/evaluator, conductor-lowering API, Gauss algorithm, or gamma/epsilon design:
these belong to 3.3/3.4 or their own function lanes. Their phase type and sign boundary are specified here.

## Sources pending

1. A lawful readable copy of Tate's thesis to verify section 2.2 attribution and its own signs.
   The project signs are separately sourced from refs/src/tate-poonen/notes.txt:693-700 and :733-740.
2. FLINT dirichlet exponent/Conrey-pairing documentation under refs/.
   The oracle's chi_exponent interpretation follows the existing golden generator and matches its vectors;
   this is finite evidence, not a sourced theorem about the labeling. acb_dirichlet.rst:358-380 does
   source the positive Gauss sign and Conrey-number interface, but not the full exponent convention.

No pending source is used to replace the own proofs Q1 to Q5. No quadratic Gauss sign theorem or
classification theorem for continuous characters is required by those proofs.
