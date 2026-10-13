# Lane t5-slice5: report (slice 5e, the functional equation computed by the caller)

Status: all five deliverables are done, end to end. Worktree `/home/user/adelefeld-wt/t5-slice5`, branch
`lane/t5-slice5`, nothing committed. There is no library change, no new C type, and no HEADER-FINDING.

## Files

New:
- `tests/test_tate_fe.c`
- `tests/ref/vectors/t5-slice5/fe.jsonl` (153 records, 114 KB)
- `tests/driver/tate-fe.cmd`, `tests/driver/tate-fe.out` (15 lines)
- `tests/julia/tate_fe.jl`
- Lane directory: `gen_vectors.py`, `derive_fixture.py`, `faults.py`, `faults.out`, `acceptance.txt`,
  `redgreen.md`, this file.

Changed:
- `tools/adf/adf.c`: the command `tate_fe` (`adf_drv_tate_fe`, one enum entry, one table entry, one dispatch line).
- `tools/adf/README.md`: one section at the end.
- `tests/test_julia.sh`: one block after the `tate_continue` block.
- `docs/api-5b.md`: "Slice 5e" appended.

Build trees, scratch copies and the root `build/` are removed.

## What was done

**1. The command.** `adf tate_fe CHI with S with BITS` uses the grammar and operand checks of `tate_completed`,
with `prec` from the driver setting. It runs these steps in order:
- `A = adf_tate_completed(chi, s, BITS)`;
- `B = adf_tate_completed(conj(chi), 1 - s, BITS + 4)`;
- `W = adf_char_root_number(chi)` at `wp = max(prec, BITS + m + 16)`, where `|B| < 2^m`;
- `R = W B` at `wp`, after which each coordinate diameter of `R` is checked to be at most `2^-BITS`.

It prints `A | R | overlap=yes|no | s=S`. When a step fails, it prints `<A|B|W|R>: error: STATUS` for the first
failing step.

On the widths:
- `A` gets `BITS` itself, because nothing is applied to it after the call.
- `B` gets `BITS + 4`, and `W` gets enough precision that `|B| rad(W) <= 2^-(BITS+18)`.
- My estimate of `R`'s diameter is about `0.09 2^-BITS`. That estimate depends on how `arb_mul` propagates radii,
  which is not on disk (source pending). The run-time check is the guarantee.
- Observed: the largest diameter is `0.094 2^-bits` for `A` and `0.0061 2^-bits` for `R`.

**2. The test** (`tests/test_tate_fe.c`, 19126 checks, about 6 s). `fe_sides` repeats the driver's steps. The
sections:
- `values`: 136 records = 17 golden characters (C = 1 included) x 8 points. The points are `3/8 + 3i/4`, `1/2`,
  `1/2 + 14i`, `-1`, `2 + 3i`, `1/4 + i`, `-e - 2` and `3 + e`. Every record runs at bits 53; C = 1, 4, 5, 15, 16
  also run at 20 and 80. That gives 280 OK cases. In each case:
  - `A` is narrow at `2^-bits`, `R` at `2^-(bits+2)`, `B` at `2^-(bits+4)`;
  - `A` and `R` overlap;
  - each of `A` and `R` contains both the FLINT value and the mpmath value of `Lambda(s, chi)`;
  - `B` contains both references of `Lambda(1 - s, conj chi)`;
  - `W` overlaps the golden `W`.
  That is 1680 containments. 46 records have a trivial zero on one side. Where the zero is on `A`'s side, both `A`
  and `R` exclude 0.
- `balls`: 17 balls of radius `2^-60` at `3/8 + 3i/4`. At bits 40 both sides contain all four corner references.
  At bits 80 the result is NOT_DETERMINED, named `A`, with all four outputs untouched byte for byte.
- `poles`: for C = 1 at exact 0 and 1, `A` is DOMAIN and `B` (at 1 and 0) is DOMAIN too. Balls meeting a pole
  (`1 +/- 2^-10`, the contact `[1 - 2^-9, 1]`, and `1/2 +/- 3/4` at bits `2^21 - 4`) give NOT_DETERMINED on both
  sides. `chi_4` at 0 and at 1 is OK, and both sides contain 1.
- `statuses`: bits -1, `2^21 + 1` and `WORD_MAX` give DOMAIN at `A`. The prec cap and C = 1031 give LIMIT at `A`.
  The outputs are left untouched.
- `hand`: zeta at -1 and at 2 gives `pi/6` on both sides. At 1/2, `W = 1`, `R` contains `B`, and `R != A`.
- `guard`: for zeta on `3/8 +/- 2^-k` at bits 20, 4 radii give `A` OK while `B` is NOT_DETERMINED, and the failure
  is named `B`.
- `independence`: a disjoint sentinel replaces one side's evaluator, and only that side moves (bit for bit). Run
  for 4 characters.
- `widths`: for C = 1, 5, 16 at `1/4 + i`, bits 20, 53 and 80, starting from prec 2, both sides shrink.
- `goldens`: the 17 root numbers overlap the golden `W`.

The raw pair `(8,7)` is run as given. The test checks that it lowers to `(4,3)`, so its rows repeat those of
`(4,3)`; the same holds for `(10,3)`, which lowers to `(5,3)`. No FLINT L function is called in the binary: the
FLINT values come from the vector file.

**Vectors.** `timeout 300 python3 -B lanes/t5-slice5/gen_vectors.py` takes 10 s.
- The FLINT reference is python-flint's L completed by hand (`acb_dirichlet.rst:98-100`, `:524-534`, `:567-569`).
  The mpmath reference is the oracle's Hurwitz L.
- At a trivial zero, both references are computed as `L'(s0)/(rgamma'(z0)/2)`, not through the functional
  equation. FLINT uses `acb_series`; mpmath differentiates the Hurwitz sum and uses `mp.diff`.
- The generator asserts that the two sources agree to 1e-45 relative, and that `|L - W B|` is within the same
  margin.

**3. Fixture and Julia.**
- `tests/driver/tate-fe.cmd` has 15 lines, derived before the run by `derive_fixture.py` from the vectors. For the
  exact-zero imaginary parts it uses the oracle's bound. The lines cover:
  - zeta at 1/2: the real parts print alike, and the imaginary radii are `2.1e-26` (bits 80) and `1.5e-27`
    (bits 84);
  - `chi_4` at `3/8 + 3i/4`, and the raw `(8,7)`, which prints the same line;
  - the non-real characters (5,2), (7,3) and (15,2);
  - `chi_4` at its trivial zero -1;
  - DOMAIN at s = 1 and s = 0, and NOT_DETERMINED for a ball meeting 1;
  - BITS -1 and `2^21 + 1`, a non-integer BITS, C = 1031, and a wrong operand kind.
  The first run gave PARSE on every line. After the code, all lines were equal on the first run.
- `tests/julia/tate_fe.jl` calls `adf_tate_completed` twice, `adf_char_conj` once and `adf_char_root_number` once.
  It checks finiteness, the radius bound `2^-81`, overlap, texts derived before the run, and the pole statuses.
  It passes 20 of 20.

**4. Statements.** "Slice 5e" in `docs/api-5b.md` contains:
- S15: what is computed, and why `A` and `R` must overlap (P13 `analysis.md:574`, step 4 `:613-614`);
- S16: the widths;
- S17: why this is SPEC 8's acceptance;
- S18: the statuses;
- S19: the trivial zeros;
- S20: independence;
- decisions 17 to 24, the fault summary, the cost, and the "Check:" lines.

## Acceptance table (bits 53; per character the largest coordinate diameter over its 8 points)

All 136 rows are in `lanes/t5-slice5/acceptance.txt`; each row was printed only after its checks passed.
`2^-53 = 1.11e-16`.

| raw pair | points | diam A (max) | diam R (max) | A, R overlap | both sides contain FLINT (and mpmath) |
|---|---|---|---|---|---|
| (1, 1) | 8 | 6.15e-18 | 3.78e-19 | 8/8 | 8/8 |
| (3, 2) | 8 | 5.95e-18 | 3.66e-19 | 8/8 | 8/8 |
| (4, 3) | 8 | 6.34e-18 | 3.91e-19 | 8/8 | 8/8 |
| (5, 2) | 8 | 6.01e-18 | 5.29e-19 | 8/8 | 8/8 |
| (5, 3) | 8 | 6.01e-18 | 5.29e-19 | 8/8 | 8/8 |
| (5, 4) | 8 | 8.57e-18 | 3.96e-19 | 8/8 | 8/8 |
| (7, 3) | 8 | 6.04e-18 | 4.67e-19 | 8/8 | 8/8 |
| (7, 6) | 8 | 6.04e-18 | 3.57e-19 | 8/8 | 8/8 |
| (8, 3) | 8 | 9.82e-18 | 4.01e-19 | 8/8 | 8/8 |
| (8, 5) | 8 | 6.07e-18 | 3.95e-19 | 8/8 | 8/8 |
| (8, 7) -> (4, 3) | 8 | 6.34e-18 | 3.91e-19 | 8/8 | 8/8 |
| (12, 11) | 8 | 6.13e-18 | 3.75e-19 | 8/8 | 8/8 |
| (13, 2) | 8 | 6.01e-18 | 5.05e-19 | 8/8 | 8/8 |
| (15, 2) | 8 | 6.20e-18 | 5.36e-19 | 8/8 | 8/8 |
| (16, 3) | 8 | 6.09e-18 | 6.73e-19 | 8/8 | 8/8 |
| (9, 2) | 8 | 6.34e-18 | 5.20e-19 | 8/8 | 8/8 |
| (10, 3) -> (5, 3) | 8 | 6.01e-18 | 5.29e-19 | 8/8 | 8/8 |

At bits 20 and 80 (144 more cases for C = 1, 4, 5, 15, 16), every case is also OK, narrow and overlapping, and
contains both references. Over all 280 cases, the largest diameter relative to `2^-bits` is 0.094 for `A` and
0.0061 for `R`.

Comparison with the oracle group `functional_equation` (`check_poles_fe`, `proto/tate_checks.py:668-696`; I ran
`timeout 170 python3 -B proto/tate_checks.py`: 20 checks in the group, TOTAL 466, exit 0). Its five pairs (1,1),
(4,3), (5,2), (5,4) and (15,2) at `3/8 + 3i/4`, bits 80, are all among my cases at bits 80, with the same outcome:
both sides finite, width at most `2^-80`, overlapping each other and FLINT. The differences:
- the oracle takes `bits + 4` on both sides; I give `A` exactly `bits`;
- I also check mpmath, `B` alone, and `W` against the golden.

## Fault table

`timeout 1500 python3 -B lanes/t5-slice5/faults.py` (20 s; output in `faults.out`). Each fault is planted in a
scratch copy of `fe_sides` and in a scratch copy of `adf_drv_tate_fe`. 6 of 6 are detected by both the test and
the driver fixture.

| Fault (design 7, 5.4) | test, full run (first failure) | section `values` alone | driver fixture |
|---|---|---|---|
| no conjugate (B on chi) | `:474` widths: overlap | `:232` `acb_overlaps(A, R)` | differs, lines 4-6 |
| inverse W (R = B/W) | `:453` independence: `R ~ W * sentinel` | `:232` | differs, lines 4-6 |
| negative tau (R = -W B) | `:360` poles: chi_4 at 1 overlap | `:232` | differs, lines 1-7 |
| incomplete completion (B = I, no C^z') | `:360` | `:232` | differs, lines 2-7 |
| -s instead of 1 - s | `:360` | `:232` | differs, lines 1-7 |
| one side copied (R = A) | `:386` hand: `R != A` at zeta 1/2 | passes (exit 0) | differs, lines 1, 7 |

The copy fault passes every value comparison, because `R = A` is the right number. Only `hand` and `independence`
detect it (`:449`, the sentinel), which is the design's point at `api-5.md:350-352`.

## Checks run (final code)

| Check | Result |
|---|---|
| `timeout 300 lanes/t5-slice5/build/test_tate_fe` (plain) | `tate_fe: 19126 checks`, 6.3 s |
| SAN=1, `ASAN_OPTIONS=detect_leaks=1` (`build-san`) | 19126 checks, 0 sanitizer lines, 7.9 s |
| LeakSanitizer works here | a 77-byte leak program is reported |
| INV=1 (`build-inv`) | 19126 checks, 7.3 s |
| CC=clang (`build-clang`) | 19126 checks, no warning, 7.2 s |
| `clang -Werror -fsyntax-only tools/adf/adf.c` | clean |
| `sh tests/test_driver.sh` | `100 cases, 101691 expected lines, all equal (SAN=0)` |
| `sh tests/test_julia.sh` | passed (with the libgmp `LD_PRELOAD`), `tate_fe.jl` 20 of 20, 2 min 9 s |
| `python3 tools/memcheck/check_uninit.py tests/test_tate_fe.c tools/adf/adf.c` | 0, 0, 0 (after one fix, below) |
| `timeout 170 python3 -B proto/tate_checks.py` | TOTAL 466 checks, exit 0 |
| `timeout 1500 python3 -B lanes/t5-slice5/faults.py` | 6 of 6 detected |

Builds: `make -j2 BUILD=lanes/t5-slice5/build...` (with SAN=1, INV=1, CC=clang). `check-all` was not run.
Red and green runs are in `redgreen.md`:
- the test went red with a stub `fe_sides` (an assertion), then green;
- the driver went red with PARSE, then green;
- the Julia script has no red stage, because it calls only existing functions.

## Findings against the design, the oracle and the proofs

1. **No counterexample to P13, to the design or to the oracle.** All 280 OK cases and 17 balls overlap each other
   and contain both references. No acceptance case ended in LIMIT or NOT_DETERMINED.
2. **Brief qualification:** two raw golden pairs are imprimitive, not one. `(10,3)` lowers to `(5,3)` as `(8,7)`
   lowers to `(4,3)`. This changed my count of cases with extra bits (my first count was wrong; no value check
   failed).
3. **A failure mode created by the guard target, not a defect** (S18). Because `B` has the stricter target
   `bits + 4`, the equation can fail where `A` alone succeeds: on a ball whose image width lies between
   `2^-(bits+4)` and `2^-bits`. Found for 4 radii (section `guard`).
4. **The width reckoning for `R` has no source on disk** (S16): FLINT's `arb_mul` radius propagation is not
   documented in `refs/src/flint-3.0.1/arb.rst`. The run-time check of `R`'s width is the guarantee. The test also
   asserts the stronger `2^-(bits+2)`, which held with a wide margin.
5. **Design grammar.** The design writes `adf tate-fe CHI S --bits 80 --prec 160` (`api-5.md:384`); I followed the
   brief and the `tate_completed` grammar (decision 24).
6. **A defect of my own test, found by tooling:** `check_uninit.py` found an uninitialised `mag_t` in the helper
   `ratio`; ASan had not reported it. Fixed before the final runs.

## Sources pending

- The radius propagation of `arb_mul`/`acb_mul`. This is only an expectation in S16; the value is checked at run
  time.
- Inherited: the universal FLINT Conrey label identification (api-3c). The vectors use the oracle's lowering for
  python-flint's `dirichlet_char(C, label)`; the generator's agreement with mpmath to 1e-45 checks the labels used.
- Inherited from S9 of 5d: the identity theorem and holomorphic parameter integration behind P13's continuation
  (`analysis.md:30-48`).

## Not done

- There is no driver fixture line for a failure named `B`. The radius that produces one was found by a run (the
  C test covers it in `guard`), so it could not be derived before the run.
- valgrind, fuzzing and mutation testing were not run. The brief says mutation testing is not useful for a driver
  command, and the fault table replaces it.
- The `.out` lines of the fixture exceed 116 characters, because they are program output (as in
  `tests/driver/tate.out`). All prose and code lines are at most 116.
