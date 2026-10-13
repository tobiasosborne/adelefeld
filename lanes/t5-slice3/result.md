# Lane t5-slice3: slice 5b, local gamma and epsilon factors

I built `adf_local_gamma_at` and `adf_local_epsilon_at` end to end: header, code, oracle vectors, a new test
program, two driver commands, a Julia fixture, statements and faults. All four builds pass for the new test and
for `test_tate_local`, and LeakSanitizer ran. The driver suite passes. The Julia fixture passes only with the
known LD_PRELOAD workaround. All 17 scratch faults are killed. In the new code the mutation run left 3 survivors,
all equivalent. No defect of the design, the oracle or the proofs was found. There is one tightness note about
FLINT, one inherited HEADER-FINDING and three qualifications, listed below.

## Done, per step

A. Vectors. `lanes/t5-slice3/generate_vectors.py` writes `tests/ref/vectors/t5-slice3/gamma.jsonl`:
721 rows, 1950 sample enclosures, 388323 bytes (under 400 KB).
`timeout 180 python3 -B lanes/t5-slice3/generate_vectors.py` exits 0 in about 85 s.
- Coverage: p = 2, 3, 5, 7, 13; alpha in 1, -1, i, E(1/3) (a 2^-240 ball), 2, 1/2 + i, exact and with radii.
  Conductor 1 and conductors p and p^2; at p = 2 these are 4, 8 and 16, with the non-real (16,5).
- Unramified gamma: gamma's own poles s = k, alpha = p^(k-1) (DOMAIN exact; a ball, or a non-dyadic alpha:
  NOT_DETERMINED), a point just beside each, and nonzero lattice poles. At the integral's poles alpha = p^k
  the rows check the value 0, not a status.
- epsilon: rows at gamma's poles (OK, exactly 1).
- Real place: e = 0, 1 (chars (1,1), (4,3), (5,4), (5,2)) with poles 1+e+2k, points beside them, zeros
  -e-2k (exact 0) and balls around the zeros.
- LIMIT descriptor 65537, invalid conductor, alpha = 0 and alpha balls containing 0.
- References: 60-digit oracle midpoints (`local_gamma`, `local_epsilon`, `real_gamma`), each verified to contain
  an independent rigorous point box. Real Gamma comes from zeta_checks' Taylor integration, not FLINT Gamma.
  The `exact` field holds the rational value or the cyclotomic `A^a*T^a*G, G=(poly) at X=E(1/m)`.
  Exact integer coordinates and exactly-zero coordinates are written exactly; the latter are proved zero by
  sympy arithmetic modulo Phi_m.

B. Tests. New file `tests/test_tate_gamma.c`, 8 tests plus an INV-only test, 47955 checks.
- Every row and sample is checked for containment. Width bound: 2*radius <= 64*sampled diameter
  + 2^-120*image.
- Every call is repeated with z = s, z = alpha and where = NULL; failures are checked against byte sentinels
  and where = v.
- Exact quotients -3/4, 18/5, (10-15i)/13 and 0.
- G_minus is checked against conj(eta0) with E(-u/q) and against (-1)^e tau(conj eta0). It must not overlap
  the positive kernel or eta0 put in place of eta0^-1. Characters: non-real (5,2), (13,2), (16,5), (9,2),
  (25,2), and (4,3).
- epsilon is exactly 1 and exactly i^e, also at gamma's poles. Real s = 1/2 gives 1 and i.
- Exact zeros give exact 0, including -2^1000 and -2^1000-3. Poles give DOMAIN, NOT_DETERMINED on closed
  contact, OK beside them, and DOMAIN at 2^1000+1 and 2^1000. Pole tests at p also cover the 2^20-bit
  recognizer bound.
- The duality identity holds on >= 120 cases.
- Also tested: alpha 0 and alpha balls containing 0; nonfinite s and alpha; conductor 4 at p = 3;
  eta0->s ignored; prec 2, 53, the cap, and cap+1 (LIMIT first, even with forged inputs); conductor LIMIT;
  z = s = alpha; INV aborts in children that clear their objects.

C. Code. Two declarations, with comment blocks and sources, are appended to `include/adelefeld/localfactor.h`.
The functions and private helpers are appended to `src/localfactor.c`; 5a's helpers are reused unchanged
(`fail`, `tate_exact_finite_pole`, `shift_of`, `ADF_INV_PLACE`). The INV entry checks are in `ADF_INV_CONSTANT`.
- Unramified gamma is `alpha (1 - alpha T)/(alpha - p^(s-1))`, enclosed directly. It divides only if the
  denominator excludes 0. Exact poles are found by 5a's recognizer applied to the exact s - 1.
- Ramified gamma is `alpha^a exp(-a s log p) G_minus`. G_minus is built from `adf_char_chi_phase` and
  `adf_phase_get_acb` and summed outward; the phase is the negated theta minus u/q, reduced exactly.
- Real gamma is `i^e [L_inf(1-s+e)] [pi^z rgamma(z)]`. The first factor is the existing local zeta factor at
  1-s+e. rgamma is computed by the shifted product z...(z+n-1) rgamma(z+n), with no division. Exact integer
  poles and zeros are recognized on the original dyadic number.
- epsilon is exact 1 or i^e, and gamma's ramified path in the ramified case.

D. User calls.
- Driver: `tate_gamma` and `tate_epsilon` in `tools/adf/adf.c`, with the grammar of tate_local. Enum, table
  and dispatch were appended; a handler and a direct-argv branch were added. README section appended.
- `tests/driver/tate-gamma.cmd` and `.out` hold 10 lines written by hand first. Eight matched. The two radii of
  the exact decimals -0.75 and 1 were guessed as 1e-38 and corrected from the run to 3e-39 and 1.1e-38.
- `tests/julia/tate_gamma.jl` is registered in both paths of `tests/test_julia.sh`, after tate_local.

E. Statements: section "Slice 5b" appended to `docs/api-5a.md`. It covers values, steps with their enclosure
arguments, pole and zero sets with proofs (simplicity, no cancellation, disjointness), statuses, cost, caps,
decisions and Check lines.

F. Faults and mutation: see below.

## Commands and results

Every program ran under timeout; builds used `make -s -j2 BUILD=lanes/t5-slice3/build-X` (trees removed).

| Build | test_tate_gamma tests/checks/failed | test_tate_local tests/checks/failed |
|---|---|---|
| plain | 8 / 47955 / 0 | 6 / 22747 / 0 |
| SAN=1, ASAN_OPTIONS=detect_leaks=1 | 8 / 47955 / 0 | 6 / 22747 / 0 |
| INV=1 | 9 / 47985 / 0 | 7 / 22762 / 0 |
| CC=clang | 8 / 47955 / 0 | 6 / 22747 / 0 |

LeakSanitizer works on this host: a scratch program leaking 77 bytes was reported. Valgrind was not needed.
- `test_localfactor` (plain): 13 / 306730 / 0.
- `timeout 300 sh tests/test_driver.sh`: exit 0, 98 cases, 101660 lines equal.
- Direct calls work: `build/adf tate_gamma '<S>' with 2 with '<ALPHA>' with '<CHI>'` and `build/adf tate_epsilon`.
- `timeout 60 python3 -B tools/memcheck/check_uninit.py src/localfactor.c tests/test_tate_gamma.c
  tools/adf/adf.c`: 0 / 0 / 0.
- `timeout 170 python3 -B proto/tate_checks.py`: exit 0, 466 checks (local_exact 34, local_numeric 24,
  local_poles 18, real 16, faults_51 6).
- Julia: `timeout 120 julia --startup-file=no tests/julia/tate_gamma.jl lanes/t5-slice3/libadelefeld.so`
  exits 1 at dlopen (`__gmpn_modexact_1_odd`, the known GMP mismatch). With
  `LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10` it passes 36 of 36. The library was a -fPIC build linked
  with `cc -shared`. The full Julia suite was not run.
- Line length at most 116 in every new file and new block; every file ends with a newline.

Red-green (`lanes/t5-slice3/redgreen.md`):
1. Compile red: `adf_local_epsilon_at` undeclared.
2. Stage 1, with placeholders for the ramified branch and epsilon: 535 failed assertions. Six real-zero and
   eight real-width failures among them led to the rgamma shift.
3. Then 521 failed, all in the placeholder branches.
4. Stage 2: 606 and then 4 vector failures. Both were fixed in the generator: exact values had been written as
   decimal balls, and an exactly-zero real part as a 1e-62 ball. Then green.
5. Driver: red as 10 PARSE lines, then green.
6. The Julia fixture was written after the code, so no red was seen for it.

## Hand derivations

- p = 2, alpha = 1, s = 2: (1-1/4)/(1-2) = -3/4. p = 3, alpha = -1, s = -1: 4/(1+1/9) = 18/5.
  p = 5, alpha = i, s = 0: (1-i)/(1+i/5) = (10-15i)/13. (alpha, s) = (1, 0) at p = 2: 0.
- Real s = 1/2: Gamma(1/4) rgamma(1/4) = 1 for e = 0, and i Gamma(3/4) rgamma(3/4) = i for e = 1.
- Driver: G_minus for (5,2) at p = 5, s = 0, alpha = 1 is -2 sin(4pi/5) - 2i sin(2pi/5) = -1.17557 - 1.90211i.
  At digits 5 this prints (-1.1756 +/- 3e-5) + (-1.9021 +/- 1.4e-5)*i. For (4,3) at the cap, alpha = i, s = 0:
  G = -2i and gamma = 2i.
- Duality: let H(chi) = sum chi(u) E(-u/q). Then H(chi) = chi(-1) tau(chi), and
  gamma(alpha, eta0, s) gamma(alpha^-1, eta0^-1, 1-s) = p^-a tau(eta0) tau(conj eta0) = (-1)^e = eta0(-1)
  (Lemma 8:309). The unramified quotients cancel. The real case gives i^(2e). The identity holds as written.

## Fault table (`timeout 900 python3 -B lanes/t5-slice3/run_faults.py`, 17 of 17 killed)

| Fault | Failed checks |
|---|---|
| 5.1 inverse omitted (eta0 for eta0^-1 in G_minus) | 434 |
| 5.1 G_minus positive kernel | 542 |
| 5.1 alpha omitted, ramified | 375 |
| 5.1 alpha omitted, unramified | 479 |
| 5.1 exponent a forced to 1 (conductor 4 and others) | 407 |
| 5.1 parity omitted | 166 |
| 5.1 unit volume doubled (ramified constant times 2) | 651 |
| integral pole list for gamma, real (-e-2k) | 34 |
| integral pole list for gamma, finite (alpha = p^s) | 39 |
| Gamma/Gamma division instead of rgamma | 37 |
| i^e dropped | 89 |
| T^a for T^-a | 761 |
| where not written | 444 |
| z written before the pole check | 592 |
| epsilon rejects gamma's poles, unramified | 85 |
| epsilon rejects gamma's poles, real | oracle_vectors failed; then timeout 124 (gamma at the cap prec) |
| computed zero-containing denominator returned as DOMAIN | 70 |

## Mutation

Command (staged root from `prepare_mutations.py`, SAN+INV objects; each mutant recompiles localfactor.c and
links test_tate_gamma only):

```text
timeout 1260 python3 -B tools/mutate/mutate.py --root lanes/t5-slice3/mutation-root \
  --scratch lanes/t5-slice3/mutations --files src/localfactor.c --limit 60 --seed 53001 --jobs 2 \
  --timeout 150 --san --make 'timeout 150 sh run.sh' --copy include src tests prebuilt run.sh
```

Result: 60 mutants in 242.4 s; 27 killed, 29 survived, 4 not compiled, 0 timeouts.
In the new functions (lines >= 541): 24 mutants, 19 killed, 2 not compiled, 3 survived, all equivalent:
- :715 `n = 0` -> `n = 1` for a shift above 64: rgamma(z) = z rgamma(z+1) is the same function.
- :748 `1 + e` -> `0 + e`: an integer of parity !e cannot equal e, so the test is unchanged.
- :748 `>=` -> `>`: s = 1+e exact is then rejected as DOMAIN by the delegated factor at 1-s+e = 0.
The 26 survivors outside the new functions are in code that test_tate_gamma does not target (the run was
restricted to it). They are reported, not repaired:
- 5a's adf_local_tate_at, not called: :499, :508, :522 (two), :524, :526, :534.
- 5a's tate_finite_factor: :446, a swap of commutative arguments.
- Finite zeta factor, not called by gamma: :62 (sign-branch choice), :96 (two), :115.
- real_derivative_bound and real_factor of the inherited Gamma certificate: :217, :221, :222, :223, :237, :240,
  :244, :248, :250, :252, :255, :344, :367. 5a reports the same class as open.
- :416, `||` -> `&&` in the shared recognizer's bound. It was a gap for gamma; I added a test (s = 2^62 at p = 2).
  A replay of that mutant under `ulimit -v 4000000` now dies with SIGFPE, exit 136.
`tools/mutate/equivalent.txt` is unchanged.

## Findings

- FLINT tightness, not a soundness defect. acb_rgamma on a ball near a pole of Gamma is a valid but loose
  enclosure. For z = -k +/- 2^-12, k = 0..3, it returns radii 3.7e-4 to 8.8e-2 where the variation is
  3.5e-4 to 2.1e-3. With it the real vectors fail their width test. I use the identity
  rgamma(z) = z...(z+n-1) rgamma(z+n), which costs at most 64 extra multiplications. That is more than the
  design's cost line ("one Gamma, one rgamma and one exponential").
- HEADER-FINDING, inherited from 5a and marked in gamma_real. api-5.md:33 counts guards against the precision
  cap, but the delegated local zeta factor works at prec+32 even at the cap.
- Qualification of the brief: "unit volume doubled" has no production site in gamma. Doubling the unit
  volume scales Z(F f) and Z(f) alike and cancels. The scratch fault doubles the ramified constant instead.
- Qualification of the brief: a place not of Q cannot be built with place.h. A forged handle aborts under INV
  (tested); it is not a DOMAIN case, as in 5a.
- Oracle note, not a defect: for real characters at real s, a coordinate of gamma is exactly 0, but the
  60-digit oracle gives about 1e-62. Importing char_checks lazily sets mp.dps to 60. The generator decides
  exact zeros by cyclotomic arithmetic.
- Driver names follow the brief (tate_gamma, tate_epsilon). The design writes local-gamma / local-epsilon.

## Sources

Quoted with line numbers: refs/src/flint-3.0.1/acb.rst:6-17, 637-643, 658-669, 893-902;
refs/src/tate-poonen/notes.txt:61, 62-64. Project proofs: analysis.md P9:331-400, P10:401-451, Lemma 8:295-330;
conventions 6.4. Pending, inherited: [source pending: universal FLINT Conrey pairing identification] (G_minus
reads the phases through adf_char_chi_phase), and the inherited measure/holomorphy premises of api-5.md:402-403.

## Not done

- No long differential fuzz run.
- The Julia fixture was not seen red, and the full Julia suite was not run.
- The 26 mutation survivors outside the new functions were not pursued.
- The optional midpoint refinement of the finite denominator is not implemented.
- Avoidable cost: G_minus calls adf_char_chi_phase once per unit, and each call repeats the group setup
  D(p^a). One setup per sum would do.
- The build/ tree of the worktree, made by test_driver.sh, was left in place.
