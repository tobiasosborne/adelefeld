# Judgments on the three proof-file requests

## Decision S-D10 and its source-pending line

The request is right for the implemented one-word route. The current paragraph also says "every prime"
and "with no bound of one word". That no longer states S-D10 as written in SPEC 15.3. The earlier review
already recorded the prime-width problem. This is an update of that passage, not a new specification finding.

Replace the paragraph beginning "Decision S-D10" and its source-pending line after Proposition 3.7 with:

```text
Decision S-D10 (TJO, 2026-09-29): the root finder accepts every prime that a place can hold. The place type
holds the primes below 2^64, each proved prime by adf_place_prime; it is not widened. A bound on p is a
property of an implementation slice. In the implemented slice, p <= ADF_ROOTS_P_EVAL_MAX = 128 uses 1;
larger primes use 2. The bound is the point where the method changes, measured by bench/bench_roots_modp.c.
For each class polynomial h of Algorithm P, its content at p is removed, so h modulo p is not zero
(Proposition 3.4). Form t = X^p modulo h by nmod_poly_powmod_ui_binexp and d = gcd(h, t - X) by
nmod_poly_gcd. Since t - X = X^p - X modulo h, this d is the gcd of h and X^p - X. For h constant, take d = 1.
The candidates are the roots of d returned as linear factors by nmod_poly_roots(r, d, 0). Accept them only
when they are distinct residues in [0, p), every candidate evaluates to zero in h, and their number is
deg d. Proposition 3.7(2) then proves completeness. A refused list aborts under S-D20.
The correctness of the powering, gcd, coefficient reduction and modular evaluation is trusted in FLINT.
The candidate test checks the root finder's output; it does not independently certify deg d. In particular,
a wrong gcd of smaller degree can cause an incomplete list to pass. The completeness verifier repeats the
same count and therefore has the same trust base.
Sources on disk: refs/src/flint-3.0.1/nmod_poly.rst:858 to 861 and 1716 to 1721;
refs/src/flint-src-3.0.1/nmod_poly/powmod_ui_binexp.c:26 to 52 and 67 to 88;
refs/src/flint-src-3.0.1/nmod_poly/gcd.c:16 to 24 and 27 to 74;
refs/src/flint-src-3.0.1/nmod_poly_factor/roots.c:17 to 19 and 148 to 204;
refs/src/flint-src-3.0.1/nmod_poly/find_distinct_nonzero_roots.c:15 to 51.
The root routine retries a random split until proper; it returns no failure status or finite time bound.
flint_randinit uses fixed seeds (refs/src/flint-src-3.0.1/flint.h.in:245 to 250). The implementation therefore
uses a fixed pseudorandom sequence. The algorithm's random-shift analysis is not a worst-case time bound
for that sequence. No fmpz_mod_poly source is needed for this one-word implementation.
```

## Algorithm P step 3

Reject the claim that the existing step covers only evaluation. "For every b in [0, p) with g(b) = 0"
specifies the mathematical set to process. It does not say to evaluate all residues to obtain that set.
Proposition 3.7 explicitly supplies both ways to find the same set. The partition proof uses completeness
of that set, not the method that found it. No proof change is required.

An optional clarification can replace the first two lines of step 3 with the following text. It does not
repair a false proposition:

```text
3. For an open class (a, e): compute w and g of Proposition 3.4. Find the complete list of roots b of g
   modulo p by either method of Proposition 3.7. For each root b in that list:
```

## Proposition 3.5(7)

The request is right. The cost sentence describes the evaluation route and even charges g' at every
residue, whereas the code evaluates g' only at the roots. Replace item 7 with:

```text
7. Cost: for each class opened, find the roots of its current polynomial g modulo p. The evaluation
   method of Proposition 3.7(1) uses p evaluations of g. The degree method of Proposition 3.7(2) uses
   O(log p) polynomial products and reductions modulo g, one polynomial gcd, the root finding of d,
   and deg d evaluations of g to check the candidates. For g constant, d = 1 and the list is empty.
   FLINT's root finding uses repeated pseudorandom splitting; no worst-case time bound is asserted.
   After either method, evaluate g' once for each root found. For each child opened, make one change
   of variable and remove its content at p. For each certified root, perform the lifting of 3.3.
   These counts exclude coefficient bit costs and the sorting of the output lists.
```

Replace the proof of item 7, currently "Count.", with:

```text
7. For evaluation, count the residues. For the degree method, binary powering processes the bits of p
   (refs/src/flint-src-3.0.1/nmod_poly/powmod_ui_binexp.c:40 to 48), so it uses O(log p) products and
   reductions. The gcd is computed once. The accepted candidate list has length deg d, and each
   candidate is evaluated once. Step 3 evaluates the derivative only at those roots, makes a change
   of variable only when a child is opened, and applies 3.3 only when a root is certified. Root finding
   itself has no finite retry bound in the cited source; its repeated splitting cost stays separate.
```
