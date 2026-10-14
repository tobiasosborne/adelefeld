# Lane j-slice3: report

Step J-a.3 is built. The final bounded Pkg.test passes 3568/3568: J-a.1 904, J-a.2 1015, J-a.3 1649.
The baseline was 1919/1919. Eleven of twelve planted faults were caught. The numerical address taken
before preserve was not caught and remains a review obligation. The specification and design were not changed.

The requested session prints:

    (0.33333333333333333332 +/- 3.2e-20 ; 16/3 mod 18)

Command: timeout 35 sh lanes/j-slice3/session.sh. The script evaluates:
using Adelefeld; Adele(Rat(1, 3); prec = 64) + Adele(parse(FBall, "5 mod 18"); prec = 64).
The session also reports 89 required symbols and five layout rows.

## What was built

- Adele and CAdele own aligned, initialized C storage, with independent literal rows 96/8 and 144/8.
  Init, cached clear, finalizer, close, copy, identical and is_canonical follow the existing handle mechanism.
  Repeated close/finalize is safe. Accepted operations refuse closed inputs before C and result allocation.
- RealBall and ComplexBall own a private initialized Adele/CAdele with exact finite 0. They have no separate
  prefix finalizer. The whole owner clears once. Every returned numerical carrier checks copied finite
  centre and radius for exact 0. Numerical addresses are produced inside preserve blocks.
- Constructors from Rat, Integer and Rational at prec; Adele from RealBall and FBall; CAdele from
  ComplexBall and FBall; exact Adele embedding into CAdele; ComplexBall from two RealBalls; typed text
  parsers and try_parse; C text printers, show and string. adf_str_free is in a finally, including NULL output.
- Operators +, -, *, unary -, and division by exact Rat for adeles, with explicit Integer/Rational
  conversion. Named/try binary forms accept prec=64; operators use 64. Scalar calls use *_rat. Subtraction
  uses exact negation and add_rat, since C has no sub_rat. Both Adele/CAdele embedding orders are supported.
  Division by 0 retains NOT_UNIT. Unsupported adelic Number pairs raise ArgumentError.
- Numerical carriers have same-type +, -, *, unary -, with prec on named/try binary forms.
  Mixed carrier arithmetic raises MethodError. They expose no finitepart and are not Number subtypes.
- finitepart returns a fresh FBall; realpart and complexpart return fresh numerical carriers.
  The real getter is adf_adele_get_real, so no Place type is needed.
- ^ on FBall, Adele and CAdele is refused with ArgumentError naming future pow. There is one ^ method
  per type. One shared literal_pow forwarding method also refuses negative literals.
- Precision must be an Integer fitting Int64 before C. Below 2 is passed to C, which clamps to 2.
  Parser default limits are obtained from C only after that guard. digits is checked in 1..1000000.

Important interface choices: Adele(f::FBall; prec) uses exact numerical 0, as the session requires.
ComplexBall(re, im) copies through acb_set_arb_arb, resolved from the same core handle's FLINT dependency.
Neither operation guesses a numerical coordinate from the finite centre or rounds through decimal text.

## Files written

- New: julia/Adelefeld/src/adele.jl (341 lines), src/carrier.jl (208 lines).
- Changed: src/Adelefeld.jl (exports/includes), src/load.jl (42 symbols and two rows),
  src/fball.jl (only the ^ refusal).
- Appended: test/runtests.jl, one outer testset "Adelefeld step J-a.3"; test/fixtures/fault.c,
  four layout queries and one default-limit counting fixture.
- docs/api-ja.md: Step J-a.3, statements S21-S27, DESIGN-FINDINGs 22-32; resolution under finding 19.
- lanes/j-slice3/: gate.sh, session.sh, help.sh, faults.py, powers_test.jl, check_files.py,
  summarize_faults.py, wrap_logs.py, cleanup.py, redgreen.md, this report, and the retained logs.

No C production source, header, SPEC, PLAN, api-j.md, tests/test_julia.sh or earlier testset was changed.
No git command, bd, system package installation or delegation was used.

## Checks and commands

Every gate below is timeout 200 sh lanes/j-slice3/gate.sh, redirected to the named log.
The gate itself uses timeout 180, one Julia thread and one precompile task, the system GMP preload,
JULIA_PKG_OFFLINE=true, and JULIA_DEPOT_PATH=<lane>/depot:$HOME/.julia: with the trailing colon.
Julia temporary files are directed to the lane. Fault attempts ran one at a time.

| Command / log | Exit | Result |
|---|---:|---|
| timeout 600 sh tests/test_exports.sh, exports.log | 0 | 625/625 exported; 0 missing, extra or variadic |
| gate, before.log | 0 | 904 + 1015 = 1919/1919 |
| gate, red.log | 1 | old 1919 pass; new 17 pass, 14 fail, 12 error / 43 |
| gate, green1.log | 1 | old 1919 pass; new 1543 pass, 5 fail / 1548 |
| gate, green2.log | 0 | old 1919 pass; new 1557/1557; total 3476 |
| gate, green3.log | 1 | old 1919 pass; new 1644 pass, 1 fail / 1645 |
| gate, green4.log | 0 | old 1919 pass; new 1645/1645; total 3564 |
| gate, final.log | 0 | old 1919 pass; new 1645/1645; total 3564 |
| gate, limits-red.log | 1 | old 1919 pass; new 1648 pass, 1 fail / 1649; 24 premature C calls |
| gate, limits-green.log | 124 | 180 s bound; waiting on subprocess; no completed summary |
| gate, limits-green2.log, final implementation | 0 | 904 + 1015 + 1649 = 3568/3568 |
| timeout 35 sh lanes/j-slice3/session.sh | 0 | printed line above; 89 symbols, five rows |
| timeout 35 sh lanes/j-slice3/help.sh | 0 | installed Julia 1.12.5 help captured |
| timeout 30 python3 lanes/j-slice3/summarize_faults.py | 0 | twelve summaries; pass/fail/error/total |
| timeout 30 python3 lanes/j-slice3/cleanup.py | 0 | five targets, four removed, zero remaining |
| timeout 30 python3 lanes/j-slice3/check_files.py | 0 | fifteen authored files/regions; zero violations |

The parsing smoke check used timeout 30 with the gate environment and the four types.
limits-smoke.log: exit 0; four valid 1.5 parses and four ArgumentErrors for prec=1.5.
The literal-power tests used timeout 30 with the same environment:
julia --startup-file=no --project=julia/Adelefeld lanes/j-slice3/powers_test.jl.
powers-red.log: exit 1, 9 pass and 6 fail / 15. powers-green.log: exit 0, 15/15.
The earlier powers.log diagnostic made fifteen calls; negative literals gave the wrong refusal before the fix.

An initial help capture with @doc(x) yielded nothing; Base.Docs.doc on the macro function raised MethodError.
Using REPL and evaluating the quoted @doc expressions produced the retained text/plain documentation.
These were capture attempts, not arithmetic checks.

The red-green history is in redgreen.md. The five green1 failures were a zero-live check in a scope
retaining 49 earlier inputs; zero-live acceptance now runs in a fresh process. The green3 failure was
automatic GC reducing a live-count snapshot from 628 to 317. Parent exact-count checks freeze automatic GC
and restore it in a finally. The separate GC subprocess keeps automatic GC enabled.

The actual binding defect found late was eager default TextLimits construction before the precision guard.
The red counting fixture observed 24 ArgumentErrors and 24 C default-limit calls, with 0 live handles.
After the fix it reports ERRORS=24 LIMIT_CALLS=0 LIVE=0. The first green attempt timed out; its cause was
not isolated. The bounded parsing smoke and full rerun passed. No assertion was weakened.

The final gate's test times are 50.4 s, 18.6 s and 42.7 s for J-a.1, J-a.2 and J-a.3 respectively.
Its GC subprocess prints BASELINE=0 and LIVE=0 ERRORS=0 for each of Adele, CAdele, RealBall and ComplexBall
after 10000 iterations per type, each making an input, copy, sum and negative, followed by four collections.

Numerical checks: five SPEC 4.3 rows through Adele at prec 64; actual rational containment and expected finite
equal_set; representable dyadic exactness and positive radius for 1/3; precision forwarding and clamping;
fifty random real/complex pairs against exact dyadic BigFloat results at 256 bits (750 checks);
thirty real and thirty complex adele enclosure round trips; exact two-ball copying at 192 bits;
zero imaginary embedding; NULL printers; failures, refusals, closed operands, independent projections and GC.

## Fault table

Each completed run used timeout 180 python3 lanes/j-slice3/faults.py <id>.
Every mutant lived in a scratch package under the lane; the package sources were never mutated.
The retained fault-<id>.log records its complete Pkg.test output.

| Fault | Pass / fail / error / total | Result |
|---|---|---|
| F1 carrier frame removed by wrong width | 1548/9/0/1557 | caught, exit 1 |
| F2 ring precision replaced by 64 | 1545/12/0/1557 | caught, exit 1 |
| F3 set_adele embedding skipped | 1550/7/0/1557 | caught, exit 1 |
| F4 carrier exact-zero assertion omitted | 1643/2/0/1645 | caught, exit 1 |
| F5 arb address read outside preserve | 1645/0/0/1645 | not caught, exit 0 |
| F6 div_rat status ignored | 1605/40/0/1645 | caught, exit 1 |
| F7 adele layout row not gated | 1638/7/0/1645 | caught, exit 1 |
| F8 real getter called at a finite place | 1408/237/0/1645 | caught, exit 1 |
| F9 all numerical binary operations routed to add | 1445/200/0/1645 | caught, exit 1 |
| F10 two-ball constructor loses imaginary part | 1594/51/0/1645 | caught, exit 1 |
| F11 eager Adele/CAdele default limits | 1648/1/0/1649 | caught, exit 1 |
| F12 eager carrier default limits | 1648/1/0/1649 | caught, exit 1 |

F1-F3 ran before the further boundary checks; the later totals are shown explicitly.
The first command passed F1 and F2 together and reached its outer 180 s timeout during F2.
F2 was rerun alone. One first F12 attempt reached its 175 s child bound with no test summary.
It is unverified, not a caught fault: fault-F12-timeout.log and fault-timeouts.log retain it.
The solo F12 rerun caught the mutant. The harness now labels a run without a summary as unverified.

F5 cannot be established by this bounded suite. It has no forced collection between the pointer read
and C, and the owner stayed rooted in the observed calling frame. Production code makes early checks
without producing pointers and takes every numerical address inside preserve. This requires review.

Check logs are retained with lines wrapped at 116 characters, without deleting text.
Command: timeout 30 python3 lanes/j-slice3/wrap_logs.py; the initial pass checked 29 logs and wrapped 20.
An audit initially found six overlong lines in the live runner stdout transcript. Rewrapping found zero;
the runner continues to append it. stdout.log and stderr.log are now excluded from the wrapping helper.

## DESIGN-FINDINGs

22. realpart/complexpart names follow the C getters; the convenience real getter avoids Place.
23. The radius grammar accepts decimals, not 2^-10. That example returns PARSE; use 0.0009765625.
24. Adele(FBall) explicitly injects numerical exact 0 for the required session.
25. The two-real-ball complex constructor needs acb_set_arb_arb from the same handle's FLINT dependency.
26. Unsupported adelic Number pairs raise ArgumentError; unsupported carrier/try_ pairs raise MethodError.
27. Prec uses the full representable Int64 range and the C clamp; digits uses the stated printer range.
28. Carrier printing removes one leading "(" and the five-character suffix " ; 0)". Parsing adds them back.
29. Finding 19 is resolved by ^ refusals on the three adelic ball families.
30. The numerical pointer fault is an uncaught preserve review obligation.
31. Negative literal powers need shared literal_pow forwarding, because Julia otherwise calls inv first.
32. TextLimits defaults must be deferred until after precision validation. nothing selects those C defaults.

## What is not done

No contexts/leases, Place, local values, scaled/cap surface, dump, reconstruction, text classification,
readvalue, numerical set predicates, pow implementation, Nemo bridge, two-thread support or fuzzing.
Only the adele header's tight policy is used; scaled and cap policies are outside this step.
The numerical matrix and round trips are bounded differential checks; the GC loop is an ownership smoke test.
tests/test_julia.sh was not changed or run; the required gate uses its Pkg.test command.
No full C suite was run. The export check built the shared object used by all tests.

## Sources pending

No new mathematical source is pending. FLINT numerical copy/containment conventions are on disk:
refs/src/flint-3.0.1/acb.rst:124-126,302-304,374-380; arb.rst:6-26,382-388,660-680.
Julia 1.12.5 help: help.log:14-15 (preserve), 56 (finalizers), 90-101 (precision), 272-278 (literal powers).
The earlier Julia references in lanes/j-slice1/help.log and lanes/j-slice2/help.log remain applicable.

Inherited pending item: [source pending: glibc dlsym search order through a handle's dependencies].
It applies to fixtures and the numerical copy symbol; resolution was observed in the passing gates.
The linked FLINT threadsafe configuration and FLINT malloc-failure documentation remain earlier deferred
source obligations. No wider thread or allocation-failure guarantee is claimed here.

## Findings against the specification

None. The invalid 2^-10 example concerns the brief's text example, not SPEC.
The eager default-limit call was a binding defect and was fixed.

## Cleanup

Deleted build/, the lane depot and temporary directory, and julia/Adelefeld/Manifest.toml.
Scratch copies were already removed. The cleanup checked all five authorized generated paths: 0 remain.
The shared object must be rebuilt with the export command before repeating the gate.
