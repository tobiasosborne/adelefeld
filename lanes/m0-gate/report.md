# Milestone 0 gate report

Date: 2026-09-28.

Verdict: GATE NOT PASSED.

Counts: 2 BLOCKER, 8 MAJOR, 6 MINOR. Decisions: 60 reviewed, 52 accepted, 8 rejected.

Read first: `docs/reviews/m0-gate/review.md`. It contains the evidence, locations, paste-ready replacements,
all 60 decision verdicts, the source audit, proof coverage, check commands and remaining scope.

## Blockers and majors

- G1 BLOCKER: scaled arithmetic cannot obey the lcm-context, no-allocation and global-fallback rules together.
- G2 BLOCKER: opaque context allocation, initialization failure and destruction states are not fully specified.
- G3 MAJOR: valid quotient dumps can contain several contexts, while the loader accepts only one binding.
- G4 MAJOR: the claimed C value-text fixed point fails; 0.13 rounds to 0.14, then to 0.15 on rereading.
- G5 MAJOR: generic Arb multiplication can make positive idele real inputs yield a ball containing zero.
- G6 MAJOR: pole-containing mixed balls have contradictory DOMAIN/NOT_DETERMINED instructions.
- G7 MAJOR: acb_poly normalization disagrees with the required positive length and preserved text coefficients.
- G8 MAJOR: the dump reference ignores max_items for standalone and nested context block counts.
- G10 MAJOR: the phase test requires a small single ball containing both +1 and -1.
- G11 MAJOR: PLAN 1.8 still forces global storage when only canonical cancellation leaves a local context.

Minors: G9 shared-context precondition enforcement in the oracle; G12 Boolean parentheses;
G13 quotient canonical-triple predicate; G14 string/result ABI details; G15 an uncited signed quadratic
Gauss evaluation; G16 omitted numerator reduction in Summary 26's full canonical-triple sentence.

The mathematical enclosure formulas survived the new refutation attempts. G16 changes one proof
sentence, not its canonical-modulus column. The requested 2.8e-14 analysis diagnostic comes from
finite-distance residue checks with epsilon=1e-14. The two independent value comparisons are near
1e-56, and swapping pole residues is detected with error 1.01456050167.

## Work done

Read CLAUDE.md, SPEC/PLAN/PERF 1.1, all of conventions 0.2, the requested proof repairs, the relevant
earlier review and closure records, seams R1-R9 and their supporting examples, the text parser,
the Python ring reference, and the lane finding sections. Every substantive SPEC [quoted] location
was checked at its cited physical source lines. No such citation was refuted.

Wrote independent exact/numerical checks and two disposable C probes. The probes confirm contract
counterexamples without modifying the reviewed implementation or its fixtures. Ran the existing text,
ring-reference, mutation, analysis and seams suites. Audited the final decision/finding counts and layout.

## Files written

- `docs/reviews/m0-gate/review.md`
- `docs/reviews/m0-gate/checks/README.md`
- `docs/reviews/m0-gate/checks/contracts.py`, `contracts.out`
- `docs/reviews/m0-gate/checks/flint_probe.c`, `flint_probe`, `flint_probe.out`
- `docs/reviews/m0-gate/checks/roundtrip.py`, `roundtrip.out`
- `docs/reviews/m0-gate/checks/proof_refutations.py`, `proof_refutations.out`
- `docs/reviews/m0-gate/checks/analysis_refutations.py`, `analysis_refutations.out`
- `docs/reviews/m0-gate/checks/source_audit.py`, `source_audit.out`
- `docs/reviews/m0-gate/checks/add_immediate_probe.c`, `add_immediate_probe`, `add_immediate_probe.out`
- `docs/reviews/m0-gate/checks/run_existing.py`
- `docs/reviews/m0-gate/checks/existing_text.out`, `existing_ref.out`, `existing_mutants.out`
- `docs/reviews/m0-gate/checks/existing_analysis.out`, `existing_seams.out`
- `docs/reviews/m0-gate/checks/final_audit.py`, `final_audit.out`, `reviewed_inputs.sha256`
- `lanes/m0-gate/report.md`

The extensionless probe files are generated executables, not production code.

## Checks: commands and results

Commands ran from the repository root. Below, `gate_checks=docs/reviews/m0-gate/checks`.
The review gives all commands in full context and the raw-output filenames.

1. `python3 -B $gate_checks/contracts.py`: exit 0. Final run: 14 records, 4 confirmed context-limit
   bypasses, 12 exact-cap and 27 closed-endpoint cases, mixed-context and polynomial examples.
2. `python3 -B $gate_checks/proof_refutations.py`: exit 0. Final run: 752732 exact checks, 0 failed.
   One separate S26 counterexample: (9,4,1) is not canonical; (1,4,1) is.
3. `cc -O2 -Wall -Wextra $gate_checks/flint_probe.c -o $gate_checks/flint_probe -lflint -lgmp -lmpfr`
   and `$gate_checks/flint_probe`: both exit 0. FLINT 3.0.1; input/product nonzero flags 1/0;
   zero-polynomial length 0; [1,0] polynomial length 1; 2 exact dump records for decimal-radius probes.
4. `python3 -B $gate_checks/roundtrip.py`: exit 0. Two specified-printer outputs are 0.14 and 0.15.
5. `python3 -B $gate_checks/source_audit.py`: exit 0. Final run: 10 source files, 30 opened ranges.
   Covers 15 substantive SPEC quoted locations and related evidence; 0 unsupported SPEC quotations found.
6. `python3 -B $gate_checks/run_existing.py text ref mutants`: exit 0. Its subprocesses were:

   ```sh
   python3 -B -m unittest proto/test_text_grammar.py
   python3 -B -m unittest discover -s tests/ref/tests
   python3 -B tests/ref/mutants.py
   ```

   Results: 26 tests in 12.636 s; 59 tests in 5.797 s; 58 mutation baseline tests with 0 failures/errors,
   18 mutants killed and 0 survived in 5.486 s. The golden inventory has 713 vectors.
7. `python3 -B $gate_checks/run_existing.py analysis seams`: exit 0. Its subprocesses were:

   ```sh
   python3 -B proto/analysis_checks.py
   python3 -B proto/seams_checks.py
   ```

   Results: 3339 analysis assertions, 0 failed groups, 54.094 s; seams 0 failures, 10.532 s.
   Seams' intentional mutants fail in 118/400, 142/300 and 72/300 cases.
8. `timeout 170s python3 -B $gate_checks/analysis_refutations.py --splitting`: exit 0.
   414 numerical bound comparisons, 0 failed; 17 pole-residue comparisons, max error 2.268058839e-20;
   4 instrumented splitting comparisons; 1 swapped-residue mutant killed, 0 survived.
9. `cc -O2 -Wall -Wextra $gate_checks/add_immediate_probe.c -o $gate_checks/add_immediate_probe`
   and `$gate_checks/add_immediate_probe`: both exit 0. CPU 2, affinity result 0, 5 trials x 3 chains
   x 51200000 instructions, checksum 1. Trials 1-4: immediate-add-1 0.045736..0.046009 ns;
   register-add 0.264163..0.264951 ns. This did not refute PERF's stated immediate-add assumption.
10. `python3 -B $gate_checks/final_audit.py`: final counts 16 findings and 60 decisions; final line audit
    0 lines over 116 columns; input hashes and golden count recorded in final_audit.out.

Check-development record: the first source extractor counted PDF form feeds as line separators;
it was corrected to count LF only, and rerun before source judgements. A diagnostic's manually written
record total was replaced by an actual counter. Exact checks first counted 749825; the additional
block-cancellation tests bring the final total to 752732. Intermediate final-audit runs found 2, then 1
overlong Markdown rows; those were shortened. No failing mathematical check was weakened or removed.

## Not done

No implementation, golden fixture, specification, convention or plan was changed. The G1-G3 interface
repairs still need review before freezing the affected header. No Julia binding or production C ABI
test exists yet. No quiet-machine benchmark or test on another processor was run. The mpmath checks
are numerical evidence; the repaired analytic proofs were separately read step by step.

No git command, tracker command or package installation was used. Only the owned paths were changed.
No computation ran longer than 170 s; at most two cores were used concurrently.

## Sources pending

G15: signed evaluation of primitive quadratic Gauss sums for the extra claim W_chi=1 for real characters.
The existing pending source lists in the proofs, seams and conventions remain pending unless the SPEC
quoted-label audit specifically covers them. In particular: Tate's own section number; reciprocity
names; general analytic/Gamma prerequisites; general-field background; FLINT aliasing/cleanup/Dirichlet
documentation; and Nemo layout compatibility. No claim that all proof imports were sourced is made.

## Findings against the specification

No repaired mathematical enclosure formula of SPEC was refuted. Its interface refinements need the
listed repairs: external-context identity (G3), real idele result certification (G5), and pole statuses
(G6). The false C text fixed point is in conventions and PLAN (G4). The phase-width acceptance error
is in conventions (G10), the lost raw-storage rule is in PLAN (G11), and G16 is in a proof sentence.
