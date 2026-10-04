# f-review13: referee of IL1-IL8, G7-G12 (Log on ideles) and planted faults against the tests

Worktree at 58659bb. Files written (all in lanes/f-review13/): progress.md, result.md, il_oracle.py (own oracle),
checks.py, probe.c, distinguish.txt, mutate.py, build.log. The build tree lanes/f-review13/build/ was deleted.

## Setup for every command below (repository root)

```text
timeout 600 make -j2 BUILD=lanes/f-review13/build lanes/f-review13/build/libadelefeld.a
timeout 300 make -j2 BUILD=lanes/f-review13/build lanes/f-review13/build/test_gfunc_log
cc -Iinclude -std=c11 -O2 -Wno-stringop-overflow lanes/f-review13/probe.c \
   lanes/f-review13/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review13/build/probe
timeout 170 python3 -B lanes/f-review13/mutate.py prep        # baseline of driver and probe
timeout 175 python3 -B lanes/f-review13/mutate.py <ID>        # one planted fault, prints a JSON record
echo '<probe line>' | timeout 60 lanes/f-review13/build/probe  # unmutated library
```

Probe syntax: `at p num den c M N` (Log_at at p, real 2); `ref s num den c M N k p1..pk` (Log_refine, real s,
prec 64; prints y=kept/CHANGED after a failure); `refq s num den c M N prec k ...`; `atinf s` (the four calls
at infinity with a sentinel where); M may be written B^E.

## Findings

### F1. MAJOR. Tests cannot see a wrong centre when r' = 1 and c is not +-1 at a restricted prime (fault O1)

- Input: p = 2, x = (1 ; 1 * [3 mod 8]), N = 3. Also p = 3, x = (1 ; 1 * [2 mod 9]), N = 2.
- Code under test: gfunc_log.c:134-135 takes the zero-centre shortcut only for r' = 1 AND c = 1. Fault O1 drops
  "c = 1". Mutant result: 0 + O(2^3) and 0 + O(3^2); Log_refine at {3}: (0 mod 36).
- True: Log(3) = 4 mod 8 at 2 (3 = -(1-4), log(1-4) = -4 mod 8), Log(2) = 6 mod 9 at 3 (log 4 = 3 mod 9, halved).
  The library returns 4 + O(2^3), 6 + O(3^2), (24 mod 36); the mutant's balls are disjoint from the true images.
  Every test passes the mutant: 12 tests, 2194049 checks, 0 failed; all four driver cases pass.
- Cause: lanes/f-slice11/write_selection.py keeps only c in {1, M-1, p}; c = p is not coprime at a restricted
  prime, so all 4973 restricted rows of local.jsonl have c = +-1 mod p^k (scan of the file) and Log(c) = 0 in every
  row; the 20 complete-image rows (r = 1, c = 1, M = p^k) all have centre 0. Fault O2 (c dropped from the compact
  residue) is caught only by 72 checks from the one CRT input (3/4, 5, 36); local_selection passes it.
- The library is correct on these inputs (checks.py lib, 63440 calls, includes c != +-1 with r' = 1).
- Reproduce: `mutate.py O1`; probe lines `at 2 1 1 3 8 3`, `at 3 1 1 2 9 2`, `ref 2 1 1 2 9 2 1 3`;
  driver: `prec 3` / `Log_at (1 ; 1 * [3 mod 8]) with 2` prints `2: 4 + O(2^3)`.

### F2. MAJOR (by the brief's rule: wrong status). Real-precision LIMIT before n < 0 is untested (fault B9)

- Input: adf_idele_Log_refine(y, &w, x, NULL, -1, 5, ADF_REAL_PREC_MAX + 1), any x.
- Header gfunc.h (Log_refine): "LIMIT for excessive real prec first (where=infinity)". Library: LIMIT, where = inf.
  Mutant (n < 0 tested first, gfunc_log.c:283-288 swapped): DOMAIN, where untouched. Tests call n = -1 only with
  prec 64 (test_gfunc_log.c:485). Both answers are failures; the code and test disagree on which one.
- Reproduce: `mutate.py B9`; probe `refq 2 1 1 1 0 5 3000000 -1` gives `st=10 where=inf`, mutant `st=7`.

### F3. MAJOR (by the brief's rule: wrong status). The aggregate CRT bound's baseline charge is untested (fault O3)

- Input: x = (2 ; 1 * [1]) (exact, r' = 1 everywhere), S = {3}, N = 2^25 - 1 = 33554431. Also S = {2}, N = 2^25 + 1.
- G11 step 5: total = 2 bits(2) + sum charges <= 2^26. Here 4 + 2 (2^25 - 1) = 2^26 + 2: library LIMIT, where
  untouched. Mutant (gfunc_log.c:239 total = 0, baseline not charged): OK, R = 4 * 3^(2^25-1) (53182517 bits).
  The tests pin the comparison operator (O4, O17 detected by one check each) but not the baseline term.
- Consequence: the resource bound moves by 4 bits; the value returned by the mutant is correct.
- Reproduce: `mutate.py O3`; probe `ref 2 1 1 1 0 33554431 1 3` and `ref 2 1 1 1 0 33554433 1 2`.

### F4. MINOR. Output preservation of Log_refine on a finite LIMIT is untested with a valid real part (fault B10)

- Input: x = (2 ; 3 * [1]), positive real, S = {2}, N = 2^25 (actual working-power LIMIT at 2).
- Header: "Values unchanged on any failure". Library: LIMIT, where = 2, y kept. Mutant (gfunc_log.c:356 swaps
  when st_real == OK): LIMIT, where = 2, y overwritten. The only test of this route (test_gfunc_log.c:512-513)
  uses real -2, whose DOMAIN masks the fault.
- Reproduce: `mutate.py B10`; probe `ref 2 3 1 1 0 33554432 1 2` (`y=kept`; mutant `y=CHANGED`).

### F5. MINOR. "where untouched on OK" is untested for Log_at at infinity (fault B11)

- Input: Log_at(s, &w, (2 ; 4 * [1 mod 9]), infinity, 64). Library: OK, w untouched. Mutant: OK, w = infinity.
  Every test call of adf_idele_Log_at at infinity passes where = NULL (test_gfunc_log.c:187, :367).
- Reproduce: `mutate.py B11`; probe `atinf 2`.

### F6. MINOR. Known local refusal versus an earlier aggregate refusal is untested (fault B9c)

- Input: x = (2 ; 3 * [1]), S = {2, 3, 5}, N = 2^25. Aggregate exceeded at 3, compact-power refusal known at 5,
  actual working LIMIT at 2. Header/G11: known local refusals take precedence and earlier primes are evaluated:
  library LIMIT, where = 2. Mutant (aggregate wins once flagged): LIMIT, where untouched. Test :516 uses {5, 2},
  where the aggregate is not yet exceeded when 5 is reached.
- Reproduce: `mutate.py B9c`; probe `ref 2 3 1 1 0 33554432 3 2 3 5`.

### F7. MINOR. Real-precision LIMIT before n > 65536 is untested (fault B9d)

- Input: n = 65537, prec = ADF_REAL_PREC_MAX + 1. Library LIMIT, where = inf; mutant LIMIT, where untouched.
- Reproduce: `mutate.py B9d`; probe `refq 2 1 1 1 0 5 3000000 65537`.

### F8. MINOR. The place of a main-phase local failure is tested only at the first sorted prime (fault O23)

- Input: x = (2 ; 1 * [2 mod 5^22369620]), S = {2, 5}, N = 22369620. Compact power and aggregate pass
  (4 + 3 N <= 2^26), lball_Log's working power at 5 does not. Library LIMIT, where = 5; mutant (where = ps[0])
  LIMIT, where = 2. Two main-phase failures cannot coexist under the aggregate bound (each needs
  K bits(p) > 2^26 - 27 bits(p)), so only this place is at stake.
- Reproduce: `mutate.py O23`; probe `ref 2 1 1 2 5^22369620 22369620 2 2 5` (about 5 s).

### F9. MINOR (cost). A refused request does the full local work the G12 order is meant to avoid

- G12: "an aggregate refusal precedes uncomputed actual local working-power evaluation. The alternative ...
  can do substantial work for a request whose combined modulus is refused." G11 step 4 and gfunc_log.c:309-317:
  on a known local refusal every earlier prime is evaluated with local_log, also when the aggregate is exceeded.
- Input: x = (2 ; 2 * [1]), N = 1048577. S = {3,5,7,...,47} (14 primes): LIMIT, where untouched, 5 ms.
  S plus {2^64-59} (compact power refused there): no result within timeout 170 s; it runs 14 full local Logs at
  2^20 digits before returning LIMIT. {2^64-59} alone: 8 ms. {3, 2^64-59}: LIMIT at 2^64-59 after 8.8 s.
- True: the tie rule needs only to know whether an earlier prime would hit its working-power bound; that is
  decided by the evaluator's W formula without the series. Status and where are as documented.
- Reproduce: `P="3 5 7 11 13 17 19 23 29 31 37 41 43 47"; echo "ref 2 2 1 1 0 1048577 15 $P 18446744073709551557"
  | timeout 170 lanes/f-review13/build/probe` (killed at 170 s); the same with 14 and without the last prime.

No finding against docs/SPEC.md. No BLOCKER. No false proof step was found.

## Part 1: statement by statement

Own oracle (il_oracle.py): Log mod p^H without the log series: torsion removed by a^(p^(H-1)) (odd p, asserted
w^(p-1) = 1 mod p^H) or by the sign (p = 2); the principal unit inverted through a table of exp(y), y in
p^d Z / p^H Z, exp summed in integers and cut by v(y^j/j!) >= j d - (j-1)/(p-1) (increasing in j); the table is
asserted injective with p^(H-d) entries. H = 5 at 2 and 3, H = 4 at 5 and 7; every comparison is modulo p^H,
which suffices because every claimed exponent in the grid is <= 4 < H, so a wrong exponent or centre shows as a
different residue set mod p^H.

- IL1. Checked, sound. Lemma 5.1 plus Zhat^x = prod Z_p^x (Lemma 5.2-5.3) gives the product of local factors;
  k >= 1: c is a p-unit so c + p^k Z_p consists of units; k = 0 and the 2-adic shell immediate. v_2(M) = 1 is NOT
  excluded: the predicate (ucoset.c:95-109) admits (1,2), (5,6), the driver parses `[5 mod 6]`; only results of
  operations are normal. It is handled (IL1.7, IL3.4; gfunc_log.c:134 and E = max(k,2)); my grid has M = 2, 6, 10,
  30 at p = 2 and all library calls there agree with the enumeration.
- IL2, IL3, IL4. Checked, sound, equality of sets. checks.py sets: 10840 cases (p = 2,3,5,7; M = p^k times
  {1, q, q', q q'}, k = 0..3; up to 10 residues c per M, all when phi(M) <= 10; r = p^m r', m = -2,0,1,3, eight
  r'): the enumerated image of IL1's set equals Log(r'c) + p^E Z_p (resp. p^d Z_p, singleton), with
  |image| = p^(H-E). r' contributes (r' = 2, 5, 3/4, 7/11, p+1), p^m does not; at 2, k = 2 gives 4 Z_2, k >= 3
  translates; c = 3 mod 4 and exact c = -1 occur. IL4 surjectivity is proved in functions.md Lemma 9 step 5,
  not cited. Log(r') in p^d Z_p for every p-unit since Log = log(u), u in 1 + p^c Z_p (Prop 4). "Negative r'"
  exists only as r'c with c = -1; Log(-1) = 0 (Prop 11 step 2).
- IL5. Checked, sound. CRT steps 6-7 re-derived (if K <= beta the ball contains p^beta Z_p because it contains a
  point of J_p; if K > beta the canonical centre is in p^beta Z_p). The refined ball is not claimed smallest, and
  no smallest finite ball containing the image exists for M > 0 (intersect with z = 0 mod q for an odd q outside
  the support). "0 + 4 Zhat also for exact input" is an enclosure (Prop 12 step 4), a recorded choice (N-D17).
  checks.py crt: 660 Log_refine calls (11 inputs including v_2(M) = 1, 3, 4, c != +-1, exact; 10 lists including
  7 and shuffles; N = -1..4) against (a, R) from my enumeration and a CRT by search: 0 mismatches.
- IL6. Checked, sound for k <= H (the oracle asserts it). Step 1 is Prop 11's isometry on b/a in 1 + p^H Z_p.
  checks.py hprime: 1587 cases, image from units mod p^(H+1) reduced equals image from units mod p^H.
- IL7. Checked, sound. j >= p^e >= 2^e >= 2e gives term valuation >= j/2 >= H for j >= 2H, at p = 2 and 3 as
  well; W = H + floor(log_p(T-1)) covers the division by p^e. checks.py series: the design's log_unit equals my
  exp-inversion Log on all 2736 units. Truncation at H = 5: p = 2 wrong for T = 2 (8 of 16 units), right for
  T >= 3; p = 3 wrong for T <= 4 (108 to 138 of 162), right for T >= 5; T = 2H is safe, not tight. The
  "independent routes" share log_principal; they are independent only in torsion removal (the design says so);
  my exp route is independent of the series.
- IL8. Checked. expected_ball is IL2-IL4 in constructive form and also the code's construction (compact residue,
  Log at min(K,E)). local.jsonl (6784 rows) all from expected_ball, all at grid points and N covered by the design's
  check_images/check_exact runs; for c it is weak (F1). crt.jsonl: same construction as refine(); 140 of 280 rows
  are not covered by check_crt (56 rows of input (4,1,1,0), 64 rows with N in {4,6}, 20 rows with list (7,5,3))
  and guard against regression only; my crt section covers those shapes. real.jsonl from mpmath; driver goldens
  hand-written.
- G7. Checked: prec check before INV and allocation; real ball from sball_Log_at/log_abs_at copied unchanged;
  (0,4,1) in a temporary, swapped on OK; where = inf on failure (probe `atinf 2`, `atinf -2`).
- G8. Checked: K = N exact, min(N,E) otherwise; m only bounded; shortcuts equal the proof's branches; step 6
  (t/A in 1 + p^K Z_p, K > d) re-derived. Library = own oracle on 63440 calls.
- G9. Checked: two-sided K check, compact power checked before fmpz_pow_ui, working power delegated.
- G10. Checked: 2 sorted first; replacement at L > 2 keeps 4 | b; fmpz_CRT(sign 0) preconditions hold.
- G11. Checked; order prec, n < 0, n > 65536, sort and shape, budget, evaluation is as stated; charges compared by
  floor division without overflow. F9 (cost); F2, F6, F7 are test gaps on this order, not code errors.
- G12. Checked: pending items match the design; nothing in the header is unproved beyond delegation to
  sball_Log_at, adf_real_log and lball_Log.

## Part 2: fault table

37 faults, one at a time, in lanes/f-review13/build/mut/<ID>/gfunc_log.c. "Test" is test_gfunc_log (failed checks
/ failed tests, or the crash); "Driver" lists the gfunc-log-* cases that fail (adf.c linked against the mutated
archive, byte comparison and #!exit); Julia was not run (no PIC archive; a second full build is outside the brief).
None is one of the design's eight (idele-log.md section 4).

| ID | Fault | Test | Driver |
|---|---|---|---|
| B1 | k = v_p(M) + 1 | 4684 / 5 | values, refine-values |
| B2 | k = v_p(M) - 1 | 4841 / 7 | values, refine-values |
| B3 | m = v_p(r) with wrong sign | 0: EQUIVALENT (m only enters the symmetric bound) | - |
| B4 | r' -> r in the residue numerator | 614 / 1 | - |
| B4b | r' -> r, numerator and denominator | abort (invmod of p-divisible denominator) | - |
| B5 | p=2, k=2 without the K <= 2 shortcut | SIGFPE at K <= 0 | - |
| B5p | same, shortcut kept for K <= 0 | 0: EQUIVALENT (Log(r'c) in 4 Z_2) | - |
| B6 | unrestricted odd image p^2 Z_p | 3100 / 4 | values, refine-values |
| B7 | Log uses log abs at the real place | 30 / 1 | status |
| B8 | K = N instead of min(N,E) | 9036 / 6 | values, refine-values |
| B9 | n < 0 before real prec LIMIT | 0: SURVIVES (F2) | - |
| B9c | aggregate before known local refusal | 0: SURVIVES (F6) | - |
| B9d | n > 65536 before real prec LIMIT | 0: SURVIVES (F7) | - |
| B10 | refine writes y when real OK, before finite check | 0: SURVIVES (F4) | - |
| B10b | Log_at writes y before the failure check | 2 / 1 | - |
| B11 | where written on OK by Log_at at infinity | 0: SURVIVES (F5) | - |
| B12 | CRT symmetric (unreduced) representative | 78 / 1 | refine-values |
| B12b | compact residue not reduced mod p^K | 0: EQUIVALENT (G8 step 6: any lift mod p^K) | - |
| B13 | named prime dividing neither M nor r skipped | 190 / 1 | refine-values |
| O1 | zero centre whenever r' = 1 (c ignored) | 0: SURVIVES (F1) | - |
| O2 | c dropped from the residue | 72 / 1 (CRT rows only) | - |
| O3 | baseline 2^2 not charged in the aggregate | 0: SURVIVES (F3) | - |
| O4 | aggregate >= instead of > | 1 / 1 | - |
| O5 | where = inf whenever the real part failed | 1 / 1 | - |
| O6 | exact zero not rounded to N in refine | 169 / 1 | refine-values |
| O8 | conservative ball 0 + 2 Zhat | 67 / 4 | values |
| O9 | place list not sorted | abort (fmpz_CRT) and 24 / 1 | refine-status |
| O10 | earlier primes not evaluated on a known refusal | 1 / 1 | - |
| O11 | preflight compact-power check dropped | 2 / 1 | - |
| O14 | real precision ceiling applied at a prime | 8 / 2 | - |
| O16 | exact zero for M > 0, r' = 1, c = 1 | 94996 / 2 | values, refine-values |
| O17 | named 2 charged L instead of L - 2 | 1 / 1 | - |
| O18 | real status ignored when finite OK | 4 / 1 | refine-status |
| O19 | exponent bound skipped for exact zero in refine | 1 / 1 | - |
| O22 | aggregate refusal names infinity | 2 / 1 | - |
| O23 | main-phase failure names the first prime | 0: SURVIVES (F8) | - |
| O24 | real failure returned before finite status | 3 / 1 | - |

Totals: 26 detected by test_gfunc_log, 3 equivalent, 8 survive. The driver cases detect 12 and none of the
survivors. Unmutated: 12 tests, 2194049 checks, 0 failed; all four driver cases pass with the self-linked adf.

## Checks run (commands from the repository root, all under timeout)

| Command | Result |
|---|---|
| `timeout 600 make -j2 BUILD=lanes/f-review13/build lanes/f-review13/build/libadelefeld.a` | exit 0 |
| `timeout 170 lanes/f-review13/build/test_gfunc_log` | 12 tests, 2194049 checks, 0 failed |
| `timeout 170 python3 -B lanes/f-review13/checks.py sets hprime` | 10840 set equalities; 1587 H' cases |
| `timeout 170 python3 -B lanes/f-review13/checks.py series` | 2736 units equal; truncation table above |
| `timeout 170 python3 -B lanes/f-review13/checks.py lib crt` | 63440 and 660 calls, 0 mismatches |
| `timeout 175 python3 -B lanes/f-review13/mutate.py <IDs>` (3 batches) | table above |
| probe timing lines of F9 | 5 ms; killed at 170 s; 8 ms; 8.8 s |

Not done: Julia under the mutants; the design's own oracle commands were not rerun (value hunt f-review10 exists).
Sources pending: none new.
