# S.2 Algorithm P adversarial review

## Findings

No new finding in the reviewed functions, header, tests, or `solvers.md` section 3.

## Attacks without a finding

- `planted_probe.c`: 1,800 polynomials at p = 2, 3, 5. Each was a nonzero scalar times
  two to four distinct integer factors, some repeated up to three times. The roots had
  distances up to p^7. The only roots in Z_p are the planted integers: a product is zero
  in Q_p only when a factor is zero. For the normalised product, the derivative valuation
  at root r_i is the sum of v_p(r_i - r_j) over j != i. Across 16,620 partial calls,
  6,945 were incomplete. The probe checked that every planted root lies in exactly one
  ball or class; every ball contains one planted root and has the expected s; complete
  agrees with nu = 0; strict agrees with partial status; entries and complete verifiers
  agree. It counted 33,416 certificates, 7,136 unresolved classes, and 0 failures.
- `residue_probe.c`: 6,000 small coefficient polynomials of degree 2 to 6 at p = 2, 3, 5.
  It enumerated every x modulo 2^14, 3^9, or 5^6, respectively, with f(x) = 0.
  For every enumerated x it computed s = v_p(f'(x)); 5,998 cases had M > 2s for all such x.
  The other two cases were excluded. In a usable case, Taylor expansion gives a Newton
  correction of valuation at least M - s. Repeating it increases the precision, since
  M - s > s. For two roots in the same class modulo p^(s+1), put t = v_p(alpha - beta) > s.
  Taylor expansion of f(alpha) - f(beta) has a first term of valuation s + t; every later
  term has valuation at least 2t > s + t. Their difference cannot be zero.
  Thus each group of residues modulo p^(s+1) is one root; every root appears among the
  enumerated residues. The probe compared those groups with every complete ball at depth
  12 and checked its representative precision before comparison. It also checked coverage
  in partial lists at depth 0. It found 4,632 root groups, 3,302 partial calls with roots,
  and 0 failures. A case failed on a wrong status, count, coverage, or verifier result.
- `edge_probe.c`: six directed inputs: X(X - 2^20) at 2; three roots separated by 3^7;
  a repeated root of f that is simple in g with content 27; no root modulo 2; all five
  residues roots of X^5 - X modulo 5; and two roots at p = 1048573. It checked partial
  unresolved counts below completion, strict status there, complete counts at the next
  depth, and verification at that depth. Six inputs, 0 failures.
- `build/test_roots_padic`: 8 tests, 55,131 checks, 0 failed checks, 0 failed tests.

## Checks run

All executable programs and `make` commands were run under `timeout`. All builds used
at most two cores. Compilation used this command, with `FILE` and `OUT` replaced by the
listed paths:

```
cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror \
  FILE build/libadelefeld.a -lflint -lgmp -lm -o OUT
```

- `timeout 180 make -j2`: exit 0; `build/libadelefeld.a` built.
- Compile `lanes/s2p-review/planted_probe.c` to `lanes/s2p-review/planted_probe`:
  first exit 1 for a precedence warning; after a source correction, exit 0.
- `timeout 160 lanes/s2p-review/planted_probe`: exit 0; 1,800 cases, 0 failures.
- Compile `lanes/s2p-review/residue_probe.c` to `lanes/s2p-review/residue_probe`:
  exit 0. After adding a representative precision check, recompile: exit 0.
- `timeout 150 lanes/s2p-review/residue_probe`: run before and after that check, both
  exit 0; each reported 6,000 attempted, 5,998 usable, 4,632 roots, 0 failures.
- `timeout 180 make -j2 build/test_roots_padic`: exit 0.
- `timeout 180 ./build/test_roots_padic`: exit 0; 55,131 checks, 0 failed.
- Compile `lanes/s2p-review/edge_probe.c` to `lanes/s2p-review/edge_probe`: exit 0.
- `timeout 120 lanes/s2p-review/edge_probe`: exit 0; six cases, 0 errors.

## Files written

`lanes/s2p-review/planted_probe.c`, `residue_probe.c`, `edge_probe.c`, their same-named
executables, and this report. `make` generated objects and binaries under `build/`.

## Not done

No sanitizer or leak run. No source mutation. The two residue cases without the strong
derivative bound were excluded from the independent oracle; the planted probe still
covers multiple factors and close roots. Primes above the temporary evaluation bound
were not tested, since the specified status there is UNSUPPORTED.

## Sources pending

None. The oracle claims above are proved directly. No external formula was quoted.

## Findings against the specification

No new finding. The S-D18 pre-allocation conflict for derived limits was already recorded
in `lanes/s2-slice2/result.md` and `docs/reviews/s2/review.md`; it is not repeated as a
new result here.
