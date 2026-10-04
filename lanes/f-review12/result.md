# f-review12: bug hunt through src/catalogue.c (lane f-slice13)

## Findings

None at BLOCKER, MAJOR or MINOR in `src/catalogue.c` / `include/adelefeld/catalogue.h` / the eight driver commands.
No input of mine made the code return a value outside the true set, an OK that a point of the input contradicts,
a coarser-than-finest modulus, a non-smallest tight radius, a wrong status, a touched output, or a sanitizer report.

Out of scope, noticed only: `tools/adf/a.out` (24580 bytes of data, tracked since commit 674acfe, WP 1.5) is a stray
binary in the repository. Not from this lane.

Two failures that I saw were bugs of my own oracle, not of the code (k=0 conservative radius is 0, as the header
says; v_2(F)=1 is canonicalised away also in the branch p | N). Fixed in the oracle, then rerun.

## What I attacked, with counts (all oracles written by me from the definitions, in Python exact integers;
## I did not import proto/catalogue_checks.py and did not run the repository suites)

Binomials (`orc.py`, `orc5.py`, `h3.c`)
- 6000 random (a, N, d, k) x 2 functions against: radius formula N/gcd(N,k!) (0 for k=0 or N=0), and for tight the gcd
  of differences over j = -3..4k+11 (a wider window than the proposition's j=1..k); values at j = -7,-1,1,2,3,10,101,-100
  checked inside the returned ball; centre congruent to binom(a,k); output canonical. Ranges: k 0..30, N 0..5000
  and special (1,2,3,4,6,8,12,16,24,27,30,32,48,64,720), a negative, zero, a<k, k=0 and 1. 0 failures.
  Same again 3000 x 2 under ASAN/UBSAN with ADF_CHECK_INVARIANTS: 0 failures.
- Nonintegral inputs (about 15% of the cases, d in 2,3,4,6,9,10,12): the status DOMAIN / NOT_DETERMINED against my
  enumeration of z mod d for whether (A+Nz)/d meets Z: 0 failures.
- 8 tight cases with a up to 700 bits, N up to 300 bits (also N=0, factorial multiples), k 64..256 against the gcd over
  j = -2..k+2 and 2 random j: 0 failures. 6 conservative cases at k 1000..4096: formula and containment: 0 failures.
  (Python made larger sizes too slow; the 2000-bit a with N at 2000 bits and k=4096 / 256 was run only for
  status OK and no crash in h3.c, not for value.)
- PLAN vector (0,8,4): conservative 1, tight 2; Y9 vector (2,3,2): tight radius 9: both as stated.
- Statuses (h3.c, 2 functions x 7 inputs x 8 k values, k in 0,1,4,256,257,4096,4097,100000): LIMIT before DOMAIN, the
  tight limit 256 and conservative 4096 exact, value output identical to the sentinel on every non-OK, `where` bytes
  unchanged (memcmp), where = NULL, y aliased with x (2000 random cases x 2 functions), a local-backend ball
  (modctx blocks 8,9,5, k 0..8) equal as set to the global result: 0 failures.
- Identities (1500 cases): tight ball inside the conservative ball; Pascal on exact integers: 0 failures.

Profinite power (`orc2.py`, `orc3.py`, `h3.c`, driver)
- Small oracle (`orc2.py`): for each prime power q^t it enumerates all base residues b = c mod gcd(N,q^t) and tests
  b^e and b^M against the base b0 (one class iff b^e = b0^e and b^M = 1, derived from the definition with j=0,1);
  primes up to 300 and the primes of N; the finest modulus is the product of the largest one-class prime powers,
  with the lone factor 2 dropped. About 30100 cases x 3 modes (N in 1..400 incl. 1,2,4,8,16,32,64,128, twice odd,
  210; c in -30..400; e in -300..300 incl. 0, negative; M in 0..600 incl. 0 and 1). Strict: OK exactly when one class
  mod canon(N) (both directions), value c^e mod canon(N); coarse: modulus canon(gcd(N,c^M-1)) and c^e; fine: exact
  (centre, modulus) match; g > 256 gives LIMIT. 0 failures. (Cases whose true finest modulus exceeded my enumeration
  cap, about 1%, were skipped.) 2000 of them under ASAN/UBSAN+INV: 0 failures.
- Large N (`orc3.py`): N up to 2547 bits built from known primes (small primes with exponents, up to 8 primes of 40 to
  250 bits, some squared), c random with negative lift, e and M up to 2000 bits, g = gcd up to 256 and beyond (LIMIT),
  expected values from Proposition 13's table evaluated independently (v_p(c^M-1) by modular powers). 1629 cases (+84
  under ASAN) x 3 modes: 0 failures. This tests the table, whose small-case truth the first oracle established.
- Hand vectors through the harness and the driver: N=5,c=2,e=2,M=4 gives [49 mod 120]; N=5,c=2,e=0,M=2 gives
  [1 mod 24]; N=5,c=2,e=6,M=0 gives [1009 mod 2520]; [-1] with exponent 3+3Z gives NOT_DETERMINED strict and
  [1 mod 1] coarse/fine; [-1]^(3^2000) = [-1] in all modes; [1] with a nonintegral exponent gives DOMAIN (the domain
  check precedes the exact-base shortcut); [1] with e=257 in fine gives [1] (no LIMIT), [3 mod 7] with e=257 gives
  LIMIT and with e=256 OK; g=256 with M=512 OK, e=257, M=514 LIMIT; N=1 with g=2,3,4 gives 24, [1 mod 1], 240.
- Statuses, untouched value on NOT_DETERMINED/DOMAIN/LIMIT, `where` unchanged, y aliased with a for all three modes
  (9 exponents x 3 modes), LIMIT-vs-DOMAIN order: 0 failures.

Cyclotomic action (`orc4.py`, `h2.c`, `h3.c`)
- 9350 (c, N, n) cases over N in 0..120 (exact units +-1 included), n in -3..130 (n=1, 2, 2m with m odd, n<=0,
  n multiple of N and not): own enumeration of all unit lifts mod lcm(N,n); OK exactly when one residue; value c mod
  n (_exp_u) and its inverse (_exp_uinv); the odd lift for n = 2m; DOMAIN with j untouched for n<=0; NOT_DETERMINED
  with j untouched. 0 failures. Also 1624 under ASAN+INV.
- The SPEC vector, built through the public constructors (adf_idele_set_parts with inf = +3 and -3, r = p,
  u = 1 mod p^a and p^-1 mod M, then adf_idclass_set_idele): 6 primes (2..13) x a=1..3 x 2 signs x 5 values of M,
  n = 1..200 each: 27600 checks against my lift enumeration, including the negative real part (unit -c): 0 failures;
  the exponent under _exp_uinv is p mod n for n | N prime to p, and 1 on p-power n. The Y14 vector 19 mod 63 gives 3
  (n=7), 1 (n=9), 5 (_exp_u, n=7). The function uses x->u, which carries sign(X) and no content; that is the right
  representative.
- j aliased with n, 43 values of n x 2 functions: same status and value as the non-aliased call, j untouched on
  failure, `where` unchanged: 0 failures.

Memory
- h.c, h2.c, h3.c and the driver linked against a SAN=1 INV=1 build of the library, run with
  ASAN_OPTIONS=detect_leaks=1 (I checked that LeakSanitizer is active here with a deliberate leak): no ASAN, UBSAN or
  LSAN report in h2 (27600 checks), h3 (9298 checks), the oracle batches above, and the driver files.

Driver (`drv.txt`, `drv2.txt`, `drv3.txt`: 101 lines, ASAN build)
- All eight commands: operand types (integer, rational, ball, coset, class, wrong type), counts (missing, extra
  `with`, empty operand), k = -1, 2^63-1, 2^63, 2^64-1, 2^64, 4096/4097 and 256/257, k = +3 (PARSE), 03, 0x10 (PARSE),
  -0, 4.0 (PARSE), 6/3, 1/2 (DOMAIN); radius 0, -8, 0 mod 0; nonintegral balls with and without integral points;
  n = 0, -3, 1, 2, 3/2, 3 mod 2, class with t = -17 or 1/2; [0 mod 5], [2 mod 6], coset given as a bare number. Every
  printed value agrees with the library harness and with the vectors above; the error words are the statuses the
  header names (k = -1 and k = 1/2 print DOMAIN, not PARSE; that is a choice, not a contradiction of the header).

## What would have made a case fail
A true image value outside the ball (containment test), a tight radius different from the gcd of the differences, a
status different from the enumeration (one class / meets Z / one residue mod n), a modulus different from the product
of the largest one-class prime powers, a centre different from the CRT of b0^e, a value changed on a non-OK status,
`where` bytes changed, a sanitizer report.

## Not done
- Julia `catalogue.jl` and the Julia calls: not examined.
- Tight binomials with k at 256 and 2000-bit N and a were run for status and crash only, not value.
- Fuzzing for longer than the runs above: none. The counts above are fixed-seed randomised differential runs of
  seconds to two minutes each, not a long fuzz.
- Mutation testing: none.
- The brief's "N of 2000 bits, identities only" for profpow was done with a stronger check (value from the table).
- The proofs' cited sources (log theorem, unit-group exponents) are marked `[source pending]` in catalogue.md; I
  tested their consequences exhaustively for small cases, I did not read a source.

## Files
`lanes/f-review12/` : h.c, h2.c, h3.c (harnesses), orc.py, orc2.py, orc3.py, orc4.py, orc5.py (oracles), drv.txt,
drv2.txt, drv3.txt (driver lines), progress.md, result.md. Reproduce: build the archive as in the brief, link h.c with
`-Iinclude ... -lflint -lgmp -lm` to `lanes/f-review12/h`, then `python3 lanes/f-review12/orc2.py 6000 4`
(`orc.py N seed`, `orc3.py N seed`, `orc4.py N seed`, `orc5.py seed N`). Build trees and executables deleted.
