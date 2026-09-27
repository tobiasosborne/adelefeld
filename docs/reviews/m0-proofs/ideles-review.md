# Adversarial review of work package 0.3b

Reviewed: `docs/proofs/policies.md`, `docs/proofs/ideles.md`, `docs/proofs/quotient.md`, their three
`proto/` check scripts, and `lanes/m0-proofs-ideles/report.md`. Specification: sections 4.4, 5, 6 and 9.2,
with the backend and exact-tag contracts in section 4.1. No proof or specification file was edited.

Verdicts on the 51 theorem statements: **46 VALID, 3 MINOR, 2 INVALID**. The two INVALID verdicts concern
claims about representations in policies P24 and P25. No counterexample to the stated enclosure radii
was found. Do not use the unqualified backend conclusions of finding 6 as a canonical-storage rule.

VALID means the proof closes under its stated hypotheses and the cited background facts. MINOR means the
claim or its accompanying implementation note needs the specified local repair. INVALID applies to a false
claim as written, including a false clause in an otherwise correct proposition. Finite checks are supporting
evidence; they do not establish the infinite or topological claims.

## Summary table

The counts exclude six numbered definitions and Summary 26, as does the author's 22 + 16 + 13 count.
Those items are addressed immediately after the table.

| File | Statement | Verdict | One line |
|---|---|---|---|
| policies.md | L1 | VALID | Four product witnesses force the hull; checked zero and fractional radii. |
| policies.md | L2 | VALID | Hull minimality proves monotonicity; checked coarsened inputs. |
| policies.md | T3 | VALID | Tree induction preserves enclosure; checked repeated inputs in 600 nodes. |
| policies.md | L5 | VALID | Radius determines scale; residue is unique, including K = 1. |
| policies.md | L6 | MINOR | Step 1 drops a factor K; the stated representability criterion is true. |
| policies.md | P7 | VALID | Every admissible scale divides gcd(c,R/K); tested competing scales. |
| policies.md | P8 | VALID | Scalar sum is the best scaled enclosure, including zero and negatives. |
| policies.md | P9 | VALID | Negative scalars change the residue sign; zero remains exact. |
| policies.md | P10 | MINOR | All three formulas hold; the word-gcd cost claim needs a size hypothesis. |
| policies.md | P11 | VALID | Conversion and its exactness criterion hold with shared factors. |
| policies.md | C12 | VALID | Both conversions to the lcm preserve sets; arithmetic then follows. |
| policies.md | P14 | VALID | Both exact-result choices enclose; the gcd is best among capped radii. |
| policies.md | P15 | VALID | Divisibility and idempotence hold; pure products can also need capping. |
| policies.md | L17 | VALID | CRT and radius recover all data; checked blocks 1 and shared denominators. |
| policies.md | L18 | VALID | Coprime block factors multiply; checked complete cancellation. |
| policies.md | P19 | VALID | Both integer conditions are necessary; checked candidate denominators. |
| policies.md | P20 | VALID | Admissible denominators are multiples of d0; shifts of the centre agree. |
| policies.md | P21 | VALID | Raw sum keeps H; its item 3 correctly allows subsequent cancellation. |
| policies.md | P22 | VALID | Mixed-factor products and both residue formulas agree with sampled hulls. |
| policies.md | P23 | VALID | Reduced scalar numerator must divide d; tested negative and zero scalars. |
| policies.md | P24 | INVALID | A cancelled set is still representable in its original context. |
| policies.md | P25 | INVALID | A shared factor does not always prevent reduction of A/d modulo a block. |
| ideles.md | L2 | VALID | Coordinate inverses give both directions; finite samples cannot prove the tail. |
| ideles.md | P3 | VALID | All valuations determine the positive scale; tested signed fractions. |
| ideles.md | L5 | VALID | Local conditions, nonempty cosets and the subgroup argument close. |
| ideles.md | P6 | VALID | A free prime supplies a zero coordinate; additive and unit sets differ. |
| ideles.md | L7 | VALID | The modulo-2 condition is automatic for units; tested twice-odd moduli. |
| ideles.md | P9 | VALID | Local witnesses prove necessity; exhaustive small cosets verify all predicates. |
| ideles.md | P10 | VALID | Products and inverses are exact cosets; tested N = 1, 2 and composite N. |
| ideles.md | P11 | VALID | Independent local factors give equality and minimality, including the 2-factor. |
| ideles.md | L12 | VALID | The binomial argument covers both signs of k under the stated 2-adic guard. |
| ideles.md | P13 | VALID | Every row has a sharp local witness; tested powers up to 30 and k = 0. |
| ideles.md | P14 | VALID | Valuations yield 1/r and the product formula; negative rationals included. |
| ideles.md | P15 | VALID | Kernel, section and continuous inverse close; negative scaling tests the sign. |
| ideles.md | P16 | VALID | Differences attain every local hull valuation; scale and odd lifts tested. |
| ideles.md | P17 | VALID | The explicit zero coordinate is integral at each eligible prime. |
| ideles.md | P18 | VALID | Exact rational division is a bijective scaling; negative divisors tested. |
| ideles.md | P19 | MINOR | Radius is correct; minimality uses t0+M before justifying its membership. |
| quotient.md | L1 | VALID | Prime-primary fractions give the shift; uniqueness is exactly modulo Z. |
| quotient.md | P2 | VALID | Subtracting the floor gives existence; an integer of size below 1 is zero. |
| quotient.md | P3 | VALID | Differences can only be 0 or +/-1; checked the gluing sign. |
| quotient.md | C4 | VALID | Compactness uses the product topology; its pending source is present on disk. |
| quotient.md | P5 | VALID | Both directions and nonemptiness hold, including integer upper endpoints. |
| quotient.md | P6 | VALID | Endpoint gluing removes the last degenerate piece; k+1 counts this construction. |
| quotient.md | P7 | VALID | Missing-residue witness proves necessity; tested widths below, at and above N. |
| quotient.md | P8 | VALID | Disjoint cover and lower bound close, even for covering balls not inside it. |
| quotient.md | P9 | VALID | Refinement and endpoint transfer are exact; mixed-modulus families tested. |
| quotient.md | P10 | VALID | Rational translation cancels; rounding enlarges the quotient set. |
| quotient.md | P11 | VALID | Progression and endpoint counts hold; checked radius zero and width exactly N. |
| quotient.md | P12 | VALID | Lifting gives integral roots everywhere; the advertised check depth is misstated. |
| quotient.md | P13 | VALID | Cross determinant proves uniqueness; the equality-bound counterexample is valid. |

Definitions policies D4, D13, D16 and ideles D1, D4, D8 are consistent definitions. Their edge cases are
covered by the adjacent checks. Exact rationals are separate from positive-radius scaled values; H = 0 is
excluded from block contexts; modulus 1 represents all profinite units. Blocks of size 1 are harmless.
Summary 26 needs the same storage/set distinction as P24 and the qualification in finding 6 below. It is
MINOR as a summary of raw operations, not an additional theorem in the 51-statement count.

## Detailed findings

### policies L6: incorrect equality in the necessity proof

At `docs/proofs/policies.md:131`, `(c - s u)/(s K) = c K/R - u` is false. For `K = 3`, `R = 3`,
`s = 1`, `c = 4`, `u = 1`, the two sides are 1 and 3. The correct right side is `(c K/R - u)/K`.
Its integrality still implies `c K/R` is an integer and has residue `u` modulo `K`. The theorem survives.
Repair R1 supplies the missing factor and the congruence step.

### policies P10: the operation is not necessarily a word gcd

No hypothesis bounds K by a machine word. The enclosure, loss factor and tight variant are valid.
The last sentence's cost assertion is only available for word-sized K and residues. For arbitrary contexts,
computing `h = gcd(u,v,K)` requires an integer gcd at the actual operand sizes; the variant also divides `u v`
by h and updates the scale. This does not refute the enclosure rule. Repair R2 removes the unsupported cost.

### policies P24: storage identity is not set representability

The hypothesis permits context `(4)`, denominator `d = 2`, residue 2. Take `A = 2`, so `g = 2`.
The original value and its canonical value are both `1 + 2 Zhat`:

    (2 + 4 Zhat)/2 = (1 + 2 Zhat)/1.

The canonical tuple has H = 2. The set still has the original context-4 representation `(2; [2 mod 4])`.
P19 confirms this: with H = 4, R = 2, c = 1, both H/R = 2 and c H/R = 2 are integers.
Thus the assertion that it is not a local value of the original context is false under Definition 16's set
semantics. Its intended statement about storing the canonical tuple is true. The derived block formula is
also correct. These two facts must be separated before implementation.

There is a further implicit normalization: Definition 16 permits any integer lift A, but `(A/g,H/g,d/g)`
has its numerator in canonical range only if A was first chosen in `[0,H)`. Repair R3 states that choice.

### policies P25: universal impossibility is false

For `A = 2`, `d = 2`, `q_i = 4`, gcd(d,q_i) = 2 but the rational centre A/d = 1 has residue 1 modulo 4.
Also `2 x = 2 mod 4` has two solutions, 1 and 3. The example with A = 1 proves impossibility for that
example, not for every A sharing this denominator and block.

The safe storage rule remains correct. The unreduced denominator has no modular inverse. Moreover, old
numerator residues do not in general determine a unique centre residue at the old block: lifts A = 2 and
A = 6 of the same residue give centres 1 and 3 modulo 4. Existence of a residue for one rational centre and
well-defined storage for a whole ball are distinct requirements. Repair R4 states the actual limitation.

### ideles P19: the minimality step needs a difference witness

In step 4, B contains T and its smallest ball `t0 + L' Zhat`. It has not yet been shown to contain `t0 + M`:
t0 need not lie in T. For example, an integer representative e need not be a global unit, so a e need not
belong to the unit-coset image. Choose an actual t in T. Both t and t+M lie in B, so M is in its difference
module. This justifies the needed divisibility without assuming t0 is an image point. Repair R5 closes the
step, including M = 0 and a = 0. Independent finite images reproduce the claimed smallest radius.

### Additional proof details checked

For P15.4 of policies, the displayed example establishes scalar capping. A pure product witness is C = 2:
`(0 + 2 Zhat)^2` with independent inputs has tight radius 4 and cap 2. The claim is true.

For quotient P12 at 2, successive chosen roots agree modulo `2^(n-1)`, which is enough for a Cauchy sequence.
For m > n, sum the successive differences to obtain divisibility by `2^(n-1)`; the square errors tend to zero.
There is no missing compatibility obstruction. The independent check lifts all three square equations by
enumerating next digits, rather than using the author's lifting formula.

No omitted nonemptiness hypothesis was found for ordinary real balls: they are intervals with lo <= hi.
The integer-power minimum is a coset hull, and the additive-product minimum is a ball hull. Neither is
mistaken for the exact image in the proofs. Unit squares modulo 5 test this distinction explicitly.

## Verdicts on the seven reported findings

1. **Off-by-one wording: correct, with a qualification.** k integers in the shifted open interval give k+1
   pieces in the stated closed-piece construction. For width zero there is one piece. This is a construction
   count, not a minimum after merging. For example, with N = 1 and I = [0,2], the two closed pieces can merge
   to the whole quotient. The proposed wording is sound when it says which construction and which crossings.

2. **No exact unit case: correct observation; the proposed extension is optional.** Every finite unit coset
   contains more than one unit, and there is no smallest coset containing {1}. A conversion of a rational to
   the current idele type is an enclosure; PLAN 2.3 only requires containment. An exact unit tag can represent
   the finite unit +/-1 and the zero-th power. It does not by itself make a real ball contain 1/3 exactly or
   preserve the correlation of a diagonal rational. Keeping `adf_rat`, or adding an exact rational variant,
   is sufficient for those requirements. An unspecified exact arbitrary profinite unit is not an effective
   finite representation. No mandatory exact conversion follows from the present specification.

3. **Cap on exact results: enclosure analysis correct; specification diagnosis incomplete.** Literally,
   gcd(0,C) = C, and either result encloses. But SPEC 4.1 also says exact tags stay exact under arithmetic
   with exact values. Widening their sum or product violates that requirement even though it is an enclosure.
   A sound clarification is to preserve exact tagged results and apply the cap to positive-radius results.
   Treating both readings as equally compliant with the whole specification is unjustified.

4. **Scaled-product loss: correct.** h = gcd(u,v,K) is exactly the loss factor, and the tight variant remains
   in the same scaled context. SPEC already labels its product an enclosure. The proposed mathematical rule
   is sound. Replace the unqualified word-gcd cost assertion as in R2; K has no word bound here.

5. **Power rule: correct.** The simple rule encloses; P13's modulus is the smallest coset hull for nonzero k.
   For k = 2 and N = 1, the hull is U(24), and it strictly contains the square image. Negative powers and the
   canonical removal of a single factor 2 are covered. Using the tight hull is an optional precision choice;
   the zero-th power needs an exact case or an explicitly chosen enclosure.

6. **When contexts change: false without a raw-storage qualification.** P21 and P22 correctly describe
   representations before optional canonical cancellation. A sum can change canonical H. Both inputs
   `(1+2 Zhat)/2` are canonical with H = 2 and d = 2. Their raw sum is `(2+2 Zhat)/2`; its canonical form is
   `(0+1 Zhat)/1`, so H changes from 2 to 1. Also take canonical inputs `(1+6 Zhat)/2` and `(2+6 Zhat)/3`.
   Here h = 1 divides d e = 6, but the tight product `(2+6 Zhat)/6` cancels to `(1+3 Zhat)/3`: H changes to 3.
   The statement about derived contexts requiring no factorisation is correct. Repair R6 separates raw
   representability from the canonical output. The formula in SPEC 4.1 should not be narrowed to the report's
   unconditional wording.

7. **Division radius: correct.** The proposed smallest ball has centre a e/r and radius gcd(|a|L,M)/r,
   with L = lcm(N,2). The odd inverse lift is necessary. M = 0, a = 0, fractional radii and shared factors
   all agree with independent finite images. R5 repairs the proof of minimality. The specification delegates
   this operation to the proof work; absence of a radius formula is an omission to fill, not a false assertion.

## Repairs

These are replacement texts for the author or orchestrator. None has been applied to a read-only file.

### R1: policies L6, replace proof step 1

If `c + R Zhat = s (u + K Zhat)`, equality of radii gives `R = s K`, hence `s = R/K`.
Equality of the cosets gives

    (c - s u)/(s K) = (c K/R - u)/K in Z.

Therefore `c K/R - u` is an integer multiple of K. In particular `c K/R` is an integer and
`u = c K/R mod K`.

### R2: policies P10, replace the sentence after the proof

The specification's rule is an enclosure with loss factor h. The tight variant requires computing the
integer gcd `h = gcd(u,v,K)`, dividing `u v` by h, and updating the scale to `s t h`. A word gcd applies
when K and its residues fit a machine word.

### R3: policies P24, replace its claim paragraph

Choose the numerator A with `0 <= A < H`. Put `g = gcd(A,H,d) > 1` and `g_i = gcd(g,q_i)`.
The same set remains representable in the original context by its original data. Its canonical global
triple is `(A/g,H/g,d/g)`. Storing that canonical numerator modulus requires a derived context with blocks
`q_i/g_i`. In that context its denominator is `d/g` and its residues are
`(r_i/g_i) w_i mod (q_i/g_i)`, where w_i is the inverse of `g/g_i` modulo `q_i/g_i`.
This inverse exists because `g/g_i` is coprime to q_i. A block reduced to 1 carries residue 0 and can be omitted.

### R4: policies P25, replace item 2 and its proof

Storing a residue of `A/d` modulo q_i is not a valid general replacement for numerator residues.
When `gcd(d,q_i) > 1`, d has no inverse modulo q_i. The congruence `d x = A mod q_i` has a solution
exactly when `gcd(d,q_i)` divides A, and, if it does, has that many residue solutions modulo q_i.
Indeed, writing `g = gcd(d,q_i)`, divisibility by g is necessary; after division, d/g is invertible
modulo q_i/g, giving one class there and g classes modulo q_i. Thus `(A,d,q_i) = (1,2,4)` has no
solution, while `(2,2,4)` has solutions 1 and 3. A particular reduced rational centre may have a residue,
but the old numerator residue and denominator need not determine it. Store the integer numerator residues
and apply the denominator outside the modular arithmetic.

### R5: ideles P19, replace the converse part of proof step 4

Let a ball B contain `T + M Zhat`. Since T is nonempty, choose t in T. B contains t and t+M, so
M lies in the difference module of B. Also B contains T and therefore its smallest ball `t0 + L' Zhat`.
Write `B = t0 + R Zhat`. If R = 0, containment forces both L' = 0 and M = 0. Otherwise the two
members t and t+M give `R | M`, and the contained ball gives `R | L'`. Hence `R | gcd(L',M)`.
It follows that B contains `t0 + gcd(L',M) Zhat`. This proves minimality, including the zero cases.

### R6: qualify policies Summary 26 and finding 6

The table describes representations before canonical cancellation. A raw sum has H unchanged and denominator
`lcm(d,e)`. A tight product has a representation at the old H exactly when `h | d e`; in that case its
numerator is `A B/h` and denominator `d e/h`. Otherwise the derived context with modulus H h represents it.
Subsequent canonical cancellation can change H in either case. For the tight product its canonical modulus is
`H h / gcd(A B,H h,d e)`. A canonical tuple may require a new context even when its represented set remains
representable in the old context.

### R7: quotient P12, replace its Check line

Check: `check_local_global` finds roots modulo `2^12` at 2, modulo `p^4` for odd p < 50, and modulo `p^2`
for 50 <= p < 200. It checks the absence of rational roots among the signed divisors of the constant term.

### R8: specification clarification for the orchestrator

In SPEC 4.4, absolute cap: exact tagged results remain exact. For a positive-radius tight result with
radius R, replace R by gcd(R,C), where C is a positive rational.

In SPEC 6, piece count: for an integer-radius ball and shifted interval with nonempty interior, splitting at
the k integers strictly inside that interval gives k+1 closed pieces with the gluing rule. A singleton real
interval gives one piece. Pieces may subsequently merge, so this count need not be minimal.

## Sources and the two pending items

Both pending background results are already on disk. No external source was fetched.

- Replace ideles S4's pending marker with `refs/src/milne-ant/ANT.txt:321`. Lines 321 to 324 state the
  prime factorization of every nonzero integer and say that the factorization is "essentially unique".
  Applying this to the reduced numerator and denominator gives the stated rational form and criterion.
- Replace quotient T's Tychonoff marker with `refs/src/milne-cft/CFT.txt:9215`: "Tychonoff's theorem says
  that a product of compact spaces is compact." The statement continues on line 9216.
- The CRT statement used for block arithmetic is `refs/src/baker-padic/padicnotes.txt:374`; the proof at
  line 384 uses the coprime two-factor ring isomorphism. The inverse-limit description is in
  `refs/src/tate-poonen/notes.txt:1603`.
- The local field, density and expansion background is at
  `refs/src/baker-padic/padicnotes.txt:1182`, `:1287`, and `:1326`. Units are described in
  `refs/src/tate-kudla/kudla-1.txt:85`. The root-count and finite-field facts used in P13 and quotient P12
  appear at `refs/src/baker-padic/padicnotes.txt:454`, `:462`, and `:473`.
- Local compactness at the integral domain is covered by
  `refs/src/baker-padic/padicnotes.txt:1914`. For the restricted product topology use
  `refs/src/milne-cft/CFT.txt:9373`; it gives the topology on the distinguished open product.
  The topology paragraph at `refs/src/tate-warwick/tatesthesis_notes.txt:386`, read literally, restricts
  every finite factor to Z_p and omits translates needed outside the integral open set. It should not be
  the sole citation for the topology of all of A. Milne's cited definition supplies the required reference.
- For comparison, the norm convention is at `refs/src/tate-warwick/tatesthesis_notes.txt:405`, the product
  formula at `refs/src/tate-poonen/notes.txt:1579`, and the class decomposition at
  `refs/src/tate-kudla/kudla-1.txt:123`. The review relies on the written proofs for these identities.

Sources pending for this review: none. The author must still apply the citation replacements. No external
quotation is used as a substitute for the stepwise repairs above.

## Assessment of the author's checks

The reruns give 12, 11 and 8 passing check groups, respectively. The author's mutation file kills its
22 listed mutants. These results do not cover every clause of the proofs.

- **policies:** sampled difference hulls and residue-set comparisons test the main arithmetic formulas.
  Zero and fractional radii and shared factors are included. P24's checks only verify the derived context
  and cancellation; they never test its assertion that the set leaves the old context. P25 tests only
  `2 x = 1 mod 4`, so it cannot distinguish the correct general limitation from the false universal claim.
  Neither check can detect L6's dropped factor in the written proof. The word-gcd cost claim is untested.
- **ideles:** exhaustive coset images test the subgroup rules and hull valuations. The power checks use
  only k = -4,-3,-2,-1,1,2,3,4 (`proto/ideles_checks.py:231`); they omit k = 0 and higher valuations of k.
  Division explicitly skips a = M = 0 (`proto/ideles_checks.py:374`) and some large finite products.
  It samples positive r for the final scaling but does not explicitly test negative rational division.
  Finite prime lists cannot prove the infinite-tail assertion of L2 or the topological assertion of P15.
- **quotient:** translated families always have the same positive finite modulus A, including the paired
  families used to test equality (`proto/quotient_checks.py:333`). The refinement loop over N'/N_i is
  therefore always a single iteration. An arbitrary mixed-modulus union is not tested. Exact critical
  endpoints are used for equality; the fixed grids in the other checks are adequate for their generated
  small denominators, but should not be treated as a general interval-equality algorithm.
  The root-check docstring claims exhaustive search; for odd primes it actually uses the proof's lifting
  formula. The P12 Check line also overstates its depth above 50. R7 gives the actual range.

A concrete wrong formula passes the author's quotient check. The separate audit script changes
`for j in range(Np // Ni)` to `for j in range(1)` in canonical refinement. `check_translation` still
passes. This mutation loses true points for a mixed family: a piece `[1/4,1/2] x (0 + 2 Zhat)` must cover
residue 2 modulo 6, when another piece of modulus 3 forces the common modulus 6. The mutant omits it.
The independent suite tests arbitrary mixed families and includes this witness explicitly.

The same audit script changes the endpoint transfer from m-1 to m+1 as a control; the author's check
rejects it. Thus the surviving refinement mutation is an observed coverage gap, not a failure to run the test.

Command: `python3 -B lanes/m0-review-ideles/author_mutation_probe.py`

```text
SURVIVED: drop mixed-modulus residue refinements
PASS check_translation (P9, P10): 186 balls translated: identical pieces and canonical forms;
148 different balls told apart
KILLED: wrong boundary glue sign
FAIL check_translation (P9, P10): 186 balls translated: identical pieces and canonical forms;
148 different balls told apart
TOTAL mutants=2; survived=1; killed=1
```

The two long checker lines above are wrapped for this document. The unwrapped output and exit status are in
`lanes/m0-review-ideles/author_probe.txt`.

## Checks

The independent file imports no author's functions and uses only the Python standard library. It checks
finite images, exact rational witnesses, local unit power images, endpoint cells and next-digit root lifts.
The input sets include zero and negative centres; radii 0, 1/2, 2/3 and 3/2; moduli 1, 2 and twice an odd
number; non-prime-power blocks; denominator/block shared factors; integer real endpoints; and width exactly N.
Arbitrary mixed-modulus piece families supplement the author's homogeneous families.

The command exits 1 deliberately because it preserves the two refuted literal claims as review failures.
The assertions checking valid formulas, repairs and explicit counterexamples pass. The script does not
silently substitute repaired claims for the original assertions.

Command: `python3 -B docs/reviews/m0-proofs/ideles_review_checks.py`

```text
CHECK backend_L17_L18: 2250 assertions passed
CHECK backend_P19_P20: 6760 assertions passed
CHECK backend_P21: 2250 assertions passed
CHECK backend_P22: 3825 assertions passed
CHECK backend_P23: 8633 assertions passed
CHECK backend_P24_repaired: 581 assertions passed
CHECK backend_P25_repaired: 449 assertions passed
CHECK ideles_L12: 165 assertions passed
CHECK ideles_L2_P3_P14: 1400 assertions passed
CHECK ideles_L5_P6_L7: 138 assertions passed
CHECK ideles_P10_P11: 6348 assertions passed
CHECK ideles_P13: 3917 assertions passed
CHECK ideles_P13_zero: 12 assertions passed
CHECK ideles_P15: 64 assertions passed
CHECK ideles_P16: 184 assertions passed
CHECK ideles_P17: 69 assertions passed
CHECK ideles_P18_P19: 632 assertions passed
CHECK ideles_P9: 6348 assertions passed
CHECK negative_controls: 6 assertions passed
CHECK policies_L1_L2: 7056 assertions passed
CHECK policies_L5_L6_P7: 1056 assertions passed
CHECK policies_P10: 936 assertions passed
CHECK policies_P11_C12: 2340 assertions passed
CHECK policies_P14_P15: 1757 assertions passed
CHECK policies_P8_P9: 3276 assertions passed
CHECK policies_T3: 600 assertions passed
CHECK proof_repairs: 5 assertions passed
CHECK quotient_L1_P2_P3_C4: 437 assertions passed
CHECK quotient_P11: 15969 assertions passed
CHECK quotient_P12: 55 assertions passed
CHECK quotient_P13: 4965 assertions passed
CHECK quotient_P5_P6_P8_P10: 26952 assertions passed
CHECK quotient_P7: 192 assertions passed
CHECK quotient_P8_minimal: 210 assertions passed
CHECK quotient_P9: 3480 assertions passed
REFUTED policies P24: (A,H,d)=(2,4,2), g=2: both old and cancelled data give 1+2 Zhat
REFUTED policies P25: A=2,d=2,q=4: A/d=1 reduces mod 4; 2*x=2 has solutions [1,3]
TOTAL assertions=113317; refuted_claims=2
exit=1; elapsed_seconds=3.208
```

The 55 P12 assertions are 46 prime checks and 9 positive-divisor checks; negative divisors have the same
square. Every odd prime below 200 was lifted to p^4 in this independent run, and 2 to 2^12.
The six controls are explicit witnesses against fixed radii, a missing product loss factor, removal of too
many 2-adic digits, an overfine square hull, wrong boundary transfer, and missing residue refinement.

Author reruns, with standard-library subprocess time limits of 175 seconds per command:

| Command | Result | Seconds |
|---|---|---:|
| `python3 -B proto/policies_checks.py` | 12 groups pass; exit 0 | 7.522 |
| `python3 -B proto/ideles_checks.py` | 11 groups pass; exit 0 | 46.876 |
| `python3 -B proto/quotient_checks.py` | 8 groups pass; exit 0 | 13.750 |
| `python3 -B lanes/m0-proofs-ideles/mutants.py` | 22 killed, 0 survived; exit 0 | 63.448 |
| `python3 -B lanes/m0-review-ideles/author_mutation_probe.py` | 1 killed, 1 survived; exit 0 | 6.863 |

Full outputs are under `lanes/m0-review-ideles/author_*.txt`. Each computation used one Python process;
at most two computations ran concurrently. No package was installed. No C implementation or machine-word
benchmark was run. The Python checks use unbounded integers and do not establish C overflow safety.
