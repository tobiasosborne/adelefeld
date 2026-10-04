# Progress

Read the lane rules, SPEC 6, PLAN 3.1 to 3.4, quotient proofs and review, analysis Lemma 2,
the conventions, existing headers, earlier API designs, and the S design review.

The design will retain the qclass struct and distinguish stored sets from unknown points.
Reduction must account for exact finite points and for real spill outside the closed domain.
The phase oracle will return a rational angle or a finite union of closed rational angle intervals.

First contracts designed: lift copies the adele without rounding; rational translation copies the class.
These implement quotient P10.1. No context is created or owned by a class.

Initial executable contract tests are written before the oracle. The first run must fail on its missing import.

Lifecycle, raw-pieces constructor, layout access, reduction, set predicates, group arithmetic,
and typed text/dump declarations are designed in docs/api-3.md sections 1 and 2.
Reduction uses the closed construction count before deduplication. No single-piece-only API is proposed.
Set queries use exact fibers, with positive cosets and residual exact integer points.
The first oracle run passed 4 groups and failed the non-dyadic endpoint assertion at precision 20.
The proposed rounding kernel now specifies one 30-bit successor beyond the upward radius bound.
This is an explicit output policy, with an error bound to prove, not an assertion of FLINT bit identity.

Character functions designed: adele and class default/strict, local and place variants,
exact rational phase getters, and phase-to-acb evaluation. Their output hull and error allowance are stated.
Q1 proved: legal midpoint and a quantified outward radius margin.
Q2 proved: exact set relations with mixed positive cosets, singleton fibers, endpoint gluing, and spill.
Q3 proved: addition and negation descend to sets of classes; rational translation is exactly identity.
Q4 proved: four rational distance calculations determine phase extrema, width, and singleton criteria.
Q5 proved: full-image width criterion for positive fractional radius; zero radius is excluded.
The second main oracle run passed 10 groups. Further golden and width checks are being added.

Acceptance tests now cover every proposed function family, with six explicit wrong alternatives per work package.
Three decision rows remain: bounded set-query status, construction/rounding/resource policy, class strict semantics.
The thin-slice map includes driver verbs and Julia signatures; the first slice is a lift-only callable feature.
Findings F1 to F5 have concrete witnesses: zero radius, set/point comparison, strict representation,
rounded translation, and an inside-domain enclosure of a non-dyadic-endpoint interval.

Final oracle additions: local images, exact ball-image additivity, positive fractional full-image thresholds,
Q1 error bounds, all 29 qclass text vectors, 15 valid finite phase vectors, and 17 Gauss boundary vectors.
The exact user command passed 15 groups. The static audit passed 40 distinct declarations and found
no overlength lines after table reflow. Sources pending are explicitly limited to the thesis attribution
and the external Conrey exponent labeling documentation; neither is substituted for a proof of quotient results.
No C tests, Julia tests, production mutation runs, or overnight fuzz runs have been performed in this lane.
