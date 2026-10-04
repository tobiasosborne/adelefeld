# Lane q-slice1: the first slice of milestone 3: `adf_qclass`, lift only (slice 3.1-a of `docs/api-3.md`)

Lane d-quotient designed work packages 3.1 and 3.2 before code: `docs/api-3.md` (876 lines: the type and its
common contracts in section 1, the declarations of 3.1 in section 2, statements Q1 to Q5 in section 4,
acceptance tests and faults in section 5, the thin slices in section 7) and the oracle
`proto/quotient3_checks.py`. Its report is `lanes/d-quotient/report.md`. The design is NOT reviewed yet: where
you find it wrong, contradictory or unimplementable, do not paper over it; implement the sound reading and put
the point under "Findings against the design" in your report.

You build slice **3.1-a** (section 7, line 773: "lifecycle, layout, set_adele/set_rat, form/length/get_piece,
add_rat; LIFT-only text printing"; "Decision first: none"), end to end: header, code, test against the
reference, driver command, Julia call. Nothing of reduction, pieces construction, set queries, group
arithmetic, dump or the character: those are later slices (3.1-b to 3.1-f, 3.2-a to 3.2-c), and a function of
a later slice is not declared now.

Read first (CLAUDE.md rules 3, 4): `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md`, `docs/workflow.md`;
`docs/api-3.md` sections 1, 2.1, 2.4 (the rational translation only), 2.5 (the value text: what the lift form
needs), 5 (the tests of these functions), 7; `docs/conventions.md` 5.10 (the struct, the two forms, CV-45),
3 and 4 (statuses, outputs, aliasing, debug entry checks 4.4), 9.2 and 9.4 (the value form `(r ; F) + Q`),
12.4 (layout queries); `docs/SPEC.md` 6 (line 386 on); `docs/proofs/quotient.md` Proposition 10 (P10.1,
P10.3); the headers `adele.h`, `rat.h`, `text.h` (the readers and printers of the other types: style, limits,
the classifier `adf_text_classify` already knows the kind `qclass`) and `src/text.c` for how a value form is
read and printed; `idclass.h` and `src/idclass.c` as the most recent type of this size (lifecycle, predicate,
layout queries, `INV` entry checks).

## The slice

A. `include/adelefeld/qclass.h`, `src/qclass.c`: the struct and constants exactly as conventions 5.10 and
   design section 1 give them; `adf_qclass_init`, `_clear`, `_set`, `_swap`, `_is_canonical`, `_identical`,
   `_set_adele`, `_set_rat`, `_form`, `_length`, `_get_piece`, `adf_sizeof_qclass`, `adf_alignof_qclass`
   (design 2.1, declarations and comment blocks as written there unless a finding forces a change), and the
   translation by a rational of design 2.4 (`adf_qclass_add_rat`: the class is unchanged as a set: what
   does the design say about the stored representation? follow it). `is_canonical` must already judge BOTH
   forms (a `pieces` value can be built by hand in a test through the struct, as a binding may: conventions
   12.4): the invariant of design section 1 with the midpoint rule CV-45 and the order of the pieces.
B. The value text of the LIFT form: reader and printer in the style of the other kinds (`text.h`,
   `src/text.c`): `adf_qclass_get_str` prints `(r ; F) + Q`; `adf_qclass_set_str` reads that form, and for
   the form `union(...) + Q` returns `ADF_UNSUPPORTED` in this slice (say so in the header; slice 3.1-d
   replaces it), after the syntax checks of conventions 8.5 that precede the value. Golden vectors:
   `tests/golden/qclass.tsv` (read `tests/golden/README.md`): every row of the lift form is run by your
   test; rows of the union form are run for the status only.
C. Tests first, `tests/test_qclass.c`: the lifecycle under the sanitizers; `set_adele` keeps every field of
   the adele (identity with `get_piece`); `set_rat` of 0, 1/2, -7/3, a rational of 2000 bits is the zero
   class and identical to `init`; `add_rat` by 1000 random rationals leaves the represented set unchanged
   (compare against the oracle: `proto/quotient3_checks.py` has exact set membership: generate vectors under
   `tests/ref/vectors/q-slice1/` with a script in your lane directory: for a lift `(r ; F)` and a rational
   `q`, points of `A/Q` that are and are not in the class, and the same after translation); `is_canonical`
   on hand-built values of both forms: one accepted and one refused value for each clause of the invariant
   (bad tag, `len` 0, a lift with `len` 2, a piece with midpoint outside `[0, 1]`, pieces out of order, a
   repeated piece, a piece whose adele is not canonical); `get_piece` out of range; aliasing `set(x, x)`,
   `swap(x, x)`; the layout queries against `sizeof`; the text round trips. Red first, then the code.
D. A user call: driver (`tools/adf/adf.c`, `README.md`): `qclass` becomes a value kind the driver holds (the
   pattern of lane drv-ball for `lball`, `sball`: commit `ca26a21`): `show (0.5 ; 0) + Q` prints the class,
   `type` names it, the pair commands answer as for the other kinds without arithmetic (`DOMAIN`), and one
   new command `qadd_rat X with R` (or the name design section 7 proposes). Fixture
   `tests/driver/qclass-lift.cmd` and `.out`, expected lines written before the run. SEVERAL existing
   fixtures use `(0.5 ; 0) + Q` as "a kind of the value form with no typed parser: UNSUPPORTED"
   (`06_pairs`, `07_status`, `12_status_order`, `13_dump`, `f-places-hostile`): they will answer differently.
   Do NOT edit them: list fixture, line, old and new text in the report, and propose which kind without a
   parser (`ffun`, `rfun`, `char`: look at conventions 9.2 for a short valid text of one of them and check
   that `type` names it) should replace it. `tests/julia/qclass.jl` with the calls of design section 7 that
   exist in this slice, registered in `tests/test_julia.sh`.
E. Statements: a new file `docs/api-3a.md`: for each function of the slice one statement (what set the
   result is, by reference to design section 1 and `quotient.md` P10) with a "Check:" line naming the test;
   the decisions you had to take where the design is silent, each with the alternative.

**You own:** `include/adelefeld/qclass.h`, `src/qclass.c`, `tests/test_qclass.c`, `tests/ref/vectors/q-slice1/`,
the qclass reader and printer in `include/adelefeld/text.h` and `src/text.c` (additions only), one line in
`include/adelefeld.h`, `tools/adf/adf.c` and `README.md` (the kind and the one command), new
`tests/driver/qclass-*`, `tests/julia/qclass.jl` and its lines in `tests/test_julia.sh`, `docs/api-3a.md`,
`lanes/q-slice1/`. Everything else is read-only, `docs/api-3.md`, `docs/conventions.md` and the existing
fixtures included. No git command that changes state, no `bd`.

**Checks at the end** (commands and numbers in the report; at most 2 jobs; every program under `timeout`):
`timeout 1700 make -j2 check-all` once (it will stop at the driver fixtures named in D: say where; run the
remaining parts separately: exports, Julia, both self-tests); `test_qclass` and the text tests under `SAN=1`
(your sandbox cannot run LeakSanitizer: say so), `INV=1`, `CC=clang` in build directories under your lane
directory; mutation testing of `src/qclass.c` as `lanes/COMMON-C.md` rule 5 says (at most 60 mutants, 20
minutes). Remove your build trees at the end. Lines at most 116 characters; files end with a newline.

Report: `lanes/q-slice1/report.md` (rule 8 of `lanes/COMMON.md`): what is done per part; the table of old
fixtures that change; findings against the design; mutation survivors with one line each; what is not done.
