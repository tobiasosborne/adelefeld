# Lane m4-review1: adversarial review of milestone 4, functions on the adeles

Your task is to REFUTE, not to confirm. Milestone 4 (`docs/PLAN.md` lines 325-333) landed on 2026-10-08 in seven
slices without review (TJO: one review when a major feature lands). The code under review, with the lane that
wrote it and its report (read each report: what it tested, what it could not, its findings):

| Slice | Code | Lane, model, report |
|---|---|---|
| 4a | `include/adelefeld/ffun.h`, `src/ffun.c`: the type, `set_acb_vec`, `refine`, `add`, `fourier`; the `ffun` text in `src/text.c`; driver `ffun`, `ffun_add`, `ffun_fourier` | f4-slice1, codex, `lanes/f4-slice1/report.md` |
| 4b | `src/ffun.c`: `mul`, `translate_rat`, `reflect`, `dilate_rat`, `dilate_idele`, `conj`; driver | f4-slice4, codex, `lanes/f4-slice4/report.md` |
| 4d | `include/adelefeld/rfun.h`, `src/rfun.c`: the type, `set_terms`, `add`, `mul`, `translate_rat`, `dilate_rat`, `dilate_idele`, `reflect`, `conj`, `eval`; the `rfun` text; driver | f4-slice2, Opus, `lanes/f4-slice2/result.md` |
| 4e | `src/rfun.c`: `fourier`, `derivative`, `integral`, `norm2`; driver | f4-slice3, Opus, `lanes/f4-slice3/result.md` |
| 4f | `include/adelefeld/tensor.h`, `src/tensor.c`: `ffun_eval`, `tensor_eval`, `tensor_eval_sball`, `ffun_integral`, `ffun_norm2`, `tensor_integral`, `tensor_norm2`; driver | f4-slice5, Opus, `lanes/f4-slice5/result.md` |
| 4g | `src/poisson.c`: `adf_tensor_poisson`; driver `poisson` | f4-slice6, Opus, `lanes/f4-slice6/result.md` |
| 4c | `src/dump.c`: the eight dump functions of `ffun` and `rfun`; driver `dump`, `load` | f4-slice7, codex, `lanes/f4-slice7/report.md` |

Contracts: `docs/api-4.md` (sections 1 to 7; statements F1 to F4, R1, R2, E1, P1; the decisions D1 to D3 taken
as `docs/SPEC.md` 15.4 N-D23, with the stall rule of P1 step 4 added by lane f4-slice6), the statements the
lanes wrote in `docs/api-4a.md`, `4b.md`, `4c.md`, `docs/SPEC.md` 7 (443-468), `docs/conventions.md` 5.11, 5.12,
6.1 to 6.3 (the kernel signs: the finite transform `E(-jk/L)`, the real transform with `E(+xy)`: lane f4-slice3
found that a brief had the sign of the translation covariance wrong; derive both signs yourself from
conventions 6.1 and `refs/src/tate-poonen/notes.txt:693-700, 733-740` before reading the lanes' text), 9.2,
10.1, 11.3, `docs/proofs/analysis.md` Propositions 4, 5, 7, Lemma 6, Lemma 14, Proposition 15. The design's own
oracle is `proto/functions4_checks.py`; the lanes used it: do NOT use it or the lanes' vectors for a verdict.
Build your own exact models (Python `fractions`, exact cyclotomic arithmetic for the finite transform through
`sympy` or your own roots-of-unity bookkeeping; `mpmath` at 60 digits for the real transform, integrals and
Poisson with your own error margins; `python-flint` if importable).

Hunt, in this order; stop a line of attack after some thousands of cases without a finding:
1. **The finite transform and algebra, exactly.** 5000 random `adf_ffun` with `(D, M)` up to `(12, 12)` and
   exact Gaussian-rational values: `fourier` against your exact cyclotomic transform (every coefficient
   contains the exact value; a radius bound you state; `L = 1024` as the cap); `fourier(fourier(f))` the
   reflection with `1/D`; Parseval; `refine`, `add`, `mul`, `translate_rat` (a pure permutation when the
   denominator divides `D`), `reflect`, `dilate_rat` (support holes; negative `q`), `dilate_idele` (the unit
   image modulo `D M` a singleton or `NOT_DETERMINED`; the content), `conj`, each cell against your model;
   the covariance `hat(D_a f)(y) = |a|^-1 hat f(y/a)`; ball values: enclosure and F1's correlation loss.
2. **The real functions.** 3000 random `adf_rfun` (degrees to 6, several terms, complex `A` with `Re(A) > 0`,
   parameters with radii): `translate_rat`, `dilate_rat`, `mul`, `reflect`, `conj`, `derivative` against your
   exact closure algebra; `eval` at 100 points per function (containment, and tightness on exact inputs);
   `fourier` against `mpmath.quad` of `phi(x) E(+xy)` at 60 digits (your sign from the source) at 50 `y` per
   function, and `F(F(phi))` the reflection; `integral` against `quad`; `norm2` against `quad` of `|phi|^2`;
   the shifted Gaussian `q = 1/3` at `y = 1/4`: the sign of the imaginary part; the root branch for `A` in
   every quadrant of the right half-plane (R2); `Re(A) > 0` lost under rounding at `prec 2`: `NOT_DETERMINED`
   with outputs untouched (sentinel bytes).
3. **Evaluation (E1).** 5000 finite balls `(A + H Zhat)/d` against your own enumeration of the cosets met
   and the support test: every selected value and zero (outside support) inside, a value not met excluded
   when outside the hull; point balls exact; local-backend balls (a context with blocks 8, 9, 5); `tensor_eval`
   as the product; `tensor_eval_sball` on places `{2, 3}`, `{5}`, `{}` with arch REAL and NONE against your
   own E1 step 4 and the step 5 bound checked on 1000 sampled points; arch COMPLEX `DOMAIN`.
4. **Poisson (P1).** 300 tensors (`(D, M)` to `(6, 6)`, `phi` as in 2 with `Re(A)` down to `1/8`): `left` and
   `right` each contain your 60-digit value of the respective sum (summed to convergence in `mpmath`), each
   coordinate diameter at most `2^-bits` on `OK` for `bits` 20, 53, 80, 128; the tails: your own Lemma 6
   bound at the returned `NL`, `NR` is at most `epsilon/8` (and at `NL - 1`... the search goes `0, 1, 2, 4`:
   check that the returned cutoff is one of those and the previous one fails your bound or the function's);
   the theta value `1.0864348112133080145...`; the witness `c` in `[1, 2]`: `NOT_DETERMINED`, outputs untouched;
   `bits` outside `[0, 2^21]`: `DOMAIN` after the `prec` cap; a tiny `Re(A)`: `LIMIT` within 10 s; the two
   sides independent (plant a fault in a scratch copy that derives `right` from `left`: the lanes' tests must
   catch it; hunt 7).
5. **Text and dumps.** 3000 `ffun` and `rfun` texts from your own printer (conventions 9.2 and 11.3):
   `set_str` then `get_str` encloses; the stage order on two-rule violations; dumps: `dump` then `load`
   identical, `load` then `dump` byte-identical, 20000 mutated dump bytes refused without a FLINT call on the
   bytes (interpose `arb_load_str`), under `SAN=1`; the caps (`D M = 2^20 + 1`).
6. **Memory.** Your harnesses against `build-san` with `ASAN_OPTIONS=detect_leaks=1` on 3000 cases of each
   hunt (the codex lanes could not run LeakSanitizer: this is the first leak run of `src/ffun.c` and of the
   dump functions); then the lanes' test programs under the leak detector.
7. **Faults.** Twelve of your own across the seven slices (the sign of either kernel; `1/D` for `1/M`; the
   reflection dropped; `lcm` for `gcd` in E1; the right side derived from the left; the tail without the
   factor 2; the width checked before the tail; `Re(A)` not re-checked; the loader normalising; the
   translation permuting the wrong way; the content inverted in `dilate_idele`; the hull sampled at one
   point), planted in scratch copies; build the lanes' tests against each and record which are detected.
8. **The statements.** `docs/api-4a.md`, `4b.md`, `4c.md`: each "Check:" line, is it checked by a test? One
   sentence per statement that is not. The two HEADER-FINDINGs of lane f4-slice7 (the unconditional
   dump/load identity needs the caps; load's cost includes clearing the old storage): are the header
   comments now right?

**You own:** `lanes/m4-review1/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/m4-review1/build
lanes/m4-review1/build/libadelefeld.a`, and once with `SAN=1 INV=1` into `lanes/m4-review1/build-san`; link
your programs against them (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's suites as a whole.
Every program under `timeout`, none over 170 s. Leave no files in `/tmp` and none outside your lane directory.

Severity BLOCKER: a true value outside a returned enclosure (a transform coefficient, an evaluation, an
integral, a Poisson side); a wrong sign of a kernel; a non-canonical result; a memory error, leak or undefined
behaviour; a FLINT call on unvalidated bytes; an output written on a failure status; `OK` from `poisson` with
a coordinate diameter above `2^-bits`. MAJOR: a radius beyond the stated bound; a wrong status (`LIMIT` within
the caps, `OK` beyond them; `NOT_DETERMINED` where the design says `OK`); a hang where `LIMIT` is promised; a
fault the lanes' tests pass when it loses enclosure or a sign. MINOR: everything else true.

Report: `lanes/m4-review1/result.md` (a Claude subagent cannot write `report.md`), written once, at the end,
and the same text as your final message; notes in `lanes/m4-review1/progress.md` as you go. For each finding:
severity, the exact input, what the code returns, what is true and the sentence (file and line) that says so,
the command that reproduces it. Then what you attacked without result, with counts and what would have made a
case fail; the fault table. No praise, no summary. Keep generators, not their outputs; delete your build trees
and executables at the end. Aim to finish within about ninety minutes: if the time is short, the order of
importance is hunts 1, 2 (the signs), 4, then 3, 5, 6, then the rest. If a long run of yours is still going,
wait for it with a single blocking command, not many short polls.
