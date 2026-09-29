# Lane r-slice1: result (real roots by exact isolation, issue adf-8di, first slice)

Worktree at f0a23fc; `refs/src` linked to the main checkout. Times are wall times on the shared laptop (load 1.5
to 3.8 during the runs), read from `date`.

## What was built

`adf_roots_real` now takes its candidates from `src/roots_real.c` (new) instead of
`arb_fmpz_poly_complex_roots`. The certificate of Algorithm RR is unchanged: FLINT's count (S-D11), the exact
test of P3.8 at the exact end points of every ball, the order, the count, the accuracy of S-D19
(`real_finish` in `src/roots.c`, not touched). In `src/roots.c` only the call site in `adf_roots_real`, the
declaration of the hidden function and one comment line of the slice-3 block changed.

- **Algorithm D (isolation)**: bisection with Descartes' rule of signs on the positive roots of `g` and of
  `g(-X)`, in exact integers ([SM] `sagraloff-mehlhorn/tex/arxivfinal.tex:547` to `553`, `:569`, `:571` to
  `575`), with a root bound of my own (Lemma R1, proved), FLINT's `_fmpz_poly_scale_2exp`,
  `_fmpz_poly_taylor_shift`, `_fmpz_poly_reverse`, `_fmpz_poly_div_root` (each read in its `.rst` and source,
  cited in the file). A root met at a midpoint is an exact ball and is divided out. Only nodes with `v >= 2`
  are kept on the stack (at most `deg g / 2` at a time).
- **Algorithm F (refinement)** of each isolating cell: step G (galloping on the exponent from an anchor: 0,
  or a root at an end of the cell), step Q (Eqir, [KS] `kerber-sagraloff/tex/arxiv.tex:278` to `298`,
  `:337` to `342`), step B (bisection); stop when both ends are not roots, the cell is above a static floor
  (the right end of the previous item as isolated), and the accuracy is at least `max(prec, 2)`.
- **Hidden function** `adf_roots_real_isolate(cand, &m, g, prec)` (visibility hidden; `tests/test_exports.sh`
  passes, 262 of 262 declared functions exported, none undeclared).
- `docs/design/real-roots.md` (new): Lemma R1, Algorithm D with Propositions R2 (correct) and R3 (ends; tree
  size `O(n (K - log2 sigma))`), Algorithm F with Proposition R4 (one root kept; it ends; a dyadic root with an
  odd part of at most `need + 1` bits is exact; lists nested when `prec` grows), Proposition R5 (the whole
  passes steps 5 and 6 of Algorithm RR), cost and the lower bound, a table of the statements.
- `include/adelefeld/roots.h`: only the sentences of `adf_roots_real` and of `ADF_ROOTS_REAL_PREC_MAX` about
  method, cost and when LIMIT and NOT_DETERMINED occur; no declaration, no status and no constant changed.
- `docs/PERF.md`: one row in section 4 with its floor and the measured distance.
- `bench/bench_roots_real.c`: families `thirds`, `sqrt2`, `x5`, `mignotte`; mode `eval2` (the floor); `--run`
  (the table below; `make -C bench run` runs every bench with `--run`, which this file refused before; the
  whole `make -C bench run` was not run here, only `make -C bench bench_roots_real`, which builds).
- `proto/real_isolation.py`: synced with the C code (step G, static floor, no cap on `N`); one test added.
- `tests/fuzz/diff_roots_real.py`: the C balls must equal those of `proto/real_isolation.py` exactly; the
  families of the review finding, clusters, close pairs and dyadic roots added.

Deviations from the brief, each with its reason:
1. Decision (2) (plain bisection, QIR later "if the benchmark asks for it"): the benchmark asks for it. Plain
   bisection is quadratic in `prec`: `X^2 - 2` took 0.0174 s at `prec = 4096`, 0.434 s at 16384, 6.50 s at
   65536 (factor 15 per factor 4), about 6600 s extrapolated to `ADF_ROOTS_REAL_PREC_MAX = 2^21`, which the
   existing test `tests/test_roots_real.c` asks. With step Q: 0.38 to 0.86 s. See `redgreen.log`, green 2.
2. Step G was not in the brief. Without it the refinement of a cell that touches 0 keeps accuracy -1 for
   `log2(width / |r|)` bisections: `(3X - 1)(2^(M+10) X - 1)` ran about `2^24` bisections before its LIMIT
   (green 1). Generalised to a root at an end, it also removed the `e` bisections of the review family:
   `2^e`, `2^e + 1` at `e = 10000` went from 0.10 s to 0.0006 s.
3. The floor is static (the previous item as isolated), not the previous refined ball as in the prototype of
   lane d-realroots: with the moving floor the lists are not always nested when `prec` grows
   (`(3X - 11)(3X - 13)`: `[17/4, 9/2]` at `prec = 2`, `[4, 9/2]` at `prec = 3`).
4. The root cell is taken at `max(K, KMIN)`: with `K < KMIN` the polynomial `2^(2M+2) X^2 + 1`, which has no
   real root, gave LIMIT (red 4 in `redgreen.log`); now OK with the empty list.

## Files

New: `src/roots_real.c`, `tests/test_roots_real_isolate.c`, `docs/design/real-roots.md`,
`lanes/r-slice1/{progress.md, redgreen.log, bite.py, result.md, runs/*}`.
Changed: `src/roots.c` (call site), `include/adelefeld/roots.h` (sentences), `tests/test_roots_real.c` (one
planted case added: `(X - 10^400)(X - 10^400 - 1)^2`, and the comment that said it was too slow),
`bench/bench_roots_real.c`, `docs/PERF.md` (one row), `proto/real_isolation.py`,
`proto/test_real_isolation.py`, `tests/fuzz/diff_roots_real.py`.

## Checks run (commands and results)

- Red: `timeout 300 make -j2 build/test_roots_real_isolate` -> undefined reference to `adf_roots_real_isolate`
  (link error, first test). Against the old route with a stub of the hidden function: slow family
  `pair e = 1500 at prec 2: 116.129 s, the bound is 10.0 s`, then killed at 170 s; without the timed tests:
  `11 tests, 16678 checks, 87 failed checks, 3 failed tests` (the stub; Wilkinson 20 integer roots not exact;
  16 dyadic roots not exact). Red 4: `2^(2M + 2) X^2 + 1: status 10`. Details: `redgreen.log`.
- Green: `timeout 170 ./build/test_roots_real_isolate` -> `11 tests, 19165 checks, 0 failed checks, 0 failed
  tests` (11.9 s); `timeout 170 ./build/test_roots_real` -> `8 tests, 10744 checks, 0 failed checks, 0 failed
  tests` (1.75 s; 24.34 s before this lane, with 10666 checks).
- Tests bite (brief item 6), `timeout 1200 python3 lanes/r-slice1/bite.py` (scratch copies under
  `build/bite/`): `bite: 6 of 6 faults caught`: v = 2 accepted as one root; the right half shifted by 2; an
  exact midpoint root dropped; the sign of `g'` left out in step G; step Q without the test at `m' + 1`; the
  moving floor (caught by one check only: the nesting of `191/3`, `193/3`). Output: `runs/bite.txt`.
- Fuzz, a smoke test: `timeout 180 python3 tests/fuzz/diff_roots_real.py --seconds 120 --seed 20260929` ->
  `1229 calls in 120 s; statuses {'OK': 1202, 'DOMAIN': 27}; roots 3237, exact balls 1353, lists without a root
  35, reduced inputs 654; lists equal to proto/real_isolation.py 1202; 0 disagreements`. A case would have
  failed on a status other than the reference's, a different `g`, `reduced` or count, a C ball meeting a
  reference enclosure other than its own, an accuracy below `max(prec, 2)`, a verifier refusing, or any end
  point differing from the Python implementation of the same algorithm. 120 s is a smoke test, not the long
  run of `docs/workflow.md` rule 5.
- `python3 -m unittest proto/test_real_isolation.py` -> `Ran 16 tests, OK` (1.8 s).
- `make clean && timeout 1500 make -j2 check-all` -> `check-all passed: make check, driver, exports, julia,
  mutate-selftest, memcheck-selftest` (3 min 28 s; `check passed: all 58 test programs`; Julia
  `adf_roots_real through ccall: X^3 - 2 X to 100 bits | 13 13`). Log: `runs/check_all.log`.
- `make clean && timeout 1800 make -j2 check SAN=1` -> `check passed: all 58 test programs` (4 min).
  Log: `runs/check_san.log`.
- `make clean && timeout 1800 make -j2 check CC=clang` -> `check passed: all 58 test programs`.
  Log: `runs/check_clang.log`.
- `timeout 170 sh lanes/m1-headers/check_headers.sh` -> `check_headers: passed`.
- The planted case added to `tests/test_roots_real.c` came after the three full runs; afterwards
  `make clean && make -j2 build/test_roots_real build/test_roots_real_isolate SAN=1` and both programs:
  `8 tests, 10744 checks, 0 failed checks, 0 failed tests` and `11 tests, 19165 checks, 0 failed checks, 0 failed
  tests`.
- Not run: `make check INV=1` (not asked; `src/roots.c` and `src/roots_real.c` have no `ADF_INV` lines).

## Benchmark (brief item 5)

`bench/bench_roots_real.c`, mode `adf`, one call per run, `taskset -c 2`. Before: the library at f0a23fc
(`runs/bench_before.txt`, load 1.65). After: `--run`, median of 5 (`runs/bench_after.txt`, load 3.5).
`eval2`: the exact signs at the end points of the output balls (the floor, see below).

| input | prec | before (s) | after (s) | after / eval2 |
|---|---|---|---|---|
| `1234567 (X - 2^e)(X - 2^e - 1)`, e = 600 | 2 | 0.0546 | 0.000045 | 29 |
| the same, e = 1200 / 1500 | 2 | 10.87 / 111.5 | - / 0.000070 | - / 27 |
| the same, e = 3000 / 10000 / 100000 | 2 | no answer in 175 s (review) | 0.00016 / 0.00063 / 0.014 | 26 / 20 / 20 |
| `(X - 1)(2^e X - 2^e - 1)`, e = 1200 / 10000 | 2 | 9.38 / - | 0.000061 / 0.00087 | 24 / 21 |
| `(3X - 3 2^e - 1)(3X - 3 2^e - 2)`, e = 3000 / 30000 | 2 | - | 0.0048 / 0.22 | 324 / 747 |
| `prod (X - 2^300 - i)`, d = 4 / 8 / 16 | 2 | 0.104 / 119.7 / - | 0.00057 / 0.0018 / 0.0074 | 214 / 155 / 76 |
| Wilkinson 20 | 53 | 0.0040 | 0.00030 | 27 |
| `X^2 - 2`, prec 4096 / 65536 / 2^21 | | - | 0.00032 / 0.012 / 0.86 | 6.9 / 6.0 / 6.6 |

Old and new side by side at the same moment (00:00, load 1.45, `build/red/bench_old` linked with the old
`src/roots.c`): `far` (`2^100000`, `3 2^100000`) 2.1 ms both; Wilkinson 20 at 53: 4.0 ms old, 0.22 ms new;
`deg 300 d = 4`: 105 ms old, 0.34 ms new; **`X^2 - 2` at 65536: 2.9 ms old, 5.8 ms new; at `2^21`: 0.167 s old,
0.381 s new** (the new code is about 2 times slower at high precision for well separated roots: FLINT's
Newton on balls against Eqir with exact integer evaluations). A complex pair close to the axis,
`2^(2e) (3X - 1)^2 + 1` (and times `X - 3`): new 0.020 s (0.033 s) at `e = 10000`; old 1.73 s (2.96 s) at
`e = 1000` (`runs/complex_pair.txt`).

Lower bound (`docs/PERF.md` section 4, the new row; `docs/design/real-roots.md` section 6). PROVED: read the
input, write the output. MODEL, restricted to results certified by exact signs at the end points (decision 3):
the signs of `g` at both end points of each non-exact ball. Measured as `eval2` on the balls this code outputs,
which is an upper estimate of that floor; so the distance from the floor is at least the ratios in the last
column: 20 to 29 for the review family, 5.5 to 9.5 at high precision, 21 to 27 for Wilkinson 20 and Mignotte, up
to 747 for the cluster `2^e + 1/3`, `2^e + 2/3` (the descent of the Descartes tree, one level per bit).

## What is proved and what is not

Proved in `docs/design/real-roots.md`: R1 (root bound), R2 (isolation correct, modulo Descartes' rule, cited
from [SM]), R3 (it ends; tree size, modulo [SM]:569 and :571 to 575), R4 (refinement keeps exactly the one root,
ends, returns LIMIT only when a cell below `2^KMIN` is forced, dyadic roots with a short odd part are exact,
nested lists), R5 (the output passes the tests of Algorithm RR). Not proved: the quadratic convergence of step
Q (cited from [KS]:342, which cites Kerber's analysis; not on disk); a bound on the bit size of the numbers in
the tree beyond the sketch in section 6. FLINT's count stays trusted (S-D11), Sturm's theorem still
`[source pending: Sturm's theorem]`.

## Findings

- Against the specification: none. `docs/SPEC.md` 15 has to record the deviations above from decisions (2)
  and the new steps (G, the static floor), if the orchestrator accepts them.
- The header's old sentence for NOT_DETERMINED ("a larger prec may succeed") no longer holds: with exact
  isolation the candidates pass every test by construction (R5), so this status now means a disagreement of
  FLINT's count with the isolation, a defect; the sentence is reworded. The status itself is kept (decision 5).
- Regression: about 2 times slower than FLINT's route at high precision for well separated roots (above).
- `src/roots.c` still includes `<flint/acb.h>` and `<flint/arb_fmpz_poly.h>` (lines 37 and 38), now unused; left
  because they are outside the part of `src/roots.c` this lane may change.
- `bench/bench_roots_real.c` took no `--run`, so `make -C bench run` (which runs every bench with `--run`) would
  have stopped at it with exit 2; it now has one (read from `bench/Makefile`, not run).

## Not done, left for later

- The long differential fuzz run (an hour or more, alone, `docs/workflow.md` rule 5): only the 120 s smoke.
- The isolation descends one level per bit toward a cluster far from its own scale (`thirds`: 0.22 s at
  `e = 30000`, linear in `e` levels of growing size); a quadratic or continued-fraction step in Algorithm D is a
  later slice. The same holds for roots of absolute value near `2^KMIN` next to other tiny roots: the tree
  descends up to `2^24` levels before LIMIT (not tested; step G covers a single tiny root only).
- The high-precision regression above (a Newton step on the local Taylor polynomial, or Eqir on it, is the
  candidate).
- `docs/SPEC.md` 15 and `docs/sources.md` rows for [SM] and [KS] (not owned).
- Review: nothing here has been reviewed by another model.
