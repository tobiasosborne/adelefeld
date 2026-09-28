# Lane m1-review-text report

BLOCKER FOUND: 1 BLOCKER, 6 MAJOR, 3 MINOR.

R1 is a blocker under the literal `fball.h:93` promise that the validator accepts arbitrary fields.
It is an invalid read through a fabricated context pointer. No wrong enclosure or memory error on
canonical values with live storage was found. The full review explains this distinction.

## Findings

- R1 BLOCKER: arbitrary-fields canonicality validation dereferences an invalid context pointer; SIGSEGV.
- R2 MAJOR: a finite arb accepted by the public constructor makes the value printer abort; large exponents expand.
- R3 MAJOR: a default-valid input prints a decimal exponent beyond the default parser limit.
- R4 MAJOR: the hidden 18-digit exponent limit overrides max_exp10; the reference and a test preserve the defect.
- R5 MAJOR: 21 declared text functions are absent; context extraction also rejects valid nested dumps.
- R6 MAJOR: all 5 cap operations reject admitted local inputs with UNSUPPORTED.
- R7 MAJOR: ADF_CHECK_INVARIANTS does not provide the promised context-lifetime abort.
- R8 MINOR: adele.h names 2 nonexistent FLINT operations instead of describing conversion and rounding.
- R9 MINOR: accepted fuzz inputs only reach 5 precisions and 5 digits values, excluding prec=2 and digits=1.
- R10 MINOR: sentinel memcmp checks read uninitialized bytes in the adele test and fuzzer.

## Work and files written

Read CLAUDE.md, the review rules and text brief, SPEC 4.1-4.3 and 10, PLAN 5 and milestone 1,
conventions 8, 9, 11, 12 and relevant ownership clauses. Reviewed src/text.c, its four test files, its
fuzzer, all public headers, the 15 API choices, the reference grammar and golden-vector use.
Read the text and header lane reports and the related local, adele, context, scaled and memcheck findings.

Written: `docs/reviews/m1/text/review.md` and this report. Under `docs/reviews/m1/text/checks/`:

- build.sh and bridge.c: isolated library build and dyadic extraction adapter.
- oracle.py: independent Fraction enclosure, exactness and printing checks.
- differential.py: own grammar-aware generator and comparison to proto/text_grammar.py.
- boundaries.py: limits, fault precedence, hidden exponent restriction and fuzz parameter coverage.
- probe.c and resources.py: isolated crash, printing-cost, round-trip, nested dump and contract reproducers.
- headers.py, generated declarations.c and missing_link.c: declaration/export and link checks.
- mutants.py: recreate and compare the 10 proposed equivalent mutants in owned scratch files.
- local_print.c: 612 canonical local-printing cases from the input integers.
- sentinel_padding.c and fuzz_one.c: invoke the original test helpers on one byte under Valgrind.
- audit.py: Markdown line lengths, finding counts, script syntax and reviewed-file fingerprints.
- .gitignore: generated libraries, executables, objects, scratch sources and corpora are not deliverables.
- reviewed.sha256: fingerprints of the 18 reviewed source/header/API files.
- Logs: build, enclosure, printing, differential, boundaries, headers, resources, mutants, local, lifetime,
  invariants-build, plain-build, san-build, san-tests, lsan-unavailable, fuzz-build, fuzz, valgrind,
  valgrind-classify, sentinel_padding, fuzz_one, predicate-san, fingerprints and audit, each with suffix .log.

Generated build/, san/, invariants/, mutants/, corpus/ and artifacts/ remain under checks/ and are ignored.
No production file, existing test, header or specification was changed. No git mutation or tracker command ran.

## Commands and results

All commands ran from the repository root. In the commands below, the repeated path is abbreviated by:

```sh
d=docs/reviews/m1/text/checks
```

The first adapter link omitted archive objects unused by bridge.c. The initial enclosure and printing
commands each exited 1 before running a case: ctypes could not resolve adf_str_free. build.sh was corrected
to use whole-archive. A later adapter correction initialized sentinel storage before init and memcmp;
the differential test was rerun. Neither correction changed library code or expected answers.

```sh
sh "$d/build.sh"
```

Result: exit 0 on each of 3 runs. make uses -j2, an owned BUILD path and -fPIC. The script also links bridge.so.

```sh
python3 -B "$d/oracle.py" enclosure
python3 -B "$d/oracle.py" printing
python3 -B "$d/differential.py"
python3 -B "$d/boundaries.py"
python3 -B "$d/headers.py"
python3 -B "$d/mutants.py"
```

- Enclosure: initial adapter import failure described above; then 2 completed runs, exit 0 each.
  The final run covers both complex coordinates: 106156 cases, 0 failures, 3693 exact cases, 14.805 seconds.
  Counts: 100000 random texts, 6120 halfway/dyadic boundaries, 36 exponent-limit cases, 10000 complex cases.
  First completed run inspected only the real coordinate of its complex texts; the final run corrects that.
- Printing: initial adapter import failure; completed run exit 0, 12000 nonzero balls plus 1000000 zero balls,
  0 failures, 3.406 seconds. The zero cases cover digits=1..1000000 individually.
- Differential: 2 completed runs, exit 0 each. Each: 100000 texts, 400000 typed calls, 0 disagreements.
  Final: 18.753 seconds. Typed results: OK 12689, LIMIT 12735, DOMAIN 2382, PARSE 372194.
  Classification: 57726 kinds, 39834 PARSE, 2440 LIMIT. Failed calls preserve the sentinel or kind output.
- Boundaries: exit 0; 88 checks agree with expected statuses. R4 separately records C=10 and reference=!LIMIT
  for an exponent allowed by max_exp10. Fuzz precision set 11/12/15/34/42; digits set 3/10/11/12/14.
- Headers: exit 0 for the checking script; 193 declared, 172 exported, 21 missing. Its generated application
  fails to link, exit 1, undefined reference adf_rat_dump_str. arb_add_fmpq and arb_mul_fmpq are both absent.
- Mutants: exit 0; 10 builds, 1011 parser + 1011 classifier + 270 printer comparisons per mutant.
  Total 22920 comparisons, 0 differences. Each equivalence is also argued in the review.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$d/probe.c" \
  "$d/build/libadelefeld.a" -lflint -lgmp -lm -o "$d/probe"
python3 -B "$d/resources.py"
```

Probe compilation ran twice, exit 0. resources.py: exit 0; 17 child results recorded.
Each child is bounded to 256 MiB address space and 20 CPU seconds; core dumps are disabled.

- Exact 2^e, e=100000/1000000/10000000: exit 0; 0.015/0.273/4.821 seconds;
  peak 9256/9260/18924 KiB. e=100000000: -9 at 20.009 seconds, peak 129668 KiB.
- e=1073741824: -6 allocation abort at 0.581 seconds, peak 136320 KiB.
- e=-100000/-1000000/-10000000: exit 0; 0.017/0.280/2.042 seconds;
  peak 9260/9264/34108 KiB. e=-100000000: -6 allocation abort, peak 166312 KiB.
- e=-1073741824: -6 allocation abort, peak 136320 KiB.
- e=18446744073709551616: -6 explicit printer abort; constructor=0, finite=1, canonical=1.
- 4 predicate modes: all -11, from the deliberately invalid context pointer admitted by the fball comment.
- Round-trip mode: `(9.99e100000 ; 0)` input status 0; printed `(1e100001 +/- 1.1e99998 ; 0)`;
  default reread status 10.
- Nested mode: valid fball dump returns 9; output remains NULL.

```sh
python3 -B -c 'import sys; sys.path.insert(0,"proto"); import text_grammar as t;
print(t.dump_roundtrip("adf1 Q fball l 1 6 2 2 3 0 0"))'
```

Result: exit 0, unchanged `adf1 Q fball l 1 6 2 2 3 0 0`.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude "$d/local_print.c" \
  "$d/build/libadelefeld.a" -lflint -lgmp -lm -o "$d/local_print"
"$d/probe" local
"$d/local_print"
make -j2 BUILD="$d/invariants" CPPFLAGS='-Iinclude -DADF_CHECK_INVARIANTS' all
cc -std=c11 -O2 -g -DADF_CHECK_INVARIANTS -Iinclude "$d/probe.c" \
  "$d/invariants/libadelefeld.a" -lflint -lgmp -lm -o "$d/probe-invariants"
"$d/probe-invariants" lifetime
```

All exit 0. Local input canonical=1; text `(* ; 1 mod 6)`; 5 cap statuses=8;
scaled conversion status=0, lost=0. Local printing: 612 cases, 0 failures.
Lifetime: `freed_borrowed_context_without_abort=1` in a library built with the debug define.

```sh
make -j2 BUILD="$d/san" SAN=1 "$d/san/test_text_rat" "$d/san/test_text_fball" \
  "$d/san/test_text_adele" "$d/san/test_text_classify"
for t in rat fball adele classify; do "$d/san/test_text_$t" || exit; done
for t in rat fball adele classify; do
  ASAN_OPTIONS=detect_leaks=0 "$d/san/test_text_$t" || exit
done
```

Build exit 0. First loop exits 1: rat passes 18122 checks, then LeakSanitizer reports that it cannot run
under ptrace. Second loop exits 0: 51 tests, 144843 checks, 0 failed checks, 0 ASan/UBSan reports.
Breakdown: rat 13/18122, fball 10/21545, adele 20/84400, classify 8/20776 (tests/checks).

```sh
cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude \
  "$d/probe.c" "$d/san/libadelefeld.a" -lflint -lgmp -lm -o "$d/probe-san"
ASAN_OPTIONS=detect_leaks=0 "$d/probe-san" predicate 0
```

Compile exit 0; probe exit 1. UBSan: misaligned context member access. ASan: invalid READ at address 9,
src/modctx.c:410, called by src/fball.c:358. This is R1.

```sh
make -j2 BUILD="$d/san" SAN=1 fuzz-build FUZZ_TARGET=text
mkdir -p "$d/corpus" "$d/artifacts"
cp tests/fuzz/corpus/text/* "$d/corpus/"
ASAN_OPTIONS=detect_leaks=0 "$d/san/fuzz/text" "$d/corpus" -max_total_time=20 \
  -max_len=4096 -workers=1 -rss_limit_mb=512 -print_final_stats=1 -artifact_prefix="$d/artifacts/"
```

Build and run exit 0. 500269 executions in 21 seconds; 0 crashes; peak RSS 279 MiB.
Only the owned corpus receives generated inputs. The fuzzer parameter restriction is R9.

```sh
make -j1 BUILD="$d/build" CFLAGS='-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fPIC' \
  "$d/build/test_text_rat" "$d/build/test_text_fball" \
  "$d/build/test_text_adele" "$d/build/test_text_classify"
for t in rat fball adele classify; do
  valgrind --error-exitcode=97 --leak-check=full --show-leak-kinds=definite,indirect \
    --errors-for-leak-kinds=definite,indirect "$d/build/test_text_$t" || exit
done
valgrind --error-exitcode=97 --leak-check=full --show-leak-kinds=definite,indirect \
  --errors-for-leak-kinds=definite,indirect "$d/build/test_text_classify"
```

Build exit 0. Loop: rat exit 0, fball exit 0, adele exit 97; it stops before classify.
Separate classify run exits 0. Rat/fball/classify: 0 errors. The adele test: 2748 errors from 71 contexts (R10).
All four tests pass their value assertions. Definite/indirect lost bytes: 0 for each program.
Possibly lost bytes: rat 135872, fball 139304, adele 135648, classify 0; not resolved in this review.

```sh
cc -std=c11 -O2 -g -Iinclude -Itests "$d/sentinel_padding.c" "$d/build/support/"*.o \
  "$d/build/libadelefeld.a" -lflint -lgmp -lm -o "$d/sentinel_padding"
valgrind --error-exitcode=97 --track-origins=yes --leak-check=full "$d/sentinel_padding"
cc -std=c11 -O2 -g -Iinclude -Itests "$d/fuzz_one.c" "$d/build/libadelefeld.a" \
  -lflint -lgmp -lm -o "$d/fuzz_one"
valgrind --error-exitcode=97 --track-origins=yes --leak-check=full "$d/fuzz_one"
```

Both compiles exit 0. Both memory checks exit 97. Sentinel helper: 2 errors, 1 context, parse status 9.
Fuzzer: 7 errors, 3 contexts. Both: uninitialized stack origins, 0 allocated bytes remaining at exit.
They invoke the original code, not a rewritten approximation of the failing comparison.

```sh
sha256sum src/text.c tests/test_text_*.c tests/fuzz/fuzz_text.c \
  include/adelefeld/*.h docs/api-m1.md > "$d/reviewed.sha256"
sha256sum --check "$d/reviewed.sha256"
python3 -B "$d/audit.py"
```

Fingerprint creation/check exit 0: 18 files. Initial inline Markdown/source line-length check: 0 overlong lines.
Final audit: exit 0; 2 documents, 0 overlong lines, finding counts 1/6/3, 7 Python scripts parsed,
1 shell script passes sh -n, 18 reviewed files unchanged, 0 failures.

## What is not done

No production fix, no full-library arithmetic review, no nonexistent dump implementation review.
No exhaustive nonzero printing over all digits or parsing over all positive slong precisions.
No unrestricted OOM experiment, no C++/Julia ABI rerun, no resolution of Valgrind's possibly lost blocks.
The no-crash fuzz result does not cover every precision or give an independent exact-input oracle.
Resource measurements are a bounded bracket, not a claim to locate the host's physical memory limit.

## Sources pending

- [source pending: exactness of mag_set_ui_2exp_si on every representable 30-bit mantissa].
  mag.rst:149 promises only an upper bound.
- [source pending: complete FLINT allocator-exhaustion behavior].
  flint.rst:121 and memory.rst:9 document the interface and wrappers only.
- [source pending: finite-result proof for the already-unverified claim in adele.h:26].

## Findings against the specification

No mathematical formula was refuted. The round-trip promise in PLAN:203 and conventions 9.6 needs a
limit qualification (R3). SPEC:761 needs a resource-failure decision for value printing (R2).
The arbitrary-fields predicate admission (R1) is in the header. The specification was not changed.
