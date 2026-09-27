# Gate checks

Run from the repository root. Only files in this directory and lanes/m0-gate are written.
Python commands use -B to avoid writing bytecode in other lanes.

- contracts.py records concrete contract/parser/reference examples; expected defects are asserted.
- flint_probe.c checks finite, valid inputs against installed FLINT 3.0.1.
- roundtrip.py applies the specified exact printer to that C probe's dyadic results.
- proof_refutations.py independently enumerates repaired algebraic claims with exact integers and fractions.
- analysis_refutations.py checks repaired bounds numerically and instruments the author's splitting test.
- source_audit.py opens the physical source lines cited by SPEC and related contract evidence.
- add_immediate_probe.c attempts to refute the PERF immediate-add assumption on CPU 2.
- run_existing.py runs unmodified suites sequentially with a 170-second subprocess limit.
- final_audit.py checks the review's structure/line lengths and records hashes of the reviewed inputs.

The commands and results are in ../review.md. Each .out file is raw captured output, without prose
reformatting. Numerical mpmath bounds are evidence, not certified interval proofs. The splitting mutant
changes a copied function in memory only. No production/reference file or golden vector is modified.

The two extensionless files, flint_probe and add_immediate_probe, are generated C probe executables.
They can be regenerated from the compiler commands in the review; they are not production binaries.
