# Lane m1-review-local report

BLOCKER FOUND: 1 BLOCKER, 2 MAJOR, 1 MINOR.

- R1 MAJOR: all five cap functions reject valid local inputs with undocumented ADF_UNSUPPORTED.
- R2 BLOCKER: is_canonical's explicitly unrestricted field contract permits an input causing an invalid read.
- R3 MAJOR: ADF_CHECK_INVARIANTS does not check local negation on entry or abort as documented.
- R4 MINOR: tests/test_fball.c retains an invalid local fixture for canonicalise, outside that function's domain.

R2 depends on the literal promise "whatever the fields hold" in the predicate's public header.
No wrong enclosure or memory error was found on values produced by the public constructors.

## What was done

Read CLAUDE.md, the review briefs, the relevant specification and plan sections, conventions and proofs.
Reviewed src/fball_local.c, the local paths of src/fball.c, src/cap.c, the local tests and Python reference.
Read the local, cap and original fball lane reports. Examined the four test replacements at merge def075f.
Built only under the owned review directory. No production, specification, tracker or git state was changed.

The independent Fraction oracle checked 40,208 requests with 0 mismatches in normal and sanitizer builds.
It covers tightness, raw and canonical forms, context choice, aliases, predicates and conversion statuses.
The cap checks include 20 chains of 50 operations, containment at each step, the radius invariant and idempotence.
The project reference matched 2,100 separately checked binary operations. Valgrind checked 900 requests.
Reproduced all four findings. Isolated the i <= k mutant in a review-directory source copy and killed it with ASan.
The production loop uses i < k and did not fail on the same input.

## Files written

- docs/reviews/m1/local/review.md: the complete review, evidence, coverage and limits.
- docs/reviews/m1/local/checks/probe.c: public-interface driver; R2 with argument bad-context.
- docs/reviews/m1/local/checks/oracle.py: independent integer/Fraction oracle and reference comparison.
- docs/reviews/m1/local/checks/cap_local.c: R1.
- docs/reviews/m1/local/checks/invariant_check.c: R3.
- docs/reviews/m1/local/checks/invalid_fixture.c: R4.
- docs/reviews/m1/local/checks/mutant.py and mutant.input: isolated mutation and its input.
- docs/reviews/m1/local/checks/memory.sh and run.sh: Valgrind wrapper and reproducible command recipe.
- docs/reviews/m1/local/checks/qa.py: source hashes, Python and shell syntax, and Markdown line-length checks.
- docs/reviews/m1/local/checks/reviewed.sha256: hashes of the ten inspected code/header/test files.
- docs/reviews/m1/local/checks/.gitignore: excludes generated libraries, objects and executables.
- Logs under checks/: build.log, san.log, san-noleak.log, oracle.log, oracle-san.log, reference.log,
  cap_local.log, bad-context.log, invariants-build.log, invariant_check.log, invalid_fixture.log,
  memory.log, valgrind.log, mutant.log, probe-san.log, probe-mutant.log, and qa.log.
- Generated scratch objects/libraries under checks/build, checks/san and checks/invariants; probe, probe-san,
  probe-mutant, cap_local, invariant_check, invalid_fixture, fball-mutant.c and fball-mutant.o under checks/.
  These are ignored build artifacts. The source scripts rebuild them without touching production files.
- lanes/m1-review-local/report.md: this report.

## Commands and results

Commands ran from the repository root. D below abbreviates docs/reviews/m1/local/checks.
Compilation and execution commands are shown with that abbreviation; logs retain their outputs.
No test expectation was weakened. Findings' reproducer programs print the observed contract violations.

Build:

```sh
D=docs/reviews/m1/local/checks
make -j2 BUILD=$D/build all
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude $D/probe.c \
    $D/build/libadelefeld.a -lflint -lgmp -lm -o $D/probe
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude $D/cap_local.c \
    $D/build/libadelefeld.a -lflint -lgmp -lm -o $D/cap_local
```

All three commands: exit 0. Build output is in checks/build.log.

```sh
$D/cap_local
```

Exit 0. R1: expected local statuses 0,0,0,0,0; observed 8,8,8,8,8; sentinel unchanged 5/5.
After explicit global conversion: statuses 0,0,0,0,0. See checks/cap_local.log.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 $D/oracle.py
PYTHONDONTWRITEBYTECODE=1 python3 $D/oracle.py --reference
```

Both exit 0. C oracle: 40,208 requests, 0 mismatches; 12,288 exhaustive block-4 arithmetic/alias cases;
5,896 requests each for add, sub and mul; 2,120 six-result predicate requests; 1,000 capped-chain steps.
Reference comparison: 2,100 cases, 0 mismatches. See checks/oracle.log and checks/reference.log.

```sh
make -j2 BUILD=$D/san SAN=1 check
```

Exit 2. All 30 programs report 0 failed checks, but all 30 terminate with a LeakSanitizer tracing error.
The diagnostic says LeakSanitizer cannot work under ptrace. This is not counted as a successful sanitizer run.
See checks/san.log. The initial oracle process overlapped this build; aggregate two-core use was not enforced
for that overlap. Subsequent builds and substantive checks were kept within two concurrent CPU workers.

```sh
ASAN_OPTIONS=detect_leaks=0 make -j2 BUILD=$D/san SAN=1 check
```

Exit 0. 30 programs, 348 tests, 1,200,445 checks, 0 failed checks, 0 failed tests.
ASan and UBSan remain enabled. Leak coverage is supplied separately by Valgrind.
The two test_dlopen tests perform 0 checks: build/libadelefeld.so is absent. No shared library was built elsewhere.
The following counts are included in the total, not additional runs:

| Program | Tests | Checks | Failed |
|---|---:|---:|---:|
| test_fball | 20 | 410 | 0 |
| test_fball_local | 25 | 20,554 | 0 |
| test_fball_local_vectors | 1 | 42,682 | 0 |
| test_cap | 28 | 874 | 0 |
| test_cap_vectors | 5 | 49,930 | 0 |
| test_cap_chain | 3 | 320,975 | 0 |

See checks/san-noleak.log.

```sh
cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinclude $D/probe.c $D/san/libadelefeld.a \
    -lflint -lgmp -lm -o $D/probe-san
ASAN_OPTIONS=detect_leaks=0 PYTHONDONTWRITEBYTECODE=1 \
    python3 $D/oracle.py --probe $D/probe-san
ASAN_OPTIONS=detect_leaks=0 $D/probe-san bad-context
```

Compilation: exit 0, run twice. Oracle: exit 0, 40,208 requests, 0 mismatches, 0 sanitizer diagnostics.
R2: exit 1, eight-byte heap-buffer-overflow in adf_modctx_nblocks from adf_fball_is_canonical.
The malformed-pointer probe was run first with H=0, then with H=4; both fail identically.
The retained source and checks/bad-context.log use H=4. The change affects only the bad-context branch.
See also checks/oracle-san.log.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 $D/oracle.py --mode small --probe $D/memory.sh
```

Exit 0. The wrapper runs:

```sh
valgrind --error-exitcode=99 --leak-check=full --show-leak-kinds=all \
    --log-file=docs/reviews/m1/local/checks/valgrind.log docs/reviews/m1/local/checks/probe
```

900 requests, 0 mismatches, 0 Valgrind errors. 13,829 allocations and 13,829 frees.
2,393,360 bytes allocated in total; 0 bytes in 0 blocks live at exit. See checks/memory.log and valgrind.log.

```sh
make -j2 BUILD=$D/invariants CPPFLAGS='-Iinclude -DADF_CHECK_INVARIANTS' all
cc -std=c11 -O2 -g -Wall -Wextra -Werror -DADF_CHECK_INVARIANTS -Iinclude \
    $D/invariant_check.c $D/invariants/libadelefeld.a -lflint -lgmp -lm -o $D/invariant_check
$D/invariant_check
```

All exit 0. R3: negation returned, output canonical=0. Expected an abort under the documented debug contract.
See checks/invariants-build.log and invariant_check.log.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude $D/invalid_fixture.c \
    $D/build/libadelefeld.a -lflint -lgmp -lm -o $D/invalid_fixture
$D/invalid_fixture
```

Both exit 0. R4: input satisfies L=0; canonicalise status=0; original test assertions pass=1.
See checks/invalid_fixture.log.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 $D/mutant.py
```

Exit 0. The script compiles only the copied fball.c and links it with the sanitizer archive; both cc commands
exit 0. Its exact compiler argument lists are in the script. On mutant.input, the original probe exits 0;
the i <= k mutant exits 1 with an eight-byte heap-buffer-overflow at adf_modctx_block, called from the product
loop. See checks/mutant.log, probe-san.log and probe-mutant.log. No production file was mutated.

Inspection and artifact checks:

```sh
git log -p --follow --max-count=3 -- tests/test_fball.c
git show def075f --format=short --first-parent -- tests/test_fball.c
sh -n docs/reviews/m1/local/checks/run.sh docs/reviews/m1/local/checks/memory.sh
GIT_OPTIONAL_LOCKS=0 git status --short
```

All exit 0. The merge changes four local tests and preserves valid global-input assertions. R4 is unchanged.
The sh invocation checks run.sh syntax; memory.sh is also checked separately in checks/qa.log.
Read-only git status lists the untracked reviews directory, lane.log and this report; 0 tracked changes.
The aggregate run.sh recipe was not rerun end to end; its substantive commands were executed individually above.
Final artifact check:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 docs/reviews/m1/local/checks/qa.py
```

Exit 0. Two Markdown files, 0 lines above 116 characters; 3 Python files parsed; 2 shell scripts, 0 syntax errors;
10 reviewed source hashes checked, 0 changes. See checks/qa.log.

## What is not done

No production fixes; this is a review lane. No benchmark, concurrency test, allocation-failure test or broad
mutation campaign. No independent reimplementation of FLINT itself. No shared-library/dlopen validation.
LeakSanitizer could not run in this environment. The Python reference comparison covers binary operations only;
the C oracle also covers conversion, predicates and scalars. Files outside the assigned scope were not reviewed
merely because the full test suite executed them.

## Sources pending

No new source is pending for the findings. The review gives the local refs/ source anchors checked.
Inherited and not resolved: [source pending: a Haar-measure reference stating local scaling],
from docs/proofs/catalogue.md:209-210 and include/adelefeld/fball.h:179-182.

## Findings against the specification

No counterexample to a mathematical statement in docs/SPEC.md was found. R1 is an implementation restriction
absent from its contract. R2 concerns the predicate header's malformed-input promise. R3 concerns the documented
debug mode. R4 concerns a test fixture outside its function's domain. The specification was not edited.
