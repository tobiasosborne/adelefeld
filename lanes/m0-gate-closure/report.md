# m0-gate-closure report

Verdict: GATE PASSED AFTER THE LISTED EDITS; full closure: `docs/reviews/m0-gate/closure.md`.
Done: checked G1-G16 and 8 rejected decisions against revised text; independently checked printing and contracts.
Items: 9 findings CLOSED, 7 CLOSED WITH EDIT; 3 rejected decisions CLOSED, 5 CLOSED WITH EDIT; 0 OPEN verdicts.
Pending E1: reconcile matching contexts with pointer checks; cover tight multiplication and scalar operations.
Pending E2: clarify constructor pointer replacement and remove PLAN's inline-allocation alternative.
Pending E3: remove the surviving claim that the midpoint predicate proves a piece's construction.
Pending E4: add the signed quadratic Gauss-sum scope note to analysis Proposition 13.
Pending C1 (MINOR): restore PLAN's three-way point comparison.
Pending C2 (MAJOR): fix inspection capacity, descriptor lifetime and size_t count contracts.
Pending C3 (MINOR): allow text-constructor PARSE and LIMIT statuses, with staged validation.
Pending C4 (MINOR): permit result-certification failure in NOT_DETERMINED and the inversion row.
Pending C5 (MAJOR): reconcile scaled-context conversion with its enclosing formula and exact operands.
May proceed: global ring types and value text; context/scaled/dump declarations must incorporate these edits.
Not done: applying edits, production C/header, C alias/lifetime tests, or the original full source audit.
Sources pending: signed primitive quadratic Gauss-sum evaluation; inherited pending items remain unchanged.
Findings against the specification: no new arithmetic counterexample; SPEC 10.4's constructor clause needs E2.
Cross-document conflicts: C1 contradicts SPEC 4.2; C5's old status row contradicts SPEC 4.4's enclosing conversion.

Files written: `docs/reviews/m0-gate/closure.md`; this report; files below in the owned `closure-checks/` directory.
Check sources: `printing.py`, `contracts.py`, `audit.py`.
Check outputs: printing.out, contracts.out, text.out, ref.out, policies.out, mutants.out, flint_probe.out.
Audit outputs: `audit.out`, `reviewed-inputs.sha256`. The compiled `flint_probe` executable was removed after use.
No specification, convention, plan, reference implementation or golden vector was changed.

Commands below ran at repository root; c=docs/reviews/m0-gate/closure-checks, g=docs/reviews/m0-gate/checks.
Subprocess suites used PYTHONDONTWRITEBYTECODE=1. At most two processes ran; every suite had a 170 s timeout.

| Command | Result |
|---|---|
| `git diff a46b161 HEAD -- docs proto tests` (by file groups) | Read-only inspection of changed text/code. |
| `timeout 170s python3 -B $c/printing.py` | 15032 cases, 0 failures; 12000 independent random dyadic balls. |
| `python3 -B $c/contracts.py` | First run: 875 checks; after adding C5: 878; 0 failures in either run. |
| `timeout 170s python3 -B -m unittest proto/test_text_grammar.py` | 30 tests; 0 failures; 12.609 s. |
| `timeout 170s python3 -B -m unittest discover -s tests/ref/tests` | 63 tests; 0 failures; 6.029 s. |
| `timeout 170s python3 -B proto/policies_checks.py` | 14 groups; 0 failures; S26: 960 triples. |
| `timeout 170s python3 -B tests/ref/mutants.py` | 62 baseline tests; 19 killed, 0 survived. |
| `python3 -B $c/audit.py` | First: 2 overlong report lines; final: 0; 16 findings and 8 decisions covered. |

`cc -O2 -Wall -Wextra $g/flint_probe.c -o $c/flint_probe -lflint -lgmp -lmpfr`: exit 0, 0 diagnostics.
`$c/flint_probe`: exit 0; FLINT 3.0.1; nonzero input/product flags 1/0; polynomial lengths 0/1.
Decimal rereading dumps: `1 0 10a3d70b -1f`, `1 0 23d70a3f -20`; print as `1 +/- 0.14`, `1 +/- 0.15`.
The exact-reference fixed point passed; these C rereadings still widen, as the repaired contract permits.
Check limits: no production C implementation exists; modulus-only Python tests do not establish pointer semantics.
