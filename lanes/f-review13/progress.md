# f-review13 progress notes

2026-10-04. Worktree at 58659bb. Read COMMON.md, CLAUDE.md, brief, docs/design/idele-log.md (679 lines),
docs/api-1f8.md:274-475, src/gfunc_log.c (373), tests/test_gfunc_log.c (562), proto/idlog_checks.py,
lanes/f-slice11/write_selection.py and write_crt.py, tests/driver/gfunc-log-*, gfunc.h:182-300,
SPEC 9.3.2, ideles.md Def 4, Lemmas 5, 7, Prop 6, functions.md Prop 4, Lemma 9, Props 11, 12.

Archive built once: `timeout 600 make -j2 BUILD=lanes/f-review13/build lanes/f-review13/build/libadelefeld.a`
(exit 0). test_gfunc_log built and run unmodified: 12 tests, 2194049 checks, 0 failed.

## Own tools (all in lanes/f-review13/)

- il_oracle.py: Log mod p^H WITHOUT the log series. Torsion removed by a^(p^(H-1)) (odd p) or the sign (p=2);
  the principal unit inverted through a table of exp(y), y in p^d Z/p^H, exp summed in integers with the
  Legendre bound v(y^j/j!) >= j d - (j-1)/(p-1) (increasing in j) to cut the sum. Injectivity of the table
  is asserted (p^(H-d) distinct values). H = 5 at 2 and 3, H = 4 at 5 and 7; all comparisons mod p^H.
- checks.py: sections sets, hprime, series, lib, crt. probe.c: raw library calls from stdin.

## IL1 (local input set): checked, sound

Claim: X_p = r c(1+p^k Z_p) for k>=1; p^m Z_p^x for k=0 odd; r + 2^(m+1) Z_2 for k=0 or k=1 at 2; {r c} for M=0.
Step 1: Lemma 5.1 gives u_p = c mod p^k; for k>=1 c is a p-unit so (c+p^k Z_p) meets Z_p^x in all of it. Product
structure: Lemma 5.1 is a conjunction of conditions on separate coordinates and Zhat^x = prod Z_p^x
(tate-poonen notes:1603-1605). Steps 2-6 immediate. Step 7: v_2(M)=1 is NOT excluded: the predicate
(ucoset.c:95-109) admits (1,2), (5,6); the driver parses "[5 mod 6]"; only results of operations are in normal
form (ucoset.h). The design handles k=1 (IL1.7, IL3.4) and the code does (gfunc_log.c:134 and E=max(k,2)).
My grid includes M = 2, 6, 10, 30 at p=2 and every library call there agrees with the enumeration (section lib).

## IL2, IL3, IL4 (exact images): checked, sound; equality of sets, not only inclusion

checks.py sets: 10840 cases (p in 2,3,5,7; M = p^k * {1, q, q', q q'}, k=0..3; up to 10 residues c per M,
all when phi(M) <= 10, so c != +-1 with r'=1 occurs; contents r = p^m r', m in -2,0,1,3, 8 prime-free r').
Enumerated image of IL1's input set mod p^H == Log(r'c) + p^E Z_p (resp. p^d Z_p, singleton) and
|image| = p^(H-E). A one-sided failure (inclusion only) would show as a smaller image; none.
r' contributes, p^m does not: the grid has r' = 2, 5, 3/4, 7/11, p+1 with m != 0. At 2: k=2 gives 4 Z_2
(Log(r'c) in 4 Z_2), k>=3 gives translates; c = 3 mod 4 occurs (c=M-1). Kernel {+-1}: exact c=-1 rows.
IL4 surjectivity: proved in functions.md Lemma 9 step 5 (formal inverses map the discs into each other),
not cited from outside; Log(r') in p Z_p for every p-unit r' since Log = log(u), u in 1+p^c Z_p (Prop 4).
"Negative r'" occurs only as r'c with c=-1 (r>0); Log(-1)=0 (Prop 11 step 2). Residue order always divides
p-1, so the brief's "p-1 not dividing the order" case is empty.
Library against the same oracle (checks.py lib): 63440 Log_at calls, N in {-1,0,1,2,E-1,E,E+1,H}, fields equal
to the smallest ball of exponent min(N,E) computed from my enumeration (exact zero only for M=0, r'=1):
0 mismatches.

## IL5 (all finite places, CRT): checked, sound

Step 1 (product is the image): Lemma 5 + surjectivity of IL2-IL4, preimages chosen per coordinate. Step 4 (no
ball equals D or the image): projection Z_p outside the supports of H, d, A, M versus J_p = p Z_p. Step 6: if
K_p <= beta_p then B_p = j + p^K Z_p with j in J_p subset p^beta Z_p, so B_p contains p^beta Z_p; if K_p > beta_p
the canonical centre is = j mod p^K, hence in p^beta Z_p, and B_p is inside the baseline. Step 7: z in Zhat
with z_p in b + p^L Z_p is the congruence z = b mod p^L Zhat; CRT. Smallest? Not claimed (IL5 "generally
neither the image nor its smallest global ball"; header "exactly C_S"). No smallest finite ball containing the
image exists for M > 0: for any containing ball and an odd q outside its support and M, intersecting with
z = 0 mod q gives a strictly smaller containing ball. "0 + 4 Zhat also for exact input" is an enclosure by
Prop 12 step 4; N-D17 records it as a choice. checks.py crt: 660 Log_refine calls (11 inputs incl. v_2(M) = 1,
3, 4, c not +-1, exact; 10 lists incl. 7 and shuffles; N = -1..4), expected (a, R) built from my enumeration
and a CRT by search (no modular inverse): 0 mismatches.

## IL6 (enumeration bound H' = H): checked, sound (needs k <= H, asserted by the oracle)

Step 1 is Prop 11's isometry on 1 + p^c Z_p applied to b/a in 1 + p^H Z_p, H >= d. checks.py hprime: 1587 cases,
image from units mod p^(H+1) reduced mod p^H equals image from units mod p^H. Counts in steps 4-5 recomputed.

## IL7 (series, tail, routes): checked, sound; "independent routes" share the series

Tail: j >= p^e >= 2^e >= 2e, so term valuation j v(z) - e >= j/2 >= H for j >= 2H; holds at p = 2 (v(z) >= 2)
and p = 3. Working digits W = H + floor(log_p(T-1)) >= H + e for j < T. checks.py series: design oracle
log_unit == my exp-inversion Log on all 2736 units (p=2,3 at H=5; 5,7 at H=4). Truncation sensitivity at H=5:
p=2 wrong for T=2 (8 of 16), right for T>=3; p=3 wrong for T<=4 (108..138 of 162), right for T>=5. T = 2H = 10
is safe, not tight. Routes: Teichmueller lift, powered route and 2-adic square route all call the same
log_principal; they are independent only in torsion removal. A fault in log_principal that keeps the
isometry (e.g. global sign) would pass check_kernel; the design's two witnesses (log 4 = 3 mod 9,
Log 3 = 4 mod 8) and my exp route catch it. The design says correctness rests on the series proof. No finding.

## IL8 (oracle function and fixtures): checked; fixtures share the code's construction

expected_ball is IL2-IL4 in constructive form: centre = Log of (r'c reduced mod p^L) at L = min(K,E), exact iff
M=0 and r'=1, centre 0 at unrestricted primes. That is also gfunc_log.c's construction (compact residue,
lball_Log at K). Its validation is check_images (enumeration, same log_unit) and my independent oracle above.
Fixture provenance: local.jsonl (6784 rows) all from expected_ball; all lie in the design grid at N values
check_images/check_exact cover (H=5 and H=6 runs). BUT write_selection.py keeps only c in {1, M-1, p}: at a
restricted prime c = p is excluded by gcd, so all 4973 restricted rows have c = +-1 mod p^k (scan of the file),
Log(c) = 0 in every row, and all 20 complete-image rows (r=1, c=1, M=p^k) have centre 0.
crt.jsonl (280 rows) from expected_refinement + crt, same construction as refine(); check_crt validated 4 inputs
x 6 lists x N in -2..3. Not validated by check_crt (regression only): 56 rows of input (4,1,1,0), 64 rows with
N in {4,6}, 20 rows with list (7,5,3) at N <= 3: 140 of 280. My crt section covers those shapes independently.
real.jsonl from mpmath (independent). Driver goldens hand-written.

## G7: checked, sound. conservative(): prec check before INV and allocation; real ball via sball_Log_at at
infinity, copied by get_arb (test compares arb_equal with adf_real_log); (0,4,1) written into a temporary,
swapped only on OK; where = inf on every failure.

## G8: checked, sound. describe(): K = N exact, min(N,E) else; m only bounded (never added to k). Shortcut
list equals the proof's branches. Step 6: t/A in 1+p^K Z_p and K > d, so Log(t) = Log(A) mod p^K.

## G9: checked, sound. exp_ok on K by two comparisons; compact power check K <= 2^26/bits(p) before
fmpz_pow_ui; lball_Log checks its own W. Exact-zero shortcut before the K check (WORD_MIN/WORD_MAX N fine).

## G10: checked, sound (crt section above). p = 2 first after sorting; replacement at L > 2 keeps 4 | b since
J_2 subset 4 Z_2. fmpz_CRT(sign 0) preconditions 0 <= r1 < m1, 0 <= r2 < m2, coprime: hold.

## G11: checked; one MINOR (cost). Ordering prec -> n<0 -> n>65536 -> sort/shape -> budget -> evaluation is
as stated. Budget charge comparisons exact (floor division). Step 4: on a known local refusal, every earlier
prime is fully evaluated, even when the aggregate is already refused. Measured with probe:
x = (2 ; 2 * [1]) exact, N = 1048577, S = {3,5,...,47}: LIMIT, where untouched, 5 ms;
S + {2^64-59}: killed by timeout 170 s (14 full local Logs at 2^20 digits before LIMIT at 2^64-59);
{2^64-59} alone 8 ms; {3, 2^64-59} 8.8 s. G12 gives "substantial work for a request whose combined modulus
is refused" as the reason for its order; this input does that work. Status and where are as documented.

## G12: checked. Pending items match those of the design (Iwasawa name, Teichmueller name, real Lemma 2).
Nothing in the header is unproved beyond delegation to sball_Log_at / adf_real_log / lball_Log.

## Part 2 (mutate.py, distinguish.txt)

37 faults planted one at a time in build/mut/<id>/gfunc_log.c; each: test_gfunc_log, the four driver cases
(adf.c linked against the mutated archive inside build/, byte comparison and #!exit), probe on distinguish.txt.
Julia not run (no PIC archive; a second full build is outside the brief). Result: 26 detected by
test_gfunc_log, 3 equivalent (B3, B5p, B12b), 8 survive (B9, B9c, B9d, B10, B11, O1, O3, O23).
Each survivor is distinguished by a probe line (baseline vs mutant recorded in build/mut/results.jsonl).
