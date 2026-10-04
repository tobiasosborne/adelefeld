<!-- ROLE: record of a review. The text below the rule is the report of lane f-review14 as written
     (lanes/f-review14/report.md, pi agent with stealth/space-bunny-alpha, 2026-10-05), not edited. Under review:
     Y1 to Y15 (docs/api-1f9.md), src/symbol.c, src/catalogue.c and their tests (lanes f-slice12, f-slice13, codex
     gpt-6.1-sol). A referee with planted faults, done in 30 minutes by a free model whose strength as a referee
     of proofs is not established: the fault tables are checkable, "no false step" is weaker evidence than the
     same sentence of review f-review13. F1 is repaired by a check added by the orchestrator. -->

# f-review14: referee of Y1 to Y15 (symbols and catalogue) and of the tests that cannot fail

Worktree as given. Lane directory `lanes/f-review14/`, nothing else written. The build tree and every
executable were deleted at the end; `/tmp` holds nothing of this lane.

Under review: `docs/api-1f9.md` (Y1 to Y15), `include/adelefeld/symbol.h`, `src/symbol.c`,
`include/adelefeld/catalogue.h`, `src/catalogue.c`, `tests/test_symbol.c`, `tests/test_catalogue.c`,
`proto/symbol_checks.py`, `proto/catalogue_checks.py`, `proto/catalogue2_checks.py`,
`tests/driver/symbol-*`, `tests/driver/catalogue-*`.

Files written (all in `lanes/f-review14/`):

| File | Content |
|---|---|
| `progress.md` | running notes, one entry per statement |
| `checks.py` | the 608 Hilbert fixture rows against Definition 4 and against `proto/symbol_checks.py` |
| `checks2.py` | library checks: binomial, power, cyclotomic, symbols, Proposition 2, mod 8 |
| `probe.c` | numeric entry points (`sym`, `hib`, `hibd`, `hid`, `bino`, `pp`, `cy`, `vol`) |
| `distinguish.txt` | 55 probe lines used to separate the planted faults from the library |
| `mutate.py` | 40 planted faults, one at a time, in scratch copies of `src/symbol.c` and `src/catalogue.c` |
| `show.py` | prints one line per mutant record |
| `report.md` | this file |
| `build.log`, `base-symbol.log`, `base-catalogue.log` | build and baseline logs (kept) |

Nothing in the repository outside the lane was created or changed. One slip: `checks.py` was written
once to the repository root by mistake and moved into the lane immediately; the root is unchanged.

## Findings

### F1. MAJOR (test gap). The order of the finest `g` limit and the domain check is not tested

- Input: `adf_ucoset_profpow_fine(y, &w, [1 mod 1], (301 + 602 Zhat)/2)`, that is the probe line
  `pp 2 1 1 301 602 2`.
- Fault C22 in `mutate.py`: the `g` limit is allowed to override the domain status,
  `st!=ADF_OK` replaced by `st = ADF_LIMIT` when `policy==2` and `gcd(e,M) > 256`.
- Code: `src/catalogue.c:160-163` runs `integral_status` first and returns its status, so the
  library answers `ADF_DOMAIN` (the triple is canonical, `gcd(602,2) = 2` does not divide `301`).
  The mutant answers `ADF_LIMIT`.
- True: `include/adelefeld/catalogue.h` says of the finest operation "Otherwise LIMIT, after the
  domain check and the constant exact cases above". `SPEC.md` 15.4 N-D19 gives the cap
  "`g = gcd(e, M)` at most 256, else `LIMIT`" without fixing the order against the domain check,
  so the header is the contract. The library's status is the documented one; the mutant contradicts
  the header.
- Every test passes the mutant: 9 tests, 131636 checks, 0 failed; all four catalogue driver cases
  pass. The probe line above is the only thing that separates them.
- Consequence: the wrong status only; no wrong value. The library is correct on this input.
- Reproduce: `python3 -B lanes/f-review14/mutate.py C22`, then
  `diff <(lanes/f-review14/build/mut/base/probe < lanes/f-review14/distinguish.txt)
  <(lanes/f-review14/build/mut/C22/probe < lanes/f-review14/distinguish.txt)`.

### F2. MINOR (test gap). The same order for the conservative and smallest binomial is tested; the
### profinite power is not

Recorded only to mark that the two halves of the same rule are not covered equally: the binomial
case is pinned by `tests/test_catalogue.c:132` (fault C20 is detected by one check), the power
case is not (F1).

No other finding. No BLOCKER. No false step in `docs/api-1f9.md`, in Propositions 1 to 15 of
`docs/proofs/catalogue.md`, or in the two headers was found. Nothing was found against
`docs/SPEC.md`.

## Part 1: statement by statement

Every check below is a program in this lane; the command and its numbers are in the table at the end.

**Y1 (exact symbols).** Claim: the three exact routines are Definition 1. Opened
`refs/src/pari-doc/usersch3.tex:9255-9280`: line 9263 says "total multiplicativity in both
arguments", lines 9266, 9268 and 9270-9271 give `(x|0) = 1 if |x| = 1 and 0 otherwise`,
`(x|-1) = 1 if x >= 0 and -1 otherwise`, `(x|2) = 0 if x is even and 1 if x = 1,-1 mod 8 and -1 if
x = 3,-3 mod 8`. This is Definition 1 at 0, negative `b` and 2, character for character. The
supplementary law at odd `a` is `(a/2) = (-1)^((a^2-1)/8)`, and `a^2 = 1 mod 8` for odd `a`, so the
exponent is an integer: Y1's sentence is right. FLINT domains confirmed at
`refs/src/flint-3.0.1/fmpz.rst:1166-1168` (Jacobi, odd positive `n`) and `:1170-1172` (Kronecker,
any `n`); `src/symbol.c:21-38` calls `fmpz_jacobi` only after `lower_status` has excluded even and
nonpositive `b`, and `fmpz_kronecker` only for `b <= 0`. My own values: 8660 calls of the probe
(`sym`), the value computed in Python from Definition 1 with the prime product for Jacobi and
`(a*a-1)//8` for the supplement, 0 mismatches, including 11 odd primes through 65537, all
`b <= 0` and all `b` up to 100. What would have failed: a wrong residue in the supplement, a
missing sign at a negative `b`, an abort on an even `b` for Jacobi (faults S1, S2, S9, S13 do
exactly that and are detected).

**Y2 (finite certification).** Two claims: sufficiency of `K | N`, and that the rule is sufficient
and not necessary. I enumerated, for every `b` in `-30..60` and every `N` up to 48 plus `K` itself,
all residues of `a + N Zhat` modulo `K`: 5680 certified triples, each with a single symbol value,
0 exceptions. That is Proposition 2 used in the only direction the code needs. The necessity claim
is refuted by the same scan: 438 balls with `K` not dividing `N` are nevertheless determined
(every point of `1 + 3 Zhat` has Jacobi symbol `+1` at 9; the code refuses it, as Y2 says). The
normalisation argument in Y2 is sound: `K` is odd or divisible by 8, so for `N = 2 mod 4` the
factor 2 in `N` cannot occur in `K`, and `K | 2m` iff `K | m` for `m` odd; the code uses the stored
`N` and gets the same answer as the normal form. Statuses over 8660 calls, 0 mismatches.

**Y3 (nonintegral finite balls).** Step 1: a point is in `Zhat` iff `A + Ht = 0 mod d` is solvable
in the profinite parameter, which by Bezout is iff `gcd(H,d) | A`. Step 3 needs canonicality:
`docs/conventions.md` 5.2 gives `gcd(A,H,d) = 1`, so for `d > 1` not both `A` and `H` are divisible
by `d`, and some `t` makes `A + Ht` nonzero mod `d`. Both steps hold; `src/symbol.c:73-87` implements
them. My first expectation table was wrong in 4 of 13 triples because the constructor canonicalises
the triple before the call; after canonicalising, 13 of 13 agree with `d = 1` gives `OK`,
`gcd(H,d) | A` gives `NOT_DETERMINED`, otherwise `DOMAIN`.

**Y4 (scope of denominator multiplicativity).** The counterexample `(-1/0) = 1` against
`(-1/0)(-1/3) = -1` follows from Definition 1 and from `usersch3.tex:9266`; it is the parity of
PARI's "total multiplicativity", so Y4's reading of the source is right. Where multiplicativity is
used: `tests/test_symbol.c:136-137` checks it for nonzero factors and skips `b = 0`; no file in
`src/` multiplies lower entries. Fault S11 (the odd formula's two unit factors exchanged) is
multiplicativity-adjacent and is detected 3146 times.

**Y5 (square classes of rationals).** Claim: for `a = p^alpha u` the parity is the xor of the two
nonnegative integer parities and the unit is the ratio of the stripped integers. My derivation: write
`q = n/d` with `d > 0`; `fmpz_remove` gives `v_p(n) = an`, `v_p(d) = bn`, both nonnegative, so
`v_p(q) = an - bn` and its parity is `an mod 2 xor bn mod 2`; the unit is `(n/p^an) / (d/p^bn)`,
whose residue modulo `p` (or 8 at 2) is the product of the two residues, both defined because the
stripped denominator is prime to `p`. No large valuations are subtracted, so negative valuations
cause no trouble. Checked against Definition 4 on all 608 rows of `hilbert.jsonl`: 0 disagreements
(`checks.py`). The formula-level claims of `n_mulmod2` (`ulong_extras.rst:375-379`, no restriction
on the words) and `n_invmod` (`:477-483`) were opened; `n_invmod` throws if the inverse does not
exist, so the comment "the inverse exists because the denominator is a local unit" is a real
precondition, not a hope.

**Y6 (why the finite search decides).** Read as a proof, not as a code description. Step 1:
`X^2 - u` at `X = 1` for `u = 1 mod p`, `p` odd, has value `0 mod p` and derivative `2`, a unit;
at 2, `u = 1 mod 8` gives value valuation at least 3 and derivative valuation 1, so the strict
inequality holds. Step 3: two sets of `(p+1)/2` elements in a field of `p` elements meet; at an
intersection with `y = 1`, `x` and `z` cannot both vanish because `w` is a unit, so a derivative
`2ux` or `-2z` is a unit. Step 4: `w y^2 = z^2 mod p` with `w` a nonsquare forces `p | y` and
`p | z`, so modulo `p^2` the left side vanishes and `p u x^2 = 0 mod p^2` forces `p | x`;
contradiction with primitivity. Step 5: with both valuations 1, `p | z`, divide by `p` and reduce to
`u x^2 + w y^2 = 0 mod p` with a unit derivative in `x` at `y = 1`; the condition is
`(-uw/p) = 1`, and `(a/b)_p = (-uw/p)` in that case. Step 6: an odd coordinate at an odd coefficient
gives derivative valuation 1 and value valuation at least 4, and `4 > 2`. Step 7: I re-derived the
case analysis; the only subcase left after "no odd coordinate at an odd coefficient" is `x, y` odd
with both coefficients even and `z = 2z'`, and then the divided equation has value `0 mod 8` with
derivative valuation 1, so `3 > 2` is enough. All four odd valuation parities and both parities at
2 are covered, so the proof is complete for what the oracle searches. The last claim was checked by
enumeration: `2x^2 + 6y^2 = z^2` has 32 primitive solutions modulo 8, including `(1,1,0)`, and none
modulo 16 (`checks2.py`, section `sec_mod8`).
The reduction in the oracle is load-bearing and is done in `proto/symbol_checks.py`: `unit_class`
returns the valuation **parity**, and `hilbert_search` raises the coefficient to that parity. I
first re-implemented the search with the full valuation and got the wrong sign on 17 of the 608
rows (for instance `a = 18, b = 3, p = 3`, where the true value is `(2/3) = -1` and an unreduced
search finds `+1` at level 9 because `9x^2` disappears). Y6's sentence "after reducing coefficient
valuations to 0 or 1" is therefore a condition on any reader of that script, not a remark.

**Y7 (finite precision, zero, scale cofactor).** For a canonical nonzero ball the predicate
`v < N` gives relative precision at least 1, so at an odd prime the unit residue modulo `p` is
fixed; at 2 the number of known digits is `min(3, N-v)` and `two_classes` keeps the residues
congruent to the centre modulo `2^k`; with `k = 1` that is all four odd residues, with `k = 3` one.
Every kept class is attained, since the ball is exactly `p^v u + p^N Z_p`. So the enumeration is
the complete set of square classes and the code's rule "a singleton sign is the value" is exact,
not merely sufficient; the bound `A - v(a) >= 3` at 2 in Proposition 7 is a sufficient bound that
the code improves on (it returns `OK` at relative precision 1 or 2 when the signs agree, which
Proposition 7 step 2 allows). Zero: exact zero is outside the domain (`DOMAIN`), a ball with `u = 0`
meets the domain and its complement (`NOT_DETERMINED`), and the maximum rule of conventions 3.3 is
applied by testing the exact zero first. Cofactor: with `r = s = 3` at 2 the units are `3` and
`3`, `eps(3) = om(3) = 1`, the exponent is `1 + 1 + 1 = 3` and the symbol is `-1`; with the
cofactor dropped it would be `+1`. Fault S4 removes the cofactor at an odd prime only and is
detected by 134 checks; fault S12 (the second operand not varied) by 665. Overflow: the two guards
`a->v <= WORD_MAX-2` and `a->v <= WORD_MAX-3` prevent the additions `a->v+2`, `a->v+3` from
overflowing, and the test drives `v` to `WORD_MIN` and `WORD_MAX`.

**Y8 (identities and the product formula).** The identities are on disk at
`refs/src/hilbert-bristol/lecture19.txt:19-24` (Proposition 2: symmetry, squares, `(a,-a)` and
`(a,1-a)`) and `:81-84` (Theorem 2, bilinearity); the product formula is Theorem 3 at `:89-91`. I
checked that no file under `src/` multiplies symbols over a set of places: there is no all-places
entry point in `symbol.h`, and the only product is in `tests/test_symbol.c:hilbert_product_2000`,
which is a test. So the product formula is a test, not a proof step anywhere in the slice.
The transcription finding is real and I confirmed it independently: the display at `:47-51` reads
`(u/p)^alpha (w/p)^beta`, the catalogue reads `(u/p)^beta (w/p)^alpha`; for `a = 3, b = 2, p = 3` the
catalogue gives `-1` and the display gives `+1`, and `-1` is right, because `z^2 - 2y^2 = 3` has no
solution in `Q_3` (modulo 3 it forces `y = z = 0`, then modulo 9 also `x = 0`). The catalogue's
attachment is also forced by `(u,p)_p = (u/p)`, which follows from `z^2 = u + py^2`. The slice does
not use the Bristol display, as Y8 says.

**Y9 (binomial enclosures and the smallest ball).** The claim to press is that `k` samples suffice.
Re-derived from Proposition 14 step 3 in both directions: `h(t) = binom(a+Nt,k)` has integer
Newton coefficients, `Delta^i h(0)` is an integer combination of `h(i)-h(0)` for `1 <= i <= j <= k`,
so the gcd `G1` of the first `k` differences divides every `Delta^i`, while
`h(j)-h(0) = sum_{i<=j} C(j,i) Delta^i h(0)` gives the other divisibility; continuity extends the
divisibility to profinite `t`, and the `k` integer points force minimality. Checked numerically:
1164 cases `(a, N, k)` with `a` from `-9..9`, `+-10^6`, `+-2^70`, `N` in `0,1,2,3,6,12,30,210` and
random triples, `k` in `0..8` and random; the radius from `j = 1..k` and the radius from
`j = 1..k+5` are equal in every case, and both equal the library's `H`; the conservative radius
`N/gcd(N,k!)` and the centre modulo the radius match in every case. Signed tops: `B_i` is integral
because a product of `i` consecutive integers is divisible by `i`, and for `a = -b` the product is
`(-1)^i b(b+1)...(b+i-1)`, so `fmpz_divexact_ui` is always exact; `a = +-2^70` is in the table.
The PLAN case `(0,8,4)` gives radius 2, and `a = 2, N = 3, k = 2` gives 9 while the conservative
radius is 3, as Y9 says.

**Y10 (the integral-input domain).** The proof is the congruence argument of Y3; the extra sentence
"If `d > 1` and the triple is canonical, at least one of `A,H` is not divisible by `d`" is
`gcd(A,H,d) = 1` from conventions 5.2, which I used. No `NOT_DETERMINED` boundary can occur among
wholly integral inputs, because `d = 1` gives `OK` for every `k`; my 1164 cases include `d = 1`
throughout. 13 hand triples with `d > 1` agree after canonicalisation.

**Y11 (profinite target and coarsening).** "Strict succeeds exactly when `D = N`" is
Proposition 12 step 1; I re-derived it: modulo `N` the base is `c`, and `c^e` against `c^{e+M}`
differ exactly when `c^M = 1`. Replacing `c^M` by its residue mod `N` changes `c^M - 1` by a multiple
of `N`, so the gcd is unchanged, and the same for negative `M`, where
`c^{-k} - 1 = (1 - c^k)/c^k` and `c` is a unit at `N`. Checked on 2190 triples `(c, N, e, M)`
with `N` up to 63, `|e| <= 8`, `M <= 12`, `c` coprime to `N`: the strict status, the coarse
modulus `canon(gcd(N,c^M-1))` and its centre agree with the library in every case, 0 mismatches.
The exact-base branches were read against the driver: `[1]` gives `[1]` before any limit,
`[-1]` with `M` even gives the parity of `e`, `[-1]` with `M` odd gives `NOT_DETERMINED` in the
strict interface and `[1 mod 1]` in both enclosures. The driver cases
`profpowfine [1] with 257` and `profpowfine [-1] with 1 mod 2` pin the first two.

**Y12 (the finest modulus without factoring `N`).** Each of the five steps was checked.
Step 1: the loop multiplies `A` by `gcd(rest, N)` and divides `rest`; at a prime `p` dividing `N`
each pass removes `min(v_p(rest), v_p(N))` from `rest`, so the accumulated product is exactly
`g_N` with `v_p(g_N) = v_p(g)` for `p | N` and 0 elsewhere; termination is strict decrease of a
positive integer. Step 2: `A = gcd(N g_N, c^M - 1)` computed from the residue `c^M mod A` gives
`v_p(A) = min(v_p(N) + v_p(g), v_p(c^M - 1))`, the first row of the table; `M = 0` keeps `A = N g_N`,
as `v_p(0) = infinity`. Step 3: `adf_ucoset_pow_tight(U(1), g)` is the table of
`docs/proofs/ideles.md` Proposition 13 with `a_p = 0`, namely `1 + v_p(g)` at odd `p` with
`p - 1 | g` and `2 + v_2(g)` at 2 when `g` is even, 0 otherwise; the repeated division by
`gcd(B,N)` removes every prime power at a prime dividing `N` and terminates the same way.
Step 4: `A` is supported on the primes of `N` and `B` on the others, so they are coprime and the
CRT call's domain holds; `A = 1` forces the residue 1, `B = 1` forces `c^e`, and `canon(F)` is
applied at the end. Step 5: `M = 0` reuses `pow_tight` with `|e| <= 256`, forced by the earlier `g`
limit. The whole construction was compared with an independent brute force: for each prime `p` up
to `g+1` and each prime dividing `N`, I enumerated the image of `(c w)^{e+Mt}` over all admissible
`w` and all subgroup elements of `(c w)^M` modulo `p^k`, found the largest `k` at which the image is
a singleton and its value, combined by CRT, and compared with the library: 2190 cases, 0 mismatches
for the modulus and for the centre, including `c = -1` exact units, `N` even, `c` and `N` sharing
primes, and `c^M = 1 mod N`. The `canon(F)` case `v_2(F) = 1` occurs at `N = 4, c = 3, e = 0, M = 1`
(`[1 mod 1]`) and is in the probe file.

**Y13 (Haar volume and content).** `adf_fball_haar_volume` is `src/fball.c:616-624`: 0 when `H = 0`,
else `d/H`, reading the fields of the triple, so it does exactly what Y13 says for a canonical
input, and it is the same routine the local backend reaches (the test builds a local value from
`(0 + 8 Zhat)/3` and gets `3/8`). Steps 1 to 4 of the index proof: `Zhat/d Zhat` has `d` residues and
kernel `d Zhat`; `Zhat` is a disjoint union of `H` translates of `H Zhat`, so `vol(H Zhat) = 1/H`;
`(H/d Zhat)/(H Zhat)` is carried to `Zhat/d Zhat` by `d/H`, so `vol = d/H`; a point lies in
`a + n Zhat` for every `n`, so its volume is 0 by monotonicity. Step 5: an idele component `r u_p`
with `u_p` a unit has valuation `v_p(r)`, so the ideal is `r Z`; `adf_idele_content` returns the
stored `r` and is not duplicated. Nothing in the slice touches this code, so no fault could be
planted there. The test covers 11 x 13 x 7 = 1001 centre/radius/denominator triples, a thousands-bit
radius and a local value; I did not recompute those numbers, I read the routine that produces them
and checked that it matches the statement for every canonical input.

**Y14 (the two cyclotomic exponents).** Lines checked on disk: `refs/src/milne-cft/CFT.txt:9883`
"In this case, the global reciprocity map is the reciprocal of", `:9884-9886` naming the projection
and "the canonical isomorphism"; `:9904-9908` property (b), the idele with 1 at all places but one
mapping "to the Frobenius element"; `:1307` "This sigma is called the Frobenius element of";
`:3162-3166` the action of `Zhat^x` on roots of unity by `zeta -> zeta^u`. So the inverse
exponent is the global reciprocity map and the direct exponent is the canonical isomorphism, and
the names of `catalogue.h` match conventions 6.6. Necessity and sufficiency of
`canon(n) | canon(N)`: I re-derived the converse of Proposition 15 step 3 (the unit `1 + q^j`, and
`3` at `q = 2, j = 0`, and the fact that `canon(n)` never has `v_2 = 1`) and checked the whole rule
against an independent computation on 8234 cases (`canon(n) | canon(N)` or an exact unit, otherwise
`NOT_DETERMINED`), 0 mismatches, and the exponent against `c mod n` with the unique odd lift when
`n = 2m`, inverted by a modular inverse, 0 mismatches. The odd lift is forced: `m` is odd, so
exactly one of the two lifts is odd. `n = 1` gives 0, and FLINT admits modulus 1
(`fmpz.rst:1154-1160`). The specification vector `p = 3` gives `[19 mod 63]`, `uinv` returns 3
modulo 7 and 1 modulo 9, `u` returns 5 and 1; all four values are in the driver and the test. The
attribution of the words "arithmetic" and "geometric" remains pending as the text says.

**Y15 (the elementary precision proof).** Step 1: `v_p((1+z)^p - 1) = n+1` needs
`1 + i n > n + 1` for `1 < i < p` and `p n > n + 1`; the first holds for `n >= 1, p >= 3`, the
second for `p n - n - 1 = n(p-1) - 1 >= 1`; at 2 there are no middle terms and `2n > n + 1` is
`n >= 2`. Step 2: every later term of `(1+z)^m - 1` has valuation at least `2n > n`, so the value
is `n`; iterating step 1 then step 2 gives `n + v_p(g)`, and negative powers differ by a unit.
Step 3: Fermat by cancelling the permutation product; the exponent `E` is realised by one element
because one element per prime divisor of `E` can be raised to a pure prime power order and the
product of elements of pairwise coprime orders has the product order; the root bound gives
`p-1 <= E` and `E | p-1` gives `E = p-1`. Step 4: constancy of all `g`-th powers needs
`p-1 | g` because `E` is realised; then `b^{p-1} = 1 mod p` and step 2 gives depth `1 + v_p(g)`,
attained by `1 + p`. Step 5: at 2, `3` separates modulo 4 for odd `g`, and for even `g`
`v_2(b^2-1) >= 3` because `b-1` and `b+1` are consecutive even numbers; step 2 with `n >= 2` gives
depth `2 + v_2(g)`, attained by 5 since `v_2(25-1) = 3`. Step 6: `w^{e+Mt} = 1` for all `t` is
equivalent to `w^g = 1` by Bezout, and the base 1 outside `N` forces the output residue 1. The three
rows of Proposition 13 and the CRT centre follow, and the library matches them on 2190 cases. No
external theorem is used, and the pending attribution is untouched.

## Part 2: planted faults

Faults are patches of one line or a few lines of `src/symbol.c` (18, prefix `S`) and
`src/catalogue.c` (22, prefix `C`), written to `lanes/f-review14/build/mut/<ID>/`, compiled, and
substituted for the object of the same name in a copy of the archive. Each mutant is run against
the matching test program, the six driver cases of its slice (byte comparison of the output and the
`#!exit` status) and the 55 probe lines. None of the faults is one of the six of
`lanes/f-slice12/faults.py` or the sixteen of `lanes/f-slice13/faults.py`; the two that came close
(`C2` as `j < k`, `C12` as a `canon` change) were replaced or analysed, see below.

Unmutated: test_symbol 11 tests, 719322 checks, 0 failed; test_catalogue 9 tests, 131636 checks,
0 failed; all six driver cases pass; 55 probe lines.

### src/symbol.c

| ID | Fault | test_symbol | driver | probe |
|---|---|---|---|---|
| S1 | `(a/2)`: the supplement accepts 3 mod 8, so 3 and 5 are not separated | 908, 3 | residue | differs |
| S2 | sign for a negative lower entry dropped | 259, 1 | residue | differs |
| S3 | precision rule at 2: 2 digits read as 3 | 130, 2 | hilbert | same |
| S4 | scale cofactor `r'` dropped at an odd prime only | 134, 1 | - | same |
| S5 | real place at a rational pair: `+1` always | 689, 2 | hilbert | differs |
| S6 | `where` written on success as well as on failure | 2861, 4 | - | differs |
| S7 | different local primes: the larger prime is reported | 1, 1 | - | differs |
| S8 | Kronecker certificate modulus: the factor 8 dropped | 78, 1 | residue | differs |
| S9 | Jacobi accepts an even lower entry | 8, 2 | residue | differs |
| S10 | nonintegral ball: `DOMAIN` and `NOT_DETERMINED` exchanged | 12, 1 | - | differs |
| S11 | odd formula: the `u` and `w` factors interchanged | 3146, 6 | hilbert | differs |
| S12 | `class_result`: the second operand is not varied | 665, 2 | hilbert | differs |
| S13 | even numerator at a positive even lower entry: 0 dropped | 4615, 2 | residue | differs |
| S14 | positive radius, nonpositive lower entry: a value is returned | 8, 1 | residue | differs |
| S15 | odd local ball: the unit residue is taken to be a square | 342, 2 | - | same |
| S16 | 2-adic unit classes: the known-digit mask is one digit short | 816, 4 | hilbert | same |
| S17 | idele at an odd prime outside `N`: the unknown unit is a square | 394, 2 | - | differs |
| S18 | `where` never written on failure | 1631, 5 | - | differs |

18 faults, 18 detected by the test program, 0 survivors. One of the two driver cases detects 11 of
the 18; the tests are the contract, so this is coverage information, not a defect. S3, S4, S15 and S16 are
not separated by my probe lines either, only by the fixtures.

### src/catalogue.c

| ID | Fault | test_catalogue | driver | probe |
|---|---|---|---|---|
| C1 | radius `N/gcd(N,k!)` replaced by `N/gcd(N,(k-1)!)` | 146, 2 | binomial | differs |
| C2 | `binom(a)` subtracted instead of the centre `binom(a,k)` | 156, 2 | binomial | same |
| C3 | the `k` limit decided after the domain check (still returned) | **0: EQUIVALENT** | pass | same |
| C4 | coarse modulus `D = gcd(N, c^M + 1)` | 1358, 2 | power | differs |
| C5 | finest: the outside-prime block dropped | 218, 2 | power | differs |
| C6 | finest: the CRT centre replaced by `c^e` modulo the product | 9, 2 | power | differs |
| C7 | finest: `canon` not applied to the returned modulus | 40, 1 | - | differs |
| C8 | cyclo: the inverse flag used the other way round | 162, 2 | cyclotomic | differs |
| C9 | cyclo: the exponent returned modulo `canon(n)`, not `n` | SIGABRT | cyclotomic | differs |
| C10 | `canon`: any even modulus is halved | 36, 1 | - | differs |
| C11 | cyclo: `n = 0` is not refused | SIGFPE | cyclotomic | differs |
| C12 | cyclo: the unit modulus `N` is not canonicalised | **0: EQUIVALENT** | pass | same |
| C13 | strict: `NOT_DETERMINED` only when `D = 1` | 602, 2 | power | differs |
| C14 | binomial recurrence off by one | 738, 2 | binomial | differs |
| C15 | `k = 0` with `H > 0` keeps the input radius | 144, 1 | binomial | differs |
| C16 | the finest `g` limit is one too tight | 1, 1 | - | differs |
| C17 | the integral-domain rule returns the opposite status | 31, 3 | binomial, power | differs |
| C18 | the unit coset is used without normalising it | 36, 1 | power | same |
| C19 | the exact base `-1`: odd and even exponents exchanged | 12, 1 | power | differs |
| C20 | binom: the `k` limit decided only for an in-domain input | 1, 1 | - | differs |
| C21 | profpow: the `g` limit decided before the exact-base cases | 0 | power | same |
| C22 | profpow: `LIMIT` replaces the domain status at `g > 256` | **0: SURVIVES (F1)** | pass | differs |

22 faults: 19 detected, 1 survivor (F1), 2 equivalent.
C3 is equivalent because `integral_status` writes nothing: the limit is still returned for every
input, only after a wasted gcd. C12 is equivalent by a short argument and by a scan: `canon(n)`
never has `v_2 = 1`, and the only factor `canon` removes from `N` is a single 2, so
`canon(n) | N` iff `canon(n) | canon(N)`; the scan over `1 <= N, n < 300` finds 0 differences.
C21 also survives the test program but is caught by the driver case `profpowfine [1] with 257`;
it is listed as detected because a test or a driver case fails.

### The fixtures and their generators

- `tests/ref/vectors/f-slice12/residue.jsonl` (from `proto/symbol_checks.py`): the **values** are
  independent (Euler's criterion plus a square count through 65537, the defining prime product for
  Jacobi, `(a*a-1)//8` arithmetic for the supplement, `int(abs(a) == 1)` at `b = 0`). The
  **status column** is computed with the same modulus rule as the code (`modulus()` in the
  generator against `required_modulus` in `symbol.c`), so the certificate is not checked by a
  second opinion there; fault S8 shows that a wrong `K` is nevertheless caught, because the two
  sides are written separately.
- `hilbert.jsonl` and `hilbert_finite.jsonl`: computed by the exhaustive primitive-triple search at
  `p^2` and 16, grouped by equal square residues. No Hilbert formula is imported. Checked against
  Definition 4 on all 608 rows, 0 disagreements; 114 of them have a valuation outside `{0,1}` at
  the named prime, which is why the reduction to the parity matters.
- `tests/ref/vectors/f-slice13/binomial.jsonl`: from `proto/catalogue2_checks.py`, by a period
  argument on the integer sequence `binom(a+Nt,k)` modulo `S = |h(1)-h(0)|`, not by the `k`-sample
  formula. The period claim is sound (`N T` is a multiple of `k! S`, so the numerator difference is
  divisible by `k! S` and the value difference by `S`; periodicity plus `G | S` gives the gcd over a
  period from the gcd over all integers). My independent check uses the `k`-sample formula, so the
  two routes are different.
- `power.jsonl`: from the same file, by enumerating unit lifts modulo `B = 8 N g (g+1)!` and all
  exponent classes modulo the unit period, then taking the gcd of the image differences. No
  prime-exponent formula. The cases have `N < 8` and `g <= 2`.
- `cyclotomic.jsonl`: enumeration of unit lifts at `lcm(N,n)` with `min` of the residues and a
  direct `pow(j,-1,n)` for the inverse. The status is `len(vals) == 1`, which is the definition,
  not the `canon` rule.
- `volume.jsonl`: the quotient `d/H` (or 0), computed as a `Fraction`.
- The eight driver goldens are hand-written one-line expectations; their `.out` files are the
  reference. Two of the eight are worth naming: `profpowfine [7 mod 10] with 2 mod 4` is the case
  where the normalisation of `[7 mod 10]` to `[2 mod 5]` changes the strict status, and
  `cyclo_exp_u <1 ; [2 mod 3]> with 6` is the odd lift.

## Commands run (repository root, every program under `timeout`)

| Command | Result |
|---|---|
| `timeout 600 make -j2 BUILD=lanes/f-review14/build lanes/f-review14/build/libadelefeld.a` | exit 0 |
| `timeout 600 make -j2 BUILD=lanes/f-review14/build lanes/f-review14/build/test_symbol
  lanes/f-review14/build/test_catalogue` | exit 0 |
| `timeout 180 lanes/f-review14/build/test_symbol` | 11 tests, 719322 checks, 0 failed |
| `timeout 180 lanes/f-review14/build/test_catalogue` | 9 tests, 131636 checks, 0 failed |
| `timeout 300 python3 -B lanes/f-review14/checks.py` | 608 fixture rows, 0 disagreements |
| `timeout 175 python3 -B lanes/f-review14/checks2.py bino` | 1164 cases x 2 and 13 domain triples, 0 mismatches |
| `timeout 175 python3 -B lanes/f-review14/checks2.py pow` | 2190 cases x 3, 0 mismatches, 165 s |
| `timeout 175 python3 -B lanes/f-review14/checks2.py cyclo` | 8234 cases x 2, 0 mismatches |
| `timeout 175 python3 -B lanes/f-review14/checks2.py sym` | 8660 cases, 0 mismatches |
| `checks2.py` sections `sec_prop2`, `sec_mod8` | 5680 certified triples constant mod K,
  438 uncertified but determined; 32 solutions mod 8, 0 mod 16 |
| `timeout 175 python3 -B lanes/f-review14/mutate.py prep` | 6 driver cases pass, 55 probe lines |
| `timeout 178 python3 -B lanes/f-review14/mutate.py <IDs>` (6 batches) | the two tables above |

## Not done

- The library was not run under the invariants build, sanitizers or valgrind; the entry checks were
  read, not exercised.
- No fault was planted in `src/fball.c`, `src/ucoset.c` or `src/idpow.c`, although the catalogue
  leans on `adf_ucoset_pow_tight`, `adf_ucoset_normalise` and `adf_fball_set_fmpz3`. A fault in the
  tight power of `U(1)` at `g` would change every finest modulus; the slice's own tests would see
  it as a change of expected values, so this remains open.
- The driver cases were not run under the mutants for the archimedean complex paths or for Julia.
- No differential run against another implementation of the profinite power.

## Sources pending

None new. The attribution of the words "arithmetic" and "geometric" to a reciprocity source is
still pending in `docs/proofs/catalogue.md:342-343`, as `Y14` says, and the local-unit-group and
principal-unit theorems quoted in Proposition 13 remain cited only through `Y15`, which proves the
needed valuation and exponent claims directly.