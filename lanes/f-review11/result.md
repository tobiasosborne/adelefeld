# f-review11: result (bug hunt through src/symbol.c, lane f-slice12)

Worktree: /home/tobias/Projects/adelefeld/.claude/worktrees/agent-a4462a6ee46df36b8
Checked first: `git log --oneline -1` = 88feb10; brief and src/symbol.c exist. I created the symlink `refs/src`.

## Findings

None. I found no wrong sign, no `OK` that a point of its input contradicts, no memory fault, no undefined
behaviour, no leak, no status or `where` violation, no driver defect. Nothing below is a finding that I
did not reproduce; I report none.

## What I attacked, with counts (all programs in lanes/f-review11/t/, oracle in t/orc.h)

Own oracle (orc.h), from definitions, no FLINT symbol function, nothing imported from the repository:
- Legendre: squares counted modulo p (p <= 3000), Euler's criterion by own modpow above.
- Jacobi/Kronecker: trial division of the lower entry, Definition 1 of catalogue.md (the (a/-1) rule, (a/0) = [a = +-1],
  (a/2) from a mod 8). This agrees with the header (`symbol.h` lines 29-30, 36-37: Kronecker accepts every integer b).
  I found no disagreement between header and Definition 1 (catalogue.md:17-20).
- Hilbert: (a,b)_p = 1 iff a x^2 + b y^2 = z^2 has a primitive solution (x,y,z) modulo p^k (any (x,y) not both divisible
  by p with any z; otherwise z a unit), by brute force over x, y mod p^k. Inputs are reduced to the classes
  (parity of v_p, unit mod p^k up to unit squares, found by brute force). k = 3 for odd p and 5 for p = 2. Why enough:
  after removing squares, v_p(a), v_p(b) in {0,1}; a primitive point modulo p^k with k >= 3 (odd) or 5 (2) lifts by
  Hensel (the gradient (2ax, 2by, -2z) is nonzero modulo p, or modulo 2 after dividing, at a primitive point). Check run:
  k vs k+1 (p = 2 with 6, p = 3, 5 with 4, 7, 13 with 3 only), 8000 pairs in [-20,20]^2: 0 differ.
  Real place by signs.

Step 1 (t1.c; 518578 comparisons, 0 failures):
- Legendre at 3,5,7,11,13,101,3001,10007 and Jacobi/Kronecker, a in [-150,150], b in [-60,60] (including 0, 1, -1, 2,
  even, negative; Jacobi DOMAIN and `*value` untouched for b <= 0 or even): 75250.
- Kronecker, 400 numerators of 2000 bits, 19 lower entries (0,+-1,+-2,4,6,+-8,12,+-15,45,1000,-999,2^20,2^30+5,
  3*5*7*11*13*17*19*23,-2^40): 7600.
- Hilbert, nonzero integers a, b in [-130,130], places inf,2,3,5,7,13: 405600.
- Hilbert, rationals with numerators up to 2000 bits, denominators up to 600 bits, 0 to 4 extra factors p in numerator or
  denominator, p in 2,3,5,7,13, 6000 each: 30000.
- All 64 square-class pairs at 2 (valuation parity x unit in {1,3,5,7}, twice: representatives and shifted lifts): 128.

Step 2 (t2.c, t3.c):
- `adf_lball_hilbert`, p in 2,3,5, v in -2..2, N-v in 1..4, several units each, plus exact units +-1..7: 91950 ordered
  pairs; for each, every point of both balls modulo p^(N-v+3) (342292725 point pairs) through the oracle. `OK` cases
  87786: all points agree with the returned sign. `NOT_DETERMINED` cases 4164: in every one both signs occur
  (0 refusals with a constant sign). Zero-containing and exact-zero balls tested in t5 (statuses).
- `adf_idele_hilbert_at`, p = 2 (685584 cases), 3 (342792), 5 (17140, thinned): scales 1,3,2,4,5/7,1/3,9/2,1/8,1/25,3/4,7,27/5;
  unit cosets (c,N) with N in 0..160 (N = 0 exact +-1; N with p-part 0 to 4; N not normalised like 2, 6, 10); points c + N t modulo
  p^3 or 2^5 excluding non-units at p. `OK`: 237636 + 231208 + 13775, all points agree. `NOT_DETERMINED`: 447948 + 111584 + 3365,
  both signs always occur (0 refusals with constant sign).
- Residue symbols on `adf_ucoset` (140676 calls: b in 27 values incl. 0, negative, even, 3 primes for Legendre; N in 0..96 with
  every c prime to N (thinned above 40)): 11585 `OK`, each checked against all unit residues mod lcm(N,K) congruent c
  (0 failures); 84667 `NOT_DETERMINED`, of which 15951 would have a constant symbol (conservative K | N rule, header
  lines 11-14, 34-35 promise this; not a finding). `adf_fball` (A + H Zhat)/d with H in 0..50, d in 1..4, A in -12..12:
  256500 calls: 10194 `OK` all verified by enumeration; 33618 `DOMAIN` verified against gcd(H,d) | A; 8306 refusals with
  a constant symbol (same rule). Exact N = 0 / H = 0 results compared with the oracle.

Step 3 (t4.c; 178000 Hilbert queries up to 4836-bit numerators, 4000 Jacobi/Kronecker queries, 0 failures): symmetry,
bilinearity in the first argument, (a,-a)=1, (a,1-a)=1, (a,a)=(a,-1), at inf,2,3,5,7,13,31, 2^61-1, 2^64-59; the product
formula over inf and the 16 primes <= 53 (a, b products of those primes to powers up to +-30, times a random square of
1000-2000 bits): 2000 cases; Jacobi reciprocity, supplements (-1/n), (2/n), (X^2/n)=1, and (a/bc)=(a/b)(a/c) for
Kronecker with random signed/zero/even lower entries, numerators of 2000 bits: 2000 cases each. (My first run of the
product formula failed 488 times because my own generator multiplied by a number with prime factors outside the list;
fixed in the test, not in the library.)

Large primes (t6.c, t7.c): rat, idele (from `adf_idele_set_rat`) and lball agree with each other and with the oracle
at 10 places (inf, 2, 3, 5, 7, 13, 1000003, 2^61-1, 2^64-59, 2^64-95): 30000 cases. t7: valuation-bearing pairs at
2^61-1, 2^64-59, 2^63-25, 2^63+29, 2^64-83, 2^32-5, 2^32+15 against the formula with Euler Legendre: 21000 rat/lball/idele
triples; and large-prime Legendre/Jacobi/Kronecker/unit-coset: 56000. 0 failures. (An early false alarm at 2^61-1:
my random numerators were multiples of p; fixed in the test.)

Step 4 (t5.c, t8.c; 132 checks, 0 failures): Legendre at inf and 2 (DOMAIN, where = the place, value untouched, NULL where);
fball and ucoset Legendre `NOT_DETERMINED` writes where = p; Jacobi DOMAIN/ND leave where untouched; two lballs at 3
and 5 (DOMAIN, where = 3 in both orders); exact zero, zero-containing ball, exact zero with zero-containing ball
(DOMAIN in both orders); `OK` leaves where untouched (lball, rat, real); real: exact zero, +inf, NaN (arb_indeterminate),
radius inf, a ball [-2,0] (ND), a tight negative ball, exact zero with a zero-containing ball (DOMAIN); rat zero at
inf and at a prime; idele ND writes where = v; forged lball fields with v at LONG_MAX-3.., LONG_MIN.., +-(2^62+1)
and N-v in 1..4 (p = 2): result equals the oracle's status and sign (no overflow, run also under UBSAN).

Step 5: library built with `make SAN=1 INV=1` into build-san; t3, t5, t6 and the driver under
`-fsanitize=address,undefined` with `ASAN_OPTIONS=detect_leaks=1`: no report from the library. (One leak report
in t6 came from my own unreleased `flint_rand_t`; after `flint_randclear` and `flint_cleanup` there is none.) With INV=1
every public canonical check passed on all inputs of t3, t5, t6.

Step 6 (t/h1.txt 88 lines, t/h2.txt 22 lines, each line fed to a separate `adf-san` process; 110 lines, 0 sanitizer
reports): operand counts 1 to 3 extra, 4 / 1 / 0 / -3 / 2^64 / 2^64-59 as prime, 3/1, 3/2, 5/2, 1e3, 0x10, 2.5, 1/0, `real` as
Legendre prime, local balls at the wrong place, partial balls without the component, ideles with r = 0 or wrong place
or unit mod 5 at p = 3 and 7, mixed kinds (rat with local ball and with idele), zero-containing real ideles.
Values printed agree with hand computation for 9 idele lines (derived from the formulas of catalogue.md:68-75) and with the
library calls for the rest; statuses are PARSE / DOMAIN / UNSUPPORTED / NOT_DETERMINED as README says.

## Evidence that the oracle can fail

Perturbed copies (flag PERTURB in orc.h, count of failures against the unchanged library):
- Legendre flipped (t1): 21372 failures; primitive-solution result flipped: 368128; real place flipped: 67600.
- lball (t2): 87786 failures with the flip (every OK case); idele p = 5: 13775 (every OK case).
- ucoset/fball (t3, flipped Legendre): 7609. Identities (t4): product formula flipped: 2000 of 2000; reciprocity flipped:
  1612 of 2000. t6, t7 with sign flipped: 30000 and 61203 failures.

## Not done

- Fballs of the local backend (no public constructor reachable from the driver; only global ones tested via `get_fmpz3`).
- No fuzz of seconds-to-minutes length beyond the above counts; every line of attack stopped after thousands of cases
  without a finding as the brief says. Idele units with N > 160 and lball p > 5 by full enumeration were not run
  (large-p coverage is by formula and identities, not by primitive solutions).
- The sentences of `symbol.h` on refusals (conservative K | N certificate) were not tested for sharpness: 15951
  ucoset and 8306 fball refusals with a constant symbol exist and are permitted by the header.
- I did not run the repository's test suites or mutation tests, and did not read `lanes/f-slice12/report.md`.
