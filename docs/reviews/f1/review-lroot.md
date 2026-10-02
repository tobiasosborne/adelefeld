# f-review6: adversarial review of the local roots (lane f-slice8, WP 1F.5)

No wrong enclosure, wrong exponent, wrong count, wrong identifier, wrong status, changed output on a status or
memory fault was found. 4 findings, all MINOR: one limit, two costs, one false sentence in a proof.
Commands are run from the worktree root /home/user/adelefeld-wt/f-review6. Archive built once:
`timeout 600 make -j2 BUILD=lanes/f-review6/build lanes/f-review6/build/libadelefeld.a` (exit 0).

## Findings

### F1. MINOR: LIMIT for a small exact-image result (1 + 2^(2^27) Z_2, n = 2)

- Input: the ball x = 1 + 2^R Z_2, R = 2^27 (canonical: u = 1, v = 0, N = 2^27), n = 2, seed 1, any N.
- Code returns `LIMIT`. The same happens for 1 + 2^(2^60) Z_2 (n = 2) and 1 + 5^(2^60) Z_5 (n = 3).
- True: the image is exactly 1 + 2^(R-1) Z_2 (Proposition 15, s = 1, j = 0). Its stored form is u = 1,
  N = 2^27 - 1, both inside the limits; no power of 2 is needed to state it. `adf_lball_Log` of the same ball
  returns `OK` (u = 0, N = 2^27) and `adf_lball_pow_si(1 + 2^(2^27-1) Z_2, 2)` returns `OK`: lfunc.h promises
  "no limit applies when the centre of the result is known without a sum ... the unit part of a is exactly 1".
  lroot.h says LIMIT for "a required power p^W"; src/lroot.c:154 tests `power_ok(p, L+s)` before Log, whether or
  not the power is required. Because N-D13 ignores N for balls, no request at all gives a value here.
- Reproducer: `timeout 120 lanes/f-review6/build/limits` (source lanes/f-review6/limits.c, first 4 lines of
  output; built with -fsanitize=address,undefined).

### F2. MINOR: all-branch enumeration cost, and LIMIT decided after it

- `identifiers()` (src/lroot.c:224-249) finds the d = gcd(n, p-1) roots of T^d - w^e with
  `nmod_poly_find_distinct_nonzero_roots` (Rabin's method, refs/src/flint-3.0.1/nmod_poly.rst:2392-2396).
  Measured: T^65536 - 1 at p = 65537: 5.88 s; T^6028 - 1 at p = 2^64-59: 5.76 s. Listing r0 zeta^k with
  zeta = g^((p-1)/d), g = `n_primitive_root_prime(p)` (refs/src/flint-3.0.1/ulong_extras.rst:1410), takes 0.55 ms
  and 0.21 ms for the same sets. `roots` at p = 2^64-59, d = 299756 (below ADF_LROOT_BRANCH_MAX = 1048575) did
  not finish in 100 s.
- `roots(x = exact 1 at 65537, n = 65536, N = 2^40)` returns `LIMIT` after 5.9 s; the seeded call with the same N
  returns `LIMIT` in 9 ms. For an exact input the limit test of an irrational branch depends only on N, j, s, and
  at most 2 branches are rational, so for d >= 3 the status is known before enumeration.
- Lane-named cost, measured once (p = 65537, n = 16, N = 200): `roots` of exact 3^16: 21.7 ms; of
  3^16 + 7*65537 (16 irrational branches): 33.0 ms; one seeded branch 1.47 ms, of which Log 0.35 ms and exp
  0.85 ms are recomputed identically for every branch (only the Teichmueller factor differs).
- Reproducers: `timeout 60 lanes/f-review6/build/polyroots 65537 65536`,
  `timeout 60 lanes/f-review6/build/polyroots 18446744073709551557 6028`;
  `printf 'A 65537 1 1 0 0 1 65536 0 1099511627776 65536\n' | timeout 110 lanes/f-review6/build/h` (time it);
  `timeout 120 lanes/f-review6/build/perf` and `... perf irr`.

### F3. MINOR: the requested N cannot bound the cost of a ball root (N-D13)

- Input: x = w^2 mod 2^R as the ball x + 2^R Z_2 (w random odd), n = 2, seed 1, requested N = 10.
- Code: OK with exponent R - 1 (correct, exact image). Time: R = 10^4: 0.20 s; R = 10^5: 51.8 s;
  R = 4*10^5: not finished in 110 s. `adf_lball_Log` of the same ball at N = 10 takes < 0.1 ms.
  The time is exp at p = 2 (expcost: K = 5*10^4: exp 3.9 s, Log 0.03 s). lroot.h says "No cost bound is
  promised", so this is not a contract breach; it is the measured consequence of N-D13 (see below).
- Reproducer: `timeout 110 lanes/f-review6/build/nd13 100000` (source nd13.c);
  `timeout 100 lanes/f-review6/build/expcost 2 50000`.

### F4. MINOR: a false sentence in R7 step 2 (docs/api-1f5.md:199)

- "On a guarded rootable ball, E = M - s - (n-1) j <= M." False for j < 0: p = 3, n = 2,
  x = 3^-2 (1 + 3 Z_3): m = -2, M = -1, r = 1, j = -1, E = 0 > M. Such balls are in the fixture: the rows scaled
  by p^(n*shift), shift = -1 (proto/lroot_checks.py:72-75). Those rows are not enumerated at H = M+1; their E is
  derived by scaling (E = shift + E0), which is correct. The sentence should be restricted to the integral grid
  (j >= 0). No test result changes. Checked by hand from the formula; the harness confirms the code's value:
  `printf 'S 3 1 1 -2 -1 0 2 1 0 0\n' | lanes/f-review6/build/h` prints `OK 0 -1 0 1` (v = -1, N = 0).

## N-D13: exact image for balls, regardless of N

Recommendation: reverse to K = min(N, E) for ball inputs, as in lfunc.h. A caller who wants the exact image
passes N >= E (for example WORD_MAX, which already works for exact rational branches); a caller who wants a
coarse value at bounded cost cannot get one under N-D13. F3 measures this: 51.8 s for a 10^5-digit 2-adic ball at
requested N = 10, where min(N, E) needs a 10-digit computation. F1 shows the second consequence: a valid result
becomes LIMIT and no smaller request can avoid it. min(N, E) also makes roots compose with exp/log/Log, which
already cap at N, and loses no correctness: the ball of exponent min(N, E) containing the exact image is a valid
enclosure and is the image itself whenever N >= E.

## Oracle (own, independent of the lane's)

lanes/f-review6/oracle.py uses Python integers only: no logarithm, exponential, Teichmueller lift, finite-field
factoring, or C code. Notation: U a p-adic unit, s = v_p(n), c = 2 at 2 and 1 otherwise.

- O1 (dominance). For a unit y, a unit w and e >= c: v((y + p^e w)^n - y^n) = s + e. Proof: the k-th binomial
  term has valuation v(C(n,k)) + k e >= s - v_p(k) + k e, since C(n,k) = (n/k) C(n-1,k-1). For k >= 2,
  (k-1) e > v_p(k) holds (e >= 1 and p >= 3: k-1 > log_p k; p = 2, e >= 2: 2(k-1) > log_2 k), so the k = 1 term,
  of valuation s + e, is the strict minimum.
- O2 (branch existence and digit lifting). Branch t (t mod p, or 1/3 mod 4 at 2) of U exists iff
  v(t^n - U) >= s + c. If: from y = t, while v(y^n - U) >= s + e (e >= c), one linear digit d (mod p) gives
  v((y + d p^e)^n - U) >= s + e + 1, because by O1 the k >= 2 terms are >= s + e + 1; the limit is a root.
  Only if: a root rho = t + p^c z gives v(rho^n - t^n) >= s + c by O1 (or = 0 difference).
  The lifted y is the root modulo p^L: v(y^n - rho^n) >= s + L forces v(y - rho) >= L by O1.
- O3 (exact image of a ball). Let x = p^m U + p^M Z_p, r = M - m >= c + s, j = m/n, b = p^j u with u in branch t
  and v(u^n - U) >= r. Every y in the branch has v(y - b) = j + e with e >= c, and by O1
  v(y^n - b^n) = s + n j + e. So y^n is in x iff e >= M - s - (n-1) j - j, that is iff y is in b + p^E Z_p,
  E = M - s - (n-1) j. Conversely each z in x has a root there: the map b + p^E Z_p -> x scales all distances by
  exactly p^(M-E) (O1), so it is injective on each finite quotient of equal size p^k, hence bijective there, and
  surjective by compactness. Hence a returned ball is accepted iff its exponent is E, its valuation j, its unit
  residue the seed, and its centre unit u satisfies u^n = U modulo p^r. This is exact; no comparison precision is
  involved. Exact inputs: the centre must equal the O2 root modulo p^(N-j); for N <= j the ball p^N Z_p.
- Rational branches: integer n-th roots by bisection; n > 4096 treated as the unit +-1 case only.
- Self-test against brute force modulo p^(L+s+c), p = 2,3,5,7, n = 1..12: 6672 cases, 0 mismatches
  (`timeout 110 python3 lanes/f-review6/oracle_selftest.py`). Wrong values injected by hand (exponent 2 for 3,
  centre +1 at N = 6, wrong residue) are rejected.

## Attacked without result (counts; what would have made a case fail)

A case fails on: a status other than the oracle's; a ball with exponent != E (exact image) or valuation != j; a
centre unit with u^n != U mod p^r or residue != seed; a missed rational root, a claimed rational root that is not
one, an exact-input centre != root mod p^(N-j); a list whose identifiers differ from the oracle's set or order;
any write to y, ids, len (sentinels, including slots >= len) on a non-OK status.

- attack1.py (p = 2,3,5,7,13; n = 1..12, p, p^2, p-1, 2(p-1), 3(p-1); balls with j in -2..2, r from c+s-2 to
  c+s+4, rootable and random centres including m not divisible by n; exact rationals with denominators, signs,
  j in -1..1; every seed 0..p+1 (0..13 at 13; 0,1,2,3,5 at 2); N in -3..9; count and list; zero exact, zero
  ball, n = 0): 12 runs x 27316 = 327792 cases (each run: about 4650 seeded OK, 1640 lists OK, 16300 DOMAIN,
  700 NOT_DETERMINED), 0 failures. Seed 21 ran under ASan/UBSan with lroot.c and rfunc.c instrumented.
  `timeout 300 python3 lanes/f-review6/attack1.py 1` (seeds 1..11; log of 4..11 in attack1-long.log);
  `timeout 170 python3 lanes/f-review6/attack1s.py 21 lanes/f-review6/build/asan/h`.
- attack2.py (p = 65537 with n = 2,3,4,12,16,256,4096,65536,131072,65537,3*65536,2^63,2^64-1; p = 2^64-59 with
  n = 2,3,4,5,12,p,p-1,(p-1)/4,2^63-1,2^64-1; full lists when gcd <= 70000; ids checked distinct, sorted,
  t^n = U, count = gcd): 1092 cases, 0 failures (114 s). `timeout 200 python3 lanes/f-review6/attack2.py`.
- attack3.py (balls of relative precision 200, 1000, 1500, 3000 digits at 2,3,7,13 with n in 2,3,5,p,p^2,12;
  exact A/B with 300- and 1500-bit bases to the n-th power, perturbed and not, N in 3,50,400, p up to 65537;
  word-size n = 2^63, 2^64-1, 2^63-1, 2^62, 3^40, 5^27 with s up to 63): 1131 cases, 0 failures.
- R5 at p = 13 (n = 12, 24, 36, 6, 4, 3, 2) and 65537 (n sharing 2^16 with p-1): identifier lists equal the
  oracle's (13) or are d distinct sorted residues with t^n = w (65537); count = gcd(n, p-1). The proof of R5 is
  correct: gcd(n/d, h) = 1, the order-h subgroup is the image of the d-th power map, and T^d = w^e has d roots
  each with n-th power w, which is the full set by Lemma 3.
- limits.c (ASan/UBSan): exact 2^(2^60), n = 2: exact +-2^(2^59), no power; exact 2^-(2^60), n = 2^60: exact
  2^-1; exact 7^(2^60) 2: N = 2^60 LIMIT, N = 2^59+5 OK (centre 4567, 4567^2 = 2 mod 7^5), N = -2^60 zero ball,
  N = 2^60+1, WORD_MIN, WORD_MAX LIMIT; balls at v = +-2^60: LIMIT exactly when E or r leaves the bounds;
  exact 1 at 5, n = 4, N = 2^60: seeds 1 and 4 exact, seed 2 LIMIT; roots then LIMIT with all slots untouched;
  capacity 3, 0 (LIMIT), -1 (DOMAIN), 4 = count (OK); x aliasing each of the 4 slots; y = x.
- at.c (ASan/UBSan): seed_at/sqrt_seed_at/roots_at on a partial ball with real, 3, 5, 7 components: only place
  v in the output, arch NONE; missing place 11: DOMAIN, where = 11; real place present: UNSUPPORTED, absent:
  DOMAIN; where untouched on OK, = v on every failure; y = x on success and on failure (x unchanged).
- Driver (drv1.cmd, drv2.cmd, drv3.cmd): unit coset, idele, class operands: UNSUPPORTED; real place with an adele:
  UNSUPPORTED, with a finite ball: DOMAIN; primes 1, 0, -5, 4, 2^64-57: DOMAIN; p = 2^64-59: correct;
  degrees 0, -2, 2^64, a ball: DOMAIN; `x`, `2.0`: PARSE; seed -1 at 5: DOMAIN (header: no modular reduction);
  fifth operand, trailing `with`, missing seed: PARSE; -8 at 3, 1/9 at 5, -1 at 2 (n = 3), 16 at 5 (n = 4, both
  signs rational, i-branches balls), 9 at 2: as expected. Printed values equal the library's (negative
  valuations printed as u/p^k). Observation only: the printed label `[+1]` is not accepted back as a seed
  (`+1` is PARSE; README documents `1` and `-1`).
- Statuses at 2: 3 DOMAIN, 9 roots +-3 (ids 3 and 1), 17 square, 5 DOMAIN, 1 + 4 Z_2 NOT_DETERMINED,
  1 + 8 Z_2 two branches at exponent 2, 3 + 4 Z_2 with n = 3 OK at exponent 2; 1 + 3 Z_3 with n = 3
  NOT_DETERMINED; 4 + 27 Z_3 with n = 3 DOMAIN; 125 + 625 Z_5, n = 3: 5 + 25 Z_5 (17: 41 + O(2^6) and
  23 + O(2^6) at N = 6). Command:
  `printf 'A 2 17 1 0 0 1 2 0 6 4\nA 2 5 1 0 0 1 2 0 6 4\n' | lanes/f-review6/build/h` and drv2.cmd.
- Proofs R1 to R6 refereed step by step: no false step found besides F4. R2 at p = 2 with r - s = c checked by
  O3 (e.g. 1 + 8 Z_2 -> +-1 + 4 Z_2). R3 covers negative A with even n (no rational root; ball returned).
  R6 overflow bounds hold: |j| <= 2^59, M - m <= 2^61, s <= 63.
- Lane tests: test_lroot.c checks status, ids, exponents, centres and untouched outputs against fixtures and
  pow_si; no assertion that cannot fail was found (contains(x, y) means x inside y, so the witness checks bite).
- Not exercised: d = ADF_LROOT_BRANCH_MAX exactly (needs a prime p = 1 mod 1048575 and a 10^6-degree
  factorisation); Julia test not run.

## Files written (all under lanes/f-review6/)

h.c (harness), oracle.py, oracle_selftest.py, attack1.py, attack1s.py, attack2.py, attack3.py, limits.c, at.c,
perf.c, polyroots.c, nd13.c, expcost.c, drv1.cmd, drv2.cmd, drv3.cmd, nd13_p3.in, attack1-long.log, build.log,
progress.md, result.md; binaries in build/ (build/asan/h with instrumented lroot.c, rfunc.c).
