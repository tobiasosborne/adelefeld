# Lane d-julia: report

## Result

Wrote docs/api-j.md, 445 lines. No authored line exceeds 116 characters.
It records J1-J3, chooses J4-J8, gives commented declarations for the first surface, maps the driver commands,
states lifecycle and context proofs, and divides J-a through J-e into smaller implementation steps.
All 44 current Julia fixture files are named. Three solver fixtures remain assigned to a J-S follow-up.

The heap-handle probe returned 0. It initialized, cleared and freed 100000 adf_rat values.
It made 200000 arithmetic calls and checked 100000 results against Rational{BigInt}.
After 40 GC calls the counted live handles and buffers were both 0.
This is an ownership and GC smoke probe, not an internal FLINT leak proof or a fuzzing run.

The temporary build/ directory and temporary Julia depot were deleted.
No production Julia package, C glue or C-library change was made. Pkg.test was not run.

## Decisions

- J4: aligned C heap storage owned by mutable Julia handles. It avoids depending on Julia object addresses.
  Inline allocation remains a performance alternative; no timing comparison was made.
- J5: status-returning try_ methods are primary. Constructors, operators and named methods throw AdfError.
  The error retains status, copied status text, place and useful output metadata.
- J6: values retain ModCtx objects and Julia-side leases. C clear precedes lease release.
  The last owner/lease releases the context. The proof does not depend on finalizer order.
- J7: initialized Adele/CAdele carriers provide arb/acb storage through their first members.
  This permits a standard-library-only package under J2. Their finite zero coordinate is private storage.
- J8: explicit precision keywords, default 64 bits; operators use that default. Printing defaults to 20 digits.
  There is no global precision setting.

The present core has no adf_status_count function. adf_status_str exposes eleven names and UNKNOWN at 11,
but cannot reveal a change to only the ADF_STATUS_COUNT macro. The design proposes that single core accessor
in a C-owned prerequisite slice. The literal J3 load gate needs it.

Plain using Adelefeld in this machine's ordinary, already-started Julia process cannot be promised under J1/J2.
The unpreloaded probe reproduces the missing GMP symbol. The proposed launcher sets LD_PRELOAD before Julia starts.
It does not inject a second GMP or restart a user's Julia process inside __init__.

## Files written

- docs/api-j.md.
- lanes/d-julia/probe.jl: representation, arithmetic, finalizers and ownership counts.
- lanes/d-julia/abi_probe.jl: independent literal ABI rows and status-name checks.
- lanes/d-julia/help.jl: installed Julia help capture using REPL's documentation renderer.
- lanes/d-julia/check_design.py: line limits, citation bounds and fixture inventory checks.
- lanes/d-julia/progress.md and lanes/d-julia/report.md.
- Logs in lanes/d-julia/: exports.log, probe.log, probe-unpreloaded.log, abi.log, help.log, help-first.log,
  help-second.log, environment.log, environment-writable-first.log, environment-writable.log, discovery.log,
  design-check-first.log, design-check.log and cleanup.log.

The lane brief and harness logs were not edited. No src/, include/, tests/ or other docs/ file was changed.
The explicitly requested build/ scratch tree is now absent.

## Checks and results

Commands ran from the repository root. Redirection targets are the logs named above.
Command lines are wrapped below for readability. Every test program and script ran under timeout.

1. Shared object and exports:

       timeout 600 sh tests/test_exports.sh

   Exit 0. 624 declared functions, 624 exported, 0 missing, 0 extra and 0 variadic declarations.
   The script used one compiler invocation. The shared object was removed after the probes.

2. Unpreloaded representation probe:

       timeout 30 julia --startup-file=no --threads=1 \
           lanes/d-julia/probe.jl build/libadelefeld.so

   Exit 1 at dlopen. Missing symbol: __gmpn_modexact_1_odd from libflint.so.18.
   This happened before the probe allocated any adf_rat storage.

3. Preloaded representation probe:

       timeout 180 env LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
           julia --startup-file=no --threads=1 lanes/d-julia/probe.jl build/libadelefeld.so

   Exit 0. Julia 1.12.5. Rat size 16, alignment 8.
   Init 100000; clear 100000; malloc 100000; free 100000.
   Aligned pointers 100000; add/mul calls 200000; canonical checks 100000.
   Independent exact equality checks 100000; C string allocations 100000; adf_str_free calls 100000.
   GC calls 40; repeated-finalize checks 10; live handles 0; live buffers 0.
   Arithmetic/GC loop elapsed 7.271 seconds. Ten batches reached matching init/clear/free totals.
   Inputs had 151-bit numerators and a 127-bit denominator; squared results exercise heap integers.

4. ABI/status probe:

       timeout 30 env LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10 \
           julia --startup-file=no --threads=1 lanes/d-julia/abi_probe.jl build/libadelefeld.so

   Exit 0. 19 rows, 38 size/alignment comparisons, 11 status-name comparisons, 2 UNKNOWN sentinels,
   1 version check, 2 scalar-width assertions. Effective count 11; exported count accessor absent.
   text_kind queries are present, size 4 and alignment 4. The old report's absence finding is stale.

5. Installed help capture:

       timeout 30 julia --startup-file=no --threads=1 lanes/d-julia/help.jl

   Three runs. First two exited 1 because Docs.doc methods had not been loaded.
   The final run imports REPL, exits 0, and records 10 help entries in 203 lines with Julia version 1.12.5.
   Failures are kept in help-first.log and help-second.log. The design quotes the final log.

6. Original-depot environment query:

       timeout 20 julia --startup-file=no --threads=1 -e \
           'using Libdl, Pkg; println(VERSION); Pkg.status();
            println(filter(x -> occursin("gmp", x), Libdl.dllist()))'

   Exit 1. VERSION printed 1.12.5. Pkg.status hit EROFS opening manifest_usage.toml.pid.
   No Pkg.status result was obtained.

7. Writable-depot environment query:

       timeout 20 env JULIA_DEPOT_PATH=/tmp/adelefeld-d-julia-depot:/home/tobias/.julia \
           JULIA_PKG_OFFLINE=true julia --startup-file=no --threads=1 \
           --project=/home/tobias/.julia/environments/v1.12 -e \
           'using Libdl, Pkg; println(VERSION); Pkg.status();
            println("Nemo=", Base.find_package("Nemo"));
            println(filter(x -> occursin("gmp", x), Libdl.dllist()))'

   Exit 124. Automatic standard-library precompilation began; Pkg.status did not finish.
   This attempt lacked JULIA_NUM_PRECOMPILE_TASKS. Default precompile concurrency was not bounded.
   That violated the lane's core-limit procedure. There were 0 peak-core measurements.
   No <=2 claim is made for this attempt. No package installation was requested.

8. Environment retry without compiled modules:

       timeout 60 env JULIA_DEPOT_PATH=/tmp/adelefeld-d-julia-depot:/home/tobias/.julia \
           JULIA_PKG_OFFLINE=true JULIA_NUM_PRECOMPILE_TASKS=1 \
           julia --startup-file=no --threads=1 --compiled-modules=no \
           --project=/home/tobias/.julia/environments/v1.12 -e \
           'using Libdl, Pkg; println(VERSION); Pkg.status();
            println("Nemo=", Base.find_package("Nemo"));
            println(filter(x -> occursin("gmp", x), Libdl.dllist()))'

   SIGTERM occurred at timeout. The exec session remained open and was explicitly interrupted.
   Final session exit 130. Pkg.status did not finish; this is not a successful environment check.
   Future slice test commands specify both one Julia thread and one precompile task.

9. Read-only package and loader discovery:

       timeout 20 julia --startup-file=no --threads=1 -e \
           'using Libdl; println("Julia=",VERSION); println("Nemo=",Base.find_package("Nemo"));
            println("FLINT_jll=",Base.find_package("FLINT_jll"));
            println("loaded_gmp=",filter(x -> occursin("gmp",x),Libdl.dllist()))'

   Exit 0. Julia=1.12.5; Nemo=nothing; FLINT_jll=nothing; one bundled GMP path.
   Reading v1.12 Project.toml found one direct dependency, PicoSAT. This is a file observation,
   not a successful Pkg.status run.

10. Static design check:

        timeout 20 python3 lanes/d-julia/check_design.py

    First run exited 1 for the nonexistent README line 74; corrected to its actual ending at 69.
    Later runs exited 0. Final: 445 design lines; 676 authored lines checked; 0 overlong lines;
    34 explicit-path citations and 19 bare-header citations in bounds; 44 fixtures named, 0 unnamed;
    9 sections; 3 source-pending markers; final section is the slices.
    Citation bounds do not prove that every associated claim follows from its source.

11. Preliminary width inspection used timeout 20 python3 -c with pathlib and len on design lines.
    It reported 434 lines/15 overlong lines, then 442 lines/2 overlong lines. Those lines were shortened.
    The reusable check above replaces those exploratory inspections.

12. Installed FLINT header inspection:

        timeout 20 rg -n '(PTHREAD|REENTRANT|TLS)' \
            /usr/include/flint/flint-config.h /usr/include/flint/flint.h

    Exit 0. FLINT_USES_PTHREAD=1, FLINT_USES_TLS=1, FLINT_REENTRANT undefined in the installed header.
    This does not prove the linked binary's full thread-safety contract. That gate remains pending.

13. Cleanup:

        timeout 20 rm -rf build /tmp/adelefeld-d-julia-depot

    Exit 0. A timeout 20 python3 -c pathlib assertion checked both paths.
    Exit 0; build_exists=0; temporary_depot_exists=0, in cleanup.log.
    Initial timeout 20 ls -ld inspection found build/ and docs/api-j.md absent before this work.

Setup tried timeout 20 command -v julia, exit 127 because command is a shell builtin.
The corrected timeout 20 bash -c 'command -v julia' found /home/tobias/.juliaup/bin/julia.
Source and fixture reads were bounded by timeout 20. No git or bd command was run.

## Not done and unsettled

- No julia/Adelefeld package, Pkg.test run, or production binding implementation.
- No C accessor implementation; J-status belongs to a C-owned lane.
- No context/finalizer-order, numeric-carrier arithmetic, threaded or Nemo bridge probe.
  Those are explicit acceptance obligations. Only Rat ownership and the ABI/status table were executed.
- No internal FLINT allocator instrumentation, RSS leak inference, valgrind run or inline-storage benchmark.
- No automatic artifact build, private linker namespace design, host cleanup policy or other-platform test.
- Full solver input adapters need J-S design before implementation. Complete driver parity is not claimed.

## Sources pending

1. Verification of the linked FLINT build's threadsafe configuration. The installed-header observation
   does not close this. Local cache/cleanup documentation: refs/src/flint-3.0.1/memory.rst:29-44.
2. Installed Julia extension Project.toml trigger/weakdep documentation. get_extension help alone
   does not establish extension manifest syntax or compatibility bounds.
3. Nemo source and version, fmpq/arb/acb payload layouts, pointer conversions, ownership and finalizers.
   Matching loaded FLINT/GMP identity and allocator configuration must precede direct pointer passing.

## Findings against the specification

0 counterexamples found to the SPEC sections used.
The status-count accessor gap is a J3 implementation dependency, not a proposed SPEC change.
No specification, mathematical statement or existing test was weakened.

