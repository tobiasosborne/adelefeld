# Lane f-review4: referee of F8 and F9 and review of the fast series at a prime

Your task is to REFUTE, not to confirm. Lane f-slice5 (codex gpt-6.1-sol; report `lanes/f-slice5/result.md`)
made `adf_lball_log` and `adf_lball_Log` fast (74 s to 0.04 s at `p = 2^64 - 59`, `N = 10000`) by two
new statements with proofs of its own at the end of `docs/api-1f4.md`: F8 (the working precision that
the term of degree `k` needs, and unit division by a word denominator) and F9 (a factorisation of a
principal unit into principal units of increasing valuation, and an exact binary-splitting tree for
their shorter log sums). The code: `src/lfunc.c` (the routes are chosen by the size of the working
modulus; the old loop is kept for word-size moduli). Contract: `include/adelefeld/lfunc.h`;
`docs/proofs/functions.md` Propositions 7, 7b, 8, Lemma 9, Propositions 10, 11; the review of the slow
code, `docs/reviews/f1/review-lfunc.md` (no wrong enclosure; its oracle is in `lanes/f-review3/`).

The claim to break: every result of the new routes is the SAME ball as the old routes gave (the lane
stored 2000 results of the old code in `tests/ref/vectors/f-slice5/` and compares), and F8 and F9 are true
with the proofs as written. A wrong step of F9 would be a wrong enclosure of `log` for every large input.

**You own:** `lanes/f-review4/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Every
program under `timeout`, none longer than 3 minutes. Build with `make -j2 BUILD=lanes/f-review4/build`.

1. Referee F8 and F9 as a mathematician: each step; the valuations of the factors; the claim that the
   tree loses no working digit through denominators; the residual inverses "need `K - m` digits after
   removing `m` digits"; the boundary of the ranges (`k` a power of `p`, `v = 1` at `p = 2`, the case
   `3 + 4 Z_2`, `K` just above a power of `p`); where F9 rests on F8 and on Proposition 8. Write the
   argument out, or the counterexample.
2. Read `src/lfunc.c` against F8 and F9: is each digit count of the code the one of the proof (not one
   less); which route is chosen for which modulus, and is the boundary handled by both routes with the
   same result; the word division of F8 (`j` such that `r + j Q` is divisible: the range of `j`, the
   overflow of `r + j Q` in a word).
3. Your own oracle (exact rational arithmetic with your own tail bound, or the oracle of
   `lanes/f-review3/` after you have read it): the old and the new route on the SAME inputs where the
   routes differ (moduli above the word range): `p` = 2, 3, 5, 7, 11, 65537, `2^64 - 59`; `N` from 60 to
   400 exhaustively at `p` = 2 and 3 for a few centres, and `N` = 1000, 5000, 20000; balls and exact
   inputs; `Log` with negative valuation; the point of the ball that differs from the centre in the
   last admitted digit. A difference between the old and the new result on ANY input is a finding
   (the old route: the library at commit `1cf0e42` on master, which you may build into your lane
   directory from `git show 1cf0e42:src/lfunc.c`; this is a read of git, not a change of state).
4. Cost: an input where the new code is slower than the old by more than 10 percent at small `N`
   (the lane reports at most 3 percent); an input above the word range where the time is not
   near-linear in `N` (the lane's table); memory of the splitting tree at `N` = 100000.
5. Faults: plant three in a scratch copy (a digit count one too small in F8; a factor of F9 with the
   wrong valuation; the tree combining two nodes with the wrong denominator) and show that the
   stored comparison of `tests/test_lfunc.c` catches each; if one survives, that is a finding.

Report: `lanes/f-review4/result.md`, written once, at the end, and the same text as your final message.
Findings with severity (BLOCKER: a wrong enclosure or a false step of F8 or F9 that the code relies on;
MAJOR; MINOR), input, what the code returns, what is true, the command. Then what you attacked without
result, with counts. No praise, no summary of the code.
