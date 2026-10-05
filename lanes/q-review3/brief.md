# Lane q-review3: adversarial review of the reduction of quotient classes (lane q-slice2: `adf_qclass_reduce`)

Your task is to REFUTE, not to confirm. Lane q-slice2 (codex `gpt-6.1-sol`; you are of another model family on
purpose) landed on 2026-10-05, not reviewed: `adf_qclass_reduce` in `include/adelefeld/qclass.h` and
`src/qclass.c` (algorithm R of `docs/api-3.md` section 2.2, steps 1 to 5; the rounding kernel Q1 of section 4;
the piece limit; resource bounds); the printer of the union form `union(X, X, ...) + Q` in `src/text.c`
(`adf_qclass_get_str`; it sorts and deduplicates by PRINTED keys); the driver command `qreduce X with LIMIT`
(`tools/adf/adf.c`, `README.md`); `tests/test_qclass_reduce.c`, `tests/ref/vectors/q-slice2/`,
`tests/driver/qclass-reduce.*`, `tests/julia/qclass.jl`; the appended statements of `docs/api-3a.md`. The
lane's report is `lanes/q-slice2/report.md`. Contract: the comment block of `adf_qclass_reduce`; design
sections 1, 2.2, 4 (Q1, Q5); decision N-D21 (`docs/SPEC.md` 15.4: the count is taken BEFORE rounding and
deduplication; `LIMIT`, never `NEEDS_SPLIT`; no full-image shortcut); `docs/conventions.md` 5.10 (CV-45);
`docs/proofs/quotient.md` P3, P6, P8, P10; `docs/PLAN.md` row 3.1 (its tests).

An independent exact model of the reduction exists already and is by your own family: the design review
`lanes/q-review1/review_checks.py` (membership of rational points of `A/Q` in a ball's image and in a union of
pieces, in exact rationals, no import of the author's oracle). Reuse it (copy what you need into your lane
directory); do NOT use `proto/quotient3_checks.py` or the lane's vectors for the verdict.

**You own:** `lanes/q-review3/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/q-review3/build
lanes/q-review3/build/libadelefeld.a`, and once with `SAN=1 INV=1` into `lanes/q-review3/build-san`; link your
programs against them (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's suites as a whole. Every
program under `timeout`, none over 170 s. Leave no files in `/tmp` and none outside your lane directory.

Severity BLOCKER: a point of the input class that is NOT in the returned union (the enclosure is lost); a
returned value that is not canonical; a memory error or undefined behaviour; an output changed on `LIMIT`.
MAJOR: a piece wider than the Q1 bound allows (the result is an enclosure but not the promised one); a wrong
status (`LIMIT` when the construction count is within the limit, `OK` when it exceeds it); a call that does
not return within 60 s where the header promises `LIMIT`.

Hunt, in this order; stop a line of attack after some thousands of cases without a finding:
1. **Enclosure, by exact membership.** A harness that reads a lift (dyadic midpoint and radius of the real
   ball as integers and exponents; the finite ball as the triple `(A, H, d)`), calls `adf_qclass_reduce`, and
   prints every stored piece exactly (dyadic end points, finite triple). In Python, for each case, 200
   rational points of the INPUT class (points of the ball translated by random rationals, the end points, the
   points just inside each end, points on the glued boundary `t = 0 ~ t = 1`): each must lie in some stored
   piece AS A POINT OF `A/Q` (your exact membership). Cases: 20000 random lifts: real balls with midpoint
   exponents from `-200` to `200`, widths from 0 to 40, end points exactly at integers and half-integers;
   finite radii `N` in 0, 1, 2, 3, 12, 360 and fractional `1/2, 1/3, 2/3, 3/2, 5/4, 7/360`; centres with
   denominators up to 360, negative, of 2000 bits; local-backend finite balls (a context with blocks 8, 9, 5:
   see how `tests/test_qclass.c` builds one). Then PIECES inputs: the output of a reduction reduced again; a
   hand-built piece with spill on both sides; midpoint exactly 0 and exactly 1.
2. **Tightness and count.** For each case of 1: each stored piece against the exact piece it must enclose (your
   own algorithm R in exact rationals): the stored radius exceeds the exact half-width by at most `2^-28`
   of it (Q1), the midpoint is in `[0, 1]`; the number of stored pieces equals your count after
   deduplication of STORED keys; the status against the limit: `piece_limit` equal to the construction count
   is `OK`, one less is `LIMIT` (the count before deduplication: find a case where deduplication removes a
   piece and the limit lies between the two counts).
3. **Resources**: `B` (the denominator of a fractional radius) of `10^6`, `10^30`, `2^5000`; widths of
   `10^6`, `10^30`; `piece_limit` at 1, `LONG_MAX`, 0, negative; `prec` at 2, at the cap, one above; each must
   return its documented status within 60 s (time them); allocations: the limit is compared before the
   array is allocated (a limit of `10^9` with a count of `10^9 - 1`: what happens? the header's promise on
   memory); `y = x`.
4. **The union printer**: printed text against your own printer from conventions 9.4 for 2000 reduced
   values; the order of printed entries (the lane says printed order can differ from stored order: is the
   printed order the one the conventions state?); two stored pieces that print identically (the lane
   deduplicates printed keys: is the printed union still a superset of the stored one? a case where it is
   not is a BLOCKER of the printer); digits 1, 3, 20, 1000.
5. **Memory**: your harness with `-fsanitize=address,undefined` and `ASAN_OPTIONS=detect_leaks=1` against
   `build-san` on 5000 cases of 1 and all cases of 3 (if LeakSanitizer does not run, say so and use
   `valgrind --leak-check=full --error-exitcode=77` on 500 cases).
6. **The driver**: `qreduce` with 60 lines (limits that are not integers, negative, `2^64`; operands of other
   kinds; a class in union form as input: `UNSUPPORTED` from the reader); the examples E1 to E4 of design 2.2
   and the PLAN tests of row 3.1, each derived by hand in your report before you run it.
7. **Tests that cannot fail**: plant ten faults of your own in a scratch copy of `src/qclass.c` (the upper end
   point `min(h, n + 1)` replaced by `n + 1`; the piece for the last integer dropped when `h` is an integer
   plus `2^-200`; fibres `0 <= j <= B`; the shift `-n` reduced modulo `A` with the wrong sign for negative
   `n`; Q1 with the successor omitted when the radius is a power of two; deduplication on the real key only;
   the limit compared with the count after deduplication; `y` written before the last allocation succeeds;
   the midpoint clamped into `[0, 1]` instead of the piece being built there; exponent bound off by one),
   build `test_qclass_reduce` against each and record which are detected. A fault that the test passes is a
   finding (MAJOR if it loses enclosure or gives a wrong status), with the smallest input.

Report: `lanes/q-review3/report.md`, written once, at the end; notes in `lanes/q-review3/progress.md` as you
go. For each finding: severity, the exact input, what the code returns, what is true and the sentence that
says so, the command that reproduces it. Then what you attacked without result, with counts and what would
have made a case fail; the fault table. No praise, no summary. Do not commit large generated files: keep
generators, not their outputs. Delete your build trees and executables at the end.
