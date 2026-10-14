# Lane j-slice4: report

Step J-a.4 is built. The final solo Pkg.test passes 4881/4881: J-a.1 904, J-a.2 1015, J-a.3 1649,
J-a.4 1313. Baseline: 3568/3568. Ten of ten planted faults were caught by completed assertions.
Five dump lines and six reconstruction lines match the C driver, 11/11. The specification and design
were not changed. Generated build, depot, temporary files, Manifest and driver executable are removed.

Constraint caveat: the initial export build and direct driver compilation did not explicitly set
TMPDIR. The parent environment has TMPDIR unset (environment.log). I cannot certify that those two
compiler commands kept transient compiler files out of /tmp. No lane scratch file was deliberately
written there. All Julia gates, sessions and their fixture compilers use the lane's TMPDIR. The final
gate also has one unverified 180 s timeout before its passing, unchanged solo rerun.

The session prints:

    146
    SYMBOLS=102 LAYOUT_ROWS=6

Command: timeout 35 sh lanes/j-slice4/run-julia.sh lanes/j-slice4/session.jl.
It evaluates reconstruct(parse(FBall, "146 mod 1000"), Rat(100), Rat(200)). The requested [0,50]
returns NO_SOLUTION. Full-ball reconstruction uses 146+1000k; 22/7 is not in that progression.
The UX example is a partial-residue congruence, 7*146=1022=22 mod 1000. See finding 37 below.

## What was built

- TextKind::Int32: thirteen header constants, values 0..12. The literal size/alignment row 4/4 is
  gated against both C queries, and Julia's enum layout is checked. There is no C kind-name function;
  tests compare the enum names with literals cited from text.h:122-135.
- try_readvalue/readvalue: classify in C first, read kind only on OK, dispatch supported kinds to
  the four existing typed parsers. Unsupported kinds return UNSUPPORTED with details.kind. Classifier
  failures have no kind. Working precision is validated before defaults or classification, as finding
  32 requires. Later parser statuses and stages are retained. No Julia grammar is implemented.
- Numerical value_text/try_value_text retain digits validation in 1..1000000 before C. show reads
  :adf_digits from IOContext for Adele, CAdele, RealBall and ComplexBall. Defaults and carrier frame
  removal stay unchanged. Rat/FBall keep their exact printers without a digits keyword, as finding 5
  and step J-a.2 require; IOContext cannot round their exact coordinates.
- try_dump_text/dump_text and try_load_dump/load_dump for Rat, FBall, Adele and CAdele. Dump strings
  are copied at their byte length and freed in a finally, including exceptional copies/refusals.
  Loaders pass ctx=NULL and TextLimits, construct no context, and clear failed temporary owners.
  A global value has no context occurrence (dump.h:38-47,116,144-162). Local dumps get DOMAIN for NULL.
- try_reconstruct/reconstruct on FBall with two Rat end points, and on Adele with exact real-ball end
  points. Only OK returns a fresh Rat. Every failure has no value/place/auxiliary output, as recon.h
  says. No partial-data API or search limit is added. Closed accepted inputs refuse before allocation.
- Thirteen new required symbols, from 89 to 102; one layout row, from five to six. No production C glue,
  second production library handle, context, scaled/cap or local-value surface.

## Files written

- New: julia/Adelefeld/src/text.jl, dump.jl, recon.jl.
- Changed: src/Adelefeld.jl (exports/includes), load.jl (symbols/layout), status.jl (enum/decoder and
  text-kind ABI-error message), adele.jl and carrier.jl (IOContext printing).
- Appended: test/runtests.jl, one outer testset "Adelefeld step J-a.4"; test/fixtures/fault.c,
  enum layout faults, counted calls and defensive dumper modes.
- Appended: docs/api-ja.md, Step J-a.4, statements S28-S34 and DESIGN-FINDINGs 33-39.
- lanes/j-slice4/: gate.sh, run-julia.sh, help.jl, faults.py, driver.cmd, driver_compare.jl,
  verify_driver.py, session.jl, check_files.py, wrap_logs.py, cleanup.py, redgreen.md, retained logs,
  and this report. The compiled lane driver was removed.

Initial Rat/FBall digits-keyword changes were restored to their original forms before the final gate.
No retained edit is intended in those two source files. Earlier testsets and fixture branches remain
unchanged. No C production source/header, SPEC, PLAN, api-j.md or tests/test_julia.sh was changed.
No git command, bd, system package installation or sub-agent was used.

## Checks and commands

Every gate command is timeout 200 sh lanes/j-slice4/gate.sh, redirected to the named log. The gate has
an internal timeout 180, JULIA_NUM_THREADS=1, JULIA_NUM_PRECOMPILE_TASKS=1, the system GMP preload,
JULIA_PKG_OFFLINE=true, and JULIA_DEPOT_PATH=<lane>/depot:$HOME/.julia: with the trailing colon.
TMPDIR points under the lane. Mutants ran at most two concurrently, each with one Julia thread.

| Command or log | Exit | Result |
|---|---:|---|
| timeout 600 sh tests/test_exports.sh; exports.log | 0 | 625/625 exported; 0 missing, extra, variadic |
| gate; before.log | 0 | 904+1015+1649 = 3568/3568 |
| gate; red.log | 1 | old 3568 pass; new 26 pass, 15 fail, 39 error / 80 |
| gate; green1.log | 1 | old 3568 pass; new 1341 pass, 3 fail / 1344 |
| gate; green2.log | 0 | old 3568 pass; new 1364/1364; total 4932 |
| gate; final.log | 124 | 180 s timeout, no completed summary; unverified |
| gate; final2.log | 0 | 904+1015+1649+1313 = 4881/4881 |
| timeout 35 sh run-julia.sh help.jl | 0 | installed Julia 1.12.5 help, six entries |
| timeout 35 sh run-julia.sh driver_compare.jl, first | 0 | 11 value lines plus 3 stderr diagnostics |
| first inline Python comparison, timeout 20 | 1 | 11 C lines vs 14 combined Julia lines |
| timeout 35 sh run-julia.sh driver_compare.jl, stdout/stderr separate | 0 | 11 value lines |
| timeout 20 python3 lanes/j-slice4/verify_driver.py | 0 | 11/11 matches: 5 dump, 6 reconstruction |
| timeout 35 sh run-julia.sh session.jl | 0 | 146; 102 symbols; 6 layout rows |
| timeout 20 python3 lanes/j-slice4/check_files.py, first | 0 | 17 regions, 0 violations |
| same audit before report; files-before-report.log | 0 | 19 regions, 0 violations |
| timeout 20 python3 lanes/j-slice4/wrap_logs.py | 0 | 28 logs checked, 17 wrapped |
| timeout 30 python3 lanes/j-slice4/cleanup.py | 0 | 5 targets removed; 0 scratch, 0 remaining |
| timeout 20 python3 lanes/j-slice4/check_files.py, after report | 0 | 20 regions, 0 violations |

run-julia.sh in that table is lanes/j-slice4/run-julia.sh; its Julia file arguments are under the lane.
Every Julia process uses the same lane depot/temp, offline mode and GMP preload as the gate.
The driver was built with timeout 35 cc -std=c11 -O1 -Iinclude tools/adf/adf.c -Lbuild -ladelefeld
-lflint -lgmp -lm -Wl,-rpath,'$ORIGIN/../../build' -o lanes/j-slice4/adf: exit 0.
timeout 30 lanes/j-slice4/adf < lanes/j-slice4/driver.cmd: exit 1, as expected for its three refused
reconstruction lines; all 11 expected output lines were retained in driver.log.

The first comparison merged Julia's precompile stderr with stdout. Its 11 value lines already matched;
the exact comparison failed on the three diagnostics. driver-julia-first.log retains it. The rerun kept
the streams separate and checked exact equality without filtering value lines. No assertion was weakened.
The TMPDIR inspection, timeout 20 printenv TMPDIR, returned 1; the explicit shell inspection returned 0
and wrote TMPDIR=unset to environment.log. This concerns the two initial compiler commands above.

The final gate times are 22.9 s, 12.2 s, 21.2 s and 11.7 s for the four outer testsets. Its counted-call
subprocess passes 43/43 additional checks, not included in 4881. It reports:

    DUMPS=16 FREES=16
    CONTEXTS=0
    DIGIT_ERRORS=64 PRINT_CALLS=0
    PREC_ERRORS=5 LIMIT_CALLS=0 CLASSIFY_CALLS=0
    LIVE=0

The automatic-GC subprocess makes 10000 dump/load round trips across the four types. Each is identical;
four forced collections leave ROUNDTRIPS=10000 LIVE=0 ERRORS=0, from BASELINE=0. Earlier step J-a.3
also prints all four of its type-specific LIVE=0 ERRORS=0 counters.

## Red-green and numerical evidence

Tests were appended and the failing red run completed before implementation. The three green1 failures
were oracle errors: a typed FBall/Adele/CAdele loader sees a rat body as PARSE before semantic checking.
The corrected tests use non-canonical bodies of their own requested types for DOMAIN; their common gcd
condition fails. Source: src/dump.c:1488-1489. Green2 then passes with counted fixtures added.

After green2, an identity-oracle self-check was added at default digits. The initial wider exact-printer
keyword was removed to preserve earlier finding 5 and step J-a.2. The tests now assert MethodError for
that keyword and unchanged exact printing; all four numerical printers still check digits before C.
This changes scope, not a mathematical statement or a C refusal. redgreen.md records both counts.

The final attempt ran alongside the small driver comparison and timed out. Its cause was not isolated.
The unchanged solo rerun passes within the same 180 s bound. No count or caught-fault claim is assigned
to the timed-out run. No test was disabled, bound enlarged or assertion reduced to obtain final2.

Coverage: all 13 text kinds, 9 unsupported dispatches, classification stage ordering, precision forwarding,
five SPEC 4.3 result texts as FBall, numerical digits and IOContext, 30 random dumps per type, five handwritten
driver dump oracles, exact byte-limit acceptance/refusal, malformed/version/field/domain refusals, local
context refusal, closed owners, unchanged inputs, and the full reconstruction statuses of recon.h.

For 146 mod 1000 on [0,50], kmin=ceil(-146/1000)=0 and kmax=floor(-96/1000)=-1: none.
For [100,200], kmin=kmax=0: 146. For 1 mod 2 on [0,10], the candidates 1,3,5,7,9 prove NOT_UNIQUE.
Singleton closed end points, negative/fractional progression points, exact finite points, empty intervals,
and the driver's full-adele 2/1/NO_SOLUTION examples are tested. LIMIT uses tiny canonical dumps with
positive/negative extreme midpoint and radius exponents. DOMAIN is the release-build non-finite courtesy
only, not a claim for canonical inputs or invariant builds. NOT_DETERMINED is explicitly excluded by
recon.h:25-27 and belongs to the separate partial-data API; no fictitious test/input is supplied for it.

## Fault table

Each command is timeout 180 python3 lanes/j-slice4/faults.py <id>. The runner copies the package under
the lane, mutates that copy, invokes a timeout-175 gate, retains fault-<id>.log, and removes the copy in
a finally. Production files are never faulted. Every child gate exits 1 with a completed J-a.4 summary;
the runner exits 0 after recording that caught fault. There are 0 timeouts or survivors in this table.

| Fault | J-a.4 pass/fail/error/total | Result |
|---|---|---|
| F1 read kind before checking classifier failure | 1319/2/1/1322 | caught |
| F2 FBall classification dispatched to Rat parser | 1350/3/4/1357 | caught |
| F3 dumper adf_str_free skipped | 1311/2/0/1313 | caught; counted frees 0 |
| F4 load limits replaced by NULL | 1172/141/0/1313 | caught |
| F5 Rat loader ctx non-NULL | 1308/5/0/1313 | caught by safe counted fixture |
| F6 NOT_UNIQUE converted to successful zero output | 1306/6/1/1313 | caught |
| F7 numerical digits check moved after C | 1311/2/0/1313 | caught by nonzero C call count |
| F8 identity test oracle uses value text | 1312/1/0/1313 | caught by same-text/different-balls pair |
| F9 text_kind C layout row not gated | 1307/6/0/1313 | caught |
| F10 classifier length counts characters | 1312/1/0/1313 | caught by UTF-8 byte limit |

F1/F2 ran before restoring the no-digits keyword decisions for exact types; their earlier counts are
retained. F3-F10 use the final 1313-check surface. Every earlier outer testset passes in every fault run.
F5 changes Rat only: the real Rat loader ignores ctx, so no forged pointer is dereferenced. The fixture
refuses non-NULL before forwarding, which measures the promised NULL argument. F7 supplies safe dummy
digits on invalid inputs before its moved check, keeping C within its digits precondition while proving
the wrapper entered C prematurely. F8 mutates the test oracle itself; its independent negative witness
prevents replacing exact identity with decimal equality.

## DESIGN-FINDINGs

33. No adf_text_kind_str exists. Literal header names/numbers, with only ADF_ stripped, are used.
34. Supported readvalue results keep their concrete type and classified kind; classifier failure has none.
35. Exact Rat/FBall printers retain the earlier no-digits decision. IOContext digits affects numerical types.
36. recon.h writes no failing output/auxiliary data. details is empty; NOT_DETERMINED is outside this API.
37. The requested [0,50] reconstruction has no full-ball candidate. The corrected session uses [100,200].
38. The driver dump path has no NULL/length guard. Julia's defensive LIMIT rule follows the design.
39. Global dumps need no context. Local NULL is DOMAIN. N-D25 load-limit/cap qualification remains in force.

Full statements, citations and decisions are appended in docs/api-ja.md, Step J-a.4. No finding changes
SPEC or the design. Earlier 32 findings are kept.

## What is not done

No contexts/bindings, scaled/cap, local constructors, partial residues/reconstruction, ideles/classes,
characters/functions, Nemo bridge, two-thread support, long differential fuzzing or full C-suite run.
No NOT_DETERMINED case exists for either reconstruction function in this lane. No canonical DOMAIN case
is claimed for reconstruction: the tested courtesy is outside the contract in the release build.
Random round trips and GC are bounded checks, not a FLINT leak proof or a fuzzing campaign.
tests/test_julia.sh was not changed or run. The intermittent final-gate timeout has no isolated cause.
The two initial compiler calls' transient temporary-directory use cannot be certified, as stated above.

## Sources pending

No new mathematical source is pending. FLINT dump serialization is quoted from
refs/src/flint-3.0.1/arb.rst:288-293. The non-finite test setter is documented at :241-243. Reconstruction
is the C header's full-ball progression, with the small-case calculations written above step by step.
Julia 1.12.5 help is in help.log: @enum :4-38; IOContext retrieval :57-58; other lifetime/string help is
in the earlier lane logs. Earlier pending items remain:

- [source pending: glibc dlsym search order through a handle's dependencies]. This applies to test fixtures.
- [source pending: verification of the linked FLINT build's threadsafe configuration]. One thread only.
- [source pending: FLINT 3.0.1 documentation of flint_malloc on allocation failure]. No new OOM claim.

## Findings against the specification

None. SPEC 9.2 distinguishes full balls from partial residues. The UX example, the brief's requested
NOT_DETERMINED case and its driver-dumper guard attribution differ from the C contracts, not from SPEC.

## Cleanup and final audit

cleanup.log records TARGETS=5 REMOVED=5 SCRATCH_REMOVED=0 REMAINING=0. Removed build/, the lane depot,
the lane tmp directory, julia/Adelefeld/Manifest.toml and the compiled driver. All ten fault scratch
packages had already been removed by their runners. Retained logs were wrapped at 116 characters without
discarding text; live stdout/stderr/lane harness transcripts are excluded. Source files end with newlines.
The post-report source audit is retained in files-final.log: 20 regions, 0 violations. Rebuild before rerunning
the gate, with TMPDIR explicitly directed under the recreated lane tmp directory.
