# Lane m5-review1

## Findings

### F1. MAJOR: the real integral misses the stated tightness target

Input: eta0=(1,1), infinity, alpha=1, prec=160, and
S=[-3/4-r,-3/4+r]+i[-r,r], r=2^-12. The same result occurs for eta0=(5,4).

adf_local_tate_at returns OK. Its real component is
-5.8763302470786228758337517485210573506291695187246 +/- 0.003900786137819522991776466369629.
The largest coordinate diameter is 0.00780157227563904598.
The centre value is -5.8763302250987323497332598221822095197741707245826.
The coordinate diameter sampled at the centre and four corners is 0.000101160100176048497.
The output/sample ratio is 77.1210414191. The factor-64 target is 0.00647424641126710381,
with negligible arithmetic slack at prec=160.

The target was fixed before this grid ran. It is the rule described at docs/api-5a.md:101-104.
That paragraph explicitly separates the fixture target from the API enclosure contract.
This finding concerns tightness. No sampled value is outside the ball. It is not a violation of
SPEC's local enclosure contract or of the global 2^-bits rule.

For this input, image_bound=20 is sufficient: with z=S/2, Gamma(z)=Gamma(z+2)/(z(z+1)),
Re(z+2)>1.6, |Gamma(z+2)|<2, |z(z+1)|>0.23, and |pi^-z|<2.
Thus adding 20*2^-152 does not change the failure.
Independent quadrature of Gamma(z+2), followed by the two recurrence factors, reproduces the
centre and sampled diameter above.

Reproduce after rebuilding:
`taskset -c 0,1 timeout 170 python3 -B lanes/m5-review1/extra_checks.py real`.
Result: exit 1, 2594 checks, 2 tightness failures on 480 calls. The test was not weakened.

### F2. MAJOR: S12 uses a false entire-function justification and omits a reference factor

docs/api-5b.md:250-252 includes conductor 1 but calls Lambda an entire function.
For conductor 1, Lambda has poles at 0 and 1; analysis.md:576-577 states them.
The conclusion at s0=-2,-4,... is finite, but the justification must be local regularity from S9.
The similar wording at docs/api-5b.md:58 also needs the conductor-1 qualification.

At s0=-e-2k, the completed limit is
2*(-1)^k*(pi/C)^k*L'(s0)/k!, not the naked 2*(-1)^k*L'(s0)/k! quoted in the reference
description at docs/api-5b.md:255.
For chi=(1,1), s0=-2, the latter is 0.0608969141167865415605030609423.
The true completed value is 0.1913132980155851711250765701204, which is pi times that number.
The code returns the latter with OK at bits 80. The generator applies the factor correctly at
lanes/t5-slice4/gen_vectors.py:114. This is a statement/proof finding, not a wrong implementation value.

Reproduce the numerical distinction:
`timeout 20 python3 -B -c 'import mpmath as m; m.mp.dps=60; print(-2*m.zeta(-2,1,1));
print(-2*m.pi*m.zeta(-2,1,1))'`.
Reproduce the implementation with runner input
`0 1 1 1 0 -2 0 9999 1 0 9999 80 128` under `timeout 30 lanes/m5-review1/runner`.
Its continuation and completion results enclose 0.1913132980155851711250765701204.
At s=1 the same character gives DOMAIN, contradicting the unqualified entire-function description.

### F3. MINOR: the production FE copy fault survives all five C tests

The scratch production change is tools/adf/adf.c:3592, `acb_mul(R,W,B,wp)` replaced by `acb_set(R,A)`.
Input: tate_fe, chi=(1,1), s=1/2, bits=80, default prec=128.
All five C tests exit 0: 22747, 47955, 22904, 24261 and 19126 checks respectively.
The mutated driver prints overlap=yes with A and R copied bit for bit.
Its imaginary radius for R is 2.1e-26; the live driver prints 1.5e-27.
The numeric value remains correct, so this does not lose enclosure or a sign.

docs/api-5b.md:396-401 requires independent sides. The named independence and hand tests exercise
fe_sides, a separate implementation, rather than adf_drv_tate_fe.
The existing driver fixture can detect the printed-radius difference; the five C tests cannot.
Reproduce: `taskset -c 0,1 timeout 170 python3 -B lanes/m5-review1/faults.py 11`.

### F4. MINOR: a permitted precision-cap call exceeds the review timing budget

Input: adf_tate_completed, chi=(1,1), s=1/2+2^-40 i, bits=53, prec=2097152.
Three 60-second probes time out without a library status or value.
The final one-input, one-CPU probe exits 124 after 60.0095 s, using 51.5547 user CPU seconds
and 4.6326 system CPU seconds. The true value is real and approximately
-3.9769662255065128793021766827979598091952966270937.

include/adelefeld/tate.h:64-67 permits this starting precision. It does not promise LIMIT at equality
with the cap. This is a failure of the requested 60-second timing target, not a wrong-status claim.
Reproduce: `taskset -c 0,1 timeout 170 python3 -B lanes/m5-review1/cap_checks.py`.
The last case times out. The other five cases return within 4.331 s.

### F5. MINOR: the precision-cap HEADER-FINDING remains a design discrepancy

The headers now state exact inherited delegation: include/adelefeld/localfactor.h:100-103
clips guards on the new finite path, but preserves the existing policy on trivial and real calls.
The existing policy at localfactor.h:44-46 is max(2,prec)+32.
Therefore at prec=2097152 the delegated working precision is 2097184.
This is the delegation reading of api-5:80, not the guard-cap reading of api-5:33.
The source explicitly records the exception at src/localfactor.c:491-493 and :754-756.

Concrete input: local Tate at p=2, eta0=(1,1), alpha=1, s=2, prec=2097152.
Its first logarithm receives 2097184 bits, by src/localfactor.c:45 and :63.
The value is 4/3. No output-enclosure defect is asserted.
Reproduce the contract discrepancy by reading those lines; no expensive cap computation is needed.

## Findings against the specification

No counterexample to SPEC 8's values or signs, 9.3.7's pole rules, or N-D24's accepted global widths
was found in the cases below. F1 is a stated test-target failure. F2 is a false justification in the
slice statements. F3-F5 concern test coverage, timing and the documented precision policy.
No live certificate below its true integral, forbidden continuation call, memory fault, or failure-output
write was observed. Numerical comparisons are not a proof over all inputs.

## Independent derivations and proof review

The formulas were derived before reading any slice report or implementation. No proto/tate_checks.py
or t5 reference-vector value was used for the verdict. The repository tests were used for fault detection.

Each p-adic shell has multiplicative volume 1. The unramified integral is the convergent sum of
alpha^k*p^(-ks). The ramified inverse-character vector cancels eta0 on the units and has integral 1.
Its negative-kernel transform is q^-1 times the residue-class DFT, supported on the shell v_p(y)=-a.
A separate unit average against eta0^-1 gives alpha^a*p^(-as)*G_minus.
The unramified gamma is the ratio of two independently summed shell integrals in their common strip.
Outside that strip, its continuation is the ratio derived from the geometric sums.

At infinity, x=exp(u) gives the direct integral
2*integral exp((s+e)u-pi*exp(2u)) du. Differentiating the positive Gaussian transform gives
F(phi_e)=i^e*phi_e. This fixes gamma and epsilon's signs.
Finite integral poles solve alpha*p^-s=1; finite unramified gamma poles solve alpha^-1*p^(s-1)=1.
Their numerator and denominator cannot vanish together, since their exponential product is 1/p.
Real integral poles are -e-2k; gamma poles are 1+e+2k and its zeros are -e-2k.
Ramified gamma/epsilon and real/unramified epsilon have no spectral poles.

The local FE attack uses SPEC 9.3.7 and P9: eta^-1 at 1-s.
The brief's extra |.| would count the norm twice under its definition of Z.
At p=2, eta0=(4,3), alpha=1 and s=1/2, the prescribed transform has shell norm 4.
The correct ratio is -i; adding that extra |.| changes it to -4i.
This is a qualification of the brief, not a finding against SPEC.

S9, step by step:

1. The defining integral in Re(s)>1 gives I=pi^-z*Gamma(z)*L(s,chi).
2. The negative finite DFT and positive real transform multiply to tau/(i^e*sqrt(C)).
3. Split the balanced theta integral at t=1. Inversion gives z'=(1-s+e)/2 and the terms
   1/(s-1)-1/s for conductor 1.
4. n^2*t>=n^2+t-1 gives an exponential majorant uniform on compact spectral sets. L6 and L14
   integrate that majorant. Both decaying integrals are entire.
5. Their sum with the rational terms is meromorphic, and agrees with the defining value in the
   half-plane. The identity theorem supplies the continuation.
6. Multiplication by C^-z is entire and nonzero. It preserves the stated pole set.

No false step was found in S9. Its named analytic imports remain source obligations below.

S12, step by step:

1. For C>1, the split is entire. For C=1, the permitted s0 are regular points of a meromorphic split.
2. Gamma((s+e)/2) has leading term 2*(-1)^k/(k!*(s-s0)).
3. Equality away from the local Gamma pole and finiteness of the split force L(s0)=0.
4. The prefactor pi^-z0 is pi^k. Completion contributes C^-k. This gives the corrected limit in F2.
5. No general nonvanishing theorem is needed to evaluate the finite limit. Excluding zero in the
   tested balls is a finite observation.

## Attacks and counts

The installed tools are mpmath 1.3.0, sympy 1.14.0, python-flint 0.8.0 on FLINT 3.3.1.
The C build uses FLINT 3.0.1. The requested Python version 0.9.0 was not installed here.
No package was installed. Python references used 60 decimal digits and separate 256-bit FLINT values.
The numerical membership tolerance is 1e-50*max(1,abs(reference)); exact mathematical zeros and constants
are treated separately where known. This tolerance is below the global width targets, but can exceed a
very small local coordinate radius. An independently certified point-box sweep was not done.

- Phase model: 797 descriptors, 54842 evaluated phase comparisons, 0 failures.
- Finite local calls: six primes, 49 primitive/trivial descriptors, 12 alpha choices, 40 spectral points,
  alpha/s radii and sampled corners. 23520 inputs, 70560 OK calls, 272887 checks, 0 numerical failures.
  The largest width/announced-bound ratio is 0.222479.
- Real defining integrals: 24 direct quadratures and 4 FE quadrature ratios, 84 checks, 0 failures.
- Real continuation grid: 160 inputs, 480 calls, 2594 checks, 2 tightness failures recorded in F1.
- Status/pole grid: 222 inputs, 666 calls: 375 OK, 157 DOMAIN, 122 NOT_DETERMINED, 12 LIMIT.
  951 checks, 0 failures. The nonzero pole-lattice grid adds 144 inputs: 288 OK and 144 NOT_DETERMINED.
  Five raw nonfinite cases add 15 calls. Their combined 615 checks have 0 failures.
- Global matrix: all 17 golden descriptors after lowering, 16 additional descriptors, hence conductor 1
  plus 30 distinct primitive nontrivial characters up to conductor 40. 6534 inputs and 19602 calls:
  14422 OK, 4467 DOMAIN, 713 NOT_DETERMINED; 61177 checks and 2814 reference points, 0 failures.
  The largest accepted diameter/2^-bits is 0.9583398933. Bits are 20, 53 and 80.
- The main odd-character grid contains a duplicate -3. The -4.5 supplement supplies 60 distinct centres
  per character: 20 descriptors, 60 extra inputs, 120 OK results, 422 checks, 0 failures.
- Radius inputs use 2^-30 and 2^-60. Every accepted ball was checked at its centre and four corners.
  Near-pole cases at 2^-40 with radius 2^-200 add 32 inputs and 72 OK results: 602 checks, 0 failures.
- Vectors: 31 primitive descriptors, 522 entries and 66 calls with two stored exponents; 586 checks,
  0 failures. P=x^e, A=1, B=C_real=0, D=1 and M=C were checked.
- FE: 155 cases over 31 descriptors, two independent completed calls, an exact cyclotomic W,
  both references and the corners of radius inputs. 1029 checks, 0 failures.
- Certificates: 100 trace cases, 1284 panel thresholds, 1986 checks, 0 failures. Recomputed omitted
  and panel errors are no larger than the added radii; the largest error/radius ratio is 0.999999999985.
- L14: 96 direct bound probes versus quadrature, 63 first-branch and 33 second-branch cases;
  194 checks, 0 failures. T3: four 20-panel runs, 80 true Taylor-tail comparisons, 93 checks, 0 failures.
- Width/refusal: 2000 random inputs, 1219 OK within target and 781 NOT_DETERMINED; 2008 checks,
  0 failures. Twenty-four endpoint-separation witnesses all give NOT_DETERMINED: 96 checks, 0 failures.
- Driver: 60 malformed lines, 8 lowering comparisons, 24 numeric lines and 30 printed balls;
  128 checks, 0 final failures. Expected malformed lines were fixed in the generator before the run.

A case was counted as a numerical failure on missed reference membership, a width failure on a diameter
above its declared target, and a status failure on disagreement with the derived status or changed
failure storage. The certificate probes fail if a bound is below quadrature or the recomputed formula.
The fault tests fail through their own assertions; their exit codes are recorded next.

## Fault table

Columns are test_tate_local, test_tate_gamma, test_tate, test_tate_continue, test_tate_fe.
P means exit 0; K means exit 1. Each scratch source and all five test binaries compiled with exit 0.
The first twelve are the selected requested faults. The last two replay a reported survivor and add
an explicit 5a fault. No repository source or test was changed.

| Index | Fault | Local | Gamma | Tate | Continue | FE |
|---|---|---|---|---|---|---|
| 0 | G_minus kernel positive | P | K | P | P | P |
| 1 | eta0 instead of inverse | P | K | P | P | P |
| 2 | Integral pole list for gamma | P | K | P | P | P |
| 3 | W conjugated | P | P | K | K | K |
| 4 | Dual z instead of z' | P | P | K | K | K |
| 5 | Wrong pole-term sign | P | P | K | K | K |
| 6 | L14 factor 2 omitted | P | P | K | K | P |
| 7 | T3 remainders omitted | P | P | K | K | K |
| 8 | C^-z applied twice | P | P | K | K | P |
| 9 | Width checked before added error | P | P | K | P | P |
| 10 | Gamma division for rgamma | P | K | P | P | P |
| 11 | Production FE side copied | P | P | P | P | P |
| 12 | Reported L14 second-branch fault | P | P | K | P | P |
| 13 | Ramified standard integral zero | K | P | P | P | P |

Faults 0, 1, 2, 10 and 13 produce 542, 434, 34, 37 and 1026 failed checks respectively in the
relevant local test. Fault 9 is killed at test_tate.c:283 by the final width assertion.
Fault 12 is killed at test_tate.c:314 by a cutoff mismatch. Continuation and FE alone accept it.
At r=1/4, b=1/8, R=1, its J_bound upper endpoint is 3.905019054260691404,
while direct integration gives 11.448275904614192468.
This is an unsound scratch certificate, not an unsound live certificate. The combined five tests detect it.
Fault 11 survives the C tests; the driver radius distinction is F3.

## Check-line audit

The 30 Check lines were read against the named test bodies and the targeted baseline runs.
api-5a line numbers: 50, 51, 52, 73, 109, 111, 236, 239, 242, 245, 248.
api-5b line numbers: 48, 73, 88, 103, 118, 125, 136, 213, 229, 244, 254, 263, 269,
335, 352, 365, 385, 394, 401.

The production independence claim attached to api-5b:401 is not checked by independence or hand:
those functions call fe_sides rather than the production handler, as demonstrated by fault 11.
The other 29 lines have their named checks; no missing invocation was found.
Cutoff equality is not a direct test of the truth of L14's second branch. Its absence from the
continuation suite remains visible in fault 12. The independent review supplies 33 direct second-branch cases.
S12's completion-factor description is wrong even though its generated numeric references are correct.

## Commands and results

R below means lanes/m5-review1. P means
`taskset -c 0,1 timeout 170 python3 -B`. Every C subprocess invoked by the generators also has timeout,
usually 160 s. Individual programs used at most 170 s; each archive build completed under its explicit
600 s build timeout. Builds used -j2. Later numerical and fault work was pinned to CPUs 0 and 1.
The first startup probes were not pinned; that initial overlap did not guarantee the two-core bound.
Elapsed review time exceeded the two-hour aim.

- `timeout 600 make -j2 BUILD=R/build R/build/libadelefeld.a`: exit 0, one archive build.
- Same command with BUILD=R/build-san, SAN=1 INV=1: exit 0, one archive build.
- `timeout 60 cc -O2 -g -Iinclude R/runner.c R/build/libadelefeld.a -lflint -lgmp -lm -o R/runner`:
  5 harness compilations, exit 0; SAN/INV variant with -O1 and sanitizer flags: 3, exit 0.
- Corresponding cc commands for tools/adf/adf.c, certprobe.c and taylorprobe.c: 3 builds, exit 0.
  The probes also use -Isrc. The driver links the ordinary lane archive.
- `timeout 170 make -j2 BUILD=R/build` with the five test targets: exit 0, 5 binaries.
- `timeout 160 R/build/test_tate_local`: exit 0, 22747 checks; test_tate_gamma: 47955;
  test_tate: 22904; test_tate_continue: 24261; test_tate_fe: 19126. All five exits are 0.
  The forbidden-path child's abort messages in test_tate_continue are expected self-tests.
- `P R/checks.py phase`: exit 0, 54842 checks.
- `P R/checks.py local I`, I=0..5: 6 final exits 0, 272887 checks.
- `P R/checks.py global I`, I=0..32: 33 final exits 0, 61177 checks.
- `P R/checks.py real`: exit 0, 84 checks.
- `P R/extra_checks.py real`: exit 1, 2594 checks, 2 failures; vector: exit 0, 586;
  near: exit 0, 602. `P R/grid_supplement.py`: exit 0, 422.
- `P R/certificates.py bounds`: exit 0, 1986; jbound: exit 0, 194.
  `P R/taylor_checks.py`: exit 0, 93.
- `P R/statuses.py status`: exit 0, 951; witness: exit 0, 96.
  `P R/more_statuses.py`: exit 0, 615.
- `P R/statuses.py random I`, I=0..3: 4 exits 0, 2008 checks.
- `P R/fe_checks.py I`, I=0..2: 3 exits 0, 1029 checks.
- `P R/driver_checks.py`: final exit 0, 128 checks.
- `P R/faults.py I`, I=0..13: 14 completed generator exits 0; 14 changed-object builds,
  70 test links, the 70 run exits in the table, and two extra links for the driver and bound probe.
- `ASAN_OPTIONS=detect_leaks=1 P R/memory.py 0`: exit 1 solely at LSan's fatal ptrace error;
  500 input rows completed, no address/undefined-behaviour diagnostic.
- With detect_leaks=0, `P R/memory.py I`, I=0..3: 4 exits 0, 2000 inputs, 6000 calls,
  8346 checks. Returned statuses: 5338 OK, 636 DOMAIN, 26 NOT_DETERMINED.
- With ADF_REVIEW_RUNNER=runner-san and detect_leaks=0, status, witness and more_statuses commands:
  exits 0, 951/96/615 checks. The four fast cap inputs under `timeout 60 R/runner-san` give
  LIMIT, LIMIT, LIMIT, NOT_DETERMINED; exit 0, 0 sanitizer diagnostics.
- `P R/memory.py 5 runner-valgrind 200`: final exit 0, 832 checks, 99.039 s,
  15472 allocations and frees, 0 blocks/bytes at exit, 0 errors.
  Valgrind uses --error-exitcode=9 --track-origins=yes --leak-check=full --show-leak-kinds=all
  --errors-for-leak-kinds=definite,indirect. The final direct-command adapter smoke test adds 1 row,
  2 checks, exit 0.
- `P R/cap_checks.py`: generator exit 0; the five short cases return LIMIT, LIMIT, LIMIT,
  NOT_DETERMINED, LIMIT in 0.013/3.010/0.376/0.373/4.331 s. The sixth subprocess exits 124 at 60.0095 s.
  Earlier two cap probes also exited 124; the combined cap and bits-cap probes exited 0.
- `timeout 20 python3 -B` for the positive-Gamma quadrature recurrence and S12 reference distinction:
  exit 0, the numbers in F1/F2. Version/capability probes exit 0 with the versions above.
- `timeout 20 python3 -B R/summarize.py`: exit 0, no missing final matrix or fault record.
  `timeout 10 sh -n R/rebuild.sh`: exit 0.

Nonfinal review-harness runs: the initial global reference string parser exited 1; (8,5) at s=0 then
produced 5 failures because mp.diff lost digits internally. Explicit Hurwitz derivatives fixed the
reference and its second-source agreement. Neither was attributed to the C library.
The initial argv driver harness exited 1 on a format exception; global commands require script input.
A subsequent table had 8 wrong expectations because value text lowers imprimitive characters whereas
strict dump loading rejects them. The final table retains those as separate lowering checks.
The first Valgrind run timed out after 161 of 200 rows and found 28 reads of inactive acb storage in
my success-byte comparison. Failure snapshots now initialize their sentinel's whole storage, and raw
comparison is only on failure. The corrected run is the zero-error 200-case result above.
Two multi-case parent batches reached timeout 170 while staging faults 8/9 and conductor 29;
individual reruns completed. No parent-timeout partial record is counted as a completed check.
The exploratory --help and direct global argv calls exited 2. No whole repository suite was run.

## Sources and limits

Ground truth read on disk: refs/src/tate-poonen/notes.txt:53-65, 693-700, 733-740, 890-1027,
1108-1176; refs/src/flint-3.0.1/acb_dirichlet.rst:129-132, 203-208, 508-579;
acb.rst:6-17, 893-902; dirichlet.rst:15-31, 45-65; the generator setup at
refs/src/flint-src-3.0.1/dirichlet/group_init.c:17-64.
Project contracts read: CLAUDE.md; SPEC 8, 9.3.7, N-D23/N-D24; PLAN milestone 5;
api-5 sections 1-9; api-5a and api-5b; conventions 6.1-6.5 and rows 223/225;
analysis P3/P4, L6/L8/L14 and P9-P15; design/local-zeta.md; all five slice reports.

[source pending: universal Conrey generator/label identification beyond the finite comparisons]
[source pending: inherited Haar, Fubini, dominated convergence, holomorphic parameter integration,
identity theorem and related analytic imports listed in analysis.md]
[source pending: a quantitative numerical-error contract for mpmath Hurwitz/Gamma/upper incomplete Gamma]
No universal signed quadratic Gauss-sum theorem was assumed. No new source was fetched.

Not done: certified independent point boxes for every tiny local coordinate; exhaustive full-image
sampling; a Julia run; a leak-enabled 2000-case run (ptrace prevents it); equivalence proofs or new
killing inputs for the inherited Gamma-bound survivors and the reported dropped dual-exponent bound;
an exact total-work-count test. The L14 survivor was replayed as fault 12, not declared equivalent.
The record makes no claim about untested cases.

## Files written and cleanup

Retained in lanes/m5-review1: progress.md, models.py, runner.c, checks.py, certificates.py,
certprobe.c, taylorprobe.c, taylor_checks.py, statuses.py, more_statuses.py, driver_checks.py,
extra_checks.py, fe_checks.py, memory.py, faults.py, grid_supplement.py, cap_checks.py,
summarize.py, rebuild.sh and report.md.
A final source-line audit corrected stale citations after the initial report write; the write-once
request was not met. No numerical result or finding changed.
The same final text is written last to docs/reviews/m5/review-m5.md.

The ordinary/SAN archives, objects, test executables, probes, driver, scratch sources, generated
fault JSON and generated logs are removed. The pre-existing brief.md, lane.log and stdout.log are kept.
No lasting /tmp file is left. No git state command, bd, package installation, source repair, or change
to a non-owned existing path was performed.
Rebuild from the repository root with `taskset -c 0,1 timeout 170 sh lanes/m5-review1/rebuild.sh`.
Then run the generators above; each executable they invoke is bounded by timeout.
