# F5: partial analysis; no global verdict

The fault removes M0 from the ORIGINAL Z6 bound, leaving
B5 = pi^(-min Re(S)/2) [M1 + log(pi) + sum 1/delta_j] / (2 P).
It does not remove a bound for |L_inf| from a digamma bound. The code bounds Gamma and Gamma'
after a recurrence. The relevant source is refs/src/tate-poonen/notes.txt:53-65.

Two cases of B5 are sound.

1. If n=0, Q=P=1 and the reciprocal sum is zero. The weighted-integral proof in Y16 gives
   |Gamma'(z)-log(pi) Gamma(z)|<=M1. B5 is larger than that sufficient bound.
   This covers the suggested positive examples near s=2.9232 and s=40, at sufficiently small radii.
2. If 1<=Re(z+n)<=2 throughout the rectangle, the Gamma integral gives
   |Gamma(z+n)|<=Gamma(Re(z+n)). The real function Gamma(a) is convex on [1,2], since its
   integrand t^(a-1) exp(-t) is convex in a. Gamma(1)=Gamma(2)=1, so Gamma(a)<=1 there.
3. The original Z6 integral majorants give |Gamma'(z+n)|<=M1. Differentiating the recurrence
   and prefactor now bounds the numerator by M1+log(pi)+sum 1/delta_j, which proves B5
   for the second case as well. Many thin near-pole boxes lie in this case.

This is not a proof for n>0 and max Re(z+n)>2. The factorial slack and the product of minimum
distances appear to explain the search results, but that observation is not a proof.
No M0 change was made in production. No F5 witness was added to the tests.

Search source: search.c and search_setup.py. Four interleaved families use exact dyadic inputs:
near all 63 poles admitted by the shift cap, positive boxes, and two wide complex-box families.
Each OK checks the centre and four corners. The search records the derivative bound before R
and counts whether the midpoint enlargement has a smaller component radius than the first candidate.
Gamma/digamma at 384 bits are candidate finders only (digamma: refs/src/flint-3.0.1/acb.rst:916-918).
They would not certify a final witness;
any candidate miss would need the independent Z8 reference before entering the tests.
The retained logs state all search counts. The global verdict remains not done.
