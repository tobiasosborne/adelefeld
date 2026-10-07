# d-char report

## Interface in fifteen lines

1. adf_char stores primitive conductor q, Conrey label n, parity, and finite complex ball s.
2. init/clear manage the principal initial value and the owned acb.
3. set/swap preserve all stored fields and allow whole-object aliasing.
4. is_canonical/identical and exported size/alignment queries describe storage.
5. set_conrey[_acb] lowers the input character; the primitive label comes from char_lower/char_exp.
6. set_s/get_s copy a finite exponent ball without rounding.
7. Conductor, label, parity, and character order have separate getters.
8. conj inverts the label and conjugates s; universal FLINT pairing identification remains pending.
9. set_str/get_str read and print char(q=q, n=n, s=z(s)).
10. load/dump/inspect use the existing char q n acb body and reject imprimitive dump input.
11. chi_phase returns a zero flag plus fmpq phase; chi returns an acb.
12. Unit-coset evaluation returns the rectangular hull; strict rejects finite ambiguity.
13. Class evaluation multiplies that hull by the positive-real power t^s.
14. Idele evaluation first calls adf_idclass_set_idele, including its real-sign correction.
15. gauss_sum/root_number compute positive-sign tau and tau/(i^parity sqrt(C)).

## Decision

D1 proposes a setup and direct-sum modulus cap of 65536. Calls reaching the cap check return LIMIT
before expensive work. Character constructors need an explicit placement in conventions row 223:
the generic raw-constructor row 202 does not permit their proposed LIMIT/UNSUPPORTED statuses.
The predicate remains the full word-sized predicate. Full-word FLINT setup failure semantics need a source.
The alternative is a larger measured cap. No decision about signs, strict behavior, or dump syntax is reopened.

## Findings against the specification

No counterexample was found to the named SPEC, PLAN, conventions, proof statements, or 17 numerical goldens.

Two corrections to the brief's formulas are necessary:

- Raw dirichlet_chi(5,4,2) is 2 with group exponent 4 and character order 2.
  The phase is 1/2. Dividing that raw integer by character order gives the wrong value 1.
- For (C,n,c,N)=(3,2,3,4), chi(c)=0 but the unit-coset phases are {0,1/2}.
  The multiplier must use a compatible unit lift modulo C.

The input (8,7) lowers to (4,3,1,2). Its golden tau=2i and W=1 are correct.
Conventions:929-930 lists input vectors; it does not assert every input pair is primitive.
An early draft misread that sentence. The final design makes no finding against it.

## Checks and rejection criteria

Counts below are assertions, not distinct characters or independent proofs.

| Group | Checks | A failing result would include |
|---|---:|---|
| constructors | 12 | wrong lowering, principal normalization, or raw-input rejection |
| lowering | 7864 | wrong conductor, primitive label, parity, order, or inducing values |
| characters | 192826 | wrong zero branch, inverse label, exponent scale, or multiplicativity |
| cosets | 11125 | disagreement with unit lifts modulo lcm(C,N), wrong singleton criterion |
| hulls | 20736 | any of four extrema disagreeing with root enumeration by more than 1e-50 |
| evaluation | 190 | a missed numerical sample, lost real sign, or failed rational rescaling |
| flint_c | 87111 | wrong ABI/version, lowering, exponent, Gauss overlap, or radius |
| gauss | 2565 | failed magnitude/product/root identities or signed additive twists |
| sum_bound | 812 | incorrect endpoint sum, excessive radius, or wrong exact cardinal case |
| goldens | 94 | parity/value mismatch in 17 rows or failure of eight real-input W=1 checks |
| faults_33 | 6 | failure to reject a named wrong finite-character result or strict write |
| faults_34 | 6 | failure to distinguish a named wrong Gauss/root result |
| findings | 3 | failure to reproduce the two brief issues and the (8,7) normalization |
| Total | 323350 | |

Lowering covers all 1966 characters at moduli 1..80. Character tests cover 285 primitive pairs at C<=40.
Cosets cover 5184 small cases from 108 primitive pairs at C<=24, plus a large-N case.
The sum-bound model uses 45 primitive pairs at C<=16 and p=2,20,53,128.
Python-flint 0.8.0 bundles FLINT 3.3.1; the separate C adapter requires compiled/runtime FLINT 3.0.1.
mpmath 1.3.0 evaluates at 60 digits. Gauss errors are exact rational bounds derived from a separate
256-bit certified phase sum. The hull comparisons use a numerical 1e-50 margin, not an interval proof.

Commands and results, including intermediate checks:

    timeout 120 python3 -B proto/char_checks.py

The initial lowering stub exited 1 with NotImplementedError: one test reached the missing operation.
After implementation this command, redirected to oracle.log, exited 0 with 322524 checks.

    timeout 120 python3 proto/char_checks.py > lanes/d-char/oracle.log 2>&1

Two subsequent runs exited 0: 322524 and 323349 checks as strict, idele, and sum-bound checks were added.

    timeout 120 /usr/bin/time -p -o lanes/d-char/oracle.time \
      python3 proto/char_checks.py > lanes/d-char/oracle.log 2>&1

Final run: exit 0; 323350 checks; real 5.35 s, user 5.31 s, system 0.04 s.
Each full run also compiles and executes the C adapter with separate 30-second subprocess timeouts.

    timeout 120 python3 -B lanes/d-char/mutate_oracle.py > lanes/d-char/mutation.log 2>&1

Two runs: each exited 0; 12 killed, 0 survived, 0 harness errors. Mutations changed executed Python code.
They cover lowering, parity, zero, strict writes, hull endpoints, real sign, Gauss sign, conjugation,
C=1, the factor i^e, use of input modulus, and the exponent denominator.
These are reference mutations. No future C mutation is claimed as killed.

    timeout 30 cc -std=c11 -O1 -Wall -Wextra -Werror lanes/d-char/flint_probe.c \
      -lflint -lmpfr -lgmp -o lanes/d-char/flint_probe
    timeout 30 lanes/d-char/flint_probe > lanes/d-char/flint_probe.tsv

Two standalone compile/run pairs exited 0. Both produced 1966 character rows.
The final transcript has two metadata rows: FLINT 3.0.1/3.0.1 and ABI size 120, alignment 8,
offsets 0,8,16,24. The first transcript had only the ABI metadata row.
The standalone executable was removed after the final run.

    timeout 15 python3 -B lanes/d-char/check_design.py

Two runs exited 0: 6 files checked, 3 Python ASTs parsed, 0 lines over 116 characters,
393 design lines against the 450-line cap. The report text was length-checked before its single write.

Seven earlier inspections used timeout 15 python3 -B -c with import/introspection or artifact-reading code.
All exited 0. They established the package versions; group exponent 4 versus order 2 at (5,4);
conductor 4 at (8,7); available acb phase and exact dyadic accessors; no Python Gauss-sum binding;
and intermediate line/AST counts. The last two artifact inspections found 0 long lines and parsed
2 Python ASTs; the final inspection also counted 45,108,285 primitive pairs at bounds 16,24,40.
These printed observations were then made reproducible by the final oracle, C adapter, and check_design.py.
Read-only source inventory found no dirichlet.rst or pairing source under refs/src; recursive rg also
reported the existing refs/src/src symlink loop.

## Files written

- docs/api-3c.md
- proto/char_checks.py
- lanes/d-char/flint_probe.c and flint_probe.tsv
- lanes/d-char/mutate_oracle.py and mutation.log
- lanes/d-char/check_design.py
- lanes/d-char/oracle.log and oracle.time
- lanes/d-char/progress.md
- lanes/d-char/report.md

No src, include, SPEC, PLAN, conventions, proof, or golden file was changed. No git command or bd was run.
Early adapter runs used temporary /tmp compilation directories, which were removed automatically.
The final oracle creates and removes its scratch directory inside lanes/d-char.

## Not done

No production C, driver command, Julia wrapper, parser, or dump implementation was written.
Their acceptance tests and three slices are specified; C transaction, rounding, lifecycle, and reader
fuzz tests have not run. The Python strict-output slot is only a reference transaction model.
The proposed 65536 cap has not been benchmarked at its worst case. Full-word predicate behavior is not verified.
Character products are deferred because lcm overflow, lifting, and resource bounds need their own slice.
Local factors, Tate integrals, and a faster factored Gauss algorithm are outside these work packages.

## Sources pending

- FLINT 3.0.1 dirichlet pairing and raw-exponent implementation under refs/.
  The inverse-label argument is proved for the stated bilinear pairing; identification with FLINT
  is finite evidence only. The exact lowering oracle and C lower function agree on the tested range.
- FLINT 3.0.1 group_init failure semantics on the entire word range, needed by the unbounded predicate.
- A local proof/source of the signed primitive quadratic Gauss evaluation, already pending in
  analysis.md:580-584. The design computes real-character root numbers and checks eight inputs;
  it does not assume the universal sign.
