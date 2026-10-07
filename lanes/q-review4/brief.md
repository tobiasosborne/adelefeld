# Lane q-review4: adversarial review of the additive character on adeles (lane q-slice3, slice 3.2-a)

Your task is to REFUTE, not to confirm. Lane q-slice3 (codex `gpt-6.1-sol`; you are of another model family on
purpose) landed on 2026-10-07 at `9593815`, not reviewed: `include/adelefeld/psi.h` and `src/psi.c` with the
five functions `adf_fball_psi_tate_phase`, `adf_adele_psi_tate_phase`, `adf_phase_get_acb`,
`adf_adele_psi_tate`, `adf_adele_psi_tate_strict`; the driver commands `psi X` and `psi_strict X`
(`tools/adf/adf.c`, `tools/adf/README.md`, fixtures `tests/driver/psi-*`); `tests/julia/psi.jl`;
`tests/test_psi.c` with the vectors `tests/ref/vectors/q-slice3/` (generator `lanes/q-slice3/gen_vectors.py`);
the statements `docs/api-3b.md`. The lane's report is `lanes/q-slice3/report.md` (read it: it says what was
tested and how, and what it could not run: LeakSanitizer).

Contract: the comment blocks of `psi.h`; `docs/api-3.md` section 3 (3.1 definition and phase sources, 3.2
declarations and statuses, 3.3 numerical accuracy and the rectangular hull), statement Q4 of section 4, the
six character faults of section 5; `docs/conventions.md` section 6 (the sign conventions: `psi_p(x) = E(fp_p(x))`,
`psi_inf(x) = E(-x)`, lines 844 and after; the B-th-root family on a fractional finite radius, 876-878; default
against strict, 881-887; CV-54, CV-59); `docs/SPEC.md` section 6; `docs/proofs/analysis.md` Lemma 2; the
primary source `refs/src/tate-poonen/notes.txt:693-700` and `:733-740` (open it; the sign is the thing most
easily got wrong, and the lane's own derivation is in its report under "Hand derivation of the sign": check
every step against the source, not against the lane's text). The design review's exact model of your own
family exists: `lanes/q-review1/review_checks.py` (exact rationals, no import of the author's oracle). Reuse
what you need by copying into your lane directory; do NOT use `proto/quotient3_checks.py` or the lane's
vectors for the verdict.

**You own:** `lanes/q-review4/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/q-review4/build
lanes/q-review4/build/libadelefeld.a`, and once with `SAN=1 INV=1` into `lanes/q-review4/build-san`; link your
programs against them (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's suites as a whole. Every
program under `timeout`, none over 170 s. Leave no files in `/tmp` and none outside your lane directory.

Severity BLOCKER: a phase of a point of the input adele that is NOT in the returned `acb` (the enclosure is
lost); a wrong sign (the value at `(0 ; 1/3)` must be `E(1/3) = -1/2 + i sqrt(3)/2` by conventions 6.1 and the
source; derive it yourself first); an exact phase getter returning `OK` with a wrong `theta`, or `OK` where
the phase is not a single value; a memory error, a leak, or undefined behaviour; an output written on
`NOT_DETERMINED`, `DOMAIN`, `UNSUPPORTED` or `LIMIT`. MAJOR: a rectangle wider than design 3.3 allows (the
bound the lane asserts is `4*2^-p + 2^-28*(W/2 + 2*2^-p)` per coordinate: check that this IS the bound of
3.3, and that the code meets it); `strict` returning `OK` on an input whose phase is not determined (a
fractional finite radius with more than one root, CV-59), or `NOT_DETERMINED` on one whose phase is; a
wrong status order (`LIMIT` for `prec` above the cap must come first); the driver printing a value that
differs from the library's.

Hunt, in this order; stop a line of attack after some thousands of cases without a finding:
1. **The sign and the exact phases, by your own exact model.** Your own `fp_p` and `theta(x) = -x_inf +
   sum_p fp_p(x_p) mod 1` in exact rationals from the source lines; a harness that reads an exact adele
   (dyadic real part as integer and exponent; finite part as the triple `(A, H, d)` or as a rational centre
   with modulus) and prints `theta` from `adf_adele_psi_tate_phase` and `adf_fball_psi_tate_phase` exactly.
   5000 exact adeles: finite parts with denominators 1, 2, 3, 4, 6, 8, 12, 360, prime powers up to `2^20`,
   2000-bit numerators, negative; real parts 0, integers, dyadics with exponents from -200 to 200; local
   backend finite balls (see how `tests/test_qclass.c` builds a context with blocks 8, 9, 5; a raw triple
   with `d > 1` whose canonical `d` is 1). Diagonal rationals `q` embedded exactly: `theta = 0` (triviality,
   analysis Lemma 2); additivity `theta(x + y) = theta(x) + theta(y) mod 1` on 2000 pairs.
2. **Enclosure of balls.** For adeles with real radius > 0 and finite radius 0, integral, and fractional
   (`N = A/B` with `B` in 2, 3, 4, 6, 12, 360 and 2000 bits): 200 points of the input ball (end points, points
   just inside, interior rationals; for the finite part, residues of each root class), each exact phase
   must lie in the `acb` returned by `adf_adele_psi_tate` at `prec` 2, 20, 53, 128, 300 (containment of
   `E(theta)` in the complex ball, decided exactly or with a margin you justify). Then the exact hull of the
   arc (Q4: four rational distances) against the rectangle: tightness per coordinate under the 3.3 bound.
   Arcs crossing angle 0, 1/4, 1/2, 3/4 (interior extrema), arcs of width above 1 (the full circle), a hull
   that is a line (E3 of section 5: a square must fail). The case `(0 ; 1/3)` with real radius `2^-60`.
3. **Default against strict.** For each case of 2 with fractional finite radius: `strict` must give
   `NOT_DETERMINED` with `z` untouched (seed a sentinel and compare the bytes) exactly when CV-59 says; `default`
   must return an enclosure of ALL root phases (the B-th-root family of conventions 876-878): find a root class
   whose phase is outside the returned rectangle. Finite radius integral with a fractional raw triple that
   canonicalises to integral: both must give `OK`.
4. **phase_get_acb.** The cardinal phases 0, 1/4, 1/2, 3/4 exact (zero radius) at every `prec`, including the
   cap `ADF_REAL_PREC_MAX`; 2000 other rationals with denominators to `10^6` and of 2000 bits, enclosure and
   the 3.3 bound; `theta` not in `[0, 1)` or not reduced (the header calls it a precondition under `INV`: does
   the normal build do something defensible, or read garbage?); `prec` 1, 2, cap, cap + 1 (`LIMIT` first).
5. **Resources and memory.** Finite denominators of `2^20000` bits, real exponents at `ADF_QCLASS_EXP_MAX` and
   one beyond, `prec` at the cap: each returns its documented status within 60 s (time them). Your harness
   with `-fsanitize=address,undefined` against `build-san` on 5000 cases of 1 to 4 with
   `ASAN_OPTIONS=detect_leaks=1` (the lane could NOT run LeakSanitizer: this is the first leak run of
   `src/psi.c`; if LeakSanitizer does not run for you either, say so and use `valgrind --leak-check=full
   --error-exitcode=77` on 500 cases). Then `tests/test_psi` itself under the leak detector.
6. **The driver and Julia.** `psi` and `psi_strict` with 40 lines: `(0 ; 1/3)`, diagonal rationals, balls,
   a fractional finite radius under both, operands of other kinds (a class, an idele: what status?), `prec`
   settings; the printed value against your own evaluation of `E(theta)` to the printed digits; the
   expected lines derived by hand in your report before the run. `tests/julia/psi.jl`: does the test read
   the status before the output? does it free everything?
7. **Tests that cannot fail.** Plant ten faults of your own in a scratch copy of `src/psi.c` (the real sign
   reversed; the finite sign reversed; `fp_p` replaced by the ordinary fractional part at `p = 2`; `A` roots
   instead of `B` roots for `N = A/B`; the hull from the arc's end points only; `theta` not reduced into
   `[0, 1)`; the 3.3 excess doubled; the finite radius ignored in `default`; `strict` deciding from the raw
   `d` instead of the canonical one; `z` written before the `LIMIT` check), build `test_psi` against each
   and record which are detected. A fault that the test passes is a finding (MAJOR if it loses enclosure or
   gives a wrong sign or status), with the smallest input. The lane planted 11 faults; do not reuse its list
   as such, overlap is fine.
8. **The statements.** `docs/api-3b.md`: each "Check:" line, is it checked by `tests/test_psi.c`? One sentence
   per statement that is not.

Report: `lanes/q-review4/result.md` (a Claude subagent cannot write `report.md`), written once, at the end,
and the same text as your final message; notes in `lanes/q-review4/progress.md` as you go. For each finding:
severity, the exact input, what the code returns, what is true and the sentence (file and line) that says
so, the command that reproduces it. Then what you attacked without result, with counts and what would have
made a case fail; the fault table. No praise, no summary. Keep generators, not their outputs; delete your
build trees and executables at the end. Aim to finish within about forty minutes.
