# Progress

Read CLAUDE.md, workflow rules, the d-char brief and design, SPEC 6 to 8, PLAN 4 and 5,
conventions 3.2, 5.11, 5.12, 6, 9 to 11, analysis P4 to P7, P9 to P12, L14 and P15.
Read the finite, adelic, partial-place, idele and phase interfaces and the existing analysis oracle.
No ffun.h or rfun.h exists yet. The dump bodies are already defined in conventions 10.1.

Design work: use paired real and finite functions, with borrowed arrays of pairs for finite sums.
Check refinement, exact rational dilation, and finite unit ambiguity before choosing declarations.
Partial-place evaluation must state its completion semantics: absent places have no default values.
The row for integrals and Poisson exists; the status row for test-function algebra needs a decision.

Sources read on disk: refs/src/flint-3.0.1/arb.rst (enclosure contract), acb.rst (complex balls and roots),
refs/src/tate-poonen/notes.txt (character and transform convention). No source fetched from memory.

Design now covers lifecycle/text/dump, finite algebra, both transforms, evaluation, integrals/norms,
two Poisson truncations, seven implementation slices, and six mathematical fault witnesses per package.
It has 456 lines before final review. Every line is at most 116 characters.

Oracle runs: first failed at a mistaken partial-place expected set ({2}, corrected to {2,5}); second
reached timeout 120 in the near-zero Gaussian ratio search. Added a prefix-work preflight and retained
a lower-bound witness that the cutoff is insufficient. Third run passed 2067 checks. Fourth passed
2099 after adding golden-derived transforms and certified Poisson widths. Fifth passed 2100 after
making all six Poisson fault witnesses explicit and checking the direct-transform cap boundary.
Further nontrivial unit-dilation checks are being added before the final run.

Current findings: no mathematical counterexample to SPEC/PLAN/goldens. Conventions lacks an explicit
status classification for function algebra. Fixed parameter uncertainty prevents arbitrary final width;
the theta witness has width at least 1.08643481121330801. This qualifies P15's width wording, not its formulas.

Final run: timeout 120 python3 proto/functions4_checks.py exited 0 with 2670 checks.
Nontrivial unit dilation added 130 checks; Fourier dilation covariance added 440 checks.
Final design: 457 lines. Oracle: 640 lines. No line in either file exceeds 116 characters.
The final runtime is python-flint 0.8.0 with FLINT 3.3.1; no FLINT 3.0.1 C execution is claimed.
All check invocations and earlier results are recorded in check-commands.md.
