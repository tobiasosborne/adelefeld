# Progress

Read CLAUDE.md, workflow rules, the d-quotient brief and m3-design review; read the named
specification, conventions, proofs, headers, phase design, FLINT header and on-disk Gauss documentation.

- The dump body already exists at conventions:1414: q, n, then acb s. Parity is reconstructed.
- python-flint 0.8.0 and mpmath 1.3.0 import. No Python Gauss-sum binding is exposed.
- Probe: (5,4) has character order 2, group exponent 4, raw exponent at 2 equal to 2.
  A direct use of that exponent over character order would return 1 instead of -1.
- Probe: the golden input (8,7) has conductor 4. Its numeric golden is already lowered.
- No dirichlet.rst or pairing implementation is present under refs/src. Header declarations
  alone do not specify the pairing. Keep the missing source explicit.
- The prior review requires actual fault rejection, preflight before expensive work, and
  endpoint excess tests in addition to containment. These will shape the acceptance tests.

The first oracle test was run against a lowering stub and failed with NotImplementedError.
The implemented oracle passed 322524 assertions in 12 groups on its first full run.
It uses exact exponent matching for lowering and a separate C adapter for dirichlet_char_lower
and acb_dirichlet_gauss_sum at 256 bits. The adapter covers all 1966 characters of moduli 1..80.
The ABI probe gives size 120, alignment 8, offsets q=0, n=8, parity=16, s=24.

Coset proof written in working notes: the residues modulo C are the units congruent to c
modulo gcd(C,N). Choose one such unit a0; do not apply chi directly to the displayed c.
For (C,n,c,N)=(3,2,3,4), chi(c)=0 but the coset values are {1,-1}.
The image subgroup is computed by a gcd of exact exponents. Q4 applies with radius zero.

The numerical oracle returns an exact rational error bound for each 60-digit mpmath Gauss
sum, obtained from an independent 256-bit python-flint phase sum and exact dyadic distances.
This certifies the returned point without assuming a rounding guarantee for mpmath.

Drafted docs/api-3c.md, within 450 lines. Lifecycle, constructors, field/order getters, conjugation,
typed text/dump/inspect, zero-or-phase evaluation, unit/class/idele evaluation, tau and W now have contracts.
P1 proves the exact coset image and singleton criterion; P2 proves all four rectangular extrema;
P3 proves exact lowering and states the unresolved FLINT pairing identification; P4 bounds the sum.
The sum design uses exact dyadic endpoint additions and one final radius rounding, avoiding an
unaccounted error from repeated mag additions. Its rational model passes 812 sum-bound assertions.

The revised oracle passes 323349 assertions in 13 groups. It includes 12 rational-rescaling checks
of idele evaluation and the constructor cap example (65536,1). Twelve executed reference mutations
were killed by assertions; zero survived and zero had harness errors. These are Python mutations,
not tests of an implementation that does not yet exist. Strict output mutation uses a reference slot.

One proposed decision remains: a 65536 setup/summation cap and the constructor status-row exception.
Conjugation and dump implementation are scheduled in slice b; no new dump grammar is proposed.

Citation audit corrected a draft overstatement: conventions:929-930 says real-character golden
vectors, not that every input pair is primitive. The (8,7) lowering is an integration check, not
a finding against the conventions or golden. The two findings concern the brief's raw-exponent
denominator and its literal chi(c) formula for an integer coset residue.

Final checks: 323350 oracle assertions, 13 groups, 5.35 seconds elapsed; 12 of 12 reference
mutations killed, no survivors or harness errors. Python-flint uses FLINT 3.3.1; the C adapter
checks that both compiled and runtime versions are 3.0.1. Its final transcript has 1966 data
rows and two metadata rows. Standalone C compilation used -Wall -Wextra -Werror and exited 0.
The 393-line design and five other text/code artifacts have no lines over 116 characters;
all three Python files parse. Scratch compilation is now confined to the lane directory.
The standalone probe executable was removed; its source and transcript are retained.
