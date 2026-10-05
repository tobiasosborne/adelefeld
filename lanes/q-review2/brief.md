# Lane q-review2: adversarial review of the first slice of milestone 3 (lane q-slice1: `adf_qclass`, lift only)

Your task is to REFUTE, not to confirm. Lane q-slice1 (codex `gpt-6.1-sol`; you are of another model family on
purpose) landed on 2026-10-05, not reviewed: `include/adelefeld/qclass.h`, `src/qclass.c` (lifecycle, `set`,
`swap`, `is_canonical` for BOTH forms, `identical`, `set_adele`, `set_rat`, `form`, `length`, `get_piece`,
`add_rat`, layout queries); the value text of the lift form (`adf_qclass_set_str`, `adf_qclass_get_str` in
`include/adelefeld/text.h`, `src/text.c`; the union form returns `UNSUPPORTED`); the driver kind `qclass` and
the command `qadd_rat` (`tools/adf/adf.c`, `tools/adf/README.md`); `tests/test_qclass.c`,
`tests/ref/vectors/q-slice1/`, `tests/julia/qclass.jl`, `tests/driver/qclass-lift.*`; statements
`docs/api-3a.md`. Contract: the comment blocks of `qclass.h` and of the two text functions; the design
`docs/api-3.md` sections 1, 2.1, 2.4, 2.5 (repaired after its review, `docs/reviews/m3-design/review.md`);
`docs/conventions.md` 5.10 (the struct, the two forms, CV-45: the midpoint of a piece in `[0, 1]`, the order
of the pieces), 4.4, 8.4, 8.5, 9.2, 9.4, 9.6; `docs/proofs/quotient.md` Proposition 10. The lane's report is
`lanes/q-slice1/report.md` (it says itself that the core functions were not each seen red before the code).
Another lane (q-slice2) is adding `adf_qclass_reduce` to the same files in another worktree: you review the
tree as it stands in yours.

**You own:** `lanes/q-review2/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/q-review2/build
lanes/q-review2/build/libadelefeld.a`, and once with `SAN=1 INV=1` into `lanes/q-review2/build-san`; link your
programs against them (`-Iinclude -lflint -lgmp -lm`). Do not run the repository's suites as a whole. Every
program under `timeout`, none over 170 s. Leave no files in `/tmp` and none outside your lane directory.

Severity BLOCKER: a memory error or undefined behaviour; `is_canonical` returning 1 for a value that violates
a clause of the invariant, or 0 for a value that satisfies all of them; a reader that returns `OK` with a
value that is not canonical or is not the set of the text; `add_rat` or `set` changing the represented set;
an output changed on a status other than `OK`.

Hunt, in this order; stop a line of attack after some thousands of cases without a finding:
1. **`is_canonical`, clause by clause**, against your own statement of the invariant written from conventions
   5.10 and design section 1 (Python, exact rationals; do not use `proto/quotient3_checks.py` for the verdict).
   Build PIECES values by hand through the struct (a binding may: conventions 12.4) from your own generator:
   random lists of 1 to 12 pieces with dyadic real balls and finite balls of every backend the adele admits;
   then break exactly one clause: the tag; `len` 0 or negative; a lift with `len` 2; a NULL array; a piece
   whose midpoint is `-2^-60`, `1 + 2^-60`, exactly 0, exactly 1; a piece whose BALL exceeds `[0, 1]` while
   its midpoint does not (allowed); two pieces in the wrong order under each key of the order that 5.10
   states (find the keys in the text; a pair that differs only in the last key); two equal pieces; pieces
   equal as sets but stored with different backends; a member adele that is not canonical; real balls with
   exponents of `2^60` and `-2^60` (the lane compares end points with a four-term `arf_sum`: is the sign
   right when the terms cancel exactly, when three cancel, at huge exponent gaps?). 20000 values: verdict of
   the C function against yours.
2. **The text of the lift form**: your own reader and printer of `(r ; F) + Q` from conventions 9.2 to 9.6
   (`proto/text_grammar.py` is the project's reference: compare three ways and report a disagreement between
   the two references with the sentence that decides). Valid texts of random lifts; malformed variants
   (spaces, a missing `+ Q`, `+Q`, `+ q`, two classes, a union with 0, 1, 1000 entries, nested parentheses,
   an embedded NUL, a cut at every byte, digits and exponents at the limits of 8.4); the union form must be
   `UNSUPPORTED` only AFTER the syntax checks that 8.5 puts before the value (a malformed union is `PARSE`, a
   union over `max_items` is `LIMIT`): check that order with texts that break two rules at once. On `OK`:
   canonical, and the printed text read again is a class that CONTAINS the first (conventions 9.6: the real
   part is a decimal enclosure); on failure the output is unchanged (compare the representation).
3. **`set_adele`, `set_rat`, `add_rat`, `get_piece`, `set`, `swap`**: the represented set by membership of
   rational points of `A/Q` in your own exact model (a point `(t ; q)` lies in the class of `(I ; a + N Zhat)`
   when some rational translation moves it into the ball: decide it exactly); 2000 random lifts, each with
   200 points, before and after `add_rat` by rationals of 1 to 2000 bits; aliasing (`set(x, x)`, `swap(x, x)`,
   `add_rat(x, x, r)`); `get_piece` at -1, `len`, `LONG_MAX`; contexts of local finite balls are borrowed:
   free the source adele after `set_adele` and use the class (what does the header promise about lifetime?).
4. **Memory**: your programs of 1 to 3 with `-fsanitize=address,undefined`, `ASAN_OPTIONS=detect_leaks=1`,
   against `build-san` (if LeakSanitizer does not run in your environment, say so and try
   `valgrind --leak-check=full --error-exitcode=77`); 1000 init/clear cycles with PIECES values copied.
5. **The driver**: `show`, `type`, `qadd_rat`, the pair commands with a class operand; 60 malformed lines;
   printed values against the library's.
6. **Tests that cannot fail**: plant ten faults of your own in a scratch copy of `src/qclass.c` and of the
   qclass part of `src/text.c` (the midpoint test `>= 0` made `> 0`; the order compared on the first key only;
   the duplicate test dropped; `set` copying `len - 1` pieces; `add_rat` translating the real part without
   the finite part; `set_rat` of a non-integer giving its lift instead of the zero class; the union refused
   before the syntax check; the printer omitting ` + Q` for a zero class; `get_piece` accepting `i = len`;
   `swap` exchanging two of the three fields), build `test_qclass` against each and record which are
   detected. A fault that the test passes is a finding (MAJOR if it gives a wrong set or accepts an invalid
   value), with the smallest input that shows it.

Report: `lanes/q-review2/report.md`, written once, at the end; notes in `lanes/q-review2/progress.md` as you
go. For each finding: severity (BLOCKER as above; MAJOR; MINOR), the exact input, what the code returns, what
is true and the sentence that says so, the command that reproduces it. Then what you attacked without result,
with counts and what would have made a case fail; the fault table. No praise, no summary. Delete your build
trees and executables at the end.
