# Lane f-review10: result (bug hunt through Log on ideles)

Commit checked: f7a1a2f. Worktree: /home/tobias/Projects/adelefeld/.claude/worktrees/agent-a87f63a5747342d41
Findings: NONE reproduced. No BLOCKER, no MAJOR, no MINOR. Below: what was attacked, with counts and what would
have made a case fail; then three observations that are not findings.

## Method
- Library built once: `make -j2 BUILD=lanes/f-review10/build ...` (plain) and `make SAN=1 INV=1 BUILD=lanes/f-review10/asan`
  (ASan+UBSan, ADF_CHECK_INVARIANTS). No repository test suite was run.
- My own oracle `lanes/f-review10/w/o1.py`, from the definition, in exact Python integers, not importing
  proto/idlog_checks.py: Log(x) = log(x^(p-1))/(p-1) (odd p), log(x^2)/2 (p = 2), series
  sum_{n<=4H+40} (-1)^(n+1) t^n/n with t = x^e - 1 exact mod p^(H+5); each term is p-integral and the tail has
  valuation > H+3. It enumerates ALL units u mod p^H with u = c mod p^k (all units if k = 0, or p = 2 and k = 1),
  forms the set S = {Log(r' u) mod p^H}, and takes the smallest ball containing S (centre, radius p^j).
- C harness `w/h.c` (stdin lines `AT p N prec idele`, `REF N prec primes idele`), status program `w/s3.c`,
  drivers `w/t1.py t2.py t2b.py t5.py t6.py t7.py d.py`.

## Step 1: exact local image at one place (adf_idele_Log_at)
Command: `python3 lanes/f-review10/w/t1.py SEED COUNT` (seeds 1,2,3,4,7; 60+250+250+250+250 = 1060 cases).
Inputs: p in 2,3,5,7; H = 5 (H = 4 for 7); v_p(M) in 0..4 (0..3 for 7); M with other factors 3,5,7,9,11,13,15,25;
M = 0 with [1] and [-1]; M = 1 ("[1 mod 1]"); content r with v_p(r) in -3,-1,0,1,2,4 and other prime factors in
numerator and denominator; N in -2,0,1,..,7 (below, at, above E).
A case fails if: status != OK; some s in S has s != centre mod p^min(K,H) (BLOCKER); K != min(N, j) where j is the
true radius exponent of the smallest ball (MAJOR when N >= E); an exact zero returned where S != {0}.
Result: 1060 cases, 1033 compared as balls, 27 exact-zero returns each checked S = {0}; 0 failures. The true j
was E in every compared case (so the header's E = max(v_p(M), d) is the exact image radius).
Oracle power: the same run with the oracle altered by +p^(H-1) fails 7 of 120 cases ("not contained").
Also: non-normal stored units `[13 mod 8]`, `[-3 mod 8]`, `[1 mod 2]`, `[3 mod 2]`, `[5 mod 1]`, `[7 mod 6]`,
`[-5 mod 6]` give the same ball as the normal form (by hand, 12 lines, values read off the harness).
- Large primes and exponents: `t6.py` 120 cases, p in {101, 65537, 1000003, 4294967291, 2^61-1, 2^63-25, 2^64-59},
  v_p(M) 1..3, centre compared mod p^K to my oracle: 0 failures. `t7.py` 60 cases, K in {20,64,65,130,300,1000},
  p in {2,3,5,7,11}, exact and `mod p^K` units: centre compared mod p^K: 0 failures.
- A ball strictly larger than the image at N >= E: none (above).
- Canonicity: every returned sball (and every adele, below) passed adf_sball_is_canonical / adf_adele_is_canonical
  (601 of 601 OK results in `asan_in` run).

## Step 2: refinement by CRT (adf_idele_Log_refine)
Command: `python3 lanes/f-review10/w/t2.py SEED COUNT` (400 + 150 seeds), `t2b.py` (150, N in 10..129,
M up to 2^70 and 3^15). Lists of 0..4 primes from {2,3,5,7,11,13} in shuffled order, M in 18 values incl. 0,
N in -1..5 and 10..129. For each case the oracle takes the per-place balls from adf_idele_Log_at (validated in
step 1) and requires: R = 2^max(2,K_2) * prod over odd listed p of p^K_p (K_p = N for an exact zero, factor 1 for
K_p <= 0 at odd p); A = 0 mod 4; A = centre_p mod p^K_p at each listed p; 0 <= A < R. Unlisted primes are
checked through R (no factor). Fails otherwise. Result: 700 cases, 0 failures.
- Edge shapes (w/s3.c, w/h.c): repeated prime -> DOMAIN where=3; infinity in list -> DOMAIN where=inf; n = -1 ->
  DOMAIN where untouched; n = 65537 -> LIMIT where untouched; n = 0 with NULL -> OK; 65536 distinct primes
  (all primes < 821642, N = 1) -> OK in the harness; unsorted lists correct (above); composite "primes" cannot be
  built (adf_place_prime(4) fails) and the driver gives DOMAIN for 4.
- CRT bound (idele (1;1*[1]), primes 3,5, exact zero at both, so R = 4*3^N*5^N): N = 13421773 -> LIMIT, N = 13421772
  -> OK (7.3 s). The boundary is where 4 + 5N crosses 2^26, as the header's formula says.
- Local limit before aggregate: N = 10^8 with primes 3,5 on (-2;3*[-1]): LIMIT where=5 (3 is exact zero, exempt
  from the power bound; 5 exceeds K > BITS_MAX/3), as the header orders it.

## Step 3: statuses, where, untouched outputs (w/s3.c, 34 lines; run plain and under ASan)
Read off, with y prefilled and compared by identical():
- negative I: Log DOMAIN where=inf, y unchanged; log_abs OK; Log_at inf DOMAIN where=inf; log_abs_at inf OK;
  Log_at at prime 3 OK (the real part is not looked at).
- prec = ADF_REAL_PREC_MAX OK, where untouched; prec = MAX+1: LIMIT where=inf for Log, log_abs, Log_at inf,
  log_abs_at inf; log_abs_at at prime 3 with MAX+1 -> UNSUPPORTED where=3 (as the header: LIMIT only at infinity);
  Log_at at prime 3 with prec 3,000,000 (M = 4) OK; prec 0 and -5 OK.
- exact [-1], r = 3: p = 2, N = 10^8 -> LIMIT where=2; p = 5 with N = +-(2^60+1) and N = 2^60 -> LIMIT where=5;
  r = 5 at p = 5 (exact zero) with N = 10^11 and 2^60+1 -> OK (exact zero before the check, as the header).
- refine: precedence prec>MAX (LIMIT, inf) over n<0; n<0 DOMAIN where untouched; repeated prime beats negative I;
  negative I with a local LIMIT -> LIMIT where=3 (10 > 7 numerically, as the header's maximum rule);
  unsorted (5,3) with local LIMIT at both -> where=3 (smallest prime); log_abs_refine with negative I OK.
- All failures: y unchanged (every line printed UNCHANGED); where written on every failure, never on OK.
- Real side: `t5.py` 400 random real balls (mid +-1e-250..1e250, relative radius 1e-8..1e-1, prec 10/30/80,
  digits 15) x {Log, log_abs} x {all-places, Log_at inf, log_abs_at inf} = 2400 outputs checked against mpmath
  (60 digits) for enclosure of [log|lo|, log|hi|] of the actual input ball (slack 1e-14 relative for printing);
  negative mids must give DOMAIN for Log/Log_at and OK for log_abs: 0 failures. With the target shrunk by 1e-9 the
  same script reports 42 failures, so it can fail.
- Not reached: NOT_DETERMINED from a non-finite real enclosure (no idele input produces one); not tested.

## Step 4: memory
`lanes/f-review10/w/ha` (harness) and `w/s3a` (status program) linked with the ASan+UBSan+INV library,
`ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1`. LSan itself verified to work here (a deliberate
77-byte leak is reported). Reproducer 1: 604 lines (300 AT, 300 REF with random lists, N in -3..60, 65536 primes,
65537 primes, repeated prime, negative I): rc 0, no report. Reproducer 2: the 34 status lines (all failure paths
of Log, Log_at, Log_refine incl. the `known` re-evaluation path): rc 0, no report. Would fail on: any ASan/UBSan
report, any LSan leak, any invariant abort.

## Step 5: driver (w/adf built from tools/adf/adf.c; `python3 w/d.py`, 52 lines, each in its own process)
Wrong operand types (rational, ucoset, adele operand: DOMAIN), missing `with` (PARSE), two `with` (PARSE), two
places for Log_at (PARSE), `3,5` (PARSE), `inf` (PARSE), `real` (OK for Log_at, DOMAIN for refine), `0`, `1`, `4`,
`2^64`, `-3` (DOMAIN), p = 18446744073709551557 (Log_at: `p: 0 + O(p^1)`; refine `(0 ; 0 mod 73786976294838206228)`
= 4p), `with none` (equals Log), `None`/`none 3`/empty list (PARSE), repeats (DOMAIN), Log of (0;..), (1;0*..),
`[2 mod 4]` (DOMAIN), negative real (DOMAIN, log_abs OK), log_abs_at at prime (UNSUPPORTED), `prec 0`/`-1`
(setting is DOMAIN, not a Log issue). Printed values equal the library's: `Log_at (1;4*[5 mod 2187]) with 3` at
prec 40 prints `114 + O(3^7)` = my oracle's logp(20,3,7) = 114; refine docs examples `(0;12 mod 36)`,
`(0;120 mod 180)` reproduced and checked by hand (3 mod 9; 120 = 3 mod 9, 0 mod 5, 0 mod 4). 0 defects.

## Observations (not findings; not asserted as defects)
1. Existing driver grammar: a trailing space or tab after the place list (`... with 3 `) is PARSE; the old
   `project X with 3 ` does the same, so this is the project grammar, not Log.
2. No cheap resource refusal near the BITS bound: `AT 3 33554400 5 (1 ; 2 * [-1])` (K just under 2^26/2, exact
   unit, p = 3) did not return in 60 s (it computes). Only K above the bound is refused at once (0.35 s). This is
   inside the stated limits ("Cost"), but a caller sees a minute-long call; the driver's prec cap hides it.
3. Log_refine at a prime where lball_Log would refuse but the pre-check does not (K just below 2^26/bits(p)) and
   whose aggregate sum exceeds the bound returns LIMIT with where untouched, while Log_at at that prime returns
   LIMIT with where=v (`AT 3 33554432 ...` -> st=10). The header orders "known local exponent/compact-power
   failures" before the aggregate; this working-power failure is not "known" by the pre-check, so the behaviour
   follows the header's wording. The Log_at half was run; the refine half is read from refine_budget, NOT run
   (it needs a call that would first spend minutes), so it is a reading, not a reproduction.

## Not done
- No long differential fuzz run (the cases above are seeded random batches of 60 to 400, not a long run); no
  mutation testing of src/gfunc_log.c.
- v_p(M) > 5 compared only by centre (t7: K up to 1000), not by full enumeration.
- Enumerations only for p in 2,3,5,7 (as briefed); other primes only via the centre check (t6).
- NOT_DETERMINED at infinity not reachable; invalid (non-canonical) inputs under ADF_CHECK_INVARIANTS not
  attacked; aliasing is excluded by contract and not attacked.
- Work and oracle files are under lanes/f-review10/w (no files outside the lane were changed except the symlink
  refs/src).
