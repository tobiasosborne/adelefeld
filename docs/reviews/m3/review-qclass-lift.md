<!-- ROLE: record of a review. The text below the rule is lanes/q-review2/report.md as written (pi agent with
     stealth/space-bunny-alpha, 2026-10-05), not edited. Under review: include/adelefeld/qclass.h, src/qclass.c,
     the qclass reader and printer of src/text.c, the driver kind (lane q-slice1, codex gpt-6.1-sol). -->

# lane q-review2 report: adversarial review of the first slice of milestone 3

Lane q-review2, 2026-10-06. Reviewer of a different model family from the lane under review
(q-slice1, codex `gpt-6.1-sol`). Nothing outside `lanes/q-review2/` was written; no git command
and no `bd`; at most two cores; every program under `timeout`.

## What was done

Six lines of attack, in the order of the brief.

1. `is_canonical`, clause by clause, against my own statement of the invariant written from
   conventions 5.10 (with CV-45) and docs/api-3.md section 1. Values were built by hand through the
   struct (allowed by conventions 12.4) from my own generator: random lists of 1 to 12 pieces with
   dyadic real balls and finite balls of the global backend and of four local contexts (blocks
   (2,3), (5,7), (3,5,7), (243, 2^32)), then one clause broken at a time.
2. The text of the lift form: my own reader and printer of `(r ; F) + Q` from conventions 9.1 to
   9.6 in exact rationals, a table of 72 texts with the status expected from conventions 8.2 to
   8.5, every cut of a valid text, and 2000 random valid lifts. Three-way comparison with
   `proto/text_grammar.py`.
3. `set_adele`, `set_rat`, `add_rat`, `get_piece`, `set`, `swap`: the represented set by exact
   membership of rational points of `A/Q`, before and after `add_rat`, with aliasing and
   `get_piece` out of range.
4. Memory: the programs of lines 1 to 3 against the sanitised archive with
   `ASAN_OPTIONS=detect_leaks=1`, 1000 lifecycle cycles with hand-built PIECES values and a
   borrowed context, and Valgrind.
5. The driver: `show`, `type`, `qadd_rat`, the pair commands with a class operand, 72 lines of
   which 60 are malformed or edge cases, and the printed values against the library's printer.
6. Ten planted faults from the brief and ten more of my own in a scratch copy of `src/qclass.c`
   and of the qclass part of `src/text.c`, each built against `tests/test_qclass.c`.

Result: no BLOCKER and no MAJOR finding. Two MINOR findings, both in section "Findings".

## Files written (all under `lanes/q-review2/`)

- Harnesses: `can_eval.c`, `text_eval.c`, `member_eval.c`, `cycle.c`, `borrow.c`,
  `probe_arf.c`, `probe_endcmp.c`, `probe_mag.c`, `probe_text.c`, `probe_exp.c`, `probe_big.c`.
- Oracles and drivers: `can_oracle.py`, `text_oracle.py`, `member_oracle.py`, `mutate.py`.
- Data and logs: `can20000.txt`, `can300.txt`, `can_subset.txt`, `hand_cases.txt`,
  `hand_cases2.txt`, `driver60.cmd`, `driver60.out`, `driver60.err`, `member_small.cmd`,
  `member2000.log`, `text_oracle.log`, `build-plain.log`, `build-san.log`, `bigrat.cmd`,
  `bigrat2.cmd`, `hang.cmd`, `r0.cmd`, `text_smoke.txt`.
- `progress.md` (written as the work went), this report.
- The build trees and the executables were removed at the end.

## Checks run, with commands and results

### Builds

    timeout 600 make -j2 BUILD=lanes/q-review2/build lanes/q-review2/build/libadelefeld.a
    -> exit 0
    timeout 600 make -j2 SAN=1 INV=1 BUILD=lanes/q-review2/build-san lanes/q-review2/build-san/libadelefeld.a
    -> exit 0

The driver `tools/adf` was compiled by me into the lane
(`cc -Iinclude ... tools/adf/adf.c lanes/q-review2/build/libadelefeld.a`); the tools Makefile
writes into the shared `build/`, which is outside this lane.

### Line 1: is_canonical

    timeout 170 python3 lanes/q-review2/can_oracle.py <seed> 20000

| seed | values | disagreements |
|---:|---:|---:|
| 1 | 20000 | 0 |
| 7 | 20000 | 0 |
| 11 | 20000 | 0 |
| 23 | 20000 | 0 |

Clause coverage per 20000 values (seed 11): the oracle calls 5343 values canonical and rejects the
rest as `d != 1` (5848), midpoint outside `[0,1]` (2801), order (1947), member not canonical
(1503), len (815), tag (567), lift with len != 1 (596), NULL array (556). The pieces cover the
global backend and the four local contexts, exact and positive finite radii, raw field faults
(`d = 0`, `H < 0`, `A >= H`, `A < 0`, gcd > 1, mctx on a global value, res on a global value,
backend tag 2, local without a context, res out of range, `H != K`), midpoints exactly 0 and 1,
`-2^-60`, `1 + 2^-60`, `1 - 2^-60`, `+-2^-2^60`, balls that exceed `[0,1]` while the midpoint does
not (allowed, expected canonical), and dyadics at binary exponents `+-2^60`, `+-2^40`.

Pairs that differ only in one key position, equal pieces, and the same set with two backends
(global `(1,6,1)` against local blocks (2,3) with residues (1,1) and `d = 1`, both triples
`(1,6,1)`) are generated; the last is rejected by C, as the invariant requires.

Hand cases with exact four-term cancellation, where the two large terms cancel and a small term
decides the sign, at a 61-bit and at a 10^6-bit mantissa and at binary exponents `+-2^60`:

    timeout 120 ./lanes/q-review2/can_eval < lanes/q-review2/hand_cases.txt
    timeout 120 ./lanes/q-review2/can_eval < lanes/q-review2/hand_cases2.txt
    -> 12 values and 8 values, every verdict as derived by hand (see below)

The harness was itself checked against a planted fault (the midpoint test `< 0` made `<= 0` in a
scratch copy of `src/qclass.c`): 127 disagreements in 2000 values, so the oracle does see a real
defect.

Sanitised run over the same 20000 values:

    ASAN_OPTIONS=detect_leaks=1 timeout 170 ./lanes/q-review2/can_eval_san < lanes/q-review2/can20000.txt
    -> exit 0, no report, output byte-identical to the plain build (diff exit 0)

    timeout 170 valgrind --leak-check=full --show-leak-kinds=definite,indirect ./lanes/q-review2/can_eval \
        < lanes/q-review2/can_subset.txt
    -> definitely lost 0 bytes, indirectly lost 0 bytes (1314 values)

### Line 2: the text of the lift form

    timeout 170 python3 lanes/q-review2/text_oracle.py

72 table entries, all 70 cuts (every prefix and every suffix) of
`(1.5 +/- 1e-5 ; 5/3 mod 6) + Q`, and
500 random valid lifts (2000 with `NRAND=2000 SEED=9`): 1 reported difference, the expected one
of section "Attacked without result". Every OK was canonical, changed the destination, enclosed
the exact decimal interval of the text, printed a text that my printer reproduces byte for byte,
and printed a text that my reader reads back to a ball containing the stored one. Every non-OK
left the representation unchanged (`adf_qclass_identical` against a copy taken before the call).

Union form: `UNSUPPORTED` only after the byte, grammar, exponent and count checks, with texts that
break two rules at once: a trailing comma with `max_items = 1` is `PARSE`; a text over `max_len`
is `LIMIT`; a decimal exponent over `max_exp10` is `LIMIT`; a union of 1000 entries with
`max_items = 999` is `LIMIT`, with `max_items = 1000` it is `UNSUPPORTED`, and 1001 entries with
`max_items = 1000` is `LIMIT`; `union((0.5 ; 1/0)) + Q` is `UNSUPPORTED` (stage 6 is value
semantics, after the refusal). `union()` is `PARSE`.

Union counting over the item limit, checked directly:

    entries 1000 max_items 1048576 -> UNSUPPORTED
    entries 1000 max_items 999    -> LIMIT
    entries 1000 max_items 1000   -> UNSUPPORTED
    entries 1001 max_items 1000   -> LIMIT
    entries 3    max_items 2      -> LIMIT
    entries 3    max_items 3      -> UNSUPPORTED

Limits of 8.4 at the boundary: `(1e100000 ; 0) + Q` is `OK`, `(1e100001 ; 0) + Q)` and
`(1e-100001 ; 0) + Q` are `LIMIT`, `(0e100001 ; 0) + Q` is `LIMIT`, a 19-digit exponent is
`LIMIT`, 19 digits of leading zeros then 5 is `OK`, `max_exp10 = 10^7` admits `1e9999999` and
`10^6` does not. `prec > ADF_REAL_PREC_MAX` gives `LIMIT` before the byte check, also for a text
with a forbidden byte. An embedded NUL is `PARSE`, every byte outside `0x20..0x7E`, TAB, LF, CR
is `PARSE`.

Printer of the PIECES form, on hand-built canonical values (`PIECES 4` and `PIECES 1` of the
harness): `NULL` with length 0 in both cases, as the slice contract states. `NULL` is also returned
for a lift whose stored ball has a binary exponent above `ADF_PRINT_EXP_MAX`, and not otherwise
(checked on every OK of the table and of the random lifts).

Sanitised run of the same driver of the reader and the printer:

    TEXT_EVAL=./lanes/q-review2/text_eval_san ASAN_OPTIONS=detect_leaks=1 NRAND=200 \
        timeout 170 python3 lanes/q-review2/text_oracle.py
    -> no ASAN report, 1 expected difference

### Line 3: the represented set

    timeout 170 python3 lanes/q-review2/member_oracle.py <seed> <nlifts>

| seed | lifts | points each | membership decisions | problems |
|---:|---:|---:|---:|---:|
| 3 | 2000 | 200 | 1200000 | 0 |
| 31 | 300 | 200 | 180000 | 0 |
| 37 | 300 | 200 | 180000 | 0 |

Every lift was built with the source adele cleared immediately after `set_adele` (the lifetime
promise of the header), and the stored piece was compared field by field with the input: same
midpoint, radius, `A`, `H`, `d`, same backend. `add_rat` by rationals of 1 to 2000 bits left the
representation identical and the membership answers unchanged. `set_rat` of 3/2, of 7/3 and of a
30-digit rational gave the zero class `(0 ; 0)` in every case, and its membership answers agreed
with the model of the class of `(0 ; 0)`. `set(x,x)`, `swap(x,x)` and `add_rat(x,x,q)` left the
value unchanged. `get_piece` at `-1`, `1`, `len`, `2^62`, `LONG_MAX` and `LONG_MIN` returned
`DOMAIN` and left the destination at its recognisable content; `get_piece` at `0` returned the
stored piece.

Borrowed contexts, separately (`borrow.c`): a local finite part with the blocks (2,3) survives
`set_adele` after the source is cleared and survives `set`, with the same context pointer and the
same residues; the value is canonical and identical.

    timeout 60 ./lanes/q-review2/borrow                      -> problems 0
    ASAN_OPTIONS=detect_leaks=1 ./lanes/q-review2/borrow_san -> problems 0, no report
    valgrind --leak-check=full ./lanes/q-review2/borrow      -> exit 0

### Line 4: memory

    timeout 120 ./lanes/q-review2/cycle                              -> cycles 1000, problems 0
    ASAN_OPTIONS=detect_leaks=1 timeout 170 ./lanes/q-review2/cycle_san
    -> cycles 1000, problems 0, no report (LeakSanitizer runs here)
    timeout 170 valgrind --quiet --error-exitcode=77 --leak-check=full ./lanes/q-review2/cycle
    -> exit 0, no record of any kind

The cycles build hand-built PIECES values of 1 to 12 pieces with global finite balls and with a
local finite ball in one borrowed context, then `is_canonical`, `set`, `identical`, `swap`, and
free the context after both classes are cleared.

Valgrind on the three driver programs of lines 1 to 3 (`can_eval`, `text_eval`, `member_eval`):
`definitely lost 0 bytes, indirectly lost 0 bytes` in each. The "possibly lost" records Valgrind
reports for `text_eval` and `member_eval` are GMP allocations still live when the harness process
exits (traces at `main (member_eval.c:119)`, `fmpq_set_str` in my harness, and
`tx_arb_set_ball (text.c:848)` reached from `adf_qclass_set_str`); the dedicated lifecycle program
`cycle` reports no record of any kind, and LeakSanitizer reports nothing.

### Line 5: the driver

    timeout 120 ./lanes/q-review2/adf < tests/driver/qclass-lift.cmd
    -> 19 lines, exit 1 as the fixture metadata requires, byte-identical to tests/driver/qclass-lift.out

72 lines in `driver60.cmd` (48 of them `show` or `type` on malformed or edge texts, 24 on
`qadd_rat`, the pair commands, unknown commands and missing arguments): 72 output lines, one per
input line, no crash, no line without output. Every status is the one the library's reader gives
for the same text, and every value printed by `show` is the string `adf_qclass_get_str` returns
at the driver's default of 20 digits, checked on eight texts. `qadd_rat` with a non-class first
operand, with a non-rational second operand and with a malformed class gives `DOMAIN`; missing
`with`, a missing operand and `qadd_rat` alone give `PARSE`; the eight pair commands with a class
operand give `DOMAIN`; `show` of a value whose stored ball is above `ADF_PRINT_EXP_MAX` gives
`error: LIMIT`, which is the driver's general answer for an unprintable value
(`show (1e100000 ; 0)`, an adele, gives the same).

### Line 6: planted faults

    timeout 170 python3 lanes/q-review2/mutate.py

Each fault is a scratch copy under `lanes/q-review2/mut/`, compiled with the repository's flags,
linked with the mutated object ahead of the archive and run as `tests/test_qclass.c`
(baseline: `test_qclass: 35384 checks; group all`, exit 0).

Each fault, its file, the verdict of `tests/test_qclass.c` and the smallest input that shows it:

- F1, qclass.c, the midpoint test `< 0` made `<= 0`: FAIL. PIECES of one piece, mid `0`, rad `0`,
  fin `(0,0,1)`: C 0, the invariant says 1.
- F2, qclass.c, the order compared on the first key only: FAIL. Two pieces with equal `lo` and
  `hi_A < hi_B`: C 0, the invariant says 1.
- F3, qclass.c, the duplicate test dropped (`c >= 0` made `c > 0`): FAIL. Two equal pieces
  `(1/2 ; 0)` with fin `(0,0,1)`: C 1, the invariant says 0.
- F4, qclass.c, `set` copies `len - 1` pieces: FAIL. A PIECES of two pieces: the last array entry
  is never initialised.
- F5, qclass.c, `add_rat` translates the real part only: FAIL. `add_rat` by `1/3` of the lift of
  `(1/2 ; 0)`: the real part moves and the represented set changes.
- F6, qclass.c, `set_rat` gives the lift of a non-integer: FAIL. `set_rat(1/3)`: C `(1/3 ; 0)`, the
  contract says the zero class `(0 ; 0)`.
- F7, text.c, the union refused before the syntax check: FAIL. `union() + Q`: C `UNSUPPORTED`,
  stage 3 of conventions 8.5 says `PARSE`.
- F8, text.c, the printer omits `" + Q"` when the radius is zero: FAIL. `get_str` of the lift of
  `(1/2 ; 0)`: `(1/2 ; 0)`.
- F9, qclass.c, `get_piece` accepts `i = len`: FAIL. `get_piece(a, x, 1)` on a LIFT: C `OK`, one
  entry read past the array.
- F10, qclass.c, `swap` exchanges two of the three fields: FAIL. `swap` of a LIFT and a PIECES: the
  array pointers are not exchanged.
- F11, qclass.c, the `d = 1` clause dropped: FAIL. Local blocks (243, 2^32) with `d = 2`: C 1, the
  invariant says 0.
- F12, qclass.c, the `H >= 0` clause dropped: **PASS**. No input distinguishes it; see the finding.
- F13, qclass.c, the centre range `0 <= A < H` dropped: **PASS**. No input distinguishes it; see
  the finding.
- F14, qclass.c, the upper end compared before the lower end: FAIL. Two pieces with equal `lo` and
  `hi_A > hi_B`.
- F15, qclass.c, the midpoint test accepts `1 + 2^-60`: FAIL. PIECES of one piece, mid `1 + 2^-60`.
- F16, qclass.c, `identical` compares the first piece only: FAIL. Two PIECES of two pieces that
  differ in the second.
- F17, qclass.c, `set_adele` shares the member: FAIL. Clear the source: the class is destroyed.
- F18, text.c, the reader drops the stage 6 finite check: FAIL. `(0.5 ; 1/0) + Q`: a division by
  zero inside the reader.
- F19, text.c, the union count check after the refusal: FAIL. A union of three with `max_items = 2`:
  C `UNSUPPORTED`, stage 4 says `LIMIT`.
- F20, text.c, the printer never writes `" + Q"`: FAIL. Every lift.

The two survivors were then run against my own oracle:

    CAN_EVAL=./lanes/q-review2/mut/can_F12 timeout 170 python3 lanes/q-review2/can_oracle.py 11 20000
    -> cases 20000 mismatches 0
    CAN_EVAL=./lanes/q-review2/mut/can_F13 timeout 170 python3 lanes/q-review2/can_oracle.py 11 20000
    -> cases 20000 mismatches 0
    CAN_EVAL=./lanes/q-review2/mut/can_F11 timeout 170 python3 lanes/q-review2/can_oracle.py 11 20000
    -> cases 20000 mismatches 2666

## Findings

### MINOR 1: two clauses of `adf_qclass_is_canonical` cannot fail, and the suite cannot kill them

`src/qclass.c:95-101` checks, after `adf_adele_is_canonical(a)` has passed:

    if (!fmpz_is_one(d) || fmpz_sgn(H) < 0 ||
        (fmpz_sgn(H) > 0 && (fmpz_sgn(A) < 0 || fmpz_cmp(A, H) >= 0))) { ok = 0; break; }

where `(A, H, d)` is the canonical global triple of the finite part. The sentence that decides:
`adf_adele_is_canonical` is `arb_is_finite(inf)` and `adf_fball_is_canonical` (`src/adele.c:109-112`),
and the predicate it applies is `G` for a global value, which says `d > 0`, `H >= 0` and
`0 <= A < H` when `H > 0` (docs/conventions.md 5.2, `include/adelefeld/fball.h:57-61`), and `L`
for a local value, which says `H = K` (a product of blocks, so `H >= 2`), `A = 0`, `d >= 1`
(docs/conventions.md 5.3). The canonical triple of a global value is the stored triple, and that of
a local value is `(A0/g, K/g, d/g)` with `0 <= A0 < K` (docs/conventions.md 5.3, policies
Lemma 17.3 and Lemma 18). Hence for every value that reaches the clause, `H >= 0` and
`0 <= A < H` for `H > 0` already hold, and dropping them changes no verdict. Only `d = 1` is a
live clause, because `d/g > 1` is possible for a local value.

Input that shows it: none. Two mutants (F12, F13) pass `tests/test_qclass.c` with its 35384 checks
and pass my oracle on 20000 values. The defect, if it is one, is dead code in the predicate and a
missing test of a clause that cannot be violated; it cannot make `is_canonical` wrong.

Reproduce:

    timeout 170 python3 lanes/q-review2/mutate.py | tail -3
    CAN_EVAL=./lanes/q-review2/mut/can_F13 timeout 170 python3 lanes/q-review2/can_oracle.py 11 20000

### MINOR 2: the reference printer does not finish on a large decimal exponent

`proto/text_grammar.py` reads `(1e9999999 ; 0) + Q` (with `max_exp10` raised to `10^7`) only by
exhausting the time: `_decimal_parts` (proto/text_grammar.py:132) loops on `10^(2k)` for `k` of
the order of the exponent. The C reader returns `OK` for that text in 4 ms. Because of this the
three-way comparison was run on 64 of the 72 table entries; the eight skipped texts are marked
`SKIPPED` in the output of `text_oracle.py`. This is a limitation of the reference, not of the
slice; it is recorded because it will bite the next differential run that uses large exponents.

Reproduce: `timeout 20 python3 -c "import sys; sys.path.insert(0,'proto'); import text_grammar
as t; print(t.canonical('qclass','(1e9999999 ; 0) + Q', t.Limits(max_exp10=10**7)))"` does not
return in 20 s.

## Attacked without result

With the counts, and what would have made a case fail.

1. `is_canonical` on 80000 values (four seeds) with one clause broken at a time: 0 disagreements.
   What would have made a case fail: a wrong sign in the four-term `arf_sum` at an exponent gap,
   which I looked for separately with `probe_arf.c` (the sign of `1 - 1 + 2^-1000 + 0`, of
   `t - t + t/2` at exponents `+-2^60`, of a three-term cancellation, all correct) and with
   `probe_endcmp.c` and the 10^6-bit-mantissa hand cases. A value whose keys are equal but whose
   members differ in backend is rejected, as the invariant requires.
2. The lift text on 72 hand-specified texts, 70 cuts and 2000 random lifts: 1 difference, the
   expected one. `proto/text_grammar.py` and the C differ on the lift of
   `(1.5 +/- 1e-5 ; 5/3 mod 6) + Q`: the reference prints `(1.5 +/- 1e-5 ; 5/3 mod 6) + Q`, the C
   prints `(1.5 +/- 1.1e-5 ; 5/3 mod 6) + Q`. This is conventions 9.6 ("the print-read-print
   fixed-point statement applies only to the exact-rational reference parser"; a C value-text round
   trip may widen) and is recorded in the lane's own report; the C text still encloses the stored
   ball, and rereading it gives a class that contains the first. The reference accepts the union
   form and the C refuses it with `UNSUPPORTED`; that is the slice design (api-3.md 2.5) and not a
   disagreement of the contract. The (C, reference) status pairs seen are listed in the log of
   `text_oracle.py`.
3. The represented set on 2600 lifts and 1560000 membership decisions: 0 problems. What would
   have made a case fail: an `add_rat` that translated one coordinate, a `set_rat` that returned
   the lift of its argument, an aliasing bug in `set` or `swap`, a `get_piece` that wrote on
   `DOMAIN`, or a `set_adele` that shared the member. Each of these is a fault that the mutation
   table shows the repository suite kills.
4. Memory: no finding. What would have made a case fail: a leaked array in `set`, a leak of the
   residue array in a copied local value, a double free in `clear` on a swapped class, or a read
   past the array in `clear` with `len` forged negative. The last is what F9 approaches.
5. The driver: no finding on 72 lines. What would have made a case fail: a `qclass` operand
   accepted by a pair command, a status changed for a malformed text, or a printed value that the
   library's printer does not produce.

## What is not done

- No overnight differential fuzz run. The runs above are bounded and their sizes are stated.
- No check of the dump form, of `adf_qclass_set_pieces`, `reduce`, `equal_set`, `contains`,
  `overlaps`, `neg`, `add` or `psi`: none of them exists in this slice.
- No check of the Julia test `tests/julia/qclass.jl` (out of the C contract under review here).
- No mutation sweep of `tools/adf/adf.c`.
- The mutation table uses `tests/test_qclass.c` as the detector, not the repository's mutation
  tool `tools/mutate/mutate.py`; the ten faults of the brief and ten more are mine, and I did not
  check them against the equivalence list `tools/mutate/equivalent.txt` (not owned).
- My oracles share one assumption with the implementation: that the exact dyadic midpoint and
  radius of an `arb` are what `arb_midref` and `arb_radref` hold, which no arb operation in these
  paths changes.

## Sources pending

- `[source pending: the semantics of FLINT's arf_sum at a large exponent spread]`. The exact
  wording is `refs/src/flint-3.0.1/arf.rst:638-645` ("the sum is computed as if done without any
  intermediate rounding error, with only a single rounding applied to the final result ... does not
  overflow if the magnitudes of the terms are far apart"), and the claim that a two-bit rounding
  of a nonzero sum keeps its sign follows from the exponent being unbounded; the FLINT source of
  `arf_sum` is not on disk (`refs/src/flint-src-3.0.1/` has no `arf/`), so the claim was checked by
  measurement (`probe_arf.c`, the hand cases) and not by reading the code.
- `[source pending: whether mag_set_ui_2exp_si(v, m, e) means m*2^e]`. The header marks it "TODO:
  test functions below" (`/usr/include/flint/mag.h:615`). I took the value to be
  `man * 2^(exp - MAG_BITS)`, verified by `probe_mag.c` against `mag_one` and `mag_get_d`; every
  radius of my harness is set with that function and read back through `arf_set_mag`.

## Findings against the specification

None found in this slice. The two points below are known and were already recorded by the lane.

- `docs/api-3.md` 2.5 and section 5 fix a point in conventions 9.6: at 128 bits and six digits
  `(3.14159 +/- 1e-5 ; 5/3 mod 6) + Q` prints radius `1.1e-5`, and rereading and printing gives
  `1.2e-5`. The C contract is enclosure on reread, which my checks confirm; the wording of the
  design is not the contract of the C.
- SPEC 6's full-image sentence needs a positive-radius qualification, as the lane's report says;
  nothing in this slice uses a full-image shortcut.
