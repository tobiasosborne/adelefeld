# Lane m5-review1: adversarial review of milestone 5, the Tate integrals

Your task is to REFUTE, not to confirm. Milestone 5 (`docs/PLAN.md` lines 335-343, `docs/SPEC.md` 8) landed in
five slices without review (TJO: one review when a major feature lands). The code under review, with the lane
that wrote it and its report (read each report: what it tested, what it could not, its findings, its mutation
survivors):

| Slice | Code | Lane, model, report |
|---|---|---|
| 5a | `include/adelefeld/localfactor.h`, `src/localfactor.c`: `adf_local_tate_at`; driver `tate_local` | t5-slice1, codex, `lanes/t5-slice1/report.md` |
| 5b | `src/localfactor.c`: `adf_local_gamma_at`, `adf_local_epsilon_at`; driver `tate_gamma`, `tate_epsilon` | t5-slice3, Opus, `lanes/t5-slice3/result.md` |
| 5c | `include/adelefeld/tate.h`, `src/tate.c`: `adf_tate_vector`, `adf_tate_integral`; driver `tate_vector`, `tate_integral` | t5-slice2, Opus, `lanes/t5-slice2/result.md` |
| 5d | `src/tate.c`: `adf_tate_continue`, `adf_tate_completed`; driver `tate_continue`, `tate_completed` | t5-slice4, Opus, `lanes/t5-slice4/result.md` |
| 5e | `tools/adf/adf.c`: `tate_fe` (two guarded calls and the root number; no library code) | t5-slice5, Opus, `lanes/t5-slice5/result.md` |

You run as a codex lane (gpt-6.1-sol xhigh). Four of the five authors are Claude Opus, the fifth (5a) is codex,
and the design oracle is codex's. So: derive every sign and formula yourself from the proofs and the sources
BEFORE reading the lanes' texts, and build your own models. Contracts: `docs/api-5.md` (sections 1 to 9; decisions D1 to D4 taken as `docs/SPEC.md` 15.4 N-D24;
N-D23 on input radii), the statements the lanes wrote in `docs/api-5a.md` (slices 5a, 5b) and `docs/api-5b.md`
(5c: S1 to S8; 5d: S9 to S14; 5e: S15 to S20), `docs/SPEC.md` 8 (470-502), 9.3.7, `docs/conventions.md` 6.1
to 6.5 (signs: the additive character, the Gauss sum, `G_minus` with the NEGATIVE kernel, the root number
`W = tau/(i^e sqrt C)`, the test vector), status rows 223 and 225, `docs/proofs/analysis.md` Propositions 9
to 15 and Lemmas 6, 8, 14 (the local closed forms; the splitting at norm 1; the theta identity; the bounds),
`docs/design/local-zeta.md` (the Gamma certificate the real place inherits). Sources: `refs/src/tate-poonen/
notes.txt` (the local integrals and gamma factors: find the lines; 62-64 Gamma's poles; 693-700, 733-740 the
character), `refs/src/flint-3.0.1/acb_dirichlet.rst` (the `L`, `xi`, Hurwitz normalisations: for YOUR
reference values only), `acb.rst`. The design's own oracle is `proto/tate_checks.py` and the lanes' vectors are
under `tests/ref/vectors/t5-*/`: do NOT use either for a verdict. Your models: `mpmath` at 60 digits (your own
Hurwitz-based `L`, your own `Gamma`, your own direct numerical evaluation of the LOCAL integrals as integrals
over `Z_p` shells and over `R`, not the closed forms), exact cyclotomic arithmetic (`sympy`) for `G_minus` and
the Gauss sums, `python-flint` (`acb_dirichlet`, importable here, 0.9.0) as a second independent source.

Hunt, in this order; stop a line of attack after some thousands of cases without a finding:
1. **The local integrals and constants against the integral itself.** For `p` in 2, 3, 5, 7, 11, 13 and
   `eta_0` trivial, of conductor `p`, `p^2` (`p = 2`: 4, 8, 16, including non-real characters), `alpha` in a
   dozen values (exact, complex, with radii), `s` on a grid of 40 complex points away from the poles: your
   own evaluation of `Z(f, eta, s) = integral f(x) eta(x) |x|^s d*x` over the shells `p^k Z_p^x` as a finite
   sum of character sums times `p^-ks` (summed to convergence in `mpmath`), for the PRESCRIBED `f` of
   conventions 6.5; the same for `gamma` through the local functional equation `Z(F f, eta^-1 |.|, 1 - s) =
   gamma Z(f, eta, s)` with YOUR OWN transform of the prescribed vector (P3, P4) at a point where both sides
   converge, continued by the closed form you derive; `epsilon`. Every returned ball must contain your value; a
   tightness bound you state. The real place against `mpmath.quad` of `x^e exp(-pi x^2) sign(x)^e |x|^s dx/|x|`
   for `Re(s) > 0`, and the gamma factor through P10. Then the poles: the exact pole sets of each of the three
   functions at each place (derive them; the lanes say the integral's and gamma's lists differ), statuses at,
   on and beside them, outputs untouched, `where = v`.
2. **The global value and its continuation against your own `L`.** For `C = 1` and 30 primitive characters
   (the 17 goldens and 13 of your own, up to conductor 40, both parities, non-real), `s` on a grid of 60
   points covering `Re(s)` from -6 to 6 and `|Im s|` to 20, including the trivial zeros, points near 0 and 1,
   and balls with radii `2^-30`, `2^-60`: `adf_tate_integral` (where it applies), `adf_tate_continue`,
   `adf_tate_completed` at `bits` 20, 53, 80: each `OK` result contains your 60-digit value and FLINT's, with
   every coordinate diameter at most `2^-bits`; the statuses where the design says `DOMAIN`, `NOT_DETERMINED`,
   `LIMIT`; a ball's result contains the values at the four corners and the centre. The functional equation
   through `tate_fe` and through your own two calls: `Lambda(s, chi)` against `W Lambda(1 - s, conj chi)` with
   YOUR `W` (exact cyclotomic Gauss sum): overlap, and both against your `L`.
3. **The certificates.** P15's cutoffs and L14's bound: for 100 cases, recompute `E_n`, `E_t` and the panel
   remainders yourself from the statements and check that the code's added error (the hidden hook
   `adf_tate_trace`/`adf_tate_cutoffs` in `src/tate.c`, or the difference between the returned radius and
   the radius of the midpoint value) is at least yours; L14's second branch: find a case where it is taken
   and check the bound against `mpmath.quad` of the integrand; T3's geometric remainder against the true
   tail of the Taylor series at 20 panels. A bound below the truth is a BLOCKER even if the final enclosure
   happens to hold.
4. **Width and refusal.** The N-D23 witness class: input balls whose image is wider than `2^-bits`:
   `NOT_DETERMINED`, never `OK`; `OK` results' diameters against `2^-bits` on 2000 random cases; the retry
   rule (does a result ever get `OK` with the target unmet?); time to `LIMIT` for `C = 1021`, `bits = 2^21`,
   `prec` at the cap, a tiny `|Im s|`: within 60 s each.
5. **The driver.** 60 lines of `tate_local`, `tate_gamma`, `tate_epsilon`, `tate_vector`, `tate_integral`,
   `tate_continue`, `tate_completed`, `tate_fe` with malformed operands (wrong kinds, non-integer `BITS`,
   `2^64`, a place not a prime, a non-primitive character text, a character of conductor above the caps),
   each expected line derived by hand before the run; the printed enclosures against hunt 2's values.
6. **Memory.** Your harnesses against `build-san` with `ASAN_OPTIONS=detect_leaks=1` on 2000 cases of hunts 1
   and 2 and every status case of hunt 4; `valgrind` on 200 cases if the sanitizer cannot run.
7. **Faults.** Twelve of your own across the five slices (the sign of the additive character in `G_minus`;
   `eta_0` for `eta_0^-1`; the integral's pole list used for gamma; `W` conjugated; the dual exponent `z` for
   `z'`; the pole terms with the wrong sign; the tail without the factor 2 of L14; the T3 remainder dropped;
   `C^-z` applied twice; the width checked before the error is added; the real `rgamma` replaced by a Gamma
   division; a side of `tate_fe` copied), planted in scratch copies under your lane directory; build the lanes'
   tests (`test_tate_local`, `test_tate_gamma`, `test_tate`, `test_tate_continue`, `test_tate_fe`) against
   each and record which are detected. A fault that passes is a finding (MAJOR if it loses enclosure or a sign).
8. **The statements and proofs.** `docs/api-5a.md`, `docs/api-5b.md`: each "Check:" line, is it checked by a
   test? One sentence per statement that is not. Referee S9 (the split is an identity on the whole plane) and
   S12 (the trivial zeros are finite) step by step: a false or missing step is a finding. The HEADER-FINDING of
   lane t5-slice1 (api-5:33 against :80 at the precision cap): which reading do the headers now state?

**You own:** `lanes/m5-review1/` and a new `docs/reviews/m5/review-m5.md` (the record, written last, same
content as the report). Everything else is read-only. No git command that changes state, no `bd`. At most 2
cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/m5-review1/build
lanes/m5-review1/build/libadelefeld.a`, and once with `SAN=1 INV=1` into `lanes/m5-review1/build-san`; link your
programs against them (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's suites as a whole. Every
program under `timeout`, none over 170 s. Leave no files in `/tmp` and none outside your lane directory.
Another lane (t5-repair1) appends tests to `tests/test_tate*.c` and `tests/test_localfactor.c` in another
worktree: the versions you see are the ones to plant faults against. Your worktree is `../adelefeld-wt/m5-review1`
on branch `lane/m5-review1`; `refs/src` is a symlink to the repository's copy. Slice 5e (`tate_fe`) is merged and
is in your tree.

Severity BLOCKER: a true value outside a returned enclosure; a wrong sign (the character, the Gauss sum, `W`,
`i^e`, the pole terms); a bound below the truth in a certificate; a memory error, leak or undefined behaviour;
an output written on a failure status; `OK` with a coordinate diameter above `2^-bits`; a forbidden
`acb_dirichlet` call on the continuation path. MAJOR: a radius beyond the stated bound; a wrong status; a hang
where `LIMIT` is promised; a fault the lanes' tests pass when it loses enclosure or a sign; a false step in S9
or S12. MINOR: everything else true.

Report: `lanes/m5-review1/report.md`, written once, at the end,
and the same text as your final message; notes in `lanes/m5-review1/progress.md` as you go. For each finding:
severity, the exact input, what the code returns, what is true and the sentence (file and line) that says so,
the command that reproduces it. Then what you attacked without result, with counts and what would have made a
case fail; the fault table; the "Check:" lines not covered. No praise, no summary. Keep generators, not their
outputs; delete your build trees and executables at the end. Aim to finish within about two hours: if the
time is short, the order of importance is hunts 1 and 2 (the values and the signs), 3, 7, then 4, 5, 6, 8.
If a long run of yours is still going, wait for it with a single blocking command, not many short polls.
