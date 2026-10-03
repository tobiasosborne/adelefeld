# f-review8 running notes

Read CLAUDE.md, f-review7 progress and brief, SPEC 9.3.3, 9.3.4, N-D13 to N-D15, PLAN 1F.5 and 1F.6, the named
proofs, conventions, and PERF. Only this lane is writable. No subagents or tracker. Archive: timeout 600 make -j2
BUILD=lanes/f-review8/build lanes/f-review8/build/libadelefeld.a Exit 0. Output: build-archive.log. This is the one
archive build. The apply_patch tool twice returned an empty object without creating the requested files. The first
mutation prepare command therefore exited 2 (file missing); no tests ran in that attempt. Files are now written with
shell heredocs and verified on disk.

## P1 refereed

Claim: reduction makes powering bijective on the nonzero root branches and preserves rationality both ways. Steps 1
and 3: if a e' + k n' = 1, q -> q^a x^k is an explicit inverse on the roots of x^e'. Its n'-th power is x and its
e'-th power is q. This also works for negative e'. Step 4 is degree-one identity. Step 5 uses p^k with k not
divisible by n' to prove a zero ball mixed. For a negative exponent, use nonzero p^(n' l), rather than zero, as the
domain witness. MINOR: step 2 and lpow.h say 3 is an exact root of 4 in Q_5. The roots are 2 and -2; 3 is a residue.
Noninjectivity does not make a valid seed ambiguous. Reduction avoids duplicate values and dependence on
representation. Coprimality is sufficient, not necessary at a fixed prime. The precise injectivity criterion is
gcd(e, gcd(n, p-1))=1 at odd p, and gcd(e, gcd(n,2))=1 at 2. For 3/3 at 5, x=1 has just one root. No code depends on
the false sentence. Exact-integer reproducer to follow.

## P2 refereed

Claim: the guarded rational-power branch image is exactly b^e'+p^E' Z_p, E'=e' j+r-s+v_p(e'). Steps 1 to 3: the root
image is b(1+p^(r-s) Z_p). Log, multiplication by e', and exp map the relative factor bijectively onto
1+p^(r-s+v_p(e')) Z_p. A negative scalar still maps the ideals bijectively. Inversion keeps relative precision.
Multiplication by b^e' adds e' j, including j<0. Step 4 is the same three bijections directly from the input factor
1+p^r Z_p. A reduced fraction cannot have p dividing both e' and n'. Step 5 follows by nesting balls and constant
output valuation. Step 6 applies nesting to a single point. Tightness: b^e' and b^e'+p^E' are both in the image.
Their ratio is in the output relative group; reverse the bijections to produce their input points. Exact-rational
witnesses and bounded grids to follow.

## P3 refereed

Claim: root relative precision max(1,K-e'j-v_p(e')) determines the composite centre at K. Step 1: K<=E' implies this
precision is <=r-s. Thus N-D14 returns exactly that coarse root ball. For an exact input its root ball encloses the
selected point, unless the root is rational and exact. Steps 2 and 3: L12 adds v_p(e') relative digits; at 2 with
rel_r=1 and even e' it adds one further digit. The extra digit cannot hurt containment. It is not necessary to keep
the intermediate root sign fixed: the even power of the coarse odd-unit ball has the required principal image. An
odd e' gains no digit, so rel_r=1 then requests only one output relative digit. This resolves the only exceptional
2-adic case. Steps 4 and 5: returning P or reducing its centre is valid because its exponent is >=K; at K=E',
rel_r=r-s>=c, so there is no exceptional epsilon and P is the exact image. Step 6: all output points have valuation
e'j; K<=e'j permits centre zero, after checking the branch. Large |e'| and negative j enter the valuation, not the
required relative error; the precision proof is valid. MINOR text defect to check with a reproducer: lpow.h says
working precisions are bounded by K. They are not: Nr=K-(e'-1)j-v_p(e') can exceed K when j<0 and e'>1; relative
precision can too. P3 itself gives the correct formula. The bound |e'|<2^63 in P8 also excludes admitted LONG_MIN.

## P4 refereed

Claim: domain is the product of the principal-unit ball (all odd units at 2) and Z_p. Step 1: a product meets a
product domain iff both coordinates meet it. It is contained iff both are. If only partially contained, choose a
domain point in each coordinate for a meeting witness, and an outside point in an offending coordinate with any
point of the other coordinate for an outside witness. Step 2 uses exact overlap and containment, with DOMAIN taking
priority over NOT_DETERMINED. Step 3 correctly separates centred and noncentred canonical exponent balls. No use of
the log isometry on 1+2 Z_2 is needed for this domain test.

## P5 refereed

Claim: fixed-sign cases have the exact product radius; changing sign/parity cases have the specified hull. Steps 1
and 2: exact zero exponent and base 1 give 1; exact base -1 depends only on parity. Steps 3 and 4: fixed sign
multiplication preserves base exponent; Proposition 18's product-set argument fills the radius ball, including
either exact factor. With A=1 and even exponents the signs disappear, log-principal parts run over 4 Z_2, and their
products with s fill 2^(2+min(beta,B)) Z_2. Steps 5 and 6: an available odd exponent maps every odd unit bijectively
to every odd unit, by separate bijections on the sign and on 4 Z_2. The witnesses 1 and -1 force hull exponent 1.
Step 7: with sign -1 and B=0, even powers lie in 1+8 Z_2; odd powers are 3 mod 4, so 5 mod 8 is missed. This also
holds for exact -1, whose image has only two points. Step 8 follows by nesting balls. Independent enumeration of all
small unions, including exact bases and exponent balls, to follow.

## P6 refereed

Claim: in P5(c), exact subtraction gives alpha, and one Log/product/exp gives the centre at K. Step 1 is valid only
on 1+p^c Z_p; P5(c) at 2 has A>=2 or exact base, with its sign fixed. A=1 uses a separate path, so the false
extension v(log u)=v(u-1) on all odd units is never used. For u=-1, w0*u=1 gives alpha=INF; the public function
bypasses principal() and handles parity explicitly. Subtracting u0-w0 instead of w0*u0-1 differs by the unit w0,
hence has exactly the same valuation. Steps 2 and 3: v(s0)>=0, P=max(K-beta,c), P+beta>=K; exp is an isometry and
returns exponent K. The exact-zero logarithm may make L exact; then exp is exact 1 and adding p^K Z_p gives the
stated ball. Step 4: P<=max(K,c); product exponent P+beta may exceed K. The proof's max(K,c)+beta bound holds. The
header's simpler working-precision bound by K does not literally hold.

## P7 refereed

Claim: the finite-ring oracle enumerates every residue relevant to each promised comparison. Step 1:
C(k,i)=(k/i)C(k-1,i-1) bounds the nonlinear terms strictly above s+e. For unit t the linear term is the unique
minimum, so equality holds. For nonunit t only the stated lower bound is needed. Negative powers follow by
inversion. h>=c and h+s>=r make the root test independent of the chosen representative. When used for an exact root
at h+s, P7(d) supplies lifting. Step 2: exponent changes by p^k change the principal product by valuation >=k+c. At
2, k>=1 also fixes the sign exponent. The integers 0..p^k-1 represent every residue in Z_p; each exponent ball
supplies every allowed residue by s0+p^B t, 0<=t<p^max(0,k-B). Thus the finite image is exact modulo p^H; density
plus continuity is an alternative proof, not an unstated extrapolation. For exact rational s0, reduce it modulo p^k.
For negative integers, use their nonnegative residue. At 2 and H=1, dependence on the base modulo 2 is trivial:
every value is odd. The printed proof only treats H>=c; this missing endpoint argument is a MINOR omission, supplied
here. Steps 3 and 4: scaling changes the image exponent by e'j; a modular root at c+s has a residual principal
factor whose logarithm can be divided by n'. Exponential corrects it without changing its identifier. This proves
that absent branches are DOMAIN, not merely absent in a finite sample. The oracle uses h=r-s+1+v(e'), which exceeds
the h needed for the rational-power comparison.

## P8 refereed

Limits: bounded input differences fit signed words; clamped sums fit the signed range (the lower endpoint -2^63 is
admitted, and the positive INF shortcut excludes +2^63); unsigned magnitudes in mul_clamp include LONG_MIN. K and
the stored valuation are checked. The printed strict |e'|<2^63 is false for e'=LONG_MIN, n'=3; the code's unsigned
magnitude is safe. Transactions: each commit occurs only after all failures; the zero-input route composes two
transactional routines. Aliasing: all reads end before each commit; two aliased inputs still represent independent
sets. Named places: copied components, membership before local evaluation, where written on failure, source alias
allowed. These code sentences hold. The header sentence that working precisions are bounded by K does not hold
(P3/P6 above). No repeat of f-review7's three known findings is intended.

## R8 refereed

Claim: CRT decomposition finds one d-th root, and multiplication by a primitive d-th root lists all roots. Steps 1
and 2: ord(g^h)=d; multiplying a nonzero t0 by its powers gives d distinct solutions. The degree bound, or the
kernel of the power map, proves completeness. Sorting preserves the set. Step 3: Q=q^v_q(P) for each q|d; D is their
product, R=P/D. They are pairwise coprime; each idempotent is 1 at its component and 0 at all others. Their sum is 1
modulo P. When R=1 its component is trivial and the code skips its inverse; the printed formula needs this
convention because inverse modulo 1 is not an ordinary unit inverse. Missing endpoint argument: MINOR. Step 4: d is
coprime to R, so its inverse recovers that component. Steps 5 and 6: the q-primary exponent k is divisible by q^b,
b=v_q(d). Start with its b zero digits. For digit i, subtract known lower digits, raise to q^(a-1-i), and all higher
digits vanish modulo Q. The remaining q possibilities are distinct. A finite search c<q must find the digit on valid
input. This works for repeated q in d, b=a (no digit loop), and q=2, p=3 mod 4 (a=b=1, no digit loop). Step 7: d/q^b
is a unit modulo Q; multiplying k/q^b by its inverse gives a root component. Step 8: components commute, so their
product has d-th power A. The code verifies it before listing. A nonsolvable torsion equation is rejected as DOMAIN
by criterion before identifiers/dth_root. The NOT_DETERMINED certificate failure inside identifiers is not a
nonsolvability status. Step 9: R5 identifies these d-th roots with the n-th roots, then sorting is deterministic.
Step 10's count of at most sum((a-b)*q) word products omits the modular exponentiations in each digit and CRT
reconstruction. It bounds only the trial multiplications in the digit search. A counted exact-integer reproducer
will follow. No time guarantee follows merely from q<2^20.

Proof checks complete: timeout 180 python3 -B lanes/f-review8/proof_checks.py, exit 0. P1: 5760 cyclic-group
injectivity comparisons. P2: 43200 exact Fraction point comparisons and 8640 tightness pairs, including negative
exponents and j. P3: 89950 precision inequalities, 3280 with the exceptional 2-adic gain. P5: 2112 unions at
H=3,4,5,6; 44 strict hulls missing 5 mod 8, 48 full hulls, 1592 cosets. P7: 9636 prescribed-period versus
extra-period comparisons at H=1..4. All comparison failures: 0. Endpoint harness: 3 calls, all OK. R8: timeout 60
lanes/f-review8/build/repro_r8, exit 0: 15972 cases, 13485 nonsolvable, 0 failures, exact F_p comparisons. Counted
p=17,d=2,A=4: 8 explicit modular products and 11 modular-power calls, against the stated bound 6. Each modular-power
call also costs work.

Mutation baseline: test_lpow 2760383 checks, test_lroot 1308401 checks, test_rfunc_prime 522217 checks; 0 failures
in each. The first 8 faults completed, no timeout. A+B omitted only at p=65537 survives all three programs; a
reproducer is being prepared. Other completed faults are detected by test_lpow. Several wrong-result faults survive
the wrapper comparisons because both the expected value and the wrapper use the same altered local routine.

All 16 fault injections are complete: 48 test runs, 0 test timeouts. Four faults survive all three programs. Three
change values: A+B omitted at 65537, powrat centre shifted by 65537, and an unsampled root lift (seed 42 at 65537)
shifted by 65537. One only changes when work is done: early_status always OK. Counts and exits are in
build/faults/results.jsonl. Two deliberately broken lroot variants crash test_rfunc_prime after failed assertions;
these are detected faults, not crashes of the original library. A harness replacement initially matched two commits;
its scope was corrected before the last two variants were compiled.

Survivor reproducers: timeout 180 python3 -B lanes/f-review8/reproduce_survivors.py, exit 0. All three
original/mutant pairs differ. At precision 65537^2, the wrong seed-42 lift raised to 65536 is 511319675, rather than
1. The other two mutants exclude the exact witness 1 or 65538. Early instrumentation: timeout 180 python3 -B
lanes/f-review8/early_probe_build.py, exit 0. For exact -2 at 3, n=2, N=2^60+1, both return LIMIT with len=-1, but
identifiers is called 0 times originally and 1 time in the faulty version. The 2-second guard cannot check this
order. This is a test gap, not a repeat of the known false early-status sentence.

Fixture audit: powrat unit image radii and powunit radii use the same displayed formulas, but independent modular
set equalities verify them before storage. They are not merely regression expectations. Runtime powrat scalings
j=-1,1,2, and lroot generator's two scaled unit rows, apply algebraic exponent transformations rather than repeat an
independent scaled enumeration. Those transformations are regression checks backed by the scaling proof. The pow
wrapper's want values call the local routine being tested; they do not certify its centres or radii. Three
hand-value assertions in the root wrapper do supply independent values. The f-repair4 large-list test independently
checks every identifier, canonical shape and exponent, but only 16 sampled centres. Branch 42 survives this gap.
Julia and driver rows use small hand values or modular integer equations; they were read, not run again.

Cost run: the first 180-second supervisor timed out during a second long row, after one complete trial of that row.
The retained first trial is used, and no completed trial was repeated. The 64-bit-prime N=10000 powrat total took
112.001818964 seconds; its paired components could not finish in the remaining part of the 150-second process cap.
The remaining total/component phases are separated, each under timeout 150, to obtain the ratio within the
per-process bound. Single trials are identified explicitly; their min/median/max are the same single observation.
All ratios compare call rates with call rates and are REFERENCE ratios, not lower-bound ratios.

Literal SPEC wording found: N-D15 calls min(N,R) the exact image. For u=1+9 Z_3, s=1 exact, N=1, the actual image
has 9 residues modulo 3^4, while the returned 1+3 Z_3 has 27 and contains the extra point 4. The library follows
N-D14; the isolated exact-image phrase in N-D15 is false unless N>=R. Reproducer: spec_precision.in and
audit_fixtures.py. Severity MINOR wording; no change to SPEC made.

R8 counting qualification: the original 8 products include CRT work and reconstruction. The digit loops alone use 5
explicit products (<=6) AND 6 powering calls. Those powering calls are omitted from the cost sentence. My own
binary-power reference needs 11 further products for these calls; that is a REFERENCE count, not a measured FLINT
kernel count. Exact FLINT internal counts are [source pending: FLINT 3.0.1 powmod2_ui_preinv implementation]. The R8
finding is incomplete cost accounting, not a claim that its explicit loop count is false. Command: timeout 180
python3 -B lanes/f-review8/r8_count_details.py, exit 0. The count run also repeats the same 15972 proof cases, with
0 failures.

Smaller root-fault witness: timeout 30 python3 -B lanes/f-review8/survivor_minimal.py, exit 0. At 65537,
x=1764+65537^2 Z_65537, n=2, seed=42, N=2: original centre 42, mutant 65579. The exact input point 1764 has root 42,
outside the mutant ball. This is precision 65537^2. The all-branch test's actual unsampled case remains x=1,
n=65536, seed=42, N=20.

The one-sign fault is being refined to compute the actual odd-parity component by calling principal() with exponent
ball 1+2 Z_2. The first version used -1+4 Z_2, which was one component exactly only at A=2. The final table will use
the refined fault's rerun, not the first version, so the requested fault is exercised at every fixed-sign hull
input.

Nontrivial precision witness: timeout 30 build/h < working_precision.in, exit 0, two OK calls. At 5, x=5^-30*(2+5^21
Z_5), e/n=2/3, seed=3, N=0: K=0, j=-10, e'j=-20, rel_r=20, Nr=10. The root is at absolute precision 10, the output
at 0, both with 20 relative digits. Generalise j=-L, r=2L+1: N stays 0 while work and output digits grow with L.
This disproves the header's bound by K, not the sufficiency of P3's correct precision formula. P3 boundary checks:
timeout 30 python3 -B proof_boundaries.py, exit 0, 11296 inequalities, 0 failures, exact integers, including
LONG_MIN, large numerator powers and unsigned degrees. P6: timeout 30 build/alpha, exit 0, 11 checks, 0 failures: 10
exact subtractions against specified alpha and Log enclosures at 12 digits, plus the A=1 Log image at exponent 2.

Final cost rows complete: 36 rows, 94 final timing trials, 0 median ratios above 1.5.
Six long rows use timing-only probes inside the public composition and independent full
modular certificates; one interrupted 65537 ball row uses its one completed validated pair.
The final P64 exact powrat call took 107.684523965 s; its component sum is recorded in
cost-instr-powrat-P64-exact.log. All six instrumented calls exited 0 with valid=1.
The first 112-second total was not a completed validated pair and is excluded from the final table.
The raw first-trial N=100 comparisons have 8 ratios above 1.5 with unmatched initial states;
all are disclosed in the report and cold-ratios.log, without attributing a library overhead.
Refined one-sign fault: 30 failures in test_lpow; 0 in the other two programs.
Final mutant table: 16 faults, 4 survivors, 54 actual runs including baseline and superseded
sign runs, 0 test timeouts. Two faulted wrapper runs exit SIGSEGV after 13 and 11 assertions.

Report preflight passed: 36 cost rows, 16 fault rows, 94 trials, four unchanged source hashes,
exact residue certificates at 5^20 and 65537^2, and maximum report line length 116.
The report is now written once from the checked preview; no work remains in this brief.
