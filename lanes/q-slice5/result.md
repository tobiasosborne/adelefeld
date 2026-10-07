# Lane q-slice5: the character on classes and at places (slices 3.2-b and 3.2-c)

All eight calls of the brief are implemented as declared in design 3.2 (no HEADER-FINDING), end to end: header,
code, tests against the oracle, driver, Julia, statements. Worktree /home/tobias/Projects/adelefeld-wt/q-slice5,
nothing committed (no git or bd command that changes state was run).

## Files

- include/adelefeld/psi.h: the eight declarations with the design's comment blocks and the sentences that fix
  what the design leaves open (status order, where = v on failure, strict at infinity, the bit rule for p^k).
- src/psi.c: the adele hull split into psi_exact / psi_numeric / psi_fold / psi_commit (behaviour unchanged:
  test_psi 186799 checks before and after); new psi_class, adf_qclass_psi_tate{,_strict,_phase},
  psi_ppow, psi_fp, psi_local_read, psi_image, adf_lball_psi_tate{,_strict,_phase}, psi_at,
  adf_adele_psi_tate{,_strict}_at. Sources cited at each function (file:line).
- tests/test_psi_class.c, tests/test_psi_local.c; tests/ref/vectors/q-slice5/{class,local,place}.jsonl
  (380391 bytes) from lanes/q-slice5/gen_vectors.py (proto/quotient3_checks.py as oracle).
- tools/adf/adf.c (psi commands only), tools/adf/README.md (psi section), tests/driver/psi-class.{cmd,out},
  tests/driver/psi-local.{cmd,out}, tests/julia/psi_class.jl and its block in tests/test_julia.sh.
- docs/api-3b.md: appended "Slices 3.2-b and 3.2-c".
- lanes/q-slice5/: redgreen.md, run_faults.py + faults.log, mutants.txt + mutate.log, recheck_survivors.py +
  recheck.log, psi-class-union.{cmd,out} (pending fixture, see "Not done").

## Work per step

A. `timeout 120 python3 -B lanes/q-slice5/gen_vectors.py`: exit 0, class.jsonl 32 records (11 lifts incl. E1-E4,
the arc about 0, 2/3 and 1/(2^2000+1) radii, a 2000-bit centre; 8 lifts reduced IN C by adf_qclass_reduce, whose
1 to 6 rounded pieces the test first compares exactly with the oracle's reduce + round_piece; 13 hand-built
canonical PIECES values), each with the exact union of arcs (or null), the four union distances (min over entries,
checked in the generator against the extrema of the materialized arcs), D3-3 strict status, class phase or null,
40 points per entry; local.jsonl 432 records (p = 2, 3, 5, 7, 65537, 18446744073709551557; 9 centres incl.
-1/6, 1/6; exact and e = -3..3; root families checked against enumeration); place.jsonl 30 adeles with the
local image at infinity, at every prime of den(a) den(N), and at 11, 13 (oracle asserts the sum of local phases
= global phase for singletons).

B. Tests (all expected values from the oracle or derived by hand in comments before the code):
- test_psi_class: vectors at p = 2, 20, 53, 128 (statuses, default = strict on OK, hull bound on both
  coordinates of the UNION: 256 coordinate checks, arcs, 10720 sampled points with exact membership and phase);
  e3(): the D3-3 sentence (lift NOT_DETERMINED, reduction OK, both enclose +1 and -1, line hull), a unit square
  rejected by the hull test, "first piece only" misses -1; golden(): 15 valid psi_phases.tsv rows as lifts
  (class = adele, strict and getter by angle count); statuses(): prec LIMIT first, exact-preflight LIMIT of a
  later entry beats everything (also strict on a fractional lift with an over-bound centre), certificate
  NOT_DETERMINED at the cap from one entry, getter: equal singletons OK (3/4), different singletons and real
  radius NOT_DETERMINED, LIMIT first; additivity(): 100 pairs, 10 sampled sums each; INV: forged class aborts
  all three calls. All outputs seeded with sentinels and compared byte-for-byte on failure; input unchanged.
- test_psi_local: local vectors (getter, strict, hull bound, roots, base phase); witnesses(): exact local 1/6 at 2
  (1/2, z = -1 exactly) and at 3 (2/3), e = -1, 0, 1 at p = 2, the 8th roots, 300 exact additivity triples;
  limits(): prec, exponent bounds, 2^(cap-1) admitted / 2^cap LIMIT, the formed-then-rejected window at p = 3,
  v = -10^6 and e = -2^40 at the largest 64-bit prime (LIMIT without forming the power), place LIMITs with
  where = v, the component not used is not read, strict ND at p with v_p(N) < 0, strict at real with real
  radius OK and the sign E(-1/4) = -i, numerical ND at the cap, DOMAIN for a raw non-finite real ball;
  place vectors: both where forms (NULL, sentinel untouched on OK), E(0) = 1 exactly at primes not dividing,
  exact local angles via adf_lball_set_fball + getter, their sum = global getter, 20 sampled global points in
  the product of local enclosures; golden rows at their primes; INV: forged lball (p = 4), forged place handle,
  non-finite adele abort.

C. Code: see Files and docs/api-3b.md. Class: pass 1 exact preflight of every entry (status maximum, LIMIT
final), D3-3 strict, pass 2 certified bounds folded into the union extrema, one Q1 rounding per coordinate.
Local: fp_p from v and u (no power for v >= 0), p^k bit rule before forming, the hull of Q4 with B = p^(-e).
Place: E(-I) at infinity; at p, powers of p removed from den(a), den(N) with fmpz_remove.

D. Driver: psi / psi_strict accept adele, class (lift text) and local ball; new psi_at, psi_strict_at (places
named "real" or a prime, as adf_drv_place_token of the other commands); new psi_phase (fball, adele, class,
lball). Fixtures written by hand before the run, 16 + 26 lines, equal on the first run.
`timeout 900 sh tests/test_driver.sh`: 74 cases, 101390 expected lines, all equal (72 before + 2).
Julia: `LD_PRELOAD=<system libgmp> julia --startup-file=no tests/julia/psi_class.jl <lane .so>`: 18/18 pass
(without the preload the known __gmpn_modexact_1_odd load error, which test_julia.sh retries as for psi.jl).

E. docs/api-3b.md "Slices 3.2-b and 3.2-c": statements with proofs of the enclosure steps, the union excess
bound (same formula, W = width of the union hull), why first failure of pass 2 is the maximum (PIECES
distances are dyadic, so the numerical LIMIT branch is unreachable for len > 1), statuses, limits, costs,
Check lines, decisions with alternatives.

F. Faults (`timeout 2400 python3 -B lanes/q-slice5/run_faults.py`, scratch copies, removed; base passes):

| Fault | Result | First failing assertion |
|---|---|---|
| F1l local finite sign reversed | rejected | test_psi_local.c:312 |
| F3 ordinary fractional part at p | rejected | test_psi_local.c:307 (1/6 at 2) |
| F4 A roots instead of B (class) | rejected | test_psi_class.c:263; test_psi_local.c:245 |
| F4l A roots instead of B at a prime | rejected | test_psi_local.c:398 |
| F5 end points only | rejected | test_psi_class.c:219 and test_psi_local.c:219 (hull) |
| F6c whole square, class | rejected | test_psi_class.c:269 (line hull) |
| F6l whole square, local | rejected | test_psi_local.c:321 |
| X1 only the first stored entry | rejected | test_psi_class.c:268 (-1 missing) |
| X2a strict from the first entry only | SURVIVED: equivalent | see below |
| X2b strict as singleton image | rejected | test_psi_class.c:391 |
| X3 branch at e = 0 wrong (order p^(1-e)) | rejected | test_psi_local.c:315 |
| X4 real place with the finite sign | rejected | test_psi_local.c:404 |
| X5 where written on OK | rejected | test_psi_local.c:94 |
| X6 power formed before the bit check | rejected | test_psi_local exit 136 (SIGFPE in the power) |
| X8 cofactor of den not inverted | rejected | test_psi_local.c:312 |

Faults 1 and 2 of design 5 (adele signs) were planted by q-slice3; F1l is the local analogue. X2a is equivalent
on canonical inputs: only a LIFT (one entry) can have fractional N, since PIECES entries have d = 1 and integral H.

Mutation: `ASAN_OPTIONS=detect_leaks=1 timeout 1250 python3 -u -B tools/mutate/mutate.py --root . --scratch
<scratchpad>/mutate --files src/psi.c --limit 60 --seed 320511 --jobs 1 --timeout 150 --san --make "make -s -j2
check INV=1 TEST_SRC='tests/test_psi_class.c tests/test_psi_local.c'" --copy Makefile include src tests`:
532 candidates, 60 run in 1026.8 s: 44 killed, 12 survived, 4 not compiled, 0 timed out. 38 of the 60 lie in the
3.2-a code, which these two tests do not target. `timeout 900 python3 -B lanes/q-slice5/recheck_survivors.py`:
7 of the old-code survivors are killed by test_psi (52, 53, 124, 10, 154, 290, 161). Survivors, one line each:
- psi.c:186 &&->|| in psi_distance: equivalent, argued by q-slice3 (multiplying 0 or by 1); test_psi also passes.
- psi.c:300, :307 prec LIMIT -> OK in the public adele wrappers: equivalent, psi_adele repeats the same test.
- psi.c:257 < -> <= in psi_fold: equivalent, it sets an equal value.
- psi.c:290 drop `st = ADF_NOT_DETERMINED` in psi_adele: killed by test_psi; test gap of the new tests closed
  (test_psi_class golden() now checks adf_adele_psi_tate_strict per lift).
- psi.c:507 `strict = 0` -> 1 at infinity: the statement was redundant (B = 1 there); removed from the code.
- psi.c:52, 53, 124, 10, 154, 161: 3.2-a code, killed by test_psi (recheck.log).
No equivalent.txt entry was added.

## Final checks (on the final code; at most -j2; every program under timeout)

    timeout 600 make -s -j2 BUILD=lanes/q-slice5/<v> [SAN=1|INV=1|CC=clang] lanes/q-slice5/<v>/test_psi \
      lanes/q-slice5/<v>/test_psi_class lanes/q-slice5/<v>/test_psi_local
    ASAN_OPTIONS=detect_leaks=1 timeout 600 lanes/q-slice5/<v>/<test>

All exit 0. plain, SAN (LeakSanitizer active: a probe leak of 77 bytes is reported in this environment) and
clang: test_psi 186799, test_psi_class 148763, test_psi_local 165418 checks; INV: 186799, 148772, 165412
(INV adds the child-abort checks and drops the normal-build DOMAIN checks). Driver: 74 cases, all equal.
Build trees, scratch copies and the mutation scratch are removed; no log above 100 KB. All written lines
<= 116 characters (the pre-existing long lines of adf.c and README.md are not mine); files end with a newline.

## Findings against the design / brief

1. The brief asks for local balls and a LIMIT test at a 2000-bit prime. adf_lball and place handles hold one-word
   primes (lball.h:78 "2 <= p < 2^64"; place.h; conventions 7). Used: 18446744073709551557, e = -10^6, -2^40.
2. D3-3's per-entry certificate is decided by a single entry in every canonical class (fractional N only in a
   LIFT), so "strict deciding from one entry" is unobservable. Not a defect; worth a sentence in the design.
3. Statuses "combine by maximum" holds while pass 2 stops at the first failure, because the numerical LIMIT
   branch (odd-denominator doubling) is unreachable for PIECES entries (proof in api-3b.md).

Decisions where the design is silent (alternatives in api-3b.md): p^k bit bound = ADF_QCLASS_BITS_MAX (not
ADF_LBALL_BITS_MAX); u of an lball not size-bounded; lball getter LIMIT = read bounds only (as the adele
getter); place calls read only the component at v; raw non-finite real ball DOMAIN at every place; *where = v on
every non-OK status; strict at infinity = default.

## Not done

- The union text in the driver: `adf psi 'union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q'` answers UNSUPPORTED until
  slice 3.1-d (lane q-slice4) lands the union reader. The C tests build that class by hand and test it. A
  hand-derived pending fixture is in lanes/q-slice5/psi-class-union.{cmd,out}, to append to
  tests/driver/psi-class.* after the merge (its second union assumes the reader accepts canonical key order).
- check-all and test_julia.sh as a whole were not run (orchestrator); the Julia script was run directly.
- Avoidable cost noted: pass 2 of the class recomputes each entry's exact distances instead of storing them.
- faults.log was produced before two final edits (the removed redundant `strict = 0`, a header comment);
  neither touches a planted line.
