# Progress

Read the lane rules, catalogue definitions and proofs, conventions, FLINT symbol domains,
existing gfunc interface, driver, Julia, invariant macros and review findings.
Header first: proposed N-D18 with alternatives in include/adelefeld/symbol.h.

Finding: the brief says modulus divisibility is exact. Catalogue Proposition 2:33 calls these
"sufficient moduli, not claims of minimality"; Proposition 3:55-57 says a coarser input can be constant.
The implemented Slice A certification policy follows the brief, with conservative refusals documented.
A two-point ambiguity witness cannot exist for every rejected ball. Example: (1+3 Zhat / 9)=1
on its entire input set, although 9 does not divide 3.

Slice A finished before Slice B starts.
Nine public functions; 3724 fixtures (2628 exact, 1096 finite), 390 Euler/counting comparisons.
4 C tests, 488759 checks; INV sanitizer baseline 5 tests, 488777 checks; 0 failures.
Driver: 15 hand lines match, expected exit 1. Julia: 23 assertions pass.
Six planted faults compiled and failed assertions (faults-A.log).
Mutation A: 30 sampled, 25 compiled, 20 killed, 5 survived, 5 not compiled, 0 timeouts (35.9 s).
Three survivors were test gaps, now killed by new rows and an invalid-lower debug entry test
(survivors-A.log). Two are equivalent: kinds 0 and 1 use identical Jacobi/modulus paths after
lower-entry validation; changed selector 1->0 or 0->1 cannot change an admitted result.
The mutation harness copies a bounded staging tree and runs only test_symbol with SAN=1 INV=1.
No generic mutation rerun: 30 more samples are reserved for Slice B, respecting 60 total for this file.

Setup failures: GCC 13 reported a false array-parameter overread in the existing driver negation accessor.
Replacing that accessor with fmpq_set of the same canonical field (already read directly for inf and u)
restored the strict build. A noinline attempt did not fix it and was removed.
The first mixed-domain gap row (1,1,2) canonicalised to (0,1,2) and did not kill the gcd fault.
The corrected (1,3,2) row does, and no failed attempt is counted as a killed fault.

Slice B implementation and user calls finished.
Four public Hilbert functions. Exact rational, exact local, sufficient-precision local and exact-idele
results match every one of 608 primitive-solution fixtures. Another fixture has 1920 finite idele
pairs and 800 local pairs, including the missing-digit and constant-coarse cases.
The product formula checks 2000 rational pairs at 12058 finite factors and the real place.
C before the final alias additions: 10 tests, 719310 checks, 0 failures.
Driver: 16 hand lines, expected exit 1, diff exit 0. Julia: 20 Hilbert and 23 residue assertions.
Six planted Hilbert faults compiled and failed assertions (faults-B.log).
Y5-Y8 document formulas, zero statuses, square-class enumeration, cofactor, local limits,
and the stepwise primitive-congruence proof. Thorne Lemmas 3.7 and 4.12 are on disk and
close the catalogue proof's pending Hensel sources for the oracle's applications.

Bristol lecture19.txt's displayed formula is transposed at the odd prime and has extra factors
at 2. The implementation follows catalogue.md's proved formulas and the independent search;
these discrepancies are documented in Y8. The on-disk product theorem is cited separately.

Mutation B: 30 samples, 30 compiled, 22 killed, 8 survived, 0 timeouts (91.4 s).
One survivor exposed a missing endpoint test: v=WORD_MAX-3, N=WORD_MAX has exactly three
unit digits. Added compact centre 1 and centre 5 checks; the mutated strict inequality now
fails (survivors-B.log). Seven other survivors are elementary equivalences, listed in the report.
Across both runs: 60 samples, 55 compiled. No whole-source sweep or generic rerun was used.

Final checks are complete. Broad run: timeout 900 make -j2 check-all, exit 2 at memcheck selftest.
Earlier steps passed: 78 C programs, 57 driver cases/101050 lines, 455/455 exports, Julia,
and the mutation-tool selftest. Memcheck found four cleanup omissions on debug-test child
unexpected-return paths. Added clears and reran only the failed selftest: exit 0.
The final INV=1 build first failed on ignored freopen return (-Werror), so its attempted test
exited 127 without executing. Checking freopen's return fixed the build; INV passed 13 tests,
719376 checks, 0 failures. The final memcheck selftest also passed after that fix.
SAN=1 with ASAN_OPTIONS=detect_leaks=0: 11 tests, 719322 checks, 0 failures.
CC=clang: 11 tests, 719322 checks, 0 failures.
Final ordinary symbol test: 11 tests, 719322 checks, 0 failures.
LeakSanitizer is disabled because this sandbox cannot run it, as the brief states.
The full check-all was run exactly once; its failed exit is retained, not relabelled as a pass.
The three new driver precedence rows were added after the full run's driver phase; targeted final
runs match all 15 residue lines and all 19 Hilbert lines, with expected exit 1 for each fixture.
Standalone symbol header: C11 and C++17 syntax checks exit 0.
Delivered new text/source files have a final newline and 0 lines above 116 columns.
The three fixture files total 679803 bytes.

PARI usersch3.tex:9266-9271 supplies the special Kronecker conventions, closing Definition 1's
pending convention bibliography. Its unqualified "total multiplicativity" at :9263 has the
same b=0 counterexample as the brief. Catalogue Proposition 8's reciprocity-law citation remains
pending; the product theorem itself is cited directly from the on-disk Bristol lecture.
