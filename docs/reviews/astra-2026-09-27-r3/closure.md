# Round-3 closure — 2026-09-27

**[checked by a run]** Scope: the requested changed passages in SPEC draft 4 and PLAN draft 3, against R1–R6. `python3 -B proto/precision_rules.py` exits successfully: all previous assertions pass, plus the Kronecker checks, 1,882 power-coarsening cases and 2,000 binomial cases. The rational-root examples print the expected results; they are diagnostics, not assertions.

## Closure

| ID | Status | Reason |
|---|---|---|
| R1 | RESOLVED | **[proved here]** The separate Kronecker moduli `m` and `lcm(m,8)` are sufficient; negative/zero denominators require exact integer inputs. |
| R2 | PARTLY | **[proved here]** Hilbert precision, exceptional places and ambiguity are correct; the last note wrongly excludes every other testing modulus (C2 below). |
| R3 | RESOLVED | **[proved here]** The target is explicitly N, and `D=gcd(N,c^M-1)` correctly gives the largest determined divisor of N, with signed/exact exponents distinguished. |
| R4 | RESOLVED | **[checked by a run]** SPEC and PLAN 1F.8 now return diagonal rational roots, restrict branch enumeration to named places, and handle zero and degree 1. |
| R5 | PARTLY | **[proved here]** Costs, poles and the gamma convention are repaired; the cyclotomic precision rule and test vector need C1's qualifications. |
| R6 | RESOLVED | **[checked by a run]** PLAN 1 explicitly permits disposable probes and benchmark C before production contracts are frozen. |
| N5 | RESOLVED | **[proved here]** The remaining global branch problem is closed by the rational-root restriction; local formulas were already approved. |

## Formula checks and remaining minor edits

**[proved here]** The binomial smallest-ball formula is correctly transcribed: centre `binom(a,k)` and radius `gcd(binom(a+Nj,k)-binom(a,k): 1<=j<=k)`, with exact 1 for k=0. Newton differences prove enclosure and tightness as in the review. The conservative factorial rule remains sound. For power coarsening, `gcd(N,powmod(c,M,N)-1)` equals the printed D; no factorisation or full integer power is required.

**[proved here]** The defining equation `Z(hat f,chi^-1,1-s)=gamma(s,chi) Z(f,chi,s)` has the correct inverse character and argument, using the project's transform and measure. Epsilon normalisation remains an explicit milestone-0 obligation. The local zeta formulas/poles, Gauss-sum sign reference, Haar volume, content and theta identity introduce no new mathematical defect; theta retains SPEC 7's test-function and tail requirements. **[checked by a run]** PLAN 1F.9 assigns the requested regression cases, and section 8 correctly records round 3's two major findings and one unclosed earlier item.

**C1 — cyclotomic action, MINOR [proved here].** The arithmetic/geometric exponents are correctly transcribed, but “the modulus ... in canonical form, is divisible by `n`” canonicalises only one side. Replace it with: “determined exactly when `canon(n)` divides `canon(N)`, equivalently `U(N) subset U(n)`.” For example, `[2 mod 3]` fixes `[5 mod 6]` among unit lifts, so the action on sixth roots is determined although 6 does not divide 3. The current condition is sufficient but unnecessarily restrictive.

**[proved here]** In the same row, add **`gcd(p,n)=1`** to the uniformiser test vector. Its away-from-p unit is indeed `1/p`, giving arithmetic action `z -> z^p` on roots of order prime to p. Without that guard, p=n=3 would send a primitive third root to 1, which is not an automorphism. On p-power roots this particular idele instead acts trivially, since its class unit at p is 1. **[checked by a run]** Small modular probes confirm these precision and exponent examples.

**C2 — Hilbert oracle note, MINOR [proved here].** Replace “would not be valid for other moduli or for coefficients that are not reduced” with: “These thresholds are proved for coefficients of valuation 0 or 1; arbitrary precisions or unreduced coefficients require separate justification. Higher powers of the same primes also suffice.” A primitive solution modulo 32 reduces to one modulo 16 and hence lifts; conversely a local solution reduces modulo 32. The same argument applies above 9 and 25. The implementation's actual tests remain valid.

## Ratification

**[proved here] RATIFY AFTER MINOR EDITS — no major defect remains in the reviewed changes.**

1. **[proved here]** Canonicalise both N and n in the cyclotomic precision condition.
2. **[proved here]** Restrict the cyclotomic uniformiser test vector to `gcd(p,n)=1`.
3. **[proved here]** Replace the Hilbert note's categorical exclusion with C2's qualified statement.

**[proved here]** These edits need no further design review; the existing milestone-0 proof, source and convention gates still apply.
