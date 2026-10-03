<!-- ROLE: record of a review. Assembled by the orchestrator on 2026-10-03 from two lanes; the text below the
     first rule is the report of lane f-review8 as written (lanes/f-review8/report.md), not edited. -->

# Review of the powers at a prime (lane f-slice9, WP 1F.6) and of the branch enumeration of the roots (lane f-repair4)

Two lanes reviewed this code.

1. Lane f-review7 (Claude Opus, 2026-10-02; cut off by a container restart before its report): attacks on
   values, exponents, statuses, branch lists, `_at` forms and the driver against its own oracle in exact
   integers; 0 failures; 3 MINOR (a stale cost sentence of `lpow.h`; `LIMIT` for a small `powunit` result; a
   false order sentence about `early_status`). Record: `lanes/f-review7/progress.md` (its notes and the
   orchestrator's addendum with the reruns). Repairs: lane f-repair5.
2. Lane f-review8 (codex `gpt-6.1-sol` xhigh, 2026-10-03; the three bullets f-review7 did not reach): the
   referee of P1 to P8 and R8 as proofs, tests that cannot fail, the cost. 0 BLOCKER; 3 MAJOR, all test gaps
   (F6, F7, F8: planted faults at `p = 65537` and in an unsampled branch that every test program passes; no
   defect of the library was observed); 7 MINOR (F1 to F5 sentences of proofs and headers, F9 a test gap of
   `early_status`, F10 the wording of N-D15, corrected in `docs/SPEC.md` 15.4 on 2026-10-03). Cost: 0 of 36
   median ratios of a composition to its named components exceed 1.5. Repairs: lane f-repair6.

The programs of both lanes are in `lanes/f-review7/` and `lanes/f-review8/`; paths in the report below are
relative to the worktree of lane f-review8 at commit `8f57a51`.

---

# f-review8

## Findings

F1. MINOR. P1's exact counterexample and its conclusion about seeds are false as written. The input is x=4 at 5,
e/n=2/2. api-1f6.md:81-82 and lpow.h:57-58 call 3 a root of 4 and say that duplicate powered values prevent the seed
from naming one value. In Q_5 the exact roots are 2 and -2. The residue of -2 is 3; the rational integer 3 has
square 9, not 4. Both actual roots square to 4. A seed still selects a unique root and its unique powered value when
two different seeds give the same value. Reduction avoids duplicate outputs and dependence on the representation of
the fraction. It is sufficient for the bijection, but it is not necessary for a seed to be well defined. At a fixed
odd p, injectivity holds exactly when gcd(e,gcd(n,p-1))=1; at 2 replace p-1 by 2. For example, 3/3 at 5 on x=1 is
injective despite being unreduced. The implementation reduces correctly and does not rely on the false explanation.
Reproduce with `timeout 180 python3 -B lanes/f-review8/proof_checks.py`: exact integers and finite cyclic groups;
5760 injectivity comparisons, 0 comparison failures.

F2. MINOR. The header's bound on working precision is false. lpow.h:25-26 says that working precisions are bounded
by K. At 5 take x=5^-30*(2+5^21 Z_5), e/n=2/3, seed=3, N=0. Then j=-10, E'=1, K=0, rel_r=20 and Nr=10. The library
returns centre 52272049199934, valuation -20, exponent 0. Its intermediate root is centred at 43329840692803,
valuation -10, exponent 10. Both need 20 relative digits. P3's actual formula is sufficient and correctly
implemented. The absolute request N does not bound work independently of valuation: in the family j=-L, r=2L+1,
e/n=2/3, N=0, the output and intermediate root need 2L relative digits. Reproduce with `timeout 30
lanes/f-review8/build/h < lanes/f-review8/working_precision.in`. Two OK calls; comparisons use exact parameter
arithmetic and residues modulo 5^20.

F3. MINOR. P8's strict numerator bound excludes an admitted endpoint. api-1f6.md:306 says |e'|<2^63. e=LONG_MIN, n=3
is reduced and has |e'|=2^63. The code uses unsigned magnitudes and a guarded product, so the endpoint is safe. At
5, x=1+5^3 Z_5, seed=1, N=3, it returns 1+5^3 Z_5 with OK. The proof should use <=2^63. Reproduce with `timeout 30
lanes/f-review8/build/h < lanes/f-review8/proof-inputs.in`, third line, and `timeout 30 python3 -B
lanes/f-review8/proof_boundaries.py`. The latter checks 11296 exact integer precision inequalities, with 0 failures.

F4. MINOR. Three endpoint arguments are omitted; the claims remain true. In P1 step 5, zero witnesses root existence
but not existence of a negative rational power. For e'<0 use p^(n' l), with n' l>=N, as the nonzero domain witness.
A p^k with n' not dividing k still witnesses the complement. In P7(b) the base-congruence proof explicitly uses
H>=c. At 2 and H=1 every power of every odd unit is odd, so the missing case is constant modulo 2. R8's eps_R
formula needs the trivial-component convention at R=1: set A_R=t_R=1 and omit its inverse, as the code does. These
arguments require no external source. P7's H=1 endpoint is included in the 9636 period comparisons of
proof_checks.py. R=1 and q=2 with p=3 mod 4 are included in repro_r8's 15972 exact finite-field cases.

F5. MINOR. R8 step 10 omits modular powering from its cost accounting. api-1f5.md:283-284 bounds the digit-loop word
products by sum((a-b)q), without accounting for the two modular powering calls in each digit. At p=17,d=2,A=4, the
loop has 5 explicit products and 6 powering calls. Its explicit-product bound 6 holds. An independently written
binary-power reference needs 11 further products for those calls. The whole root routine has 8 explicit modular
products and 11 powering calls, including setup and reconstruction. Those last totals are not a counterexample to
the loop-only bound. The incomplete cost sentence needs the powering work as well: an own binary-power
implementation costs O(sum((a-b)(q+log P))) word products, plus CRT setup and final checking. No numeric FLINT
internal multiplication count is claimed. [source pending: FLINT 3.0.1 n_powmod2_ui_preinv implementation for its
exact internal count] Reproduce with `timeout 180 python3 -B lanes/f-review8/r8_count_details.py`. Counters surround
actual calls; all value comparisons are exact in F_p.

F6. MAJOR test gap. Every reviewed C program passes a missing A+B term at p=65537. The fault drops that term only at
this prime. The unconditional omission was already planted by f-slice9, so this is a different fault. At p=65537
take u=1+p Z_p, s=Z_p, N=2. The original returns 1+p Z_p; the mutant returns 1+p^2 Z_p. The exact input points
u=65538 and s=1 give 65538, outside the mutant ball. A=1, B=0, N=2 are the first domain-allowed exponents that
distinguish this fault. The mutant's consequence would be BLOCKER. This finding concerns missing test coverage, not
an observed defect in the current library. There are no fixtures at 65537, and the word-prime identities use exact
bases/exponents or cases where A+B is not needed. Reproduce with `timeout 180 python3 -B
lanes/f-review8/reproduce_survivors.py`. Comparison precision is p^2; the witness is an exact integer power.

F7. MAJOR test gap. Every reviewed C program passes a wrong powrat centre at p=65537. The fault adds p to a nonzero
ball centre while keeping its valuation and exponent. Take x=1+p^2 Z_p, e/n=1/2, seed=1, N=2. The original centre is
1 and the mutant centre is 65538. The input point 1 has root 1, which the mutant excludes modulo p^2. n'=2 and two
relative output digits are the first parameters that lose a point under this fault. At one digit the shifted
representative already violates canonical form. The mutant's consequence would be BLOCKER. The local tests have no
nontrivial 65537 power rows. powers_at_prime obtains most expected answers from the same local routine, so those
wrapper equalities do not certify its centre or radius. A separate hand assertion for the exact value 27 does not
cover this prime or ball case. Reproduce with `timeout 180 python3 -B lanes/f-review8/reproduce_survivors.py`.

F8. MAJOR test gap. The large all-branch test passes an incorrect unsampled root lift. The fault adds p to the
nonzero result centre only for p=65537 and seed=42. At x=1, n=65536, N=20, the f-repair4 test checks every
identifier, ordering, canonical shape and exponent, but raises only 16 sampled roots to n. Seed 42 is not sampled.
All three programs pass. At N=2 the original seed-42 centre is 3467431638; the mutant centre is 3467497175, whose
65536-th power is 511319675 modulo p^2, not 1. A smaller reproducer uses x=1764+p^2 Z_p, n=2, seed=42, N=2. Its
exact point 1764 has root 42. The mutant returns 65579+p^2 Z_p and excludes 42. The mutant's consequence would be
BLOCKER. A valid identifier, canonical centre and correct exponent do not prove the branch's centre is correct.
Reproduce with `timeout 30 python3 -B lanes/f-review8/survivor_minimal.py` and `timeout 180 python3 -B
lanes/f-review8/reproduce_survivors.py`. Precision is p^2.

F9. MINOR test gap. early_status always OK passes all three C programs. At 3, x=-2 exact, n=2, N=2^60+1, the
original calls identifiers 0 times before LIMIT; the faulty version calls it 1 time. Both leave len=-1 and return
LIMIT. The test's 2-second bound cannot detect one inexpensive enumeration. This does not repeat the known false
order sentence: it records the requested fault's survival and shows what the existing timing assertion fails to
certify. Reproduce with `timeout 180 python3 -B lanes/f-review8/early_probe_build.py`. Two instrumented builds and
two calls, both exit 0; counts are exact, not timings.

## Findings against the specification

F10. MINOR wording. N-D15 calls min(N,R) the exact image, which is false when N<R. At 3 take u=1+9 Z_3, s=1 exact,
N=1. The image is 1+9 Z_3, while the library returns 1+3 Z_3. Modulo 3^4 they contain 9 and 27 residues
respectively. The extra point 4 is in the returned ball and outside the image. N-D14 and the detailed header
correctly specify an enclosure at N. No specification file was changed. Reproduce with `timeout 30
lanes/f-review8/build/h < lanes/f-review8/spec_precision.in` and `timeout 30 python3 -B
lanes/f-review8/audit_fixtures.py`.

## Statement-by-statement checks

P1. Claim: after reduction, root-then-power labels each nonzero value uniquely and preserves rationality in both
directions. Step 1: if a e'+k n'=1, the explicit inverse q -> q^a x^k has n'-th power x and e'-th power q. This
proves both directions without counting roots and works for negative e'. Step 2 has F1. Step 3 gives rationality by
the same formula, since x and q are nonzero rationals. Step 4 is the degree-one identity, including its pow_si
statuses. Step 5 gives the exact-zero cases and a mixed zero ball; F4 supplies its negative-power witness. The 5760
group comparisons would fail if the claimed injectivity criterion disagreed with the number of distinct powered
roots. All comparisons are exact, without p-adic truncation.

P2. Claim: the guarded branch power image is exactly b^e'+p^E' Z_p. Step 1: write the root image as b(1+p^(r-s)
Z_p). Steps 2 and 3: log maps its relative factor onto p^(r-s) Z_p; multiplying by nonzero e' maps that onto
p^(r-s+v_p(e')) Z_p; exp maps it onto 1+p^(r-s+v_p(e')) Z_p. The strong guard gives r-s>=c. Scaling by b^e' adds e'j
to difference valuations. Negative e' is a nonzero scalar with the same valuation rule; inversion of a unit
preserves relative precision. Negative j changes only the last scaling. Step 4 repeats these bijections from the
input factor a(1+p^r Z_p). p cannot divide both reduced e' and n'. Unreduced simultaneous divisibility is removed
before this statement applies. Step 5 follows by ball nesting and constant valuation. Step 6 applies the same
containment to the exact point of an irrational branch. For exactness, the points b^e' and b^e'+p^E' belong to the
image: reverse the bijections to attain each. Their difference has valuation E', ruling out exponent E'+1. The
program checks 43200 exact Fraction point comparisons and 8640 tightness pairs. Example:
p=3,n'=3,e'=-2,j=-1,M=-1,E'=3. Roots 1/3 and 4/3 give input points 1/27 and 64/27, outputs 9 and 9/16. Their
difference has valuation 3. A case fails if an input difference is below M, an output difference is below E', or the
tightness pair's difference valuation differs from E'. No truncation is used.

P3. Claim: rel_r=max(1,K-e'j-v_p(e')) suffices for every branch centre that succeeds. Step 1: K<=E' implies
rel_r<=r-s for a ball input, so the coarse root ball contains the whole root image. For an exact input it contains
the selected point, or is exact. Step 2: integer powers add v_p(e') relative digits. At 2 with rel_r=1 and even e',
they add one extra digit. This extra digit is harmless. An even power removes the unfixed sign of the coarse root
ball. For odd e', rel_r=1 requests only one relative digit, so the larger root ball still suffices. Step 3:
rel_r+v_p(e')>=K-e'j, with a strict inequality when the max selects 1 unnecessarily. Reducing that centre to K
cannot lose an image point. Steps 4 and 5: P at K can be returned directly; at K=E' the root precision is r-s>=c, so
the power gives the exact image. Step 6: K<=e'j permits the zero-centred ball after status/rationality checks. Large
|e'| and very negative j affect valuations, not this inequality. Saturation cannot silently undercompute a
successful centre: positive saturated e'j yields the zero ball; negative saturated e'j plus r-s+v_p(e') is still
below the result exponent bound. Out-of-bound auxiliary requests fail transactionally rather than return a false
centre. 89950 small-parameter inequalities include 3280 exceptional 2-adic gains. Another 11296 inequalities use
word endpoints and large prime powers. All are exact integers. The two nontrivial library calls in
working_precision.in have 20 relative output digits. A failed inequality or a successful result based on fewer
relative digits would refute P3. F2 concerns the header's separate cost-bound sentence, not this sufficiency proof.

P4. Claim: domain membership is membership in a product of two discs. Step 1: the product meets the domain exactly
when both factors meet their coordinate domains; it lies inside exactly when both lie inside. An offending outside
coordinate paired with any point of the other factor proves noncontainment. A meeting point in each factor proves
intersection. Thus DOMAIN wins when either misses, otherwise partial membership is NOT_DETERMINED. Steps 2 and 3:
canonical exact values and nested or disjoint balls make overlap/containment exact. At 2 the base domain is all odd
units. No log isometry on 1+2 Z_2 is used here. Original test_lpow's domain/status checks pass as part of its
2760383 checks. The proof uses exact set membership, not a sample test.

P5. Claim: each fixed-sign image is the radius ball; changing sign/parity has the stated union or hull. Steps 1 and
2: the exact zero exponent and base 1 give 1; exact base -1 depends on parity. Step 3: sign multiplication preserves
A. Log is bijective on the principal disc; the independent product of ell+p^A Z_p and s0+p^B Z_p fills the ball of
radius R. If both centres are outside their error ideals, hold the centre of the factor whose linear term has
minimum valuation fixed and vary the other. If only one centre is outside, hold it fixed and vary the centred-ideal
factor. If both are in their ideals, fix one factor to p^A and vary the other. These cases prove both inclusions,
including infinite alpha/beta and either exact input. Exp sends that product ball bijectively onto the stated unit
coset. Step 4: at A=1 and even exponent, signs disappear, log principal parts range over 4 Z_2, and products fill
2^(2+min(beta,B)) Z_2. Steps 5 and 6: an available odd exponent maps each of the two signs and the principal disc
bijectively; hence the full odd-unit image at A=1. Points 1 and -1 force hull exponent 1. Step 7: for sign -1 and
B=0, even powers lie in 1+8 Z_2 and odd powers are 3 mod 4. Thus 5+8 Z_2 is missed, including exact base -1. Step 8
follows by nesting the ball at K over that exact image or smallest hull. 2112 independently enumerated unions use
H=3,4,5,6: 44 strict hulls, 48 full hulls, 1592 radius cosets, and the remaining exact cases. A missing residue of a
claimed coset, an extra residue in an exact image, failure to attain both signs, or any value 5 mod 8 in a
fixed-negative-sign B=0 image would fail the checks. Failures: 0.

P6. Claim: alpha is an exact difference valuation in case P5(c), and centre precision is sufficient. Step 1: w0*u0
is in 1+p^c Z_p. The log series has its linear term at valuation v_p(w0*u0-1), and every later term has strictly
greater valuation there. Thus the logarithm has that valuation. The subtraction u0-w0 used by the code differs by
multiplication by the unit w0. At A=1 the public function takes a separate path; for exact -1 it handles the exact
zero logarithm and parity before principal(). Step 2: s0*ell is in p^c Z_p, so its exponential is 1 modulo p^K for
K<=c. If s0=0 the centre is exactly that sign. Step 3: Log precision P=max(K-beta,c) produces exponent P or exact
zero; multiplication by exact s0 produces exponent P+beta>=K. Exp at K then determines the required centre. If it is
exact 1, adding p^K Z_p gives the prescribed ball. Step 4's max(K,c)+beta bound holds; F2 notes that the header's
shorter bound by K does not. alpha.c checks 10 exact subtractions and their prescribed valuations against Log at 12
digits, plus the A=1 Log image at exponent 2. Failures: 0. Its Log comparisons are a component cross-check, not an
independent log implementation; the series linear-term argument supplies the proof.

P7. Claim: the enumerated finite residues exhaust the promised output residues. Step 1: C(k,i)=(k/i)C(k-1,i-1) gives
nonlinear term valuation at least v_p(k)-v_p(i)+i e. For i>=2, (i-1)e>v_p(i) on the principal disc. The linear term
is the unique minimum for unit t, proving the equality witness. Negative powers follow by unit inversion. h>=c and
h+s>=r make the modular root test independent of representatives. For exact input, a solution modulo p^(h+s) lifts
without changing its p^h residue: log of the residual ratio can be divided by n' into p^h Z_p. Distinct roots in one
branch cannot differ below h, by the same power estimate. Step 2: changing s by p^k changes the principal
exponential by valuation >=k+c; at 2, k>=1 also fixes parity. Every Z_p residue modulo p^k has an integer
representative. The exponent ball has exactly the residues s0+p^B t for 0<=t<p^max(0,k-B). This proves H' suffices,
without assuming finite-grid evidence proves an infinite set equality. Exact rational exponents reduce modulo p^k;
negative integers use their nonnegative representatives. F4 handles H=1 at 2. Step 3 is exact valuation scaling.
Step 4 corrects a root modulo p^(c+s) by the principal residual exponential; a missing valid seed is therefore
DOMAIN. The oracle's h=r-s+1+v_p(e') is more than sufficient for its power comparison. 9636 comparisons of the
prescribed exponent period with the larger period H+1 use H=1..4 and p=2,3,5,7. Any difference of their finite image
sets fails. Failures: 0.

P8. Claim: limits cannot overflow, failures preserve outputs, aliases are supported, and named-place wrappers
preserve the local contract. Sentence 1: bounded input M-m and s fit signed words; when INF is not absorbed the sum
lies between -2^63 and 2^63-2 before clamping, and unsigned magnitude division guards the product. K and each stored
valuation are checked. F3 corrects the strict LONG_MIN bound. Sentence 2: each component enforces its own power
limits, and the known-centre shortcuts skip reduction powers. Sentence 3: commits follow every possible failure; the
zero-input route composes two transactional routines. No read follows a commit, so each stated alias is valid.
Inputs aliased to each other still denote independent sets. Sentence 4: membership is checked before local
evaluation; copied components and temporary partial balls preserve outputs on failure and where on success. F2
identifies the separate header sentence the code does not keep. No failure of transaction or aliasing was found. The
selected tests' baseline checks cover these branches; no repetition of f-review7's aliasing or driver campaigns was
performed.

R8. Claim: CRT computes one d-th root, and a primitive d-th root of unity enumerates all branches. Steps 1 and 2:
ord(g^h)=d; t0*zeta^i are d distinct roots. The kernel of the d-th power map, or the degree bound, proves
completeness. Sorting preserves that set. Step 3: the Q=q^v_q(P) for q|d, and R=P/product(Q), are pairwise coprime.
Each CRT exponent is 1 at its component and 0 at all others. Their sum is 1 modulo P, so the powered components
multiply to A. F4 supplies the trivial R=1 component. Step 4: gcd(d,R)=1 permits its inverse. Step 5: the q-primary
exponent k is divisible by q^b because A is a d-th power and b=v_q(d)<=v_q(P). Step 6: start with b zero digits;
subtract known lower digits and raise to q^(a-1-i). Higher digits vanish modulo Q and the next digit has q distinct
candidates. Each inner search ends within q trials, and the outer loop has a-b iterations. Repeated prime factors in
d change b, without changing the argument. At q=2 and p=3 mod 4, a=b=1 and the loop is empty; its component is 1.
q<=d<=ADF_LROOT_BRANCH_MAX<2^20. D and Q divide P, so their products fit a word. The final qi update reaches Q, and
each accumulated k stays below Q. Step 7: d/q^b is invertible modulo Q, and dividing k by q^b before multiplication
recovers a root of A_q. Step 8: multiplying the root components gives d-th power A; the final certificate is checked
before publishing. A nonsolvable public torsion request fails criterion's w^h=1 test as DOMAIN before identifiers.
Its equivalence follows by writing w=g^k: w^h=1 iff d divides k. The internal NOT_DETERMINED certificate failure is
not a claimed proof of nonexistence. Step 9 follows from R5: (n/d)e=1 mod h, so these d-th roots are exactly the
n-th roots of w. Step 10 has F5. repro_r8 checks dth_root directly against integer power enumeration for every odd
p<150, every d>=2 dividing p-1, and every nonzero A. It is a raw CRT algorithm check, not a repeat of f-review7's
public branch-list campaign. 15972 cases include 13485 nonsolvable equations; 0 differences. All comparisons are
exact in F_p. A false existence decision, wrong returned d-th power, nontermination, or missing digit would fail.
The count instrumentation repeats these 15972 cases once.

## Fault tests and assertions

The final fault table reports failed checks over completed checks, not merely exit status. A positive failed-check
count means exit 1. Zero means exit 0. The two SIGSEGV cells have 13 and 11 failed assertions before exit -11; their
runners do not reach a summary. The original programs have no crash. There are 16 final faults and 48 final test
runs. An earlier one-sign approximation adds 3 superseded runs, and baseline adds 3 runs: 54 actual test executions,
0 test timeouts. The final one-sign fault computes the exact odd-parity component through principal() with exponent
ball 1+2 Z_2.

The first four faults perturb separately e'j, M-m, v_p(n') and v_p(e') in E'. Two pairs have the same algebraic
effect; their equal counts are expected. The three R faults omit A+beta, B+alpha and A+B respectively; the last is
restricted to 65537 to differ from the prior unconditional omission. K=N is tested in both powunit and the root
implementation. The other faults alter zeta's order to d/q, exchange CRT projectors, bypass early_status, compare
the seed's power modulo p-1, shift a power centre, and shift an unsampled root lift.

Build/run method: compile each selected test object once; compile the altered source into its own object; link that
object before the unchanged archive. The archive is never rebuilt. Every compile/link uses timeout 150, -std=gnu11
-O2 -g, -Iinclude -Itests -Isrc, and links -lflint -lgmp -lm. Every test run is `timeout 150
lanes/f-review8/build/faults/<fault>/test_<lpow|lroot|rfunc_prime>`. Full source copies, compile/link logs, runner
logs and results.jsonl are under build/faults/. No assertion or fixture was edited.

| Fault | lpow failed/checks | lroot failed/checks | rfunc failed/checks |
|---|---:|---:|---:|
| E_ej_plus1 | 61763/2720829 | 0/1308401 | 5/522217 |
| E_relative_minus1 | 80476/2760383 | 0/1308401 | 0/522217 |
| E_denominator_plus1 | 80476/2760383 | 0/1308401 | 0/522217 |
| E_numerator_plus1 | 61763/2720829 | 0/1308401 | 5/522217 |
| R_drop_A_beta | 4875/2759437 | 0/1308401 | 1/522217 |
| R_drop_B_alpha | 4213/2758547 | 0/1308401 | 0/522217 |
| R_drop_A_B_65537 | 0/2760383 | 0/1308401 | 0/522217 |
| hull_one_negative_sign | 30/2760383 | 0/1308401 | 0/522217 |
| powunit_K_N | 11212/2754423 | 0/1308401 | 1/522217 |
| root_K_N | 3796/2760383 | 32153/1143862 | SIGSEGV; 13 emitted |
| zeta_order_d_over_q | 0/2760383 | 119841/1308402 | 2296/522217 |
| CRT_swap_two_idempotents | 0/2760383 | 501/1303571 | 0/522217 |
| early_status_always_OK | 0/2760383 | 0/1308401 | 0/522217 |
| seed_mod_p_minus1 | 210559/2627722 | 39228/1308401 | SIGSEGV; 11 emitted |
| powrat_wrong_centre_65537 | 0/2760383 | 0/1308401 | 0/522217 |
| root_wrong_unsampled_lift | 0/2760383 | 0/1308401 | 0/522217 |

## Fixture generators and tests that can agree with a wrong library

proto/lpow_checks.py computes Erel=r-s+v_p(e') and R from the displayed formulas, but then checks their predicted
cosets against independent modular integer power sets. The 4788 unscaled powrat image witnesses and 3748 powunit
image witnesses therefore do more than store the implementation's formula. The exact-input power centres come from
modular integer root enumeration, not the log/exp construction. The 23 hull rows check both signs and full versus
strict hulls modulo 16. The exact rational tests do integer root tests, but their integer implementation differs
from the C fmpz_root API. No unscaled successful image row was found to rely only on a copied radius formula.

The formula-only transformations are these:

- test_lpow.c:222-250 derives runtime scaled expectations from the stored unscaled row
  by e'j+E for j=-1,1,2, rather than enumerate each scaled image independently.
- Its 1260 stored n'=1 rows get the expected result from pow_si, which verifies the
  composition/delegation contract but cannot independently detect a shared pow_si defect.
- proto/lroot_checks.py:72-75 adds 250 scaled unit rows with E+shift, shift=-1 or 1,
  reusing the original centre and identifiers. These check that algebraic regression.

P7(c) and the scaling proof justify these transformations; they do not make them new independent numerical oracles.
A shared error in the transformation could survive them.

powers_at_prime in test_rfunc_prime.c:1185 and :1221 computes want from powrat or powunit and compares the wrapper
with that result. Those checks hold for many wrong local libraries. Their useful contract is projection, places,
statuses, transactions and aliases. The exact-27 and independent 1+3 Z_3 expectations cover two additional
mathematical cases. The root wrapper also has hand values 3 and 122/15622 at capped and image precisions, so it is
not wholly dependent on local expectations. Its small-prime fixture tests detect some enumeration faults. The table
records which actual faults each whole program catches.

f-repair4's large-list test_lroot.c:610-620 proves all identifiers are distinct and valid, and checks
shapes/exponents for all outputs. Its 16 powered-centre samples do not prove all lifts are correct. F8 demonstrates
an incorrect canonical centre that passes them.

Julia and driver fixtures were read without rerunning their prior campaigns. Their nontrivial displayed centres
satisfy the independent equations 6520516^2=6 modulo 5^10 and 379^3=3 modulo 2^10; 6520516 modulo 5^4 is 516. The
ball radii in the driver comments are hand instances of the same formulas, so those radius lines guard against
regression at the stated inputs. They are not an independent proof of the general formulas.

## Cost

P64 means 18446744073709551557. The machine reports Intel Core i7-1365U, x86_64, Linux 6.8.0-146. These are call
rates, reused initialized output objects, internally allocating library calls, one FLINT thread, seed 0,
CLOCK_MONOTONIC_RAW. Compiler: GCC 13.3.0, -std=gnu11 -O2 -g. FLINT 3.0.1; GMP 6.3.0. Calls are pinned to logical
cpu 0 or 1; at most two calls run concurrently. Other sessions can load the laptop, so wall times are provisional.
No conversion to cycles is made.

powrat input: x=1+2p exact, or (1+2p)+p^(N+3) Z_p; exponent -5/2, seed 1. j=0 and v_p(-5)=0 at the measured primes,
so Nr=N. Input centre bits are 3,18,65 at the three primes. The named reference is one root_seed at N and one
pow_si(-5). The output is a nonzero ball at N, with dense unit centre modulo p^N.

powunit input: u=1+p exact, s=2 exact; or u=(1+p)+p^(N+3) Z_p and s=2+p^(N+2) Z_p. Input centre bits are 3,17,64 for
u and 2 for s. R=N+3 and K=N on these ball inputs. The named reference is Log of the exact base centre at N, one
product by the exact exponent centre, and exp at N. The result centre is the small integer (1+p)^2. The exact
subtraction for alpha and final ball addition are outside that reference. This is the code's current component
composition.

The ratio is median total over median component sum, a REFERENCE upper-bound comparison, not a lower-bound ratio.
Min/median/max are shown. A one-trial row repeats that one observation in all three columns; it supplies no estimate
of timing spread. Three-trial rows execute the total and then the separate components in each trial. Six final
one-trial rows instrument the named component calls inside the same public call, avoiding a second hundred-second
computation. This changes only timing probes in a scratch lpow.c. Full modular output certificates run outside the
timed region. The powrat certificate is y^2*x^5=1 modulo p^N and y=1 modulo p. Since 2 is a unit and the branch is
fixed, it identifies the correct full residue. The powunit certificate compares the complete centre with (1+p)^2
modulo p^N. Other completed paired trials compare equal_set outside both timed regions. The same-call probes include
their small clock-reading overhead in the numerator.

0 of the 36 final median ratios exceed 1.5. There is no qualifying reviewed-code line to identify for those matched
comparisons. The largest costs are in the named component calls, not extra composition work: root_seed at lpow.c:105
and exp at lpow.c:219. The dense-input exp Horner multiplication/reduction is lfunc.c:276-280; it accounts for most
of the long rows. This is component cost, not a new composition-overhead finding.

Eight raw first-trial ratios exceed 1.5, all at N=100. powrat at 3 gives 5.736/5.086 (exact/ball), at 65537 gives
4.014/3.934. powunit at 3 gives 4.619/4.213, at 65537 2.400/2.374. These are unmatched initial-state comparisons:
cost.c:51-53 times the first total before cost.c:59-68 times the component sequence. The later trials have reused
outputs and prior arithmetic calls. The run does not isolate initialization, clock changes or cache effects from
wrapper overhead. These eight numbers cannot be assigned to a particular lpow.c line or used as an avoidable cost
finding. They are retained in cold-ratios.log and in each row's maximum rather than silently dropped.

Timeouts are part of this record. The initial 180-second supervisor ended during the second trial of the 65537
N=10000 ball row; one complete validated pair is retained. At P64,N=10000, a total-only phase completed in
112.001818964 seconds, but its subsequent component recomputation ran into the 150-second process cap. Two
concurrent separate phases also hit that cap. An isolated component phase completed in 96.725110058 seconds with a
full residue certificate. Those partial attempts are retained in phase logs. The final P64 exact row uses one fully
certified same-call measurement instead; the incomplete first attempt is not counted as a completed comparative
trial. There was no optimization or benchmark sweep between these measurements. The change of measurement method was
required to complete the comparisons under the process cap.

| Function | p | N | Input | Total min/med/max s | Components min/med/max s | Ratio | Trials |
|---|---:|---:|---|---:|---:|---:|---:|
| powrat | 3 | 100 | exact | 6.028e-05/6.348e-05/0.0003791 | 6.001e-05/6.609e-05/7.629e-05 | 0.960 | 3 |
| powrat | 3 | 100 | ball | 0.0001104/0.0001125/0.0004868 | 9.571e-05/0.0001057/0.0001069 | 1.064 | 3 |
| powrat | 3 | 1000 | exact | 0.00609/0.006195/0.006496 | 0.006102/0.006151/0.006414 | 1.007 | 3 |
| powrat | 3 | 1000 | ball | 0.009293/0.009356/0.009376 | 0.009225/0.009237/0.009416 | 1.013 | 3 |
| powrat | 3 | 10000 | exact | 1.734/1.82/1.825 | 1.647/1.794/1.816 | 1.014 | 3 |
| powrat | 3 | 10000 | ball | 1.706/1.723/1.751 | 1.672/1.694/1.734 | 1.017 | 3 |
| powrat | 65537 | 100 | exact | 0.0001842/0.0001898/0.0008033 | 0.0001854/0.0001867/0.0002001 | 1.016 | 3 |
| powrat | 65537 | 100 | ball | 0.0001854/0.0001934/0.0007808 | 0.0001838/0.0001852/0.0001985 | 1.044 | 3 |
| powrat | 65537 | 1000 | exact | 0.05202/0.05329/0.05416 | 0.05291/0.05337/0.05597 | 0.998 | 3 |
| powrat | 65537 | 1000 | ball | 0.05647/0.0582/0.06 | 0.05749/0.058/0.06056 | 1.003 | 3 |
| powrat | 65537 | 10000 | exact | 15.88/16.47/17.62 | 14.66/15.61/16.7 | 1.056 | 3 |
| powrat | 65537 | 10000 | ball | 15.25/15.25/15.25 | 15.4/15.4/15.4 | 0.990 | 1 |
| powrat | P64 | 100 | exact | 0.001673/0.002305/0.00242 | 0.002271/0.002289/0.002315 | 1.007 | 3 |
| powrat | P64 | 100 | ball | 0.002302/0.002317/0.002434 | 0.002282/0.002285/0.002328 | 1.014 | 3 |
| powrat | P64 | 1000 | exact | 0.6696/0.7074/0.7424 | 0.6639/0.6761/0.7055 | 1.046 | 3 |
| powrat | P64 | 1000 | ball | 0.6582/0.6616/0.7245 | 0.6652/0.6785/0.6923 | 0.975 | 3 |
| powrat | P64 | 10000 | exact | 107.7/107.7/107.7 | 107.7/107.7/107.7 | 1.000 | 1 |
| powrat | P64 | 10000 | ball | 101.5/101.5/101.5 | 101.5/101.5/101.5 | 1.000 | 1 |
| powunit | 3 | 100 | exact | 0.0001079/0.0001098/0.0005233 | 0.0001106/0.0001108/0.0001133 | 0.991 | 3 |
| powunit | 3 | 100 | ball | 0.0001077/0.000112/0.0004965 | 0.000105/0.0001123/0.0001178 | 0.997 | 3 |
| powunit | 3 | 1000 | exact | 0.008217/0.008763/0.009266 | 0.008045/0.008236/0.009043 | 1.064 | 3 |
| powunit | 3 | 1000 | ball | 0.008708/0.009166/0.009502 | 0.008575/0.008599/0.00878 | 1.066 | 3 |
| powunit | 3 | 10000 | exact | 1.588/1.631/1.998 | 1.56/1.595/1.792 | 1.023 | 3 |
| powunit | 3 | 10000 | ball | 2.046/2.05/2.125 | 1.918/2.009/3.876 | 1.020 | 3 |
| powunit | 65537 | 100 | exact | 0.0001842/0.0001864/0.0004642 | 0.0001847/0.0001934/0.0001946 | 0.964 | 3 |
| powunit | 65537 | 100 | ball | 0.0001836/0.0001856/0.0004546 | 0.000185/0.0001913/0.0001915 | 0.970 | 3 |
| powunit | 65537 | 1000 | exact | 0.1094/0.1129/0.119 | 0.114/0.115/0.1159 | 0.982 | 3 |
| powunit | 65537 | 1000 | ball | 0.1122/0.1124/0.1159 | 0.1091/0.1095/0.1103 | 1.026 | 3 |
| powunit | 65537 | 10000 | exact | 26.86/26.86/26.86 | 26.86/26.86/26.86 | 1.000 | 1 |
| powunit | 65537 | 10000 | ball | 23.11/23.11/23.11 | 23.11/23.11/23.11 | 1.000 | 1 |
| powunit | P64 | 100 | exact | 0.001586/0.0016/0.001664 | 0.001577/0.001584/0.001591 | 1.010 | 3 |
| powunit | P64 | 100 | ball | 0.001617/0.001624/0.001682 | 0.001569/0.001635/0.001654 | 0.993 | 3 |
| powunit | P64 | 1000 | exact | 0.6359/0.6535/0.6548 | 0.6407/0.6671/0.7083 | 0.980 | 3 |
| powunit | P64 | 1000 | ball | 0.6352/0.6544/0.6735 | 0.6413/0.6455/0.6561 | 1.014 | 3 |
| powunit | P64 | 10000 | exact | 96.96/96.96/96.96 | 96.96/96.96/96.96 | 1.000 | 1 |
| powunit | P64 | 10000 | ball | 103.4/103.4/103.4 | 103.4/103.4/103.4 | 1.000 | 1 |

## Commands and results

All paths below are relative to the repository root. The archive was built once: `timeout 600 make -j2
BUILD=lanes/f-review8/build lanes/f-review8/build/libadelefeld.a`. Exit 0; build-archive.log has the complete
compiler/archive output.

Mutation preparation: `timeout 180 python3 -B lanes/f-review8/mutations.py prepare`, exit 0, 3 test-object and 2
support-object compilations. Every nested compiler and linker is under timeout 150. `timeout 180 python3 -B
lanes/f-review8/mutations.py baseline`, exit 0: 9 tests/2760383 checks for lpow, 10/1308401 for lroot, 15/522217 for
rfunc_prime; all three have 0 failed checks. The archive is linked unchanged in every case.

Fault batch commands, each under timeout 180 with the same script: `mutations.py 0 4`, `4 8`, `8 12`, `12 16`, `14
16`, and `7 8` (the final sign fault). Batch exits are 0,0,0,1,0,0. The exit-1 batch ran the early-status and seed
faults, then stopped on a replacement-count assertion before compiling the centre fault. fix_mutation_scope.py
corrected that harness needle, after which 14..16 ran. refine_sign_fault.py corrected the sign fault to the exact
odd-parity image; its 7..8 rerun supplies the final table. All final faults compile/link. The table gives every
individual runner result; 54 actual executions include baseline and superseded runs.

`timeout 60 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review8/h.c lanes/f-review8/build/libadelefeld.a -lflint -lgmp
-lm -o lanes/f-review8/build/h`: exit 0. h.c is copied from f-review7; no earlier attack script was rerun. `timeout
180 python3 -B lanes/f-review8/proof_checks.py`: exit 0; 5760 group cases, 43200 point comparisons, 8640 tightness
pairs, 89950 precision inequalities, 2112 unions, 9636 exponent-period comparisons, and 3 endpoint calls; 0
failures. `timeout 30 python3 -B lanes/f-review8/proof_boundaries.py`: exit 0; 11296 inequalities, 0 failures.
`timeout 30 lanes/f-review8/build/h < lanes/f-review8/working_precision.in`: exit 0, 2 OK calls. `timeout 30
lanes/f-review8/build/h < lanes/f-review8/spec_precision.in`: exit 0, 1 OK call. `timeout 30 python3 -B
lanes/f-review8/audit_fixtures.py`: exit 0; 6930/3024/4352 fixture rows counted, 1260 degree-one rows, 4788
root-power images, 250 scaled root rows, 3 displayed integer certificates, and the 9-versus-27-residue SPEC example.

`timeout 60 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review8/alpha.c lanes/f-review8/build/libadelefeld.a -lflint
-lgmp -lm -o lanes/f-review8/build/alpha`: exit 0. `timeout 30 lanes/f-review8/build/alpha`: exit 0, 11 checks, 0
failures. `timeout 60 cc -std=gnu11 -O2 -g -Iinclude -Isrc lanes/f-review8/repro_r8.c
lanes/f-review8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review8/build/repro_r8`: exit 0. `timeout 60
lanes/f-review8/build/repro_r8`: exit 0, 15972 cases, 13485 nonsolvable, 0 failures. `timeout 180 python3 -B
lanes/f-review8/r8_count_details.py`: exit 0, same 15972 cases/0 failures, 5 digit products/6 powering calls/11
reference products. Its nested compile and test each have timeout 60.

`timeout 180 python3 -B lanes/f-review8/reproduce_survivors.py`: exit 0, 3 original/mutant pairs distinguished at
p^2; nested compiles use timeout 60, executions timeout 30. `timeout 30 python3 -B
lanes/f-review8/survivor_minimal.py`: exit 0, 1 pair distinguished, exact root witness 42. `timeout 180 python3 -B
lanes/f-review8/early_probe_build.py`: exit 0, 2 instrumented builds and 2 calls; identifier counts 0 and 1, both
LIMIT. Nested compiles use timeout 60, executions timeout 30.

Cost compilation: `timeout 60 cc -std=gnu11 -O2 -g -Iinclude lanes/f-review8/cost.c
lanes/f-review8/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-review8/build/cost`, exit 0. The command was
repeated after adding bounded-run arguments and flushing completed phases; no library code changed. cost_phase.c is
compiled with the same flags and timeout 60. `timeout 90 python3 -B lanes/f-review8/instrument_cost.py`: exit 0, one
scratch timing-only lpow source and one benchmark executable. Its nested compile has timeout 60.

Cost supervisors: `timeout 180 python3 -B lanes/f-review8/cost_run.py` (exit 124), then ranges `12 16` (0), `16 17`
(124), `18 24` (0), `24 28` (0), and `30 34` (0). The last range is pinned with `taskset -c 1`. Their case
executables all use timeout 150. The first range's 65537 ball row retains only its completed first trial. The P64
exact total's partial phase is recorded but excluded from the final table.

Each of these phase calls uses `timeout 150 lanes/f-review8/build/cost_phase`: `powrat P64 10000 0 components 0` and
`powrat P64 10000 1 total 1` (both exit 124); an isolated `powrat P64 10000 0 components 0` (exit 0, 96.725110058 s,
valid=1). P64 is expanded to its decimal value in the actual commands and log names.

Each final instrumented call uses timeout 150, executable build/cost_instrument, arguments `FUNCTION PRIME 10000
BALLFLAG total CPU`. Six calls all exit 0, valid=1: powrat P64 exact and ball; powunit 65537 exact and ball; powunit
P64 exact and ball. The table contains their total/component timings; each has one complete trial. No timed
executable exceeds its 150-second cap. Output comparisons are outside timing.

`timeout 30 python3 -B lanes/f-review8/tables.py`: exit 0; 16 final faults, 54 actual test runs, 0 test timeouts; 36
complete cost rows, 94 final timing trials, 0 median ratios over 1.5. `timeout 30 python3 -B
lanes/f-review8/cold_ratios.py`: exit 0; 8 initial-state ratios over 1.5. `timeout 10 lscpu` and `timeout 10 uname
-a`: exit 0; hardware/version metadata quoted above. The source hashes in source-sha256.txt match the files used by
the scratch copies. `timeout 30 python3 -B lanes/f-review8/final_checks.py preflight` and the same command without
preflight validate 36 cost rows, 16 fault rows, 94 final trials, 4 source hashes, report structure/line lengths and
exact survivor/precision residue certificates. The preflight result is 0 failures. The final check also validates
the file manifest.

Setup failures retained in progress.md: the apply_patch API returned empty results without creating two files; the
first prepare attempt therefore exited 2, running no tests. The files were then written and checked on disk. The
mutation replacement assertion above was also a harness failure, not a survivor. No state-changing git, bd,
installation or whole repository suite command was run. Core dumps were disabled (ulimit -c=0).

## Files written

All outputs are under lanes/f-review8/. files-manifest.txt lists their exact paths. The main text artifacts are
progress.md, report.md, the staging prose, fault-table.md, cost-table.md, cost-data.json, source-sha256.txt and the
per-run logs. Programs: h.c; proof_checks.py; proof_boundaries.py; alpha.c; repro_r8.c; repro_r8_counted.c;
r8_count_details.py; audit_fixtures.py; mutations.py; reproduce_survivors.py; survivor_minimal.py; early_probe.c;
early_probe_build.py; cost.c; cost_phase.c; cost_instrument.c; instrument_cost.py; cost_run.py; tables.py;
cold_ratios.py; assemble_report.py; final_checks.py. Small setup scripts are also listed in the manifest. Their
commands were `timeout 30 python3 -B lanes/f-review8/NAME.py`, with NAME=add_root_fault, fix_mutation_scope,
finish_cost_setup, cost_flush, refine_sign_fault, prose_corrections, final_setup; every command exited 0. They alter
only this lane's scratch harness or prose. build/ holds the one archive, objects, executables, timing-only source
copies, all 16 fault sources and their logs. The lane's pre-existing brief.md and automatic lane/stdout logs were
not edited by this work.

## Not done

No product source, public header, specification, proof file, assertion or fixture was changed. No repair is proposed
as an unreviewed patch. No new broad value/status/branch-list campaign, full repository suite, driver campaign,
Julia execution, long fuzz run or sanitizer archive was run. Numerical proof grids are finite; the written
derivations cover the general claims. The full branch-count maximum was not measured. Single-trial rows have no
timing-spread estimate. The eight raw cold ratios have not been assigned to a reviewed-code line because initial
states differ; no repeat cold-start campaign was performed to diagnose them.

## Sources pending

[source pending: FLINT 3.0.1 n_powmod2_ui_preinv implementation for exact internal product counts] Only its
documented operation is used: refs/src/flint-3.0.1/ulong_extras.rst:537-541. The primitive-root guarantee is read at
:1410-1413, unrestricted modular products at :368-373. No undisclosed FLINT algorithm or timing floor is quoted from
memory. Existing naming sources for Teichmueller representatives and Iwasawa Log remain pending in functions.md:101
and :343. The proof review uses their definitions and written algebra, not an external naming convention.
