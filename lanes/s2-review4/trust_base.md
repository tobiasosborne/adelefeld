# Trust base of completeness above 128

## Statements that are required

1. Reduction must preserve each integer coefficient modulo p and remove leading zero coefficients.
   refs/src/flint-3.0.1/fmpz_poly.rst:3142 to 3145 says: "Sets the coefficients of Amod to the
   coefficients in A, reduced by the modulus of Amod." The input to the modular routine must be nonzero.
   Content removal provides that fact; adf_roots_modp aborts if the reduction is zero.
2. nmod_poly_powmod_ui_binexp must return X^p modulo h, for every prime that fits ulong, including
   p = 2^64 - 59, and for nonmonic h. refs/src/flint-3.0.1/nmod_poly.rst:860 to 861 says: "Sets res to
   poly raised to the power e modulo f, using binary exponentiation." The C source declares e as ulong
   at refs/src/flint-src-3.0.1/nmod_poly/powmod_ui_binexp.c:57 to 59. It reduces a base whose length is at
   least the modulus length at lines 78 to 88. Its work buffer lengths depend on deg h at lines 32 to 36,
   and the powering loop processes the bits of e at lines 40 to 48. A constant modulus returns zero
   at lines 73 to 75; the adelefeld code takes d = 1 without calling it for constant h.
3. nmod_poly_gcd must return the actual gcd of h and t - X, up to a nonzero scalar. Its degree must be
   the actual degree. refs/src/flint-3.0.1/nmod_poly.rst:1718 says: "Computes the GCD of A and B."
   refs/src/flint-src-3.0.1/nmod_poly/gcd.c:30 to 32 orders the degrees; lines 44 to 46 make gcd(A, 0)
   monic; lines 61 to 74 set and normalise the result from the kernel. Lines 19 to 24 dispatch to the
   Euclidean or half-gcd kernel. This review does not prove those kernels correct.
4. Modular evaluation and field arithmetic must give the actual values for reduced residues, including
   moduli with their top word bit set. refs/src/flint-src-3.0.1/nmod_poly/evaluate_nmod.c:32 to 35 uses
   n_mulmod2_preinv and n_addmod in Horner's loop; lines 21 to 25 treat zero and constant polynomials.
   The candidate check uses this evaluation. It is not an independent check of FLINT's word arithmetic.
5. For termination without a defect, nmod_poly_roots must handle a nonzero, split, squarefree d over F_p
   and return its linear factors in the representation that the caller parses. It need not be monic
   on entry: refs/src/flint-src-3.0.1/nmod_poly_factor/roots.c:196 makes it monic. Lines 155 and 35 assert
   a probable-prime modulus. Lines 157 to 174 clear the factor count, handle degrees zero and one,
   and throw on a zero polynomial. Lines 58 to 73 extract the zero root. Lines 96 to 102 make the first
   split, and lines 121 to 143 store linear factors or split further.

## Statements checked by adelefeld

src/roots.c:1117 to 1131 requires fac->num <= deg d, linear factor length 2, and a nonzero linear
coefficient before converting a factor q0 + q1 X to -q0/q1. modp_check, at lines 1052 to 1071, checks
the candidate count against deg d, each residue against [0, p), each value of h at that residue, and
distinctness. Thus it checks the mathematical candidate list from the root finder, conditional on
the trusted count and evaluation. It also trusts the ordinary memory representation of FLINT objects.

Wrong root-finder output alone cannot give an accepted wrong list if the count and evaluation are right:
an invalid root fails evaluation; a duplicate fails distinctness; a short list fails the count.
The accepted list then has exactly as many distinct true roots as h has.

## The smaller-gcd case is not checked

A computed d of smaller degree can still divide the true d and contain only true roots. Root finding on
that d gives a short list which satisfies every candidate test. The public completeness verifier repeats
the same modular count and can accept it again. This is the trust described in roots.h, not a new
counterexample to the current library with correct FLINT.

The planted fault in small_gcd.c sets d = 1 after the gcd call. trust_probe.py compares the original and
the lane copies on f = X(X - 1), p = 2^64 - 59, prec = 3, depth = 0. The original returns the two
certificates (0, 3, 0) and (1, 3, 0). The smaller-gcd copy returns OK, complete = 1, no certificates,
verify_entries = 1 and verify_complete = 1. The polynomial's two roots are the integers 0 and 1.
The unchanged differential harness nevertheless rejects this copy by independent root coverage.

The missing_root copy drops one root after modp_check has accepted the original list. It too fools the
rerun and returns complete = 1 with one ball, but the differential harness rejects it on its first case.
This is a separate defect simulation; it does not say the unmodified candidate check misses a short list.

## Termination and p-sized work

refs/src/flint-src-3.0.1/nmod_poly_factor/roots.c:17 to 19 names "Rabin's Las Vegas algorithm".
Its split helper, refs/src/flint-src-3.0.1/nmod_poly/find_distinct_nonzero_roots.c:15, says:
"split f assuming that f has degree(f) distinct nonzero roots in Fp". Lines 29 to 42 retry a
pseudorandom shift until the gcd has proper positive degree. It has no failure return or finite retry
bound. roots.c:177 creates the state; flint.h.in:245 to 250 supplies fixed seeds. The actual sequence is
fixed. A probability model for random shifts is not a worst-case time guarantee for this sequence.
The header admits unbounded splitting time; this review found no contrary runtime promise on that route.

The only residue loop in the cited root finder is roots.c:38 to 41, conditional on p < 10. It cannot be
entered by the automatic route above 128. The FLINT modular-power buffer is O(deg h), not O(p), and
adelefeld's root buffer is deg g + 1. The cited splitting loops retry or traverse factors; they do not
enumerate all p residues. No p-sized allocation or p-step enumeration was found on the new route.
