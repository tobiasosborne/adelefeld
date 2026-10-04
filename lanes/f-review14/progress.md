# f-review14 progress (running notes)

Setup: archive and both test programs built with
`timeout 600 make -j2 BUILD=lanes/f-review14/build lanes/f-review14/build/libadelefeld.a` and
`make ... test_symbol test_catalogue` (exit 0). Baseline: test_symbol 11 tests, 719322 checks,
0 failed; test_catalogue 9 tests, 131636 checks, 0 failed.

Read: docs/api-1f9.md; docs/proofs/catalogue.md Definitions 1/4/11, Propositions 2-8 and 10-15;
docs/conventions.md 3.1-3.3, 4.3, 4.4, 5.2, 5.6, 5.8, 6.6; SPEC 9.3.7 and 15.4 (N-D18, N-D19);
include/adelefeld/symbol.h, catalogue.h; src/symbol.c, src/catalogue.c; tests/test_symbol.c,
tests/test_catalogue.c; the eight driver cases; docs/reviews/f1/review-gfunc-log-referee.md (form);
lanes/f-slice12/faults.py and lanes/f-slice13/faults.py (their own faults, not repeated).

Sources opened: refs/src/pari-doc/usersch3.tex:9255-9280, refs/src/flint-3.0.1/fmpz.rst:1140-1180,
refs/src/hilbert-bristol/lecture19.txt:17-26,44-52,78-92, refs/src/milne-cft/CFT.txt:1300-1310,
3155-3168, 9880-9910, refs/src/flint-3.0.1/ulong_extras.rst (n_mulmod2, n_invmod, n_factor,
n_is_prime), refs/src/thorne-padic/jackthornenotes.txt not needed (the claim is checked directly).

## Statement notes, in order

- Y1. Definition 1 agrees with usersch3.tex:9266-9271 line for line; "total multiplicativity" at :9263.
  FLINT domains fmpz.rst:1166-1168 (Jacobi: odd positive n) and :1170-1172 (Kronecker: any n) confirmed.
- Y2. Sufficiency of K checked by enumeration: 5680 triples (a,N,b) with K|N, all residues of the
  ball mod K give one symbol. Necessity refuted by 438 uncertified balls that are nonetheless
  determined; the code refuses those on purpose. Normalisation argument checked (K is odd or 8|N,
  so K|2m iff K|m for m odd); the code uses the stored N and that is sound.
- Y3. gcd(H,d)|A argument re-derived; canonicality gives gcd(A,H,d)=1, so d>1 leaves a point
  outside Zhat. My first domain expectations were wrong because the constructor canonicalises;
  after canonicalising, 13 triples agree.
- Y4. Multiplicativity in the denominator is used nowhere in src/; only in the test, where b=0 is
  excluded. The counterexample (-1/0)=1 vs (-1/0)(-1/3)=-1 is in the test.
- Y5. Rational class reduction: parity = xor of the two nonnegative parities; the unit is the
  ratio of the stripped integers; checked against Definition 4 on 608 fixture rows.
- Y6. The lifting argument is complete for all four odd valuation parities and at 2; the mod-8
  claim checked: 32 primitive solutions mod 8, none mod 16. The oracle's reduction to the
  valuation parity is load-bearing: without it 17 of the 608 rows get the wrong sign.
- Y7. Enumeration of admitted unit classes verified; precision k = min(3, N-v); r = s = 3 gives
  -1 at 2. Overflow guards read.
- Y8. Product formula used only in a test; no source file multiplies over places. The Bristol
  display at :47-51 is transposed; I confirmed independently that (3,2)_3 = -1 and that the
  Bristol form gives +1, so the catalogue formula is the right one.
- Y9. Smallest radius over j=1..k equals that over j=1..k+5 on 1164 cases; the Newton/finite
  difference argument re-derived in both directions.
- Y10. Same congruence argument as Y3; no NOT_DETERMINED boundary among wholly integral inputs.
- Y11. Strict criterion and D = gcd(N,c^M-1) over 2190 cases; the exact-unit branches and the
  [-1] parity cases read and matched against the driver.
- Y12. Inside block, outside block, CRT centre and canon(F) against a brute-force computation
  prime by prime on 2190 cases, 0 mismatches. Both gcds terminate (strictly decreasing).
- Y13. haar_volume returns d/H, 0 for a point, for both backends; index proof re-read.
- Y14. Cited lines of CFT.txt confirmed; canon(n)|canon(N) necessary and sufficient (proof of the
  converse re-derived); the odd lift and n=1 checked over 8234 cases.
- Y15. All six steps re-derived; the table of Proposition 13 matches line by line.