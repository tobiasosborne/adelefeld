# Lane m3-review1: adversarial review of milestone 3, work packages 3.1 and 3.2 (the quotient and the character)

Your task is to REFUTE, not to confirm. Six slices landed between 2026-10-07 and 2026-10-08 without review
(TJO: one review when a major feature lands, not one per slice). The code under review, with the lane that
wrote it and its report (read each report: what it tested and what it could not):

| Slice | Code | Lane, model, report |
|---|---|---|
| 3.2-a | `include/adelefeld/psi.h`, `src/psi.c`: the five adele and phase functions; driver `psi`, `psi_strict` | q-slice3, codex, `lanes/q-slice3/report.md` |
| 3.2-b, 3.2-c | `src/psi.c`: `adf_qclass_psi_tate`, `_strict`, `_phase`; `adf_lball_psi_tate`, `_strict`, `_phase`; `adf_adele_psi_tate_at`, `_strict_at`; driver `psi_at`, `psi_phase` | q-slice5, Opus, `lanes/q-slice5/result.md` |
| 3.1-d | `adf_qclass_set_pieces` (`src/qclass.c`), the union branch of `adf_qclass_set_str` (`src/text.c`), the four qclass dump functions (`src/dump.c`); driver `print`, `dump`, `load` | q-slice4, codex, `lanes/q-slice4/report.md` |
| 3.1-e | `src/qclass_sets.c`: `adf_qclass_equal_set`, `_contains`, `_overlaps`; driver `qequal`, `qcontains`, `qoverlaps` | q-slice6, codex, `lanes/q-slice6/report.md` |
| 3.1-f | `src/qclass_arith.c`: `adf_qclass_neg`, `adf_qclass_add`; driver `qneg`, `qadd` | q-slice7, Opus, `lanes/q-slice7/result.md` |
| repair | `src/dump.c` piece-count preflight (`DP_QPIECE_MIN`) | r-dump1, Opus, `lanes/r-dump1/result.md` |

Contracts: `docs/api-3.md` (sections 1, 2.2 to 2.5, 3, Q1 to Q5, section 5's faults, section 6's decisions
D3-1 to D3-3), the statements the lanes wrote in `docs/api-3a.md` (slices 3.1-d, 3.1-e, 3.1-f) and
`docs/api-3b.md` (3.2-a, 3.2-b, 3.2-c), `docs/SPEC.md` 6 and 15.4 (N-D21), `docs/conventions.md` 5.10, 6.1
(the sign: `psi_p(x) = E(fp_p(x))`, `psi_inf(x) = E(-x)`), 9.2 to 9.5, 10.1, 10.2, 11.3,
`docs/proofs/quotient.md` P3, P6, P8, P9, P10, `docs/proofs/analysis.md` Lemma 2, `docs/proofs/precision.md`
P1, and the primary source `refs/src/tate-poonen/notes.txt:693-700` (open it: derive the sign yourself before
reading the lanes' derivations). Independent exact models of your own family exist: `lanes/q-review1/
review_checks.py` (membership in `A/Q`, exact rationals) and `lanes/q-review3/model.py`, `check.py` (the
exact containment model of a reduced class). Reuse them by copying into your lane directory; do NOT use
`proto/quotient3_checks.py` or the lanes' vectors for a verdict.

The hunt list for the character (3.2-a) is written already: `lanes/q-review4/brief.md`, hunts 1 to 8
(the sign and exact phases; enclosure of balls; default against strict; `phase_get_acb`; resources and
LeakSanitizer; driver and Julia; ten planted faults; the statements). Follow it, and extend each hunt to the
class, local and place calls of 3.2-b and 3.2-c (D3-3 per stored entry; the E3 witness: the lift of E3 is
`NOT_DETERMINED` under strict, its reduction is `OK`, both enclose `{+1, -1}`; `fp_p` at `p = 2` with `e < 0`;
the product over places against the global value; `where` untouched on `OK`; primes near `2^64`).

For 3.1-d, 3.1-e, 3.1-f, hunt in this order; stop a line after some thousands of cases without a finding:
1. **Set queries by brute force.** Your own exact decision of equality, containment and overlap of two
   represented sets (fibres over the lcm of the moduli, glued at `t = 0 ~ t = 1`, spill, point fibres) on
   5000 random pairs (lifts and PIECES; moduli 2 to 12 and 360; fractional radii `1/2, 2/3`; end points at
   integers and half-integers; 2000-bit centres), each decided also by exhaustive rational point sampling as a
   second check; then `adf_qclass_equal_set`, `_contains`, `_overlaps` against it; the budget: `LIMIT` exactly
   when `K > W`, `L > W` or `(2E+1) K L > W` by YOUR count of `K`, `L`, `E` (the design 2.3 definitions), and
   `truth` untouched then; a `B = 10^100` radius refused before any loop (time it).
2. **Arithmetic by membership.** For 3000 pairs, 200 rational points of `x` and `y` each: every `u + v` lies
   in some stored piece of `adf_qclass_add(x, y)` and every `-u` in `adf_qclass_neg(x)` (exact membership in
   `A/Q`); each stored piece against the exact piece of your own construction (sum of end points, `(a + b) +
   gcd(N, M) Zhat` with the gcd of rationals as the subgroup generator, R, then Q1): radius excess at most
   `2^-28` of the exact radius; the count before deduplication against `piece_limit`; `x + x` wider than `2x`
   (independent variation); aliasing `z = x = y` with nonzero radii.
3. **The union reader and the constructor.** 5000 union texts (your own printer from conventions 9.4):
   `adf_qclass_set_str` then `adf_qclass_get_str`: the stored set encloses the text's set (exact end points
   in 11.3's enclosure sense) and is canonical (sorted by exact keys, no equal keys); `max_items` at the
   count and one below; the stage order on texts violating two rules; `set_pieces` with duplicates (the
   count before deduplication; which backend is kept); a non-finite entry; `piece_limit` 0.
4. **The dump forms.** 5000 dumps of random classes (lifts and pieces, global and local finite parts with one
   and several contexts): `dump` then `load` identical; `load` then `dump` byte-identical for valid texts;
   `inspect` the context count; mutated bytes (a letter in a count, a truncated body, a count of 0, a body out
   of canonical order: `DOMAIN`, not a normalisation), every one without a FLINT call on the bytes (interpose
   `arb_load_str` as the lane did), under `SAN=1`; `load_str_binds` with `nbinds` wrong; the M1-D9 bound.
5. **Memory.** Your harnesses against `build-san` with `ASAN_OPTIONS=detect_leaks=1` (the codex lanes could
   not run LeakSanitizer; this is the first leak run of `src/qclass.c`'s new code, `src/qclass_sets.c` and the
   dump functions), 5000 cases per hunt; then the lanes' own test programs under the leak detector.
6. **The driver.** 60 lines across `qequal`, `qcontains`, `qoverlaps`, `qneg`, `qadd`, `print`, `dump`, `load`,
   `psi*`: values derived by hand first; operands of other kinds; limits that are not integers; a union with
   duplicate pieces; a load of a dump out of order.
7. **Faults.** Ten of your own across the six slices (not the lanes' lists; overlap is fine), planted in scratch
   copies; build the lanes' test programs against each and record which are detected.
8. **The statements.** For `docs/api-3a.md` (3.1-d, e, f) and `docs/api-3b.md`: each "Check:" line, is it checked
   by a test? One sentence per statement that is not.

**You own:** `lanes/m3-review1/` only. Everything else is read-only. No git command that changes state, no `bd`.
At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/m3-review1/build
lanes/m3-review1/build/libadelefeld.a`, and once with `SAN=1 INV=1` into `lanes/m3-review1/build-san`; link
your programs against them (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's suites as a whole.
Every program under `timeout`, none over 170 s. Leave no files in `/tmp` and none outside your lane directory.

Severity BLOCKER: a point of an input set not in the result (enclosure lost; a wrong set-query truth; a phase
outside the returned rectangle; a wrong sign); a non-canonical result; a memory error, leak or undefined
behaviour; a FLINT call on unvalidated bytes; an output written on a failure status. MAJOR: a piece wider than
Q1 allows; a wrong status (`LIMIT` within the budget, `OK` beyond it; `strict` `OK` on an undetermined input);
a hang where the header promises `LIMIT`; a fault that the lanes' tests pass when it loses enclosure or changes
a truth or a sign. MINOR: everything else true.

Report: `lanes/m3-review1/result.md` (a Claude subagent cannot write `report.md`), written once, at the end,
and the same text as your final message; notes in `lanes/m3-review1/progress.md` as you go. For each finding:
severity, the exact input, what the code returns, what is true and the sentence (file and line) that says so,
the command that reproduces it. Then what you attacked without result, with counts and what would have made
a case fail; the fault table. No praise, no summary. Keep generators, not their outputs; delete your build
trees and executables at the end. Aim to finish within about ninety minutes: if the time is short, the order
of importance is hunts 1, 2 and the character's sign and enclosure, then 3, 4, 5, then the rest.
