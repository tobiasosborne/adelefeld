# Report of lane m0-proofs-ideles (work package 0.3b)

Date: 2026-09-27. Author: Claude opus subagent. Written to disk by the orchestrator from the agent's final
message (the harness did not let the subagent write this file); shortened, the facts unchanged.
Status: finished; not yet reviewed.

## Files written
`docs/proofs/policies.md` (22 statements), `docs/proofs/ideles.md` (16), `docs/proofs/quotient.md` (13);
`proto/policies_checks.py`, `proto/ideles_checks.py`, `proto/quotient_checks.py`;
`lanes/m0-proofs-ideles/mutants.py`. 51 statements, all proved, 0 open; some modulo standard facts cited from
`refs/` by key, file and line.

## Checks run (as reported by the lane)
| Command | Result |
|---|---|
| `python3 proto/policies_checks.py` | 12 checks PASS, exit 0, 17.0 s |
| `python3 proto/ideles_checks.py` | 11 checks PASS, exit 0, 82.4 s |
| `python3 proto/quotient_checks.py` | 8 checks PASS, exit 0, 31.2 s |
| `python3 lanes/m0-proofs-ideles/mutants.py` | 22 of 22 mutants killed, exit 0, 85.9 s |

## Not done
No review. Characters belong to another lane. Merging of reduced pieces is not specified. For partial-data
reconstruction only uniqueness is proved, not the algorithm. Outward rounding of real balls is treated only as
monotonicity.

## Sources pending
Tychonoff (quotient.md Corollary 4.2); fundamental theorem of arithmetic (ideles.md fact S4).

## Findings against the specification
1. SPEC 6: "one piece for each integer it crosses" is off by one: k integers crossed give k+1 pieces
   (quotient.md P6).
2. SPEC 5 and PLAN 4: `adf_ucoset` has no exact case (N >= 1; no coset is a single point, ideles.md P13.6), so
   rational to idele (PLAN 2.3) and x^0 can only be enclosed. Suggestion: an exact case.
3. SPEC 4.4 item 3: the cap on exact results is ambiguous (gcd(0, C) = C); both readings are sound
   (policies.md P14).
4. SPEC 4.4 scaled product: sound, not tight; it loses h = gcd(u, v, K). The tight product is itself a scaled
   value, at the cost of one word gcd (policies.md P10). Observation, not an error.
5. PLAN 2.1: no power rule for unit cosets. c^k U(N) is sound, not tight; the tight hull has modulus M_k
   (ideles.md P13); every square of a unit lies in U(24).
6. SPEC 4.1 "can change H and d" made precise (policies.md Summary 26): a sum never changes H, only d; a tight
   product changes H exactly when h does not divide d e; the derived contexts need no factorisation.
7. SPEC 5 gives no radius for division by an idele: (a e)/r + (gcd(|a| lcm(N,2), M)/r) Zhat (ideles.md P19).

Confirmed as stated: SPEC 2 fact 2; 4.5; 5 (decomposition, U(2N) = U(N), canonical form, gcd rule, norm, class
map, both idele-to-adele balls); 6 (domain, gluing, splitting); 9.2. On the brief's question: gcd(N, N') is the
best modulus for a product of independent cosets, up to the factor 2 of U(2N) = U(N).

## Repair after review (2026-09-27, added by the orchestrator from the agent's final message)

Review: `docs/reviews/m0-proofs/ideles-review.md` (codex gpt-6-astra): 46 valid, 3 minor, 2 invalid. The author
reproduced both counterexamples and agrees with both INVALID verdicts.

- policies P24: the set stays in the context; only the canonical triple needs the derived context. A backend
  holding canonical data moves to the derived context or to the global backend; equality and printing go through
  the canonical triple.
- policies P25: `d x = A mod q_i` is solvable exactly when `gcd(d, q_i)` divides `A`, with that many solutions;
  the ball fixes one residue modulo `q_i` exactly when `gcd(d, q_i) = 1`. The C code stores numerator residues and
  `d` once per value, applies `d` after recombination, inverts only coprime pairs, and never stores or reports a
  solution of `d x = A` as a residue of the value.
- L6, P10, ideles P19 repaired; Summary 26 is now a table of raw result against canonical modulus; P14: exact
  results keep their tag (SPEC 4.1), the cap applies only to positive radii; the two pending sources are quoted
  from disk.
- Findings: 3 withdrawn (settled by SPEC 4.1); 6 restated; 2 is an optional extension; 1, 4, 5, 7 stand.
- Checks: policies 13 groups, ideles 12, quotient 9, all pass; 27 of 27 mutants killed, among them the reviewer's
  surviving mutant and the two refuted claims, which the old checks had let pass.
  `docs/reviews/m0-proofs/ideles_review_checks.py` still exits 1 by construction: it hard-codes the two refuted
  literal claims and does not read the proof files.
