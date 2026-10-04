# Lane f-slice12 report

Both slices are implemented, in order. Slice A was recorded as finished in progress.md before Slice B began.
There are 13 exported public functions, driver calls and a Julia example. SPEC.md was not changed.

## Functions and decisions

Proposed N-D18 is in include/adelefeld/symbol.h, written before each slice's code.
The alternatives and statements Y1-Y8 are in docs/api-1f9.md.

- Slice A: adf_fmpz_legendre, adf_fmpz_jacobi, adf_fmpz_kronecker; the same three verbs on
  adf_fball and adf_ucoset. Exact integers and certified finite input sets return an exact -1, 0 or +1.
- Slice B: adf_lball_hilbert, adf_real_hilbert, adf_rat_hilbert_at, adf_idele_hilbert_at.
  Hilbert results are exact signs. Finite cases enumerate the permitted square classes.

The value is an int through a pointer and the return value is a status. Alternative: return the symbol
itself, which would confuse symbol 0 with OK. where is optional, after the value output. Every failure
preserves the value. Success preserves where. Legendre failures name their supplied place; Jacobi and
Kronecker failures name no place. Every Hilbert failure names its place; incompatible local primes
name the smaller prime. Exact zero gives DOMAIN; a zero-containing ball gives NOT_DETERMINED, with
DOMAIN taking precedence if the other input is exact zero. Canonical ideles cannot give DOMAIN.

Files: symbol.h and symbol.c, in the existing header/source style. Alternatives: three separate headers
or declarations in the existing input headers. The API uses existing types and introduces no new type.
Legendre takes an odd prime place, below 2^64; its numerator is fmpz, fball or ucoset. Alternative:
an arbitrary fmpz lower prime with a new certification/limit policy. Jacobi and Kronecker lower entries
are arbitrary fmpz integers. Nonintegral finite balls distinguish an empty domain from a mixed domain
by gcd(H,d)|A, rather than returning an unproved DOMAIN.

Slice A uses Proposition 2's sufficient modulus K. A finite input is certified only when K|N.
Alternative: Proposition 3 enumeration or a sharper conductor calculation. Exact inputs bypass that test.
Normalising the coset modulus cannot change K|N: K is odd or divisible by 8.
Slice B enumerates all compatible classes, rather than always requiring the sufficient precision bound.
Alternative: conservative refusal for every coarse input or a new object exposing both possible signs.
The idele unit includes the cofactor of the scale. Additive hull projection was rejected because it
would change the input set. No all-places Hilbert operation is offered; the product formula is a test.

No additional integer bit cap or factorisation is imposed. The largest place prime works. Hilbert only
reads valuation parity and up to three unit digits, so compact lballs at slong endpoints work without
the exponent limits of local arithmetic. Alternative: inherit its exponent cap and return LIMIT even
where no large power is needed. There is no LIMIT status in these symbol routines. Debug entry checks
are present for every public routine reading an adelefeld value. Raw fmpz and arb have no adelefeld
canonical-value predicate. Non-finite arb inputs receive DOMAIN as a documented courtesy.

## Proofs and sources

Y1 cites catalogue Definition 1:14-27 and FLINT's documented symbol domains:
refs/src/flint-3.0.1/fmpz.rst:1166-1172; ulong_extras.rst:456-462.
The special rules at lower entries 0, -1 and 2 are on disk in
refs/src/pari-doc/usersch3.tex:9266-9271, closing Definition 1's pending convention citation.
Y2 cites catalogue Proposition 2:31-39 and Proposition 3:43-57. Y3 proves the nonintegral-ball
classification step by step. Y4 writes out the zero-denominator multiplicativity counterexample.

Y5 cites catalogue Definition 4:63-78, Proposition 5:82-99 and Proposition 6:101-133, and proves
rational unit reduction. Y6 proves the primitive congruence oracle: p^2 for odd p, 16 at 2, after
square reduction to coefficient valuations 0 or 1. It uses on-disk Hensel statements at
refs/src/thorne-padic/jackthornenotes.txt:574-579 and :874-877. Dividing by a unit leading coefficient
makes each lifting polynomial monic. These close the source pending at catalogue.md:95 and :129
for these applications. Y7 cites catalogue Proposition 7:135-165 and explains zero statuses and
exact enumeration. Y8 cites the product theorem at refs/src/hilbert-bristol/lecture19.txt:89-91.
The Bristol display is not the formula oracle; its text discrepancies are described below.

The Python oracle uses only exact integers, fractions and standard-library code. It does not import
FLINT or proto/catalogue_checks.py. Legendre uses Euler's criterion and independent square counting.
Jacobi uses the defining prime product. Kronecker's supplement is evaluated arithmetically, rather
than by the C lookup. Hilbert signs come from primitive modular solutions, without a Hilbert formula.

## Test evidence

Three fixture files total 679803 bytes. Every row is read by tests/test_symbol.c:

- residue.jsonl: 3724 rows, 2628 exact and 1096 finite. There are 390 Euler/square-count comparisons;
  square residues are enumerated through 65537. Large-prime rows use Euler only. Numerators reach
  4096 bits, and known-factor Jacobi denominators have thousands of bits.
- hilbert.jsonl: 608 primitive-solution rows, including all 64 pairs at 2. Each row checks rational
  and exact-idele calls, and at finite primes exact and sufficient-precision local calls.
- hilbert_finite.jsonl: 1920 idele pairs and 800 applicable local pairs. The oracle searches all
  admitted reduced unit classes at 2, 3, 5 and 7, including determined coarse cases and ambiguity.

Additional C tests check multiplicativity, reciprocity, both supplements, numerator periodicity,
input aliases, local-backend fballs, all statuses, where and untouched values. Hilbert tests check
bilinearity in both arguments, symmetry, (a,-a)=1 and (a,1-a)=1 on their nonzero domains. The product
formula checks 2000 rational pairs at 12058 finite factors and 2000 real factors; support includes
all primes dividing numerators and denominators and 2. Word-prime tests include 3, 5, 7, 65537 and
2^64-59, independent Euler tests for (p,u), 4097-bit square coefficients and slong endpoint exponents.
Debug tests verify public-function entry diagnostics, both Hilbert arguments, and abort signals.

A wrong sign, status, reported place, changed sentinel, failed identity, or non-unit aggregate product
fails a case. Explicit ambiguity witnesses are 1 and 5 in 1+4 Z_2 against 2, 3 and 9 in 3 Z_3 against 2,
and unit pairs (1,1) versus (3,3) at 2. The rejected constant residue case is retained as a finding.
No output/input alias is permitted across the scalar and value types; same input objects are tested.

Red-green evidence is in redgreen.log. The first build had unresolved symbols (exit 2). After ND stubs,
the dedicated Slice A probe failed all 9 function assertions, and the Slice B probe failed all 4.
The first Slice A identity run failed 15 zero-denominator cases; Y4 records the mathematical reason.
The final ordinary and sanitizer/Clang runs have 11 tests, 719322 checks and 0 failures.
INV has 13 tests, 719376 checks and 0 failures.

## Planted faults

All 12 faults compiled with exit 0 and ran with exit 1. The numbers below are failed assertions.
The scratch copies and execution logs are under build/fault-*. The scripts alter no product source.

| Slice | Fault | Failing test | Failures |
|---|---|---|---:|
| A | wrong residues for supplement at 2 | residue_vectors | 917 |
| A | even numerator returns nonzero at 2 | residue_vectors | 7374 |
| A | coarse finite input accepted | residue_vectors | 354 |
| A | inverted Jacobi oddness guard | residue_vectors | 78134 |
| A | failure clobbers value | residue_vectors | 177 |
| A | missing/wrong where | residue_vectors | 163 |
| B | epsilon(u)epsilon(w) omitted | hilbert_vectors | 940 |
| B | rational valuation parity ignored | hilbert_vectors | 2130 |
| B | two real negatives return +1 | hilbert_vectors | 23 |
| B | scale cofactor omitted at 2 | hilbert_vectors | 67 |
| B | coarse local ball accepted | hilbert_statuses_and_precision | 387 |
| B | wrong odd-prime reciprocity sign | hilbert_vectors | 971 |

## Mutation testing

The bounded staging tree copies Makefile, include, src, tests and lanes. Its matching archive has
SAN=1 INV=1. The command compiles the mutated symbol.c and test_symbol only, then executes test_symbol.
The tool used --seed 1, --jobs 2, --san, timeout 1300 and 30 samples per slice, 60 total for this file.

The actual last summary lines were:

- A: 30 mutants in 35.9 s: 20 killed, 5 survived, 5 not compiled, 0 timed out, 0 excused. Exit 1.
- B: 30 mutants in 91.4 s: 22 killed, 8 survived, 0 not compiled, 0 timed out, 0 excused. Exit 1.

Thus 55 sampled mutants compiled. Four survivors were gaps, fixed with tests and killed by targeted
replay. Nine others are equivalent. Every survivor is listed below; lines refer to the original logs.

- A:82, drop gcd: mixed-domain gap; new triple (1,3,2) kills it.
- A:131, drop Jacobi entry check: invalid input plus even lower entry kills it.
- A:108, symbol kind 1->0: both paths use Jacobi after positive-odd validation.
- A:125, ball kind 0->1: its lower entry is already an odd prime, so the paths coincide.
- A:44, modulus &&->||: positive odd finite Kronecker rows kill it.
- B:283, u*u-1 -> u*u+1: an odd square is 1 mod 8; both integer quotients are identical.
- B:287, sign product -> division: the divisor is +1 or -1.
- B:282, (w-1)/2 -> w/2: w is positive and odd.
- B:246, <=WORD_MAX-3 -> <WORD_MAX-3: new three-digit endpoint cases kill it.
- B:227, loop <8 -> <=8: only odd values are visited; both stop before 9.
- B:284, exponent addition -> subtraction: parity is unchanged, including negative odd exponents.
- B:332, rational sign<0 -> sign<1: zero was rejected; signs are -1 and +1.
- B:261, Legendre-sign product -> division: the divisor is +1 or -1.

The first gcd-gap attempt used (1,1,2), which canonicalises to centre 0 and did not kill that fault.
That replay stopped with exit 1 and was not counted as a detection. The corrected plain replay killed
2 gaps; a later INV replay killed all 3 A gaps. The B endpoint replay killed its 1 gap. No generic
rerun was used, and tools/mutate/equivalent.txt was not changed. Mutation baseline checks passed.

## Commands and results

All builds used at most 2 jobs. Tests and scripts, including nested fault compilers/runners, use timeout.
Read-only source inspections are not numerical checks. Check commands are recorded here with their results.
B below abbreviates lanes/f-slice12/build; the recorded logs contain expanded paths.

1. timeout 60 python3 -B proto/symbol_checks.py: exit 0. Initial 3503 rows/500203 bytes;
   after gap rows 3724/517050. Repeated after each fixture change, with the same final count.
2. timeout 90 python3 -B proto/symbol_checks.py --hilbert: exit 0, 608 exact rows then 1920 finite rows;
   total fixture bytes 679803. The first B generation preceded the finite generator and wrote 608 rows.
3. timeout 180 make -s -j2 BUILD=B B/test_symbol: initial exit 2/link errors; subsequent builds exit 0.
   timeout 180 make -s -j2 BUILD=B all built the stub archives, exit 0.
4. timeout 60 cc -std=c11 -Wall -Wextra -Werror -Iinclude -Itests lanes/f-slice12/red.c
   B/libadelefeld.a -lflint -lgmp -lm -o B/red: exit 0; timeout 60 B/red: exit 1, 9 failed assertions.
   The same commands with red-B.c/red-B: compile 0, test 1, 4 failed assertions.
5. timeout 60 B/test_symbol: first real run exit 1, 15 Y4 counterexamples. Green A runs exit 0:
   3 tests/483446 checks, then 4/483453, then final A 4/488759. The B timeout was 90:
   8/671661, then 10/719310, then final 11/719322; all these B runs exit 0.
6. timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude tools/adf/adf.c
   B/libadelefeld.a -lflint -lgmp -lm -o B/adf: initially exit 1 from GCC's existing accessor
   false overread; attempted executions exit 127. A noinline attempt also failed and was removed.
   Copying the canonical content field with fmpq_set fixed it. Later A, B and final compiles exit 0.
7. timeout 60 B/adf < tests/driver/symbol-residue.cmd: expected exit 1, 15 hand lines.
   timeout 60 B/adf < tests/driver/symbol-hilbert.cmd: expected exit 1, initially 16 hand lines;
   final 19 lines include 3 staged-error cases. timeout 10 diff -u EXPECTED GOT: exit 0 for each
   successful A/B/final run. Expected files were written before execution.
8. timeout 120 cc -shared -fPIC -Iinclude -std=c11 -O1 -g src/*.c -o B/libadelefeld.so
   -lflint -lgmp -lm: exit 0, once per slice.
   LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 timeout 60 julia --startup-file=no
   tests/julia/symbol.jl B/libadelefeld.so: exit 0, A 23 assertions, B 20+23 assertions.
9. timeout 180 python3 -B lanes/f-slice12/faults.py A, then B: both exit 0, 6/6 faults detected each.
   Each nested compiler/linker and test has timeout 60; exact flags are in that script.
10. timeout 180 make -s -j2 SAN=1 INV=1 BUILD=lanes/f-slice12/build-mutbase all: exit 0.
    timeout 90 sh lanes/f-slice12/mutation.sh: exit 0, first 3/483446, later 5/488777 checks.
11. timeout 30 python3 -B lanes/f-slice12/prepare_mutation.py: exit 0, once per slice.
    The two tool commands, with X=A and then X=B, were:

    timeout 1300 python3 -B tools/mutate/mutate.py --root lanes/f-slice12/build/mutroot \
      --scratch lanes/f-slice12/build/mutate-X --files src/symbol.c --limit 30 --seed 1 \
      --jobs 2 --san --timeout 90 --make 'make -s -j1 check-symbol SAN=1 INV=1' \
      --copy Makefile include src tests lanes

    Both exit 1 from the survivors, with the compiled counts given above.
12. timeout 180 python3 -B lanes/f-slice12/survivors.py A: first attempt exit 1/no detection;
    next plain attempt exit 0/2 detections; final INV attempt exit 0/3 detections.
    timeout 180 python3 -B lanes/f-slice12/survivors.py B: exit 0/1 detection.
13. timeout 900 make -j2 check-all: RUN ONCE, EXIT 2. Before the last step it passed all 78 C
    programs, 57 driver cases/101050 expected lines, 455/455 exports, Julia, and mutate selftest.
    Memcheck selftest found four init-without-clear paths in my debug child tests. The paths occur
    if a routine unexpectedly returns instead of aborting. Added clears; abort assertions remain.
    timeout 180 python3 -B tools/memcheck/selftest.py: repair run exit 0; final run exit 0.
    The full suite was not rerun or relabelled as passing. check-all.log retains the failed exit.
14. timeout 180 make -s -j2 SAN=1 BUILD=lanes/f-slice12/build-san
    lanes/f-slice12/build-san/test_symbol: build exit 0.
    ASAN_OPTIONS=detect_leaks=0 timeout 90 lanes/f-slice12/build-san/test_symbol:
    exit 0, 11 tests, 719322 checks, 0 failures. LeakSanitizer is disabled because the sandbox
    cannot run it, as the brief states; address and undefined-behaviour sanitizers ran.
15. timeout 180 make -s -j2 INV=1 BUILD=lanes/f-slice12/build-inv
    lanes/f-slice12/build-inv/test_symbol: first build exit 2, ignored freopen result under -Werror;
    attempted test exit 127/no executable. Checking freopen's result fixed it. Same build exit 0;
    timeout 90 lanes/f-slice12/build-inv/test_symbol: exit 0, 13 tests, 719376 checks, 0 failures.
16. timeout 180 make -s -j2 CC=clang BUILD=lanes/f-slice12/build-clang
    lanes/f-slice12/build-clang/test_symbol: build exit 0.
    timeout 90 lanes/f-slice12/build-clang/test_symbol: exit 0, 11/719322, 0 failures.
17. Final targeted ordinary build/run, using command 3 then timeout 90 B/test_symbol:
    exit 0, 11/719322, 0 failures. Final strict driver compile and both fixtures use commands 6/7,
    exit 0 for compile and diffs; expected exit 1 for each driver fixture. This verifies the 3 new
    precedence rows added after the broad run's driver step.
18. timeout 60 cc -std=c11 -Wall -Wextra -Werror -Iinclude -fsyntax-only lanes/f-slice12/header.c:
    exit 0. timeout 60 c++ -std=c++17 -Wall -Wextra -Werror -Iinclude -x c++ -fsyntax-only
    lanes/f-slice12/header.c: exit 0. The new header stands alone in both languages.
19. timeout 10 python3 -B with the newline/line-length/fixture-size audit: exit 0; 10 delivered
    new source/text files, 0 missing final newlines, 0 lines above 116 columns, 679803 fixture bytes.

## Findings against the specification, proofs or brief

1. The brief says finite precision determines the residue symbol "exactly when that modulus divides N".
   Catalogue.md:33 says "sufficient moduli, not claims of minimality"; :55-56 says a coarser input
   can give one value. SPEC 9.3.7's Kronecker row also says "sufficient, not always necessary".
   Every point of 1+3 Zhat has Jacobi symbol +1 at lower entry 9, although 9 does not divide 3.
   This refutes necessity and the requested universal two-point ambiguity witness for rejected balls.
   The implemented conservative policy is documented as an algorithmic certificate, not a necessity claim.
2. The unqualified multiplicativity request fails at a=-1 and denominator factors 0,3:
   (-1/(0*3))=1, but (-1/0)*(-1/3)=-1. Definition 1:19 supplies the rule at zero, and the supplement
   supplies (-1/3)=-1. PARI usersch3.tex:9263 says "total multiplicativity in both arguments";
   its explicit rules at :9266-9271 also have this exception. Tests assert the counterexample and
   check the law on nonzero denominator factors. No definition or expected symbol was changed.
3. The Bristol text display at refs/src/hilbert-bristol/lecture19.txt:47-51 transposes the two
   unit Legendre exponents relative to catalogue.md:68. For (3,2) at 3, the transposed expression
   gives +1 and primitive solvability gives -1. The 2-adic display at :52-57 also has extra
   Legendre factors, unlike catalogue.md:73. This is a text/transcription finding; the typeset
   PDF was not inspected. The proved catalogue formulas and independent congruence search agree.
4. Silence on resource limits was resolved by the no-extra-cap policy described above. Silence on
   real mixed-zero status was resolved by conventions 3.1, rather than claiming DOMAIN for a mixed ball.
   There is no identified false mathematical Hilbert or residue-symbol statement in SPEC itself.

## Files written

Product files: include/adelefeld/symbol.h; src/symbol.c; tests/test_symbol.c;
tests/julia/symbol.jl; proto/symbol_checks.py; docs/api-1f9.md;
tests/ref/vectors/f-slice12/{residue,hilbert,hilbert_finite}.jsonl;
tests/driver/symbol-{residue,hilbert}.{cmd,out}. Modified owned driver files:
tools/adf/adf.c and tools/adf/README.md. Added only the required include in include/adelefeld.h
and the two execution lines in tests/test_julia.sh.

Lane files: progress.md, redgreen.log, red.c, red-B.c, header.c, faults.py, survivors.py,
prepare_mutation.py, mutation.sh, survivor-notes.md, this report, and the named compiler/test/mutation
logs and driver outputs. Lane build directories contain objects, archives, executables, staging copies,
and planted source faults. Final check-all used build/ as explicitly requested. Other source paths were
read-only. No state-changing git command, bd command, package installation or delegation was used.

## What is not done and sources pending

Slice A and Slice B are complete. The other catalogue functions are outside this lane.
The API intentionally has no arbitrary-size Legendre lower prime and no set-valued residue output or
all-places Hilbert family. The conservative residue test does not implement Proposition 3's sharper
enumeration. No long differential fuzz campaign or performance benchmark was run. There is avoidable
work in repeated modular reductions and in computing unit classes that an even valuation can ignore.
The simple implementation was retained. LeakSanitizer was not run. The full suite's one failed run is
retained; the failed component and all required symbol build variants pass after the repairs.

[source pending: an on-disk statement of quadratic reciprocity and both supplementary laws for the
conditional proof in catalogue.md:167-178.] The product theorem has a direct on-disk citation in Y8.
The Hensel and special Kronecker-convention sources formerly pending in that proof file are supplied
above. No modification of that read-only proof file was made.
