# Lane m3-review1: adversarial review of milestone 3, work packages 3.1 and 3.2

Recorded by the orchestrator from `lanes/m3-review1/result.md` (Claude Opus, 31 min, 326k tokens). The three MINOR findings were repaired on master right after the review (the cardinal shortcut guarded by canonicity with a regression check in `tests/test_psi.c`; the stale text of `text.h` and the driver README; the statement numbers of `docs/api-3a.md`).

Claude Opus, 2026-10-08, worktree `/home/tobias/Projects/adelefeld-wt/m3-review1` at `2903820`. No file
outside `lanes/m3-review1/` was changed. No BLOCKER and no MAJOR finding. There are three MINOR findings.

## Findings

### m1 (MINOR). In the normal build, `adf_phase_get_acb` returns the wrong value for theta outside `[0,1)` or not reduced, but only when the denominator is 4 or 2/4

- Input: `theta = 5/4`, `-3/4` or `9/4` (canonical fmpq, outside `[0,1)`), and `theta = 2/4` (not reduced), at
  `prec 53`.
- Returned: `ADF_OK` with `-i` in all four cases.
- True value: `E(5/4) = E(-3/4) = E(9/4) = +i` and `E(2/4) = -1`. Other theta outside the range get the
  periodic value: `4/3` gives `E(1/3)`, `5/2` gives `-1`, `-1/4` gives `-i`. The cause is the cardinal shortcut
  at `src/psi.c:154-157`, which tests `num == 1` for denominator 4 without reducing.
- Sentence: `include/adelefeld/psi.h:36-37`: "Noncanonical or out-of-range theta violates the precondition
  (checked under INV)." This is a precondition violation, and the INV build aborts. It is reported because the
  normal build is right for every other out-of-range theta and wrong only here, and because callers through
  ccall (Julia) do not get the INV check.
- Reproduce:
  `timeout 600 make -j2 BUILD=lanes/m3-review1/build lanes/m3-review1/build/libadelefeld.a &&
  gcc -std=c11 -O1 -Iinclude lanes/m3-review1/ph.c lanes/m3-review1/build/libadelefeld.a -lflint -lgmp -lm
  -o lanes/m3-review1/ph && timeout 10 lanes/m3-review1/ph`

### m2 (MINOR). The public contract text is stale and contradicts the behaviour that shipped in slice 3.1-d

- `include/adelefeld/text.h:185` ("Union syntax gives ADF_UNSUPPORTED after byte, grammar, exponent and count
  checks ...") and `:198` ("The union reader remains UNSUPPORTED until slice 3.1-d."). Since slice 3.1-d,
  `adf_qclass_set_str` reads union text as PIECES. I read 5500 valid union texts and all returned `OK`
  (hunt 3).
- `tools/adf/README.md:735`: "Pair arithmetic and set queries give `DOMAIN` in this slice." The driver answers
  `qequal`, `qcontains`, `qoverlaps`, `qadd` and `qneg` (hunt 6).
- `tools/adf/README.md:786-787`: "the union text ... is read once slice 3.1-d lands (until then the reader
  answers `UNSUPPORTED`)". The driver reads union text now: `psi_strict union((0 ; 0 mod 1), (0.5 ; 0 mod 1))
  + Q` prints `(0 +/- 1.1) + (0)*i`.

### m3 (MINOR). Statement numbers in `docs/api-3a.md` are used twice

Slice 3.1-e numbers its statements 19 to 21 (`docs/api-3a.md:196-198`). Slice 3.1-d numbers its statements 19
to 24 (`:270-324`), and slice 3.1-f numbers its statements 22 and 23 (`:372,377`). So a reference such as
"statement 19" or "statement 22" is ambiguous.

## What was attacked without result

Every check uses my own exact model in exact rationals. Nothing is imported from `proto/` or from the lanes'
vectors. `qr3_model.py` is a copy of `lanes/q-review3/model.py` (R and Q1 only). The sign comes from
`refs/src/tate-poonen/notes.txt:693-700`: `psi_R(x) = e^(-2 pi i x)`; at `Q_p` the composite
`Q_p -> Q_p/Z_p ~ Z[1/p]/Z -> R/Z -> T` with `psi(1/p^n) = e^(2 pi i/p^n)`. Hence `psi(0 ; 1/3) = E(1/3)`, and
the C code returns `-1/2 + 0.866... i`. The harness is `h.c`. Builds: `build/` (normal) and `build-san/`
(`SAN=1 INV=1`), both rc 0, both deleted at the end.

1. **Set queries** (`sets_check.py`, seeds 1-7 plus 21-25 under the sanitizers). This was 15000 pairs × 3 queries.
   The decision does not use algorithm R. Membership of `(s, z)` is decided per residue `rho mod L` and per
   integer point `e`, from `N Zhat ∩ Q = N Z`, and every closed-interval endpoint in `[0,1)` and every gap
   midpoint is tested. Witnesses are re-checked through a second membership formula (`common.member`). There
   are 30 random rational points per pair.
   - Pairs were LIFT and PIECES. Moduli were 2-12, 360, 7 and 11. Fractional radii were 1/2, 2/3, 3/2, 5/6 and
     4/3. End points were at integers, half-integers and quarters. Spill was included. Some centres had 2000 bits.
   - Related pairs were rational translates (equal sets), exact R pieces (equal sets), shrunk pieces, modulus
     multiples and an added piece.
   - Results without LIMIT: equality true about 3100 times, containment true about 4800 times, overlap true
     about 9300 times.
   - Budget: `W` was set to `(2E+1)KL` exactly, to one below that, and to `K`, `K-1`, `L` and `L-1`, with `K`,
     `L` and `E` from my own R count (E counts distinct end points of the constructed pieces). LIMIT appeared
     exactly when expected, with `truth` untouched. There were 0 findings.
   - What would have failed a case: a wrong truth, LIMIT within the budget or OK beyond it, or `truth` written
     on LIMIT.
   - `edge_sets.py`: `N = 1/10^100` and width `2^1001` gave LIMIT. Those nine calls took 2.8 ms in total.
     `W = LONG_MAX` was handled.
   - Glued family `[1/2,1] x (c mod H)` against `[0,1/2] x (c-1 mod H)` and `x (c-2 mod H)`, for H = 2 to 8
     and every centre: 70/70 answers correct.

2. **Arithmetic** (`arith_check.py`, seeds 2-7 plus 21-25 under the sanitizers). This was 8300 pairs, each
   with `qadd`, `qneg`, the `z=x` alias and `z=x=y`.
   - On every OK, every stored piece list is bit-identical to my construction. That construction pairs the
     stored entries, takes `[lo1+lo2, hi1+hi2] x ((a+b) + gcd(N,M) Zhat)` with my own rational gcd (negation:
     `[-hi,-lo] x (-a + N Zhat)`), then applies R and the Q1 kernel. This covers the 30-bit successor and
     sorting and deduplication by stored keys.
   - LIMIT occurred exactly when the raw count was above `lim` or `lim < 1`, with the output untouched.
   - About 1.1M membership points (`u+v` and `-u` in some stored piece, exact A/Q membership) all held.
   - The independent sum `x + x` was included in 15% of the pairs.
   - There were 0 findings. A case would have failed on a missing point, a radius excess beyond Q1, a count off
     by one, or an alias mismatch.

3. **Character** (`psi_check.py`, seeds 1-6, 11 and 21-24). This was 12000 cases.
   - Adeles had real parts 0, integers and dyadics with exponents from -200 to 200. Real radii were 0, tiny,
     moderate and large. Finite denominators were 1, 2, 3, 4, 6, 8, 12, 360, prime powers up to `2^20`, and
     `2^64+13` and `10^30` times small numbers. Numerators were negative and up to 2000 bits. Finite radii were
     integral or fractional (B up to 360, plus 2^64+13 and 3^50). Local-backend pieces used blocks 8, 9, 5.
     PIECES classes had 1 to 4 entries.
   - About 6.8M exact point phases were checked against the returned rectangle with mpmath at 130 digits. The
     points were end points and interior points, every root class for `B <= 400`, and the points whose angle is
     exactly 0, 1/4, 1/2 or 3/4. My own `fp_p` checked triviality on `Q` for every sampled point.
   - About 33000 hull checks compared against an enumeration of the arcs (not the Q4 formula), with the bound
     `4*2^-p + 2^-28 (W/2 + 2*2^-p)` per end point.
   - Strict gave `NOT_DETERMINED` with the output untouched exactly for fractional finite radius. The exact
     getters (class, adele, fball) were correct, including two entries with different phases.
   - There were 0 findings.
   - `psi_local.py`, seeds 1-5 and 21-22: 7300 lballs (p = 2, 3, 5, 7, 11, `2^61-1`, and the primes
     `2^64-59` and `2^64-83`; e from -6 to 6; exact and ball); 7300 `psi_at` and `psi_strict_at` calls (`where`
     untouched on OK, equal to v otherwise); 1825 products over places containing `E(c - m)` (interval product
     in mpmath). There were 0 findings.
   - E3: the lift gives strict `NOT_DETERMINED` with the output untouched. The default gives real
     `[-1.0000000019, 1.0000000019]` and imaginary part exactly 0. The two-piece reduction gives strict `OK`
     with the same box.
   - Resources (`res.c`, timed): a finite denominator of about 20130 bits takes 1 ms (default `OK`, strict
     `NOT_DETERMINED`). A real exponent above `ADF_QCLASS_EXP_MAX` gives `LIMIT` from `psi`, the phase getter,
     `equal_set` and `neg`, each in under 1 ms. `prec` at the cap on `(0 ; 1/3)` gives `NOT_DETERMINED` in
     13 ms, `prec` cap+1 gives `LIMIT` first, and phase `7/10^6` at the cap gives `NOT_DETERMINED` in 1.8 s.
     The cardinal phases are exact at prec 1, 2, 53 and the cap.

4. **Union reader and constructor** (`uni_check.py`, seeds 1-11 and 21-25). This was 10500 texts from my own
   printer (9.2 grammar, 9.4 templates), with 30% exact duplicates, decimals up to 60 digits and exponents to
   1e-40.
   - Every text was `OK` and canonical. Every text piece is enclosed by a stored piece with the same canonical
     `(A mod H, H)`, and the stored length is at most the number of distinct pieces.
   - `max_items` equal to the count gave `OK`. One below the count gave `LIMIT` with the class untouched.
   - Rereading `get_str` encloses every stored piece.
   - Stage order: PARSE beats LIMIT, LIMIT beats DOMAIN, and an exponent LIMIT beats DOMAIN. `1/2 mod 1/2`
     and a centre `1/2` give DOMAIN.
   - `sp.c`: duplicates count before deduplication (`n=2, limit=1` gives LIMIT). The first occurrence's backend
     is kept (global first gives global, local first gives local). `piece_limit 0` with `NULL` gives LIMIT,
     `n 0` and `n -3` give DOMAIN, non-finite mid, radius or NaN gives DOMAIN, midpoint `1+2^-200` gives DOMAIN,
     spill is OK, and a raw non-canonical triple gives DOMAIN. All failures leave the class untouched.
   - There were 0 findings.

5. **Dumps** (`dump_check.py`, seeds 1-7 and 21-25). This was 12000 classes: LIFT (exponents to ±3000) and
   PIECES (1-7 entries; global, ctx 8·9·5 and ctx 7·11·16 mixed).
   - dump then load gives an identical class, the re-dump is byte-identical, and `inspect` counts the local
     occurrences.
   - About 35000 mutants were tried: a letter in the count, count 0, count+1, two pieces swapped, a duplicated
     piece, truncation and random byte edits. Every one meant to be invalid was refused. Every edit that
     loaded `OK` re-dumped byte-identically. Every failure left the class untouched.
   - `fmpz_set_str` was interposed and `arb_load_str`, `arf_load_str` and `mag_load_str` were stubbed: there
     were 0 calls on any PARSE or LIMIT text and 0 calls to the arb/arf/mag loaders at all. `inspect` agreed
     with `load` on every non-binding status.
   - M1-D9: a PIECES exponent of `2^20` gives `OK`, `2^20+1` gives LIMIT (mid or radius), and LIFT is
     unrestricted.
   - Binding counts 0, 1 and 2 against one occurrence, and a wrong context, give DOMAIN. Zero-arch gives DOMAIN.
   - There were 0 findings.

6. **Memory.** The sanitized INV harness was run with `ASAN_OPTIONS=detect_leaks=1` (LeakSanitizer
   confirmed working with a planted leak). The counts per hunt are listed in items 1-5. The lanes'
   `test_psi`, `test_psi_class`, `test_psi_local`, `test_qclass_arith`, `test_qclass_dump`, `test_qclass_sets`
   and `test_qclass_text` all exit with rc 0 under LSan, with no leak and no ASan or UBSan report. The
   sanitized driver ran the 61-line script with no report. There were 0 findings.

7. **Driver** (`drv.txt`: 61 lines, expected values derived by hand first).
   - 59 lines equal my hand values. They cover E3 equality and its budget 20 against 19, containment both
     ways, overlap through a rational translate, the glued `(1 ; 0 mod 3)` against `(0 ; 2 mod 3)`, a union
     with duplicate pieces, other kinds giving DOMAIN, limits 0, 1/2 and `abc`, `qneg` and `qadd` with exact
     printed results, `x+x` of `(0.5 +/- 0.25 ; 0)`, the `psi` cardinal phases, `psi_phase` of
     `union((0 ; 5), (1 ; 0))` (= 0), `fp_2` and `fp_3` on lballs, `psi_at` at real, 2, 3 and `2^64-59`, and
     print, dump and load including a dump out of order (DOMAIN).
   - The two other lines differ only from guesses of mine: `with 2.5` is `PARSE`, not DOMAIN, since a decimal
     is not a rational token; and the printed radius at 5 digits includes the decimal rounding of the midpoint.
     For the second line, the stored radius from the harness is `7.7e-40` real and `4e-58` imaginary, within
     the bound `1.2e-38`.
   - The driver prints the library's values.
   - `tests/julia/psi.jl` was read, not run: it tests the status before the output and clears every object in
     `finally`.

## Fault table (hunt 7)

`faults.py` plants each fault in a scratch copy and builds it into a copy of the normal archive. A test that
`#include`s `../src/X.c` (`test_qclass_arith`, `test_qclass_sets`) is compiled from a scratch copy that
includes the faulty source. Before any fault, all seven tests pass on the unmodified archive.

| Id | File | Fault | Lanes' tests | My checker |
|---|---|---|---|---|
| F1 | psi.c | real sign reversed, `E(a+m)` | test_psi, test_psi_class: detected | 850 findings |
| F2 | psi.c | Q4 distance ignores real radius r | test_psi, test_psi_class: detected | 1592 |
| F3 | psi.c | class hull from last entry only | test_psi_class: detected | 436 |
| F4 | psi.c | class strict passes a fractional LIFT | test_psi_class: detected | 81 |
| F5 | psi.c | `psi_at` infinity `E(+m)` | test_psi_local: detected | 122 |
| F6 | psi.c | `fp_p` without the inverse of the prime-to-p denominator | test_psi_local: detected | 167 |
| F7 | qclass_sets.c | glue `(1,m)~(0,m-1)` dropped for cosets | test_qclass_sets: detected | 3 |
| F8 | qclass_sets.c | budget `E K L` instead of `(2E+1) K L` | test_qclass_sets: detected | 387 |
| F9 | qclass_arith.c | sum radius N1 instead of gcd(N1,N2) | test_qclass_arith: detected | 231 |
| F10 | qclass_arith.c | negation keeps the finite centre | test_qclass_arith: detected | 377 |
| F11 | qclass.c | set_pieces keeps the last of equal keys | test_qclass_text: detected | (not run) |
| F12 | dump.c | stage-6 validator accepts equal adjacent keys | test_qclass_dump: detected | 0 (load's final `is_canonical` still refuses) |
| F13 | qclass_sets.c | inclusion ignores residue 0 | test_qclass_sets: detected | 23 |
| F14 | psi.c | hull radius RU30 without the successor | test_psi, test_psi_class: detected | 0 (encloses) |
| F15 | psi.c | cosine certificate `2^(13-p)` instead of `2^-p` | all three psi tests: detected | 0 |
| F16 | qclass_arith.c | Q1 midpoint rounded down | test_qclass_arith: detected | 164 |

All 16 faults are detected by the lanes' tests. The lanes' tests passed no fault that loses enclosure,
changes a truth or a sign, or changes a status.

## The statements (hunt 8)

For the "Check:" lines of `docs/api-3a.md` (slices 3.1-d, 3.1-e, 3.1-f) and `docs/api-3b.md`, I checked that
every test function they name exists and is called from `main`. These are 22 names in total:
- `test_qclass_text`: `construction`, `vectors`, `local_and_bounds`, `golden`, `limits`;
- `test_qclass_dump`: `golden`, `vectors`, `malformed`, `binding_order`, `random_roundtrips`, `check_dump`;
- `test_qclass_sets`: `exact_and_local`;
- `test_psi`: `golden`, `vectors`, `exact_and_additivity`, `edges`, `hullcheck`;
- `test_psi_class`: `vectors`, `statuses`, `e3`;
- `test_psi_local`: `local_vectors`, `place_vectors`.

The counts the tests print agree with the statements: 2000 random round trips, 32 class records, 432 local
records and 576 place evaluations. I did not find a statement whose check is missing, beyond the stale
header and README text of m2.

## Not done

- Julia tests were not run (only read).
- I did not plant faults in `text.c` (the union reader).
- I did not verify the numeric claims of each statement beyond the printed counts.
- No allocation-failure injection.

## Files

All files are in `lanes/m3-review1/`:
- harness: `h.c`;
- generators and checkers: `common.py`, `qr3_model.py`, `sets_check.py`, `edge_sets.py`, `arith_check.py`,
  `psi_check.py`, `psi_local.py`, `uni_check.py`, `dump_check.py`, `faults.py`;
- probes: `sp.c`, `res.c`, `ph.c`;
- driver script with hand expectations: `drv.txt`;
- notes: `progress.md`; this report: `result.md`.

Build trees, executables and run logs were deleted. No files were written to `/tmp`.

Rebuild for any rerun:

    timeout 600 make -j2 BUILD=lanes/m3-review1/build lanes/m3-review1/build/libadelefeld.a
    gcc -std=c11 -O1 -g -Iinclude lanes/m3-review1/h.c lanes/m3-review1/build/libadelefeld.a \
        -lflint -lgmp -lm -ldl -o lanes/m3-review1/h

Then run, for example, `cd lanes/m3-review1 && timeout 170 python3 sets_check.py 1 1500`.
