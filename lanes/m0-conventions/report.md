# Lane m0-conventions: report (work package 0.4, part A)

Agent: Claude opus subagent, 2026-09-27. Saved to disk by the orchestrator from the agent's final message,
verbatim.

## What was done
1. `docs/conventions.md` (version 0.1, draft for review):
   - §1: FLINT conventions, each cited from `/usr/include/flint`.
   - §2: naming, argument order, life cycle.
   - §3: status codes with values and meanings, which classes of function may return which, and how statuses combine over places.
   - §4: aliasing, ownership, output state after every status, invalid input, threads, context lifetime.
   - §5: canonical forms and storage invariants as checkable predicates for every type of PLAN §4, with two own uniqueness proofs.
   - §6: stubs marked PENDING (part B) for signs, measures, Gauss sums, root numbers, class-group characters, reciprocity and the seams recommendations.
   - §7: places.
   - §8: text interface, alphabet, tokens, limits, order of checks.
   - §9: value form — EBNF for all 13 types, semantic constraints, printing templates, the exact real-ball printing algorithm (including constrained printing and a termination argument), round trips, classification, and 16 ambiguities of PLAN §5 resolved.
   - §10: dump grammar and strict loading rules.
   - §11: golden file format and how a C test uses the vectors.
   - §12: foreign-function interface (Julia `ccall`), added on the coordinator's request.
   - §13: table of 46 proposed decisions. §14: findings.
2. Golden vectors: 17 files, 666 vectors, 307 of them expected statuses, plus a README with counts per file.
3. `proto/text_grammar.py`:
   - parser and printer of the value form for all 13 types;
   - reads real balls into exact rational intervals (no floats);
   - classification by syntax;
   - strict dump loader and printer for all types.
   Tests are in `proto/test_text_grammar.py`.
4. `lanes/m0-conventions/mutants.py`: mutation check with 25 mutants.

## Checks run (from the repository root)
1. Red: `python3 -m unittest proto/test_text_grammar.py` before the module existed gave "Ran 1 test, FAILED (errors=1)", ModuleNotFoundError (saved in `red.txt`).
2. First run after implementing: 23 tests, 5 failures and 1 error. None was fixed by weakening a test:
   - Three hand-computed vectors were wrong:
     - `…890/7`: the numerator is divisible by 7, so the vector now uses `/11`;
     - the radius for 2^-70 is `1.7e-42`, not `1.7e-41`;
     - a qclass vector assumed `0.625 +/- 0.25` prints unchanged; it prints `0.62 +/- 0.26`.
   - One test bug: a 3-tuple was unpacked as a 2-tuple.
   - One design gap found by a property test: constrained printing was not idempotent. Example: mid 20396493/16 with radius about 1.258e6 prints `1275000 +/- 1259000`, and printing that again gives `1280000 +/- 1270000`.
     - Fix: the printer now repeats until the text is stable (§9.5, with a proof that this ends).
     - Quotient pieces are now ordered by their printed real parts (§9.4) for the same reason.
3. Final run: 23 tests OK in 12.7 s. Coverage:
   - all golden files, and every expected output is a fixed point that classifies as its own type;
   - 4000 random dyadic balls for enclosure and idempotence, 4000 for the loop count (at most 3 passes observed), 3000 constrained cases;
   - 3000 finite parts checked against `canon()` of `tests/ref/adfref`;
   - 2000 local-ball centres checked against the definition;
   - fuzzing: 6000 mutated golden inputs, 3000 random byte strings × 13 parsers plus the dump loader, 4000 mutated dumps. No exception occurred, and every accepted input prints to a fixed point of the same type.
4. Mutation check: `python3 lanes/m0-conventions/mutants.py a b`, run in four ranges of under 2 minutes each.
   - 24 of 25 killed. Two were killed by the 90 s timeout: without digit growth in constrained printing, and with DOMAIN checked before LIMIT, the reference does not terminate — which is what those rules prevent.
   - 1 survivor, equivalent to the original: "NUL accepted" (the grammar rejects NUL anyway).
   - Two earlier equivalent mutants were replaced, and both replacements were killed: the stored unit residue range, and `rm>2^30` versus `rm>=2^30`.
5. FLINT probes, run as C programs against `libflint.so.18.0.1` (FLINT 3.0.1):
   - `arb_dump_str` format: 0.5 → `1 -1 0 0`; -0.75 → `-3 -2 0 0`; +inf → `0 -1 0 0`; nan → `0 -3 0 -1`; `3.14159 +/- 1e-5` at 53 bits → `c90fcf80dc337 -32 a7c5ac5 -2c`.
   - `arb_load_str` aborts with SIGABRT on 8 strings: `1 0 0 0 5`, `0 5 0 0`, `0 1 0 0`, `0 -4 0 0`, `1 0 0 1`, `1 0 0 -2`, `1 0 40000001 0`, `1 0 7fffffff 0`.
   - It silently changes `2 0 0 0`, `A 0 0 0`, `1 0 -1 0` and `1 0 40000000 0`, and accepts a trailing space.
   - All 14 `arb` groups in the valid dump vectors load and dump back identically.
   - Sizes in bytes: fmpz 8, fmpq 16, mag 16, arf 32, arb 48, acb 96, padic 24, padic_ctx 48, nmod_t 24.
   - `nm -D` shows the header inline functions exported (`fmpz_init`, `fmpq_init`, `arb_init`, `arb_swap`).
   - The enclosure of [0.9, 1] exceeds 1 at prec 20, 53 and 128.
6. Dirichlet characters, checked with python-flint 0.8.0:
   - For q ≤ 60, lowering (q,n) to (f, n mod f) gives the same values on all 91683 checked character values. In 20 cases (f, n mod f) is not primitive: (16,9) lowers to (8,5), not (8,1).
   - 2^64-59 is prime and 2^64+1 is not.
7. No prose line of `conventions.md` exceeds 116 characters. Table rows are longer, as in SPEC.md.

## Proposed decisions
CV-01 to CV-46, listed in `conventions.md` §13 (titles are in the summary above).

## Not done
- No C code (not in the brief). The comparison rule for real parts in C (§11.3) is written but untested.
- The Python dump loader does not model the caller's context argument.
- The reference finds primitive characters by search with python-flint and refuses q > 10^5. It also refuses arb exponents above 2^20 where a value must be computed. Both are limits of the reference only.
- PENDING (part B): §6.1–6.7, the scaled-policy rules for exact values (§5.4), and the invariants of quotient pieces (§5.10).
- The value forms of qclass, ffun, rfun and char are syntax proposals; their semantics wait for part B.

## Sources pending
- FLINT 3.0.1 documentation for:
  - the aliasing rule;
  - the `arb_dump_str` format (only probed here);
  - the reduced form of `padic`;
  - the Conrey numbering in `dirichlet`;
  - `flint_cleanup`;
  - the normalisation of `acb_dirichlet_gauss_sum`.
- Nemo.jl's FLINT version and struct layouts (marked [unverified] in §12).

## Findings against the specification and the plan
- F1: SPEC 5 and PLAN 4 require N≥1 for unit cosets, but SPEC 9.3.7 and catalogue Proposition 12 return "the exact 1", and the idele of an exact rational needs an exact sign unit. Proposed: CV-15.
- F2: SPEC 6 "pieces inside [0,1]" cannot hold for `arb` enclosures of non-dyadic end points (probed). Proposed: CV-45.
- F3: PLAN 4's scaled struct cannot hold the exact 0 that precision.md Proposition 6(2) produces. Proposed: CV-14.
- F4: SPEC 4.1 "or is given a new context" is ruled out by caller-owned contexts (CV-11). This narrows the spec rather than contradicting it.
- F5: FLINT's `arb_load_str` aborts or silently alters malformed input, so the C loader must validate first and fuzzing must never reach it with raw text.
- F6: value-form real balls are lossy on output as well as input. Without constrained printing an idele could print so that it reads back as DOMAIN; handled in §9.5 and §9.6.
