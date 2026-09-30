# Lane n-review1: review of what has had no second reader, and two closure checks

Your task is to REFUTE, not to confirm. Four parts, in this order; write the report as you go through
them so that an interruption loses little (the report is written once, but its parts in order).

Part A, closure of `docs/reviews/f1/review-sball-rfunc.md` R3 to R8, repaired by lane f-repair2
(`lanes/f-repair2/result.md`): judge each CLOSED or OPEN with a reproducer (the reviewer's programs in
`lanes/f-review2/`). R1 and R2 were accepted as `LIMIT` by decision N-D7 (`docs/SPEC.md` 15.4): judge
whether the reworded rule of `include/adelefeld/lball.h` ("Limits") is TRUE as it stands now.

Part B, closure of `docs/reviews/m2/review-slices-2-3.md` F1 to F5, repaired by lane i-repair1
(`lanes/i-repair1/result.md`; programs in `lanes/i-review2/`): CLOSED or OPEN each.

Part C, review of lane f-slice3 (Claude Sonnet; `lanes/f-slice3/result.md`): `adf_lball_decompose_teich`,
`adf_lball_teichmuller`, `adf_lball_frac`, `adf_lball_unit_mod`, `adf_lball_pow_si` in
`include/adelefeld/lball.h`, `src/lball_decomp.c`, `src/lball.c`; the early `LIMIT` of
`adf_lball_set_fball` (L13); statements L9 to L13 in `docs/api-1f.md` (section "Slice 1F.3-b").
Contract: `docs/proofs/functions.md` Proposition 4 and 19, Lemma 3. Your own oracle: the Teichmueller
representative as the limit of `a^(p^n)` by your own powering; the decomposition of every point of a
ball by brute force; `frac` by exact rationals; powers by repeated multiplication and enumeration for
tightness (the `e` term at `p = 2`: the squares of `1 + 2 Z_2` are `1 + 8 Z_2`). The prime `2^64 - 59`.
Statuses, `NOT_DETERMINED` at `p = 2` with relative precision 1, the exact 0, limits, aliasing.

Part D, review of lane f-slice6 (Claude Sonnet; `lanes/f-slice6/result.md`; `src/rfunc.c` at a prime,
the driver commands `project`, `exp_at`, `log_at` in `tools/adf/adf.c`) and of lane t-slice1 (Claude
Sonnet; `lanes/t-slice1/result.md`; the readers and printers of unit cosets, ideles and classes in
`src/text.c` and `src/text_idele.c`, `include/adelefeld/text.h`; the driver commands `inv`, `pow`,
`powtight`, `norm`, `class`, `idele`, `hull`, `hullsimple`, `unitof`, `valuation`, `abs`; Statements P and Q
of `docs/api-2.md` section 4). Contract: `docs/conventions.md` 8, 9 (9.5 constrained printing, 9.6
round trips, 9.7 classification), the golden files `tests/golden/ucoset.tsv`, `idele.tsv`, `idclass.tsv`,
`proto/text_grammar.py`, M1-D1, M1-D6, M1-D7, N-D1, N-D10. What counts: a text read to a value that does
not contain the set the text denotes, or to a wrong status; a printed text that a round trip does not
enclose; the constrained printer hiding a sign or printing a ball that does not contain the value; a
crash or a hang on hostile text (the author reports the constrained printer at 24 s for a radius of
40000 bits: find the worst valid input under the bounds of M1-D6 and say whether it is acceptable);
the driver: a command whose printed line claims more than the library proved; the driver text of a
local ball (N-D10) against the value form of `adf_lball` in conventions 9 (are they the same grammar? if
not, is that acceptable or a finding). Read the author's decisions t-1 to t-12 and judge t-2 (the real
ball of the readers: the enclosing ball of 9.5 first, else kernel B on rounded end points).

**You own:** `lanes/n-review1/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Every
program under `timeout`, none longer than 3 minutes. Build with `make -j2 BUILD=lanes/n-review1/build`.

Report: `lanes/n-review1/report.md`: part A and B as tables CLOSED/OPEN with the reproducer; parts C and D
with findings (severity BLOCKER: a wrong value, a wrong text, a crash; MAJOR; MINOR), input, what the
code returns, what is true and why, the command; then what you attacked without result, with counts and
what would have made a case fail. No praise, no summary of the code.
