# Lane f-review14: referee of the proofs Y1 to Y15 (symbols and catalogue), and the tests that cannot fail

Your task is to REFUTE, not to confirm. The symbols (lane f-slice12) and the second catalogue group (lane
f-slice13) of WP 1F.9 were proved, coded and tested by one model family (codex `gpt-6.1-sol`); you are of
another family on purpose. Two bug hunts compared VALUES and statuses with their own oracles: f-review11 on
`src/symbol.c` (`docs/reviews/f1/review-symbol.md`: no finding) and f-review12 on `src/catalogue.c`
(`lanes/f-review12/result.md` if it has landed: read it, do NOT repeat its attacks). Nobody has read the
proofs as a referee, and nobody has planted faults against the tests. That is your task.

Under review: `docs/api-1f9.md` (Y1 to Y15, 400 lines); `include/adelefeld/symbol.h`, `src/symbol.c` (351
lines); `include/adelefeld/catalogue.h`, `src/catalogue.c` (270 lines); `tests/test_symbol.c` (684 lines),
`tests/test_catalogue.c` (482 lines), `proto/symbol_checks.py`, `proto/catalogue_checks.py`,
`tests/driver/symbol-*`, `tests/driver/catalogue-*`. Contract: `docs/SPEC.md` 9.3.7 and 15.4 (N-D18, N-D19);
`docs/proofs/catalogue.md` Definition 1, Propositions 2 to 8 and 10 to 15; `docs/conventions.md` 3.1 to 3.3,
4.3, 6.6. `refs/src/` is on disk (`hilbert-bristol/lecture19.txt`, `pari-doc/usersch3.tex`, `milne-cft/CFT.txt`,
FLINT 3.0.1).

**You own:** `lanes/f-review14/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive ONCE with `timeout 600 make -j2 BUILD=lanes/f-review14/build
lanes/f-review14/build/libadelefeld.a`; single test programs into the same directory (`make -j2
BUILD=lanes/f-review14/build lanes/f-review14/build/test_catalogue`). Do not run the repository's suites as a
whole. Every program under `timeout`; none over 180 s.

## Part 1: referee Y1 to Y15 as proofs

Read each statement and its proof line by line, as a referee who is paid for each false step. For each: the
claim in your own words; every step checked; for each step that is not immediate your own derivation, or a
counterexample computed in exact integers by a program in your directory. Points to press, among others:
- Y1 to Y4: the conventions of the Kronecker symbol for lower entries 0, negative, even against the quoted
  source (open the file and line); "sufficient, not always necessary" for the modulus; Y3's rule for
  nonintegral balls; Y4: where exactly multiplicativity in the denominator is used and where it fails.
- Y5 to Y7: the square-class formula at 2 and at odd `p` against the source; Y6: why solvability modulo
  `p^k` decides the local question (the `k`, Hensel, primitive solutions): is the proof complete for
  valuations of both parities and at 2? Y7: finite precision (`A - v(a) >= 3` at 2), a ball containing 0,
  the cofactor of the scale of an idele (`r = s = 3`, unit 1: `(3,3)_2 = -1`).
- Y8: is the product formula used as a proof step anywhere, or only as a test?
- Y9, Y10: the smallest radius as the gcd over `j = 1..k`: why `k` values suffice (the proof by finite
  differences or whatever is written: check it; test `k` values against `k + 5` by program); negative tops;
  the domain rule `gcd(H, d) | A`.
- Y11, Y12, Y15: strict "exactly when `c^M = 1 mod N`" at `canon(N)`; the coarse modulus; the finest modulus
  `canon(F)` without factoring `N`: the inside block `gcd(N g_N, c^M - 1)`, the outside block from the tight
  power of `U(1)` at `g`, the separation by repeated gcds (termination, correctness when `g` and `N` share
  primes, at 2, when `c = -1`, when `c^M = 1`): prove or refute each, and compare the table of Y15 with
  Proposition 13 line by line. The CRT centre.
- Y13: reuse of `adf_fball_haar_volume` and `adf_idele_content`: does the statement match what those
  functions do for every canonical input?
- Y14: which exponent is which against `milne-cft/CFT.txt:9883-9886`, `:9904-9908`, `:1307` (open them); the
  condition `canon(n) | canon(N)`: necessary and sufficient? the odd lift for `n = 2 m`.
A step that is true but not proved as written is a MINOR with the missing argument supplied; a false step
with a counterexample input to the LIBRARY is a BLOCKER or MAJOR by its consequence; a false step the code
does not rely on is a MINOR.

## Part 2: tests that cannot fail

Plant faults in scratch copies of `src/symbol.c` and `src/catalogue.c` under your build directory: your OWN
faults, at least 10 in each file, different from the lanes' own (`lanes/f-slice12/report.md`,
`lanes/f-slice13/faults.py`: read which those are). Among them, for the symbols: `(a/2)` with the residues 3
and 5 exchanged; the sign for a negative lower entry dropped; the precision rule at 2 with 2 in place of 3;
the cofactor `r'` dropped at an odd prime only; the real place returning `+1` always; `where` written on
failure. For the catalogue: the radius `N / gcd(N, k!)` with `(k-1)!`; the tight gcd over `j = 1..k-1`;
`LIMIT` after the domain check; `D = gcd(N, c^M + 1)`; the outside block of the finest modulus dropped; the
CRT centre replaced by `c^e`; `canon` omitted on the returned modulus; `_exp_u` and `_exp_uinv` exchanged;
the exponent reduced modulo `canon(n)` instead of `n`. Build the two test programs against each fault, run
them, and record which fault each detects. A fault that every test passes is a finding (MAJOR if the fault
gives a wrong value, enclosure or status, MINOR otherwise) with the smallest input that distinguishes it.
Read the fixtures' generators too: which stored rows are computed by the same formula as the code?

## Report

`lanes/f-review14/result.md`, written once, at the end (the harness refuses the name `report.md` for a Claude
subagent); running notes in `lanes/f-review14/progress.md` as you go (a note after every statement refereed).
For each finding: severity, the proof step or the input, what the text or the code says, what is true and
why, the command that reproduces it. Then, statement by statement, what you checked and how (counts, what
would have made a case fail). Then the two fault tables. No praise, no summary. Give the same text as your
final message. Delete your build tree and executables at the end.
